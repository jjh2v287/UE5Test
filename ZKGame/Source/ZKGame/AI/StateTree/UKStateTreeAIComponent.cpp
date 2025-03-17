// Fill out your copyright notice in the Description page of Project Settings.


#include "UKStateTreeAIComponent.h"
#include "StateTreeExecutionContext.h"
#include "UKStateTreeAIComponentSchema.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UKStateTreeAIComponent)

TSubclassOf<UStateTreeSchema> UUKStateTreeAIComponent::GetSchema() const
{
	return UUKStateTreeAIComponentSchema::StaticClass();
}

bool UUKStateTreeAIComponent::SetContextRequirements(FStateTreeExecutionContext& Context, bool bLogErrors /*= false*/)
{
	Context.SetCollectExternalDataCallback(FOnCollectStateTreeExternalData::CreateUObject(this, &UUKStateTreeAIComponent::CollectExternalData));
	return UUKStateTreeAIComponentSchema::SetContextRequirements(*this, Context, bLogErrors);
}
