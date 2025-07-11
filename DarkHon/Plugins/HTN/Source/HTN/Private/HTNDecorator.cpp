// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "HTNDecorator.h"
#include "HTNObjectVersion.h"
#include "HTNPlan.h"

#include "Misc/RuntimeErrors.h"
#include "Misc/StringBuilder.h"
#include "VisualLogger/VisualLogger.h"
#include "WorldStateProxy.h"

UHTNDecorator::UHTNDecorator(const FObjectInitializer& Initializer) : Super(Initializer),
	bNotifyOnEnterPlan(false),
	bModifyStepCost(false),
	bNotifyOnExitPlan(false),
	bNotifyExecutionStart(false),
	bNotifyTick(false),
	bNotifyExecutionFinish(false),
	bInverseCondition(false),
	bCheckConditionOnPlanEnter(true),
	bCheckConditionOnPlanExit(false),
	bCheckConditionOnPlanRecheck(false),
	bCheckConditionOnTick(true),
	bCheckConditionOnTickOnlyOnce(false),
	ConditionCheckInterval(0.0f),
	ConditionCheckIntervalRandomDeviation(0.0f),
	TickFunctionInterval(0.0f),
	TickFunctionIntervalRandomDeviation(0.0f)
{}

void UHTNDecorator::Serialize(FArchive& Ar)
{
	Ar.UsingCustomVersion(FHTNObjectVersion::GUID);

	const int32 HTNObjectVersion = Ar.CustomVer(FHTNObjectVersion::GUID);
	if (HTNObjectVersion < FHTNObjectVersion::DisableDecoratorRecheckByDefault)
	{
		// When loading an older version asset, restore this value to the old default before loading.
		// If the value was changed in asset, it will be overwritten during loading. 
		// If it wasn't, it'll stay the old default.
		bCheckConditionOnPlanRecheck = true;
	}

	Super::Serialize(Ar);
}

FString UHTNDecorator::GetStaticDescription() const
{
	TStringBuilder<2048> SB;
	SB << (IsInversed() ? TEXT("(inversed)\n") : TEXT("")) << GetConditionCheckDescription();
	if (SB.Len() > 0)
	{
		SB << TEXT("\n");
	}

	SB << GetTickFunctionIntervalDescription();
	SB << Super::GetStaticDescription();

	return SB.ToString();
}

void UHTNDecorator::WrappedEnterPlan(UHTNComponent& OwnerComp, const FHTNPlan& Plan, const FHTNPlanStepID& StepID) const
{
	check(OwnerComp.GetPlanningWorldStateProxy()->IsWorldState());
	check(OwnerComp.GetPlanningWorldStateProxy()->IsEditable());
	
	if (bNotifyOnEnterPlan)
	{
		OnEnterPlan(OwnerComp, Plan, StepID);
	}
}

void UHTNDecorator::WrappedModifyStepCost(UHTNComponent& OwnerComp, FHTNPlanStep& Step) const
{
	check(OwnerComp.GetPlanningWorldStateProxy()->IsWorldState());
	
	if (bModifyStepCost)
	{
		const int32 CostBefore = Step.Cost;
		ModifyStepCost(OwnerComp, Step);
		const int32 CostAfter = Step.Cost;

		if (CostAfter < 0)
		{
			UE_VLOG_UELOG(OwnerComp.GetOwner(), LogHTN, Error, TEXT("UHTNDecorator: Plan step cost after ModifyStepCost was negative, which is not allowed. Resetting step cost to 0. When modifying node %s by decorator %s"), 
				*Step.Node->GetShortDescription(), *GetShortDescription());
			Step.Cost = 0;
		}
	}
}

void UHTNDecorator::WrappedExitPlan(UHTNComponent& OwnerComp, const FHTNPlan& Plan, const FHTNPlanStepID& StepID) const
{
	check(OwnerComp.GetPlanningWorldStateProxy()->IsWorldState());
	check(OwnerComp.GetPlanningWorldStateProxy()->IsEditable());
	
	if (bNotifyOnExitPlan)
	{
		OnExitPlan(OwnerComp, Plan, StepID);
	}
}

bool UHTNDecorator::WrappedRecheckPlan(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlanStep& SubmittedPlanStep) const
{
	check(!IsInstance());
	const EHTNDecoratorTestResult Result = WrappedTestCondition(OwnerComp, NodeMemory, EHTNDecoratorConditionCheckType::PlanRecheck);
	const bool bPassedCondition = Result != EHTNDecoratorTestResult::Failed;
	return bPassedCondition;
}

