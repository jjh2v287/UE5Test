// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "Tasks/HTNTask_EQSQuery.h"
#include "HTNObjectVersion.h"

#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_ActorBase.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Misc/ScopeExit.h"
#include "Misc/StringBuilder.h"
#include "Runtime/Launch/Resources/Version.h"
#include "VisualLogger/VisualLogger.h"

UHTNTask_EQSQuery::UHTNTask_EQSQuery(const FObjectInitializer& Initializer) : Super(Initializer),
	MaxNumCandidatePlans(1),
	Cost(0)
{
	bShowTaskNameOnCurrentPlanVisualization = false;

	BlackboardKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, BlackboardKey), UObject::StaticClass());
	BlackboardKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, BlackboardKey));
}

void UHTNTask_EQSQuery::Serialize(FArchive& Ar)
{
	Ar.UsingCustomVersion(FHTNObjectVersion::GUID);

	const int32 HTNObjectVersion = Ar.CustomVer(FHTNObjectVersion::GUID);
	if (HTNObjectVersion < FHTNObjectVersion::DisableDefaultCostOnSomeTasks)
	{
		// When loading an older version asset, restore this value to the old default before loading.
		// If the value was changed in asset, it will be overwritten during loading. 
		// If it wasn't, it'll stay at the old default.
		Cost = 100;
	}

	Super::Serialize(Ar);
}

void UHTNTask_EQSQuery::InitializeFromAsset(UHTN& Asset)
{
	Super::InitializeFromAsset(Asset);
	EQSRequest.InitForOwnerAndBlackboard(*this, GetBlackboardAsset());
}

void UHTNTask_EQSQuery::CreatePlanSteps(UHTNComponent& OwnerComp, UAITask_MakeHTNPlan& PlanningTask, const TSharedRef<const FBlackboardWorldState>& WorldState) const
{
	struct Local
	{
		static AActor* GetQueryOwner(UHTNComponent& HTNComp)
		{
			AActor* const ComponentOwner = HTNComp.GetOwner();
			if (AController* const Controller = Cast<AController>(ComponentOwner))
			{
				return Controller->GetPawn();
			}
			return ComponentOwner;
		}
	};

	if (!EQSRequest.IsValid())
	{
		return;
	}

	AActor* const QueryOwner = Local::GetQueryOwner(OwnerComp);
	if (!QueryOwner)
	{
		return;
	}

	const int32 RequestID = EQSRequest.Execute(*QueryOwner, *WorldState, FQueryFinishedSignature::CreateWeakLambda(const_cast<UHTNTask_EQSQuery*>(this), 
	[
		this, 
		WorldStatePtr = TWeakPtr<const FBlackboardWorldState>(WorldState), 
		PlanningTaskPtr = TWeakObjectPtr<UAITask_MakeHTNPlan>(&PlanningTask),
		PlanningID = PlanningTask.GetPlanningID()
	]
	(TSharedPtr<FEnvQueryResult> Result)
	{
		ON_SCOPE_EXIT
		{
			if (PlanningTaskPtr.IsValid() && PlanningTaskPtr->GetPlanningID() == PlanningID)
			{
				PlanningTaskPtr->FinishLatentCreatePlanSteps(this);
			}
		};

		if (!PlanningTaskPtr.IsValid() || PlanningTaskPtr->GetPlanningID() != PlanningID)
		{
			return;
		}
		
#if ENGINE_MAJOR_VERSION >= 5
		const bool bSuccess = Result.IsValid() && Result->IsSuccessful() && Result->Items.Num() > 0;
#else
		const bool bSuccess = Result.IsValid() && Result->IsSuccsessful() && Result->Items.Num() > 0;
#endif
		if (!bSuccess)
		{
			PlanningTaskPtr->SetNodePlanningFailureReason(TEXT("EQS query failed"));
			return;
		}

		const TSharedPtr<const FBlackboardWorldState> OldWorldState = WorldStatePtr.Pin();
		if (!OldWorldState.IsValid())
		{
			return;
		}

		UEnvQueryItemType* const ItemTypeCDO = Result->ItemType->GetDefaultObject<UEnvQueryItemType>();
		if (!ensure(ItemTypeCDO))
		{
			return;
		}

		const int32 MaxNumSteps = EQSRequest.RunMode == EEnvQueryRunMode::AllMatching ? FMath::Max(MaxNumCandidatePlans, 0) : 1;
		const int32 NumSteps = MaxNumSteps > 0 ? FMath::Min(Result->Items.Num(), MaxNumSteps) : Result->Items.Num();
		for (int32 ItemIndex = 0; ItemIndex < NumSteps; ++ItemIndex)
		{
			const TSharedRef<FBlackboardWorldState> NewWorldState = OldWorldState->MakeNext();
			const uint8* const RawItemData = Result->RawData.GetData() + Result->Items[ItemIndex].DataOffset;
			if (StoreInWorldState(ItemTypeCDO, BlackboardKey, *NewWorldState, RawItemData))
			{
#if HTN_DEBUG_PLANNING && ENABLE_VISUAL_LOG
				const FString Description = FVisualLogger::IsRecording() ? ItemTypeCDO->GetDescription(RawItemData) : TEXT("");
#else
				const FString Description = TEXT("");
#endif
				PlanningTaskPtr->SubmitPlanStep(this, NewWorldState, FMath::Max(0, Cost), Description);
			}
			else
			{
				UE_LOG(LogHTN, Error, TEXT("%s: Failed to store result %i (%s) into world state at key %s"),
					*GetShortDescription(), ItemIndex, *ItemTypeCDO->GetDescription(RawItemData), *BlackboardKey.SelectedKeyName.ToString());
			}
		}
	}));

	if (RequestID != INDEX_NONE)
	{
		PlanningTask.WaitForLatentCreatePlanSteps(this);
	}
}

