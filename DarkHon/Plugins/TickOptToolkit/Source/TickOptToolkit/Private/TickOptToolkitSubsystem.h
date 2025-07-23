// Copyright 2021 Marcin Swiderski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/EngineVersionComparison.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "TickOptToolkitSettings.h"
#include "TickOptToolkitFocusComponent.h"
#include "TickOptToolkitSubsystem.generated.h"

class UTickOptToolkitTargetComponent;

struct FTickFocusCache
{
	FVector Location;
	FVector Forward;
	uint16 Layer;

	FORCEINLINE FTickFocusCache(const UTickOptToolkitFocusComponent* Focus);
};

typedef TArray<FTickFocusCache, TInlineAllocator<8>> TTickFocusCachesArray;

struct FTickOptToolkitSphereRangeCache
{
	FVector Location;
	float RadiusSqr;
	uint16 LayerMask;
	
	FORCEINLINE FTickOptToolkitSphereRangeCache(const UTickOptToolkitTargetComponent* Target);
	FORCEINLINE void UpdateRadius(const UTickOptToolkitTargetComponent* Target);
	FORCEINLINE bool IsInRange(const FTickFocusCache& Focus) const;
};

struct FTickOptToolkitSphereZoneCache
{
	FVector Location;
	float InnerRadiusSqr;
	float OuterRadiusSqr;
	uint16 LayerMask;
	
	FORCEINLINE FTickOptToolkitSphereZoneCache(const UTickOptToolkitTargetComponent* Target);
	FORCEINLINE void UpdateRadius(const UTickOptToolkitTargetComponent* Target);
	FORCEINLINE int GetZoneDiff(const FTickFocusCache& Focus) const;
};

struct FTickOptToolkitBoxRangeCache
{
	FVector Location;
	FVector Extents;
	float Sin;
	float Cos;
	uint16 LayerMask;
	
	FORCEINLINE FTickOptToolkitBoxRangeCache(const UTickOptToolkitTargetComponent* Target);
	FORCEINLINE void UpdateExtents(const UTickOptToolkitTargetComponent* Target);
	FORCEINLINE bool IsInRange(const FTickFocusCache& Focus) const;
};

struct FTickOptToolkitBoxZoneCache
{
	FVector Location;
	FVector InnerExtents;
	float OuterSize;
	float Sin;
	float Cos;
	uint16 LayerMask;
	
	FORCEINLINE FTickOptToolkitBoxZoneCache(const UTickOptToolkitTargetComponent* Target);
	FORCEINLINE void UpdateExtents(const UTickOptToolkitTargetComponent* Target);
	FORCEINLINE int GetZoneDiff(const FTickFocusCache& Focus) const;
};

struct FTickByVisFrontCache
{
	FVector Location;
	uint16 LayerMask;

	FORCEINLINE FTickByVisFrontCache(const UTickOptToolkitTargetComponent* Target);
	FORCEINLINE bool IsVisible(const FTickFocusCache& Focus) const;
};

UCLASS()
class TICKOPTTOOLKIT_API UTickOptToolkitSubsystem
	: public UWorldSubsystem
	, public FTickableGameObject
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	UTickOptToolkitSettings* Settings;
	
	UPROPERTY(Transient)
	TArray<UTickOptToolkitFocusComponent*> Focuses;

	UPROPERTY(Transient)
	TArray<UTickOptToolkitTargetComponent*> SphereRangeTargets;
	TArray<FTickOptToolkitSphereRangeCache> SphereRangeCaches;
	TArray<bool> bSpheresInRange;

	UPROPERTY(Transient)
	TArray<UTickOptToolkitTargetComponent*> SphereZoneTargets;
	TArray<FTickOptToolkitSphereZoneCache> SphereZoneCaches;

	UPROPERTY(Transient)
	TArray<UTickOptToolkitTargetComponent*> BoxRangeTargets;
	TArray<FTickOptToolkitBoxRangeCache> BoxRangeCaches;
	TArray<bool> bBoxesInRange;

	UPROPERTY(Transient)
	TArray<UTickOptToolkitTargetComponent*> BoxZoneTargets;
	TArray<FTickOptToolkitBoxZoneCache> BoxZoneCaches;

	UPROPERTY(Transient)
	TArray<UTickOptToolkitTargetComponent*> VisFrontTargets;
	TArray<FTickByVisFrontCache> VisFrontCaches;
	TArray<bool> bVisibleFront;

	UPROPERTY(Transient)
	TArray<UTickOptToolkitTargetComponent*> VisRenderedTargets;
