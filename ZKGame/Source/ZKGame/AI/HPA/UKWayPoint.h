// Copyright Kong Studios, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "NavigationInvokerComponent.h"
#include "UKNavigationDefine.h"
#include "GameFramework/Actor.h"
#include "UKWayPoint.generated.h"


class UUKNavigationManager;

UCLASS()
class ZKGAME_API AUKWayPoint : public AActor
{
	GENERATED_BODY()
public:
	AUKWayPoint(const FObjectInitializer& ObjectInitializer);
    
	virtual void BeginPlay() override;
	virtual void Destroyed() override;
	virtual void Tick(float DeltaTime) override;

	// This is the ID of the cluster that the Wei Point belongs to. -1 or index_none can mean that it is not assigned.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waypoint")
	int32 ClusterID = INDEX_NONE;
	
	// Information of connected Wey Points
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Waypoint")
	TArray<AUKWayPoint*> PathPoints;

	virtual bool ShouldTickIfViewportsOnly() const override { return true; };

	FWayPointHandle GetWayPointHandle() const
	{
		return WayPointHandle;
	}

	void SetWayPointHandle(FWayPointHandle Handle)
	{
		WayPointHandle = Handle;
	}
	
protected:
	UPROPERTY(VisibleDefaultsOnly)
	UNavigationInvokerComponent* InvokerComponent{nullptr};

	FWayPointHandle WayPointHandle;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};