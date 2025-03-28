// CoffeeHTNPlanner.cpp - 구현 파일

#include "HTNPlanner.h"
#include "HTNCompoundTask.h"
#include "HTNPrimitiveTask.h"

void UHTNPlanner::Initialize()
{
    // 루트 태스크 인스턴스 생성
    if (RootTaskClass)
    {
        RootTask = NewObject<UHTNTask>(this, RootTaskClass);
        if (RootTask)
        {
            RootTask->Initialize();
        }
    }
    
    // 계획 초기화
    Plan.Empty();
}

bool UHTNPlanner::CreatePlan()
{
    Plan.Empty();
    
    // 루트 태스크가 없으면 실패
    if (!RootTask)
    {
        UE_LOG(LogTemp, Error, TEXT("HTN: 루트 태스크가 설정되지 않음"));
        return false;
    }
    
    // 계획 비용 (최적화에 사용)
    float TotalCost = 0.0f;
    
    // 계획 수립 시작
    UE_LOG(LogTemp, Display, TEXT("HTN: '%s' 목표로 계획 수립 시작"), *RootTask->TaskName);
    
    bool bSuccess = DecomposeTask(RootTask, WorldState, Plan, TotalCost);
    
    if (bSuccess)
    {
        UE_LOG(LogTemp, Display, TEXT("HTN: 계획 수립 성공 (%d 단계, 비용: %.2f)"), Plan.Num(), TotalCost);
        return true;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("HTN: 계획 수립 실패"));
        Plan.Empty();
        return false;
    }
}

bool UHTNPlanner::DecomposeTask(UHTNTask* Task, FCoffeeWorldState CurrentState, 
                                     TArray<UHTNTask*>& OutPlan, float& OutTotalCost)
{
    if (!Task)
    {
        return false;
    }
    
    // 태스크 조건 확인
    if (!Task->CheckPreconditions(CurrentState))
    {
        UE_LOG(LogTemp, Warning, TEXT("HTN: '%s' 태스크의 선행 조건 미충족"), *Task->TaskName);
        return false;
    }
    
    // 태스크 유형에 따라 처리
    if (Task->TaskType == EHTNTaskType::Primitive)
    {
        // 원시 태스크는 계획에 직접 추가
        UHTNPrimitiveTask* PrimitiveTask = Cast<UHTNPrimitiveTask>(Task);
        if (PrimitiveTask)
        {
            OutPlan.Add(PrimitiveTask);
            OutTotalCost += PrimitiveTask->Cost;
            
            // 상태 업데이트 (시뮬레이션)
            FCoffeeWorldState UpdatedState = CurrentState;
            PrimitiveTask->ApplyEffects(UpdatedState);
            
            UE_LOG(LogTemp, Display, TEXT("HTN: 계획에 원시 태스크 추가: '%s'"), *PrimitiveTask->TaskName);
            return true;
        }
    }
    else if (Task->TaskType == EHTNTaskType::Compound)
    {
        // 복합 태스크는 하위 태스크로 분해
        UHTNCompoundTask* CompoundTask = Cast<UHTNCompoundTask>(Task);
        if (CompoundTask)
        {
            UE_LOG(LogTemp, Display, TEXT("HTN: 복합 태스크 분해: '%s'"), *CompoundTask->TaskName);
            
            // 최적의 분해 방법 선택
            TArray<UHTNTask*> SubTasks = CompoundTask->FindBestDecomposition(CurrentState);
            
            if (SubTasks.Num() == 0)
            {
                UE_LOG(LogTemp, Warning, TEXT("HTN: '%s' 태스크에 적용 가능한 메서드 없음"), *CompoundTask->TaskName);
                return false;
            }
            
            // 임시 계획 및 비용
            TArray<UHTNTask*> TempPlan;
            float TempCost = 0.0f;
            
            // 현재 상태의 복사본
            FCoffeeWorldState StateSnapshot = CurrentState;
            
            // 각 하위 태스크 처리
            for (UHTNTask* SubTask : SubTasks)
            {
                if (!DecomposeTask(SubTask, StateSnapshot, TempPlan, TempCost))
                {
                    UE_LOG(LogTemp, Warning, TEXT("HTN: 하위 태스크 '%s' 처리 실패"), *SubTask->TaskName);
                    return false;
                }
                
                // 상태 업데이트
                if (SubTask->TaskType == EHTNTaskType::Primitive)
                {
                    UHTNPrimitiveTask* PrimSubTask = Cast<UHTNPrimitiveTask>(SubTask);
                    if (PrimSubTask)
                    {
                        PrimSubTask->ApplyEffects(StateSnapshot);
                    }
                }
            }
            
            // 임시 계획을 실제 계획에 추가
            OutPlan.Append(TempPlan);
            OutTotalCost += TempCost;
            
            return true;
        }
    }
    
    return false;
}

TArray<UHTNTask*> UHTNPlanner::GetPlan() const
{
    return Plan;
}

void UHTNPlanner::DebugPrintPlan() const
{
    UE_LOG(LogTemp, Display, TEXT("==== HTN 계획 디버그 출력 ===="));
    
    if (Plan.Num() == 0)
    {
        UE_LOG(LogTemp, Display, TEXT("계획이 비어 있습니다."));
        return;
    }
    
    for (int32 i = 0; i < Plan.Num(); i++)
    {
        if (Plan[i])
        {
            UE_LOG(LogTemp, Display, TEXT("단계 %d: %s"), i + 1, *Plan[i]->TaskName);
        }
    }
    
    UE_LOG(LogTemp, Display, TEXT("==== 계획 끝 (%d 단계) ===="), Plan.Num());
}