// AvoidanceCharacter.cpp
#include "AvoidanceCharacter.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"

AAvoidanceCharacter::AAvoidanceCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    
    // CharacterMovementComponent 초기화
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 360.0f, 0.0f);
    GetCharacterMovement()->MaxWalkSpeed = MaxSpeed;
    GetCharacterMovement()->MaxAcceleration = MaxAcceleration;
    GetCharacterMovement()->bUseControllerDesiredRotation = false;
}

void AAvoidanceCharacter::BeginPlay()
{
    Super::BeginPlay();
}

void AAvoidanceCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // 근처 액터 찾기
    TArray<AAvoidanceCharacter*> NearbyActors;
    FindNearbyActors(NearbyActors, ObstacleDetectionDistance);

    // 회피력 계산
    FVector SeparationForce = CalculateSeparationForce(NearbyActors);
    FVector PredictiveAvoidanceForce = CalculatePredictiveAvoidanceForce(NearbyActors);
    FVector EnvironmentForce = CalculateEnvironmentAvoidanceForce();
    FVector SteerForce = CalculateSteeringForce(DeltaTime);

    if (!bAvoidanceEnabled)
    {
        PredictiveAvoidanceForce = FVector::ZeroVector;
    }

    if (!bSeparationEnabled)
    {
        SeparationForce = FVector::ZeroVector;
    }
    
    // 총 이동력 계산
    SteeringForce = SteerForce + SeparationForce + PredictiveAvoidanceForce + EnvironmentForce;

    // 최대 가속도로 제한
    if (SteeringForce.SizeSquared() > FMath::Square(MaxAcceleration))
    {
        SteeringForce = SteeringForce.GetSafeNormal() * MaxAcceleration;
    }
    
    // 현재 속도 업데이트 (참조용으로만 유지, AddInputVector가 실제 움직임을 결정)
    CurrentVelocity = GetCharacterMovement()->Velocity;
    
    // 입력 벡터 계산 및 적용
    // FVector InputVector = SteeringForce.GetSafeNormal(); // 방향만 추출
    
    // 입력 벡터 계산 및 적용
    FVector InputVector = SteeringForce;
    if (InputVector.Length() > MaxSpeed)
    {
        // InputVector = SteeringForce.GetSafeNormal() * MaxSpeed;
    }

    // 방향 업데이트 (부드러운 회전)
    float SmoothingRate = DeltaTime / FMath::Max(OrientationSmoothingTime, 0.01f);
    InputVector = FMath::Lerp(CurrentVelocity, InputVector, SmoothingRate);

    // 방향 업데이트 (부드러운 회전) - 정밀한 제어를 위한 개선된 Lerp 방식
    // if (CurrentVelocity.SizeSquared2D() > 1.0f)  // 유의미한 속도가 있을 때만 회전
    // {
    //     // 1. 현재 방향과 목표 방향을 정규화
    //     FVector CurrentForward = CurrentVelocity.GetSafeNormal();
    //     FVector DesiredForward = InputVector.GetSafeNormal();
    //
    //     // 2. 현재 방향에서 목표 방향으로의 회전을 quaternion으로 계산
    //     // FindBetweenNormals는 자동으로 가장 짧은 회전 경로를 계산합니다
    //     FQuat RotationDelta = FQuat::FindBetweenNormals(CurrentForward, DesiredForward);
    //
    //     // 3. 회전을 부드럽게 보간
    //     float SmoothingRate = DeltaTime / FMath::Max(OrientationSmoothingTime, 0.01f);
    //     FQuat SmoothRotation = FQuat::Slerp(FQuat::Identity, RotationDelta, SmoothingRate);
    //
    //     // 4. 현재 방향에 보간된 회전을 적용
    //     FVector NewDirection = SmoothRotation.RotateVector(CurrentForward);
    //
    //     // 5. 새 방향으로 입력 벡터 업데이트
    //     float InputMagnitude = InputVector.Size();
    //     InputVector = NewDirection * InputMagnitude;
    // }

    // 캐릭터 무브먼트를 통해 이동 적용
    GetCharacterMovement()->AddInputVector(InputVector);
    
    // 디버그 드로잉
    if (bDrawDebug)
    {
        FVector StartLocation = GetActorLocation() + FVector(0,0, 50);
        DrawDebugDirectionalArrow(GetWorld(), StartLocation, StartLocation + CurrentVelocity, 15, FColor::Black, false, -1, 0, 1.0f);
        StartLocation += FVector(0,0, 2);
        DrawDebugDirectionalArrow(GetWorld(), StartLocation, StartLocation + SeparationForce, 15, FColor::Yellow, false, -1, 0, 1.0f);
        StartLocation += FVector(0,0, 4);
        DrawDebugDirectionalArrow(GetWorld(), StartLocation, StartLocation + PredictiveAvoidanceForce, 15, FColor::Purple, false, -1, 0, 1.0f);
        StartLocation += FVector(0,0, 6);
        DrawDebugDirectionalArrow(GetWorld(), StartLocation, StartLocation + SteerForce, 15, FColor::Red, false, -1, 0, 1.0f);
        StartLocation += FVector(0,0, 8);
        DrawDebugDirectionalArrow(GetWorld(), StartLocation, StartLocation + SteeringForce, 15, FColor::Green, false, -1, 0, 1.0f);
        StartLocation += FVector(0,0, 10);
        DrawDebugDirectionalArrow(GetWorld(), StartLocation, StartLocation + InputVector, 15, FColor::Magenta, false, -1, 0, 1.0f);

        DrawDebugSphere(GetWorld(), StartLocation, AgentRadius, 12, FColor::Green, false, -1, 0, 1.0f);
        DrawDebugSphere(GetWorld(), MoveTargetLocation, AgentRadius, 12, FColor::Red, false, -1, 0, 1.0f);
    }
}