void UHTNDecorator::WrappedExecutionStart(UHTNComponent& OwnerComp, uint8* NodeMemory) const
{
	check(!IsInstance());
	UHTNDecorator* const Decorator = StaticCast<UHTNDecorator*>(GetNodeFromMemory(OwnerComp, NodeMemory));
	if (!ensure(Decorator))
	{
		return;
	}

	UE_VLOG(&OwnerComp, LogHTN, VeryVerbose, TEXT("decorator execution start: '%s'"), *Decorator->GetShortDescription());
	if (Decorator->bNotifyExecutionStart)
	{
		Decorator->OnExecutionStart(OwnerComp, NodeMemory);
	}
}

void UHTNDecorator::WrappedTickNode(UHTNComponent& OwnerComp, uint8* NodeMemory, float DeltaTime) const
{
	check(!IsInstance());
	UHTNDecorator* const Decorator = StaticCast<UHTNDecorator*>(GetNodeFromMemory(OwnerComp, NodeMemory));
	if (!ensure(Decorator))
	{
		return;
	}

	FHTNDecoratorSpecialMemory& Memory = *GetSpecialNodeMemory<FHTNDecoratorSpecialMemory>(NodeMemory);
	Memory.ConditionCheckCountdown.Tick(DeltaTime);

	if (Decorator->bNotifyTick && Memory.TickFunctionCountdown.Tick(DeltaTime))
	{
		UE_VLOG(&OwnerComp, LogHTN, VeryVerbose, TEXT("decorator tick '%s'"), *Decorator->GetShortDescription());

		const float EffectiveDeltaTime = Memory.bFirstTick ? DeltaTime : Memory.TickFunctionCountdown.GetElapsedTimeWithFallback(DeltaTime);
		Decorator->TickNode(OwnerComp, NodeMemory, EffectiveDeltaTime);

		Memory.bFirstTick = false;
		Memory.TickFunctionCountdown.Interval = GetTickFunctionInterval();
		Memory.TickFunctionCountdown.Reset();
	}
}

void UHTNDecorator::WrappedExecutionFinish(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNNodeResult NodeResult) const
{
	check(!IsInstance());
	UHTNDecorator* const Decorator = StaticCast<UHTNDecorator*>(GetNodeFromMemory(OwnerComp, NodeMemory));
	if (!ensure(Decorator))
	{
		return;
	}
	
	UE_VLOG(&OwnerComp, LogHTN, VeryVerbose, TEXT("decorator execution finish: '%s'"), *Decorator->GetShortDescription());
	if (bNotifyExecutionFinish)
	{
		Decorator->OnExecutionFinish(OwnerComp, NodeMemory, NodeResult);
	}
}

EHTNDecoratorTestResult UHTNDecorator::WrappedTestCondition(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNDecoratorConditionCheckType CheckType) const
{
	check(!IsInstance());
	const UHTNDecorator* const Decorator = NodeMemory ? 
		StaticCast<UHTNDecorator*>(GetNodeFromMemory(OwnerComp, NodeMemory)) :
		this;
	if (!ensure(Decorator))
	{
		return EHTNDecoratorTestResult::Failed;
	}

	return Decorator->TestCondition(OwnerComp, NodeMemory, CheckType);
}

EHTNDecoratorTestResult UHTNDecorator::TestCondition(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNDecoratorConditionCheckType CheckType) const
{
	if (ShouldCheckCondition(OwnerComp, NodeMemory, CheckType))
	{
		const bool bRawValue = CalculateRawConditionValue(OwnerComp, NodeMemory, CheckType);
		const bool bEffectiveValue = bInverseCondition ? !bRawValue : bRawValue;
		const EHTNDecoratorTestResult Result = bEffectiveValue ? EHTNDecoratorTestResult::Passed : EHTNDecoratorTestResult::Failed;
		UE_VLOG(&OwnerComp, LogHTN, VeryVerbose, TEXT("decorator check '%s' (%s): %s"),
			*GetShortDescription(),
			*UEnum::GetDisplayValueAsText(CheckType).ToString(),
			*UEnum::GetDisplayValueAsText(Result).ToString());

		if (CheckType == EHTNDecoratorConditionCheckType::Execution)
		{
			// We can't store during planning because the node memory doesn't exist at that point,
			// and we don't store the result of recheck because that might interfere with Execution checks that are happening on the current worldstate.
			// When event-based decorators notify the HTN component of their conditions, the other decorators 
			// should be evaluated on the current worldstate, not planned future worldstates.
			SetLastEffectiveConditionValue(NodeMemory, Result);

			// We reset the countdown for checking the condition next.
			FHTNDecoratorSpecialMemory& Memory = *GetSpecialNodeMemory<FHTNDecoratorSpecialMemory>(NodeMemory);
			Memory.ConditionCheckCountdown.Interval = GetConditionCheckInterval();
			Memory.ConditionCheckCountdown.Reset();
		}

		return Result;
	}

	// If bCheckConditionOnPlanRecheck is disabled, we want to return NotTested instead of the last tested execution test.
	if (CheckType == EHTNDecoratorConditionCheckType::Execution)
	{
		return GetLastEffectiveConditionValue(NodeMemory);	
	}

	return EHTNDecoratorTestResult::NotTested;
}

