// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "HTNExtension.h"
#include "HTNNode.h"
#include "HTNService.generated.h"

struct HTN_API FHTNServiceSpecialMemory : public FHTNNodeSpecialMemory
{
	FHTNServiceSpecialMemory(float Interval);

	FIntervalCountdown TickCountdown;
	bool bFirstTick;
};

// A subnode used for updating values and generally running code at regular intervals.
UCLASS(Abstract)
class HTN_API UHTNService : public UHTNNode
{
	GENERATED_BODY()

public:
	UHTNService(const FObjectInitializer& Initializer);
	virtual FString GetStaticDescription() const override;
	virtual uint16 GetSpecialMemorySize() const override;
	virtual void InitializeSpecialMemory(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlan& Plan, const FHTNPlanStepID& StepID) const override;

	void WrappedExecutionStart(UHTNComponent& OwnerComp, uint8* NodeMemory) const;
	void WrappedTickNode(UHTNComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) const;
	void WrappedExecutionFinish(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNNodeResult NodeResult) const;

protected:
	virtual FString GetStaticServiceDescription() const;
	FString GetStaticTickIntervalDescription() const;
	
	virtual void OnExecutionStart(UHTNComponent& OwnerComp, uint8* NodeMemory) {}
	virtual void TickNode(UHTNComponent& OwnerComp, uint8* NodeMemory, float DeltaTime) {}
	virtual void OnExecutionFinish(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNNodeResult Result) {}

	float GetInterval() const;
	
	UPROPERTY(EditAnywhere, Category = Service, Meta = (ClampMin = "0.001", ForceUnits = s))
	float TickInterval;
	
	UPROPERTY(EditAnywhere, Category = Service, Meta = (ClampMin = "0.0", ForceUnits = s))
	float TickIntervalRandomDeviation;

	// If true, the service will keep track of the countdown to the next tick in a way that persists between different plans.
	// E.g. if the service has a 5 second interval, but the plan was aborted after 3 seconds, this service will tick 2 seconds after the beginning of the next plan.
	// If false, the countdown is reset every time a new plan begins executing, causing the service to tick every time that happens.
	UPROPERTY(EditAnywhere, Category = Service)
	uint8 bUsePersistentTickCountdown : 1;
	
	bool bNotifyExecutionStart : 1;
	bool bNotifyTick : 1;
	bool bNotifyExecutionFinish : 1;
};

UCLASS()
class HTN_API UHTNExtension_ServicePersistentTickCountdown : public UHTNExtension
{
	GENERATED_BODY()

public:
	virtual void HTNStarted() override;

#if ENABLE_VISUAL_LOG
	virtual void DescribeSelfToVisLog(struct FVisualLogEntry* Snapshot) const override;
#endif

	bool GetPersistentTickCountdown(FIntervalCountdown& OutCountdown, const UHTNService* Service) const;
	void SetPersistentTickCountdown(const UHTNService* Service, const FIntervalCountdown& Countdown);

private:
	UPROPERTY()
	TMap<TObjectPtr<const UHTNService>, FIntervalCountdown> ServiceToCountdown;
};