FString UHTNTask_EQSQuery::GetNodeName() const
{
	if (NodeName.Len())
	{
		return Super::GetNodeName();
	}

	if (EQSRequest.bUseBBKeyForQueryTemplate)
	{
		return FString::Printf(TEXT("EQS Query from key %s"), *EQSRequest.EQSQueryBlackboardKey.SelectedKeyName.ToString());
	}
	else
	{
		if (IsValid(EQSRequest.QueryTemplate))
		{
			return GetSubStringAfterUnderscore(GetNameSafe(EQSRequest.QueryTemplate));
		}
		else
		{
			return TEXT("EQS Query");
		}
	}
}

FString UHTNTask_EQSQuery::GetStaticDescription() const
{
	TStringBuilder<2048> StringBuilder;
	StringBuilder << Super::GetStaticDescription() << TEXT(": ");

	if (EQSRequest.bUseBBKeyForQueryTemplate)
	{
		StringBuilder << TEXT("EQS query indicated by ") << EQSRequest.EQSQueryBlackboardKey.SelectedKeyName.ToString() << TEXT(" blackboard key");
	}
	else
	{
		StringBuilder << GetNameSafe(EQSRequest.QueryTemplate);
	}

	StringBuilder << FString::Printf(TEXT("\nResult Blackboard key: %s"), *BlackboardKey.SelectedKeyName.ToString());

	if (EQSRequest.RunMode == EEnvQueryRunMode::AllMatching)
	{
		if (MaxNumCandidatePlans > 1)
		{
			StringBuilder << FString::Printf(TEXT("\nProduces up to %i candidate steps"), MaxNumCandidatePlans);
		}
		else if (MaxNumCandidatePlans < 0)
		{
			StringBuilder << TEXT("\nProduces any number of candidate steps");
		}
	}

	if (Cost > 0)
	{
		StringBuilder << FString::Printf(TEXT("\nCost: %i"), Cost);
	}

	return StringBuilder.ToString();
}

#if WITH_EDITOR
void UHTNTask_EQSQuery::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.MemberProperty &&
		PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UHTNTask_EQSQuery, EQSRequest))
	{
		EQSRequest.PostEditChangeProperty(*this, PropertyChangedEvent);
	}
}

FName UHTNTask_EQSQuery::GetNodeIconName() const
{
	return FName("BTEditor.Graph.BTNode.Task.RunEQSQuery.Icon");
}

UObject* UHTNTask_EQSQuery::GetAssetToOpenOnDoubleClick(const UHTNComponent* DebuggedHTNComponent) const
{
	if (!EQSRequest.bUseBBKeyForQueryTemplate)
	{
		return EQSRequest.QueryTemplate;
	}

	if (DebuggedHTNComponent)
	{
		if (const UBlackboardComponent* const BlackboardComponent = DebuggedHTNComponent->GetBlackboardComponent())
		{
			UObject* QueryTemplateObject = BlackboardComponent->GetValue<UBlackboardKeyType_Object>(EQSRequest.EQSQueryBlackboardKey.GetSelectedKeyID());
			if (UEnvQuery* const QueryTemplate = Cast<UEnvQuery>(QueryTemplateObject))
			{
				return QueryTemplate;
			}
		}
	}

	return Super::GetAssetToOpenOnDoubleClick(DebuggedHTNComponent);
}

#endif

// A rather ugly workaround for the fact that UEnvQueryItemType only supports storing results into a Blackboard and not a WorldState
bool UHTNTask_EQSQuery::StoreInWorldState(UEnvQueryItemType* ItemTypeCDO, 
	const FBlackboardKeySelector& KeySelector,
	FBlackboardWorldState& WorldState, 
	const uint8* RawData
) const
{	
	if (UEnvQueryItemType_VectorBase* const ItemTypeVectorBase = Cast<UEnvQueryItemType_VectorBase>(ItemTypeCDO))
	{
		if (KeySelector.SelectedKeyType == UBlackboardKeyType_Vector::StaticClass())
		{
			const FVector MyLocation = ItemTypeVectorBase->GetItemLocation(RawData);
			return WorldState.SetValue<UBlackboardKeyType_Vector>(KeySelector.GetSelectedKeyID(), MyLocation);
		}

		if (UEnvQueryItemType_ActorBase* const ItemTypeActorBase = Cast<UEnvQueryItemType_ActorBase>(ItemTypeVectorBase))
		{
			if (KeySelector.SelectedKeyType == UBlackboardKeyType_Object::StaticClass())
			{
				UObject* const MyObject = ItemTypeActorBase->GetActor(RawData);
				return WorldState.SetValue<UBlackboardKeyType_Object>(KeySelector.GetSelectedKeyID(), MyObject);
			}
		}

		UE_LOG(LogHTN, Error, TEXT("HTNTask_EQSQuery %s: Cannot store an UEnvQueryItemType %s into a key %s of type %s"),
			*GetShortDescription(),
			*GetNameSafe(ItemTypeCDO->GetClass()),
			*KeySelector.SelectedKeyName.ToString(),
			*GetNameSafe(KeySelector.SelectedKeyType));
		return false;
	}

	UE_LOG(LogHTN, Error, TEXT("HTNTask_EQSQuery %s: Unsupported UEnvQueryItemType: %s"), 
		*GetShortDescription(),
		ItemTypeCDO ? *GetNameSafe(ItemTypeCDO->GetClass()) : TEXT("None"));
	return false;
}
