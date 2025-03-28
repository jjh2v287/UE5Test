// Fill out your copyright notice in the Description page of Project Settings.

#include "CleanGrinderTask.h"

#include "GameFramework/Character.h"

UCleanGrinderTask::UCleanGrinderTask()
{
	TaskName = TEXT("그라인더 청소");
	ExecutionTime = 3.0f;
}

bool UCleanGrinderTask::CheckPreconditions_Implementation(const FCoffeeWorldState& WorldState)
{
	// 그라인더가 연결되어 있어야 함
	return WorldState.bIsGrinderPluggedIn;
}

void UCleanGrinderTask::ApplyEffects_Implementation(FCoffeeWorldState& WorldState)
{
	// 그라인더 준비 완료
	WorldState.bIsGrinderReady = true;
}

void UCleanGrinderTask::OnTaskStart_Implementation(AAIController* Controller)
{
	Super::OnTaskStart_Implementation(Controller);
    
	// AI 캐릭터 애니메이션
	if (Controller && Controller->GetPawn())
	{
		ACharacter* Character = Cast<ACharacter>(Controller->GetPawn());
		if (Character)
		{
			// Character->PlayAnimMontage(Character->CleanGrinderMontage);
			UE_LOG(LogTemp, Display, TEXT("AI: 그라인더 청소 중..."));
		}
	}
}