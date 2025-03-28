// CoffeeHTNMethod.cpp - 구현 파일

#include "HTNMethod.h"

bool UHTNMethod::CheckPreconditions_Implementation(const FCoffeeWorldState& WorldState)
{
	// 기본 구현은 항상 적용 가능 (하위 클래스에서 오버라이드)
	return true;
}

void UHTNMethod::Initialize()
{
	// 하위 태스크 인스턴스 생성
	SubTasks.Empty();
	for (TSubclassOf<UHTNTask> TaskClass : SubTaskClasses)
	{
		if (TaskClass)
		{
			UHTNTask* Task = NewObject<UHTNTask>(this, TaskClass);
			if (Task)
			{
				Task->Initialize();
				SubTasks.Add(Task);
			}
		}
	}
}

FString UHTNMethod::GetDebugInfo() const
{
	FString Info = MethodName + FString::Printf(TEXT(" (우선순위: %d)"), Priority);
    
	if (SubTasks.Num() > 0)
	{
		Info += TEXT("\n      하위 태스크:");
		for (UHTNTask* Task : SubTasks)
		{
			if (Task)
			{
				Info += TEXT("\n      - ") + Task->TaskName;
			}
		}
	}
    
	return Info;
}