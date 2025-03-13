// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKStateTreeGlobalTask.h"
#include "StateTreeExecutionContext.h"
#include "TimerManager.h"
#include "Components/CapsuleComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UKStateTreeGlobalTask)

#define LOCTEXT_NAMESPACE "GameplayStateTree"

EStateTreeRunStatus UUKGlobalTaskData::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition)
{
	RunStatus = EStateTreeRunStatus::Running;
	// Event binding and initial setting of data
	if (AIController)
	{
		AActor* Instigator = AIController->GetInstigator();
		if (Instigator)
		{
			UCapsuleComponent* CapsuleComponent = Instigator->GetComponentByClass<UCapsuleComponent>();
			if (CapsuleComponent)
			{
				CapsuleComponent->OnComponentHit.AddDynamic(this, &UUKGlobalTaskData::OnHitCallback);
			}
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("UUKGlobalTaskData EnterState"));
	return RunStatus;
}

EStateTreeRunStatus UUKGlobalTaskData::Tick(FStateTreeExecutionContext& Context, const float DeltaTime)
{
	UE_LOG(LogTemp, Warning, TEXT("UUKGlobalTaskData Tick %f"), DeltaTime);
	return RunStatus;
}

void UUKGlobalTaskData::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition)
{
	UE_LOG(LogTemp, Warning, TEXT("UUKGlobalTaskData ExitState"));
}

void UUKGlobalTaskData::OnHitCallback(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	UE_LOG(LogTemp, Warning, TEXT("UUKGlobalTaskData OnHitCallback %s"), *OtherActor->GetName());
}

EStateTreeRunStatus FUKStateTreeGlobalTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	UUKGlobalTaskData* Instance = Context.GetInstanceDataPtr<UUKGlobalTaskData>(*this);
	check(Instance);
	return Instance->EnterState(Context, Transition);
	
	/*FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
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
	return Super::EnterState(Context, Transition);*/
}

EStateTreeRunStatus FUKStateTreeGlobalTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	UUKGlobalTaskData* Instance = Context.GetInstanceDataPtr<UUKGlobalTaskData>(*this);
	check(Instance);
	return Instance->Tick(Context, DeltaTime);
}

void FUKStateTreeGlobalTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	UUKGlobalTaskData* Instance = Context.GetInstanceDataPtr<UUKGlobalTaskData>(*this);
	check(Instance);
	Instance->ExitState(Context, Transition);
}

#undef LOCTEXT_NAMESPACE
