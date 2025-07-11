// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "Nodes/HTNNode_If.h"
#include "AITask_MakeHTNPlan.h"
#include "HTNDecorator.h"
#include "Misc/StringBuilder.h"
#include "VisualLogger/VisualLogger.h"

UHTNNode_If::UHTNNode_If(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer),
	bCanConditionsInterruptTrueBranch(true),
	bCanConditionsInterruptFalseBranch(true),
	PlanAdjustmentMode(EHTNNodeIfPlanAdjustmentMode::NoAdjustmentAllowed)
{
	bAllowFailingDecoratorsOnNodeDuringPlanning = true;
}

void UHTNNode_If::MakePlanExpansions(FHTNPlanningContext& Context)
{
	const auto MakePlanStep = [&](bool bTrueBranch, bool bIsDifferentBranchFromCurrentPlan = false)
	{
		FHTNPlanStep* AddedStep = nullptr;
		FHTNPlanStepID AddedStepID;
		const TSharedRef<FHTNPlan> NewPlan = Context.MakePlanCopyWithAddedStep(AddedStep, AddedStepID);
		NewPlan->bDivergesFromCurrentPlanDuringPlanAdjustment |= bIsDifferentBranchFromCurrentPlan;
		AddedStep->bIsPotentialDivergencePointDuringPlanAdjustment = IsPotentialDivergencePoint(bTrueBranch);

		AddedStep->bIsIfNodeFalseBranch = !bTrueBranch;
		AddedStep->bCanConditionsInterruptTrueBranch = bCanConditionsInterruptTrueBranch;
		AddedStep->bCanConditionsInterruptFalseBranch = bCanConditionsInterruptFalseBranch;

		if (bTrueBranch)
		{
			AddedStep->SubLevelIndex = AddInlinePrimaryLevel(Context, *NewPlan, AddedStepID);
		}
		else
		{
			AddedStep->SecondarySubLevelIndex = AddInlineSecondaryLevel(Context, *NewPlan, AddedStepID);
		}

		static const FString TrueBranchDescription = TEXT("true branch");
		static const FString FalseBranchDescription = TEXT("false branch");
		static const FString TrueBranchDescriptionAdjusted = TEXT("true branch: adjustment from current plan");
		static const FString FalseBranchDescriptionAdjusted = TEXT("false branch: adjustment from current plan");
		FString Description;
		UE_IFVLOG(Description = bIsDifferentBranchFromCurrentPlan ?
			(bTrueBranch ? TrueBranchDescriptionAdjusted : FalseBranchDescriptionAdjusted) :
			(bTrueBranch ? TrueBranchDescription : FalseBranchDescription));
		Context.SubmitCandidatePlan(NewPlan, Description);
	};

	const bool bIsTrueBranch = Context.bDecoratorsPassed;
	if (const FHTNPlanStep* const StepToReplace = Context.GetExistingPlanStepToAdjust())
	{
		const bool bWasTrueBranch = !StepToReplace->bIsIfNodeFalseBranch;
		const bool bSwitchingFromFalseToTrue = !bWasTrueBranch && bIsTrueBranch;
		const bool bSwitchingFromTrueToFalse = bWasTrueBranch && !bIsTrueBranch;
		switch (PlanAdjustmentMode)
		{
			case EHTNNodeIfPlanAdjustmentMode::NoAdjustmentAllowed:
			{
				if (bIsTrueBranch == bWasTrueBranch)
				{
					MakePlanStep(bIsTrueBranch);
				}
				break;
			}
			case EHTNNodeIfPlanAdjustmentMode::TrySwitchToTrueBranch:
			{
				if (!bSwitchingFromTrueToFalse)
				{
					MakePlanStep(bIsTrueBranch, bSwitchingFromFalseToTrue);
				}
				break;
			}
			case EHTNNodeIfPlanAdjustmentMode::TrySwitchToFalseBranch:
			{
				if (!bSwitchingFromFalseToTrue)
				{
					MakePlanStep(bIsTrueBranch, bSwitchingFromTrueToFalse);
				}
				break;
			}
			case EHTNNodeIfPlanAdjustmentMode::TrySwitchToOtherBranch:
			{
				MakePlanStep(bIsTrueBranch, bWasTrueBranch != bIsTrueBranch);
				break;
			}
			default:
			{
				checkNoEntry();
			}
		}
	}
	else
	{
		MakePlanStep(bIsTrueBranch);
	}
}

void UHTNNode_If::GetNextPrimitiveSteps(FHTNGetNextStepsContext& Context, const FHTNPlanStepID& ThisStepID)
{
	const FHTNPlanStep& Step = Context.Plan.GetStep(ThisStepID);

	if (Step.SubLevelIndex != INDEX_NONE)
	{
		Context.AddFirstPrimitiveStepsInLevel(Step.SubLevelIndex);
	}
	else if (Step.SecondarySubLevelIndex != INDEX_NONE)
	{
		Context.AddFirstPrimitiveStepsInLevel(Step.SecondarySubLevelIndex);
	}
}

FString UHTNNode_If::GetNodeName() const
{
	if (NodeName.Len())
	{
		return Super::GetNodeName();
	}

	TStringBuilder<2048> SB;
	SB << TEXT("If");

	int32 I = 0;
	for (UHTNDecorator* const Decorator : Decorators)
	{
		if (IsValid(Decorator))
		{
			SB << FStringView(I++ == 0 ? TEXT(" ") : TEXT(", "));
			SB << Decorator->GetNodeName();
		}
	}

	return *SB;
}

FString UHTNNode_If::GetStaticDescription() const
{
	TStringBuilder<2048> SB;

	SB << Super::GetStaticDescription();

	if (!bCanConditionsInterruptTrueBranch)
	{
		SB << TEXT("\n(decorators won't interrupt true branch)");
	}

	if (!bCanConditionsInterruptFalseBranch)
	{
		SB << TEXT("\n(decorators won't interrupt false branch)");
	}

	if (PlanAdjustmentMode != EHTNNodeIfPlanAdjustmentMode::NoAdjustmentAllowed)
	{
		SB << TEXT("\nPlan adjustment mode:\n") << UEnum::GetDisplayValueAsText(PlanAdjustmentMode).ToString();
	}

	return *SB;
}

bool UHTNNode_If::IsPotentialDivergencePoint(bool bIsTrueBranch) const
{
	switch (PlanAdjustmentMode)
	{
	case EHTNNodeIfPlanAdjustmentMode::NoAdjustmentAllowed:
		return false;
	case EHTNNodeIfPlanAdjustmentMode::TrySwitchToTrueBranch:
		return !bIsTrueBranch;
	case EHTNNodeIfPlanAdjustmentMode::TrySwitchToFalseBranch:
		return bIsTrueBranch;
	case EHTNNodeIfPlanAdjustmentMode::TrySwitchToOtherBranch:
		return true;
	default:
		checkNoEntry();
	}

	return false;
}
