// Copyright 2021 Marcin Swiderski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TickOptToolkitAnimUpdateRateOptComponent.generated.h"

/** Anim Update Rate Optimization modes */
UENUM()
enum class ETickOptToolkitAnimUROMode : uint8
{
	/** Map relative screen size of a component (based on bounding sphere) to the number of frames skipped */
	ScreenSize,
	/** Map currently used LOD of a component to the number of frames skipped */
	LOD,
	/** Map minimal LOD of a component to the number of frames skipped */
	MinLOD,
};

USTRUCT()
struct FTickOptToolkitUROOptimizationLevel
{
	GENERATED_BODY()

	/** Relative screen size (based on bounding sphere) thresholds mapping to frames skipped.
	 *  Each thresholds index is equal to the number of frames skipped if the screen size is larger the threshold.
	 *  e.g. first threshold (at index 0) maps to 0 frames skipped, meaning a normal update rate,
	 *  third threshold (at index 2) maps to 2 frames skipped, meaning update each 3 frames (1 frame of update, 2 frames skipped).
	 */
	UPROPERTY(EditAnywhere, Category="TickOptToolkit")
	TArray<float> FramesSkippedScreenSizeThresholds = { 0.5f, 0.35f };

	/** LOD mapping to frames skipped.
	 *  For each LOD index, frames skipped value at the same index will be taken. If LOD index exceeds the bounds of the array,
	 *  the last value of frames skipped will be used.
	 *  0 frames skipped means a normal update rate, and e.g. 2 frames skipped means update each 3 frames (1 frame of update, 2 frames skipped).
	 */
	UPROPERTY(EditAnywhere, Category="TickOptToolkit")
	TArray<int> LODToFramesSkipped = { 0, 1, 2 };

	/** Number of frames skipped when the component is not being rendered.
	 *  0 frames skipped means a normal update rate, and e.g. 2 frames skipped means update each 3 frames (1 frame of update, 2 frames skipped).
	 */
	UPROPERTY(EditAnywhere, Category="TickOptToolkit")
	int NonRenderedFramesSkipped = 3;

	/** Maximum number of frames skipped that will trigger frame interpolation.
	 *  0 frames skipped means a normal update rate, and e.g. 2 frames skipped means update each 3 frames (1 frame of update, 2 frames skipped).
	 */
	UPROPERTY(EditAnywhere, Category="TickOptToolkit")
	int MaxFramesSkippedForInterpolation = 3;
};

UCLASS(Blueprintable, ClassGroup=(TickOptToolkit), meta=(BlueprintSpawnableComponent),
	HideCategories=(Rendering, Physics, Collision, LOD, Activation, Cooking, ComponentReplication))
class TICKOPTTOOLKIT_API UTickOptToolkitAnimUpdateRateOptComponent : public UActorComponent
{
	GENERATED_BODY()

	/** Anim Update Rate Optimization mode */
	UPROPERTY(EditAnywhere, BlueprintGetter="GetAnimUpdateRateOptimizationsMode", BlueprintSetter="SetAnimUpdateRateOptimizationsMode", Category="TickOptToolkit")
	ETickOptToolkitAnimUROMode AnimUpdateRateOptimizationsMode = ETickOptToolkitAnimUROMode::ScreenSize;

	/** Relative screen size (based on bounding sphere) thresholds mapping to frames skipped.
	 *  Each thresholds index is equal to the number of frames skipped if the screen size is larger the threshold.
	 *  e.g. first threshold (at index 0) maps to 0 frames skipped, meaning a normal update rate,
	 *  third threshold (at index 2) maps to 2 frames skipped, meaning update each 3 frames (1 frame of update, 2 frames skipped).
	 */
	UPROPERTY(EditAnywhere, BlueprintGetter="GetFramesSkippedScreenSizeThresholds", BlueprintSetter="SetFramesSkippedScreenSizeThresholds", Category="TickOptToolkit", meta=(EditCondition="AnimUpdateRateOptimizationsMode==ETickOptToolkitAnimUROMode::ScreenSize"))
	TArray<float> FramesSkippedScreenSizeThresholds = { 0.5f, 0.35f };

	/** LOD mapping to frames skipped.
	 *  For each LOD index, frames skipped value at the same index will be taken. If LOD index exceeds the bounds of the array,
	 *  the last value of frames skipped will be used.
	 *  0 frames skipped means a normal update rate, and e.g. 2 frames skipped means update each 3 frames (1 frame of update, 2 frames skipped).
	 */
	UPROPERTY(EditAnywhere, BlueprintGetter="GetLODToFramesSkipped", BlueprintSetter="SetLODToFramesSkipped", Category="TickOptToolkit", meta=(EditCondition="AnimUpdateRateOptimizationsMode!=ETickOptToolkitAnimUROMode::ScreenSize"))
	TArray<int> LODToFramesSkipped = { 0, 1, 2 };

	/** Number of frames skipped when the component is not being rendered.
	 *  0 frames skipped means a normal update rate, and e.g. 2 frames skipped means update each 3 frames (1 frame of update, 2 frames skipped).
	 */
	UPROPERTY(EditAnywhere, BlueprintGetter="GetNonRenderedFramesSkipped", BlueprintSetter="SetNonRenderedFramesSkipped", Category="TickOptToolkit")
	int NonRenderedFramesSkipped = 3;

