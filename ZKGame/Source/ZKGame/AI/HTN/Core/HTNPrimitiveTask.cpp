// Fill out your copyright notice in the Description page of Project Settings.


#include "HTNPrimitiveTask.h"

UHTNPrimitiveTask::UHTNPrimitiveTask()
{
	TaskType = EHTNTaskType::Primitive;
	Status = EHTNTaskStatus::Inactive;
}

EHTNTaskStatus UHTNPrimitiveTask::Execute_Implementation(AAIController* Controller, float DeltaTime)
{
	// 태스크 처음 시작 시
	if (Status == EHTNTaskStatus::Inactive)
	{
		Status = EHTNTaskStatus::InProgress;
		ElapsedTime = 0.0f;
		OnTaskStart(Controller);
	}
    
	// 진행 중인 경우
	if (Status == EHTNTaskStatus::InProgress)
	{
		ElapsedTime += DeltaTime;
        
		// 실행 시간이 지나면 완료
		if (ElapsedTime >= ExecutionTime)
		{
			Status = EHTNTaskStatus::Succeeded;
			OnTaskComplete(Controller, true);
		}
	}
    
	return Status;
}

void UHTNPrimitiveTask::OnTaskStart_Implementation(AAIController* Controller)
{
	// 블루프린트에서 오버라이드 (기본 구현은 비어있음)
}

void UHTNPrimitiveTask::OnTaskComplete_Implementation(AAIController* Controller, bool bSuccess)
{
	// 블루프린트에서 오버라이드 (기본 구현은 비어있음)
}

void UHTNPrimitiveTask::Initialize()
{
	Super::Initialize();
	Status = EHTNTaskStatus::Inactive;
	ElapsedTime = 0.0f;
}