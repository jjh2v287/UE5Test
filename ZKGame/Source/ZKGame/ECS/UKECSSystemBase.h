// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UKECSEntity.h"
#include "UObject/Object.h"
#include "UKECSSystemBase.generated.h"

class UUKECSManager;

/**
 * 
 */
UCLASS()
class ZKGAME_API UUKECSSystemBase : public UObject
{
	GENERATED_BODY()
public:
	/**
	 * @brief 시스템이 처음 등록될 때 호출됩니다.
	 * 필요한 초기화 로직 (예: 쿼리 생성)을 여기에 구현합니다.
	 */
	virtual void Register() {};
	
	/**
	 * @brief 매 프레임 호출되어 시스템 로직을 실행합니다.
	 * @param DeltaTime 프레임 델타 타임.
	 * @param ECSManager 프레임 델타 타임.
	 */
	virtual void Tick(float DeltaTime, UUKECSManager* ECSManager) {};

protected:
	TArray<FECSComponentTypeID> QueryTypes;
};

UCLASS()
class ZKGAME_API UECSMove : public UUKECSSystemBase
{
	GENERATED_BODY()
public:
	virtual void Register() override;
	virtual void Tick(float DeltaTime, UUKECSManager* ECSManager) override;
};