// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/TextRenderComponent.h"
#include "UKPlatformActor.generated.h"

class AUKPlatformGroupActor;

UCLASS()
class UKPLATFORMTOOLKIT_API AUKPlatformActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AUKPlatformActor(const FObjectInitializer& ObjectInitializer);
	virtual void PostDuplicate(bool bDuplicateForPIE) override;
	
#if WITH_EDITOR
	AUKPlatformGroupActor* PreEditChangePlatformGroup = nullptr;
	virtual void PreEditChange(FProperty* PropertyThatWillChange) override;
	virtual void PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
protected:
	virtual void BeginPlay() override;
	virtual void Destroyed() override;
	
public:
	void ActorTickEnabled(bool Enabled);

	void SetDebugText(FString Str);

private:
	TInlineComponentArray<UActorComponent*> AllComponents;
	
public:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UTextRenderComponent> DebugTextRenderComponent;
	
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = PlatformActor)
	TObjectPtr<AUKPlatformGroupActor> PlatformGroupActor;
};
