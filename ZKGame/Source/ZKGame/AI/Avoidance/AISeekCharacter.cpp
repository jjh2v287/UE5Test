// AISeekCharacter.cpp
#include "AISeekCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DrawDebugHelpers.h"

AAISeekCharacter::AAISeekCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    bIsMoving = false;
    
    // 캐릭터 무브먼트 컴포넌트 설정
    UCharacterMovementComponent* CharMovement = GetCharacterMovement();
    if (CharMovement)
    {
        CharMovement->bOrientRotationToMovement = true; // 이동 방향으로 회전
        CharMovement->RotationRate = FRotator(0.0f, 640.0f, 0.0f); // 회전 속도
        CharMovement->bUseControllerDesiredRotation = false;
        CharMovement->MaxWalkSpeed = MaxSpeed;
    }
}

void AAISeekCharacter::BeginPlay()
{
    Super::BeginPlay();
    
    // 초기 목표 위치를 현재 위치로 설정 (이동 방지)
    // TargetLocation = GetActorLocation();

    UCharacterMovementComponent* CharMovement = GetCharacterMovement();
    if (CharMovement)
    {
        // CharMovement->GravityScale = 0.0f;
        // CharMovement->BrakingFrictionFactor = 0.0f;
        // CharMovement->GroundFriction = 0.0f;
        // CharMovement->BrakingDecelerationWalking = 0.0f;
    }
}

void AAISeekCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    FVector CurrentLocationd =RootComponent->GetComponentLocation();
    FVector CurrentLocation = GetActorLocation();
    
    // 이동 중이면 Seek 동작 실행
    if (bIsMoving && !HasReachedTarget())
    {
        ExecuteSeekBehavior(DeltaTime);
    }
    else if (HasReachedTarget() && bIsMoving)
    {
        // 목표에 도달했으면 이동 중지
        bIsMoving = false;
        GetCharacterMovement()->StopMovementImmediately();
    }
    
    // 디버그 시각화
    if (bShowDebug)
    {
        DrawDebugSphere(GetWorld(), TargetLocation, ArrivalRadius, 12, FColor::Green, false, -1, 0, 1.0f);
        DrawDebugLine(GetWorld(), GetActorLocation(), TargetLocation, FColor::Red, false, -1, 0, 0.1f);
    }
}

void AAISeekCharacter::SetTargetLocation(FVector NewTargetLocation)
{
    TargetLocation = NewTargetLocation;
    bIsMoving = true; // 새로운 목표가 설정되면 이동 시작
}

bool AAISeekCharacter::HasReachedTarget() const
{
    float DistanceToTarget = FVector::Dist(GetActorLocation(), TargetLocation);
    return DistanceToTarget <= ArrivalRadius;
}

