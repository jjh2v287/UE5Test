// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "Decorators/HTNDecorator_BlueprintBase.h"

#include "HTNBlueprintLibrary.h"
#include "Utility/HTNHelpers.h"
#include "WorldStateProxy.h"

#include "AIController.h"
#include "BlueprintNodeHelpers.h"
#include "VisualLogger/VisualLogger.h"

UHTNDecorator_BlueprintBase::UHTNDecorator_BlueprintBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer),
	bShowPropertyDetails(true),
	CachedNodeMemory(nullptr)
{
#define IS_IMPLEMENTED(FunctionName) \
	(BlueprintNodeHelpers::HasBlueprintFunction(GET_FUNCTION_NAME_CHECKED(ThisClass, FunctionName), *this, *StaticClass()))
	bImplementsPerformConditionCheck = IS_IMPLEMENTED(PerformConditionCheck);
	bImplementsModifyStepCost = IS_IMPLEMENTED(ReceiveModifyStepCost);
	bImplementsOnPlanEnter = IS_IMPLEMENTED(ReceiveOnPlanEnter);
	bImplementsOnPlanExit = IS_IMPLEMENTED(ReceiveOnPlanExit);
	bImplementsOnExecutionStart = IS_IMPLEMENTED(ReceiveExecutionStart);
	bImplementsTick = IS_IMPLEMENTED(ReceiveTick);
	bImplementsOnExecutionFinish = IS_IMPLEMENTED(ReceiveExecutionFinish);
	bImplementsOnPlanExecutionStarted = IS_IMPLEMENTED(ReceiveOnPlanExecutionStarted);
	bImplementsOnPlanExecutionFinished = IS_IMPLEMENTED(ReceiveOnPlanExecutionFinished);
#undef IS_IMPLEMENTED
	
	bNotifyOnEnterPlan = bImplementsOnPlanEnter;
	bNotifyOnExitPlan = bImplementsOnPlanExit;
	bNotifyExecutionStart = bImplementsOnExecutionStart;
	bNotifyTick = bImplementsTick;
	// We need OnExecutionFinish to be called even if ReceiveExecutionFinish is not implemented in Blueprints,
	// so that we can abort latent actions (e.g. Delay nodes) and timers.
	bNotifyExecutionFinish = true;
	bNotifyOnPlanExecutionStarted = bImplementsOnPlanExecutionStarted;
	// Likewise for OnPlanExecutionFinished.
	bNotifyOnPlanExecutionFinished = true;
	bModifyStepCost = bImplementsModifyStepCost;

	// All blueprint-based nodes must create instances
	bCreateNodeInstance = true;
	// Since tasks created from blueprints won't set this to true on their own.
	bOwnsGameplayTasks = true;
	
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		BlueprintNodeHelpers::CollectPropertyData(this, StaticClass(), PropertyData);
	}
}

void UHTNDecorator_BlueprintBase::InitializeFromAsset(UHTN& Asset)
{
	Super::InitializeFromAsset(Asset);

	FHTNHelpers::ResolveBlackboardKeySelectors(*this, *StaticClass(), Asset.BlackboardAsset);
}

FString UHTNDecorator_BlueprintBase::GetStaticDescription() const
{
	FString Description = Super::GetStaticDescription();

	if (bShowPropertyDetails)
	{
		if (UHTNDecorator_BlueprintBase* const CDO = GetClass()->GetDefaultObject<UHTNDecorator_BlueprintBase>())
		{
			const FString PropertyDescription = FHTNHelpers::CollectPropertyDescription(this, StaticClass(), CDO->PropertyData);
			if (PropertyDescription.Len())
			{
				Description += TEXT(":\n\n");
				Description += PropertyDescription;
			}
		}
	}

	return Description;
}

void UHTNDecorator_BlueprintBase::InitializeMemory(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlan& Plan, const FHTNPlanStepID& StepID) const
{
	CachedNodeMemory = NodeMemory;
}

void UHTNDecorator_BlueprintBase::CleanupMemory(UHTNComponent& OwnerComp, uint8* NodeMemory) const
{
	CachedNodeMemory = nullptr;
}

