// Fill out your copyright notice in the Description page of Project Settings.

#include "BrewTask.h"

#include "GameFramework/Character.h"

UBrewTask::UBrewTask()
{
	TaskName = TEXT("커피 추출");
	ExecutionTime = 5.0f; // 더 오래 걸림
}

bool UBrewTask::CheckPreconditions_Implementation(const FCoffeeWorldState& WorldState)
{
	// 머신이 준비되어 있어야 함
	return WorldState.bIsMachineReady;
}

void UBrewTask::ApplyEffects_Implementation(FCoffeeWorldState& WorldState)
{
	// 커피 추출 완료
	WorldState.bHaveBrewedCoffee = true;
	WorldState.bIsMachineReady = false;
	WorldState.bIsFilterInPlace = false;
	WorldState.bAreGroundsInFilter = false;
	WorldState.bIsMachineClean = false; // 이제 머신 청소 필요
}

void UBrewTask::OnTaskStart_Implementation(AAIController* Controller)
{
	Super::OnTaskStart_Implementation(Controller);
    
	// AI 캐릭터 애니메이션
	if (Controller && Controller->GetPawn())
	{
		ACharacter* Character = Cast<ACharacter>(Controller->GetPawn());
		if (Character)
		{
			// Character->PlayAnimMontage(Character->BrewMontage);
			// Character->PlayBrewingSound();
			UE_LOG(LogTemp, Display, TEXT("AI: 커피 추출 중..."));
		}
	}
}

void UBrewTask::OnTaskComplete_Implementation(AAIController* Controller, bool bSuccess)
{
	Super::OnTaskComplete_Implementation(Controller, bSuccess);
    
	if (bSuccess)
	{
		// 완료 알림 또는 효과 (예: 캐릭터가 커피 마시는 애니메이션)
		if (Controller && Controller->GetPawn())
		{
			ACharacter* Character = Cast<ACharacter>(Controller->GetPawn());
			if (Character)
			{
				// Character->PlayCoffeeDoneEffect();
				UE_LOG(LogTemp, Display, TEXT("AI: 커피 제조 완료!"));
			}
		}
	}
}