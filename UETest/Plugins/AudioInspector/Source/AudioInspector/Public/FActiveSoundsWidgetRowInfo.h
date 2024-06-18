// Copyright 2023, SweejTech Ltd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class USoundBase;
class USoundClass;

struct FActiveSoundsWidgetRowInfo
{
	FActiveSoundsWidgetRowInfo()
		: Sound(nullptr)
		, SoundClass(nullptr)
		, Name()
		, AzimuthAndElevation(FVector2f::ZeroVector)
		, Distance(0.0f)
		, AngleToListener(0.0f)
		, Volume(0.0f)
		, AtOrigin(false)
	{
	}

	TWeakObjectPtr<USoundBase> Sound;

	TWeakObjectPtr<USoundClass> SoundClass;

	FName Name;

	FVector2f AzimuthAndElevation;

	float Distance;

	float AngleToListener;

	float Volume;

	bool AtOrigin;
};

typedef TSharedPtr<FActiveSoundsWidgetRowInfo> FActiveSoundsWidgetRowInfoPtr;