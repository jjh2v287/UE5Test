// Fill out your copyright notice in the Description page of Project Settings.


#include "UKECSSystemBase.h"
#include "UKECSComponentBase.h"
#include "UKECSManager.h"

void UECSMove::Register()
{
	QueryTypes.Emplace(FUKECSPositionComponent::StaticStruct());
	QueryTypes.Emplace(FUKECSVelocityComponent::StaticStruct());
}

void UECSMove::Tick(float DeltaTime, UUKECSManager* ECSManager)
{
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
	});
}
