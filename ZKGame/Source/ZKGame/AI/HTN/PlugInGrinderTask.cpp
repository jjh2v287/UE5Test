// PlugInGrinderTask.cpp
#include "PlugInGrinderTask.h"

#include "GameFramework/Character.h"

UPlugInGrinderTask::UPlugInGrinderTask()
{
	TaskName = TEXT("Plug In Grinder");
	ExecutionTime = 2.0f; // 2초 소요
}

bool UPlugInGrinderTask::CheckPreconditions_Implementation(const FCoffeeWorldState& WorldState)
{
	// 그라인더가 이미 연결되어 있지 않아야 함
	return !WorldState.bIsGrinderPluggedIn;
}

void UPlugInGrinderTask::ApplyEffects_Implementation(FCoffeeWorldState& WorldState)
{
	// 그라인더 연결 상태로 변경
	WorldState.bIsGrinderPluggedIn = true;
}

void UPlugInGrinderTask::OnTaskStart_Implementation(AAIController* Controller)
{
	Super::OnTaskStart_Implementation(Controller);
    
	// AI 캐릭터에서 플러그인 애니메이션 재생
	if (Controller && Controller->GetPawn())
	{
		ACharacter* Character = Cast<ACharacter>(Controller->GetPawn());
		if (Character)
		{
			// Character->PlayAnimMontage(Character->PlugInGrinderMontage);
			// 디버그 메시지 출력
			UE_LOG(LogTemp, Display, TEXT("AI: 그라인더 플러그 연결 중..."));
		}
	}
}