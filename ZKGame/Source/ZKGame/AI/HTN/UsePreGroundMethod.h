// 메서드 구현 - 이미 분쇄된 원두 사용
// UsePreGroundMethod.h
#pragma once

#include "CoreMinimal.h"
#include "Core/HTNMethod.h"
#include "UsePreGroundMethod.generated.h"

UCLASS(Blueprintable)
class ZKGAME_API UUsePreGroundMethod : public UHTNMethod
{
	GENERATED_BODY()
    
public:
	UUsePreGroundMethod();
    
	virtual bool CheckPreconditions_Implementation(const FCoffeeWorldState& WorldState) override;
};
