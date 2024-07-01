// Copyright 2023-2024, Le Binh Son, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "ImpactModalObj.h"
#include "ResidualData.h"
#include "Utils.h"
#include "UObject/Object.h"
#include "IAudioProxyInitializer.h"

#include "MultiImpactData.generated.h"

class FMultiImpactDataAssetProxy;
using FMultiImpactDataAssetProxyPtr = TSharedPtr<FMultiImpactDataAssetProxy, ESPMode::ThreadSafe>;
class UImpactModalObj;
class UResidualData;
class FImpactModalObjAssetProxy;
class FResidualDataAssetProxy;

USTRUCT(BlueprintType)
struct FImpactModalData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, meta = (DisplayName = "Amplitude", ClampMin="-1", ClampMax="1"), Category="Modal")
	float Amplitude = 1.f;
	
	UPROPERTY(EditAnywhere, meta = (DisplayName = "Decay Rate", ClampMin="0."), Category="Modal")
	float DecayRate = 1.f;

	UPROPERTY(EditAnywhere, meta = (DisplayName = "Frequency", ClampMin="20", ClampMax="20000"), Category="Modal")
	float Frequency = 440.f;
};

USTRUCT(BlueprintType)
struct FImpactSpawnInfo
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category="SpawnTime", meta = (DisplayName = "Start Spawning Time", ClampMin="-1", ToolTip="This variation can be applied to new impacts from this time."))
	float StartTime = -1.f;

	UPROPERTY(EditAnywhere, Category="SpawnTime", meta = (DisplayName = "Stop Spawning Time", ClampMin="-1", ToolTip="This variation will not be used after this time. If <= 0, never end."))
	float EndTime = -1.f;

	UPROPERTY(EditAnywhere, Category="Impact", meta = (DisplayName = "Damping Ratio", ClampMin="0.", ClampMax="1.", ToolTip="Affect how long this variation will continue to ring/vibrate after an impact. Dense and stiff objects like rocks should have a ratio of 1. For thin/hollow metallic objects, this should be nearly zero."))
	float DampingRatio = 0.5f;
	
	UPROPERTY(EditAnywhere, Category="Modal", meta = (DisplayName = "Use Modal Synthesizer"))
	bool bUseModalSynth = true;

	UPROPERTY(EditAnywhere, Category="Modal", meta = (DisplayName = "Modal Start Time"))
	float ModalStartTime = 0.f;
	
	UPROPERTY(EditAnywhere, Category="Modal", meta = (DisplayName = "Modal Duration", ClampMin="-1."))
	float ModalDuration = -1.f;
	
	UPROPERTY(EditAnywhere, Category="Modal", meta = (DisplayName = "Modal Amplitude Min", ClampMin="0."))
	float ModalAmpScale = 1.f;

	UPROPERTY(EditAnywhere, Category="Modal", meta = (DisplayName = "Modal Amplitude Range", ClampMin="0.", ToolTip="Amplitude = Rand(Amplitude Min, Amplitude Min + Range)."))
	float ModalAmpScaleRange = 0.f;

	UPROPERTY(EditAnywhere, Category="Modal", meta = (DisplayName = "Modal Decay Scale Min", ClampMin="0.05"))
	float ModalDecayScale = 1.f;

	UPROPERTY(EditAnywhere, Category="Modal", meta = (DisplayName = "Modal Decay Scale Range", ClampMin="0.", ToolTip="Decay scale = Rand(Decay Scale Min, Decay Scale Min + Range)."))
	float ModalDecayScaleRange = 0.f;
	
	UPROPERTY(EditAnywhere, Category="Modal", meta = (DisplayName = "Modal Pitch Shift Min", ClampMin="-72", ClampMax="72"))
	float ModalPitchShift = 0.f;

	UPROPERTY(EditAnywhere, Category="Modal", meta = (DisplayName = "Modal Pitch Shift Range",  ClampMin="0", ClampMax="2", ToolTip="Pitch Shift = Rand(Pitch Shift Min, Pitch Shift Min + Range)."))
	float ModalPitchShiftRange = 0.f;

	UPROPERTY(EditAnywhere, Category="Residual", meta = (DisplayName = "Use Residual Synthesizer"))
	bool bUseResidualSynth = true;
	
	UPROPERTY(EditAnywhere, Category="Residual", meta = (DisplayName = "Residual Start Time"))
	float ResidualStartTime = 0.f;
	
	UPROPERTY(EditAnywhere, Category="Residual", meta = (DisplayName = "Residual Duration", ClampMin="-1."))
	float ResidualDuration = -1.f;
	
	UPROPERTY(EditAnywhere, Category="Residual", meta = (DisplayName = "Residual Amplitude Min", ClampMin="0."))
	float ResidualAmpScale = 1.f;

	UPROPERTY(EditAnywhere, Category="Residual", meta = (DisplayName = "Residual Amplitude Range", ClampMin="0.", ToolTip="Amplitude = Rand(Amplitude Min, Amplitude Min + Range)."))
	float ResidualAmpScaleRange = 0.f;
	
	UPROPERTY(EditAnywhere, Category="Residual", meta = (DisplayName = "Residual Play Speed Min", ClampMin="0.05"))
	float ResidualPlaySpeed = 1.f;

	UPROPERTY(EditAnywhere, Category="Residual", meta = (DisplayName = "Residual Play Speed Range", ClampMin="0.", ToolTip="Play speed = Rand(Play Speed Min, Play Speed Min + Range)."))
	float ResidualPlaySpeedRange = 0.f;

	UPROPERTY(EditAnywhere, Category="Residual", meta = (DisplayName = "Residual Pitch Shift Min", ClampMin="-72", ClampMax="72"))
	float ResidualPitchShift = 0.f;

	UPROPERTY(EditAnywhere,Category="Residual", meta = (DisplayName = "Residual Pitch Shift Range", ClampMin="0", ClampMax="2", ToolTip="Pitch Shift = Rand(Pitch Shift Min, Pitch Shift Min + Range)."))
	float ResidualPitchShiftRange = 0.f;

	float GetModalAmplitudeScaleRand(const FRandomStream& RandomStream) const
	{
		return FMath::Max(0.f, LBSImpactSFXSynth::GetRandRange(RandomStream, ModalAmpScale, ModalAmpScaleRange));
	}

	float GetModalDecayScaleRand(const FRandomStream& RandomStream, const float Bias = 1.f) const
	{
		return FMath::Max(0.f, LBSImpactSFXSynth::GetRandRange(RandomStream, ModalDecayScale * Bias, ModalDecayScaleRange));
	}

	float GetModalPitchScaleRand(const FRandomStream& RandomStream, const float Offset = 0.f) const
	{
		return LBSImpactSFXSynth::GetPitchScaleClamped(Offset + LBSImpactSFXSynth::GetRandRange(RandomStream, ModalPitchShift, ModalPitchShiftRange));
	}

	float GetResidualAmplitudeScaleRand(const FRandomStream& RandomStream) const
	{
		return FMath::Max(0.f, LBSImpactSFXSynth::GetRandRange(RandomStream, ResidualAmpScale, ResidualAmpScaleRange));
	}

	float GetResidualPlaySpeedScaleRand(const FRandomStream& RandomStream, const float Bias = 1.f) const
	{
		return FMath::Max(0.05f, LBSImpactSFXSynth::GetRandRange(RandomStream, ResidualPlaySpeed * Bias, ResidualPlaySpeedRange));
	}
	
	float GetResidualPitchScaleRand(const FRandomStream& RandomStream, const float Offset = 0.f) const
	{
		return LBSImpactSFXSynth::GetPitchScaleClamped(Offset + LBSImpactSFXSynth::GetRandRange(RandomStream, ResidualPitchShift, ResidualPitchShiftRange));
	}

	bool CanSpawn(const float CurrentTime) const
	{
		const bool bIsNotEnd = EndTime > -1 ? CurrentTime < EndTime : true; 
		return CurrentTime >= StartTime && bIsNotEnd;
	}
};


