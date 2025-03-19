// AvoidanceActor.cpp
#include "AvoidanceActor.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"

AAvoidanceActor::AAvoidanceActor()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AAvoidanceActor::BeginPlay()
{
    Super::BeginPlay();
}

void AAvoidanceActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // 근처 액터 찾기
    TArray<AAvoidanceActor*> NearbyActors;
    FindNearbyActors(NearbyActors, ObstacleDetectionDistance);
    
    // 회피력 계산
    FVector SeparationForce = CalculateSeparationForce(NearbyActors);
    FVector PredictiveAvoidanceForce = CalculatePredictiveAvoidanceForce(NearbyActors);
    FVector EnvironmentForce = CalculateEnvironmentAvoidanceForce();
    FVector SteerForce = CalculateSteeringForce(DeltaTime);
    
    // 총 이동력 계산
    SteeringForce = SteerForce + SeparationForce + PredictiveAvoidanceForce + EnvironmentForce;
    
    // 최대 가속도로 제한
    if (SteeringForce.SizeSquared() > FMath::Square(MaxAcceleration))
    {
        SteeringForce = SteeringForce.GetSafeNormal() * MaxAcceleration;
    }
    
    // 속도 업데이트
    CurrentVelocity += SteeringForce * DeltaTime;
    
    // 최대 속도로 제한
    if (CurrentVelocity.SizeSquared() > FMath::Square(MaxSpeed))
    {
        CurrentVelocity = CurrentVelocity.GetSafeNormal() * MaxSpeed;
    }
    
    // 위치 업데이트
    FVector NewLocation = GetActorLocation() + CurrentVelocity * DeltaTime;
    SetActorLocation(NewLocation);
    
    // 방향 업데이트 (부드러운 회전)
    if (CurrentVelocity.SizeSquared() > 1.0f)
    {
        FVector DesiredForward = CurrentVelocity.GetSafeNormal();
        FVector CurrentForward = GetActorForwardVector();
        
        // 현재 Yaw 각도
        float CurrentYaw = FMath::Atan2(CurrentForward.Y, CurrentForward.X);
        
        // 목표 Yaw 각도
        float DesiredYaw = FMath::Atan2(DesiredForward.Y, DesiredForward.X);
        
        // 각도 차이 계산 (Wrap 처리)
        float DeltaAngle = DesiredYaw - CurrentYaw;
        if (DeltaAngle > PI) DeltaAngle -= PI * 2;
        if (DeltaAngle < -PI) DeltaAngle += PI * 2;
        
        // 부드러운 각도 변화
        float SmoothingRate = DeltaTime / FMath::Max(OrientationSmoothingTime, 0.01f);
        float NewYaw = CurrentYaw + DeltaAngle * SmoothingRate;
        
        // 회전 적용
        FRotator NewRotation = FRotator(0, FMath::RadiansToDegrees(NewYaw), 0);
        SetActorRotation(NewRotation);
    }
    
    // 디버그 드로잉
    if (bDrawDebug)
    {
        FVector StartLocation = GetActorLocation() + FVector(0,0, 150);
        DrawDebugDirectionalArrow(GetWorld(), StartLocation, StartLocation + CurrentVelocity, 15, FColor::Black, false, -1, 0, 3.0f);
        StartLocation += FVector(0,0, 2);
        DrawDebugDirectionalArrow(GetWorld(), StartLocation, StartLocation + SeparationForce, 15, FColor::Yellow, false, -1, 0, 3.0f);
        StartLocation += FVector(0,0, 4);
        DrawDebugDirectionalArrow(GetWorld(), StartLocation, StartLocation + PredictiveAvoidanceForce, 15, FColor::Purple, false, -1, 0, 3.0f);
        StartLocation += FVector(0,0, 6);
        DrawDebugDirectionalArrow(GetWorld(), StartLocation, StartLocation + SteerForce, 15, FColor::Red, false, -1, 0, 3.0f);
        StartLocation += FVector(0,0, 8);
        DrawDebugDirectionalArrow(GetWorld(), StartLocation, StartLocation + SteeringForce, 15, FColor::Green, false, -1, 0, 3.0f);

        DrawDebugSphere(GetWorld(), GetActorLocation(), AgentRadius, 12, FColor::Green, false, -1, 0, 1.0f);
        DrawDebugSphere(GetWorld(), MoveTargetLocation, AgentRadius, 12, FColor::Red, false, -1, 0, 1.0f);
    }
}

