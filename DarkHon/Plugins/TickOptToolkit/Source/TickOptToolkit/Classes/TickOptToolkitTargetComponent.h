// Copyright 2021 Marcin Swiderski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TickOptToolkitFocusComponent.h"
#include "TickOptToolkitSettings.h"
#include "Components/SceneComponent.h"
#include "Components/TimelineComponent.h"
#include "TickOptToolkitTargetComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnTickChange, bool, bZoneChanged, bool, bVisibilityChanged, int, Zone, bool, bVisible);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTickZoneChange, int, Zone);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTickVisibilityChange, bool, bVisible);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTickEnabledChange, bool, bTickEnabled);

class UTickOptToolkitMimicComponent;
class UTickOptToolkitSubsystem;
class UTimelineComponent;
class FTickOptToolkitComponentCustomization;

/** Tick by distance mode. */
UENUM()
enum class ETickOptToolkitDistanceMode : uint8
{
	/** Distance check disabled */
	None,
	/** Sphere Zone */
	Sphere,
	/** Box Zone (Z Axis Aligned) */
	Box,
};

/** Tick by visibility mode. */
UENUM()
enum class ETickByVisibilityMode : uint8
{
	/** Visibility check disabled */
	None,
	/** Visible when in front */
	Front,
	/** Visible when recently rendered */
	Rendered,
};

/** Tick settings to use with calculated distance zone / visibility. */
USTRUCT(BlueprintType)
struct FTickOptToolkitTickSettings
{
	GENERATED_BODY()

	/** Tick interval when visible */
	UPROPERTY(EditAnywhere, Category="TickOptToolkit", meta=(DisplayPriority=0))
	float IntervalVisible = 0.0f;

	/** Tick interval when hidden */
	UPROPERTY(EditAnywhere, Category="TickOptToolkit", meta=(DisplayPriority=1))
	float IntervalHidden = 0.0f;

	/** Tick enabled when visible */
	UPROPERTY(EditAnywhere, Category="TickOptToolkit", meta=(DisplayPriority=0))
	bool bEnabledVisible = true;

	/** Tick enabled when hidden */
	UPROPERTY(EditAnywhere, Category="TickOptToolkit", meta=(DisplayPriority=1))
	bool bEnabledHidden = false;
};

USTRUCT()
struct FTickOptToolkitOptimizationLevel
{
	GENERATED_BODY()

	/** Distance sphere radius */
	UPROPERTY(EditAnywhere, Category="TickOptToolkit")
	float SphereRadius = 1000.0f;

	/** Distance box extents */
	UPROPERTY(EditAnywhere, Category="TickOptToolkit")
	FVector BoxExtents = FVector(1000.0f, 1000.0f, 1000.0f);

	/** Tick by distance buffer size.
	 *  It's added to distance check when the focus is moving further away. Helps to mitigate constant switching issue
	 *  when the focus is exactly at the tick distance.
	 */
	UPROPERTY(EditAnywhere, Category="TickOptToolkit")
	float BufferSize = 100.0f;

	/** Sizes for additional distance zones.
	 *  The sizes are stacked up on the basic distance shape as an offset.
	 */
	UPROPERTY(EditAnywhere, EditFixedSize, Category="TickOptToolkit")
	TArray<float> MidZoneSizes;
	
	/** Distance zone / visibility dependent tick settings used when controlling ticks of owning actor and its components. */
	UPROPERTY(EditAnywhere, Category="TickOptToolkit")
	TArray<FTickOptToolkitTickSettings> TickSettings = {{0.0f, 0.0f, true, false}, {0.0f, 0.0f, false, false}};
};

USTRUCT()
struct FTickControlledTimeline
{
	GENERATED_BODY()

	UPROPERTY()
	UTimelineComponent* Timeline = nullptr;

	UPROPERTY()
	bool bSyncTimelineToWorld = false;

	friend bool operator==(const FTickControlledTimeline& Lhs, const FTickControlledTimeline& Rhs)
	{
		return Lhs.Timeline == Rhs.Timeline;
	}
};

#define TOT_OPT_LEVEL_GET(Property) ([this]() -> const auto& { \
	const FTickOptToolkitOptimizationLevel* OptimizationLevel = GetCurrentOptimizationLevel(); \
	return OptimizationLevel == &DefaultOptimizationLevel ? Property : OptimizationLevel->Property; \
}())

UCLASS(Blueprintable, ClassGroup=(TickOptToolkit), meta=(BlueprintSpawnableComponent),
	HideCategories=(Rendering, Physics, Collision, LOD, Activation, Cooking, ComponentReplication))
class TICKOPTTOOLKIT_API UTickOptToolkitTargetComponent : public USceneComponent
{
	GENERATED_BODY()

