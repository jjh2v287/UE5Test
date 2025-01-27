// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "ZKGameplayAbilityBase.h"
#include "ZKCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UZKAbilitySystemComponent;
struct FInputActionValue;

UCLASS(Blueprintable)
class ZKGAME_API AZKCharacter : public ACharacter, public IAbilitySystemInterface 
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	explicit AZKCharacter(const FObjectInitializer& ObjectInitializer);

	void BeginPlay() override;

	/** Abilities to grant to this character on creation. These will be activated by tag or event and are not bound to specific inputs */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Abilities)
	TArray<TSubclassOf<UGameplayAbility>> GameplayAbilities;

	/** Passive gameplay effects applied on creation */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Abilities)
	TArray<TSubclassOf<UGameplayEffect>> PassiveGameplayEffects;

	/** List of attributes modified by the ability system */
	UPROPERTY()
	UAttributeSet* AttributeSet;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ZK", meta = (AllowPrivateAccess = "true"))
	UZKAbilitySystemComponent* AbilitySystemComponent;

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ZK", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ZK", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ZK", meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ZK", meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ZK", meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ZK", meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ZK", meta = (AllowPrivateAccess = "true"))
	UInputAction* RollAction;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ZK", meta = (AllowPrivateAccess = "true"))
	UInputAction* AttackAction;

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	UFUNCTION(BlueprintImplementableEvent, Category = "ZK")
	void OnRollEvent();

	UFUNCTION(BlueprintImplementableEvent, Category = "ZK")
	void OnAttackEvent();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:
	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	/** Called for looking input */
	void Roll();

	/** Called for looking input */
	void Attack();

	/* StartupGameplayAbilities */
	void AddStartupGameplayAbilities();
			
	virtual void NotifyControllerChanged() override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	
};
