// GrindBeansTask.cpp
#include "GrindBeansTask.h"

#include "GameFramework/Character.h"

UGrindBeansTask::UGrindBeansTask()
{
	TaskName = TEXT("Grind Beans");
	ExecutionTime = 4.0f; // 4초 소요
}

bool UGrindBeansTask::CheckPreconditions_Implementation(const FCoffeeWorldState& WorldState)
{
	// 원두가 있고, 그라인더가 준비되어 있어야 함
	return WorldState.bHaveBeans && WorldState.bIsGrinderReady;
}

void UGrindBeansTask::ApplyEffects_Implementation(FCoffeeWorldState& WorldState)
{
	// 원두는 소비되고, 그라운드 커피가 생성됨
	WorldState.bHaveBeans = false;
	WorldState.bHaveGroundCoffee = true;
}

void UGrindBeansTask::OnTaskStart_Implementation(AAIController* Controller)
{
	Super::OnTaskStart_Implementation(Controller);
    
	// AI 캐릭터에서 그라인딩 애니메이션 재생
	if (Controller && Controller->GetPawn())
	{
		ACharacter* Character = Cast<ACharacter>(Controller->GetPawn());
		if (Character)
		{
			// Character->PlayAnimMontage(Character->GrindBeansMontage);
			// 그라인더 사운드 재생
			// Character->PlayGrinderSound();
			// 디버그 메시지 출력
			UE_LOG(LogTemp, Display, TEXT("AI: 원두 그라인딩 중..."));
		}
	}
}