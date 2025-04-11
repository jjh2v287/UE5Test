// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HierarchicalHashGrid2D.h"
#include "StructUtils/InstancedStruct.h"
#include "Subsystems/WorldSubsystem.h"
#include "UKAgentManager.generated.h"

class AUKAgent;

#pragma region Agent HashGrid
// 1. Agent handle definition (FSmartObjectHandle imitation)
USTRUCT(BlueprintType)
struct ZKGAME_API FAgentHandle
{
    GENERATED_BODY()
public:
    FAgentHandle() : ID(0) {}

    bool IsValid() const { return ID != 0; }
    void Invalidate() { ID = 0; }

    friend FString LexToString(const FAgentHandle Handle)
    {
        return FString::Printf(TEXT("Agent_0x%016llX"), Handle.ID);
    }

    bool operator==(const FAgentHandle Other) const { return ID == Other.ID; }
    bool operator!=(const FAgentHandle Other) const { return !(*this == Other); }
    bool operator<(const FAgentHandle Other) const { return ID < Other.ID; }

    friend uint32 GetTypeHash(const FAgentHandle Handle)
    {
        return CityHash32(reinterpret_cast<const char*>(&Handle.ID), sizeof Handle.ID);
    }

private:
    // Only the UUKAgentManager can set the ID
    friend class UUKAgentManager;

    explicit FAgentHandle(const uint64 InID) : ID(InID) {}
    
    UPROPERTY(VisibleAnywhere, Category = Agent)
    uint64 ID;

public:
    static const FAgentHandle Invalid;
};

// THierarchicalHashGrid2D Type Definition (FSmartObjectHashGrid2D imitation)
using FAgentHashGrid2D = THierarchicalHashGrid2D<2, 4, FAgentHandle>;

// 2. Spatial split data definition (fsmartobjectSpatialEntryData imitation)
USTRUCT()
struct ZKGAME_API FAgentSpatialEntryData
{
    GENERATED_BODY()
    virtual ~FAgentSpatialEntryData() = default;
};

// 3. Space split data for hash grid (FSmartObjectHashGridEntryData imitation)
USTRUCT()
struct ZKGAME_API FAgentHashGridEntryData : public FAgentSpatialEntryData
{
    GENERATED_BODY()

    // Grid cell location storage
    FAgentHashGrid2D::FCellLocation CellLoc;
};

// 4. Agent Runtime Data Definition (FSmartObjectRuntime imitation)
USTRUCT()
struct ZKGAME_API FAgentRuntimeData
{
    GENERATED_BODY()

public:
    FAgentRuntimeData() = default;

    explicit FAgentRuntimeData(AUKAgent* InAgent, const FAgentHandle InHandle)
        : AgentObject(InAgent)
        , Handle(InHandle)
    {
        // Initialization with space data for hash grid
        SpatialEntryData.InitializeAs<FAgentHashGridEntryData>();
    }

    UPROPERTY(VisibleAnywhere, Category = Agent)
    TWeakObjectPtr<AUKAgent> AgentObject;

    UPROPERTY(VisibleAnywhere, Category = Agent)
    FAgentHandle Handle;

    UPROPERTY(VisibleAnywhere, Category = Agent)
    FTransform Transform;

    UPROPERTY(VisibleAnywhere, Category = Agent)
    FBox Bounds;

    UPROPERTY()
    FInstancedStruct SpatialEntryData;
};
#pragma endregion Agent HashGrid

UCLASS()
class ZKGAME_API UUKAgentManager : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

    // FTickableGameObject implementation Begin
    virtual TStatId GetStatId() const override;
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override { return true; }
    virtual bool IsTickableInEditor() const override { return false; }
    // FTickableGameObject implementation End

	// --- Agent management ---
	FAgentHandle RegisterAgent(AUKAgent* Agent);
	bool UnregisterAgent(AUKAgent* Agent);
    void AgentMoveUpdate(const AUKAgent* Agent);

    UFUNCTION(BlueprintCallable, Category = "Agent", meta = (DisplayName = "Find Agent In Range"))
    TArray<AUKAgent*> FindCloseAgentRange(const AUKAgent* Agent, const float Range = 400.0f) const;

private:
    // Registered Way Point Management Map (handle-> runtime data)
        UPROPERTY(Transient)
        TMap<FAgentHandle, FAgentRuntimeData> RuntimeAgents;

        // Counter for creating a unique handle ID
        uint64 NextHandleID = 1;
    
        // hash grid instance
        FAgentHashGrid2D AgentGrid;
};
