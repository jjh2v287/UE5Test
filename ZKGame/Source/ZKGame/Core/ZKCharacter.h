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
    // Constructor
    explicit AZKCharacter(const FObjectInitializer& ObjectInitializer);

    // Override functions
    virtual void Tick(float DeltaSeconds) override;
    virtual void BeginPlay() override;
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    // Gameplay Ability System Properties
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Abilities)
    TArray<TSubclassOf<UGameplayAbility>> GameplayAbilities;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Abilities)
    TArray<TSubclassOf<UGameplayEffect>> PassiveGameplayEffects;

    UPROPERTY()
    UAttributeSet* AttributeSet;

    // Component Properties
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ZK", meta = (AllowPrivateAccess = "true"))
    UZKAbilitySystemComponent* AbilitySystemComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ZK", meta = (AllowPrivateAccess = "true"))
    USpringArmComponent* CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ZK", meta = (AllowPrivateAccess = "true"))
    UCameraComponent* FollowCamera;

    // Input Properties
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ZK", meta = (AllowPrivateAccess = "true"))
    UInputMappingContext* DefaultMappingContext;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ZK", meta = (AllowPrivateAccess = "true"))
    UInputAction* MoveAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ZK", meta = (AllowPrivateAccess = "true"))
    UInputAction* LookAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ZK", meta = (AllowPrivateAccess = "true"))
    UInputAction* JumpAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ZK", meta = (AllowPrivateAccess = "true"))
    UInputAction* RollAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ZK", meta = (AllowPrivateAccess = "true"))
    UInputAction* AttackAction;

    // Getter Functions
    FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
    FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

    // Blueprint Events
    UFUNCTION(BlueprintImplementableEvent, Category = "ZK")
    void OnRollEvent();

    UFUNCTION(BlueprintImplementableEvent, Category = "ZK")
    void OnAttackEvent();

protected:
    // Input Functions
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void Roll();
    void Attack();

    // Override Functions
    virtual void NotifyControllerChanged() override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    // Gameplay Ability Functions
    void AddStartupGameplayAbilities();
    
    // Rotation Function
    void CalculateRotationRate();

private:
    // Rotation Properties
    FRotator TargetRotation;
    bool bIsRotating;
    float CurrentRotationRate;
};