// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Actor.h"
#include "UKMapCaptureBox.generated.h"

UENUM()
enum class EMapCaptureBoxType
{
	SCB_World UMETA(DisplayName="World"),
	SCB_Dungeon UMETA(DisplayName="Dungeon"),
};

UCLASS()
class UETEST_API AUKMapCaptureBox : public AActor
{
	GENERATED_BODY()
public:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Capture)
	TObjectPtr<UBoxComponent> BoxComponent = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Capture)
	EMapCaptureBoxType SceneCaptureBoxType = EMapCaptureBoxType::SCB_World;
	
public:	
	// Sets default values for this actor's properties
	explicit AUKMapCaptureBox(const FObjectInitializer& ObjectInitializer);
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }
	virtual void OnConstruction(const FTransform& Transform) override;
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	void SetBoxExtent(FVector BoxSize);
	FVector GetBoxExtent();

};
