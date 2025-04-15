// Fill out your copyright notice in the Description page of Project Settings.

#include "UKAgentManager.h"
#include "UKAgent.h"

const FAgentHandle FAgentHandle::Invalid = FAgentHandle(0);

void UUKAgentManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UUKAgentManager::Deinitialize()
{
	Super::Deinitialize();
}

TStatId UUKAgentManager::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UKAgentManager, STATGROUP_Tickables);
}

void UUKAgentManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Hash Grid
	const TSet<FAgentHashGrid2D::FCell>& AllCells = AgentGrid.GetCells();
	for (const FAgentHashGrid2D::FCell& Cell : AllCells)
	{
		FBox CellBounds = AgentGrid.CalcCellBounds(FAgentHashGrid2D::FCellLocation(Cell.X, Cell.Y, Cell.Level));
		DrawDebugBox(GetWorld(), CellBounds.GetCenter(), CellBounds.GetExtent(), GColorList.GetFColorByIndex(Cell.Level), false, -1.0f, SDPG_Foreground);
	}
}

FAgentHandle UUKAgentManager::RegisterAgent(AUKAgent* Agent)
{
	// Create a new handle
	FAgentHandle NewHandle = FAgentHandle(NextHandleID++);
	if (!NewHandle.IsValid())
	{
		return FAgentHandle::Invalid;
	}
	
	//----------- HashGrid -----------
	if (RuntimeAgents.Contains(Agent->GetAgentHandle()))
	{
		return Agent->GetAgentHandle();
	}

	// Creating and adding to the runtime data to the map
	FAgentRuntimeData& RuntimeData = RuntimeAgents.Emplace(NewHandle, FAgentRuntimeData(Agent, NewHandle));

	// Boundary box and transform calculation/storage
	const FVector Location = Agent->GetActorLocation();
	const FVector Extent(50.0f);
	FBox Box = FBox(Location - Extent, Location + Extent);
	RuntimeData.Bounds = Box;
	RuntimeData.Transform = Agent->GetActorTransform();

	// Add to the Hash grid
	FAgentHashGridEntryData* GridEntryData = RuntimeData.SpatialEntryData.GetMutablePtr<FAgentHashGridEntryData>();
	if (GridEntryData)
	{
		GridEntryData->CellLoc = AgentGrid.Add(NewHandle, RuntimeData.Bounds);
	}
	else
	{
		// Registration failure processing: removal from the map, etc.
		RuntimeAgents.Remove(NewHandle);
		return FAgentHandle::Invalid;
	}

	return NewHandle;
}

bool UUKAgentManager::UnregisterAgent(AUKAgent* Agent)
{
	if (!IsValid(Agent))
	{
		return false;
	}

	//----------- HashGrid -----------
	FAgentRuntimeData RuntimeData;
	if (RuntimeAgents.RemoveAndCopyValue(Agent->GetAgentHandle(), RuntimeData))
	{
		const FAgentHashGridEntryData* GridEntryData = RuntimeData.SpatialEntryData.GetPtr<FAgentHashGridEntryData>();
		if (GridEntryData)
		{
			AgentGrid.Remove(Agent->GetAgentHandle(), GridEntryData->CellLoc);
		}
		Agent->SetAgentHandle(FAgentHandle::Invalid);
		return true;
	}

	return false;
}

void UUKAgentManager::AgentMoveUpdate(const AUKAgent* Agent)
{
	if (Agent->GetAgentHandle() == FAgentHandle::Invalid)
	{
		return;
	}
	
	if (!RuntimeAgents.Contains(Agent->GetAgentHandle()))
	{
		return;
	}

	const FVector Location = Agent->GetActorLocation();
	const FVector Extent(50.0f);
	FBox NewBounds = FBox(Location - Extent, Location + Extent);

	FAgentRuntimeData* RuntimeData = RuntimeAgents.Find(Agent->GetAgentHandle());
	FAgentHashGridEntryData* GridEntryData = RuntimeData->SpatialEntryData.GetMutablePtr<FAgentHashGridEntryData>();
	if (GridEntryData)
	{
		// Move 메서드를 사용해 그리드 내 위치 업데이트
		GridEntryData->CellLoc = AgentGrid.Move(Agent->GetAgentHandle(), GridEntryData->CellLoc, NewBounds);
	}
}

TArray<AUKAgent*> UUKAgentManager::FindCloseAgentRange(const AUKAgent* Agent, const float Range) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(AUKAgent_FindCloseAgentRange);
	TArray<AUKAgent*> Agents;

	FVector Location = Agent->GetActorLocation();
	FVector SearchExtent(Range);
	FBox QueryBox = FBox(Location - SearchExtent, Location + SearchExtent);

	// Grid query
	TArray<FAgentHandle> FoundHandlesInGrid;
	AgentGrid.QuerySmall(QueryBox, FoundHandlesInGrid);

	// Grid results verification (further filtering, such as the existence and activation of runtime data)
	for (const FAgentHandle& Handle : FoundHandlesInGrid)
	{
		const FAgentRuntimeData* RuntimeData = RuntimeAgents.Find(Handle);
		if (RuntimeData && RuntimeData->AgentObject.IsValid())
		{
			if (RuntimeData->AgentObject.Get() != Agent)
			{
				Agents.Emplace(RuntimeData->AgentObject.Get());
			}
		}
	}

	return Agents;
}