	friend class UTickOptToolkitSubsystem;
	friend class UTickOptToolkitMimicComponent;
	friend class FTickOptToolkitComponentCustomization;

	static FTickOptToolkitOptimizationLevel DefaultOptimizationLevel;
	
	UPROPERTY(Transient)
	UTickOptToolkitSubsystem* Subsystem;

	int TickByZoneIndex = INDEX_NONE;
	int TickByVisIndex = INDEX_NONE;
	int BalancingBinIndex = INDEX_NONE;

	bool bTickOptRegistered = false;
	bool bZoneUpdated = false;
	bool bVisUpdated = false;

	const FTickOptToolkitOptimizationLevel* CurrentOptimizationLevel = nullptr;
	
	UPROPERTY(Transient)
	TArray<UActorComponent*> TickControlledComponents;
	
	UPROPERTY(Transient)
	TArray<FTickControlledTimeline> TickControlledTimelines;

	UPROPERTY(Transient)
	TArray<UTickOptToolkitMimicComponent*> Mimics;

	/** Tick by distance mode */
	UPROPERTY(EditAnywhere, BlueprintGetter="GetDistanceMode", BlueprintSetter="SetDistanceMode", Category="TickOptToolkit")
	ETickOptToolkitDistanceMode DistanceMode = ETickOptToolkitDistanceMode::Sphere;

	/** Distance sphere radius */
	UPROPERTY(EditAnywhere, BlueprintGetter="GetSphereRadius", BlueprintSetter="SetSphereRadius", Category="TickOptToolkit", meta=(EditCondition="DistanceMode==ETickOptToolkitDistanceMode::Sphere"))
	float SphereRadius = 1000.0f;

	/** Distance box extents */
	UPROPERTY(EditAnywhere, BlueprintGetter="GetBoxExtents", BlueprintSetter="SetBoxExtents", Category="TickOptToolkit", meta=(EditCondition="DistanceMode==ETickOptToolkitDistanceMode::Box"))
	FVector BoxExtents = FVector(1000.0f, 1000.0f, 1000.0f);

	/** Distance box rotation around z axis */
	UPROPERTY(EditAnywhere, BlueprintGetter="GetBoxRotation", BlueprintSetter="SetBoxRotation", Category="TickOptToolkit", meta=(EditCondition="DistanceMode==ETickOptToolkitDistanceMode::Box"))
	float BoxRotation = 0.0f;

	/** Tick by distance buffer size.
	 *  It's added to distance check when the focus is moving further away. Helps to mitigate constant switching issue
	 *  when the focus is exactly at the tick distance.
	 */
	UPROPERTY(EditAnywhere, BlueprintGetter="GetBufferSize", BlueprintSetter="SetBufferSize", Category="TickOptToolkit", meta=(EditCondition="DistanceMode!=ETickOptToolkitDistanceMode::None"))
	float BufferSize = 100.0f;

	/** Sizes for additional distance zones.
	 *  The sizes are stacked up on the basic distance shape as an offset.
	 */
	UPROPERTY(EditAnywhere, BlueprintGetter="GetMidZoneSizes", BlueprintSetter="SetMidZoneSizes", Category="TickOptToolkit", meta=(EditCondition="DistanceMode!=ETickOptToolkitDistanceMode::None"))
	TArray<float> MidZoneSizes;

	/** Tick by visibility mode */
	UPROPERTY(EditAnywhere, BlueprintGetter="GetVisibilityMode", BlueprintSetter="SetVisibilityMode", Category="TickOptToolkit")
	ETickByVisibilityMode VisibilityMode = ETickByVisibilityMode::None;

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

	/** Should force first execution of the tick after BeginPlay, even when it's disabled. */
	UPROPERTY(EditAnywhere, BlueprintGetter="ShouldForceExecuteFirstTick", BlueprintSetter="SetForceExecuteFirstTick", Category="TickOptToolkit")
	bool bForceExecuteFirstTick = false;
	
	/** Distance zone / visibility dependent tick settings used when controlling ticks of owning actor and its components. */
	UPROPERTY(EditAnywhere, BlueprintGetter="GetTickSettings", BlueprintSetter="SetTickSettings", Category="TickOptToolkit")
	TArray<FTickOptToolkitTickSettings> TickSettings = {{0.0f, 0.0f, true, false}, {0.0f, 0.0f, false, false}};

	/** Additional optimization levels, controlled with tot.TickOptimizationLevel cvar. */
	UPROPERTY(EditAnywhere, Category="TickOptToolkit", AdvancedDisplay)
	TArray<FTickOptToolkitOptimizationLevel> OptimizationLevels;

