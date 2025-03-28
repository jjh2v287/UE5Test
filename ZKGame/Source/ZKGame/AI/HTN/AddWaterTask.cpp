// Fill out your copyright notice in the Description page of Project Settings.

#include "AddWaterTask.h"

#include "GameFramework/Character.h"

UAddWaterTask::UAddWaterTask()
{
	TaskName = TEXT("물 추가");
	ExecutionTime = 2.5f;
}

bool UAddWaterTask::CheckPreconditions_Implementation(const FCoffeeWorldState& WorldState)
{
	// 물이 있고 원두 가루가 필터에 있어야 함
	return WorldState.bHaveWater && WorldState.bAreGroundsInFilter;
}

void UAddWaterTask::ApplyEffects_Implementation(FCoffeeWorldState& WorldState)
{
	// 물 사용 및 머신 준비 완료
	WorldState.bHaveWater = false;
	WorldState.bIsMachineReady = true;
}

void UAddWaterTask::OnTaskStart_Implementation(AAIController* Controller)
{
	Super::OnTaskStart_Implementation(Controller);
    
	// AI 캐릭터 애니메이션
	if (Controller && Controller->GetPawn())
	{
		ACharacter* Character = Cast<ACharacter>(Controller->GetPawn());
		if (Character)
		{
			// Character->PlayAnimMontage(Character->AddWaterMontage);
			UE_LOG(LogTemp, Display, TEXT("AI: 물 붓는 중..."));
		}
	}
}