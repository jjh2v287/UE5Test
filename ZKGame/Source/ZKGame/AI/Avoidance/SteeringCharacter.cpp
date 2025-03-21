// SteeringCharacter.cpp
#include "SteeringCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Components/CapsuleComponent.h"

ASteeringCharacter::ASteeringCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    // 충돌 감지용 스피어 컴포넌트 생성
    DetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionSphere"));
    DetectionSphere->SetupAttachment(RootComponent);
    DetectionSphere->SetSphereRadius(DetectionRadius);
    DetectionSphere->SetCollisionProfileName(TEXT("OverlapAll"));
    DetectionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    DetectionSphere->SetHiddenInGame(true);
}

void ASteeringCharacter::BeginPlay()
{
    Super::BeginPlay();

    // 캐릭터 무브먼트 컴포넌트 설정
    UCharacterMovementComponent* CharMovement = GetCharacterMovement();
    if (CharMovement)
    {
        CharMovement->bOrientRotationToMovement = true; // 이동 방향으로 회전
        CharMovement->RotationRate = FRotator(0.0f, 640.0f, 0.0f); // 회전 속도
        CharMovement->bUseControllerDesiredRotation = false;
        CharMovement->MaxWalkSpeed = MaxSpeed;

        // CharMovement->GravityScale = 0.0f;
        // CharMovement->BrakingFrictionFactor = 0.0f;
        // CharMovement->GroundFriction = 0.0f;
        // CharMovement->BrakingDecelerationWalking = 0.0f;

        /*
        * CharMovement->GravityScale = 0.0f;
        * CharMovement->BrakingFrictionFactor = 0.0f;
        * CharMovement->GroundFriction = 0.0f;
        * CharMovement->BrakingDecelerationWalking = 0.0f;
        * 위 4개로 인한 감소로 SteerForce가 갑자기 튀는 현상이 있다
        * 일전속도 이하가 되면 GetVelocity()가 0이 됨 대략 30 아래이면 다음 틱에서 바로 0
        */
    }
    
    // 초기 타겟 위치 설정
    GoalLocation = GetActorLocation();
    
    // 감지 반경 설정
    DetectionSphere->SetSphereRadius(DetectionRadius);
}

void ASteeringCharacter::SetMoveTarget(const FVector& TargetLocation)
{
    GoalLocation = TargetLocation;
}

void ASteeringCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 근처의 SteeringCharacter 찾기
    TArray<ASteeringCharacter*> NearbyActors = GetNearbySteeringCharacters();

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

    FVector NesSteeringForce = FVector::ZeroVector;
    // 캐릭터 무브먼트를 통해 이동 적용
    if (!SteeringForce.IsNearlyZero())
    {
        NesSteeringForce = FMath::Lerp(GetVelocity(), SteeringForce, DeltaTime);
        GetCharacterMovement()->RequestDirectMove(NesSteeringForce, false);
        GetCharacterMovement()->Velocity = NesSteeringForce;
        // GetCharacterMovement()->Velocity += NesSteeringForce * DeltaTime;
        // GetCharacterMovement()->RequestDirectMove(GetCharacterMovement()->Velocity + (NesSteeringForce * DeltaTime), false);
    }

    FVector StartLocation = GetActorLocation() + FVector(0,0, 50);
    DrawDebugDirectionalArrow(GetWorld(), StartLocation, StartLocation + GetVelocity(), 15, FColor::Black, false, -1, 0, 1.0f);
    StartLocation += FVector(0,0, 2);
    DrawDebugDirectionalArrow(GetWorld(), StartLocation, StartLocation + SeparationForce, 15, FColor::Yellow, false, -1, 0, 1.0f);
    StartLocation += FVector(0,0, 4);
    DrawDebugDirectionalArrow(GetWorld(), StartLocation, StartLocation + PredictiveAvoidanceForce, 15, FColor::Purple, false, -1, 0, 1.0f);
    StartLocation += FVector(0,0, 6);
    DrawDebugDirectionalArrow(GetWorld(), StartLocation, StartLocation + SteerForce, 15, FColor::Red, false, -1, 0, 1.0f);
    StartLocation += FVector(0,0, 8);
    DrawDebugDirectionalArrow(GetWorld(), StartLocation, StartLocation + SteeringForce, 15, FColor::Green, false, -1, 0, 1.0f);
    StartLocation += FVector(0,0, 10);
    DrawDebugDirectionalArrow(GetWorld(), StartLocation, StartLocation + NesSteeringForce, 15, FColor::Magenta, false, -1, 0, 1.0f);
    
    DrawDebugSphere(GetWorld(), GoalLocation, 20, 12, FColor::Red, false, -1, 0, 1.0f);
}

