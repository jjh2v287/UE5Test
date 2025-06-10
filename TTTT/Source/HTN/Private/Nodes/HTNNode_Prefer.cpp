// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "Nodes/HTNNode_Prefer.h"
#include "AITask_MakeHTNPlan.h"
#include "Misc/StringBuilder.h"
#include "VisualLogger/VisualLogger.h"

UHTNNode_Prefer::UHTNNode_Prefer(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer), 
	PlanAdjustmentMode(EHTNNodePreferPlanAdjustmentMode::NoAdjustmentAllowed)
{}

void UHTNNode_Prefer::MakePlanExpansions(FHTNPlanningContext& Context)
{
	const auto MakePlanStep = [&](bool bTopBranch, bool bIsDifferentFromStepToReplace = false)
	{
		FHTNPlanStep* AddedStep = nullptr;
		FHTNPlanStepID AddedStepID;
		const TSharedRef<FHTNPlan> NewPlan = Context.MakePlanCopyWithAddedStep(AddedStep, AddedStepID);
		NewPlan->bDivergesFromCurrentPlanDuringPlanAdjustment |= bIsDifferentFromStepToReplace;
		AddedStep->bIsPotentialDivergencePointDuringPlanAdjustment = IsPotentialDivergencePoint(bTopBranch);

		// We store the index of the branch we're storing so that we can reliably tell 
		// which branch was picked even if both branches are empty and thus both SubLevelIndex and SecondarySubLevelIndex are INDEX_NONE.
		// That is useful during TryToAdjustExistingPlan replanning: see StepToReplace below.
		AddedStep->CustomData = bTopBranch ? 1 : 0;
		if (bTopBranch)
		{
			AddedStep->SubLevelIndex = AddInlinePrimaryLevel(Context, *NewPlan, AddedStepID);
		}
		else
		{
			AddedStep->SecondarySubLevelIndex = AddInlineSecondaryLevel(Context, *NewPlan, AddedStepID);
		}

		static const FString TopBranchDescription = TEXT("top branch");
		static const FString BottomBranchDescription = TEXT("bottom branch");
		static const FString TopBranchDescriptionAdjusted = TEXT("top branch: adjustment from current plan");
		static const FString BottomBranchDescriptionAdjusted = TEXT("bottom branch: adjustment from current plan");
		FString Description;
		UE_IFVLOG(Description = bIsDifferentFromStepToReplace ? 
			(bTopBranch ? TopBranchDescriptionAdjusted : BottomBranchDescriptionAdjusted) :
			(bTopBranch ? TopBranchDescription : BottomBranchDescription));
		Context.SubmitCandidatePlan(NewPlan, Description, /*bOnlyUseIfPreviousFails=*/true);
	};

	// If we're doing a "try switch branches" type of planning, 
	// we need to compare the new planning against the plan we're trying to replace.
	if (const FHTNPlanStep* const StepToReplace = Context.GetExistingPlanStepToAdjust())
	{
		const bool bWasTopBranch = StepToReplace->CustomData > 0;
		switch (PlanAdjustmentMode)
		{
			case EHTNNodePreferPlanAdjustmentMode::NoAdjustmentAllowed:
			{
				MakePlanStep(bWasTopBranch);
				break;
			}
			case EHTNNodePreferPlanAdjustmentMode::TrySwitchToHigherPriorityBranch:
			{
				if (bWasTopBranch)
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
			case EHTNNodePreferPlanAdjustmentMode::TrySwitchToLowerPriorityBranch:
			{
				if (bWasTopBranch)
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
			case EHTNNodePreferPlanAdjustmentMode::TrySwitchToOtherBranch:
			{
				if (bWasTopBranch)
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

void UHTNNode_Prefer::GetNextPrimitiveSteps(FHTNGetNextStepsContext& Context, const FHTNPlanStepID& ThisStepID)
{
	Context.AddFirstPrimitiveStepsInAnySublevelOf(ThisStepID);
}

FString UHTNNode_Prefer::GetStaticDescription() const
{
	TStringBuilder<2048> SB;

	SB << Super::GetStaticDescription();
	
	if (PlanAdjustmentMode != EHTNNodePreferPlanAdjustmentMode::NoAdjustmentAllowed)
	{
		SB << TEXT("\nPlan adjustment mode:\n") << UEnum::GetDisplayValueAsText(PlanAdjustmentMode).ToString();
	}

	return SB.ToString();
}

bool UHTNNode_Prefer::IsPotentialDivergencePoint(bool bIsTopBranch) const
{
	switch (PlanAdjustmentMode)
	{
		case EHTNNodePreferPlanAdjustmentMode::NoAdjustmentAllowed:
			return false;
		case EHTNNodePreferPlanAdjustmentMode::TrySwitchToHigherPriorityBranch:
			return !bIsTopBranch;
		case EHTNNodePreferPlanAdjustmentMode::TrySwitchToLowerPriorityBranch:
			return bIsTopBranch;
		case EHTNNodePreferPlanAdjustmentMode::TrySwitchToOtherBranch:
			return true;
		default:
			checkNoEntry();
	}

	return false;
}
