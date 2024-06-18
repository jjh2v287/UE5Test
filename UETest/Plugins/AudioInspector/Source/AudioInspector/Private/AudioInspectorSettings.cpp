// Copyright SweejTech Ltd. All Rights Reserved.


#include "AudioInspectorSettings.h"

#include "Modules/ModuleManager.h"
#include "AudioInspector.h"
#include "SweejNameof.h"

void UAudioInspectorSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName PropertyName = PropertyChangedEvent.GetPropertyName();

	// When the active sounds update interval setting changes, contact the Audio Inspector module and change its update interval
	if (PropertyName == FName( SWEEJ_NAMEOF(ActiveSoundsUpdateInterval) ))
	{
		FAudioInspectorModule::GetModule().SetUpdateInterval(ActiveSoundsUpdateInterval);
	}

	// If any of the column toggles change, rebuild the active sounds table columns
	if (PropertyName == SWEEJ_FNAMEOF(ShowNameColumn) ||
		PropertyName == SWEEJ_FNAMEOF(ShowDistanceColumn) ||
		PropertyName == SWEEJ_FNAMEOF(ShowVolumeColumn) || 
		PropertyName == SWEEJ_FNAMEOF(ShowAttenuationColumn) || 
		PropertyName == SWEEJ_FNAMEOF(ShowSoundClassColumn) ||
		PropertyName == SWEEJ_FNAMEOF(ShowAzimuthColumn) ||
		PropertyName == SWEEJ_FNAMEOF(ShowAngleToListenerColumn) ||
		PropertyName == SWEEJ_FNAMEOF(ShowElevationColumn))
	{
		FAudioInspectorModule::GetModule().RebuildActiveSoundsColumns();
	}
}