// CoffeeHTNCompoundTask.cpp - 구현 파일

#include "HTNCompoundTask.h"
#include "HTNMethod.h"

UHTNCompoundTask::UHTNCompoundTask()
{
	TaskType = EHTNTaskType::Compound;
}

TArray<UHTNTask*> UHTNCompoundTask::FindBestDecomposition(const FCoffeeWorldState& WorldState)
{
	TArray<UHTNTask*> Result;
    
	// 우선순위에 따라 메서드 정렬
	DecompositionMethods.Sort([](const UHTNMethod& A, const UHTNMethod& B) {
		return A.Priority > B.Priority;
	});
    
	// 적용 가능한 첫 번째 메서드 찾기
	for (UHTNMethod* Method : DecompositionMethods)
	{
		if (Method && Method->CheckPreconditions(WorldState))
		{
			// 디버그 출력
			UE_LOG(LogTemp, Display, TEXT("HTN: '%s' 태스크에 '%s' 메서드 적용"), *TaskName, *Method->MethodName);
			return Method->SubTasks;
		}
	}
    
	// 적용 가능한 메서드가 없으면 빈 배열 반환
	UE_LOG(LogTemp, Warning, TEXT("HTN: '%s' 태스크에 적용 가능한 메서드 없음"), *TaskName);
	return Result;
}

void UHTNCompoundTask::Initialize()
{
	Super::Initialize();
    
	// 메서드 인스턴스 생성
	DecompositionMethods.Empty();
	for (TSubclassOf<UHTNMethod> MethodClass : DecompositionMethodClasses)
	{
		if (MethodClass)
		{
			UHTNMethod* Method = NewObject<UHTNMethod>(this, MethodClass);
			if (Method)
			{
				Method->Initialize();
				DecompositionMethods.Add(Method);
			}
		}
	}
}

FString UHTNCompoundTask::GetDebugInfo() const
{
	FString BaseInfo = Super::GetDebugInfo();
	FString MethodsInfo = TEXT("\n  메서드:");
    
	for (UHTNMethod* Method : DecompositionMethods)
	{
		if (Method)
		{
			MethodsInfo += TEXT("\n    - ") + Method->GetDebugInfo();
		}
	}
    
	return BaseInfo + MethodsInfo;
}