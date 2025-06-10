// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EngineDefines.h"
#include "HTNExtension.generated.h"

// The base class for HTN extensions.
// Extensions are owned by the HTNComponent. 
// They are used to store custom information persistent between plan executions
// (e.g. which DoOnce tags are active, or which dynamic subnetworks are associated with which gameplay tags).
// Use the FindOrAddExtensionByClass function of the HTNComponent to createand instance of it on the HTNComponent.
// For an example, see UHTNExtension_DoOnce and UHTNDecorator_DoOnce.
UCLASS(Abstract, BlueprintType, DefaultToInstanced)
class HTN_API UHTNExtension : public UObject
{
	GENERATED_BODY()

public:
	UHTNExtension();

#if WITH_ENGINE
	UWorld* GetWorld() const override;
#endif

	virtual void Initialize() {};
	virtual void HTNStarted() {};
	virtual void Tick(float DeltaTime) {};
	virtual void Cleanup() {};

#if ENABLE_VISUAL_LOG
	virtual void DescribeSelfToVisLog(struct FVisualLogEntry* Snapshot) const {};
#endif

	UFUNCTION(BlueprintPure, Category = "HTN")
	class UHTNComponent* GetHTNComponent() const;

	FORCEINLINE bool WantsNotifyTick() const { return bNotifyTick; }

protected:
	uint8 bNotifyTick : 1;
};
