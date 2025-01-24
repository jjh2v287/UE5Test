// Fill out your copyright notice in the Description page of Project Settings.


#include "ZKCharacter.h"

// Sets default values
AZKCharacter::AZKCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// GetCapsuleComponent()->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.0f));
	// UKCharacterMovementComponent = Cast<UUKCharacterMovementComponent>(GetCharacterMovement());
	// UKSkeletalMeshComponent = Cast<UUKCharacterSkeletalMeshComponent>(GetMesh());
	// AnimInstance = Cast<UUKCharacterAnimInstance>(GetMesh()->GetAnimInstance());
	// CapsuleModifierComponent = CreateDefaultSubobject<UUKCapsuleModifierComponent>(TEXT("CapsuleModifierComponent"));
	// AbilitySystemComponent = CreateOptionalDefaultSubobject<UUKAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
}

// Called when the game starts or when spawned
void AZKCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AZKCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AZKCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

