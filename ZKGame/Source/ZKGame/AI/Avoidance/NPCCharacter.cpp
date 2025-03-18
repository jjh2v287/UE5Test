// NPCCharacter.cpp
#include "NPCCharacter.h"

ANPCCharacter::ANPCCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    // 기본 장애물 타입으로 월드 스태틱 설정
    ObstacleObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldStatic));
    
    // 캐릭터 무브먼트 컴포넌트 설정
    UCharacterMovementComponent* MovementComp = GetCharacterMovement();
    if (MovementComp)
    {
        MovementComp->bOrientRotationToMovement = true;
        MovementComp->RotationRate = FRotator(0.0f, 360.0f, 0.0f);
        MovementComp->MaxWalkSpeed = MaxMovementSpeed;
    }
    
    // 기본 NPC 클래스를 자신의 클래스로 설정
    NPCClass = ANPCCharacter::StaticClass();
}

void ANPCCharacter::BeginPlay()
{
    Super::BeginPlay();
}

void ANPCCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // 목표에 도달하지 않았을 때만 이동 로직 실행
    if (!bReachedTarget)
    {
        MoveTowardTarget(DeltaTime);
    }
}

void ANPCCharacter::SetTargetLocation(const FVector& NewLocation)
{
    TargetLocation = NewLocation;
    bReachedTarget = false;
    
    // 디버그 시각화
    if (bShowDebugLines)
    {
        DrawDebugSphere(
            GetWorld(),
            TargetLocation,
            AcceptanceRadius,
            16,
            FColor::Green,
            false,
            5.0f,
            0,
            1.0f
        );
    }
}

FVector ANPCCharacter::CalculateTargetDirection()
{
    // 목표 방향 계산
    FVector Direction = TargetLocation - GetActorLocation();
    Direction.Z = 0.0f; // 수평 평면에서만 이동
    
    // 목표까지의 거리
    float DistanceToTarget = Direction.Size();
    
    // 목표에 도달했는지 확인
    if (DistanceToTarget <= AcceptanceRadius)
    {
        bReachedTarget = true;
        return FVector::ZeroVector;
    }
    
    // 방향 정규화
    Direction.Normalize();
    
    // 디버그 시각화
    if (bShowDebugLines)
    {
        DrawDebugLine(
            GetWorld(),
            GetActorLocation(),
            GetActorLocation() + Direction * 200.0f,
            FColor::Blue,
            false,
            -1.0f,
            0,
            2.0f
        );
    }
    
    return Direction;
}

void ANPCCharacter::MoveTowardTarget(float DeltaTime)
{
    // 목표 방향 계산
    FVector TargetDirection = CalculateTargetDirection();
    
    // 이미 목표에 도달했으면 정지
    if (bReachedTarget)
    {
        return;
    }
    
    // 장애물 회피 벡터 계산
    FVector AvoidanceVector = CalculateAvoidanceVector(DeltaTime);
    
    // NPC 분리 벡터 계산
    FVector SeparationVector = CalculateSeparationVector(DeltaTime);
    
    // 최종 이동 방향 계산 (목표 방향 + 회피 방향 + 분리 방향)
    PreFinalDirection = FinalDirection;
    FinalDirection = (TargetDirection * TargetWeight)  
                     + (AvoidanceVector * AvoidanceWeight)
                     + (SeparationVector * SeparationWeight);

    FinalDirection = FMath::Lerp(PreFinalDirection, FinalDirection, DeltaTime);
    
    // 방향 정규화
    if (!FinalDirection.IsNearlyZero())
    {
        FinalDirection.Normalize();
        
        // 캐릭터 무브먼트에 입력 벡터 추가
        GetCharacterMovement()->AddInputVector(FinalDirection);
        
        // 디버그 시각화
        if (bShowDebugLines)
        {
            DrawDebugLine(
                GetWorld(),
                GetActorLocation(),
                GetActorLocation() + FinalDirection * 200.0f,
                FColor::White,
                false,
                -1.0f,
                0,
                3.0f
            );
        }
    }
}

FVector ANPCCharacter::CalculateAvoidanceVector(float DeltaTime)
{
    FVector AvoidanceVector = FVector::ZeroVector;
    
    // 구체 트레이스 수행 (정적 장애물)
    TArray<ANPCCharacter*> NPCActors;
    FindNearbyActors(NPCActors, ObstacleDetectionRadius);
    
    // 히트 결과가 있으면 각 장애물에 대한 회피 벡터 계산
    for (ANPCCharacter* OtherNPC : NPCActors)
    {
        FVector SteeringForce = SteerAwayFromObstacle(OtherNPC, DeltaTime);
        AvoidanceVector += SteeringForce;
    }
    
    return AvoidanceVector;
}