	/** Layer mask for filtering focuses that this target should take into account. */
	UPROPERTY(EditAnywhere, BlueprintGetter="GetFocusLayerMask", BlueprintSetter="SetFocusLayerMask", Category="TickOptToolkit", AdvancedDisplay, meta = (Bitmask, BitmaskEnum="/Script/TickOptToolkit.ETickOptToolkitFocusLayer"))
	int FocusLayerMask = 1;

	/** Disable tot.TickDistanceScale cvar influence. */
	UPROPERTY(EditAnywhere, BlueprintGetter="ShouldDisableDistanceScale", BlueprintSetter="SetDisableDistanceScale", Category="TickOptToolkit", AdvancedDisplay)
	bool bDisableDistanceScale = false;
	
protected:
	/** Is current Tick Zone and Visibility forced, meaning they won't react to change of distance / visibility. */
	UPROPERTY(EditAnywhere, BlueprintGetter="IsForced", Category="TickOptToolkit", AdvancedDisplay)
	bool bForced = false;

	/** Current tick distance zone (0 is the closest). If set out of bounds, will be clamped. */
	UPROPERTY(EditAnywhere, BlueprintGetter="GetTickZone", Category="TickOptToolkit", AdvancedDisplay, meta=(EditCondition="bForced&&DistanceMode!=ETickOptToolkitDistanceMode::None"))
	int TickZone = 0;

	/** Current tick visibility */
	UPROPERTY(EditAnywhere, BlueprintGetter="IsTickVisible", Category="TickOptToolkit", AdvancedDisplay, meta=(EditCondition="bForced&&VisibilityMode!=ETickByVisibilityMode::None"))
	bool bTickVisible = true;

public:
	UTickOptToolkitTargetComponent();
	
	/** Triggered when either distance zone or visibility changes. */
	UPROPERTY(BlueprintAssignable, Category="TickOptToolkit")
	FOnTickChange OnTickChanged;

	/** Triggered when distance zone changes. */
	UPROPERTY(BlueprintAssignable, Category="TickOptToolkit")
	FOnTickZoneChange OnTickZoneChanged;

	/** Triggered when visibility changes. */
	UPROPERTY(BlueprintAssignable, Category="TickOptToolkit")
	FOnTickVisibilityChange OnTickVisibilityChanged;

    /** Triggered when tick enables/disables with tick settings. */
	UPROPERTY(BlueprintAssignable, Category="TickOptToolkit")
	FOnTickEnabledChange OnTickEnabledChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void OnRegister() override;
	virtual void OnUnregister() override;

	virtual void OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
public:
	UFUNCTION(BlueprintGetter, Category="TickOptToolkit")
	ETickOptToolkitDistanceMode GetDistanceMode() const { return DistanceMode; }
	UFUNCTION(BlueprintSetter, Category="TickOptToolkit")
	void SetDistanceMode(ETickOptToolkitDistanceMode InDistanceMode); 

	UFUNCTION(BlueprintGetter, Category="TickOptToolkit")
	ETickByVisibilityMode GetVisibilityMode() const { return VisibilityMode; }
	UFUNCTION(BlueprintSetter, Category="TickOptToolkit")
	void SetVisibilityMode(ETickByVisibilityMode InVisibilityMode);
	
	UFUNCTION(BlueprintGetter, Category="TickOptToolkit")
	float GetSphereRadius() const { return TOT_OPT_LEVEL_GET(SphereRadius); }
	UFUNCTION(BlueprintSetter, Category="TickOptToolkit")
	void SetSphereRadius(float InRadius);
	
	UFUNCTION(BlueprintGetter, Category="TickOptToolkit")
	FVector GetBoxExtents() const { return TOT_OPT_LEVEL_GET(BoxExtents); }
	UFUNCTION(BlueprintSetter, Category="TickOptToolkit")
	void SetBoxExtents(FVector InExtents);
	
	UFUNCTION(BlueprintGetter, Category="TickOptToolkit")
	float GetBoxRotation() const { return BoxRotation; }
	UFUNCTION(BlueprintSetter, Category="TickOptToolkit")
	void SetBoxRotation(float InRotation);
	
	UFUNCTION(BlueprintGetter, Category="TickOptToolkit")
	float GetBufferSize() const { return TOT_OPT_LEVEL_GET(BufferSize); }
	UFUNCTION(BlueprintSetter, Category="TickOptToolkit")
	void SetBufferSize(float InBufferSize);
	
	UFUNCTION(BlueprintGetter, Category="TickOptToolkit")
	const TArray<float>& GetMidZoneSizes() const { return TOT_OPT_LEVEL_GET(MidZoneSizes); }
	UFUNCTION(BlueprintSetter, Category="TickOptToolkit")
	void SetMidZoneSizes(const TArray<float>& InMidZoneSizes);
	
	float GetZoneInnerSize(int InZone) const;
	uint32 GetMidZonesNum() const { return GetMidZoneSizes().Num(); }
	uint32 GetZonesNum() const;

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

