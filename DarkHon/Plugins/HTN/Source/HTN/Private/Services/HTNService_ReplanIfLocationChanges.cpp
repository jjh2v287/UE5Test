// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "Services/HTNService_ReplanIfLocationChanges.h"
#include "HTNObjectVersion.h"
#include "HTNPlanInstance.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "VisualLogger/VisualLogger.h"

UHTNService_ReplanIfLocationChanges::UHTNService_ReplanIfLocationChanges(const FObjectInitializer& Initializer) : Super(Initializer),
	Tolerance(100.0f)
{
	NodeName = TEXT("Replan If Location Changes");
	
	bNotifyExecutionStart = true;
	bNotifyTick = true;

	TickInterval = 0.2f;
	TickIntervalRandomDeviation = 0.05f;

	// Force Replan by default.
	// This way if we're in a SubPlan, we'll actually replan it even if it's not configured to Loop when Failed.
	ReplanParameters.bForceReplan = true;
}

void UHTNService_ReplanIfLocationChanges::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar.UsingCustomVersion(FHTNObjectVersion::GUID);

	const int32 HTNObjectVersion = Ar.CustomVer(FHTNObjectVersion::GUID);
	if (HTNObjectVersion < FHTNObjectVersion::AddReplanSettingsStructToHTNServiceReplanIfLocationChanges)
	{
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		ReplanParameters.bForceAbortPlan = bForceAbortPlan_DEPRECATED;
		ReplanParameters.bForceRestartActivePlanning = bForceRestartActivePlanning_DEPRECATED;
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}
}

void UHTNService_ReplanIfLocationChanges::InitializeFromAsset(UHTN& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (Asset.BlackboardAsset)
	{
		BlackboardKey.ResolveSelectedKey(*Asset.BlackboardAsset);
	}
}

uint16 UHTNService_ReplanIfLocationChanges::GetInstanceMemorySize() const
{
	return sizeof(FMemory);
}

void UHTNService_ReplanIfLocationChanges::InitializeMemory(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlan& Plan, const FHTNPlanStepID& StepID) const
{
	Super::InitializeMemory(OwnerComp, NodeMemory, Plan, StepID);
	new(NodeMemory) FMemory;
}

FString UHTNService_ReplanIfLocationChanges::GetStaticDescription() const
{
	TStringBuilder<2048> SB;

	SB << Super::GetStaticDescription() << TEXT(":\n");
	SB << TEXT("\nBlackboard Key: ") << BlackboardKey.SelectedKeyName.ToString();
	SB << TEXT("\nTolerance: ") << FString::SanitizeFloat(Tolerance) << TEXT("cm");

	SB << TEXT("\n\nReplan Parameters:\n") << ReplanParameters.ToDetailedString();

	return SB.ToString();
}

void UHTNService_ReplanIfLocationChanges::OnExecutionStart(UHTNComponent& OwnerComp, uint8* NodeMemory)
{
	SetInitialLocation(OwnerComp, NodeMemory);
}

void UHTNService_ReplanIfLocationChanges::TickNode(UHTNComponent& OwnerComp, uint8* NodeMemory, float DeltaTime)
{
	FMemory& Memory = *CastInstanceNodeMemory<FMemory>(NodeMemory);

	// No need to triggera replan if we're already planning.
	if (const UHTNPlanInstance* const PlanInstance = ReplanParameters.bReplanOutermostPlanInstance ?
		&OwnerComp.GetRootPlanInstance() :
		OwnerComp.FindPlanInstanceByNodeMemory(NodeMemory))
	{
		if (PlanInstance->IsPlanning())
		{
			return;
		}
	}

	if (!Memory.InitialLocation.IsSet())
	{
		SetInitialLocation(OwnerComp, NodeMemory);
		return;
	}
	
	bool bShouldReplan = false;
	
	FVector CurrentLocation;
	if (GetLocation(OwnerComp, CurrentLocation))
	{
		const float DistSquared = FVector::DistSquared(CurrentLocation, Memory.InitialLocation.GetValue());
		if (DistSquared >= Tolerance * Tolerance)
		{
			bShouldReplan = true;
		}
	}
	else
	{
		bShouldReplan = true;
	}

	if (bShouldReplan)
	{
		FHTNReplanParameters Params = ReplanParameters;
		UE_IFVLOG(
			if (Params.DebugReason.IsEmpty())
			{
				Params.DebugReason = FString::Printf(TEXT("Key %s moved more than %s cm"),
					*BlackboardKey.SelectedKeyName.ToString(),
					*FString::SanitizeFloat(Tolerance));
			});
		Replan(OwnerComp, NodeMemory, Params);
	}
}

bool UHTNService_ReplanIfLocationChanges::GetLocation(UHTNComponent& OwnerComp, FVector& OutLocation) const
{
	if (UBlackboardComponent* const BlackboardComponent = OwnerComp.GetBlackboardComponent())
	{
		if (BlackboardComponent->GetLocationFromEntry(BlackboardKey.GetSelectedKeyID(), OutLocation))
		{
			return true;
		}
	}

	return false;
}

bool UHTNService_ReplanIfLocationChanges::SetInitialLocation(UHTNComponent& OwnerComp, uint8* NodeMemory) const
{
	FVector InitialLocation;
	if (GetLocation(OwnerComp, InitialLocation))
	{
		FMemory& Memory = *CastInstanceNodeMemory<FMemory>(NodeMemory);
		Memory.InitialLocation = InitialLocation;
		return true;
	}

	return false;
}
