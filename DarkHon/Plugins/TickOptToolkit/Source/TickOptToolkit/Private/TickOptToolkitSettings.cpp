// Copyright 2021 Marcin Swiderski. All Rights Reserved.

#include "TickOptToolkitSettings.h"

TICKOPTTOOLKIT_API TAutoConsoleVariable<int32> CVarTickOptToolkitTickOptimizationLevel(
	TEXT("tot.TickOptimizationLevel"),
	0,
	TEXT("Optimization level used by the Tick Optimization Toolkit Target components."),
	ECVF_Default);

TICKOPTTOOLKIT_API TAutoConsoleVariable<float> CVarTickOptToolkitTickDistanceScale(
	TEXT("tot.TickDistanceScale"),
	1.0f,
	TEXT("Distance scale applied to all target component distance settings (sphere radius, box extents, mid zone sizes, buffer size). ")
	TEXT("Can be disabled on per target component basis."),
	ECVF_Default);

TICKOPTTOOLKIT_API TAutoConsoleVariable<int32> CVarTickOptToolkitUROOptimizationLevel(
	TEXT("tot.UROOptimizationLevel"),
	0,
	TEXT("Optimization level used by the Tick Optimization Toolkit Anim Update Rate Optimization components."),
	ECVF_Default);

TICKOPTTOOLKIT_API TAutoConsoleVariable<float> CVarTickOptToolkitUROScreenSizeScale(
	TEXT("tot.UROScreenSizeScale"),
	1.0f,
	TEXT("Screen size scale applied to frames skipped screen size thresholds. ")
	TEXT("Can be disabled on per Anim Update Rate Optimization component basis."),
	ECVF_Default);

#if WITH_EDITOR

void UTickOptToolkitSettings::PostInitProperties()
{
	Super::PostInitProperties();
	UpdateFocusLayerNameOverrides();
}

void UTickOptToolkitSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	static FName NAME_FocusLayerNameOverrides = TEXT("FocusLayerNameOverrides");
	if (PropertyChangedEvent.Property->GetFName() == NAME_FocusLayerNameOverrides)
	{
		UpdateFocusLayerNameOverrides();
	}
}

void UTickOptToolkitSettings::UpdateFocusLayerNameOverrides() const
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		if (const UEnum* FocusLayerEnum = StaticEnum<ETickOptToolkitFocusLayer>())
		{
			for (int I = 0; I < UE_ARRAY_COUNT(FocusLayerNameOverrides); ++I)
			{
				if (FocusLayerNameOverrides[I] != NAME_None)
				{
					FocusLayerEnum->SetMetaData(TEXT("DisplayName"), *FocusLayerNameOverrides[I].ToString(), I);
				}
				else
				{
					FocusLayerEnum->RemoveMetaData(TEXT("DisplayName"), I);
				}
			}
		}
	}
}

#endif // WITH_EDITOR
