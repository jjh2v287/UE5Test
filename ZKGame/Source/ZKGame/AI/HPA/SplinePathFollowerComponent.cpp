#include "SplinePathFollowerComponent.h"

#include "UKNavigationManager.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h" // ACharacter 확인용
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h" // 수학 함수 사용

USplinePathFollowerComponent::USplinePathFollowerComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false; // 처음에는 Tick 비활성화

    // 내부 SplineComponent 생성
    SplineComp = CreateDefaultSubobject<USplineComponent>(TEXT("PathSplineComponent"));
    if (SplineComp)
    {
        SplineComp->SetMobility(EComponentMobility::Movable); // 동적으로 업데이트되므로 Movable로 설정
        SplineComp->SetDrawDebug(bDrawDebugSpline); // 디버그 드로잉 설정 초기화
    }
}

void USplinePathFollowerComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        UE_LOG(LogTemp, Error, TEXT("SplinePathFollowerComponent: OwnerActor is null!"));
        SetComponentTickEnabled(false);
        return;
    }

    // 소유 액터가 캐릭터인지 확인하고 무브먼트 컴포넌트 캐시
    ACharacter* OwnerCharacter = Cast<ACharacter>(OwnerActor);
    if (OwnerCharacter)
    {
        MovementComponent = OwnerCharacter->GetCharacterMovement();
    }

    // SplineComp가 Actor에 Attach되도록 설정
    if (SplineComp && !SplineComp->IsAttachedTo(OwnerActor->GetRootComponent()))
    {
        SplineComp->SetupAttachment(OwnerActor->GetRootComponent());
        SplineComp->RegisterComponent(); // 컴포넌트 등록
    }

    // 초기 디버그 드로잉 설정 적용
    if (SplineComp)
    {
        SplineComp->SetDrawDebug(bDrawDebugSpline);
    }
}

void USplinePathFollowerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 컴포넌트 정리 (필요한 경우)
    if (SplineComp)
    {
        SplineComp->DestroyComponent();
        SplineComp = nullptr;
    }

    Super::EndPlay(EndPlayReason);
}


void USplinePathFollowerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bIsMoving || !OwnerActor || !SplineComp || SplineComp->GetNumberOfSplinePoints() < 2)
    {
        return; // 이동 중이 아니거나 필요한 요소가 없으면 중단
    }

    // 1. 이번 프레임에 이동할 거리 계산
    CurrentDistance = SplineComp->GetDistanceAlongSplineAtLocation(OwnerActor.Get()->GetActorLocation(), ESplineCoordinateSpace::World);
    CurrentDistance += MovementSpeed * DeltaTime;

    // 2. 스플라인 총 길이 확인 및 완료/루프 처리
    const float TotalSplineLength = SplineComp->GetSplineLength();
    if (CurrentDistance >= TotalSplineLength)
    {
        if (bLoop)
        {
            // 루프: 거리를 처음으로 되돌림 (오버된 만큼 유지)
            CurrentDistance = FMath::Fmod(CurrentDistance, TotalSplineLength);
        }
        else
        {
            // 완료: 마지막 지점에 고정하고 이동 중지
            CurrentDistance = TotalSplineLength;
            StopMovement();
            // 여기에 이동 완료 이벤트 호출 등을 추가할 수 있습니다.
            // OwnerActor->SetActorLocation(SplineComp->GetLocationAtDistanceAlongSpline(CurrentDistance, ESplineCoordinateSpace::World)); // 정확한 마지막 위치로 설정 (선택 사항)
            return; // 이동 완료 시 Tick 종료
        }
    }

    // 3. 현재 거리(CurrentDistance)에 해당하는 스플라인 상의 목표 위치 및 방향 계산 (월드 좌표계 기준)
    const FVector TargetLocation = SplineComp->GetLocationAtDistanceAlongSpline(CurrentDistance, ESplineCoordinateSpace::World);
    const FRotator TargetRotation = SplineComp->GetRotationAtDistanceAlongSpline(CurrentDistance, ESplineCoordinateSpace::World);

    // 4. 액터 이동 처리
    const FVector CurrentLocation = OwnerActor->GetActorLocation();
    const FVector MoveDirection = (TargetLocation - CurrentLocation).GetSafeNormal(); // 현재 위치에서 목표 위치까지의 방향

    if (!MoveDirection.IsNearlyZero())
    {
        if (MovementComponent) // 캐릭터 무브먼트 컴포넌트 사용 (권장)
        {
            MovementComponent->AddInputVector(TargetLocation - CurrentLocation, true); // 계산된 방향으로 이동 입력 추가
            // 필요하다면 속도 직접 제어: MovementComponent->Velocity = MoveDirection * MovementSpeed;
        }
        else // 일반 액터의 경우 직접 위치 설정 (덜 부드러울 수 있음)
        {
             // 다음 틱에서의 예상 위치로 이동 (약간의 오차가 있을 수 있음)
             // 또는 FMath::VInterpTo 등을 사용하여 부드럽게 이동
             FVector NewLocation = FMath::VInterpTo(CurrentLocation, TargetLocation, DeltaTime, MovementSpeed / 50.0f); // Interp 속도 조절 필요
             OwnerActor->SetActorLocation(NewLocation);
        }
    }

    // 5. 액터 회전 처리 (옵션)
    if (bFollowSplineRotation)
    {
        const FRotator CurrentRotation = OwnerActor->GetActorRotation();
        // 목표 회전값으로 부드럽게 보간
        FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, RotationInterpSpeed);
        OwnerActor->SetActorRotation(NewRotation);
    }
}

