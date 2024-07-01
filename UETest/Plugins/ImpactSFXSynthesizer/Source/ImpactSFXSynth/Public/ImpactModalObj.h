// Copyright 2023-2024, Le Binh Son, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "IAudioProxyInitializer.h"

#include "ImpactModalObj.generated.h"

class UAssetImportData;
class FImpactModalObjAssetProxy;
using FImpactModalObjAssetProxyPtr = TSharedPtr<FImpactModalObjAssetProxy, ESPMode::ThreadSafe>;

struct FImpactModalModInfo
{
	int32 NumModals;
	float AmpScale;
	float DecayScale;
	float FreqScale;

	FImpactModalModInfo(const int32 InNum, const float InAmp, const float InDecay, const float InFreqScale)
	: NumModals(InNum), AmpScale(InAmp), DecayScale(InDecay), FreqScale(InFreqScale)
	{
	}

	bool IsEqual(const FImpactModalModInfo& Other) const
	{
		return (Other.NumModals == NumModals) && (Other.AmpScale == AmpScale)
				&& (Other.DecayScale == DecayScale) && (Other.FreqScale == FreqScale);
	}
};

UCLASS(hidecategories=Object, BlueprintType,  meta = (LoadBehavior = "LazyOnDemand"))
class IMPACTSFXSYNTH_API UImpactModalObj : public UObject, public IAudioProxyDataFactory
{
	GENERATED_BODY()

public:
	TArray<float> Params;
	
	UPROPERTY(VisibleAnywhere, Category = "Version")
	int32 Version;

	UPROPERTY(VisibleAnywhere, Category = "Version")
	int32 NumModals;
	
public:
	UImpactModalObj(const FObjectInitializer& ObjectInitializer);

	//~Begin IAudioProxyDataFactory Interface.
	virtual TSharedPtr<Audio::IProxyData, ESPMode::ThreadSafe> CreateProxyData(const Audio::FProxyDataInitParams& InitParams) override;
	//~ End IAudioProxyDataFactory Interface.
	FImpactModalObjAssetProxyPtr CreateNewImpactModalObjProxyData();
	FImpactModalObjAssetProxyPtr GetProxy();
	
#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Instanced, Category = ImportSettings)
	TObjectPtr<class UAssetImportData> AssetImportData;
#endif

	int32 GetVersion() const { return Version; }
	int32 GetNumModals() const { return NumModals; }

	//~ Begin UObject Interface. 
	virtual void Serialize(FArchive& Ar) override;
	virtual void PostInitProperties() override;
	//~ End UObject Interface.

private:
	FImpactModalObjAssetProxyPtr Proxy{ nullptr };
};

class IMPACTSFXSYNTH_API FImpactModalObjAssetProxy : public Audio::TProxyData<FImpactModalObjAssetProxy>, public TSharedFromThis<FImpactModalObjAssetProxy, ESPMode::ThreadSafe>
{
public:
	IMPL_AUDIOPROXY_CLASS(FImpactModalObjAssetProxy);

	FImpactModalObjAssetProxy(const FImpactModalObjAssetProxy& InAssetProxy) :
		Params(InAssetProxy.Params), NumModals(InAssetProxy.NumModals), bIsParamsChanged(false)
	{
	}

	explicit FImpactModalObjAssetProxy(const TArray<float>& InParams, const int32 InNumModals);

	FImpactModalObjAssetProxy(const FImpactModalObjAssetProxyPtr& A, const FImpactModalModInfo& ModA, const FImpactModalObjAssetProxyPtr& B, const FImpactModalModInfo& ModB);

	FImpactModalObjAssetProxy(const FImpactModalObjAssetProxyPtr& InPtr, const int32 NumUsedModals);

	FImpactModalObjAssetProxy(const TArrayView<const float>& InParams, const int32 NumUsedModals);
	
	TArrayView<const float> GetParams()
	{
		return TArrayView<const float>(Params);
	}

	/// Fast setting internal value with absolutely no check on index and value.
	/// The caller must make sure index is inbound.
	/// @param Index Index of the parameter
	/// @param NewValue New value
	void SetModalParam(const int32 Index, const float NewValue)
	{
		Params[Index] = NewValue;
	}

	void SetIsParamChanged(const bool InValue)
	{
		bIsParamsChanged = InValue;
	}

	bool IsParamChanged() const { return bIsParamsChanged; }
	
	int32 GetNumModals() const { return NumModals; }
	int32 GetNumParams() const { return Params.Num(); }

	void CopyAllParams(const TArrayView<const float>& SourceData);

protected:
	TArray<float> Params;
	int32 NumModals;
	bool bIsParamsChanged;
};