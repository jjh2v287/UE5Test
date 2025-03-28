// Fill out your copyright notice in the Description page of Project Settings.

#include "AddGroundsTask.h"

#include "GameFramework/Character.h"

UAddGroundsTask::UAddGroundsTask()
{
	TaskName = TEXT("원두 가루 추가");
	ExecutionTime = 3.0f;
}

bool UAddGroundsTask::CheckPreconditions_Implementation(const FCoffeeWorldState& WorldState)
{
	// 필터가 장착되고 그라운드 커피가 있어야 함
	return WorldState.bIsFilterInPlace && WorldState.bHaveGroundCoffee;
}

void UAddGroundsTask::ApplyEffects_Implementation(FCoffeeWorldState& WorldState)
{
	// 그라운드 커피 사용 및 필터에 담기
	WorldState.bHaveGroundCoffee = false;
	WorldState.bAreGroundsInFilter = true;
}

void UAddGroundsTask::OnTaskStart_Implementation(AAIController* Controller)
{
	Super::OnTaskStart_Implementation(Controller);
    
	// AI 캐릭터 애니메이션
	if (Controller && Controller->GetPawn())
	{
		ACharacter* Character = Cast<ACharacter>(Controller->GetPawn());
		if (Character)
		{
			// Character->PlayAnimMontage(Character->AddGroundsMontage);
			UE_LOG(LogTemp, Display, TEXT("AI: 원두 가루 담는 중..."));
		}
	}
}