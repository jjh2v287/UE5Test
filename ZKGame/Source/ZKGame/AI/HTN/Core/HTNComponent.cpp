// CoffeeHTNExecutorComponent.cpp - 구현 파일

#include "HTNComponent.h"
#include "AIController.h"
#include "HTNPrimitiveTask.h"
#include "DrawDebugHelpers.h"

UHTNComponent::UHTNComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    bIsExecutingPlan = false;
    CurrentTaskIndex = 0;
    bEnableDebugDisplay = true;
}

void UHTNComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // 소유자인 AI 컨트롤러 레퍼런스 저장
    OwnerController = Cast<AAIController>(GetOwner());
    
    // 플래너가 설정되어 있다면 초기화
    if (Planner)
    {
        Planner->Initialize();
    }
    else
    {
        // PlannerClass를 사용하여 플래너 인스턴스 생성
        if (PlannerClass)
        {
            Planner = NewObject<UHTNPlanner>(this, PlannerClass);
            if (Planner)
            {
                Planner->Initialize(); // 초기화 함수가 있다면 호출
            }
        }
        else
        {
            // 플래너 인스턴스 생성
            Planner = NewObject<UHTNPlanner>(this);
            if (Planner)
            {
                Planner->Initialize();
            }
        }
    }
}

void UHTNComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    if (!bIsExecutingPlan || !Planner)
    {
        return;
    }
    
    // 현재 태스크 실행
    if (CurrentPlan.Num() > 0 && CurrentTaskIndex < CurrentPlan.Num())
    {
        ExecuteCurrentTask(DeltaTime);
    }
    else if (CurrentTaskIndex >= CurrentPlan.Num())
    {
        // 모든 태스크가 완료됨
        UE_LOG(LogTemp, Display, TEXT("HTN: 계획 실행 완료"));
        bIsExecutingPlan = false;
        
        // 이벤트 핸들러 호출 가능
    }
    
    // 디버그 정보 표시
    if (bEnableDebugDisplay)
    {
        DrawDebugStatus();
    }
}

void UHTNComponent::SetPlan(const TArray<UHTNTask*>& NewPlan)
{
    CurrentPlan = NewPlan;
    CurrentTaskIndex = 0;
    
    // 계획이 있다면 실행 시작
    bIsExecutingPlan = (CurrentPlan.Num() > 0);
    
    if (bIsExecutingPlan)
    {
        UE_LOG(LogTemp, Display, TEXT("HTN: 새 계획 실행 시작 (%d 단계)"), CurrentPlan.Num());
    }
}

bool UHTNComponent::ReplanIfNeeded()
{
    if (!Planner)
    {
        return false;
    }
    
    // 실제 월드 상태 업데이트
    UpdateWorldState();
    
    // 새로운 계획 수립
    if (Planner->CreatePlan())
    {
        // 새 계획 설정
        SetPlan(Planner->GetPlan());
        return true;
    }
    
    return false;
}

void UHTNComponent::SetPlanExecution(bool bExecute)
{
    bIsExecutingPlan = bExecute;
    
    if (bExecute)
    {
        UE_LOG(LogTemp, Display, TEXT("HTN: 계획 실행 재개"));
    }
    else
    {
        UE_LOG(LogTemp, Display, TEXT("HTN: 계획 실행 일시 정지"));
    }
}

void UHTNComponent::ExecuteCurrentTask(float DeltaTime)
{
    UHTNPrimitiveTask* CurrentTask = Cast<UHTNPrimitiveTask>(CurrentPlan[CurrentTaskIndex]);
    if (!CurrentTask || !OwnerController)
    {
        return;
    }
    
    // 태스크 실행
    EHTNTaskStatus Status = CurrentTask->Execute(OwnerController, DeltaTime);
    
    // 완료 또는 실패한 경우 다음 태스크로 이동
    if (Status == EHTNTaskStatus::Succeeded)
    {
        UE_LOG(LogTemp, Display, TEXT("HTN: 태스크 완료: '%s'"), *CurrentTask->TaskName);
        
        // 효과 적용
        CurrentTask->ApplyEffects(Planner->WorldState);
        
        // 다음 태스크로 이동
        CurrentTaskIndex++;
    }
    else if (Status == EHTNTaskStatus::Failed)
    {
        UE_LOG(LogTemp, Warning, TEXT("HTN: 태스크 실패: '%s', 재계획 필요"), *CurrentTask->TaskName);
        
        // 계획 다시 수립
        ReplanIfNeeded();
    }
}

void UHTNComponent::UpdateWorldState()
{
    if (!Planner)
    {
        return;
    }
    
    // 여기서는 게임 상태에서 플래너의 월드 상태로 데이터를 복사해야 함
    // 예를 들어, 캐릭터 인벤토리, 객체 상태 등을 검사하여 업데이트
    
    // 예시 구현:
    // ACoffeeSimCharacter* Character = Cast<ACoffeeSimCharacter>(OwnerController->GetPawn());
    // if (Character)
    // {
    //     Planner->WorldState.bHaveBeans = Character->bHasBeans;
    //     Planner->WorldState.bHaveWater = Character->bHasWater;
    //     // 등등...
    // }
    
    // 커피 만들기 장소에 있는지 검사
    // Planner->WorldState.bIsNearCoffeeMachine = ...
    
    // 현재 단순화를 위해 주석 처리됨
}

void UHTNComponent::DrawDebugStatus() const
{
    if (!OwnerController || !OwnerController->GetPawn())
    {
        return;
    }
    
    FVector PawnLocation = OwnerController->GetPawn()->GetActorLocation();
    PawnLocation.Z += 200.0f; // 캐릭터 머리 위에 표시
    
    // 현재 계획 상태 텍스트
    FString StatusText;
    if (bIsExecutingPlan && CurrentPlan.Num() > 0 && CurrentTaskIndex < CurrentPlan.Num())
    {
        StatusText = FString::Printf(TEXT("실행 중: %s (%d/%d)"), 
            *CurrentPlan[CurrentTaskIndex]->TaskName, 
            CurrentTaskIndex + 1, 
            CurrentPlan.Num());
    }
    else if (CurrentTaskIndex >= CurrentPlan.Num() && CurrentPlan.Num() > 0)
    {
        StatusText = TEXT("계획 완료");
    }
    else
    {
        StatusText = TEXT("계획 없음");
    }
    
    // 상태 텍스트 표시
    DrawDebugString(
        GetWorld(),
        PawnLocation,
        StatusText,
        nullptr,
        FColor::Yellow,
        0.0f,
        true
    );
}