#if UE_VERSION_OLDER_THAN(5, 6, 0)
	TArray<float const*> VisRenderedLastRenderTime;
#else
	TArray<struct FActorLastRenderTime const*> VisRenderedLastRenderTime;
#endif
	TArray<bool> bVisibleRendered;

	// ReSharper disable once CppUE4ProbableMemoryIssuesWithUObjectsInContainer
	TArray<UTickOptToolkitTargetComponent*> UpdatedTargets;

	TArray<TArray<UTickOptToolkitTargetComponent*>> BalancingBins;
	TArray<int> BalancingBinsPermutation;
	int BalancingBinsNextInsert;
	int BalancingBinsNextUpdate;
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	void RegisterFocus(UTickOptToolkitFocusComponent* Focus);
	void UnregisterFocus(UTickOptToolkitFocusComponent* Focus);

	void RegisterTarget(UTickOptToolkitTargetComponent* Target);
	void UnregisterTarget(UTickOptToolkitTargetComponent* Target);
	void UpdateTargetLocation(const UTickOptToolkitTargetComponent* Target);
	void UpdateTargetLayerMask(const UTickOptToolkitTargetComponent* Target);

	void BalanceTargetTickInterval(UTickOptToolkitTargetComponent* Target);
	void CancelBalancingTargetTickInterval(UTickOptToolkitTargetComponent* Target);
	
	virtual ETickableTickType GetTickableTickType() const override;
	virtual TStatId GetStatId() const override;
	virtual void Tick(float DeltaTime) override;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	void OnScalabilityCVarChanged(IConsoleVariable*);
	
	FORCEINLINE static bool IsRange(const UTickOptToolkitTargetComponent* Target);
	
	FORCEINLINE void AddSphereRangeTarget(UTickOptToolkitTargetComponent* Target);
	FORCEINLINE void RemoveSphereRangeTarget(UTickOptToolkitTargetComponent* Target);

	FORCEINLINE void AddSphereZoneTarget(UTickOptToolkitTargetComponent* Target);
	FORCEINLINE void RemoveSphereZoneTarget(UTickOptToolkitTargetComponent* Target);

	FORCEINLINE void AddBoxRangeTarget(UTickOptToolkitTargetComponent* Target);
	FORCEINLINE void RemoveBoxRangeTarget(UTickOptToolkitTargetComponent* Target);

	FORCEINLINE void AddBoxZoneTarget(UTickOptToolkitTargetComponent* Target);
	FORCEINLINE void RemoveBoxZoneTarget(UTickOptToolkitTargetComponent* Target);

	FORCEINLINE void AddVisFrontTarget(UTickOptToolkitTargetComponent* Target);
	FORCEINLINE void RemoveVisFrontTarget(UTickOptToolkitTargetComponent* Target);

	FORCEINLINE void AddVisRenderedTarget(UTickOptToolkitTargetComponent* Target);
	FORCEINLINE void RemoveVisRenderedTarget(UTickOptToolkitTargetComponent* Target);
	
	template<bool bMultipleFocuses, bool bSupportLayers, typename CacheType>
	void UpdateTargetsInRange(const TTickFocusCachesArray& FocusCaches, const TArray<UTickOptToolkitTargetComponent*>& Targets, const TArray<CacheType>& Caches, TArray<bool>& bInRange);

	template<bool bMultipleFocuses, bool bSupportLayers, typename CacheType>
	void UpdateTargetsZoneDiff(const TTickFocusCachesArray& FocusCaches, const TArray<UTickOptToolkitTargetComponent*>& Targets, const TArray<CacheType>& Caches);

	template<bool bMultipleFocuses, bool bSupportLayers, typename CacheType>
	void UpdateTargetsVisibility(const TTickFocusCachesArray& FocusCaches, const TArray<UTickOptToolkitTargetComponent*>& Targets, const TArray<CacheType>& Caches, TArray<bool>& bVisible);

	void UpdateRenderVisibility();
	
	FORCEINLINE void UpdateZone(UTickOptToolkitTargetComponent* Target, int ZoneDiff);
	FORCEINLINE void UpdateVisibility(UTickOptToolkitTargetComponent* Target, bool bVisible);

	FORCEINLINE void UpdateZoneCache(UTickOptToolkitTargetComponent* Target);
};
