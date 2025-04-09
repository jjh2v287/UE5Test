#include "ECSTestActor.h"
#include "SimpleECS.h"

AECSTestActor::AECSTestActor()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AECSTestActor::BeginPlay()
{
    Super::BeginPlay();

    // 1. ECS 매니저 생성
    FECSManager ECSManager;

    // 2. 아키타입 구성 정의 (Position + Velocity)
    FECSArchetypeComposition MovableComposition;
    MovableComposition.Add(FPositionComponent::StaticStruct());
    MovableComposition.Add(FVelocityComponent::StaticStruct());

    // 3. 엔티티 생성
    FECSEntityHandle Entity1 = ECSManager.CreateEntity(MovableComposition);
    FECSEntityHandle Entity2 = ECSManager.CreateEntity(MovableComposition);

    // 4. 컴포넌트 데이터 설정
    if (FPositionComponent* Pos1 = ECSManager.GetComponentData<FPositionComponent>(Entity1))
    {
        Pos1->Position = FVector(0.f, 0.f, 0.f);
    }
    if (FVelocityComponent* Vel1 = ECSManager.GetComponentData<FVelocityComponent>(Entity1))
    {
        Vel1->Velocity = FVector(10.f, 0.f, 0.f);
    }

    // 5. 시스템 로직 실행 (위치 업데이트)
    float DeltaTime = 0.1f;
    TArray<FECSComponentTypeID> QueryTypes = { 
        FPositionComponent::StaticStruct(), 
        FVelocityComponent::StaticStruct() 
    };

    ECSManager.ForEachChunk(QueryTypes, [&](FECSArchetypeChunk& Chunk)
    {
        int32 NumInChunk = Chunk.GetEntityCount();
        
        // 청크에서 직접 데이터 배열 가져오기 (SoA 방식)
        FPositionComponent* Positions = static_cast<FPositionComponent*>(Chunk.GetComponentRawDataArray(FPositionComponent::StaticStruct()));
        const FVelocityComponent* Velocities = static_cast<const FVelocityComponent*>(Chunk.GetComponentRawDataArray(FVelocityComponent::StaticStruct()));

        if (Positions && Velocities)
        {
            for (int32 i = 0; i < NumInChunk; ++i)
            {
                Positions[i].Position += Velocities[i].Velocity * DeltaTime;
            }
        }
    });

    // 6. 컴포넌트 데이터 변경을 확인
    FVector TestData = FVector::ZeroVector;
    if (FPositionComponent* Pos1 = ECSManager.GetComponentData<FPositionComponent>(Entity1))
    {
        TestData = Pos1->Position;
    }

    // 7. 컴포넌트 제거 (아키타입 변경 발생)
    ECSManager.RemoveComponent(Entity1, FVelocityComponent::StaticStruct());

    // 8. 엔티티 파괴
    ECSManager.DestroyEntity(Entity1);
    ECSManager.DestroyEntity(Entity2);
}