// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ZKMeleeAttack_AnimNotifyState.generated.h"

/**
 * 
 */
UCLASS()
class ZKGAME_API UZKMeleeAttack_AnimNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()
	
public:
	UZKMeleeAttack_AnimNotifyState();

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZK")
	FName BoneName = NAME_None;
protected:
	double PreMontagePosition;
	double CurrentMontagePosition;
};