#if WITH_EDITOR
DECLARE_DELEGATE_ThreeParams(FOnPlayVariationSound, const class UMultiImpactData*, const FImpactSpawnInfo&, const bool)
#endif

UCLASS(editinlinenew, BlueprintType,  meta = (LoadBehavior = "LazyOnDemand"))
class IMPACTSFXSYNTH_API UMultiImpactData : public UObject, public IAudioProxyDataFactory
{
	GENERATED_BODY()

protected:

	UPROPERTY(EditDefaultsOnly, meta = (DisplayName = "Modal Object"), Category="Spawnning")
	UImpactModalObj* ModalObj;

	UPROPERTY(EditDefaultsOnly, meta = (DisplayName = "Number of Modals"), Category="Spawnning")
	int32 NumUsedModals = -1;

	UPROPERTY(EditDefaultsOnly, meta = (DisplayName = "Randomly Get Modal Data", ToolTip="If true, randomly get modals from modal object instead of following their priority order."), Category="Spawnning")
	bool bRandomlyGetModal = false;
	
	UPROPERTY(EditDefaultsOnly, meta = (DisplayName = "Manually Set Modal Data",  ToolTip="If true, use Modal Data instead of Modal Object.", Category="Spawnning"))
	bool bUseManualModalData;
	
	UPROPERTY(EditDefaultsOnly, meta = (DisplayName = "Modal Data",  ToolTip="Manually set modal data.", Category="Spawnning"))
	TArray<FImpactModalData> ModalData;
	
