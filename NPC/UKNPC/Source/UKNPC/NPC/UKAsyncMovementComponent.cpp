// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKAsyncMovementComponent.h"
#include "NavigationSystem.h"
#include "GameFramework/Pawn.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/HitResult.h"
#include "NavMesh/RecastNavMesh.h"

UUKAsyncMovementComponent::UUKAsyncMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UUKAsyncMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	
	Mesh = PawnOwner->FindComponentByClass<USkeletalMeshComponent>();
	MovementMode = EMovementMode::MOVE_Walking;
}

// 1. AI의 이동 요청 수신
void UUKAsyncMovementComponent::RequestDirectMove(const FVector& MoveVelocity, bool bForceMaxSpeed)
{
	// AIController가 계산한 목표 속도(MoveVelocity)를 받아옵니다.
	// 이를 통해 별도의 AddInputVector 호출 없이도 AI가 의도한 방향을 알 수 있습니다.
	FVector MoveDirection = MoveVelocity.GetSafeNormal();
    
	// NPC의 이동 입력을 강제로 주입합니다.
	// (PawnMovementComponent의 내부 입력 벡터에 값을 넣습니다)
	AddInputVector(MoveDirection);
}

// 2. 매 프레임 이동 처리
void UUKAsyncMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UKAsyncMovement_Tick);
    if (!PawnOwner || !UpdatedComponent) return;

    // -------------------------------------------------------------
    // 1. 루트 모션 처리 (루트 모션은 애니메이션 동기화 때문에 동기 처리가 안전)
    // -------------------------------------------------------------
    const FRootMotionMovementParams TempRootMotionParams = Mesh ? Mesh->ConsumeRootMotion() : FRootMotionMovementParams();

    if (TempRootMotionParams.bHasRootMotion)
    {
        // 루트 모션은 복잡한 물리 처리 없이 즉시 적용 (기존 코드 유지)
        FTransform RootMotionDelta = TempRootMotionParams.GetRootMotionTransform();
        FQuat NewRotation = UpdatedComponent->GetComponentQuat() * RootMotionDelta.GetRotation();
        UpdatedComponent->SetWorldRotation(NewRotation);
        
        FVector MoveDelta = Mesh->GetComponentQuat().RotateVector(RootMotionDelta.GetTranslation());
        FHitResult Hit;
        SafeMoveUpdatedComponent(MoveDelta, NewRotation, true, Hit);
        if (Hit.IsValidBlockingHit())
        {
             SlideAlongSurface(MoveDelta, 1.f - Hit.Time, Hit.Normal, Hit, true);
        }
        return; // 루트 모션 중에는 일반 이동 로직 건너뜀
    }

    // -------------------------------------------------------------
    // 2. 비동기 이동 로직 (Async Physics Movement)
    // -------------------------------------------------------------
    
    // 이미 요청 중이라면 이번 틱은 스킵 (또는 로직에 따라 대기)
    if (bIsAsyncTracePending) return;

    // [1단계] 입력 및 이동 벡터 계산
    FVector InputVector = ConsumeInputVector();
    
    // 이동량이 거의 없으면 처리 중단
    if (InputVector.IsNearlyZero() && GravityVelocity.IsNearlyZero()) return;

    bool bIsGrounded = CurrentFloorNormal.Z > 0.7f;
    FVector DesiredMove = InputVector * 70.0f * DeltaTime;

    // 경사면 보정
    if (bIsGrounded)
    {
        DesiredMove = FVector::VectorPlaneProject(DesiredMove, CurrentFloorNormal);
    }

    // 중력 계산
    if (bIsGrounded)
    {
        GravityVelocity = FVector(0, 0, -500.0f); // Sticking Force
    }
    else
    {
        GravityVelocity += -(GetGravityDirection() * GetGravityZ() * DeltaTime);
        GravityVelocity = GravityVelocity.GetClampedToMaxSize(4000.f); // MaxFallSpeed
    }
    
    // 최종 이동 벡터
    FVector TotalMove = DesiredMove ;//+ (GravityVelocity * DeltaTime);

    // -------------------------------------------------------------
    // [핵심] 비동기 충돌 검사 요청 (Request Async Sweep)
    // SafeMove를 직접 호출하는 대신, 갈 수 있는지 비동기로 물어봅니다.
    // -------------------------------------------------------------
    
    UWorld* World = GetWorld();
    if (World)
    {
        FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SimpleMovementAsync), false, PawnOwner);
        FCollisionShape CollisionShape = UpdatedPrimitive->GetCollisionShape();
        
        // 데이터 저장 (콜백에서 사용하기 위함)
        PendingMoveData.DesiredDelta = TotalMove;
        PendingMoveData.DeltaTime = DeltaTime;
        PendingMoveData.bIsActive = true;
        bIsAsyncTracePending = true; // 락 걸기

        // 델리게이트 바인딩
        FTraceDelegate TraceDelegate;
        TraceDelegate.BindUObject(this, &UUKAsyncMovementComponent::OnAsyncSweepCompleted);

        // 비동기 스위프 요청 (현재 위치 -> 이동할 위치)
        // 주의: 1프레임 딜레이가 발생하므로 NPC 반응속도에 영향이 없는지 확인 필요
        LastTraceHandle = World->AsyncSweepByChannel(
            EAsyncTraceType::Single,
            UpdatedComponent->GetComponentLocation(),
            UpdatedComponent->GetComponentLocation() + TotalMove,
            UpdatedComponent->GetComponentQuat(),
            UpdatedComponent->GetCollisionObjectType(),
            CollisionShape,
            QueryParams,
            FCollisionResponseParams::DefaultResponseParam,
            &TraceDelegate
        );
    }
}