TArray<ASteeringCharacter*> ASteeringCharacter::GetNearbySteeringCharacters() const
{
    TArray<ASteeringCharacter*> Result;
    TArray<AActor*> OverlappingActors;
    
    // 컴포넌트 범위 내 액터들 가져오기
    DetectionSphere->GetOverlappingActors(OverlappingActors);
    
    // SteeringCharacter만 필터링
    for (AActor* Actor : OverlappingActors)
    {
        ASteeringCharacter* SteeringChar = Cast<ASteeringCharacter>(Actor);
        if (SteeringChar && SteeringChar != this)
        {
            Result.Add(SteeringChar);
        }
    }
    
    return Result;
}

FVector ASteeringCharacter::CalculateSeparationForce(const TArray<ASteeringCharacter*>& NearbyActors)
{
    FVector TotalForce = FVector::ZeroVector;
    
    for (ASteeringCharacter* Other : NearbyActors)
    {
        // 현재 위치 기준 상대 위치 계산
        FVector RelPos = GetActorLocation() - Other->GetActorLocation();
        RelPos.Z = 0.0f; // 평면 상에서만 계산
        
        const float ConDist = RelPos.Size();
        if (ConDist <= 0.0f) continue; // 같은 위치에 있으면 스킵
        
        const FVector ConNorm = RelPos / ConDist;
        
        // 자신과 상대방의 반경 계산
        const float MyRadius = GetCapsuleComponent()->GetScaledCapsuleRadius() * SeparationRadiusScale;
        const float OtherRadius = Other->GetCapsuleComponent()->GetScaledCapsuleRadius() * SeparationRadiusScale;
        
        // 분리력 계산 (penetration 기반)
        const float PenSep = (MyRadius + OtherRadius + SeparationDistance) - ConDist;
        const float SeparationMag = FMath::Square(FMath::Clamp(PenSep / SeparationDistance, 0.0f, 1.0f));
        const FVector Force = ConNorm * SeparationStiffness * SeparationMag;
        
        TotalForce += Force;
    }
    
    return TotalForce;
}

float ASteeringCharacter::ComputeClosestPointOfApproach(
    const FVector& RelPos, const FVector& RelVel, const float TotalRadius, const float TimeHorizon) const
{
    // 상대적 에이전트 위치와 속도를 기반으로 충돌 시간 계산
    const float A = FVector::DotProduct(RelVel, RelVel);
    const float Inv2A = A > SMALL_NUMBER ? 1.0f / (2.0f * A) : 0.0f;
    const float B = FMath::Min(0.0f, 2.0f * FVector::DotProduct(RelVel, RelPos));
    const float C = FVector::DotProduct(RelPos, RelPos) - FMath::Square(TotalRadius);
    
    // 판별식을 이용한 충돌 시간 계산 (충돌이 없으면 CPA 사용)
    const float Discr = FMath::Sqrt(FMath::Max(0.0f, B * B - 4.0f * A * C));
    const float T = (-B - Discr) * Inv2A;
    
    return FMath::Clamp(T, 0.0f, TimeHorizon);
}

FVector ASteeringCharacter::CalculatePredictiveAvoidanceForce(const TArray<ASteeringCharacter*>& NearbyActors)
{
    FVector TotalForce = FVector::ZeroVector;
    
    for (ASteeringCharacter* Other : NearbyActors)
    {
        // 현재 위치 기준 상대 위치와 속도 계산
        FVector RelPos = GetActorLocation() - Other->GetActorLocation();
        RelPos.Z = 0.0f; // 평면 상에서만 계산
        
        FVector MyVel = GetVelocity();
        MyVel.Z = 0.0f;
        FVector OtherVel = Other->GetVelocity();
        OtherVel.Z = 0.0f;
        FVector RelVel = MyVel - OtherVel;
        
        // 반경 계산
        const float MyRadius = GetCapsuleComponent()->GetScaledCapsuleRadius() * PredictiveAvoidanceRadiusScale;
        const float OtherRadius = Other->GetCapsuleComponent()->GetScaledCapsuleRadius() * PredictiveAvoidanceRadiusScale;
        
        // 최근접 지점 시간 계산 (Closest Point of Approach)
        const float CPA = ComputeClosestPointOfApproach(RelPos, RelVel, MyRadius + OtherRadius, PredictiveAvoidanceTime);
        
        // CPA에서의 위치 계산
        const FVector AvoidRelPos = RelPos + RelVel * CPA;
        const float AvoidDist = AvoidRelPos.Size();
        if (AvoidDist <= 0.0f) continue;
        
        const FVector AvoidNormal = AvoidRelPos / AvoidDist;
        
        // 회피력 계산
        const float AvoidPenetration = (MyRadius + OtherRadius + PredictiveAvoidanceDistance) - AvoidDist;
        const float AvoidMag = FMath::Square(FMath::Clamp(AvoidPenetration / PredictiveAvoidanceDistance, 0.0f, 1.0f));
        const float AvoidMagDist = (1.0f - (CPA / PredictiveAvoidanceTime));
        const FVector Force = AvoidNormal * AvoidMag * AvoidMagDist * PredictiveAvoidanceStiffness;
        
        TotalForce += Force;
    }
    
    return TotalForce;
}