	UPROPERTY(EditDefaultsOnly, meta = (DisplayName = "Residual Data"), Category="Spawnning")
	UResidualData* ResidualData;
	
	UPROPERTY(EditDefaultsOnly, meta = (DisplayName = "Impact Spawn Variations"), Category="Spawnning")
	TArray<FImpactSpawnInfo> SpawnInfos;

public:

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditDefaultsOnly, meta = (DisplayName = "Previewed Variation Index", ToolTip="If -1, play a random variation.", ClampMin="-1"), Category="Preview")
	int32 PreviewVariationIndex = -1;

	UPROPERTY(EditDefaultsOnly, meta = (DisplayName = "Previewed Impact Strength", ClampMin="0.1"), Category="Preview")
	float PreviewImpactStrength = 1.0f;
	
	FOnPlayVariationSound OnPlayVariationSound;
#endif
	
	//~Begin IAudioProxyDataFactory Interface.
	virtual TSharedPtr<Audio::IProxyData> CreateProxyData(const Audio::FProxyDataInitParams& InitParams) override;
	//~ End IAudioProxyDataFactory Interface.
	FMultiImpactDataAssetProxyPtr CreateNewMultiImpactProxyData();

	FImpactModalObjAssetProxyPtr GetModalObjProxy() const;
	FResidualDataAssetProxyPtr GetResidualDataAssetProxy() const;

	bool IsCanUseModalObj() const;
	
	int32 GetNumUsedModals() const { return NumUsedModals; }

	bool IsRandomlyGetModal() const;
	
#if WITH_EDITOR
	UFUNCTION(CallInEditor, Category="Preview")
	void PlayVariationSound() const;
	
	UFUNCTION(CallInEditor, Category="Preview")
	void StopPlaying() const;

	float GetPreviewImpactStrength() const { return PreviewImpactStrength; }

	const FImpactSpawnInfo* GetCurrentValidPreviewSpawnInfo() const; 
#endif

private:
	// cached proxy
	FMultiImpactDataAssetProxyPtr Proxy{ nullptr };
};

class IMPACTSFXSYNTH_API FMultiImpactDataAssetProxy : public Audio::TProxyData<FMultiImpactDataAssetProxy>, public TSharedFromThis<FMultiImpactDataAssetProxy, ESPMode::ThreadSafe>
{
public:
	IMPL_AUDIOPROXY_CLASS(FMultiImpactDataAssetProxy);

	FMultiImpactDataAssetProxy(const FMultiImpactDataAssetProxy& InAssetProxy);
	FMultiImpactDataAssetProxy(const UMultiImpactData* InMultiImpactData, const TArray<FImpactSpawnInfo>& InSpawnInfos);

	const FImpactModalObjAssetProxyPtr& GetModalProxy() const;
	const FResidualDataAssetProxyPtr& GetResidualProxy() const;
	TArrayView<const FImpactSpawnInfo> GetSpawnInfos() const { return SpawnInfos; }
	int32 GetNumVariation() const { return SpawnInfos.Num(); }
	int32 GetNumUsedModals() const { return NumUsedModals; }
	bool IsRandomlyGetModals() const;
	
protected:
	FImpactModalObjAssetProxyPtr ModalObjAssetProxyPtr;
	FResidualDataAssetProxyPtr ResidualDataAssetProxyPtr;
	TArrayView<const FImpactSpawnInfo> SpawnInfos;
	int32 NumUsedModals;
	bool bIsRandomlyGetModal;
};
