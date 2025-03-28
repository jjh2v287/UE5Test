// CoffeeHTNPrimitiveTask.h - 원시 태스크 클래스

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "HTNTask.h"
#include "HTNPrimitiveTask.generated.h"

// 실행 가능한 원시 태스크 클래스
UCLASS()
class ZKGAME_API UHTNPrimitiveTask : public UHTNTask
{
	GENERATED_BODY()
    
public:
	UHTNPrimitiveTask();
    
	// 태스크 실행 상태
	UPROPERTY(BlueprintReadOnly, Category = "HTN")
	EHTNTaskStatus Status;
    
	// 실행 시간 (초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HTN")
	float ExecutionTime = 1.0f;
    
	// 현재 경과 시간
	UPROPERTY(BlueprintReadOnly, Category = "HTN")
	float ElapsedTime = 0.0f;
    
	// 태스크 실행 로직
	virtual EHTNTaskStatus Execute_Implementation(AAIController* Controller, float DeltaTime) override;
    
	// 태스크 시작 시 호출
	UFUNCTION(BlueprintNativeEvent, Category = "HTN")
	void OnTaskStart(AAIController* Controller);
	virtual void OnTaskStart_Implementation(AAIController* Controller);
    
	// 태스크 완료 시 호출
	UFUNCTION(BlueprintNativeEvent, Category = "HTN")
	void OnTaskComplete(AAIController* Controller, bool bSuccess);
	virtual void OnTaskComplete_Implementation(AAIController* Controller, bool bSuccess);
    
	// 태스크 초기화 (재사용 시)
	virtual void Initialize() override;
};