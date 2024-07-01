// Copyright 2023-2024, Le Binh Son, All rights reserved.


#include "ResidualObj.h"

#include "ImpactSFXSynthLog.h"
#include "EditorFramework/AssetImportData.h"
#include "ImpactSFXSynth/Public/Utils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ResidualObj)

UResidualObj::UResidualObj(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UResidualObj::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar << Version;
	Ar << NumFFT;
	Ar << HopSize;
	Ar << NumErb;
	Ar << NumFrame;
	Ar << SamplingRate;
	Ar << Data;
}

void UResidualObj::PostInitProperties()
{
	UObject::PostInitProperties();
	
#if WITH_EDITORONLY_DATA
    if (!HasAnyFlags(RF_ClassDefaultObject))
    {
    	AssetImportData = NewObject<UAssetImportData>(this, TEXT("AssetImportData"));
    }
#endif
}

void UResidualObj::SetProperties(const int32 InVersion, const int32 InNumFFT, const int32 InHopSize,
								 const int32 InNumErb, const int32 InNumFrame, const float InSamplingRate,
								 const float InErbMax, const float InErbMin)
{
	Version = InVersion;
	
	NumFFT = InNumFFT;
	HopSize = InHopSize;
	NumErb = InNumErb;
	NumFrame = InNumFrame;
	SamplingRate = InSamplingRate;
	ErbMax = InErbMax;
	ErbMin = InErbMin;
	
	if(!LBSImpactSFXSynth::IsPowerOf2(NumFFT))
		UE_LOG(LogImpactSFXSynth, Error, TEXT("UResidualObj::SetProperties: Expect NumFFT is a power of 2 not %d"), NumFFT);

	if(NumErb < 4)
		UE_LOG(LogImpactSFXSynth, Error, TEXT("UResidualObj::SetProperties: Expect NumErb is equal or larger than 4"));

	if(NumFrame < 2)
		UE_LOG(LogImpactSFXSynth, Error, TEXT("UResidualObj::SetProperties: Expect NumFrame is equal or larger than 2"));
}