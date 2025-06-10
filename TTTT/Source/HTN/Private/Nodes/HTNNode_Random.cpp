// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "Nodes/HTNNode_Random.h"
#include "Nodes/HTNNode_RandomWeight.h"
#include "AITask_MakeHTNPlan.h"

#include "Algo/Transform.h"
#include "Misc/StringBuilder.h"
#include "VisualLogger/VisualLogger.h"

UHTNNode_Random::UHTNNode_Random(const FObjectInitializer& Initializer) : Super(Initializer),
	bFallBackToOtherBranchesIfSelectedBranchFailsPlanning(true)
{
	bPlanNextNodesAfterThis = false;
	NodeName = TEXT("Random");
}

FString UHTNNode_Random::GetStaticDescription() const
{
	TStringBuilder<2048> SB;
	SB << Super::GetStaticDescription() << TEXT(": picks random branch");

	if (bFallBackToOtherBranchesIfSelectedBranchFailsPlanning)
	{
		SB << TEXT("\n\nIf selected branch fails planning,\nfalls back to another branch.");
	}

	return SB.ToString();
}

void UHTNNode_Random::MakePlanExpansions(FHTNPlanningContext& Context)
{
	const auto MakePlanStep = [&](int32 SelectedNodeIndex)
	{
		FHTNPlanStep* AddedStep = nullptr;
		FHTNPlanStepID AddedStepID;
		const TSharedRef<FHTNPlan> NewPlan = Context.MakePlanCopyWithAddedStep(AddedStep, AddedStepID);
		AddedStep->SubLevelIndex = Context.AddInlineLevel(*NewPlan, AddedStepID);

		// This is how we decide which node each submitted plan will use.
		// This value is used in GetNextNodes to decide which node comes next after this one during planning.
		*reinterpret_cast<int32*>(&AddedStep->CustomData) = SelectedNodeIndex;

		FString Description;
		UE_IFVLOG(Description = FString::Printf(TEXT("%d: %s"), SelectedNodeIndex, *NextNodes[SelectedNodeIndex]->GetShortDescription()));
		Context.SubmitCandidatePlan(NewPlan, Description, /*bOnlyUseIfPreviousFails=*/bFallBackToOtherBranchesIfSelectedBranchFailsPlanning);
	};

	// If we're merely adjusting an existing plan, we don't pick a random option but stick to the one we picked for the ongoing plan.
	if (const FHTNPlanStep* const StepToReplace = Context.GetExistingPlanStepToAdjust())
	{
		const int32 SelectedStepID = *reinterpret_cast<const int32*>(&StepToReplace->CustomData);
		MakePlanStep(SelectedStepID);
		return;
	}

	struct FShuffleOption
	{
		int32 NodeIndex;
		float RandomSortingValue;
	};

	// Random shuffle the next nodes in a weighted way. 
	TArray<FShuffleOption, TInlineAllocator<16>> Options;
	for (int32 NextNodesIndex = 0; NextNodesIndex < NextNodes.Num(); ++NextNodesIndex)
	{
		const UHTNNode* const Node = NextNodes[NextNodesIndex];
		if (IsValid(Node))
		{
			float RandomWeight = 1.0f;
			if (const UHTNNode_RandomWeight* const RandomWeightNode = Cast<UHTNNode_RandomWeight>(Node))
			{
				RandomWeight = RandomWeightNode->RandomWeight;
			}

			const float RandomSortingValue = RandomWeight > 0.0f ? -FMath::Pow(FMath::FRand(), 1.0f / RandomWeight) : 0.0f;
			Options.Add({ NextNodesIndex, RandomSortingValue });
		}
	}
	Algo::SortBy(Options, &FShuffleOption::RandomSortingValue);

	// Make a separate plan expansion for each of the shuffled options.
	// Do it such that subsequent options can only be tried if the previous options failed.
	for (const FShuffleOption& Option : Options)
	{
		MakePlanStep(Option.NodeIndex);

		// If fallbacks aren't allowed, stop after submitting one candidate plan.
		if (!bFallBackToOtherBranchesIfSelectedBranchFailsPlanning)
		{
			break;
		}
	}
}

void UHTNNode_Random::GetNextNodes(FHTNNextNodesBuffer& OutNextNodes, const FHTNPlan& Plan, const FHTNPlanStepID& ThisStepID, int32 SubLevelIndex)
{
	if (const FHTNPlanStep* const Step = Plan.FindStep(ThisStepID))
	{
		ensure(Step->Node == this);
		const int32 SelectedNextNodeIndex = *reinterpret_cast<const int32*>(&Step->CustomData);
		if (NextNodes.IsValidIndex(SelectedNextNodeIndex))
		{
			OutNextNodes.Add(NextNodes[SelectedNextNodeIndex]);
		}
	}
}

void UHTNNode_Random::GetNextPrimitiveSteps(FHTNGetNextStepsContext& Context, const FHTNPlanStepID& ThisStepID)
{
	const FHTNPlanStep& Step = Context.Plan.GetStep(ThisStepID);
	if (Step.SubLevelIndex != INDEX_NONE)
	{
		Context.AddFirstPrimitiveStepsInLevel(Step.SubLevelIndex);
	}
}
