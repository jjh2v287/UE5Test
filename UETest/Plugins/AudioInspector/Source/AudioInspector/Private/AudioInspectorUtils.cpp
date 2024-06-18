// Copyright SweejTech Ltd. All Rights Reserved.


#include "AudioInspectorUtils.h"

FLinearColor AudioInspectorUtils::GetVUMeterColorForVolumeDecibels(float VolumeInDecibels)
{
	FLinearColor Color;

	if (VolumeInDecibels >= GetClippingThreshold())
	{
		// White hot!
		Color = FLinearColor::White;
	}
	else if (VolumeInDecibels >= GetHotSignalThreshold())
	{
		// Hot Red
		FLinearColor HotRed = FLinearColor(1.0f, 0.1f, 0.1f);
		
		// Burnt orange
		FLinearColor BurntOrange = FLinearColor(0.996f, 0.532f, 0.0f);

		// Interpolate
		Color = FMath::Lerp(HotRed, BurntOrange,
			FMath::GetMappedRangeValueClamped(FVector2f(GetHotSignalThreshold(), GetWarmSignalThreshold()), FVector2f(0.0f, 1.0f), VolumeInDecibels));
	}
	else if (VolumeInDecibels >= GetWarmSignalThreshold())
	{
		Color = FLinearColor::Yellow;
	}
	else if (VolumeInDecibels >= GetCoolSignalThreshold())
	{
		// Slightly less bright green
		Color = FLinearColor(0.0f, 0.9f, 0.0f);
	}
	else
	{
		Color = FMath::Lerp(FLinearColor::Green, FLinearColor::Blue, 0.15f);
	}
	
	Color = FMath::Lerp(Color, Color * 0.35f, FMath::GetMappedRangeValueClamped(FVector2f(-160.0f, -6.0f), FVector2f(1.0f, 0.0f), VolumeInDecibels));

	return Color;
}

FLinearColor AudioInspectorUtils::GetRelevanceColorForVolumeDecibels(float VolumeInDecibels)
{
	FLinearColor Color;

	if (VolumeInDecibels >= GetClippingThreshold())
	{
		Color = FLinearColor::White;
	}
	else if (VolumeInDecibels >= GetHotSignalThreshold())
	{
		// Slightly less bright green
		Color = FLinearColor(0.0f, 0.9f, 0.0f);
	}
	else if (VolumeInDecibels >= GetWarmSignalThreshold())
	{
		Color = FLinearColor::Yellow;
	}
	else if (VolumeInDecibels >= GetCoolSignalThreshold())
	{
		Color = FLinearColor::Blue.Desaturate(0.5f);
		Color.A = 1.0f;
	}
	else
	{
		Color = FLinearColor::White * 0.7f;
	}

	return Color;
}