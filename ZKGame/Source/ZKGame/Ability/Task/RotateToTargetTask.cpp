// Fill out your copyright notice in the Description page of Project Settings.


#include "Ability/Task/RotateToTargetTask.h"

URotateToTargetTask* URotateToTargetTask::RotateActorToTarget(UGameplayAbility* OwningAbility, FRotator TargetRotation, float Duration)
{
	URotateToTargetTask* MyTask = NewAbilityTask<URotateToTargetTask>(OwningAbility);
	MyTask->TargetRotation = TargetRotation;
	MyTask->Duration = Duration;
	return MyTask;
}

void URotateToTargetTask::Activate()
{
	if (AActor* Actor = GetAvatarActor())
	{
		StartRotation = Actor->GetActorRotation();
		ElapsedTime = 0.0f;
        
		// 틱 활성화
		bTickingTask = true;
	}
	else
	{
		EndTask();
	}
}

void URotateToTargetTask::TickTask(float DeltaTime)
{
	if (AActor* Actor = GetAvatarActor())
	{
		ElapsedTime += DeltaTime;
		float Alpha = FMath::Min(ElapsedTime / Duration, 1.0f);
        
		// 선형 보간으로 회전 계산
		FRotator NewRotation = FMath::Lerp(StartRotation, TargetRotation, Alpha);
		Actor->SetActorRotation(NewRotation);

		// 회전이 완료되면 태스크 종료
		if (Alpha >= 1.0f)
		{
			OnRotationCompleted.Broadcast();
			EndTask();
		}
	}
	else
	{
		EndTask();
	}
}