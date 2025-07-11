// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "Nodes/HTNNode_Optional.h"
#include "AITask_MakeHTNPlan.h"
#include "Misc/StringBuilder.h"
#include "VisualLogger/VisualLogger.h"

UHTNNode_Optional::UHTNNode_Optional(const FObjectInitializer& Initializer) : Super(Initializer),
	PlanAdjustmentMode(EHTNNodeOptionalPlanAdjustmentMode::NoAdjustmentAllowed)
{
	bPlanNextNodesAfterThis = false;
}

void UHTNNode_Optional::MakePlanExpansions(FHTNPlanningContext& Context)
{
	const auto MakePlanStep = [&](bool bWithContent, bool bIsDifferentFromStepToReplace = false)
	{
		FHTNPlanStep* AddedStep = nullptr;
		FHTNPlanStepID AddedStepID;
		const TSharedRef<FHTNPlan> NewPlan = Context.MakePlanCopyWithAddedStep(AddedStep, AddedStepID);
		NewPlan->bDivergesFromCurrentPlanDuringPlanAdjustment |= bIsDifferentFromStepToReplace;
		AddedStep->bIsPotentialDivergencePointDuringPlanAdjustment = IsPotentialDivergencePoint(bWithContent);
		
		// We store the index of the branch we're storing so that we can reliably tell 
		// which branch was picked even if both branches are empty and thus both SubLevelIndex and SecondarySubLevelIndex are INDEX_NONE.
		// That is useful during TryToAdjustExistingPlan replanning: see StepToReplace below.
		AddedStep->CustomData = bWithContent ? 1 : 0;
		if (bWithContent)
		{
			AddedStep->SubLevelIndex = NextNodes.Num() ? Context.AddInlineLevel(*NewPlan, AddedStepID) : INDEX_NONE;
		}

		static const FString WithContentDescription = TEXT("with content");
		static const FString WithoutContentDescription = TEXT("without content");
		static const FString WithContentDescriptionAdjusted = TEXT("with content: adjustment from current plan");
		static const FString WithoutContentDescriptionAdjusted = TEXT("without content: adjustment from current plan");
		FString Description;
		UE_IFVLOG(Description = bIsDifferentFromStepToReplace ?
			(bWithContent ? WithContentDescriptionAdjusted : WithoutContentDescriptionAdjusted) :
			(bWithContent ? WithContentDescription : WithoutContentDescription));
		Context.SubmitCandidatePlan(NewPlan, bWithContent ? TEXT("with content") : TEXT("empty"), /*bOnlyUseIfPreviousFails=*/true);
	};

	// If we're doing a "try switch branches" type of planning, 
	// we need to compare the new planning against the plan we're trying to replace.
	if (const FHTNPlanStep* const StepToReplace = Context.GetExistingPlanStepToAdjust())
	{
		const bool bWasWithContent = StepToReplace->CustomData > 0;
		switch (PlanAdjustmentMode)
		{
		case EHTNNodeOptionalPlanAdjustmentMode::NoAdjustmentAllowed:
		{
			MakePlanStep(bWasWithContent);
			break;
		}
		case EHTNNodeOptionalPlanAdjustmentMode::TrySwitchToWithContent:
		{
			if (bWasWithContent)
			{
				MakePlanStep(true);
			}
			else
			{
				MakePlanStep(true, true);
				MakePlanStep(false);
			}
			break;
		}
		case EHTNNodeOptionalPlanAdjustmentMode::TrySwitchToWithoutContent:
		{
			if (bWasWithContent)
			{
				MakePlanStep(true);
				MakePlanStep(false, true);
			}
			else
			{
				MakePlanStep(false);
			}
			break;
		}
		case EHTNNodeOptionalPlanAdjustmentMode::TrySwitchToOther:
		{
			if (bWasWithContent)
			{
				MakePlanStep(true);
				MakePlanStep(false, true);
			}
			else
			{
				MakePlanStep(true, true);
				MakePlanStep(false);
			}
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
		MakePlanStep(true);
		MakePlanStep(false);
	}
}

void UHTNNode_Optional::GetNextPrimitiveSteps(FHTNGetNextStepsContext& Context, const FHTNPlanStepID& ThisStepID)
{
	const FHTNPlanStep& Step = Context.Plan.GetStep(ThisStepID);
	if (Step.SubLevelIndex != INDEX_NONE)
	{
		Context.AddFirstPrimitiveStepsInLevel(Step.SubLevelIndex);
	}
}

FString UHTNNode_Optional::GetStaticDescription() const
{
	TStringBuilder<2048> SB;

	SB << Super::GetStaticDescription();

	if (PlanAdjustmentMode != EHTNNodeOptionalPlanAdjustmentMode::NoAdjustmentAllowed)
	{
		SB << TEXT("\nPlan adjustment mode:\n") << UEnum::GetDisplayValueAsText(PlanAdjustmentMode).ToString();
	}

	return SB.ToString();
}

bool UHTNNode_Optional::IsPotentialDivergencePoint(bool bWithContent) const
{
	switch (PlanAdjustmentMode)
	{
	case EHTNNodeOptionalPlanAdjustmentMode::NoAdjustmentAllowed:
		return false;
	case EHTNNodeOptionalPlanAdjustmentMode::TrySwitchToWithContent:
		return !bWithContent;
	case EHTNNodeOptionalPlanAdjustmentMode::TrySwitchToWithoutContent:
		return bWithContent;
	case EHTNNodeOptionalPlanAdjustmentMode::TrySwitchToOther:
		return true;
	default:
		checkNoEntry();
	}

	return false;
}
