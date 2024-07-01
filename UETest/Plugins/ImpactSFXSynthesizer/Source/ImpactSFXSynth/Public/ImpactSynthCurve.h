// Copyright 2023-2024, Le Binh Son, All rights reserved.

#pragma once

#include "Curves/RichCurve.h"
#include "Curves/CurveFloat.h"
#include "ImpactSynthCurve.generated.h"

class UCurveFloat;

UENUM()
enum class EImpactSynthCurveSource : uint8
{
	None = 0,
	Custom,
	Shared,
	
	Generated		UMETA(Hidden),
	Count		UMETA(Hidden),
};

UENUM()
enum class EImpactSynthCurveDisplayType : uint8
{
	ScaleOverTime UMETA(DisplayName = "Scale over Time"),
	ScaleOverFrequency UMETA(DisplayName = "Scale over Frequency"),
	MagnitudeOverTime UMETA(DisplayName = "Magnitude Over Time")
};

USTRUCT(BlueprintType)
struct FImpactSynthCurve
{
	GENERATED_BODY()

	static constexpr float FreqMax = 20000.f;
	static constexpr float ErbMax =  41.52404151352981f;
	
	UPROPERTY(EditAnywhere, Category = Input, BlueprintReadWrite, meta = (DisplayName = "Curve Type"))
	EImpactSynthCurveSource CurveSource = EImpactSynthCurveSource::None;

	UPROPERTY()
	EImpactSynthCurveDisplayType DisplayType  = EImpactSynthCurveDisplayType::ScaleOverTime;
	
	UPROPERTY()
	FRichCurve CurveCustom;

	/** Asset curve reference to apply if output curve type is set to 'Shared.' */
	UPROPERTY(EditAnywhere, meta = (DisplayName = "Asset"), Category = Curve, BlueprintReadWrite)
	TObjectPtr<UCurveFloat> CurveShared = nullptr;

	UPROPERTY()
	FString DisplayName;

	FImpactSynthCurve() : DisplayName(TEXT("Scale over Time")) { }
	
	FImpactSynthCurve(EImpactSynthCurveSource InSource, EImpactSynthCurveDisplayType InDisplayType, const FString& InString)
	: CurveSource(InSource), DisplayType(InDisplayType), DisplayName(InString) { }
	
	/** Caches curve data.  Should a shared curve be selected, curve points are copied locally
	  * to CurveCustom and Curve type is set accordingly.  Can be used to both avoid keeping a
	  * uobject reference should the transform be used on a non-game thread and potentially to
	  * further customize a referenced curve locally.
	  */
	void CacheCurve();
	
	static bool TryGetMinValue(EImpactSynthCurveDisplayType InDisplayType, double& MinValue)
	{
		MinValue = 0.;
		return true;
	}
	
	static bool TryGetTimeRangeX(EImpactSynthCurveDisplayType InDisplayType, double& MinValue, double& MaxValue)
	{
		switch (InDisplayType)
		{
		case EImpactSynthCurveDisplayType::MagnitudeOverTime:
		case EImpactSynthCurveDisplayType::ScaleOverTime:
			break;
		case EImpactSynthCurveDisplayType::ScaleOverFrequency:
			MinValue = 0.;
			MaxValue = 1.;
			return true;
		default:
			break;
		}
		
		return false;
	}
};