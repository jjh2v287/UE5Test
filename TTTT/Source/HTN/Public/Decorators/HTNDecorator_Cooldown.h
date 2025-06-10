// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "HTNTypes.h"
#include "HTNDecorator.h"
#include "HTNExtension.h"
#include "GameplayTagContainer.h"
#include "HTNDecorator_Cooldown.generated.h"

// Cooldown decorator node.
// A decorator node that bases its condition on whether a cooldown timer has expired.
UCLASS()
class HTN_API UHTNDecorator_Cooldown : public UHTNDecorator
{
	GENERATED_BODY()
	
public:
	UHTNDecorator_Cooldown(const FObjectInitializer& Initializer);

	virtual FString GetNodeName() const override;
	virtual FString GetStaticDescription() const override;
	virtual uint16 GetInstanceMemorySize() const override;
	virtual void InitializeMemory(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlan& Plan, const FHTNPlanStepID& StepID) const override;
	
	// If true, the cooldown will be locked after execution. 
	// If false, the decorator will only check if the cooldown is off on the specified GameplayTag.
	UPROPERTY(EditAnywhere, Category = "Cooldown")
	uint8 bLockCooldown : 1;

	UPROPERTY(EditAnywhere, Category = "Cooldown", Meta = (EditCondition = "bLockCooldown", EditConditionHides))
	uint8 bLockEvenIfExecutionAborted : 1;

	UPROPERTY(EditAnywhere, Category = "Cooldown", Meta = (ForceUnits = s, EditCondition = "bLockCooldown", EditConditionHides))
	float CooldownDuration;

	// If not zero, the cooldown duration will be in range [CooldownDuration - RandomDeviation, CooldownDuration + RandomDeviation]
	UPROPERTY(EditAnywhere, Category = "Cooldown", Meta = (ForceUnits = s, UIMin = 0, ClampMin = 0, EditCondition = "bLockCooldown", EditConditionHides))
	float RandomDeviation;

	// If not None, all decorators with this tag are connected (within the same HTNComponent).
	// In that case it is also possible possible to influence decorators with this gameplay tag by using AddCooldownTagDuration.
	UPROPERTY(EditAnywhere, Category = "Cooldown")
	FGameplayTag GameplayTag;

protected:
	virtual bool CalculateRawConditionValue(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNDecoratorConditionCheckType CheckType) const override;
	virtual void OnExecutionFinish(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNNodeResult Result) override;

	struct FNodeMemory
	{
		bool bIsIfNodeFalseBranch = false;
	};

private:
	float GetCooldownEndTime(UHTNComponent& OwnerComp) const;
	void LockCooldown(UHTNComponent& OwnerComp);
};

UCLASS()
class HTN_API UHTNExtension_Cooldown : public UHTNExtension
{
	GENERATED_BODY()

public:
	virtual void HTNStarted() override;

#if ENABLE_VISUAL_LOG
	virtual void DescribeSelfToVisLog(struct FVisualLogEntry* Snapshot) const override;
#endif

	UFUNCTION(BlueprintCallable, Category = "AI|HTN|Cooldown")
	float GetCooldownEndTime(const UObject* CooldownOwner) const;

	UFUNCTION(BlueprintCallable, Category = "AI|HTN|Cooldown")
	float GetTagCooldownEndTime(const FGameplayTag& CooldownTag) const;

	UFUNCTION(BlueprintCallable, Category = "AI|HTN|Cooldown")
	void AddCooldownDuration(const UObject* CooldownOwner, float CooldownDuration, bool bAddToExistingDuration);

	UFUNCTION(BlueprintCallable, Category = "AI|HTN|Cooldown")
	void AddTagCooldownDuration(const FGameplayTag& CooldownTag, float CooldownDuration, bool bAddToExistingDuration);

	UFUNCTION(BlueprintCallable, Category = "AI|HTN|Cooldown")
	void ResetCooldownsByTag(const FGameplayTag& CooldownTag);

	UFUNCTION(BlueprintCallable, Category = "AI|HTN|Cooldown")
	void ResetAllCooldownsWithoutGameplayTag();

	UFUNCTION(BlueprintCallable, Category = "AI|HTN|Cooldown")
	void ResetAllCooldowns();

private:
	// Map from cooldown owners (usually HTNDecorator_Cooldown nodes) without a gameplay tag to time of ending the cooldown.
	UPROPERTY()
	TMap<TObjectPtr<const UObject>, float> CooldownOwnerToEndTime;

	// Maps from gameplay tag to their cooldown end times.
	UPROPERTY()
	TMap<FGameplayTag, float> CooldownTagToEndTime;
};
