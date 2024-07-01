// Copyright 2023-2024, Le Binh Son, All rights reserved.


#include "MultiImpactData.h"

#include "ImpactModalObj.h"
#include "ResidualData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MultiImpactData)

TSharedPtr<Audio::IProxyData> UMultiImpactData::CreateProxyData(const Audio::FProxyDataInitParams& InitParams)
{
#if WITH_EDITOR
	//Always return new proxy in editor as we can edit this data after MetaSound Graph is loaded
	return CreateNewMultiImpactProxyData();
#endif
	
	if (!Proxy.IsValid())
	{
		Proxy = CreateNewMultiImpactProxyData();
	}
	return Proxy;
}

FMultiImpactDataAssetProxyPtr UMultiImpactData::CreateNewMultiImpactProxyData()
{
	return MakeShared<FMultiImpactDataAssetProxy, ESPMode::ThreadSafe>(this, SpawnInfos);
}

FImpactModalObjAssetProxyPtr UMultiImpactData::GetModalObjProxy() const
{
	if(bUseManualModalData)
	{
		if(ModalData.Num() < 1)
			return nullptr;

		TArray<float> Params;
		for(int i = 0; i < ModalData.Num(); i++)
		{
			Params.Emplace(ModalData[i].Amplitude);
			Params.Emplace(ModalData[i].DecayRate);
			Params.Emplace(ModalData[i].Frequency);
		}
		//Don't have to cache this as we expect it's cached by the caller function
		return MakeShared<FImpactModalObjAssetProxy, ESPMode::ThreadSafe>(Params, ModalData.Num());
	}
	
	if(ModalObj)
		return ModalObj->GetProxy();
	else
		return nullptr;
}

FResidualDataAssetProxyPtr UMultiImpactData::GetResidualDataAssetProxy() const
{
	if(ResidualData)
		return ResidualData->GetProxy();
	else
		return nullptr;
}

bool UMultiImpactData::IsCanUseModalObj() const
{
	return bUseManualModalData ? (ModalData.Num() > 0) : (ModalObj != nullptr); 
}

bool UMultiImpactData::IsRandomlyGetModal() const
{
	if(NumUsedModals < 0 || !bRandomlyGetModal || !IsCanUseModalObj())
		return false;

	if(bUseManualModalData)
	{
		return NumUsedModals < ModalData.Num();
	}
	
	return NumUsedModals < ModalObj->GetNumModals();
}

#if WITH_EDITOR

void UMultiImpactData::PlayVariationSound() const
{
	if(const FImpactSpawnInfo* SpawnInfo = GetCurrentValidPreviewSpawnInfo())
	{
		if(OnPlayVariationSound.IsBound())
			OnPlayVariationSound.Execute(this, *SpawnInfo, true);
	}
}

void UMultiImpactData::StopPlaying() const
{
	OnPlayVariationSound.ExecuteIfBound(this, FImpactSpawnInfo(), false);
}

const FImpactSpawnInfo* UMultiImpactData::GetCurrentValidPreviewSpawnInfo() const
{
	if(PreviewVariationIndex >= SpawnInfos.Num() || SpawnInfos.Num() == 0)
		return nullptr;

	const int32 Index = PreviewVariationIndex > -1 ? PreviewVariationIndex : FMath::RandRange(0, SpawnInfos.Num() - 1);
	const FImpactSpawnInfo* SpawnInfo = &SpawnInfos[Index];
	
	if((ResidualData == nullptr || !SpawnInfo->bUseResidualSynth) && (!IsCanUseModalObj() || !SpawnInfo->bUseModalSynth))
		return nullptr;

	return SpawnInfo;
}

#endif

FMultiImpactDataAssetProxy::FMultiImpactDataAssetProxy(const FMultiImpactDataAssetProxy& InAssetProxy)
: ModalObjAssetProxyPtr(InAssetProxy.ModalObjAssetProxyPtr)
, ResidualDataAssetProxyPtr(InAssetProxy.ResidualDataAssetProxyPtr)
, SpawnInfos(InAssetProxy.SpawnInfos)
, NumUsedModals(InAssetProxy.NumUsedModals)
, bIsRandomlyGetModal(InAssetProxy.bIsRandomlyGetModal)
{
	
}

FMultiImpactDataAssetProxy::FMultiImpactDataAssetProxy(const UMultiImpactData* InMultiImpactData, const TArray<FImpactSpawnInfo>& InSpawnInfos)
{
	if(InMultiImpactData == nullptr)
		return;

	ModalObjAssetProxyPtr = InMultiImpactData->GetModalObjProxy();
	ResidualDataAssetProxyPtr = InMultiImpactData->GetResidualDataAssetProxy();
	SpawnInfos = TArrayView<const FImpactSpawnInfo>(InSpawnInfos);
	NumUsedModals = InMultiImpactData->GetNumUsedModals();
	bIsRandomlyGetModal = InMultiImpactData->IsRandomlyGetModal();
}

const FImpactModalObjAssetProxyPtr& FMultiImpactDataAssetProxy::GetModalProxy() const
{
	return ModalObjAssetProxyPtr;
}

const FResidualDataAssetProxyPtr& FMultiImpactDataAssetProxy::GetResidualProxy() const
{
	return ResidualDataAssetProxyPtr;
}

bool FMultiImpactDataAssetProxy::IsRandomlyGetModals() const
{
	if(ModalObjAssetProxyPtr == nullptr || NumUsedModals < 0 || !bIsRandomlyGetModal)
		return false;

	return NumUsedModals < ModalObjAssetProxyPtr->GetNumModals();
}
