// CoffeeHTNMethod.h - 메서드 클래스

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "HTNTypes.h"
#include "HTNTask.h"
#include "Templates/SubclassOf.h"
#include "HTNMethod.generated.h"

// 메서드 - 태스크를 하위 태스크로 분해하는 방법
UCLASS(Blueprintable)
class ZKGAME_API UHTNMethod : public UObject
{
	GENERATED_BODY()
    
public:
	// 메서드 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HTN", meta = (DisplayName = "메서드 이름"))
	FString MethodName;
    
	// 분해할 하위 태스크 클래스들
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HTN", meta = (DisplayName = "하위 태스크들"))
	TArray<TSubclassOf<UHTNTask>> SubTaskClasses;
    
	// 실제 인스턴스화된 하위 태스크들
	UPROPERTY(BlueprintReadOnly, Category = "HTN")
	TArray<UHTNTask*> SubTasks;
    
	// 이 메서드의 선행 조건 확인
	UFUNCTION(BlueprintNativeEvent, Category = "HTN")
	bool CheckPreconditions(const FCoffeeWorldState& WorldState);
	virtual bool CheckPreconditions_Implementation(const FCoffeeWorldState& WorldState);
    
	// 메서드의 우선순위 (높을수록 먼저 시도)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HTN", meta = (DisplayName = "우선순위"))
	int32 Priority = 0;
    
	// 초기화 - 하위 태스크 인스턴스 생성
	UFUNCTION(BlueprintCallable, Category = "HTN")
	virtual void Initialize();
    
	// 디버그 정보
	UFUNCTION(BlueprintCallable, Category = "HTN")
	virtual FString GetDebugInfo() const;
};