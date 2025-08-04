// Fill out your copyright notice in the Description page of Project Settings.

#include "AttackTokenTestActor.h"

AAttackTokenTestActor::AAttackTokenTestActor()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AAttackTokenTestActor::BeginPlay()
{
	Super::BeginPlay();
	
	// 데코레이터 패턴으로 공격 조건 조합
	TSharedPtr<IAttackCondition> AttackConditions =
		MakeShared<FAngleConditionDecorator>(
			MakeShared<FDistanceConditionDecorator>(
				MakeShared<FHasAttackTokenCondition>()
			)
		);

	// 최종 조건 충족 여부 확인
	if (AttackConditions->IsSatisfied())
	{
		// 토큰 보유 + 거리 조건 + 각도 조건 : 공격 가능
		UE_LOG(LogTemp, Log, TEXT("%s : 공격 가능"), *AttackConditions->GetDescription());
	}
	else
	{
		// 토큰 보유 + 거리 조건 + 각도 조건 : 공격 불가능
		UE_LOG(LogTemp, Log, TEXT("%s : 공격 불가능"), *AttackConditions->GetDescription());
	}
	// 토큰 보유 + 거리 조건 + 각도 조건 : 공격 가능
}
