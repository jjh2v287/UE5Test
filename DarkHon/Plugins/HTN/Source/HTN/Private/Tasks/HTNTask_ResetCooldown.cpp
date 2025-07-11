// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "Tasks/HTNTask_ResetCooldown.h"
#include "Decorators/HTNDecorator_Cooldown.h"

#include "Misc/StringBuilder.h"

UHTNTask_ResetCooldown::UHTNTask_ResetCooldown(const FObjectInitializer& Initializer) : Super(Initializer),
	AffectedCooldowns(EHTNResetCooldownAffectedCooldowns::CooldownsWithGameplayTag),
	GameplayTag(FGameplayTag::EmptyTag)
{}

FString UHTNTask_ResetCooldown::GetNodeName() const
{
	if (NodeName.Len())
	{
		return Super::GetNodeName();
	}

	switch (AffectedCooldowns)
	{
	case EHTNResetCooldownAffectedCooldowns::CooldownsWithGameplayTag:
		return FString::Printf(TEXT("Reset Cooldowns By Tag %s"), *GameplayTag.ToString());
	case EHTNResetCooldownAffectedCooldowns::CooldownsWithoutGameplayTag:
		return TEXT("Reset All Cooldowns With No Gameplay Tag");
	case EHTNResetCooldownAffectedCooldowns::AllCooldowns:
		return TEXT("Reset All Cooldowns");
	default:
		checkNoEntry();
		return Super::GetNodeName();
	}
}

FString UHTNTask_ResetCooldown::GetStaticDescription() const
{
	TStringBuilder<2048> StringBuilder;

	StringBuilder << Super::GetStaticDescription() << TEXT(":");

	switch (AffectedCooldowns)
	{
	case EHTNResetCooldownAffectedCooldowns::CooldownsWithGameplayTag:
		StringBuilder << TEXT("\nResets all decorators with tag matching ") << GameplayTag.ToString();
		break;
	case EHTNResetCooldownAffectedCooldowns::CooldownsWithoutGameplayTag:
		StringBuilder << TEXT("\nResets all decorators with no tag");
		break;
	case EHTNResetCooldownAffectedCooldowns::AllCooldowns:
		StringBuilder << TEXT("\nResets all decorators");
		break;
	default:
		checkNoEntry();
		break;
	}

	return *StringBuilder;
}

void UHTNTask_ResetCooldown::CreatePlanSteps(UHTNComponent& OwnerComp, UAITask_MakeHTNPlan& PlanningTask, const TSharedRef<const FBlackboardWorldState>& WorldState) const
{
	PlanningTask.SubmitPlanStep(this, WorldState->MakeNext(), 0);
}

EHTNNodeResult UHTNTask_ResetCooldown::ExecuteTask(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlanStepID& PlanStepID)
{
	if (UHTNExtension_Cooldown* const Extension = OwnerComp.FindExtension<UHTNExtension_Cooldown>())
	{
		switch (AffectedCooldowns)
		{
		case EHTNResetCooldownAffectedCooldowns::CooldownsWithGameplayTag:
			Extension->ResetCooldownsByTag(GameplayTag);
			break;
		case EHTNResetCooldownAffectedCooldowns::CooldownsWithoutGameplayTag:
			Extension->ResetAllCooldownsWithoutGameplayTag();
			break;
		case EHTNResetCooldownAffectedCooldowns::AllCooldowns:
			Extension->ResetAllCooldowns();
			break;
		default:
			checkNoEntry();
			break;
		}
	}

	return EHTNNodeResult::Succeeded;
}
