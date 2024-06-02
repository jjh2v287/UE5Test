// (c) 2021 Sergio Lena. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ActionsManager.generated.h"

class UAction;
/**
 * Manager for managing  Action lifetime.
 */
UCLASS(Config=Game, meta=(DisplayName=" Actions Manager"))
class ACTIONS_API UActionsManager : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:

	/**
	 * Lifetime for all AI actions after their execution has finished.
	 */
	UPROPERTY(BlueprintReadOnly, Config, Category = "Action")
	float LifetimeAfterExecutionStopped = 30.f;

	// Called to add an AI Action to a set of Actions which will not be garbage collected.
	static void RegisterAction(UAction* Action);

	// Called to move an AI Action of a controller to a delete buffer, where it will stay for LifetimeAfterExecutionStopped before being garbage collected.
	static void DeregisterAction(UAction* Action);

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override
	{
		return GetStatID();
	}

private:

	/**
	 * A map of all running Actions.
	 */
	UPROPERTY()
	TArray<UAction*> RunningActions;

	/**
	 * A list of Actions to be deleted.
	 */
	UPROPERTY()
	TMap<UAction*, float> DeleteBuffer;

	// Gets the manager statically.
	static UActionsManager* Get();
	
};
