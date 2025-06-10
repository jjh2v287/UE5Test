// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "Decorators/HTNDecorator_FocusScope.h"
#include "AIController.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Rotator.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "Misc/StringBuilder.h"
#include "VisualLogger/VisualLogger.h"

UHTNDecorator_FocusScope::UHTNDecorator_FocusScope(const FObjectInitializer& Initializer) : Super(Initializer),
	bSetNewFocus(true),
	bObserveBlackboardValue(false),
	bUpdateFocalPointFromRotatorKeyEveryFrame(true),
	bRestoreOldFocusOnExecutionFinish(true),
	FocusPriority(EAIFocusPriority::Gameplay),
	RotationKeyLookAheadDistance(10000.0f)
{
	bNotifyExecutionStart = true;
	bNotifyExecutionFinish = true;

	bCheckConditionOnPlanEnter = false;
	bCheckConditionOnPlanExit = false;
	bCheckConditionOnPlanRecheck = false;
	bCheckConditionOnTick = false;

	FocusTarget.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, FocusTarget), AActor::StaticClass());
	FocusTarget.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, FocusTarget));
	FocusTarget.AddRotatorFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, FocusTarget));
}

FString UHTNDecorator_FocusScope::GetNodeName() const
{
	if (NodeName.Len())
	{
		return Super::GetNodeName();
	}

	if (bSetNewFocus)
	{
		return FString::Printf(TEXT("Focus %s %s"), 
			IsFocusTargetRotator()  ? TEXT("along") : TEXT("on"), 
			*FocusTarget.SelectedKeyName.ToString());
	}
	else if (bRestoreOldFocusOnExecutionFinish)
	{
		return TEXT("Restore Focus After");
	}
	else
	{
		return TEXT("Focus Scope");
	}
}

FString UHTNDecorator_FocusScope::GetStaticDescription() const
{
	TStringBuilder<2048> StringBuilder;
	StringBuilder << Super::GetStaticDescription() << TEXT(":");

	if (bSetNewFocus)
	{
		StringBuilder << TEXT("\nFocus target: ") << FocusTarget.SelectedKeyName.ToString();

		if (bObserveBlackboardValue)
		{
			StringBuilder << TEXT("\nObserves blackboard value");
		}

		if (IsFocusTargetRotator())
		{
			StringBuilder << TEXT("\nFocuses on Pawn's viewpoint + ") << FocusTarget.SelectedKeyName.ToString() << TEXT(" * ") << FString::SanitizeFloat(RotationKeyLookAheadDistance) << TEXT("cm");
		}

		if (ShouldUpdateFocalPointEveryFrame())
		{
			StringBuilder << TEXT("\nUpdates every tick");
		}
	}

	if (bRestoreOldFocusOnExecutionFinish)
	{
		StringBuilder << TEXT("\nRestores old focus on execution finish");
	}

	StringBuilder << TEXT("\nFocus Priority: ") << FocusPriority;

	return *StringBuilder;
}

void UHTNDecorator_FocusScope::InitializeFromAsset(UHTN& Asset)
{
	Super::InitializeFromAsset(Asset);

	const UBlackboardData* const BBAsset = GetBlackboardAsset();
	if (IsValid(BBAsset))
	{
		FocusTarget.ResolveSelectedKey(*BBAsset);
	}
	else
	{
		UE_LOG(LogHTN, Warning, TEXT("Can't initialize %s due to missing blackboard data."), *GetNodeName());
		FocusTarget.InvalidateResolvedKey();
	}

	bNotifyTick = ShouldUpdateFocalPointEveryFrame();
}

uint16 UHTNDecorator_FocusScope::GetInstanceMemorySize() const { return sizeof(FNodeMemory); }

void UHTNDecorator_FocusScope::InitializeMemory(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlan& Plan, const FHTNPlanStepID& StepID) const
{
	FNodeMemory& Memory = *(new(NodeMemory) FNodeMemory);
}

void UHTNDecorator_FocusScope::TickNode(UHTNComponent& OwnerComp, uint8* NodeMemory, float DeltaTime)
{
	if (ShouldUpdateFocalPointEveryFrame())
	{
		if (const UBlackboardComponent* const Blackboard = OwnerComp.GetBlackboardComponent())
		{
			FNodeMemory& Memory = *CastInstanceNodeMemory<FNodeMemory>(NodeMemory);
			SetFocusFromBlackboard(*Blackboard, Memory);
		}
	}
}