bool UHTNDecorator_BlueprintBase::CalculateRawConditionValue(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNDecoratorConditionCheckType CheckType) const
{
	if (!bImplementsPerformConditionCheck)
	{
		return Super::CalculateRawConditionValue(OwnerComp, NodeMemory, CheckType);
	}

	FGuardValue_Bitfield(bForceUsingPlanningWorldState, CheckType == EHTNDecoratorConditionCheckType::PlanRecheck ? true : bForceUsingPlanningWorldState);
	const FGuardOwnerComponent OwnerGuard(*this, &OwnerComp);
	check(UHTNNodeLibrary::GetOwnersWorldState(this) == GetWorldStateProxy(OwnerComp, CheckType));
	check(GetWorldStateProxy(OwnerComp, CheckType)->IsBlackboard() == (CheckType == EHTNDecoratorConditionCheckType::Execution));
	return PerformConditionCheck(
		OwnerComp.GetOwner(),
		OwnerComp.GetAIOwner(), 
		OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr,
		CheckType);
}

void UHTNDecorator_BlueprintBase::ModifyStepCost(UHTNComponent& OwnerComp, FHTNPlanStep& Step) const
{
	if (!bImplementsModifyStepCost)
	{
		Super::ModifyStepCost(OwnerComp, Step);
		return;
	}

	const FGuardOwnerComponent OwnerGuard(*this, &OwnerComp);
	check(UHTNNodeLibrary::GetOwnersWorldState(this) == OwnerComp.GetPlanningWorldStateProxy());
	Step.Cost = ReceiveModifyStepCost(Step.Cost,
		OwnerComp.GetOwner(), 
		OwnerComp.GetAIOwner(), 
		OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr);

	if (Step.Cost < 0)
	{
		UE_VLOG_UELOG(OwnerComp.GetOwner(), LogHTN, Warning, TEXT("Plan step modified by %s is %i. Negative costs aren't allowed, resetting to zero."), 
			*GetShortDescription(), Step.Cost);
		Step.Cost = 0;
	}
}

void UHTNDecorator_BlueprintBase::OnEnterPlan(UHTNComponent& OwnerComp, const FHTNPlan& Plan, const FHTNPlanStepID& StepID) const
{
	if (!bImplementsOnPlanEnter)
	{
		return;
	}
	
	const FGuardOwnerComponent OwnerGuard(*this, &OwnerComp);
	check(UHTNNodeLibrary::GetOwnersWorldState(this) == OwnerComp.GetPlanningWorldStateProxy());
	check(UHTNNodeLibrary::GetOwnersWorldState(this)->IsEditable());
	ReceiveOnPlanEnter(OwnerComp.GetOwner(), OwnerComp.GetAIOwner(), OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr);
}

void UHTNDecorator_BlueprintBase::OnExitPlan(UHTNComponent& OwnerComp, const FHTNPlan& Plan, const FHTNPlanStepID& StepID) const
{
	if (!bImplementsOnPlanExit)
	{
		return;
	}

	const FGuardOwnerComponent OwnerGuard(*this, &OwnerComp);
	check(UHTNNodeLibrary::GetOwnersWorldState(this) == OwnerComp.GetPlanningWorldStateProxy());
	check(UHTNNodeLibrary::GetOwnersWorldState(this)->IsEditable());
	ReceiveOnPlanExit(OwnerComp.GetOwner(), OwnerComp.GetAIOwner(), OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr);
}

void UHTNDecorator_BlueprintBase::OnExecutionStart(UHTNComponent& OwnerComp, uint8* NodeMemory)
{	
	if (!bImplementsOnExecutionStart)
	{
		return;
	}

	check(GetOwnerComponent() == &OwnerComp);
	check(UHTNNodeLibrary::GetOwnersWorldState(this) == OwnerComp.GetBlackboardProxy());
	check(UHTNNodeLibrary::GetOwnersWorldState(this)->IsBlackboard());
	check(UHTNNodeLibrary::GetOwnersWorldState(this)->IsEditable());
	ReceiveExecutionStart(OwnerComp.GetOwner(), OwnerComp.GetAIOwner(), OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr);
}

void UHTNDecorator_BlueprintBase::TickNode(UHTNComponent& OwnerComp, uint8* NodeMemory, float DeltaTime)
{
	if (!bImplementsTick)
	{
		return;
	}

	check(GetOwnerComponent() == &OwnerComp);
	check(UHTNNodeLibrary::GetOwnersWorldState(this) == OwnerComp.GetBlackboardProxy());
	check(UHTNNodeLibrary::GetOwnersWorldState(this)->IsBlackboard());
	check(UHTNNodeLibrary::GetOwnersWorldState(this)->IsEditable());
	ReceiveTick(OwnerComp.GetOwner(), OwnerComp.GetAIOwner(), OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr, DeltaTime);
}

