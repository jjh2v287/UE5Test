// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "Decorators/HTNDecorator_BlackboardBase.h"
#include "HTNDecorator_Blackboard.generated.h"

// Checks a condition on the value of a key in the Blackboard/Worldstate.
UCLASS()
class HTN_API UHTNDecorator_Blackboard : public UHTNDecorator_BlackboardBase
{
	GENERATED_BODY()

public:
	UHTNDecorator_Blackboard(const FObjectInitializer& Initializer);

	virtual FString GetNodeName() const override;
	virtual FString GetStaticDescription() const override;

protected:
	virtual bool CalculateRawConditionValue(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNDecoratorConditionCheckType CheckType) const override;

	// Value for arithmetic operations
	UPROPERTY(Category=Blackboard, EditAnywhere, meta=(DisplayName="Key Value"))
	int32 IntValue;

	// Value for arithmetic operations
	UPROPERTY(Category=Blackboard, EditAnywhere, meta=(DisplayName="Key Value"))
	float FloatValue;

	// Value for string operations
	UPROPERTY(Category=Blackboard, EditAnywhere, meta=(DisplayName="Key Value"))
	FString StringValue;

	// Cached description
	UPROPERTY()
	FString CachedDescription;

	// Operation type
	UPROPERTY()
	uint8 OperationType;

	// When the decorator fails during plan execution,
	// it will either abort the plan instantly (if ticked)
	// of wait until a new plan is made (if unticked).
	UPROPERTY(Category = Condition, EditAnywhere)
	uint8 bCanAbortPlanInstantly : 1;
	
#if WITH_EDITORONLY_DATA

	UPROPERTY(Category=Blackboard, EditAnywhere, meta=(DisplayName="Key Query"))
	TEnumAsByte<EBasicKeyOperation::Type> BasicOperation;

	UPROPERTY(Category=Blackboard, EditAnywhere, meta=(DisplayName="Key Query"))
	TEnumAsByte<EArithmeticKeyOperation::Type> ArithmeticOperation;

	UPROPERTY(Category=Blackboard, EditAnywhere, meta=(DisplayName="Key Query"))
	TEnumAsByte<ETextKeyOperation::Type> TextOperation;

#endif

#if WITH_EDITOR

	// Describe decorator and cache it
	virtual void BuildDescription();
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void InitializeFromAsset(UHTN& Asset) override;

#endif

	virtual EBlackboardNotificationResult OnBlackboardKeyValueChange(const UBlackboardComponent& Blackboard, FBlackboard::FKey ChangedKeyID, uint8* NodeMemory) override;
	friend class FHTNBlackboardDecoratorDetails;

private:
	bool EvaluateConditionOnWorldState(const UWorldStateProxy& WorldStateProxy) const;
};