void AAvoidanceCharacter::FindNearbyActors(TArray<AAvoidanceCharacter*>& OutNearbyActors, float SearchRadius)
{
    OutNearbyActors.Empty();
    
    // 모든 AvoidanceCharacter 찾기
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAvoidanceCharacter::StaticClass(), FoundActors);
    
    FVector MyLocation = GetActorLocation();
    float SquaredRadius = FMath::Square(SearchRadius);
    
    // 검색 반경 내 액터 필터링
    for (AActor* Actor : FoundActors)
    {
        if (Actor == this) continue; // 자기 자신 제외
        
        AAvoidanceCharacter* AvoidanceCharacter = Cast<AAvoidanceCharacter>(Actor);
        if (!AvoidanceCharacter) continue;
        
        float DistSquared = FVector::DistSquared(MyLocation, Actor->GetActorLocation());
        if (DistSquared <= SquaredRadius)
        {
            OutNearbyActors.Add(AvoidanceCharacter);
        }
    }
}

FVector AAvoidanceCharacter::CalculateSeparationForce(const TArray<AAvoidanceCharacter*>& NearbyActors)
{
    FVector TotalForce = FVector::ZeroVector;
    
    // 분리 반경 계산
    float SeparationRadius = AgentRadius * SeparationRadiusScale;
    
    for (AAvoidanceCharacter* OtherActor : NearbyActors)
    {
        // 상대 위치 계산
        FVector RelPos = GetActorLocation() - OtherActor->GetActorLocation();
        RelPos.Z = 0; // 평면 위에서만 고려
        
        float Distance = RelPos.Size();
        FVector Direction = RelPos / FMath::Max(Distance, 0.01f); // 0으로 나누기 방지
        
        // 다른 액터의 반경 고려
        float OtherRadius = OtherActor->AgentRadius;
        float TotalRadius = SeparationRadius + OtherRadius;
        
        // 분리 페널티 계산
        float SeparationPenalty = (TotalRadius + ObstacleSeparationDistance) - Distance;
        
        // 정지 상태 액터는 회피력 감소
        float StandingScaling = 1.0f;
        if (OtherActor->GetCharacterMovement()->Velocity.SizeSquared() < 1.0f)
        {
            StandingScaling = StandingObstacleAvoidanceScale;
        }
        
        // 거리에 따른 분리력 감소 (Falloff)
        float SeparationMag = FMath::Square(FMath::Clamp(SeparationPenalty / ObstacleSeparationDistance, 0.0f, 1.0f));
        
        // 분리력 계산
        FVector SeparationForce = Direction * ObstacleSeparationStiffness * SeparationMag * StandingScaling;
        
        // 총 분리력에 추가
        TotalForce += SeparationForce;
    }
    
    return TotalForce;
}

FVector AAvoidanceCharacter::CalculatePredictiveAvoidanceForce(const TArray<AAvoidanceCharacter*>& NearbyActors)
{
    FVector TotalForce = FVector::ZeroVector;
    
    // 예측적 회피 반경 계산
    float AvoidanceRadius = AgentRadius * PredictiveAvoidanceRadiusScale;
    
    for (AAvoidanceCharacter* OtherActor : NearbyActors)
    {
        // 상대 위치와 속도 계산
        FVector RelPos = GetActorLocation() - OtherActor->GetActorLocation();
        RelPos.Z = 0; // 평면 위에서만 고려
        FVector RelVel = GetCharacterMovement()->Velocity - OtherActor->GetCharacterMovement()->Velocity;
        
        // 다른 액터의 반경 고려
        float OtherRadius = OtherActor->AgentRadius;
        float TotalRadius = AvoidanceRadius + OtherRadius;
        
        // 정지 상태 액터는 회피력 감소
        float StandingScaling = 1.0f;
        if (OtherActor->GetCharacterMovement()->Velocity.SizeSquared() < 1.0f)
        {
            StandingScaling = StandingObstacleAvoidanceScale;
        }
        
        // 가장 가까운 접근 시간(CPA) 계산
        float CPA = ComputeClosestPointOfApproach(RelPos, RelVel, TotalRadius, PredictiveAvoidanceTime);
        
        // CPA에서의 상대 위치 계산
        FVector AvoidRelPos = RelPos + RelVel * CPA;
        float AvoidDist = AvoidRelPos.Size();
        FVector AvoidNormal = AvoidDist > 0.01f ? (AvoidRelPos / AvoidDist) : FVector::ForwardVector;
        
        // 예측 침투 계산
        float AvoidPenetration = (TotalRadius + PredictiveAvoidanceDistance) - AvoidDist;
        
        // 침투에 따른 회피력 계산
        float AvoidMag = FMath::Square(FMath::Clamp(AvoidPenetration / PredictiveAvoidanceDistance, 0.0f, 1.0f));
        
        // 거리에 따른 감소
        float AvoidMagDist = (1.0f - (CPA / PredictiveAvoidanceTime));
        
        // 최종 회피력 계산
        FVector AvoidForce = AvoidNormal * AvoidMag * AvoidMagDist * ObstaclePredictiveAvoidanceStiffness * StandingScaling;
        
        // 총 회피력에 추가
        TotalForce += AvoidForce;
    }
    
    return TotalForce;
}

