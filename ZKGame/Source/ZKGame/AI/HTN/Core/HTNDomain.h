// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HTNCompoundTask.h"
#include "Engine/DataAsset.h"
#include "Templates/SubclassOf.h"
#include "HTNDomain.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class ZKGAME_API UHTNDomain : public UObject
{
	GENERATED_BODY()
public:
	UHTNDomain(const FObjectInitializer& ObjectInitializer);

	// 도메인 이름 (식별용)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HTN Domain")
	FName DomainName;

	// 이 도메인의 루트 태스크 타입 (계획 수립의 시작점)
	// 일반적으로 복합 태스크여야 합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HTN Domain")
	TSubclassOf<UHTNCompoundTask> RootTaskClass;

	/*
	 * 도메인에서 이 설정을 해줘야한다 음... 나중에 비쥬얼 그래프로 해야할둣
	 *
	 *                          RootTask(CompoundTask)
	 *                                   /\
	 *                                  /  \
	 *                                 /    \
	 *                Method(컨디션 1)        Method(컨디션 2)
	 *                    |                    /\          \
	 *                    |                   /  \          \
	 *                    |                  /    \          \
	 *        TaskA(PrimitiveTask)    TaskB(CompoundTask)    TaskC(PrimitiveTask)
	 *                                     /\
	 *                                    /  \
	 *                                   /    \
	 *                      Method(컨디션 3)    Method(컨디션 4)
	 *                          |                  |
	 *                          |                  |
	 *                TaskD(PrimitiveTask)    TaskE(PrimitiveTask)
	 *
	 * HTN 계층 구조 설명:
	 * 1. RootTask는 최상위 복합 태스크로, 두 가지 메서드로 분해될 수 있습니다.
	 * 2. Method(컨디션 1)은 단일 원시 태스크 TaskA로 분해됩니다.
	 * 3. Method(컨디션 2)는 복합 태스크 TaskB와 원시 태스크 TaskC로 분해됩니다.
	 * 4. TaskB는 다시 두 가지 메서드로 분해될 수 있습니다:
	 *    - Method(컨디션 3)은 TaskD(원시 태스크)로 분해됩니다.
	 *    - Method(컨디션 4)는 TaskE(원시 태스크)로 분해됩니다.
	 * 
	 * 계획 수립 과정:
	 * 1. RootTask에서 시작하여 현재 월드 상태에 적용 가능한 메서드를 선택합니다.
	 * 2. 선택된 메서드의 하위 태스크를 순차적으로 평가합니다.
	 * 3. 복합 태스크를 만나면 재귀적으로 분해 과정을 반복합니다.
	 * 4. 최종적으로 실행 가능한 원시 태스크의 시퀀스가 계획으로 생성됩니다.
	 * 
	 * 예시 계획 1: 컨디션 1이 참인 경우 → [TaskA]
	 * 예시 계획 2: 컨디션 2와 컨디션 3이 참인 경우 → [TaskD, TaskC]
	 * 예시 계획 3: 컨디션 2와 컨디션 4가 참인 경우 → [TaskE, TaskC]
	 */

	// 도메인에서 사용 가능한 모든 태스크 정의
	// 플래너가 태스크 클래스로부터 인스턴스를 생성할 때 참조할 수 있습니다.
	// 또는, 태스크 정의를 별도의 에셋으로 관리하고 여기서 참조할 수도 있습니다.
	// 여기서는 루트 태스크만 정의하고, 실제 태스크들은 CompoundTask 내부의 Method와 SubTask 정의를 통해 참조됩니다.
	// 만약 특정 태스크(예: 모든 Primitive Task) 목록이 필요하다면 여기에 추가:
	// UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HTN Domain", meta = (MustImplement = "/Script/YourProjectName.HTNPrimitiveTask"))
	// TArray<TSubclassOf<UHTNPrimitiveTask>> AvailablePrimitiveTasks;

	// UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HTN Domain", meta = (MustImplement = "/Script/YourProjectName.HTNCompoundTask"))
	// TArray<TSubclassOf<UHTNCompoundTask>> AvailableCompoundTasks;

	// UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HTN Domain", meta = (MustImplement = "/Script/YourProjectName.HTNCompoundTask"))
	// TArray<TSubclassOf<UHTNMethod>> AvailableMethods;

	// 도메인 설명
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HTN Domain", meta = (MultiLine = true))
	FText Description;
};
