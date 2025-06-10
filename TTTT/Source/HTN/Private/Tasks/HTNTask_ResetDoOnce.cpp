// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "Tasks/HTNTask_ResetDoOnce.h"
#include "Decorators/HTNDecorator_DoOnce.h"

#include "Misc/StringBuilder.h"

UHTNTask_ResetDoOnce::UHTNTask_ResetDoOnce(const FObjectInitializer& Initializer) : Super(Initializer),
	AffectedDecorators(EHTNResetDoOnceAffectedDecorators::DoOnceDecoratorsWithGameplayTag),
	GameplayTag(FGameplayTag::EmptyTag)
{}

FString UHTNTask_ResetDoOnce::GetNodeName() const
{
	if (NodeName.Len())
	{
		return Super::GetNodeName();
	}

	switch (AffectedDecorators)
	{
	case EHTNResetDoOnceAffectedDecorators::DoOnceDecoratorsWithGameplayTag:
		return FString::Printf(TEXT("Reset Do Once By Tag %s"), *GameplayTag.ToString());
	case EHTNResetDoOnceAffectedDecorators::DoOnceDecoratorsWithoutGameplayTag:
		return TEXT("Reset All Do Once Decorators With No Gameplay Tag");
	case EHTNResetDoOnceAffectedDecorators::AllDoOnceDecorators:
		return TEXT("Reset All Do Once Decorators");
	default:
		checkNoEntry();
		return Super::GetNodeName();
	}
}

FString UHTNTask_ResetDoOnce::GetStaticDescription() const
{
	TStringBuilder<2048> StringBuilder;

	StringBuilder << Super::GetStaticDescription() << TEXT(":");

	switch (AffectedDecorators)
	{
	case EHTNResetDoOnceAffectedDecorators::DoOnceDecoratorsWithGameplayTag:
		StringBuilder << TEXT("\nResets all decorators with tag matching ") << GameplayTag.ToString();
		break;
	case EHTNResetDoOnceAffectedDecorators::DoOnceDecoratorsWithoutGameplayTag:
		StringBuilder << TEXT("\nResets all decorators with no tag");
		break;
	case EHTNResetDoOnceAffectedDecorators::AllDoOnceDecorators:
		StringBuilder << TEXT("\nResets all decorators");
		break;
	default:
		checkNoEntry();
		break;
	}

	return *StringBuilder;
}

void UHTNTask_ResetDoOnce::CreatePlanSteps(UHTNComponent& OwnerComp, UAITask_MakeHTNPlan& PlanningTask, const TSharedRef<const FBlackboardWorldState>& WorldState) const
{
	PlanningTask.SubmitPlanStep(this, WorldState->MakeNext(), 0);
}

EHTNNodeResult UHTNTask_ResetDoOnce::ExecuteTask(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlanStepID& PlanStepID)
{
	if (UHTNExtension_DoOnce* const Extension = OwnerComp.FindExtension<UHTNExtension_DoOnce>())
	{
		switch (AffectedDecorators)
		{
		case EHTNResetDoOnceAffectedDecorators::DoOnceDecoratorsWithGameplayTag:
			Extension->SetDoOnceLocked(GameplayTag, /*bNewLocked=*/false);
			break;
		case EHTNResetDoOnceAffectedDecorators::DoOnceDecoratorsWithoutGameplayTag:
			Extension->ResetAllDoOnceDecoratorsWithoutGameplayTag();
			break;
		case EHTNResetDoOnceAffectedDecorators::AllDoOnceDecorators:
			Extension->ResetAllDoOnceDecorators();
			break;
		default:
			checkNoEntry();
			break;
		}
	}

	return EHTNNodeResult::Succeeded;
}