EHTNDecoratorTestResult UHTNDecorator::GetLastEffectiveConditionValue(const uint8* NodeMemory) const
{
	if (!NodeMemory)
	{
		return EHTNDecoratorTestResult::NotTested;
	}

	const FHTNDecoratorSpecialMemory& Memory = *GetSpecialNodeMemory<FHTNDecoratorSpecialMemory>(NodeMemory);
	return Memory.LastEffectiveConditionValue;
}

bool UHTNDecorator::ShouldCheckCondition(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNDecoratorConditionCheckType CheckType) const
{
	switch (CheckType)
	{
		case EHTNDecoratorConditionCheckType::PlanEnter: return bCheckConditionOnPlanEnter;
		case EHTNDecoratorConditionCheckType::PlanExit: return bCheckConditionOnPlanExit;
		case EHTNDecoratorConditionCheckType::PlanRecheck: return bCheckConditionOnPlanRecheck;
		case EHTNDecoratorConditionCheckType::Execution:
		{
			if (!bCheckConditionOnTick)
			{
				return false;
			}

			// If this function is called not during execution, but to check ahead of time if this node checks conditions during execution 
			// (e.g. to display things in editor etc.)
			if (!NodeMemory)
			{
				return true;
			}

			const FHTNDecoratorSpecialMemory& Memory = *GetSpecialNodeMemory<FHTNDecoratorSpecialMemory>(NodeMemory);
			// Don't check execution again if we only need to do that once and we've already done it.
			if (bCheckConditionOnTickOnlyOnce && Memory.LastEffectiveConditionValue != EHTNDecoratorTestResult::NotTested)
			{
				return false;
			}
			return Memory.ConditionCheckCountdown.TimeLeft <= 0;
		}
		default: return false;
	}
}

void UHTNDecorator::InitializeSpecialMemory(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlan& Plan, const FHTNPlanStepID& StepID) const
{
	FHTNDecoratorSpecialMemory* const Memory = new(GetSpecialNodeMemory<FHTNDecoratorSpecialMemory>(NodeMemory)) 
		FHTNDecoratorSpecialMemory(GetConditionCheckInterval(), GetTickFunctionInterval());
}

FString UHTNDecorator::GetConditionCheckDescription() const
{
	TArray<FString, TInlineAllocator<4>> CheckDescriptions;
	if (bCheckConditionOnPlanEnter)
	{
		CheckDescriptions.Add(TEXT("plan enter"));
	}

	if (bCheckConditionOnPlanExit)
	{
		CheckDescriptions.Add(TEXT("plan exit"));
	}

	if (bCheckConditionOnPlanRecheck)
	{
		CheckDescriptions.Add(TEXT("plan recheck"));
	}

	if (bCheckConditionOnTick)
	{
		if (bCheckConditionOnTickOnlyOnce)
		{
			CheckDescriptions.Add(TEXT("execution start"));
		}
		else
		{
			CheckDescriptions.Add(TEXT("tick"));
		}
	}

	TStringBuilder<2048> SB;

	if (CheckDescriptions.Num() > 0)
	{
		SB << TEXT("(checks on: ") << FString::Join(CheckDescriptions, TEXT(", ")) << TEXT(")");

		if (bCheckConditionOnTick && !bCheckConditionOnTickOnlyOnce && ConditionCheckInterval > 0.0f)
		{
			SB << TEXT("\n(check interval: ") << FString::SanitizeFloat(ConditionCheckInterval);
			if (ConditionCheckIntervalRandomDeviation > 0.0f)
			{
				SB << TEXT("+-") << FString::SanitizeFloat(ConditionCheckIntervalRandomDeviation);
			}
			SB << TEXT("s)");
		}
	}

	return SB.ToString();
}

FString UHTNDecorator::GetTickFunctionIntervalDescription() const
{
	TStringBuilder<2048> SB;

	if (bNotifyTick && TickFunctionInterval > 0.0f)
	{
		SB << TEXT("(tick function interval: ") << FString::SanitizeFloat(TickFunctionInterval);
		if (TickFunctionIntervalRandomDeviation > 0.0f)
		{
			SB << TEXT("+-") << FString::SanitizeFloat(TickFunctionIntervalRandomDeviation);
		}
		SB << TEXT("s)\n");
	}

	return SB.ToString();
}

