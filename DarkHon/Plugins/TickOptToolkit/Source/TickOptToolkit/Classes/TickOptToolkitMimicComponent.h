// Copyright 2021 Marcin Swiderski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TickOptToolkitTargetComponent.h"
#include "TickOptToolkitMimicComponent.generated.h"

UCLASS(ClassGroup=(TickOptToolkit), meta=(BlueprintSpawnableComponent),
	HideCategories=(Rendering, Physics, Collision, LOD, Activation, Cooking, ComponentReplication))
class TICKOPTTOOLKIT_API UTickOptToolkitMimicComponent : public UActorComponent
{
	GENERATED_BODY()

	friend class UTickOptToolkitTargetComponent;

	UPROPERTY(Transient)
	TArray<UActorComponent*> TickControlledComponents;
	
	UPROPERTY(Transient)
	TArray<FTickControlledTimeline> TickControlledTimelines;

	UPROPERTY(Transient)
	UTickOptToolkitTargetComponent* MimicTarget = nullptr;
	
	/** Should use tick settings to control the ticking of owning actor (primary actor tick). */
	UPROPERTY(EditAnywhere, BlueprintGetter="IsActorTickControl", BlueprintSetter="SetActorTickControl", Category="TickOptToolkit")
	bool bActorTickControl = true;

	/** Should use tick settings to control the ticking of all non-timeline components present in the owning actor during BeginPlay (primary component tick). */ 
	UPROPERTY(EditAnywhere, BlueprintGetter="IsComponentsTickControl", BlueprintSetter="SetComponentsTickControl", Category="TickOptToolkit")
	bool bComponentsTickControl = true;

	/** Should use tick settings to control the ticking of all timelines present in the owning actor during BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintGetter="IsTimelinesTickControl", BlueprintSetter="SetTimelinesTickControl", Category="TickOptToolkit")
	bool bTimelinesTickControl = true;

	/** Should sync to the world time all timelines present in the owning actor during BeginPlay.
	 *  This will ensure that the looping timelines will seem to tick even when not ticking. Non-looping timelines won't be affected.
	 */
	UPROPERTY(EditAnywhere, BlueprintGetter="IsSyncTimelinesToWorld", BlueprintSetter="SetSyncTimelinesToWorld", Category="TickOptToolkit", meta=(EditCondition="bTimelinesTickControl"))
	bool bSyncTimelinesToWorld = false;

public:
	/** Triggered when either distance zone or visibility changes. */
	UPROPERTY(BlueprintAssignable, Category="TickOptToolkit")
	FOnTickChange OnTickChanged;

	/** Triggered when distance zone changes. */
	UPROPERTY(BlueprintAssignable, Category="TickOptToolkit")
	FOnTickZoneChange OnTickZoneChanged;

	/** Triggered when visibility chagnes. */
	UPROPERTY(BlueprintAssignable, Category="TickOptToolkit")
	FOnTickVisibilityChange OnTickVisibilityChanged;

    /** Triggered when tick enables/disables with tick settings. */
	UPROPERTY(BlueprintAssignable, Category="TickOptToolkit")
	FOnTickEnabledChange OnTickEnabledChanged;

public:
	UTickOptToolkitMimicComponent();
	
protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnNewPawn(APawn* Pawn);
	
public:
	UFUNCTION(BlueprintGetter, Category="TickOptToolkit")
	bool IsActorTickControl() const { return bActorTickControl; }
	UFUNCTION(BlueprintSetter, Category="TickOptToolkit")
	void SetActorTickControl(bool bInActorTickControl);
	
	UFUNCTION(BlueprintGetter, Category="TickOptToolkit")
	bool IsComponentsTickControl() const { return bComponentsTickControl; }
	UFUNCTION(BlueprintSetter, Category="TickOptToolkit")
	void SetComponentsTickControl(bool bInComponentsTickControl);
	
	UFUNCTION(BlueprintGetter, Category="TickOptToolkit")
	bool IsTimelinesTickControl() const { return bTimelinesTickControl; }
	UFUNCTION(BlueprintSetter, Category="TickOptToolkit")
	void SetTimelinesTickControl(bool bInTimelinesTickControl);
	
	UFUNCTION(BlueprintGetter, Category="TickOptToolkit")
	bool IsSyncTimelinesToWorld() const { return bSyncTimelinesToWorld; }
	UFUNCTION(BlueprintSetter, Category="TickOptToolkit")
	void SetSyncTimelinesToWorld(bool bInSyncTimelinesToWorld);

	/** Register a target component for this component to mimic.
	 *
	 *  @Target Target component this component should mimic
	 */
	UFUNCTION(BlueprintCallable, Category="TickOptToolkit")
	void RegisterMimicTarget(UTickOptToolkitTargetComponent* Target);

	/** Unregister currently mimicked target component. */
	UFUNCTION(BlueprintCallable, Category="TickOptToolkit")
	void UnregisterMimicTarget();
	
	/** Add non-timeline component to list of controlled components, meaning their ticking will be controlled using tick settings.
	 *  This function can be used when bComponentsTickControl is false, or when you dynamically create a component after BeginPlay.
	 *
	 *  @Component Component that should be controlled using tick settings
	 */
	UFUNCTION(BlueprintCallable, Category="TickOptToolkit")
	void AddComponentTickControl(UActorComponent* Component);

	/** Remove non-timeline component from list of controlled components, meaning their ticking will NOT be controlled using tick settings.
	 *  This function can be used to remove components added with AddComponentTickControl, or components added using bComponentTickControl flag.
	 *  In the later case please mind the order of execution of BeginPlay functions (first components, than owning actor).
	 *
	 *  @Component Component that should NOT be controlled using tick settings
	 */
	UFUNCTION(BlueprintCallable, Category="TickOptToolkit")
	void RemoveComponentTickControl(UActorComponent* Component);

	/** Add timeline to list of controlled timelines, meaning their ticking will be controlled using tick settings.
	 *  This function can be used when bTimelinesTickControl is false, or when you dynamically create a component after BeginPlay.
	 *
	 *  @Timeline Timeline that should be controlled using tick settings
	 *  @SyncToWorld Should sync the timeline to the world time
	 */
	UFUNCTION(BlueprintCallable, Category="TickOptToolkit")
	void AddTimelineTickControl(UTimelineComponent* Timeline, bool bSyncToWorld);

	/** Remove timeline from list of controlled timelines, meaning their ticking will NOT be controlled using tick settings.
	 *  This function can be used to remove timelines added with AddTimelineTickControl, or timelines added using bTimelinesTickControl flag.
	 *  In the later case please mind the order of execution of BeginPlay functions (first components, than owning actor).
	 *
	 *  @Timeline Timeline that should NOT be controlled using tick settings
	 */
	UFUNCTION(BlueprintCallable, Category="TickOptToolkit")
	void RemoveTimelineTickControl(UTimelineComponent* Timeline);

private:
	void OnUpdated(bool bTickEnabled, float TickInterval, int TickZone, bool bZoneUpdated, bool bTickVisible, bool bVisUpdated);
	void OnBalanceTickInterval(float TickInterval);
};
