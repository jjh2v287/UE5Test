// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UKAudioSettings.generated.h"

/**
 * 
 */
UCLASS()
class UETEST_API UUKAudioSettings : public UDeveloperSettings
{
	GENERATED_UCLASS_BODY()
public:
    // Registers Parameter Interfaces defined by the engine module.
    // Called on engine start-up. Can be called when engine is not
    // initialized as well by consuming plugins (ex. on cook by plugins
    // requiring interface system to be loaded).
    void RegisterParameterInterfaces();

private:
	bool bParameterInterfacesRegistered;
};
