// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "HTNTypes.h"
#include "HTNDecorator.h"
#include "HTNExtension.h"
#include "GameplayTagContainer.h"
#include "HTNDecorator_DoOnce.generated.h"

// OnExecutionFinish, locks itself, so it will not pass during any subsequent planning.
// If a GameplayTag is specified, can be reset using that gameplay tag via 
// the SetDoOnceLocked helper function of the HTNExtension_DoOnce or using HTNTask_ResetDoOnce.
UCLASS()
class HTN_API UHTNDecorator_DoOnce : public UHTNDecorator
{
	GENERATED_BODY()
	
public:
	UHTNDecorator_DoOnce(const FObjectInitializer& Initializer);
	virtual FString GetNodeName() const override;
	virtual FString GetStaticDescription() const override;

	virtual uint16 GetInstanceMemorySize() const override;
	virtual void InitializeMemory(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlan& Plan, const FHTNPlanStepID& StepID) const override;

	bool IsLockableByGameplayTag() const;

	// If not None, it will be possible to lock/unlock all DoOnce decorators with this gameplay tag using SetDoOnceLocked.
	// If None, the decorator will still work, but the only way to unlock it would be 
	// to call ResetAllDoOnceDecoratorsWithoutGameplayTag or ResetAllDoOnceDecorators.
	UPROPERTY(EditAnywhere, Category = "Do Once")
	FGameplayTag GameplayTag;

	// If true, will lock the DoOnce even if the execution is aborted before finishing.
	UPROPERTY(EditAnywhere, Category = "Do Once")
	uint8 bLockEvenIfExecutionAborted : 1;

	// When the decorator fails during plan execution,
	// it will either abort the plan instantly (if ticked)
	// or wait until a new plan is made (if unticked).
	UPROPERTY(Category = Condition, EditAnywhere)
	uint8 bCanAbortPlanInstantly : 1;

protected:
	virtual bool CalculateRawConditionValue(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNDecoratorConditionCheckType CheckType) const override;
	virtual void OnExecutionStart(UHTNComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnExecutionFinish(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNNodeResult Result) override;

private:
	void OnGameplayTagLockStatusChanged(UHTNExtension_DoOnce& Sender, const FGameplayTag& ChangedTag, bool bNewLocked, uint8* NodeMemory);
	void OnDecoratorLockStatusChanged(UHTNExtension_DoOnce& Sender, const UHTNDecorator_DoOnce* Decorator, bool bNewLocked, uint8* NodeMemory);

	struct FNodeMemory
	{
		bool bIsIfNodeFalseBranch = false;
		FDelegateHandle OnGameplayTagLockStatusChangedHandle;
		FDelegateHandle OnDecoratorLockStatusChangedHandle;
	};
};

UCLASS()
class HTN_API UHTNExtension_DoOnce : public UHTNExtension
{
	GENERATED_BODY()

public:
	virtual void Cleanup() override;

#if ENABLE_VISUAL_LOG
	virtual void DescribeSelfToVisLog(struct FVisualLogEntry* Snapshot) const override;
#endif

	bool IsDoOnceLocked(const UHTNDecorator_DoOnce* Decorator) const;

	UFUNCTION(BlueprintCallable, Category = "AI|HTN|DoOnce", Meta = (ReturnDisplayName = "Is Locked"))
	bool IsDoOnceLocked(const FGameplayTag& Tag) const;

	bool SetDoOnceLocked(const UHTNDecorator_DoOnce* Decorator, bool bNewLocked = true);

	UFUNCTION(BlueprintCallable, Category = "AI|HTN|DoOnce", Meta = (ReturnDisplayName = "Lock Status Changed"))
	bool SetDoOnceLocked(const FGameplayTag& Tag, bool bNewLocked = true);

	UFUNCTION(BlueprintCallable, Category = "AI|HTN|DoOnce")
	void ResetAllDoOnceDecoratorsWithoutGameplayTag();

	UFUNCTION(BlueprintCallable, Category = "AI|HTN|DoOnce")
	void ResetAllDoOnceDecorators();

	DECLARE_EVENT_ThreeParams(UHTNExtension_DoOnce, FHTNGameplayTagLockStatusChangedSignature, UHTNExtension_DoOnce& /*Sender*/, const FGameplayTag& /*ChangedTag*/, bool /*bNewLocked*/)
	FHTNGameplayTagLockStatusChangedSignature OnGameplayTagLockStatusChangedDelegate;

	DECLARE_EVENT_ThreeParams(UHTNExtension_DoOnce, FHTNDecoratorLockStatusChangedSignature, UHTNExtension_DoOnce& /*Sender*/, const UHTNDecorator_DoOnce* /*Decorator*/, bool /*bNewLocked*/)
	FHTNDecoratorLockStatusChangedSignature OnDecoratorLockStatusChangedDelegate;

private:
	// The gameplay tags of DoOnce decorators that are blocked at the moment.
	UPROPERTY()
	TSet<FGameplayTag> LockedGameplayTags;

	// The DoOnce decorators without a gameplay tag which are blocked.
	UPROPERTY()
	TSet<TObjectPtr<const UHTNDecorator_DoOnce>> LockedDecorators;
};