FVector AAvoidanceCharacter::CalculateEnvironmentAvoidanceForce()
{
    // 환경 회피력은 별도의 충돌/레이캐스트 로직으로 구현해야 함
    // 여기서는 기본적인 틀만 제공
    FVector TotalForce = FVector::ZeroVector;
    
    // TODO: 레이캐스트나 오버랩 테스트를 통해 환경 장애물 감지 및 회피력 계산
    
    return TotalForce;
}

float AAvoidanceCharacter::ComputeClosestPointOfApproach(const FVector& RelPos, const FVector& RelVel, float TotalRadius, float TimeHorizon)
{
    // 상대 속도와 위치로 충돌 시간 계산
    float A = FVector::DotProduct(RelVel, RelVel);
    float Inv2A = A > KINDA_SMALL_NUMBER ? 1.0f / (2.0f * A) : 0.0f;
    float B = FMath::Min(0.0f, 2.0f * FVector::DotProduct(RelVel, RelPos));
    float C = FVector::DotProduct(RelPos, RelPos) - FMath::Square(TotalRadius);
    
    // 판별식 계산 (음수인 경우 최대값으로)
    float Discr = FMath::Sqrt(FMath::Max(0.0f, B * B - 4.0f * A * C));
    
    // 최단 충돌 시간 계산
    float T = (-B - Discr) * Inv2A;
    
    // 시간 범위 제한
    return FMath::Clamp(T, 0.0f, TimeHorizon);
}

FVector AAvoidanceCharacter::CalculateSteeringForce(float DeltaTime)
{
    // 목표까지 방향 및 거리 계산
    FVector DirectionToTarget = MoveTargetLocation - GetActorLocation();
    DirectionToTarget.Z = 0; // 평면 위에서만 고려
    
    DistanceToGoal = DirectionToTarget.Size();
    
    // 목표 지점에 도달한 경우
    if (DistanceToGoal < 1.0f)
    {
        return -GetCharacterMovement()->Velocity * (1.0f / FMath::Max(DeltaTime, 0.1f)); // 정지력
    }
    
    // 정규화된 방향
    FVector Direction = DirectionToTarget / DistanceToGoal;
    
    // 현재 전방 벡터
    FVector CurrentForward = GetActorForwardVector();
    
    // 방향에 따른 속도 스케일링 (전방 이동이 가장 빠름)
    float ForwardDot = FVector::DotProduct(CurrentForward, Direction);
    float SpeedScale = 0.5f + 0.5f * FMath::Max(0.0f, ForwardDot); // 0.5 ~ 1.0 범위
    
    // 목표 속도 계산 (이것이 최종적으로 MaxWalkSpeed에 영향을 줌)
    DesiredVelocity = Direction * DesiredSpeed * SpeedScale;
    
    // 도착 시 감속
    float ArrivalDistance = 200.0f; // 감속 시작 거리
    if (DistanceToGoal < ArrivalDistance)
    {
        float SpeedFactor = FMath::Sqrt(DistanceToGoal / ArrivalDistance); // 감속 커브
        DesiredVelocity *= SpeedFactor;
        
        // 속도 제한 업데이트
        GetCharacterMovement()->MaxWalkSpeed = DesiredSpeed * SpeedFactor;
    }

    // 현재 속도와 원하는 속도의 차이가 스티어링 힘
    // 조향력 계산 (목표 속도와 현재 속도의 차이)
    float SteerStrength = 1.0f / 0.3f; // 반응 시간의 역수
    FVector SteerForce = (DesiredVelocity - GetCharacterMovement()->Velocity) * SteerStrength;

    return SteerForce;
}

void AAvoidanceCharacter::SetMoveTarget(const FVector& TargetLocation, const FVector& TargetForward)
{
    MoveTargetLocation = TargetLocation;
    MoveTargetForward = TargetForward.GetSafeNormal();
}

void AAvoidanceCharacter::SetDesiredSpeed(float Speed)
{
    DesiredSpeed = FMath::Max(0.0f, Speed);
    GetCharacterMovement()->MaxWalkSpeed = DesiredSpeed;
}