// 메서드 구현 - 직접 원두에서 추출
// GrindAndBrewMethod.h
#pragma once

#include "CoreMinimal.h"
#include "Core/HTNMethod.h"
#include "GrindAndBrewMethod.generated.h"

UCLASS(Blueprintable)
class ZKGAME_API UGrindAndBrewMethod : public UHTNMethod
{
	GENERATED_BODY()
    
public:
	UGrindAndBrewMethod();
    
	virtual bool CheckPreconditions_Implementation(const FCoffeeWorldState& WorldState) override;
};