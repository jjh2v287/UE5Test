// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AttackTokenTestActor.generated.h"

// 공통 인터페이스: 공격 조건
class IAttackCondition
{
public:
	virtual ~IAttackCondition() = default;
	// 조건이 충족되었는지 여부를 bool로 반환합니다.
	virtual bool IsSatisfied() const = 0;
	virtual FString GetDescription() const = 0;
};

// 기본 조건: 공격 토큰 보유 여부
class FHasAttackTokenCondition : public IAttackCondition
{
public:
	virtual bool IsSatisfied() const override
	{
		// TODO: 실제 토큰 보유 여부 검사 로직
		return true; // 토큰이 있다고 가정
	}
	
	virtual FString GetDescription() const override
	{
		return TEXT("토큰 보유");
	}
};

// Decorator 베이스
class FAttackConditionDecorator : public IAttackCondition
{
protected:
	TSharedPtr<IAttackCondition> InnerCondition;
public:
	explicit FAttackConditionDecorator(TSharedPtr<IAttackCondition> InCondition)
		: InnerCondition(MoveTemp(InCondition))
	{}
};

// Concrete Decorators
class FAngleConditionDecorator : public FAttackConditionDecorator
{
public:
	using FAttackConditionDecorator::FAttackConditionDecorator;
	virtual bool IsSatisfied() const override
	{
		// TODO: 실제 각도 검사 로직
		bool bIsInAngle = true; // 각도 내에 있다고 가정
		
		// 내부 조건과 현재 조건이 모두 참이어야 함 (논리 AND)
		return InnerCondition->IsSatisfied() && bIsInAngle;
	}
	
	virtual FString GetDescription() const override
	{
		return InnerCondition->GetDescription() + TEXT(" + 각도 조건");
	}
};

class FDistanceConditionDecorator : public FAttackConditionDecorator
{
public:
	using FAttackConditionDecorator::FAttackConditionDecorator;
	virtual bool IsSatisfied() const override
	{
		// TODO: 실제 거리 검사 로직
		bool bIsInDistance = true; // 거리 내에 있다고 가정

		// 내부 조건과 현재 조건이 모두 참이어야 함 (논리 AND)
		return InnerCondition->IsSatisfied() && bIsInDistance;
	}
	
	virtual FString GetDescription() const override
	{
		return InnerCondition->GetDescription() + TEXT(" + 거리 조건");
	}
};

UCLASS()
class DARKHON_API AAttackTokenTestActor : public AActor
{
	GENERATED_BODY()
public:
	AAttackTokenTestActor();

protected:
	virtual void BeginPlay() override;
};
