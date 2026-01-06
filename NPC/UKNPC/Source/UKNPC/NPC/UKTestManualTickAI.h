// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UKNPCUpdateInterface.h"
#include "UKSimpleMovementComponent.h"
#include "GameFramework/Character.h"
#include "UKTestManualTickAI.generated.h"

UCLASS()
class UKNPC_API AUKTestManualTickAI : public ACharacter, public IUKNPCUpdateInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AUKTestManualTickAI();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// 인터페이스 구현
	virtual void ManualTick(float DeltaTime) override;
	
	UPROPERTY(Transient)
	UUKSimpleMovementComponent* SMComponent;
};
