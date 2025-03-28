// CoffeeHTNCompoundTask.h - 복합 태스크 클래스

#pragma once

#include "CoreMinimal.h"
#include "HTNTask.h"
#include "HTNMethod.h"
#include "Templates/SubclassOf.h"
#include "HTNCompoundTask.generated.h"

// 복합 태스크 - 하위 태스크로 분해되는 태스크
UCLASS(Blueprintable)
class ZKGAME_API UHTNCompoundTask : public UHTNTask
{
	GENERATED_BODY()
    
public:
	UHTNCompoundTask();
    
	// 이 태스크를 분해하는 메서드들
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HTN", meta = (DisplayName = "분해 메서드"))
	TArray<TSubclassOf<UHTNMethod>> DecompositionMethodClasses;
    
	// 실제 메서드 인스턴스들
	UPROPERTY(BlueprintReadOnly, Category = "HTN")
	TArray<UHTNMethod*> DecompositionMethods;
    
	// 최적의 메서드를 선택하고 하위 태스크 목록 반환
	UFUNCTION(BlueprintCallable, Category = "HTN")
	TArray<UHTNTask*> FindBestDecomposition(const FCoffeeWorldState& WorldState);
    
	// 초기화 시 메서드 인스턴스 생성
	virtual void Initialize() override;
    
	// 디버그 정보
	virtual FString GetDebugInfo() const override;
};