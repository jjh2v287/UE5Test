// 커피 제조용 HTN 태스크 구현 예제

// MakeCoffeeTask.h - 루트 복합 태스크
#pragma once

#include "CoreMinimal.h"
#include "Core/HTNCompoundTask.h"
#include "MakeCoffeeTask.generated.h"

UCLASS(Blueprintable)
class ZKGAME_API UMakeCoffeeTask : public UHTNCompoundTask
{
	GENERATED_BODY()
    
public:
	UMakeCoffeeTask();
};