// -------------------------------------------------------------
// [핵심] 비동기 결과 처리 (Callback on Game Thread)
// -------------------------------------------------------------
void UUKAsyncMovementComponent::OnAsyncSweepCompleted(const FTraceHandle& Handle, FTraceDatum& Datum)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UKAsyncMovement_OnAsyncSweepCompleted);
    // 요청했던 핸들과 일치하는지 확인
    if (Handle != LastTraceHandle) return;

    bIsAsyncTracePending = false; // 락 해제
    if (!PendingMoveData.bIsActive || !UpdatedComponent) return;

    FVector MoveDelta = PendingMoveData.DesiredDelta;
    
    // 비동기 결과 확인
    // Datum.OutHits에 결과가 들어옵니다.
    bool bHit = false;
    FHitResult HitResult;

    if (Datum.OutHits.Num() > 0)
    {
        HitResult = Datum.OutHits[0];
        bHit = HitResult.bBlockingHit;
    }

    // [최적화 로직]
    if (!bHit)
    {
        // CASE A: 충돌 없음 (허공/평지)
        // SafeMoveUpdatedComponent의 무거운 연산 없이 좌표만 갱신합니다. (매우 빠름)
        // Sweep=false로 설정하여 내부 물리 검사를 생략합니다.
        UpdatedComponent->SetWorldLocation(UpdatedComponent->GetComponentLocation() + MoveDelta, false, nullptr, ETeleportType::None);
        
        // 바닥 정보 초기화 (공중일 수 있으므로)
        CurrentFloorNormal = FVector::UpVector;
    }
    else
    {
        // CASE B: 충돌 있음 (벽, 바닥, 경사로)
        // 어차피 부딪혔으므로 정밀한 처리를 위해 동기식 SafeMove + Slide 로직을 수행합니다.
        // 여기서 비용이 발생하지만, 허공을 걷는 대부분의 프레임에서는 비용이 절약됩니다.
        
        /*FHitResult MoveHit;
        SafeMoveUpdatedComponent(MoveDelta, UpdatedComponent->GetComponentRotation(), true, MoveHit);

        if (MoveHit.IsValidBlockingHit())
        {
            // 벽 타기 / 경사로 타기
            FVector SlopeMove = FVector::VectorPlaneProject(MoveDelta, MoveHit.Normal);
            // 속도 보존
            if (MoveHit.Normal.Z > 0.7f) // 경사로만 속도 보존
            {
                 SlopeMove = SlopeMove.GetSafeNormal() * MoveDelta.Size();
            }

            SlideAlongSurface(SlopeMove, 1.f - MoveHit.Time, MoveHit.Normal, MoveHit, true);
            
            // 바닥 정보 갱신
            if (MoveHit.Normal.Z > 0.7f)
            {
                CurrentFloorNormal = MoveHit.Normal;
                GravityVelocity = FVector::ZeroVector;
            }
        }*/
    }
    
    // 회전 처리는 여기서 수행 (혹은 Tick에서 별도로 수행)
    if (!MoveDelta.IsNearlyZero())
    {
        FRotator TargetRotation = MoveDelta.Rotation();
        TargetRotation.Pitch = 0.0f;
        TargetRotation.Roll = 0.0f;
        FRotator NewRot = FMath::RInterpTo(UpdatedComponent->GetComponentRotation(), TargetRotation, PendingMoveData.DeltaTime, 10.0f);
        UpdatedComponent->SetWorldRotation(NewRot);
    }

    PendingMoveData.bIsActive = false;
}