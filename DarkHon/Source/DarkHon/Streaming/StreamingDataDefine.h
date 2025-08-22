#pragma once

#include "CoreMinimal.h"

#include "WorldPartition/WorldPartitionHandle.h"

class FManualActorLoader
{
public:
	FManualActorLoader(UWorld* InWorld);

	void FindActorGuidsInVolume(const FName GridName, const FBox& BoundingBox, TArray<FGuid>& OutActorGuids);
	
	// 지정된 GUID의 액터를 로드 요청합니다.
	void LoadActor(const FGuid& ActorGuid);

	// 지정된 GUID의 액터들을 로드 요청합니다.
	void LoadActors(const TArray<FGuid>& ActorGuids);

	// 지정된 GUID의 액터를 언로드 요청합니다.
	void UnloadActor(const FGuid& ActorGuid);

	// 지정된 GUID의 액터들을 언로드 요청합니다.
	void UnloadActors(const TArray<FGuid>& ActorGuids);

	// 모든 로드된 액터를 언로드합니다.
	void UnloadAllActors();

	// 액터가 완전히 로드되었는지 확인합니다.
	bool IsActorLoaded(const FGuid& ActorGuid) const;

	// 로드된 액터 객체를 가져옵니다. 로드 중이거나 실패하면 nullptr을 반환합니다.
	AActor* GetLoadedActor(const FGuid& ActorGuid) const;

private:
	// 로드된 액터들에 대한 강한 참조(FWorldPartitionReference)를 유지하기 위한 맵
	TMap<FGuid, FWorldPartitionReference> LoadedActorReferences;

	TWeakObjectPtr<UWorld> World;
};