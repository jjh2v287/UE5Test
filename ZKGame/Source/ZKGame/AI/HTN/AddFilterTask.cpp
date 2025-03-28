// Fill out your copyright notice in the Description page of Project Settings.

#include "AddFilterTask.h"
#include "GameFramework/Character.h"

UAddFilterTask::UAddFilterTask()
{
	TaskName = TEXT("필터 추가");
	ExecutionTime = 2.0f;
}

bool UAddFilterTask::CheckPreconditions_Implementation(const FCoffeeWorldState& WorldState)
{
	// 필터가 있어야 함
	return WorldState.bHaveFilter;
}

void UAddFilterTask::ApplyEffects_Implementation(FCoffeeWorldState& WorldState)
{
	// 필터 사용 및 장착
	WorldState.bHaveFilter = false;
	WorldState.bIsFilterInPlace = true;
}

void UAddFilterTask::OnTaskStart_Implementation(AAIController* Controller)
{
	Super::OnTaskStart_Implementation(Controller);
    
	// AI 캐릭터 애니메이션
	if (Controller && Controller->GetPawn())
	{
		ACharacter* Character = Cast<ACharacter>(Controller->GetPawn());
		if (Character)
		{
			// Character->PlayAnimMontage(Character->AddFilterMontage);
			UE_LOG(LogTemp, Display, TEXT("AI: 필터 장착 중..."));
		}
	}
}