	UFUNCTION(BlueprintGetter, Category="TickOptToolkit")
	bool ShouldForceExecuteFirstTick() const { return bForceExecuteFirstTick; }
	UFUNCTION(BlueprintSetter, Category="TickOptToolkit")
	void SetForceExecuteFirstTick(bool bInForceExecuteFirstTick);

	UFUNCTION(BlueprintGetter, Category="TickOptToolkit")
	int GetFocusLayerMask() const { return FocusLayerMask; }
	UFUNCTION(BlueprintSetter, Category="TickOptToolkit")
	void SetFocusLayerMask(int InFocusLayerMask);

	UFUNCTION(BlueprintGetter, Category="TickOptToolkit")
	bool ShouldDisableDistanceScale() const { return bDisableDistanceScale; }
	UFUNCTION(BlueprintSetter, Category="TickOptToolkit")
	void SetDisableDistanceScale(bool bInDisableDistanceScale);
	
	UFUNCTION(BlueprintGetter, Category="TickOptToolkit")
	const TArray<FTickOptToolkitTickSettings>& GetTickSettings() const { return TOT_OPT_LEVEL_GET(TickSettings); }
	UFUNCTION(BlueprintSetter, Category="TickOptToolkit")
	void SetTickSettings(const TArray<FTickOptToolkitTickSettings>& InTickSettings);
	
	UFUNCTION(BlueprintGetter, Category="TickOptToolkit")
	int GetTickZone() const { return TickZone; }
	UFUNCTION(BlueprintGetter, Category="TickOptToolkit")
	bool IsTickVisible() const { return bTickVisible; }

	UFUNCTION(BlueprintGetter, Category="TickOptToolkit")
	bool IsForced() const { return bForced; }

	/** Force tick distance zone and visibility, so they won't react to changes in distance / visibility
	 *
	 *  @Zone Tick distance zone to be forced
	 *  @Visible Tick visibility to be forced
	 */
	UFUNCTION(BlueprintCallable, Category="TickOptToolkit")
	void Force(int InZone, bool bInVisible);

	/** Release forced tick distance zone and visibility, so they will react to changes in distance / visibility */
	UFUNCTION(BlueprintCallable, Category="TickOptToolkit")
	void Release();

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
	
	FORCEINLINE float GetDistanceScale() const
	{
		return bDisableDistanceScale ? 1.0f : CVarTickOptToolkitTickDistanceScale.GetValueOnGameThread();
	}

private:
	void AddMimic(UTickOptToolkitMimicComponent* Mimic);
	void RemoveMimic(UTickOptToolkitMimicComponent* Mimic);
	
	void OnUpdated();
	void OnBalanceTickInterval();
	void OnUpdatedInBeginPlay();
	void ForceExecuteFirstTick();

	FORCEINLINE void GetCurrentTickSettings(bool& bTickEnabled, float& TickInterval) const
	{
		const TArray<FTickOptToolkitTickSettings>& PerDeviceTickSettings = TOT_OPT_LEVEL_GET(TickSettings); 
		const FTickOptToolkitTickSettings& Settings = TickZone < PerDeviceTickSettings.Num()
			? PerDeviceTickSettings[TickZone]
			: FTickOptToolkitTickSettings();
		bTickEnabled = bTickVisible ? Settings.bEnabledVisible : Settings.bEnabledHidden;
		TickInterval = bTickVisible ? Settings.IntervalVisible : Settings.IntervalHidden;
	}
	
	FORCEINLINE static void SyncTimelineToWorldTime(UTimelineComponent* Timeline, float WorldTime)
	{
		const float Length = Timeline->GetTimelineLength();
		const float PlayRate = Timeline->GetPlayRate();
		const float Position = FMath::Fmod(WorldTime, Length / PlayRate) * PlayRate;
		Timeline->SetPlaybackPosition(Position, false, false);
	}

	FORCEINLINE const FTickOptToolkitOptimizationLevel* GetCurrentOptimizationLevel(bool bRecache = false) const
	{
		if (CurrentOptimizationLevel != nullptr && !bRecache)
		{
			return CurrentOptimizationLevel;
		}
		
		const int32 OptimizationLevel = FMath::Clamp(CVarTickOptToolkitTickOptimizationLevel.GetValueOnGameThread(), 0, OptimizationLevels.Num());
		return OptimizationLevel == 0 ? &DefaultOptimizationLevel : &OptimizationLevels[OptimizationLevel - 1];
	}

	FORCEINLINE bool IsBalancingTickInterval() const { return BalancingBinIndex != INDEX_NONE; }
	
#if WITH_EDITOR
public:
	bool DoesOwnerStartWithTickEnabled() const;
#endif // WITH_EDITOR
};
