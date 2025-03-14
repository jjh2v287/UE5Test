// Fill out your copyright notice in the Description page of Project Settings.


#include "UKStateTreeAIComponent.h"
#include "StateTreeExecutionContext.h"
#include "UKStateTreeAIComponentSchema.h"


TSubclassOf<UStateTreeSchema> UUKStateTreeAIComponent::GetSchema() const
{
	return UUKStateTreeAIComponentSchema::StaticClass();
}

bool UUKStateTreeAIComponent::SetContextRequirements(FStateTreeExecutionContext& Context, bool bLogErrors /*= false*/)
{
	Context.SetCollectExternalDataCallback(FOnCollectStateTreeExternalData::CreateUObject(this, &UUKStateTreeAIComponent::CollectExternalData));
	return UUKStateTreeAIComponentSchema::SetContextRequirements(*this, Context, bLogErrors);
}