void UHTNDecorator_FocusScope::OnExecutionStart(UHTNComponent& OwnerComp, uint8* NodeMemory)
{
	FNodeMemory& Memory = *CastInstanceNodeMemory<FNodeMemory>(NodeMemory);

	if (bRestoreOldFocusOnExecutionFinish)
	{
		AAIController* const Controller = OwnerComp.GetAIOwner();
		if (IsValid(Controller))
		{
			if (AActor* const PreviousFocusActor = Controller->GetFocusActorForPriority(FocusPriority))
			{
				UE_VLOG_UELOG(Controller, LogHTN, Verbose, TEXT("%s: Saving previous focus %s (priority %i)"),
					*GetShortDescription(), *GetNameSafe(PreviousFocusActor), FocusPriority);
				Memory.OldFocus.Set<TWeakObjectPtr<AActor>>(PreviousFocusActor);
			}
			else
			{
				const FVector PreviousFocalPoint = Controller->GetFocalPointForPriority(FocusPriority);
				UE_VLOG_UELOG(Controller, LogHTN, Verbose, TEXT("%s: Saving previous focus %s (priority %i)"),
					*GetShortDescription(), *AILocationToString(PreviousFocalPoint), FocusPriority);
				Memory.OldFocus.Set<FVector>(PreviousFocalPoint);
			}
		}
	}

	if (bSetNewFocus)
	{
		UBlackboardComponent* const Blackboard = OwnerComp.GetBlackboardComponent();
		if (IsValid(Blackboard))
		{
			// Subscribe to key changes.
			if (bObserveBlackboardValue)
			{
				Memory.BlackboardObserverHandle = Blackboard->RegisterObserver(FocusTarget.GetSelectedKeyID(), this,
					FOnBlackboardChangeNotification::CreateUObject(this, &UHTNDecorator_FocusScope::OnBlackboardKeyValueChange, NodeMemory));
			}

			// Memorize the rotator to focus along so that we ignore changes to it if bObserveBlackboardValue is false.
			if (IsFocusTargetRotator())
			{
				Memory.RotationToFocusAlong = Blackboard->GetValue<UBlackboardKeyType_Rotator>(FocusTarget.GetSelectedKeyID());
			}

			SetFocusFromBlackboard(*Blackboard, Memory);
		}
	}
}

void UHTNDecorator_FocusScope::OnExecutionFinish(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNNodeResult Result)
{
	FNodeMemory& Memory = *CastInstanceNodeMemory<FNodeMemory>(NodeMemory);

	if (Memory.BlackboardObserverHandle.IsValid())
	{
		UBlackboardComponent* const Blackboard = OwnerComp.GetBlackboardComponent();
		if (IsValid(Blackboard)) 
		{
			Blackboard->UnregisterObserver(FocusTarget.GetSelectedKeyID(), Memory.BlackboardObserverHandle);
		}
		Memory.BlackboardObserverHandle.Reset();
	}

	if (bRestoreOldFocusOnExecutionFinish)
	{
		AAIController* const Controller = OwnerComp.GetAIOwner();
		if (IsValid(Controller))
		{
			if (Memory.OldFocus.IsType<TWeakObjectPtr<AActor>>())
			{
				const TWeakObjectPtr<AActor> PreviousFocusActor = Memory.OldFocus.Get<TWeakObjectPtr<AActor>>();
				UE_VLOG_UELOG(Controller, LogHTN, Verbose, TEXT("%s: Restoring focus to %s (priority %i)"), 
					*GetShortDescription(), *GetNameSafe(PreviousFocusActor.Get()), FocusPriority);
				Controller->SetFocus(PreviousFocusActor.Get(), FocusPriority);
			}
			else
			{
				const FVector PreviousFocalPoint = Memory.OldFocus.Get<FVector>();
				UE_VLOG_UELOG(Controller, LogHTN, Verbose, TEXT("%s: Restoring focus to %s (priority %i)"), 
					*GetShortDescription(), *AILocationToString(PreviousFocalPoint), FocusPriority);
				Controller->SetFocalPoint(PreviousFocalPoint, FocusPriority);
			}
		}
	}
}

bool UHTNDecorator_FocusScope::IsFocusTargetRotator() const
{
	return FocusTarget.SelectedKeyType == UBlackboardKeyType_Rotator::StaticClass();
}

