// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HTNTask.h"
#include "HTNTask_BlackboardBase.generated.h"

// A base class for tasks parametrized with a single blackboard key.
UCLASS(Abstract)
class HTN_API UHTNTask_BlackboardBase : public UHTNTask
{
	GENERATED_BODY()

public:
	virtual void InitializeFromAsset(UHTN& Asset) override;
	FORCEINLINE FName GetSelectedBlackboardKey() const { return BlackboardKey.SelectedKeyName; }

protected:
	// Blackboard key selector
	UPROPERTY(EditAnywhere, Category = Blackboard)
	FBlackboardKeySelector BlackboardKey;
};