void AAISeekCharacter::ExecuteSeekBehavior(float DeltaTime)
{
    /*// 목표까지 방향 및 거리 계산
    FVector DirectionToTarget = TargetLocation - GetActorLocation();
    DirectionToTarget.Z = 0; // 평면 위에서만 고려
    
    float DistanceToGoal = DirectionToTarget.Size();
    
    // 목표 지점에 도달한 경우
    if (DistanceToGoal < 1.0f)
    {
        FVector Stop = -GetVelocity() * (1.0f / FMath::Max(DeltaTime, 0.1f)); // 정지력
        GetCharacterMovement()->AddInputVector(Stop);
        return;
    }
    
    // 정규화된 방향
    // FVector Direction = DirectionToTarget / DistanceToGoal;
    FVector Direction = DirectionToTarget.GetSafeNormal2D();;
    
    // 현재 전방 벡터
    FVector CurrentForward = GetActorForwardVector();
    
    // 방향에 따른 속도 스케일링 (전방 이동이 가장 빠름)
    float ForwardDot = FVector::DotProduct(CurrentForward, Direction);
    float SpeedScale = 0.5f + 0.5f * FMath::Max(0.0f, ForwardDot); // 0.5 ~ 1.0 범위
    
    // 목표 속도 계산
    FVector DesiredVelocity = Direction * MaxSpeed * SpeedScale;
    
    // 도착 시 감속
    float ArrivalDistance = 200.0f; // 감속 시작 거리
    if (DistanceToGoal < ArrivalDistance)
    {
        float SpeedFactor = FMath::Sqrt(DistanceToGoal / ArrivalDistance); // 감속 커브
        DesiredVelocity *= SpeedFactor;
    }
    
    // 조향력 계산 (목표 속도와 현재 속도의 차이)
    float SteerStrength = 1.0f / 0.3f; // 반응 시간의 역수
    FVector SteeringForce = (DesiredVelocity - GetVelocity()) * SteerStrength;
    
    GetCharacterMovement()->AddInputVector(SteeringForce.GetSafeNormal());*/

    //------------------------------------------------------------------------------------------------------------------
    
    // 현재 위치
    FVector CurrentLocation = GetActorLocation();
    
    // 목표까지의 방향 벡터 계산
    FVector DirectionToTarget = TargetLocation - CurrentLocation;
    float DistanceToGoal = DirectionToTarget.Size();
    
    // 방향 정규화
    if (DistanceToGoal > 0.0f)
    {
        DirectionToTarget /= DistanceToGoal;
    }
    
    // 현재 전방 벡터
    FVector CurrentForward = GetVelocity().GetSafeNormal2D();
    
    // 정규화된 방향
    FVector Direction = DirectionToTarget.GetSafeNormal2D();
    
    // 방향에 따른 속도 스케일링 (전방 이동이 가장 빠름)
    float ForwardDot = FVector::DotProduct(CurrentForward, Direction);
    float SpeedScale = 0.5f + 0.5f * FMath::Max(0.0f, ForwardDot); // 0.5 ~ 1.0 범위
    
    // 원하는 속도 계산 (방향 * 최대 속도)
    FVector DesiredVelocity = DirectionToTarget * MaxSpeed * SpeedScale;
    
    // 현재 속도
    FVector CurrentVelocity = GetVelocity();
    
    // 스티어링 힘 계산 (원하는 속도 - 현재 속도)
    FVector SteeringForce = DesiredVelocity - CurrentVelocity;
    
    float SteerStrength = 1.0f / 0.3f; // 반응 시간의 역수
    
    // 스티어링 힘 제한
    SteeringForce = SteeringForce.GetClampedToMaxSize(MaxSteeringForce * MaxSpeed * SteerStrength);
    
    // 스티어링 힘 제한
    float MaxForce = MaxSteeringForce * MaxSpeed;
    float SteeringMagnitude = FMath::Min(SteeringForce.Size(), MaxForce);

    // 힘의 크기에 비례한 입력 크기 계산 (0.0 ~ 1.0 사이로 스케일링)
    float InputScale = SteeringMagnitude / MaxForce;

    // 방향을 유지하면서 크기를 조절
    FVector MovementInput;
    if (SteeringForce.SizeSquared2D() > KINDA_SMALL_NUMBER)
    {
        MovementInput = SteeringForce.GetSafeNormal2D() * InputScale * 2;
    }
    else
    {
        MovementInput = FVector::ZeroVector;
    }

    // 캐릭터 무브먼트에 입력 적용
    // GetCharacterMovement()->AddInputVector(MovementInput);

    // void UCharacterMovementComponent::PhysWalking(
    // void UCharacterMovementComponent::MoveAlongFloor(
    // bool UMovementComponent::SafeMoveUpdatedComponent(
    // FORCEINLINE_DEBUGGABLE bool UMovementComponent::MoveUpdatedComponent(
    // bool UMovementComponent::MoveUpdatedComponentImpl(
    // FORCEINLINE_DEBUGGABLE bool USceneComponent::MoveComponent(
    // bool UPrimitiveComponent::MoveComponentImpl(
    // bool USceneComponent::InternalSetWorldLocationAndRotation(
    // SetRelativeLocation_Direct(NewLocation);
    // SetRelativeRotation_Direct(NewRelativeRotation);
    
    // void UCharacterMovementComponent::CalcVelocity(
    // bool UCharacterMovementComponent::ApplyRequestedMove(
    GetCharacterMovement()->RequestDirectMove(SteeringForce, false);
    // GetCharacterMovement()->RequestDirectMove(MovementInput, true)

    FVector StartLocation = GetActorLocation() + FVector(0,0, 50);
    DrawDebugDirectionalArrow(GetWorld(), StartLocation, StartLocation + GetVelocity(), 15, FColor::Black, false, -1, 0, 1.0f);
    StartLocation += FVector(0,0, 5);
    DrawDebugDirectionalArrow(GetWorld(), StartLocation, StartLocation + MovementInput, 15, FColor::Blue, false, -1, 0, 2.0f);
}