// Fill out your copyright notice in the Description page of Project Settings.


#include "UKPlatformActor.h"
#include "UKPlatformGroupActor.h"
#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"

AUKPlatformActor::AUKPlatformActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	USceneComponent* SceneComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(this, TEXT("Scene"));
	RootComponent = SceneComponent;
}

void AUKPlatformActor::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);
	if(PlatformGroupActor)
	{
		PlatformGroupActor->Add(*this);
	}
}

#if WITH_EDITOR
void AUKPlatformActor::PreEditChange(FProperty* PropertyThatWillChange)
{
	Super::PreEditChange(PropertyThatWillChange);
	
	if(PropertyThatWillChange == nullptr)
	{
		return;	
	}

	const FName PropertyName = PropertyThatWillChange->GetFName();
	bool IsVariableName = PropertyName == GET_MEMBER_NAME_CHECKED(AUKPlatformActor, PlatformGroupActor);
	
	if (IsVariableName)
	{
		PreEditChangePlatformGroup = nullptr;
		PreEditChangePlatformGroup = PlatformGroupActor;
	}
}

void AUKPlatformActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if(PropertyChangedEvent.Property == nullptr)
	{
		return;	
	}

	const FName PropertyName = PropertyChangedEvent.Property->GetFName();
	bool IsVariableName = PropertyName == GET_MEMBER_NAME_CHECKED(AUKPlatformActor, PlatformGroupActor);
	
	if (IsVariableName)
	{
		if(PreEditChangePlatformGroup && !PlatformGroupActor)
		{
			PreEditChangePlatformGroup->Remove(*this);
			PreEditChangePlatformGroup = nullptr;
		}
		else if(!PreEditChangePlatformGroup && PlatformGroupActor)
		{
			PlatformGroupActor->Add(*this);
		}
	}
}
#endif

void AUKPlatformActor::BeginPlay()
{
	Super::BeginPlay();

	if(DebugTextRenderComponent)
	{
		DebugTextRenderComponent->UnregisterComponent();
		DebugTextRenderComponent->DestroyComponent();
		DebugTextRenderComponent = nullptr;
	}
	
	GetComponents(AllComponents);
	if(PlatformGroupActor)
	{
		ActorTickEnabled(false);
	}
}

void AUKPlatformActor::Destroyed()
{
	Super::Destroyed();

	if(PlatformGroupActor == nullptr)
	{
		return;
	}
	
	if (GetWorld()->IsGameWorld())
	{
		PlatformGroupActor->Remove(*this);	
	}
}

void AUKPlatformActor::ActorTickEnabled(bool Enabled)
{
	SetActorTickEnabled(Enabled);
	for (auto Component : AllComponents)
	{
		Component->SetComponentTickEnabled(Enabled);
	}
}

void AUKPlatformActor::SetDebugText(FString Str)
{
#if WITH_EDITOR
	FText Text = FText::FromString(Str);
	FBox ActorBox = GetComponentsBoundingBox( true );
	Modify();
	
	if(Str.IsEmpty())
	{
		if(DebugTextRenderComponent)
		{
			DebugTextRenderComponent->UnregisterComponent();
			DebugTextRenderComponent->DestroyComponent();
			DebugTextRenderComponent = nullptr;
		}
		return;
	}
	
	if(DebugTextRenderComponent == nullptr)
	{
		DebugTextRenderComponent = NewObject<UTextRenderComponent>(this);
		DebugTextRenderComponent->SetMobility(EComponentMobility::Movable);
		DebugTextRenderComponent->SetRelativeLocation(FVector(0.0f, 0.0f, ActorBox.Max.Z));
		DebugTextRenderComponent->SetText(Text);
		DebugTextRenderComponent->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
		DebugTextRenderComponent->SetHorizontalAlignment(EHTA_Center);
		DebugTextRenderComponent->SetVerticalAlignment(EVRTA_TextCenter);
		DebugTextRenderComponent->SetHiddenInGame(true);
		DebugTextRenderComponent->SetGenerateOverlapEvents(false);
		DebugTextRenderComponent->SetCanEverAffectNavigation(false);
		DebugTextRenderComponent->SetTextRenderColor(FColor::Black);
		DebugTextRenderComponent->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		DebugTextRenderComponent->RegisterComponent();
	}
	else
	{
		DebugTextRenderComponent->SetText(Text);
		DebugTextRenderComponent->SetRelativeLocation(FVector(0.0f, 0.0f, ActorBox.Max.Z));
	}
#endif
}