float UHTNDecorator::GetConditionCheckInterval() const
{
	if (ConditionCheckInterval <= 0.0f || ConditionCheckIntervalRandomDeviation <= 0.0f)
	{
		return ConditionCheckInterval;
	}

	return FMath::FRandRange(
		FMath::Max(0.0f, ConditionCheckInterval - ConditionCheckIntervalRandomDeviation),
		ConditionCheckInterval + ConditionCheckIntervalRandomDeviation);
}

float UHTNDecorator::GetTickFunctionInterval() const
{
	if (TickFunctionInterval <= 0.0f || TickFunctionIntervalRandomDeviation <= 0.0f)
	{
		return TickFunctionInterval;
	}

	return FMath::FRandRange(
		FMath::Max(0.0f, TickFunctionInterval - TickFunctionIntervalRandomDeviation),
		TickFunctionInterval + TickFunctionIntervalRandomDeviation);
}

void UHTNDecorator::SetLastEffectiveConditionValue(uint8* NodeMemory, EHTNDecoratorTestResult Value) const
{
	FHTNDecoratorSpecialMemory& Memory = *GetSpecialNodeMemory<FHTNDecoratorSpecialMemory>(NodeMemory);
	Memory.LastEffectiveConditionValue = Value;
}

bool UHTNDecorator::NotifyEventBasedCondition(UHTNComponent& OwnerComp, uint8* NodeMemory, bool bRawConditionValue, bool bCanAbortPlanInstantly) const
{
	if (!IsValid(this))
	{
		UE_VLOG_UELOG(&OwnerComp, LogHTN, Warning, 
			TEXT("Decorator '%s' called NotifyEventBasedCondition while no longer valid. Ignoring."),
			*GetShortDescription());
		return false;
	}

	UE_VLOG(&OwnerComp, LogHTN, Log, 
		TEXT("Decorator '%s' called NotifyEventBasedCondition (bRawConditionValue=%s, bCanAbortPlanInstantly=%s)"),
		*GetShortDescription(),
		bRawConditionValue ? TEXT("True") : TEXT("False"),
		bCanAbortPlanInstantly ? TEXT("True") : TEXT("False"));

	// Fallback in case user code did not provide NodeMemory.
	// Find NodeMemory belonging to the decorator. 
	// This may return the wrong memory if the decorator is present in the plan multiple times,
	// so providing NodeMemory is preferred
	if (!NodeMemory)
	{
		const FHTNNodeInPlanInfo Info = OwnerComp.FindActiveDecoratorInfo(this);
		NodeMemory = Info.NodeMemory;

		UE_CVLOG(Info.IsValid(), &OwnerComp, LogHTN, Log,
			TEXT("Decorator '%s' called NotifyEventBasedCondition without providing NodeMemory, found NodeMemory in plan instance %s"),
			*GetShortDescription(),
			*Info.PlanInstance->GetLogPrefix());

		UE_CVLOG_UELOG(!Info.IsValid(), &OwnerComp, LogHTN, Error,
			TEXT("Decorator '%s' called NotifyEventBasedCondition without providing NodeMemory, could not find NodeMemory"),
			*GetShortDescription());
	}
	 
	if (NodeMemory)
	{
		const bool bConditionValue = IsInversed() ? !bRawConditionValue : bRawConditionValue;

		// If there are multiple decorators on an If becoming true via event, that needs to be recorded, 
		// so that when in the False branch, the last one to become true actually triggers a replan, since it knows the others are true already.
		SetLastEffectiveConditionValue(NodeMemory, bConditionValue ? EHTNDecoratorTestResult::Passed : EHTNDecoratorTestResult::Failed);

		if (UHTNPlanInstance* const PlanInstance = OwnerComp.FindPlanInstanceByNodeMemory(NodeMemory))
		{
			return PlanInstance->NotifyEventBasedDecoratorCondition(this, NodeMemory, bConditionValue, bCanAbortPlanInstantly);
		}
		else
		{
			UE_VLOG_UELOG(&OwnerComp, LogHTN, Error,
				TEXT("'%s' NotifyEventBasedCondition could not find an active plan instance to which the decorator belongs."),
				*GetShortDescription());
		}
	}

	return false;
}

UHTNDecorator::FHTNDecoratorSpecialMemory::FHTNDecoratorSpecialMemory(float ConditionCheckInterval, float TickFunctionInterval) :
	ConditionCheckCountdown(ConditionCheckInterval),
	TickFunctionCountdown(TickFunctionInterval),
	bFirstTick(true)
{}