	/** Maximum number of frames skipped that will trigger frame interpolation.
	 *  0 frames skipped means a normal update rate, and e.g. 2 frames skipped means update each 3 frames (1 frame of update, 2 frames skipped).
	 */
	UPROPERTY(EditAnywhere, BlueprintGetter="GetMaxFramesSkippedForInterpolation", BlueprintSetter="SetMaxFramesSkippedForInterpolation", Category="TickOptToolkit")
	int MaxFramesSkippedForInterpolation = 3;

	/** Should force enable anim update rate optimization on all skinned mesh components present in the actor during BeginPlay.
	 *  If false, optimization will have to by turned on by hand on each component that needs it.
	 */
	UPROPERTY(EditAnywhere, BlueprintGetter="ShouldForceEnableAnimUpdateRateOptimizations", BlueprintSetter="SetForceEnableAnimUpdateRateOptimizations", Category="TickOptToolkit")
	bool bForceEnableAnimUpdateRateOptimizations = true;

	/** Disable tot.UROScreenSizeScale cvar influence. */
	UPROPERTY(EditAnywhere, BlueprintGetter="ShouldDisableScreenSizeScale", BlueprintSetter="SetDisableScreenSizeScale", Category="TickOptToolkit", AdvancedDisplay)
	bool bDisableScreenSizeScale = false;

	/** Additional optimization levels, controlled with tot.UROOptimizationLevel cvar. */
	UPROPERTY(EditAnywhere, Category="TickOptToolkit", AdvancedDisplay)
	TArray<FTickOptToolkitUROOptimizationLevel> OptimizationLevels;
	
	UPROPERTY(Transient)
	TArray<USkinnedMeshComponent*> RegisteredSkinnedMeshComponents;

	FDelegateHandle OnOptimizationLevelHandle;
	FDelegateHandle OnScreenSizeScaleHandle;
	
public:
	UTickOptToolkitAnimUpdateRateOptComponent();
	
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
public:
	UFUNCTION(BlueprintGetter, Category="TickOptToolkit")
	ETickOptToolkitAnimUROMode GetAnimUpdateRateOptimizationsMode() const { return AnimUpdateRateOptimizationsMode; }
	UFUNCTION(BlueprintSetter, Category="TickOptToolkit")
	void SetAnimUpdateRateOptimizationsMode(ETickOptToolkitAnimUROMode InAnimUpdateRateOptimizationsMode);

	UFUNCTION(BlueprintGetter, Category="TickOptToolkit")
	const TArray<float>& GetFramesSkippedScreenSizeThresholds() const { return FramesSkippedScreenSizeThresholds; }
	UFUNCTION(BlueprintSetter, Category="TickOptToolkit")
	void SetFramesSkippedScreenSizeThresholds(const TArray<float>& InFramesSkippedScreenSizeThresholds);

	UFUNCTION(BlueprintGetter, Category="TickOptToolkit")
	const TArray<int>& GetLODToFramesSkipped() const { return LODToFramesSkipped; }
	UFUNCTION(BlueprintSetter, Category="TickOptToolkit")
	void SetLODToFramesSkipped(const TArray<int>& InLODToFramesSkipped);

	UFUNCTION(BlueprintGetter, Category="TickOptToolkit")
	int GetNonRenderedFramesSkipped() const { return NonRenderedFramesSkipped; }
	UFUNCTION(BlueprintSetter, Category="TickOptToolkit")
	void SetNonRenderedFramesSkipped(int InNonRenderedFramesSkipped);

	UFUNCTION(BlueprintGetter, Category="TickOptToolkit")
	int GetMaxFramesSkippedForInterpolation() const { return MaxFramesSkippedForInterpolation; }
	UFUNCTION(BlueprintSetter, Category="TickOptToolkit")
	void SetMaxFramesSkippedForInterpolation(int InMaxFramesSkippedForInterpolation);

	UFUNCTION(BlueprintGetter, Category="TickOptToolkit")
	bool ShouldForceEnableAnimUpdateRateOptimizations() const { return bForceEnableAnimUpdateRateOptimizations; }
	UFUNCTION(BlueprintSetter, Category="TickOptToolkit")
	void SetForceEnableAnimUpdateRateOptimizations(bool bInForceEnableAnimUpdateRateOptimizations);

	UFUNCTION(BlueprintGetter, Category="TickOptToolkit")
	bool ShouldDisableScreenSizeScale() const { return bDisableScreenSizeScale; }
	UFUNCTION(BlueprintSetter, Category="TickOptToolkit")
	void SetDisableScreenSizeScale(bool bInDisableScreenSizeScale);
	
	/** Register dynamically created Skinned Mesh Components into the Anim Update Rate Optimization component.
	 *  This function MUST be called for all components that are created in the owning actors BeginPlay function and later,
	 *  even if the component will not have the update rate optimization enabled.
	 *
	 *  @SkinnedMeshComponent Skinned Mesh Component to register.
	 */
	UFUNCTION(BlueprintCallable, Category="TickOptToolkit")
	void RegisterDynamicSkinnedMeshComponent(USkinnedMeshComponent* SkinnedMeshComponent);
	
private:
	void OnAnimUpdateRateParamsCreated(FAnimUpdateRateParameters* AnimUpdateRateParameters);
	void OnScalabilityCVarChanged(IConsoleVariable*);
};
