// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UKPlatformActor.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Actor.h"
#include "UKPlatformGroupActor.generated.h"

UCLASS()
class UKPLATFORMTOOLKIT_API AUKPlatformGroupActor : public AActor
{
	GENERATED_BODY()
public:
	AUKPlatformGroupActor(const FObjectInitializer& ObjectInitializer);
	
	virtual void BeginPlay() override;
	virtual void Destroyed() override;
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }
	virtual void Tick(float DeltaSeconds) override;
	void PlatformActorsTickEnabled(bool Enabled);

	UFUNCTION()
	void BoxBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& OverlapInfo);
	UFUNCTION()
	void BoxEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

#if WITH_EDITOR
	TArray<TObjectPtr<class AUKPlatformActor>> PreEditChangePrimaryPlatformActors;
	virtual void PreEditChange(FProperty* PropertyThatWillChange) override;
	virtual void PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
public:	
	void Add(AActor& InActor);
	void Remove(AActor& InActor);

	void CenterGroupLocation(bool IsBox = true);
	void GetBoundingVectorsForGroup(FVector& OutVectorMin, FVector& OutVectorMax);
	void GenerateCollision(bool IsBox, FVector Extent);

public:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = PlatformActor)
	TObjectPtr<UBoxComponent> BoxComponent = nullptr;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = PlatformActor)
	TObjectPtr<USphereComponent> SphereComponent = nullptr;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = PlatformActor)
	TArray<TObjectPtr<class AUKPlatformActor>> PlatformActors;
};