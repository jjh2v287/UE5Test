// (c) 2021 Sergio Lena. All Rights Reserved.

#include "ActionsStatics.h"


UAction* UActionsStatics::RunAction(const TSubclassOf<UAction> Action)
{


	if (!ensureAlwaysMsgf(IsValid(Action), TEXT("UActionsStatics::RunAction(): Action class is not valid.")))
	{
		return nullptr;
	}

	UAction* NewAction = NewObject<UAction>(GetTransientPackage(), Action);
	if (!ensureAlwaysMsgf(IsValid(NewAction), TEXT("UActionsStatics::RunAction(): could not spawn a Action.")))
	{
		return nullptr;
	}

	// This will announce the Action to the ActionsManager, where it will be safe from garbage collection.
	NewAction->Initialize();
	return NewAction;
}
