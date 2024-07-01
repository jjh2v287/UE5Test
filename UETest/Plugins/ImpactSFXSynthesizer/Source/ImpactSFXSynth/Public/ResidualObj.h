// Copyright 2023-2024, Le Binh Son, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "DSP/AlignedBuffer.h"
#include "ResidualObj.generated.h"

class UAssetImportData;

UCLASS(hidecategories=Object, BlueprintType)
class IMPACTSFXSYNTH_API UResidualObj : public UObject
{
	GENERATED_BODY()
public:
	static constexpr int32 NumMinErbFrames = 2;
	
private:
	
	UPROPERTY(VisibleAnywhere, Category = "Version")
	int32 Version;

	UPROPERTY(VisibleAnywhere, Category = "Config")
	int32 NumFFT;

	UPROPERTY(VisibleAnywhere, Category = "Config")
	int32 HopSize;

	UPROPERTY(VisibleAnywhere, Category = "Config")
	int32 NumErb;

	UPROPERTY(VisibleAnywhere, Category = "Config")
	int32 NumFrame;

	UPROPERTY(VisibleAnywhere, Category = "Config")
	float SamplingRate;
	
	UPROPERTY(VisibleAnywhere, Category = "Config")
	float ErbMax;

	UPROPERTY(VisibleAnywhere, Category = "Config")
	float ErbMin;

	UResidualObj(const FObjectInitializer& ObjectInitializer);
	
public:
	Audio::FAlignedFloatBuffer Data;

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Instanced, Category = ImportSettings)
	TObjectPtr<class UAssetImportData> AssetImportData;
#endif

	int32 GetVersion() const { return Version; }
	int32 GetNumFFT() const { return NumFFT; }
	int32 GetHopSize() const { return HopSize; }
	int32 GetNumErb() const { return NumErb; }
	int32 GetNumFrame() const { return NumFrame; }
	float GetSamplingRate() const { return SamplingRate; }
	float GetErbMin() const { return ErbMin; }
	float GetErbMax() const { return ErbMax; }

	TArrayView<const float> GetDataView() const { return TArrayView<const float>(Data); }
	
	//~ Begin UObject Interface. 
	virtual void Serialize(FArchive& Ar) override;
	virtual void PostInitProperties() override;
	//~ End UObject Interface.
	
	void SetProperties(const int32 InVersion, const int32 InNumFFT, const int32 InHopSize,
					  const int32 InNumErb, const int32 InNumFrame, const float InSamplingRate,
					  const float InErbMax, const float InErbMin);
	
};