// Fill out your copyright notice in the Description page of Project Settings.

#include "HTNTask.h"

UHTNTask::UHTNTask()
{
	TaskType = EHTNTaskType::Primitive;
}

bool UHTNTask::CheckPreconditions_Implementation(const FCoffeeWorldState& WorldState)
{
	// 기본적으로 조건 없음 (오버라이드하여 구현)
	return true;
}

void UHTNTask::ApplyEffects_Implementation(FCoffeeWorldState& WorldState)
{
	// 기본적으로 효과 없음 (오버라이드하여 구현)
}

EHTNTaskStatus UHTNTask::Execute_Implementation(AAIController* Controller, float DeltaTime)
{
	// 기본 구현은 바로 성공 (오버라이드하여 구현)
	return EHTNTaskStatus::Succeeded;
}

void UHTNTask::Initialize()
{
	// 초기화 로직
}

FString UHTNTask::GetDebugInfo() const
{
	return FString::Printf(TEXT("%s (Type: %s, Cost: %.2f)"), 
		*TaskName, 
		TaskType == EHTNTaskType::Primitive ? TEXT("Primitive") : TEXT("Compound"), 
		Cost);
}