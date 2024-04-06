// Copyright Epic Games, Inc. All Rights Reserved.

#include "UE5TestGameMode.h"
#include "UE5TestCharacter.h"
#include "UObject/ConstructorHelpers.h"

AUE5TestGameMode::AUE5TestGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
