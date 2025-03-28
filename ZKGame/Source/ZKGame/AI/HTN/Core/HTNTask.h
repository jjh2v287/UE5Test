// CoffeeHTNTask.h - HTN 태스크 기본 클래스

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "HTNTypes.h"
#include "HTNTask.generated.h"

// HTN 태스크의 기본 클래스
UCLASS(BlueprintType, Blueprintable)
class ZKGAME_API UHTNTask : public UObject
{
	GENERATED_BODY()
    
public:
	UHTNTask();

	// 태스크 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HTN")
	FString TaskName;
    
	// 태스크 타입 (원시 또는 복합)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HTN")
	EHTNTaskType TaskType;
    
	// 태스크 비용 (계획 최적화에 사용)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HTN")
	float Cost = 1.0f;
    
	// 태스크의 선행 조건 확인
	UFUNCTION(BlueprintNativeEvent, Category = "HTN")
	bool CheckPreconditions(const FCoffeeWorldState& WorldState);
	virtual bool CheckPreconditions_Implementation(const FCoffeeWorldState& WorldState);
    
	// 태스크 실행 후 월드 상태 업데이트
	UFUNCTION(BlueprintNativeEvent, Category = "HTN")
	void ApplyEffects(UPARAM(ref) FCoffeeWorldState& WorldState);
	virtual void ApplyEffects_Implementation(FCoffeeWorldState& WorldState);
    
	// 태스크 실행 (원시 태스크용)
	UFUNCTION(BlueprintNativeEvent, Category = "HTN")
	EHTNTaskStatus Execute(AAIController* Controller, float DeltaTime);
	virtual EHTNTaskStatus Execute_Implementation(AAIController* Controller, float DeltaTime);
    
	// 태스크 초기화
	UFUNCTION(BlueprintCallable, Category = "HTN")
	virtual void Initialize();
    
	// 태스크 디버그 정보 출력
	UFUNCTION(BlueprintCallable, Category = "HTN")
	virtual FString GetDebugInfo() const;
};