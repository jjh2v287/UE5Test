#include "StreamingDataDefine.h"
#include "WorldPartition/WorldPartition.h"
#include "WorldPartition/WorldPartitionRuntimeCell.h"
#include "WorldPartition/WorldPartitionRuntimeCellData.h"
#include "WorldPartition/RuntimeHashSet/WorldPartitionRuntimeCellDataHashSet.h"
#include "WorldPartition/WorldPartitionActorDescInstance.h"
#include "WorldPartition/WorldPartitionSubsystem.h"

FManualActorLoader::FManualActorLoader(UWorld* InWorld)
    : World(InWorld)
{
}

void FManualActorLoader::FindActorGuidsInVolume(const FName GridName, const FBox& BoundingBox, TArray<FGuid>& OutActorGuids)
{
    UWorldPartitionSubsystem* WorldPartitionSubsystem = World->GetSubsystem<UWorldPartitionSubsystem>();
    if (!WorldPartitionSubsystem)
    {
        return;
    }

    // 1. 가상의 쿼리 소스를 생성합니다.
    FWorldPartitionStreamingQuerySource QuerySource;
    QuerySource.bSpatialQuery = true;
    QuerySource.Location = BoundingBox.GetCenter();
    QuerySource.Radius = BoundingBox.GetExtent().Size(); // 박스를 포함하는 구의 반지름
    // 더 정확하게 하려면 Shapes를 사용하여 박스 모양을 직접 정의할 수 있습니다.

    TArray<const IWorldPartitionCell*> IntersectingCells;

    // 2. 월드 파티션에 교차하는 셀을 쿼리합니다.
    // 참고: 여러 월드 파티션 인스턴스가 있을 수 있으므로 서브시스템을 통해 쿼리합니다.
    // 이 예제에서는 메인 월드 파티션만 가정합니다.
    if (UWorldPartition* WorldPartition = World->GetWorldPartition())
    {
        WorldPartition->GetIntersectingCells({ QuerySource }, IntersectingCells);
    }

    // 3. 각 셀에서 액터 GUID를 추출합니다.
    for (const IWorldPartitionCell* CellInterface : IntersectingCells)
    {
        const UWorldPartitionRuntimeCell* Cell = Cast<const UWorldPartitionRuntimeCell>(CellInterface);
        if (Cell)
        {
            // UWorldPartitionRuntimeCell* Cell 변수가 있다고 가정
            if (const UWorldPartitionRuntimeCellDataHashSet* CellData = Cast<UWorldPartitionRuntimeCellDataHashSet>(Cell->RuntimeCellData))
            {
                FName CellGridName = CellData->GridName;
                if (CellGridName == GridName)
                {
                    // 이 셀은 "MyCustomGrid"에 속함
                }
            }
        }
    }
}

void FManualActorLoader::LoadActor(const FGuid& ActorGuid)
{
    if (!ActorGuid.IsValid() || LoadedActorReferences.Contains(ActorGuid))
    {
        return;
    }

    if (UWorldPartition* WorldPartition = World->GetWorldPartition())
    {
        // FWorldPartitionHandle을 사용하여 액터 디스크립터가 유효한지 먼저 확인합니다.
        FWorldPartitionHandle ActorHandle(WorldPartition, ActorGuid);
        if (ActorHandle.IsValid())
        {
            // FWorldPartitionReference를 생성합니다.
            // 이 객체가 생성되는 순간, 하드 참조 카운트가 증가하고 월드 파티션은 로딩을 시작합니다.
            FWorldPartitionReference ActorReference(WorldPartition, ActorGuid);
            LoadedActorReferences.Add(ActorGuid, MoveTemp(ActorReference));
        }
    }
}

void FManualActorLoader::LoadActors(const TArray<FGuid>& ActorGuids)
{
    // 여러 액터를 동시에 로드/언로드할 때는 FWorldPartitionLoadingContext::FDeferred를 사용하여
    // 작업을 그룹화하고 프레임 끝에 한 번에 처리하는 것이 효율적입니다.
    FWorldPartitionLoadingContext::FDeferred LoadingContext;
    for (const FGuid& ActorGuid : ActorGuids)
    {
        LoadActor(ActorGuid);
    }
}

void FManualActorLoader::UnloadActor(const FGuid& ActorGuid)
{
    // 맵에서 FWorldPartitionReference를 제거합니다.
    // TMap에서 제거되면서 참조 객체의 소멸자가 호출되고, 하드 참조 카운트가 감소합니다.
    // 카운트가 0이 되면 월드 파티션은 언로딩을 시작합니다.
    LoadedActorReferences.Remove(ActorGuid);
}

void FManualActorLoader::UnloadActors(const TArray<FGuid>& ActorGuids)
{
    FWorldPartitionLoadingContext::FDeferred LoadingContext;
    for (const FGuid& ActorGuid : ActorGuids)
    {
        UnloadActor(ActorGuid);
    }
}

void FManualActorLoader::UnloadAllActors()
{
    FWorldPartitionLoadingContext::FDeferred LoadingContext;
    LoadedActorReferences.Empty();
}

bool FManualActorLoader::IsActorLoaded(const FGuid& ActorGuid) const
{
    if (const FWorldPartitionReference* Reference = LoadedActorReferences.Find(ActorGuid))
    {
        return Reference->IsLoaded();
    }
    return false;
}

AActor* FManualActorLoader::GetLoadedActor(const FGuid& ActorGuid) const
{
    if (const FWorldPartitionReference* Reference = LoadedActorReferences.Find(ActorGuid))
    {
        // IsLoaded()를 먼저 확인하거나, FlushAsyncLoading을 호출해야 안전하게 액터를 가져올 수 있습니다.
        // 여기서는 간단하게 IsLoaded()로 확인합니다.
        if (Reference->IsLoaded())
        {
            return Reference->GetActor();
        }
    }
    return nullptr;
}