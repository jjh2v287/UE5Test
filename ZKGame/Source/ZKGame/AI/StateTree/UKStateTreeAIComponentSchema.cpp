// Fill out your copyright notice in the Description page of Project Settings.


#include "UKStateTreeAIComponentSchema.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "GameFramework/Pawn.h"
#include "StateTreeExecutionContext.h"
#include "Conditions/StateTreeAIConditionBase.h"
#include "Tasks/StateTreeAITask.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UKStateTreeAIComponentSchema)

UUKStateTreeAIComponentSchema::UUKStateTreeAIComponentSchema(const FObjectInitializer& ObjectInitializer /*= FObjectInitializer::Get()*/)
	: AIControllerClass(AAIController::StaticClass())
{
	check(ContextDataDescs.Num() == 1 && ContextDataDescs[0].Struct == AActor::StaticClass());
	// Make the Actor a pawn by default so it binds to the controlled pawn instead of the AIController.
	ContextActorClass = APawn::StaticClass();
	ContextDataDescs[0].Struct = ContextActorClass.Get();
	ContextDataDescs.Emplace(TEXT("AIController"), AIControllerClass.Get(), FGuid::NewGuid());
	ContextDataDescs.Emplace(TEXT("Blackboard"), BlackboardComponentClass.Get(), FGuid::NewGuid());
}

void UUKStateTreeAIComponentSchema::PostLoad()
{
	Super::PostLoad();
	ContextDataDescs[1].Struct = AIControllerClass.Get();
	ContextDataDescs[2].Struct = BlackboardComponentClass.Get();
}

bool UUKStateTreeAIComponentSchema::IsStructAllowed(const UScriptStruct* InScriptStruct) const
{
	return Super::IsStructAllowed(InScriptStruct)
		|| InScriptStruct->IsChildOf(FStateTreeAITaskBase::StaticStruct())
		|| InScriptStruct->IsChildOf(FStateTreeAIConditionBase::StaticStruct());
}

bool UUKStateTreeAIComponentSchema::SetContextRequirements(UBrainComponent& BrainComponent, FStateTreeExecutionContext& Context, bool bLogErrors /*= false*/)
{
	if (!Context.IsValid())
	{
		return false;
	}

	Context.SetContextDataByName(TEXT("AIController"), FStateTreeDataView(BrainComponent.GetAIOwner()));

	AAIController* AIController = BrainComponent.GetAIOwner();
	UBlackboardComponent* BlackboardComponent = AIController->GetBlackboardComponent();
	Context.SetContextDataByName(TEXT("Blackboard"), FStateTreeDataView(BlackboardComponent));

	return Super::SetContextRequirements(BrainComponent, Context, bLogErrors);
}

#if WITH_EDITOR
void UUKStateTreeAIComponentSchema::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	if (FProperty* Property = PropertyChangedEvent.Property)
	{
		if (Property->GetOwnerClass() == UUKStateTreeAIComponentSchema::StaticClass()
			&& Property->GetFName() == GET_MEMBER_NAME_CHECKED(UUKStateTreeAIComponentSchema, AIControllerClass))
		{
			ContextDataDescs[1].Struct = AIControllerClass.Get();
		}
		else if (Property->GetOwnerClass() == UUKStateTreeAIComponentSchema::StaticClass()
			&& Property->GetFName() == GET_MEMBER_NAME_CHECKED(UUKStateTreeAIComponentSchema, BlackboardComponentClass))
		{
			ContextDataDescs[2].Struct = BlackboardComponentClass.Get();
		}
	}
}
#endif // WITH_EDITOR