void AAvoidanceActor::FindNearbyActors(TArray<AAvoidanceActor*>& OutNearbyActors, float SearchRadius)
{
    OutNearbyActors.Empty();
    
    // 모든 AvoidanceActor 찾기
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAvoidanceActor::StaticClass(), FoundActors);
    
    FVector MyLocation = GetActorLocation();
    float SquaredRadius = FMath::Square(SearchRadius);
    
    // 검색 반경 내 액터 필터링
    for (AActor* Actor : FoundActors)
    {
        if (Actor == this) continue; // 자기 자신 제외
        
        AAvoidanceActor* AvoidanceActor = Cast<AAvoidanceActor>(Actor);
        if (!AvoidanceActor) continue;
        
        float DistSquared = FVector::DistSquared(MyLocation, Actor->GetActorLocation());
        if (DistSquared <= SquaredRadius)
        {
            OutNearbyActors.Add(AvoidanceActor);
        }
    }
}

FVector AAvoidanceActor::CalculateSeparationForce(const TArray<AAvoidanceActor*>& NearbyActors)
{
    FVector TotalForce = FVector::ZeroVector;
    
    // 분리 반경 계산
    float SeparationRadius = AgentRadius * SeparationRadiusScale;
    
    for (AAvoidanceActor* OtherActor : NearbyActors)
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
        if (OtherActor->CurrentVelocity.SizeSquared() < 1.0f)
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

FVector AAvoidanceActor::CalculatePredictiveAvoidanceForce(const TArray<AAvoidanceActor*>& NearbyActors)
{
    FVector TotalForce = FVector::ZeroVector;
    
    // 예측적 회피 반경 계산
    float AvoidanceRadius = AgentRadius * PredictiveAvoidanceRadiusScale;
    
    for (AAvoidanceActor* OtherActor : NearbyActors)
    {
        // 상대 위치와 속도 계산
        FVector RelPos = GetActorLocation() - OtherActor->GetActorLocation();
        RelPos.Z = 0; // 평면 위에서만 고려
        FVector RelVel = CurrentVelocity - OtherActor->CurrentVelocity;
        
        // 다른 액터의 반경 고려
        float OtherRadius = OtherActor->AgentRadius;
        float TotalRadius = AvoidanceRadius + OtherRadius;
        
        // 정지 상태 액터는 회피력 감소
        float StandingScaling = 1.0f;
        if (OtherActor->CurrentVelocity.SizeSquared() < 1.0f)
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

FVector AAvoidanceActor::CalculateEnvironmentAvoidanceForce()
{
    // 환경 회피력은 별도의 충돌/레이캐스트 로직으로 구현해야 함
    // 여기서는 기본적인 틀만 제공
    FVector TotalForce = FVector::ZeroVector;
    
    // TODO: 레이캐스트나 오버랩 테스트를 통해 환경 장애물 감지 및 회피력 계산
    
    return TotalForce;
}

float AAvoidanceActor::ComputeClosestPointOfApproach(const FVector& RelPos, const FVector& RelVel, float TotalRadius, float TimeHorizon)
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

FVector AAvoidanceActor::CalculateSteeringForce(float DeltaTime)
{
    // 목표까지 방향 및 거리 계산
    FVector DirectionToTarget = MoveTargetLocation - GetActorLocation();
    DirectionToTarget.Z = 0; // 평면 위에서만 고려
    
    DistanceToGoal = DirectionToTarget.Size();
    
    // 목표 지점에 도달한 경우
    if (DistanceToGoal < 1.0f)
    {
        return -CurrentVelocity * (1.0f / FMath::Max(DeltaTime, 0.1f)); // 정지력
    }
    
    // 정규화된 방향
    FVector Direction = DirectionToTarget / DistanceToGoal;
    
    // 현재 전방 벡터
    FVector CurrentForward = GetActorForwardVector();
    
    // 방향에 따른 속도 스케일링 (전방 이동이 가장 빠름)
    float ForwardDot = FVector::DotProduct(CurrentForward, Direction);
    float SpeedScale = 0.5f + 0.5f * FMath::Max(0.0f, ForwardDot); // 0.5 ~ 1.0 범위
    
    // 목표 속도 계산
    DesiredVelocity = Direction * DesiredSpeed * SpeedScale;
    
    // 도착 시 감속
    float ArrivalDistance = 200.0f; // 감속 시작 거리
    if (DistanceToGoal < ArrivalDistance)
    {
        float SpeedFactor = FMath::Sqrt(DistanceToGoal / ArrivalDistance); // 감속 커브
        DesiredVelocity *= SpeedFactor;
    }
    
    // 조향력 계산 (목표 속도와 현재 속도의 차이)
    float SteerStrength = 1.0f / 0.3f; // 반응 시간의 역수
    FVector SteerForce = (DesiredVelocity - CurrentVelocity) * SteerStrength;
    
    return SteerForce;
}

void AAvoidanceActor::SetMoveTarget(const FVector& TargetLocation, const FVector& TargetForward)
{
    MoveTargetLocation = TargetLocation;
    MoveTargetForward = TargetForward.GetSafeNormal();
}

void AAvoidanceActor::SetDesiredSpeed(float Speed)
{
    DesiredSpeed = FMath::Max(0.0f, Speed);
}