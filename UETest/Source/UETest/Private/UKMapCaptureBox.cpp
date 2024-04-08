// Fill out your copyright notice in the Description page of Project Settings.


#include "UETest/Public/UKMapCaptureBox.h"

// Sets default values
AUKMapCaptureBox::AUKMapCaptureBox(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	BoxComponent = ObjectInitializer.CreateDefaultSubobject<UBoxComponent>(this, TEXT("Bounds"));
	if(BoxComponent)
	{
		BoxComponent->SetBoxExtent(FVector(100.0f,100.0f,100.0f));
		BoxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		BoxComponent->SetCanEverAffectNavigation(false);
		BoxComponent->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
		BoxComponent->SetGenerateOverlapEvents(false);
		BoxComponent->SetLineThickness(10.0f);
		// BoxComponent->SetIsVisualizationComponent(true);
		BoxComponent->SetupAttachment(RootComponent);
	}
}

// Called when the game starts or when spawned
void AUKMapCaptureBox::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AUKMapCaptureBox::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AUKMapCaptureBox::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

void AUKMapCaptureBox::SetBoxExtent(FVector BoxSize)
{
	if(BoxComponent)
		BoxComponent->SetBoxExtent(BoxSize);
}

FVector AUKMapCaptureBox::GetBoxExtent()
{
	return BoxComponent->GetUnscaledBoxExtent();
}
