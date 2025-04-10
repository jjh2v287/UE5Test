#include "UKECSTestActor.h"
#include "UKECSComponentBase.h"
#include "UKECSManager.h"
#include "Engine/World.h"

AUKECSTestActor::AUKECSTestActor()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AUKECSTestActor::BeginPlay()
{
    Super::BeginPlay();

    // 1. WorldSubsystem에서 ECS 매니저 가져오기
    UUKECSManager* ECSManager = GetWorld()->GetSubsystem<UUKECSManager>();
    if (!ECSManager)
    {
        return;
    }
    
    // 2. 아키타입 구성 정의 (Position + Velocity)
    // MovableComposition.Add(FPositionComponent::StaticStruct());
    // MovableComposition.Add(FVelocityComponent::StaticStruct());
    FECSArchetypeComposition MovableComposition;
    for (const FInstancedStruct& Struct : ECSComponentInstances)
    {
        if (Struct.IsValid())
        {
            MovableComposition.Add(Struct.GetScriptStruct());
        }
    }

    // 3. 엔티티 생성
    FECSEntityHandle Entity1 = ECSManager->CreateEntity(MovableComposition);
    FECSEntityHandle Entity2 = ECSManager->CreateEntity(MovableComposition);
    Entity = Entity1;

    // 4. 컴포넌트 데이터 설정
    if (FUKECSPositionComponent* Pos1 = ECSManager->GetComponentData<FUKECSPositionComponent>(Entity1))
    {
        Pos1->Position = FVector(0.f, 0.f, 0.f);
    }
    if (FUKECSVelocityComponent* Vel1 = ECSManager->GetComponentData<FUKECSVelocityComponent>(Entity1))
    {
        Vel1->Velocity = FVector(10.f, 0.f, 0.f);
    }

    // 5. 시스템 로직 실행 (위치 업데이트)
    /*float DeltaTime = 0.1f;
    TArray<FECSComponentTypeID> QueryTypes = { FUKECSPositionComponent::StaticStruct(), FUKECSVelocityComponent::StaticStruct() };
    ECSManager->ForEachChunk(QueryTypes, [&](FECSArchetypeChunk& Chunk)
    {
        int32 NumInChunk = Chunk.GetEntityCount();
        
        // 청크에서 직접 데이터 배열 가져오기 (SoA 방식)
        FUKECSPositionComponent* Positions = static_cast<FUKECSPositionComponent*>(Chunk.GetComponentRawDataArray(FUKECSPositionComponent::StaticStruct()));
        const FUKECSVelocityComponent* Velocities = static_cast<const FUKECSVelocityComponent*>(Chunk.GetComponentRawDataArray(FUKECSVelocityComponent::StaticStruct()));
    
        if (Positions && Velocities)
        {
            for (int32 i = 0; i < NumInChunk; ++i)
            {
                Positions[i].Position += Velocities[i].Velocity * DeltaTime;
            }
        }
    });*/
    for (TSubclassOf<UUKECSSystemBase>& System : ECSSystemBaseClasses)
    {
        ECSManager->AddSystem(System);
    }
    
    // 6. 컴포넌트 데이터 변경을 확인
    FVector TestData = FVector::ZeroVector;
    if (FUKECSPositionComponent* Pos1 = ECSManager->GetComponentData<FUKECSPositionComponent>(Entity1))
    {
        TestData = Pos1->Position;
    }

    // 7. 컴포넌트 제거 (아키타입 변경 발생)
    ECSManager->RemoveComponent(Entity1, FUKECSVelocityComponent::StaticStruct());
    
    // 8. 엔티티 파괴
    ECSManager->DestroyEntity(Entity1);
    ECSManager->DestroyEntity(Entity2);
}

void AUKECSTestActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    UUKECSManager* ECSManager = GetWorld()->GetSubsystem<UUKECSManager>();
    if (!ECSManager)
    {
        return;
    }
    
    if (FUKECSPositionComponent* Pos1 = ECSManager->GetComponentData<FUKECSPositionComponent>(Entity))
    {
         FVector Position = Pos1->Position;
        SetActorLocation(Position);
    }
}
