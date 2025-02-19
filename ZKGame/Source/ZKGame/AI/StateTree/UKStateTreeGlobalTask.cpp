// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKStateTreeGlobalTask.h"
#include "StateTreeExecutionContext.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UKStateTreeGlobalTask)

#define LOCTEXT_NAMESPACE "GameplayStateTree"

EStateTreeRunStatus FUKStateTreeGlobalTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	TWeakObjectPtr<AActor> OwnerActor = InstanceData.Actor.Get();

	if (InstanceData.Actor->IsValidLowLevel())
	{
		InstanceData.MovementComponent = InstanceData.Actor->GetComponentByClass<UCharacterMovementComponent>();
	}

	auto [IsMove, VectorPtr, ActorPtr, ArrayOfVector, ArrayOfActor] = InstanceData.IsMove.GetMutablePtrTuple<bool, FVector, AActor*, TArray<FVector>, TArray<AActor*>>(Context);
	if (IsMove)
	{
		*IsMove = true;
	}

	// if (OwnerActor.IsValid())
	// {
	// 	OwnerActor->GetWorld()->GetTimerManager().SetTimer(InstanceData.TimerHandle, [this, OwnerActor, &InstanceData, &Context]()
	// 	{
	// 		auto [IsMove, VectorPtr, ActorPtr, ArrayOfVector, ArrayOfActor] = InstanceData.IsMove.GetMutablePtrTuple<bool, FVector, AActor*, TArray<FVector>, TArray<AActor*>>(Context);
	// 		if (IsMove)
	// 		{
	// 			*IsMove = true;
	// 		}
	// 		// auto [IsMove, VectorPtr, ActorPtr, ArrayOfVector, ArrayOfActor] = InstanceData.IsMove.GetMutablePtrTuple<bool, FVector, AActor*, TArray<FVector>, TArray<AActor*>>(Context);
	// 		// if (IsMove)
	// 		// {
	// 		// 	*IsMove = true;
	// 		// }
	// 		// OwnerActor->GetWorld()->GetTimerManager().ClearTimer(InstanceData.TimerHandle);
	// 	}, 1.0f, false);
	// }
	return Super::EnterState(Context, Transition);
}

EStateTreeRunStatus FUKStateTreeGlobalTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	return Super::Tick(Context, DeltaTime);
}

void FUKStateTreeGlobalTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	Super::ExitState(Context, Transition);
}

#undef LOCTEXT_NAMESPACE
