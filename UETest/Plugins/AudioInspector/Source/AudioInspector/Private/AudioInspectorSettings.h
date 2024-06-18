// Copyright 2023, SweejTech Ltd. All Rights Reserved.

#pragma once

#include "Engine/DeveloperSettings.h"

#include "AudioInspectorSettings.generated.h"

UENUM()
enum class EAudioInspectorVolumeColoringMode : uint8
{
	None			UMETA(DisplayName = "None"),
	VUMeter			UMETA(DisplayName = "VU Meter"),
	Relevance		UMETA(DisplayName = "Relevance"),
	
	Max				UMETA(Hidden)
};

/**
 * 
 */
UCLASS(config = EditorPerProjectUserSettings, meta = (DisplayName = "Audio Inspector"))
class UAudioInspectorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	/* How frequently the Active Sounds view updates, in seconds */
	UPROPERTY(config, EditAnywhere, Category = "Active Sounds", meta = (ClampMin = 0, Units = "s"))
	float ActiveSoundsUpdateInterval = 0.1f;

	UPROPERTY(config, EditAnywhere, Category = "Active Sounds")
	EAudioInspectorVolumeColoringMode VolumeColoringMode = EAudioInspectorVolumeColoringMode::Relevance;

	UPROPERTY(config, EditAnywhere, Category = "Active Sounds|Columns")
	bool ShowNameColumn = true;

	UPROPERTY(config, EditAnywhere, Category = "Active Sounds|Columns")
	bool ShowDistanceColumn = true;

	UPROPERTY(config, EditAnywhere, Category = "Active Sounds|Columns")
	bool ShowVolumeColumn = true;

	UPROPERTY(config, EditAnywhere, Category = "Active Sounds|Columns")
	bool ShowSoundClassColumn = true;

	UPROPERTY(config, EditAnywhere, Category = "Active Sounds|Columns")
	bool ShowAttenuationColumn = true;

	UPROPERTY(config, EditAnywhere, Category = "Active Sounds|Columns")
	bool ShowAngleToListenerColumn = false;

	UPROPERTY(config, EditAnywhere, Category = "Active Sounds|Columns")
	bool ShowAzimuthColumn = false;

	UPROPERTY(config, EditAnywhere, Category = "Active Sounds|Columns")
	bool ShowElevationColumn = false;


	//~ Begin UDeveloperSettings
	virtual FName GetCategoryName() const override { return TEXT("SweejTech"); }
	//~ End UDeveloperSettings

	//~ Begin UObject
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	//~ End UObject
};
