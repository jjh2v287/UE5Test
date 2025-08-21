// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/WorldPartitionStreamingSourceComponent.h"
#include "GameFramework/Actor.h"
#include "UKStreamingSourceProvider.h"
#include "Actors/Spawner/UKNPCSpawner.h"
#include "UKNPCStreamingSourceProvider.generated.h"

class UWorldPartitionStreamingSourceComponent;

UCLASS()
class UKGAME_API AUKNPCStreamingSourceProvider : public AUKStreamingSourceProvider
{
	GENERATED_BODY()

public:
	explicit AUKNPCStreamingSourceProvider(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void BeginDestroy() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "World Partition", meta = (WorldContext = "WorldContextObject", DevelopmentOnly))
	static int32 GetActorCountInGridCells(const UObject* WorldContextObject, FName GridName);

	void GetLoadedActorsInStreamingSource(const FName& GridName, TMap<AUKNPCSpawner*, float>& OutActors) const;

private:
	enum ENPCSpawnState : uint8
	{
		None,
		Spawn
	};
	
	ENPCSpawnState NPCSpawnState = ENPCSpawnState::None;
	int32 NPCSpawnMaxCount = 0;
	FName TargetGridName = NAME_None;

	UPROPERTY(Transient)
	TMap<AUKNPCSpawner*, float> SpawnerActors;
};