bool UHTNDecorator_FocusScope::ShouldUpdateFocalPointEveryFrame() const
{
	return bSetNewFocus && bUpdateFocalPointFromRotatorKeyEveryFrame && IsFocusTargetRotator();
}

EBlackboardNotificationResult UHTNDecorator_FocusScope::OnBlackboardKeyValueChange(const UBlackboardComponent& Blackboard, FBlackboard::FKey ChangedKeyID, uint8* NodeMemory)
{
	UE_VLOG_UELOG(Blackboard.GetOwner(), LogHTN, Verbose, TEXT("%s: Blackboard key %s changed, updating focus."), 
		*GetShortDescription(), *Blackboard.GetKeyName(ChangedKeyID).ToString());

	FNodeMemory& Memory = *CastInstanceNodeMemory<FNodeMemory>(NodeMemory);
	if (IsFocusTargetRotator())
	{
		Memory.RotationToFocusAlong = Blackboard.GetValue<UBlackboardKeyType_Rotator>(FocusTarget.GetSelectedKeyID());
	}

	SetFocusFromBlackboard(Blackboard, Memory);

	return EBlackboardNotificationResult::ContinueObserving;
}

void UHTNDecorator_FocusScope::SetFocusFromBlackboard(const UBlackboardComponent& Blackboard, const FNodeMemory& Memory)
{
	AAIController* const Controller = Cast<AAIController>(Blackboard.GetOwner());
	if (IsValid(Controller))
	{
		if (FocusTarget.SelectedKeyType == UBlackboardKeyType_Object::StaticClass())
		{
			AActor* const FocusActor = Cast<AActor>(Blackboard.GetValue<UBlackboardKeyType_Object>(FocusTarget.GetSelectedKeyID()));
			UE_VLOG_UELOG(Controller, LogHTN, Verbose, TEXT("%s: Setting focus to %s (priority %i)"), 
				*GetShortDescription(), *GetNameSafe(FocusActor), FocusPriority);
			Controller->SetFocus(FocusActor, FocusPriority);
		}
		else if (FocusTarget.SelectedKeyType == UBlackboardKeyType_Vector::StaticClass())
		{
			const FVector FocalPoint = Blackboard.GetValue<UBlackboardKeyType_Vector>(FocusTarget.GetSelectedKeyID());
			UE_VLOG_UELOG(Controller, LogHTN, Verbose, TEXT("%s: Setting focus to %s (priority %i)"), 
				*GetShortDescription(), *AILocationToString(FocalPoint), FocusPriority);
			Controller->SetFocalPoint(FocalPoint, FocusPriority);
		}
		else if (FocusTarget.SelectedKeyType == UBlackboardKeyType_Rotator::StaticClass())
		{
			if (const APawn* const Pawn = Controller->GetPawn())
			{
				if (FAISystem::IsValidRotation(Memory.RotationToFocusAlong))
				{
					const FVector PointInDistance = Pawn->GetPawnViewLocation() + Memory.RotationToFocusAlong.Vector() * RotationKeyLookAheadDistance;
					UE_VLOG_UELOG(Controller, LogHTN, Verbose, TEXT("%s: Setting focus along rotation %s to %s (priority %i)"),
						*GetShortDescription(), *AIRotationToString(Memory.RotationToFocusAlong), *AILocationToString(PointInDistance), FocusPriority);
					Controller->SetFocalPoint(PointInDistance, FocusPriority);
				}
				else
				{
					UE_VLOG_UELOG(Controller, LogHTN, Verbose,
						TEXT("%s: Could not set focal point along rotation because the rotation was invalid"),
						*GetShortDescription());
				}
			}
			else
			{
				UE_VLOG_UELOG(Controller, LogHTN, Verbose, 
					TEXT("%s: Could not set focal point to rotation because the AIController is not controlling a Pawn, so we can't determine what to set the focal point to"),
					*GetShortDescription());
			}
		}
	}
}

FString UHTNDecorator_FocusScope::AILocationToString(const FVector& Location)
{
	return FAISystem::IsValidLocation(Location) ? Location.ToString() : TEXT("(invalid location)");
}

FString UHTNDecorator_FocusScope::AIRotationToString(const FRotator& Rotation)
{
	return FAISystem::IsValidRotation(Rotation) ? Rotation.ToString() : TEXT("(invalid rotation)");
}