FVector ASteeringCharacter::CalculateEnvironmentAvoidanceForce()
{
    FVector TotalForce = FVector::ZeroVector;
    
    /*// 환경 장애물 감지 및 회피 로직
    // 이 부분은 환경에 따라 구현을 확장할 수 있습니다
    // Mass AI에서는 FNavigationAvoidanceEdge를 사용하지만,
    // ACharacter에서는 Line Trace나 Shape Trace를 사용하여 구현할 수 있습니다
    
    // 예시: 여러 방향으로 레이캐스트하여 장애물 감지
    const FVector CurrentLocation = GetActorLocation();
    const float AgentRadius = GetCapsuleComponent()->GetScaledCapsuleRadius();
    
    // 여러 방향으로 레이캐스트
    const int32 NumRays = 8;
    const float CheckDistance = AgentRadius + EnvironmentSeparationDistance;
    
    for (int32 i = 0; i < NumRays; ++i)
    {
        const float Angle = 2.0f * PI * i / NumRays;
        const FVector Direction(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
        
        FHitResult HitResult;
        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(this);
        
        // 레이캐스트 수행
        if (GetWorld()->LineTraceSingleByChannel(
            HitResult, 
            CurrentLocation, 
            CurrentLocation + Direction * CheckDistance,
            ECC_WorldStatic, 
            QueryParams))
        {
            // 장애물과의 거리 계산
            const float HitDistance = HitResult.Distance;
            const FVector HitNormal = HitResult.ImpactNormal;
            
            // 분리력 계산
            const float SeparationPenalty = CheckDistance - HitDistance;
            const float SeparationMag = FMath::Clamp(SeparationPenalty / EnvironmentSeparationDistance, 0.0f, 1.0f);
            const FVector Force = HitNormal * EnvironmentSeparationStiffness * SeparationMag;
            
            TotalForce += Force;
        }
    }*/
    
    return TotalForce;
}

FVector ASteeringCharacter::CalculateSteeringForce(float DeltaTime)
{
    // 기본 스티어링 힘 - 현재 위치에서 목표 위치로 이동하는 힘
    FVector CurrentLocation = GetActorLocation();
    FVector DesiredDirection = GoalLocation - CurrentLocation;
    DesiredDirection.Z = 0.0f; // 평면 이동
    
    const float DistanceToTarget = DesiredDirection.Size();
    
    if (DistanceToTarget > 1.0f)
    {
        DesiredDirection /= DistanceToTarget; // 방향 벡터 정규화
        
        // 최대 속도로 제한된 희망 속도
        FVector DesiredVelocity = DesiredDirection * MaxSpeed;
        
        // 도착 시 감속
        const float SlowdownRadius = 200.0f;
        if (DistanceToTarget < SlowdownRadius)
        {
            DesiredVelocity *= (DistanceToTarget / SlowdownRadius);
        }
        
        // 현재 속도
        FVector CurrentVelocity = GetVelocity();
        UE_LOG(LogTemp,Warning, TEXT("%f"), CurrentVelocity.Length());
        CurrentVelocity.Z = 0.0f;
        
        // Steering = 희망속도 - 현재속도
        float SteerStrength = 1.0f / 0.3f; // 반응 시간의 역수
        FVector NewSteeringForce = (DesiredVelocity - CurrentVelocity) * SteerStrength;

        return NewSteeringForce;
    }
    
    return FVector::ZeroVector;
}