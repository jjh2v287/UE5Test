// Fill out your copyright notice in the Description page of Project Settings.


#include "MyCharacterMovementComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"

UMyCharacterMovementComponent::UMyCharacterMovementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UMyCharacterMovementComponent::UpdateBasedMovement(float DeltaSeconds)
{
	Super::UpdateBasedMovement(DeltaSeconds);
}

void UMyCharacterMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	Super::PhysCustom(deltaTime, Iterations);

	// if (!Box.IsValid())
	//	return;

	//const UPrimitiveComponent* MovementBase = Box->GetComponentByClass<UStaticMeshComponent>();
	//if (!MovementBaseUtility::UseRelativeLocation(MovementBase))
	//{
	//	return;
	//}

	//FQuat DeltaQuat = FQuat::Identity;
	//FQuat NewBaseQuat;
	//FVector NewBaseLocation;
	//if (!MovementBaseUtility::GetMovementBaseTransform(MovementBase, CharacterOwner->GetBasedMovement().BoneName, NewBaseLocation, NewBaseQuat))
	//{
	//	return;
	//}

	//const bool bRotationChanged = !OldBaseQuat.Equals(NewBaseQuat, 1e-8f);
	//if (bRotationChanged)
	//{
	//	DeltaQuat = NewBaseQuat * OldBaseQuat.Inverse();
	//}

	//// only if base moved
	//if (bRotationChanged || (OldBaseLocation != NewBaseLocation))
	//{
	//	// Calculate new transform matrix of base actor (ignoring scale).
	//	const FQuatRotationTranslationMatrix OldLocalToWorld(OldBaseQuat, OldBaseLocation);
	//	const FQuatRotationTranslationMatrix NewLocalToWorld(NewBaseQuat, NewBaseLocation);

	//	FQuat FinalQuat = UpdatedComponent->GetComponentQuat();

	//	if (bRotationChanged && !bIgnoreBaseRotation)
	//	{
	//		// Apply change in rotation and pipe through FaceRotation to maintain axis restrictions
	//		const FQuat PawnOldQuat = UpdatedComponent->GetComponentQuat();
	//		const FQuat TargetQuat = DeltaQuat * FinalQuat;
	//		FRotator TargetRotator(TargetQuat);
	//		CharacterOwner->FaceRotation(TargetRotator, 0.f);
	//		FinalQuat = UpdatedComponent->GetComponentQuat();

	//		if (PawnOldQuat.Equals(FinalQuat, 1e-6f))
	//		{
	//			// Nothing changed. This means we probably are using another rotation mechanism (bOrientToMovement etc). We should still follow the base object.
	//			// @todo: This assumes only Yaw is used, currently a valid assumption. This is the only reason FaceRotation() is used above really, aside from being a virtual hook.
	//			if (bOrientRotationToMovement || (bUseControllerDesiredRotation && CharacterOwner->Controller))
	//			{
	//				TargetRotator.Pitch = 0.f;
	//				TargetRotator.Roll = 0.f;
	//				MoveUpdatedComponent(FVector::ZeroVector, TargetRotator, false);
	//				FinalQuat = UpdatedComponent->GetComponentQuat();
	//			}
	//		}

	//		// Pipe through ControlRotation, to affect camera.
	//		if (CharacterOwner->Controller)
	//		{
	//			const FQuat PawnDeltaRotation = FinalQuat * PawnOldQuat.Inverse();
	//			FRotator FinalRotation = FinalQuat.Rotator();
	//			UpdateBasedRotation(FinalRotation, PawnDeltaRotation.Rotator());
	//			FinalQuat = UpdatedComponent->GetComponentQuat();
	//		}
	//	}

	//	float HalfHeight, Radius;
	//	CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleSize(Radius, HalfHeight);

	//	FVector const BaseOffset(0.0f, 0.0f, HalfHeight);
	//	FVector const LocalBasePos = OldLocalToWorld.InverseTransformPosition(UpdatedComponent->GetComponentLocation() - BaseOffset);
	//	FVector const NewWorldPos = ConstrainLocationToPlane(NewLocalToWorld.TransformPosition(LocalBasePos) + BaseOffset);

	//	UpdatedComponent->SetWorldLocationAndRotation(NewWorldPos, FinalQuat, false);

	//	OldBaseQuat = FinalQuat;
	//	OldBaseLocation = NewWorldPos;
	//}
}