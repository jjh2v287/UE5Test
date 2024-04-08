// Copyright Epic Games, Inc. All Rights Reserved.

#include "UETestGameMode.h"
#include "UETestCharacter.h"
#include "UObject/ConstructorHelpers.h"

AUETestGameMode::AUETestGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
