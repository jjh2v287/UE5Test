// Copyright SweejTech Ltd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class AUDIOINSPECTOR_API AudioInspectorUtils
{
public:

	static constexpr float GetClippingThreshold() { return 0.0f; }
	static constexpr float GetHotSignalThreshold() { return -6.0f; }
	static constexpr float GetWarmSignalThreshold() { return -12.0f; }
	static constexpr float GetCoolSignalThreshold() { return -18.0f; }

	static FLinearColor GetVUMeterColorForVolumeDecibels(float VolumeInDecibels);

	static FLinearColor GetRelevanceColorForVolumeDecibels(float VolumeInDecibels);
};
