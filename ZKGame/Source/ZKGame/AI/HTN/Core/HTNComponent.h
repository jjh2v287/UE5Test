// CoffeeHTNExecutorComponent.h - HTN 계획 실행기 컴포넌트

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HTNTask.h"
#include "HTNPlanner.h"
#include "HTNComponent.generated.h"

// HTN 계획 실행기 컴포넌트 - AI 컨트롤러에 추가됨
UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class ZKGAME_API UHTNComponent : public UActorComponent
{
	GENERATED_BODY()
    
public:    
	UHTNComponent();
    
	// 틱에서 계획 실행
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HTN", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UHTNPlanner> PlannerClass;
	
	// HTN 플래너 레퍼런스
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HTN")
	UHTNPlanner* Planner;
    
	// 현재 실행 중인 계획
	UPROPERTY(BlueprintReadOnly, Category = "HTN")
	TArray<UHTNTask*> CurrentPlan;
    
	// 현재 실행 중인 태스크 인덱스
	UPROPERTY(BlueprintReadOnly, Category = "HTN")
	int32 CurrentTaskIndex;
    
	// 새 계획 설정
	UFUNCTION(BlueprintCallable, Category = "HTN")
	void SetPlan(const TArray<UHTNTask*>& NewPlan);
    
	// 계획 다시 수립
	UFUNCTION(BlueprintCallable, Category = "HTN")
	bool ReplanIfNeeded();
    
	// 현재 계획 실행 상태
	UPROPERTY(BlueprintReadOnly, Category = "HTN")
	bool bIsExecutingPlan;
    
	// 계획 실행 일시 정지/재개
	UFUNCTION(BlueprintCallable, Category = "HTN")
	void SetPlanExecution(bool bExecute);
    
	// 디버그 표시 활성화 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HTN")
	bool bEnableDebugDisplay;
    
private:
	// 현재 태스크 실행
	void ExecuteCurrentTask(float DeltaTime);
    
	// 현재 AI 컨트롤러 레퍼런스
	AAIController* OwnerController;
    
	// 월드 상태 업데이트 (실제 게임 상태 반영)
	void UpdateWorldState();
    
	// 디버그 상태 표시
	void DrawDebugStatus() const;
};