void USplinePathFollowerComponent::SetPathPoints(const TArray<FVector>& NewPathPoints, ESplineCoordinateSpace::Type CoordinateSpace, bool bUpdateSpline)
{
    if (!SplineComp) return;

    InputPathPoints = NewPathPoints; // 원본 데이터 저장
    InputCoordinateSpace = CoordinateSpace;

    SplineComp->ClearSplinePoints(false); // 스플라인 업데이트는 나중에 한 번만 하도록 false 전달

    if (NewPathPoints.Num() >= 2)
    {
        for (int32 i = 0; i < NewPathPoints.Num(); ++i)
        {
            // 좌표계에 맞춰 FVector 생성
            // USplineComponent는 내부적으로 Local Space를 사용하므로, World 좌표계 입력 시 변환 필요
            const FVector PointLocation = (CoordinateSpace == ESplineCoordinateSpace::World)
                ? SplineComp->GetComponentTransform().InverseTransformPosition(NewPathPoints[i])
                : NewPathPoints[i];

            // 각 포인트를 SplineComponent에 추가 (기본적으로 부드러운 Curve 타입 사용)
            // 마지막 포인트는 다른 포인트와 동일한 타입 사용
            ESplinePointType::Type PointType = (i == NewPathPoints.Num() - 1 && i > 0)
                ? SplineComp->GetSplinePointType(i - 1)
                : ESplinePointType::Linear;

            // FSplinePoint 구조체 생성 및 추가 (Rotation, Scale 등은 기본값 사용)
            FSplinePoint SplinePoint(static_cast<float>(i), PointLocation); // InputKey는 인덱스를 사용 (0, 1, 2...)
            SplinePoint.Type = PointType;
            SplineComp->AddPoint(SplinePoint, false); // 여기서도 업데이트는 나중에
        }
    }

    if (bUpdateSpline)
    {
        SplineComp->UpdateSpline(); // 모든 포인트 추가 후 스플라인 최종 업데이트 (ReparamTable 등 계산)
    }

    ResetMovement(); // 경로가 변경되었으므로 이동 상태 초기화
    UE_LOG(LogTemp, Log, TEXT("SplinePathFollowerComponent: Path set with %d points."), NewPathPoints.Num());
}


void USplinePathFollowerComponent::StartMovement(FVector TargetLocation)
{
    const FVector StartLocation = GetOwner()->GetActorLocation();
    
    // 예시 경로 데이터 (월드 좌표계)
    TArray<FVector> MyPath = UUKNavigationManager::Get()->FindPath(StartLocation, TargetLocation);;

    // 경로 설정
    SetPathPoints(MyPath, ESplineCoordinateSpace::World);

    if (SplineComp && SplineComp->GetNumberOfSplinePoints() >= 2 && SplineComp->GetSplineLength() > 0)
    {
        bIsMoving = true;
        SetComponentTickEnabled(true); // Tick 활성화
        UE_LOG(LogTemp, Log, TEXT("SplinePathFollowerComponent: Movement started."));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("SplinePathFollowerComponent: Cannot start movement. Spline is not valid or has zero length."));
    }
}

void USplinePathFollowerComponent::StopMovement()
{
    bIsMoving = false;
    SetComponentTickEnabled(false); // Tick 비활성화

    // 캐릭터 이동 컴포넌트가 있다면 입력 중지
    if (MovementComponent)
    {
        // AddInputVector는 매 프레임 추가되므로 Tick 비활성화로 충분할 수 있음
        // 만약 Velocity를 직접 제어했다면 0으로 설정: MovementComponent->Velocity = FVector::ZeroVector;
    }
    UE_LOG(LogTemp, Log, TEXT("SplinePathFollowerComponent: Movement stopped."));
}

void USplinePathFollowerComponent::ResetMovement()
{
    CurrentDistance = 0.0f;
    StopMovement(); // 이동 중지

    // 액터를 스플라인 시작 위치로 이동 (선택 사항)
    if (OwnerActor && SplineComp && SplineComp->GetNumberOfSplinePoints() > 0)
    {
        const FVector StartLocation = SplineComp->GetLocationAtDistanceAlongSpline(0.0f, ESplineCoordinateSpace::World);
        OwnerActor->SetActorLocation(StartLocation);
        if (bFollowSplineRotation)
        {
             const FRotator StartRotation = SplineComp->GetRotationAtDistanceAlongSpline(0.0f, ESplineCoordinateSpace::World);
             OwnerActor->SetActorRotation(StartRotation);
        }
    }
    UE_LOG(LogTemp, Log, TEXT("SplinePathFollowerComponent: Movement reset to start."));
}
