// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "HTNDecorator.h"
#include "HTNDecorator_BlackboardBase.generated.h"

// A base class for decorators parametrized with a single blackboard key.
UCLASS(Abstract)
class HTN_API UHTNDecorator_BlackboardBase : public UHTNDecorator
{
	GENERATED_BODY()

public:
	UHTNDecorator_BlackboardBase(const FObjectInitializer& Initializer);
	virtual void InitializeFromAsset(UHTN& Asset) override;
	virtual uint16 GetSpecialMemorySize() const override;

	FORCEINLINE FName GetSelectedBlackboardKey() const { return BlackboardKey.SelectedKeyName; }

#if WITH_EDITOR
	virtual FName GetNodeIconName() const override;
#endif
	
protected:
	virtual void OnExecutionStart(UHTNComponent& OwnerComp, uint8* NodeMemory) override; 
	virtual void OnExecutionFinish(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNNodeResult Result) override;
	virtual EBlackboardNotificationResult OnBlackboardKeyValueChange(const UBlackboardComponent& Blackboard, FBlackboard::FKey ChangedKeyID, uint8* NodeMemory);
	
	struct FHTNDecoratorBlackboardBaseSpecialMemory : public FHTNDecoratorSpecialMemory
	{
		FDelegateHandle OnBlackboardKeyValueChangeHandle;
	};

	// Blackboard key selector
	UPROPERTY(EditAnywhere, Category = Blackboard)
	FBlackboardKeySelector BlackboardKey;

	// If set (true by default), OnExecutionStart will subscribe the OnBlackboardKeyValueChange function to changes of the BlackboardKey.
	// OnExecutionFinish will unsubscribe.
	UPROPERTY(EditDefaultsOnly, Category = Blackboard)
	uint8 bNotifyOnBlackboardKeyValueChange : 1;
};
