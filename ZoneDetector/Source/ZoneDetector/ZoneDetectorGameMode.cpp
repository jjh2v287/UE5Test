// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZoneDetectorGameMode.h"
#include "ZoneDetectorCharacter.h"
#include "UObject/ConstructorHelpers.h"

AZoneDetectorGameMode::AZoneDetectorGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
