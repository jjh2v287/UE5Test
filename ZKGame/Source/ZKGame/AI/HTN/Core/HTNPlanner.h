// CoffeeHTNPlanner.h - HTN 계획 수립기

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "HTNTypes.h"
#include "HTNTask.h"
#include "Templates/SubclassOf.h"
#include "HTNPlanner.generated.h"

// HTN 계획 수립기 클래스
UCLASS(BlueprintType, Blueprintable)
class ZKGAME_API UHTNPlanner : public UObject
{
	GENERATED_BODY()
    
public:
	// 루트 태스크 클래스 (목표)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HTN", meta = (DisplayName = "루트 태스크"))
	TSubclassOf<UHTNTask> RootTaskClass;
    
	// 루트 태스크 인스턴스
	UPROPERTY(BlueprintReadOnly, Category = "HTN")
	UHTNTask* RootTask;
    
	// 현재 월드 상태
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HTN", meta = (DisplayName = "월드 상태"))
	FCoffeeWorldState WorldState;
    
	// 계획 수립 결과 (원시 태스크 목록)
	UPROPERTY(BlueprintReadOnly, Category = "HTN")
	TArray<UHTNTask*> Plan;
    
	// 계획 수립 함수
	UFUNCTION(BlueprintCallable, Category = "HTN", meta = (DisplayName = "계획 수립"))
	bool CreatePlan();
    
	// 플래너 초기화
	UFUNCTION(BlueprintCallable, Category = "HTN", meta = (DisplayName = "초기화"))
	void Initialize();
    
	// 현재 계획 취득
	UFUNCTION(BlueprintCallable, Category = "HTN", meta = (DisplayName = "계획 취득"))
	TArray<UHTNTask*> GetPlan() const;
    
	// 계획 단계별 디버깅 정보 출력
	UFUNCTION(BlueprintCallable, Category = "HTN", meta = (DisplayName = "계획 디버그 출력"))
	void DebugPrintPlan() const;
    
private:
	// 재귀적으로 태스크 분해하는 내부 함수
	bool DecomposeTask(UHTNTask* Task, FCoffeeWorldState CurrentState, 
					   TArray<UHTNTask*>& OutPlan, float& OutTotalCost);
};