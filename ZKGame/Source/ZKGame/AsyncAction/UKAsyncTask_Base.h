// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
#include "Engine/CancellableAsyncAction.h"
#include "UKAsyncTask_Base.generated.h"

UCLASS(Abstract)
class UUKAsyncTask_Base : public UCancellableAsyncAction, public FTickableGameObject
{

	GENERATED_UCLASS_BODY()
	
protected:
	bool bTickable = false;

public:
	// UBlueprintAsyncActionBase interface
	virtual void Activate() override;
	// ~UBlueprintAsyncActionBase interface
	
protected:
	// UCancellableAsyncAction interface
	virtual void Cancel() override;
	// ~UCancellableAsyncAction interface

	// FTickableGameObject Interface.
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;
	virtual bool IsTickableInEditor()const override;
	// ~FTickableGameObject Interface.
};
