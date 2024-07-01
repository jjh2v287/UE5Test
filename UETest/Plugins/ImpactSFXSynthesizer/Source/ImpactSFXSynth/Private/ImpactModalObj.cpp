// Copyright 2023-2024, Le Binh Son, All rights reserved.

#include "ImpactModalObj.h"

#include "ImpactSFXSynthLog.h"
#include "EditorFramework/AssetImportData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ImpactModalObj)

#define NUM_PARAM_PER_MODAL (3)

UImpactModalObj::UImpactModalObj(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	Version = 0;
	NumModals = -1;
}

TSharedPtr<Audio::IProxyData, ESPMode::ThreadSafe> UImpactModalObj::CreateProxyData(const Audio::FProxyDataInitParams& InitParams)
{
	return GetProxy();
}

FImpactModalObjAssetProxyPtr UImpactModalObj::GetProxy()
{
#if WITH_EDITOR
	//v1.2: return new proxy in editor as we can update this modal
	return CreateNewImpactModalObjProxyData();
#endif
	
	if (!Proxy.IsValid())
	{
		Proxy = CreateNewImpactModalObjProxyData();
	}
	return Proxy;
}

FImpactModalObjAssetProxyPtr UImpactModalObj::CreateNewImpactModalObjProxyData()
{
	return MakeShared<FImpactModalObjAssetProxy>(Params, NumModals);
}

void UImpactModalObj::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	Ar << Version;
	Ar << NumModals;
	Ar << Params;
}

void UImpactModalObj::PostInitProperties()
{
	UObject::PostInitProperties();

#if WITH_EDITORONLY_DATA
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		AssetImportData = NewObject<UAssetImportData>(this, TEXT("AssetImportData"));
	}
#endif
}

FImpactModalObjAssetProxy::FImpactModalObjAssetProxy(const TArray<float>& InParams, const int32 InNumModals)
{
	Params = InParams;
	NumModals = InNumModals;
	bIsParamsChanged = false;
}

FImpactModalObjAssetProxy::FImpactModalObjAssetProxy(const FImpactModalObjAssetProxyPtr& A, const FImpactModalModInfo& ModA,
													 const FImpactModalObjAssetProxyPtr& B, const FImpactModalModInfo& ModB)
{
	if(!A.IsValid() || !B.IsValid())
		return;

	bIsParamsChanged = false;
	
	const int32 NumModalsAUsed = ModA.NumModals > 0 ? FMath::Min(ModA.NumModals, A->NumModals) : A->NumModals;
	const int32 NumModalsBUsed = ModB.NumModals > 0 ? FMath::Min(ModB.NumModals, B->NumModals) : B->NumModals;

	NumModals = NumModalsAUsed + NumModalsBUsed;
	const int32 MinNumParams = FMath::Min(NumModalsAUsed, NumModalsBUsed) * NUM_PARAM_PER_MODAL;
	Params.Empty(NumModals * NUM_PARAM_PER_MODAL);
	int i = 0; 
	int j = 0;
	for(; i < MinNumParams; i++, j++)
	{
		Params.Emplace(A->Params[i] * ModA.AmpScale);
		i++;
		Params.Emplace(A->Params[i] * ModA.DecayScale);
		i++;
		Params.Emplace(A->Params[i] * ModA.FreqScale);

		Params.Emplace(B->Params[j] * ModB.AmpScale);
		j++;
		Params.Emplace(B->Params[j] * ModB.DecayScale);
		j++;
		Params.Emplace(B->Params[j] * ModB.FreqScale);
	}

	const int32 NumUsedParamsA = NumModalsAUsed * NUM_PARAM_PER_MODAL;
	for(; i < NumUsedParamsA; i++)
	{
		Params.Emplace(A->Params[i] * ModA.AmpScale);
		i++;
		Params.Emplace(A->Params[i] * ModA.DecayScale);
		i++;
		Params.Emplace(A->Params[i] * ModA.FreqScale);
	}

	const int32 NumUsedParamsB = NumModalsBUsed * NUM_PARAM_PER_MODAL;
	for(; j < NumUsedParamsB; j++)
	{
		Params.Emplace(B->Params[j] * ModB.AmpScale);
		j++;
		Params.Emplace(B->Params[j] * ModB.DecayScale);
		j++;
		Params.Emplace(B->Params[j] * ModB.FreqScale);
	}
}

FImpactModalObjAssetProxy::FImpactModalObjAssetProxy(const FImpactModalObjAssetProxyPtr& InPtr, const int32 NumUsedModals)
{
	if(!InPtr.IsValid())
		return;
	
	bIsParamsChanged = false;
	
	NumModals = NumUsedModals > 0 ? FMath::Min(NumUsedModals, InPtr->NumModals) : InPtr->NumModals;
	const int32 MinNumParams = NumModals * NUM_PARAM_PER_MODAL;
	
	Params.SetNumUninitialized(MinNumParams);
	FMemory::Memcpy(Params.GetData(), InPtr->GetParams().GetData(), MinNumParams * sizeof(float));
}

FImpactModalObjAssetProxy::FImpactModalObjAssetProxy(const TArrayView<const float>& InParams, const int32 NumUsedModals)
{
	bIsParamsChanged = false;
	NumModals = NumUsedModals;
	Params.SetNumUninitialized(NumUsedModals * NUM_PARAM_PER_MODAL);
	CopyAllParams(InParams);
}

void FImpactModalObjAssetProxy::CopyAllParams(const TArrayView<const float>& SourceData)
{
	if(SourceData.Num() < Params.Num())
	{
		UE_LOG(LogImpactSFXSynth, Error, TEXT("FImpactModalObjAssetProxy::CopyAllParams: source data length is invalid!"));
		return;
	}
	
	FMemory::Memcpy(Params.GetData(), SourceData.GetData(), Params.Num() * sizeof(float));
}
