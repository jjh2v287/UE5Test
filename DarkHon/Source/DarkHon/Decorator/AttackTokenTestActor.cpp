// Fill out your copyright notice in the Description page of Project Settings.


#include "AttackTokenTestActor.h"


// Sets default values
AAttackTokenTestActor::AAttackTokenTestActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AAttackTokenTestActor::BeginPlay()
{
	Super::BeginPlay();
	// 사용 FSword FFireWeapon
	TSharedPtr<IWeapon> Weapon =
		MakeShared<FFireWeapon>(   // 화염
			MakeShared<FPoisonWeapon>( // +독
				MakeShared<FSword>() ) ); // 기본검

	// 출력: "기본 검+독+화염 : 140"
	UE_LOG(LogTemp, Log, TEXT("%s : %f"), *Weapon->GetName(), Weapon->GetDamage());
}

// Called every frame
void AAttackTokenTestActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

