// Fill out your copyright notice in the Description page of Project Settings.


#include "UKStateTreeAIComponentSchema.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "GameFramework/Pawn.h"
#include "StateTreeExecutionContext.h"

UUKStateTreeAIComponentSchema::UUKStateTreeAIComponentSchema(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	check(ContextDataDescs.Num() == 1 && ContextDataDescs[0].Struct == AActor::StaticClass());
	// Make the Actor a pawn by default so it binds to the controlled pawn instead of the AIController.
	// ContextActorClass = APawn::StaticClass();
	// ContextDataDescs[0].Struct = ContextActorClass.Get();
	ContextDataDescs.Emplace(TEXT("Blackboard"), BlackboardComponentClass.Get(), FGuid::NewGuid());
}

void UUKStateTreeAIComponentSchema::PostLoad()
{
	Super::PostLoad();
	ContextDataDescs[2].Struct = BlackboardComponentClass.Get();
}

bool UUKStateTreeAIComponentSchema::IsStructAllowed(const UScriptStruct* InScriptStruct) const
{
	return Super::IsStructAllowed(InScriptStruct);
}

bool UUKStateTreeAIComponentSchema::SetContextRequirements(UBrainComponent& BrainComponent, FStateTreeExecutionContext& Context, bool bLogErrors)
{
	if (!Context.IsValid())
	{
		return false;
	}

	const FName ABlackboardName(TEXT("Blackboard"));
	Context.SetContextDataByName(ABlackboardName, FStateTreeDataView(BrainComponent.GetAIOwner()->GetBlackboardComponent()));

	return Super::SetContextRequirements(BrainComponent, Context, bLogErrors);
}

#if WITH_EDITOR
void UUKStateTreeAIComponentSchema::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	if (FProperty* Property = PropertyChangedEvent.Property)
	{
		if (Property->GetOwnerClass() == UUKStateTreeAIComponentSchema::StaticClass()
			&& Property->GetFName() == GET_MEMBER_NAME_CHECKED(UUKStateTreeAIComponentSchema, BlackboardComponentClass))
		{
			ContextDataDescs[2].Struct = BlackboardComponentClass.Get();
		}
	}
}
#endif // WITH_EDITOR