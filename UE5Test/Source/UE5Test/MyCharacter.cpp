// Fill out your copyright notice in the Description page of Project Settings.


#include "MyCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "MyCharacterMovementComponent.h"

// Sets default values
AMyCharacter::AMyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UMyCharacterMovementComponent>(TEXT("MyCharMoveComp")))
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AMyCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	GetCharacterMovement()->SetMovementMode(MOVE_Custom);
	//SetBase(Box->GetComponentByClass<UStaticMeshComponent>());

	UMyCharacterMovementComponent* MovementComponent = Cast<UMyCharacterMovementComponent>(GetCharacterMovement());
	if (MovementComponent)
		MovementComponent->Box = Box;

	GetCharacterMovement()->OldBaseQuat = Box->GetActorRotation().Quaternion();
	GetCharacterMovement()->OldBaseLocation = Box->GetComponentByClass<UStaticMeshComponent>()->GetComponentLocation();
}

// Called every frame
void AMyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UMyCharacterMovementComponent* Mc = Cast<UMyCharacterMovementComponent>(GetCharacterMovement());
	if (!Mc)
		return;

	const UPrimitiveComponent* MovementBase = Box->GetComponentByClass<UStaticMeshComponent>();
	FQuat NewBaseQuat;
	FVector NewBaseLocation;
	if (!MovementBaseUtility::GetMovementBaseTransform(MovementBase, NAME_None, NewBaseLocation, NewBaseQuat))
	{
		return;
	}

	UCapsuleComponent* capsuleC = GetCapsuleComponent();
	float HalfHeight, Radius;
	capsuleC->GetScaledCapsuleSize(Radius, HalfHeight);

	const FQuatRotationTranslationMatrix OldLocalToWorld(Mc->OldBaseQuat, Mc->OldBaseLocation);
	const FQuatRotationTranslationMatrix NewLocalToWorld(NewBaseQuat, NewBaseLocation);
	
	FVector DeltaPosition = NewBaseLocation - Mc->OldBaseLocation;
	FQuat DeltaQuat = NewBaseQuat * Mc->OldBaseQuat.Inverse();
	const bool bRotationChanged = !Mc->OldBaseQuat.Equals(NewBaseQuat, 1e-8f);

	capsuleC->GetScaledCapsuleSize(Radius, HalfHeight);
	FVector const BaseOffset(0.0f, 0.0f, HalfHeight);
	FVector const LocalBasePos = OldLocalToWorld.InverseTransformPosition(capsuleC->GetComponentLocation() - BaseOffset);
	FVector const NewWorldPos = NewLocalToWorld.TransformPosition(LocalBasePos) + BaseOffset;
	DeltaPosition = NewWorldPos - capsuleC->GetComponentLocation();
	capsuleC->MoveComponent(DeltaPosition, capsuleC->GetComponentQuat(), false, nullptr, EMoveComponentFlags::MOVECOMP_IgnoreBases, ETeleportType::None);
	//capsuleC->MoveComponent(DeltaPosition, capsuleC->GetComponentQuat(), false, nullptr, EMoveComponentFlags::MOVECOMP_IgnoreBases, ETeleportType::None);

	////if (bRotationChanged)
	//{
	//	DeltaQuat = NewBaseQuat * Mc->OldBaseQuat.Inverse();
	//	FVector DeltaLocation = MovementBase->GetComponentLocation() - capsuleC->GetComponentLocation();
	//	FTransform NewComponentToWorld;
	//	NewComponentToWorld.SetLocation(MovementBase->GetOwner()->GetActorLocation() + NewBaseQuat.RotateVector(DeltaLocation));
	//	capsuleC->SetComponentToWorld(NewComponentToWorld);
	//}

	Mc->OldBaseLocation = NewBaseLocation;
	Mc->OldBaseQuat = NewBaseQuat;
}

// Called to bind functionality to input
void AMyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