void UHTNDecorator_BlueprintBase::OnExecutionFinish(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNNodeResult Result)
{
	BlueprintNodeHelpers::AbortLatentActions(OwnerComp, *this);
	
	if (!bImplementsOnExecutionFinish)
	{
		return;
	}
	
	check(GetOwnerComponent() == &OwnerComp);
	check(UHTNNodeLibrary::GetOwnersWorldState(this) == OwnerComp.GetBlackboardProxy());
	check(UHTNNodeLibrary::GetOwnersWorldState(this)->IsBlackboard());
	check(UHTNNodeLibrary::GetOwnersWorldState(this)->IsEditable());
	ReceiveExecutionFinish(OwnerComp.GetOwner(), OwnerComp.GetAIOwner(), OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr, Result);
}

void UHTNDecorator_BlueprintBase::OnPlanExecutionStarted(UHTNComponent& OwnerComp, uint8* NodeMemory)
{
	if (!bImplementsOnPlanExecutionStarted)
	{
		return;
	}

	FGuardValue_Bitfield(bForceUsingPlanningWorldState, true);
	check(GetOwnerComponent() == &OwnerComp);
	check(UHTNNodeLibrary::GetOwnersWorldState(this) == OwnerComp.GetPlanningWorldStateProxy());
	check(!UHTNNodeLibrary::GetOwnersWorldState(this)->IsBlackboard());
	check(!UHTNNodeLibrary::GetOwnersWorldState(this)->IsEditable());
	ReceiveOnPlanExecutionStarted(OwnerComp.GetOwner(), OwnerComp.GetAIOwner(), OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr);
}

void UHTNDecorator_BlueprintBase::OnPlanExecutionFinished(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNPlanExecutionFinishedResult Result)
{
	// Ths handles the case where we started a latent action (e.g., a delay node) in OnPlanExecutionStarted, 
	// but then the plan got aborted before the node even activated, so OnExecutionStart never got called and
	// therefore OnExecutionFinish didn't either.
	BlueprintNodeHelpers::AbortLatentActions(OwnerComp, *this);

	if (!bImplementsOnPlanExecutionFinished)
	{
		return;
	}

	FGuardValue_Bitfield(bForceUsingPlanningWorldState, true);
	check(GetOwnerComponent() == &OwnerComp);
	check(UHTNNodeLibrary::GetOwnersWorldState(this) == OwnerComp.GetPlanningWorldStateProxy());
	check(!UHTNNodeLibrary::GetOwnersWorldState(this)->IsBlackboard());
	check(!UHTNNodeLibrary::GetOwnersWorldState(this)->IsEditable());
	ReceiveOnPlanExecutionFinished(
		OwnerComp.GetOwner(), OwnerComp.GetAIOwner(), OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr,
		Result);
}

void UHTNDecorator_BlueprintBase::BP_Replan(const FHTNReplanParameters& Params) const
{
	if (UHTNComponent* const HTNComponent = GetTypedOuter<UHTNComponent>())
	{
		if (CachedNodeMemory)
		{
			return Replan(*HTNComponent, CachedNodeMemory, Params);
		}
		else
		{
			UE_VLOG_UELOG(HTNComponent, LogHTN, Error,
				TEXT("Decorator '%s' called Replan while not initialized for execution."),
				*GetShortDescription());
		}
	}
	else
	{
		UE_VLOG_UELOG(this, LogHTN, Warning,
			TEXT("Decorator '%s': Replan could not find the owning HTNComponent. Was it called during planning?"),
			*GetShortDescription());
	}
}

bool UHTNDecorator_BlueprintBase::BP_NotifyEventBasedCondition(bool bRawConditionValue, bool bCanAbortPlanInstantly)
{
	if (UHTNComponent* const HTNComponent = GetTypedOuter<UHTNComponent>())
	{
		if (CachedNodeMemory)
		{
			return NotifyEventBasedCondition(*HTNComponent, CachedNodeMemory, bRawConditionValue, bCanAbortPlanInstantly);
		}
		else
		{
			UE_VLOG_UELOG(HTNComponent, LogHTN, Error,
				TEXT("Decorator '%s' called NotifyEventBasedCondition while not initialized for execution."), 
				*GetShortDescription());
		}
	}
	else
	{
		UE_VLOG_UELOG(this, LogHTN, Warning,
			TEXT("Decorator '%s': NotifyEventBasedCondition could not find the owning HTNComponent. Was it called during planning?"),
			*GetShortDescription());
	}

	return false;
}
