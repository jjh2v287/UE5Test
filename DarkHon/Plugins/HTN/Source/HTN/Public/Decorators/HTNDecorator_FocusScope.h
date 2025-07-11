// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "UObject/ObjectMacros.h"
#include "Misc/TVariant.h"
#include "HTNDecorator.h"
#include "HTNDecorator_FocusScope.generated.h"

// On execution start, optionally sets the focus of the AIController to the value of a blackboard key.
// On execution finish, restores the focus back to its original value.
UCLASS()
class HTN_API UHTNDecorator_FocusScope : public UHTNDecorator
{
	GENERATED_BODY()

public:
	UHTNDecorator_FocusScope(const FObjectInitializer& Initializer);

	virtual FString GetNodeName() const override;
	virtual FString GetStaticDescription() const override;
	
	virtual void InitializeFromAsset(UHTN& Asset) override;
	virtual uint16 GetInstanceMemorySize() const override;
	virtual void InitializeMemory(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlan& Plan, const FHTNPlanStepID& StepID) const override;
	
	virtual void TickNode(UHTNComponent& OwnerComp, uint8* NodeMemory, float DeltaTime) override;

protected:
	virtual void OnExecutionStart(UHTNComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnExecutionFinish(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNNodeResult Result) override;

	bool IsFocusTargetRotator() const;
	bool ShouldUpdateFocalPointEveryFrame() const;

	// If true, will set the focus of the AIController to the value of the FocusTarget blackboard key.
	// Upon execution finish, the focus will be restored to the value it had on execution start regardless.
	UPROPERTY(EditAnywhere, Category = Focus)
	uint8 bSetNewFocus : 1;
	
	// The blackboard key on which to focus. Actor, Vector, and Rotator keys are supported.
	// If they key is a Rotator, the focal point of the AIController will be updated every frame in case the pawn moves (see RotationKeyLookAheadDistance).
	UPROPERTY(EditAnywhere, Category = Focus, Meta = (EditCondition = "bSetNewFocus"))
	FBlackboardKeySelector FocusTarget;

	// If true, the decorator will respond to changes in the FocusTarget key.
	UPROPERTY(EditAnywhere, Category = Focus, Meta = (EditCondition = "bSetNewFocus"))
	uint8 bObserveBlackboardValue : 1;
	
	// When the FocusTarget is a Rotator key, should we update the focal point (as specified by RotationKeyLookAheadDistance) every frame, 
	// or just on start (and possibly when the Blackboard key changes).
	UPROPERTY(EditAnywhere, Category = Focus, Meta = (EditCondition = "bSetNewFocus"))
	uint8 bUpdateFocalPointFromRotatorKeyEveryFrame : 1;

	// If true, OnExecutionFinish focus will be restored to the value it had before entering this focus.
	UPROPERTY(EditAnywhere, Category = Focus)
	uint8 bRestoreOldFocusOnExecutionFinish : 1;

	// AIControllers allow multiple focuses to be active at the same time.
	// The Blueprint functions SetFocus and SetFocalPoint use priority 2 (the highest - Gameplay).
	// See EAIFocusPriority.
	UPROPERTY(EditAnywhere, Category = Focus)
	uint8 FocusPriority;

	// If the FocusTarget is a Rotator key, the focal point will be set to (Pawn->GetPawnViewLocation() + Rotation * RotationKeyLookAheadDistance).
	// It will be updated each frame.
	UPROPERTY(EditAnywhere, Category = Focus, Meta = (ForceUnits = "cm"))
	float RotationKeyLookAheadDistance;

	struct FNodeMemory
	{
		TVariant<TWeakObjectPtr<AActor>, FVector> OldFocus;
		FDelegateHandle BlackboardObserverHandle;

		// When the FocusTarget is a Rotator key, we store the rotator in memory so that 
		// if bObserveBlackboardValue is false we keep using the value we had when the decorator began execution.
		FRotator RotationToFocusAlong;
	};

private:
	EBlackboardNotificationResult OnBlackboardKeyValueChange(const class UBlackboardComponent& Blackboard, FBlackboard::FKey ChangedKeyID, uint8* NodeMemory);
	void SetFocusFromBlackboard(const class UBlackboardComponent& Blackboard, const FNodeMemory& Memory);
	static FString AILocationToString(const FVector& Location);
	static FString AIRotationToString(const FRotator& Rotation);
};