FVector ANPCCharacter::SteerAwayFromObstacle(ANPCCharacter* OtherNPC, float DeltaTime)
{
    // 장애물로부터의 방향 벡터 계산
    FVector ObstacleDirection = GetActorLocation() - OtherNPC->GetActorLocation();
    ObstacleDirection.Z = 0.0f; // 수평 평면에서만 회피
    
    // 장애물까지의 거리 계산
    float DistanceToObstacle = ObstacleDirection.Size();
    
    // 장애물이 너무 멀리 있으면 무시
    if (DistanceToObstacle > ObstacleDetectionRadius)
    {
        return FVector::ZeroVector;
    }
    
    // 방향 벡터 정규화
    ObstacleDirection.Normalize();
    
    // 장애물까지의 거리에 따라 회피력 계산
    // 가까울수록 더 강한 회피력
    float AvoidanceStrength = FMath::Max(0.0f, (MinObstacleDistance - DistanceToObstacle) / MinObstacleDistance);
    AvoidanceStrength = FMath::Clamp(AvoidanceStrength, 0.0f, 1.0f);
    
    // 회피 벡터 계산 (장애물로부터 멀어지는 방향)
    FVector AvoidanceForce = ObstacleDirection * ObstacleAvoidanceForce * AvoidanceStrength * DeltaTime;
    
    // 디버그 시각화
    if (bShowDebugLines)
    {
        DrawDebugLine(
            GetWorld(),
            OtherNPC->GetActorLocation(),
            OtherNPC->GetActorLocation() + ObstacleDirection * 100.0f,
            FColor::Yellow,
            false,
            -1.0f,
            0,
            2.0f
        );
    }
    
    return AvoidanceForce;
}

FVector ANPCCharacter::CalculateSeparationVector(float DeltaTime)
{
    FVector SeparationVector = FVector::ZeroVector;
    FVector MyLocation = GetActorLocation();
    
    // 모든 NPC AI 객체 찾기
    TArray<ANPCCharacter*> NPCActors;
    FindNearbyActors(NPCActors, SeparationRadius);
    
    int32 NeighborCount = 0;
    
    // 각 NPC AI 객체에 대한 분리 벡터 계산
    for (ANPCCharacter* OtherNPC : NPCActors)
    {
        // 자기 자신은 제외
        if (OtherNPC != this)
        {
            FVector OtherLocation = OtherNPC->GetActorLocation();
            FVector ToMe = MyLocation - OtherLocation;
            ToMe.Z = 0.0f; // 수평 평면에서만 분리
            
            float Distance = ToMe.Size();
            
            // 분리 반경 내에 있는 NPC만 처리
            if (Distance < SeparationRadius && Distance > 0.0f)
            {
                // 거리에 따른 분리 벡터 계산 (가까울수록 더 강함)
                float Strength = 1.0f - (Distance / SeparationRadius);
                Strength = FMath::Pow(Strength, 2); // 비선형 감소 적용
                ToMe.Normalize();
                SeparationVector += ToMe * Strength;
                NeighborCount++;
                
                // 디버그 시각화
                if (bShowDebugLines)
                {
                    DrawDebugLine(
                        GetWorld(),
                        OtherLocation,
                        OtherLocation + ToMe * 100.0f * Strength,
                        FColor::Magenta,
                        false,
                        -1.0f,
                        0,
                        1.0f
                    );
                }
            }
        }
    }
    
    // 이웃 NPC가 있으면 평균 벡터 계산
    if (NeighborCount > 0)
    {
        // 평균 벡터 계산 및 정규화
        SeparationVector /= NeighborCount;
        if (!SeparationVector.IsNearlyZero())
        {
            SeparationVector.Normalize();
            
            // 최종 분리 벡터 계산
            SeparationVector *= SeparationForce * DeltaTime;
            
            // 디버그 시각화
            if (bShowDebugLines)
            {
                DrawDebugLine(
                    GetWorld(),
                    MyLocation,
                    MyLocation + SeparationVector * 200.0f,
                    FColor::Purple,
                    false,
                    -1.0f,
                    0,
                    2.0f
                );
            }
        }
    }
    
    return SeparationVector;
}

void ANPCCharacter::FindNearbyActors(TArray<ANPCCharacter*>& OutNearbyActors, float SearchRadius)
{
    OutNearbyActors.Empty();
    
    // 모든 NPCCharacter 찾기
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANPCCharacter::StaticClass(), FoundActors);
    
    FVector MyLocation = GetActorLocation();
    float SquaredRadius = FMath::Square(SearchRadius);
    
    // 검색 반경 내 액터 필터링
    for (AActor* Actor : FoundActors)
    {
        if (Actor == this) continue; // 자기 자신 제외
        
        ANPCCharacter* OtherNPC = Cast<ANPCCharacter>(Actor);
        if (!OtherNPC) continue;
        
        float DistSquared = FVector::DistSquared(MyLocation, Actor->GetActorLocation());
        if (DistSquared <= SquaredRadius)
        {
            OutNearbyActors.Add(OtherNPC);
        }
    }
}