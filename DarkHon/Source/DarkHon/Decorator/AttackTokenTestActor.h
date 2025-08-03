// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AttackTokenTestActor.generated.h"


// 공통 인터페이스
class IWeapon
{
public:
	virtual ~IWeapon() = default;
	virtual float GetDamage() const = 0;
	virtual FString GetName() const = 0;
};

// 기본 무기
class FSword : public IWeapon
{
public:
	virtual float GetDamage() const override { return 100.f; }
	virtual FString GetName()  const override { return TEXT("기본 검"); }
};

// Decorator 베이스
class FWeaponDecorator : public IWeapon
{
protected:
	TSharedPtr<IWeapon> Inner;
public:
	explicit FWeaponDecorator(TSharedPtr<IWeapon> In) : Inner(MoveTemp(In)) {}
};

// Concrete Decorators
class FPoisonWeapon : public FWeaponDecorator
{
public:
	using FWeaponDecorator::FWeaponDecorator;
	virtual float GetDamage() const override { return Inner->GetDamage() + 15.f; }
	virtual FString GetName()  const override { return Inner->GetName() + TEXT("+독"); }
};

class FFireWeapon : public FWeaponDecorator
{
public:
	using FWeaponDecorator::FWeaponDecorator;
	virtual float GetDamage() const override { return Inner->GetDamage() + 25.f; }
	virtual FString GetName()  const override { return Inner->GetName() + TEXT("+화염"); }
};

UCLASS()
class DARKHON_API AAttackTokenTestActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AAttackTokenTestActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
