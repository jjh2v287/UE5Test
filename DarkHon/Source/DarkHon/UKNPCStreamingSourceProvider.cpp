// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKNPCStreamingSourceProvider.h"
#include "Engine/LevelStreaming.h"
#include "Common/UKCommonLog.h"
#include "Components/WorldPartitionStreamingSourceComponent.h"
#include "WorldPartition/RuntimeHashSet/WorldPartitionRuntimeHashSet.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UKNPCStreamingSourceProvider)

AUKNPCStreamingSourceProvider::AUKNPCStreamingSourceProvider(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void AUKNPCStreamingSourceProvider::BeginPlay()
{
	Super::BeginPlay();

	SetActorTickEnabled(false);
	NPCSpawnState = ENPCSpawnState::None;
	if (GetWorldPartitionStreamingSource()->TargetGrids.Num() > 0)
	{
		TargetGridName = GetWorldPartitionStreamingSource()->TargetGrids[0];
		NPCSpawnMaxCount = GetActorCountInGridCells(this, TargetGridName);
	}
}

void AUKNPCStreamingSourceProvider::BeginDestroy()
{	
	Super::BeginDestroy();
}

void AUKNPCStreamingSourceProvider::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void AUKNPCStreamingSourceProvider::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const bool bIsStreamingSourceEnabled = GetWorldPartitionStreamingSource()->IsStreamingSourceEnabled();
	const bool bIsStreamingCompleted = GetWorldPartitionStreamingSource()->IsStreamingCompleted();

	if (bIsStreamingSourceEnabled && bIsStreamingCompleted && NPCSpawnState == ENPCSpawnState::None)
	{
		GetLoadedActorsInStreamingSource(TargetGridName, SpawnerActors);
		if (SpawnerActors.Num() == NPCSpawnMaxCount)
		{
			NPCSpawnState = ENPCSpawnState::Spawn;
			// 플레이어와의 거리 로직 추가
			SpawnerActors.ValueSort([](const float& A, const float& B) {
				return A < B;
			});
		}
	}
	else if (NPCSpawnState == ENPCSpawnState::Spawn)
	{
		// 매 틱 마다 나누어서 액터 스폰
		const int32 MaxSpawnPerTick = 1;
		int32 SpawnCount = 0;
       
		for (auto It = SpawnerActors.CreateIterator(); It; ++It)
		{
			if (SpawnCount >= MaxSpawnPerTick)
			{
				break;
			}
          
			AUKNPCSpawner* NPCSpawner = It.Key();
			if (NPCSpawner)
			{
				NPCSpawner->Spawn();
				SpawnCount++;
			}

			// 스폰 후 맵에서 제거
			It.RemoveCurrent();
		}
		
		// 마지막
		if (SpawnerActors.IsEmpty())
		{
			NPCSpawnState = ENPCSpawnState::None;
			SetActorTickEnabled(false);
		}
	}
}

int32 AUKNPCStreamingSourceProvider::GetActorCountInGridCells(const UObject* WorldContextObject, FName GridName)
{
	int32 TotalCount = 0;
	UWorld* World = WorldContextObject->GetWorld();
	if (!World || !World->IsPartitionedWorld())
	{
		return TotalCount;
	}

	UWorldPartition* WorldPartition = World->GetWorldPartition();

	if (UWorldPartitionRuntimeHashSet* RuntimeHashSet = Cast<UWorldPartitionRuntimeHashSet>(WorldPartition->RuntimeHash))
	{
		TMap<FString, int32> CellActorCounts;
		RuntimeHashSet->ForEachStreamingCells([&](const UWorldPartitionRuntimeCell* Cell)
		{
			if (Cell && Cell->RuntimeCellData && Cell->RuntimeCellData->GridName == GridName)
			{
				TArray<FName> Actors = Cell->GetActors();
				CellActorCounts.Add(Cell->GetDebugName(), Actors.Num());
			}
			return true;
		});

		for (const TPair<FString, int32>& Pair : CellActorCounts)
		{
			TotalCount += Pair.Value;
		}
	}

	return TotalCount;
}

void AUKNPCStreamingSourceProvider::GetLoadedActorsInStreamingSource(const FName& GridName, TMap<AUKNPCSpawner*, float>& OutActors) const
{
	OutActors.Empty();

	UWorldPartitionStreamingSourceComponent* StreamingSourceComponent = GetWorldPartitionStreamingSource();
    if (!StreamingSourceComponent || !StreamingSourceComponent->IsStreamingSourceEnabled())
    {
        return;
    }

    UWorld* World = StreamingSourceComponent->GetWorld();
    UWorldPartition* WorldPartition = World ? World->GetWorldPartition() : nullptr;
    if (!WorldPartition)
    {
        return;
    }

    // 1. 스트리밍 소스로부터 현재 프레임의 쿼리 정보를 가져옴.
    FWorldPartitionStreamingSource CurrentSource;
    if (!StreamingSourceComponent->GetStreamingSource(CurrentSource))
    {
        return;
    }

    // 2. 월드에 현재 로드되어 있는 모든 스트리밍 레벨(셀)을 순회 [월드 파티션에서는 각 셀이 하나의 스트리밍 레벨에 해당]
	for (ULevelStreaming* StreamingLevel : World->GetStreamingLevels())
	{
		// 스트리밍 레벨이 유효하고, 실제로 로드가 완료되었는지 확인.
		if (StreamingLevel && StreamingLevel->IsLevelLoaded())
		{
			// 해당 스트리밍 레벨이 월드 파티션 셀인지 확인.
			const UWorldPartitionRuntimeCell* Cell = Cast<const UWorldPartitionRuntimeCell>(StreamingLevel->GetWorldPartitionCell());

			if (Cell && Cell->RuntimeCellData)
			{
				// 이 셀이 찾고 있는 GridName에 속하는지 확인.
				if (Cell->RuntimeCellData->GridName == GridName)
				{
					// 그리드가 일치하면, 이 셀(레벨)에 포함된 모든 액터를 결과 배열에 추가.
					ULevel* LoadedLevel = StreamingLevel->GetLoadedLevel();
					if (LoadedLevel)
					{
						// 유효한 액터만 추가합니다.
						for (AActor* Actor : LoadedLevel->Actors)
						{
							AUKNPCSpawner* NPCSpawner = Cast<AUKNPCSpawner>(Actor);
							if (IsValid(NPCSpawner))
							{
								OutActors.Add({NPCSpawner, 0.0f});
							}
						}
					}
				}
			}
		}
	}
}
