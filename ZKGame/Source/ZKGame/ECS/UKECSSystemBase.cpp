// Fill out your copyright notice in the Description page of Project Settings.


#include "UKECSSystemBase.h"
#include "UKECSComponentBase.h"
#include "UKECSManager.h"
#include "Async/Async.h"
#include "Async/ParallelFor.h"

void UECSMove::Register()
{
	QueryTypes.Emplace(FUKECSForwardMoveComponent::StaticStruct());
}

void UECSMove::Tick(float DeltaTime, UUKECSManager* ECSManager)
{
	ECSManager->ForEachChunk(QueryTypes, [&](FECSArchetypeChunk& Chunk)
	{
		int32 NumInChunk = Chunk.GetEntityCount();
        
		// 청크에서 직접 데이터 배열 가져오기 (SoA 방식)
		FUKECSForwardMoveComponent* MoveComponent = static_cast<FUKECSForwardMoveComponent*>(Chunk.GetComponentRawDataArray(FUKECSForwardMoveComponent::StaticStruct()));

		if (MoveComponent)
		{
			AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [=]()
			{
				ParallelFor(NumInChunk, [&](int32 i)
				{
					FVector Direction = FVector(MoveComponent[i].TargetLocation - MoveComponent[i].Location);
					MoveComponent[i].ForwardVector = Direction.GetSafeNormal();
					MoveComponent[i].Location += MoveComponent[i].ForwardVector * MoveComponent[i].Speed * DeltaTime;
					MoveComponent[i].Rotator = MoveComponent[i].ForwardVector.Rotation();
				});
			});
			/*for (int32 i = 0; i < NumInChunk; ++i)
			{
				FVector Direction = FVector(MoveComponent[i].TargetLocation - MoveComponent[i].Location);
				MoveComponent[i].ForwardVector = Direction.GetSafeNormal();
				MoveComponent[i].Location += MoveComponent[i].ForwardVector * MoveComponent[i].Speed * DeltaTime;
				MoveComponent[i].Rotator = MoveComponent[i].ForwardVector.Rotation();
			}*/
		}
	});
}
