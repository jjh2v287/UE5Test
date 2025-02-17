// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKStateTreePatrolSplineMove.h"
#include "StateTreeExecutionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UKStateTreePatrolSplineMove)

#define LOCTEXT_NAMESPACE "GameplayStateTree"

EStateTreeRunStatus FUKStateTreePatrolSplineMove::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	TWeakObjectPtr<AActor> OwnerActor = InstanceData.ReferenceActor.Get();

	if (InstanceData.ReferenceActor->IsValidLowLevel())
	{
		InstanceData.MovementComponent = InstanceData.ReferenceActor->GetComponentByClass<UCharacterMovementComponent>();
	}
	
	return Super::EnterState(Context, Transition);
}

EStateTreeRunStatus FUKStateTreePatrolSplineMove::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (InstanceData.MovementComponent.IsValid())
	{
		InstanceData.MovementComponent;
	}
	return Super::Tick(Context, DeltaTime);
}

void FUKStateTreePatrolSplineMove::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	Super::ExitState(Context, Transition);
}

#undef LOCTEXT_NAMESPACE
