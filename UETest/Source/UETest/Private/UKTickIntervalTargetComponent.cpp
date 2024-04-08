// Fill out your copyright notice in the Description page of Project Settings.

#include "UETest/Public/UKTickIntervalTargetComponent.h"
#include "UETest/Public/UKTickIntervalSubsystem.h"
#include "Engine/World.h"

// Sets default values for this component's properties
UUKTickIntervalTargetComponent::UUKTickIntervalTargetComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UUKTickIntervalTargetComponent::BeginPlay()
{
	Super::BeginPlay();

	const UWorld* World = GetWorld();
	if(World == nullptr)
	{
		return;
	}

	TickIntervalSubsystem = UKTickIntervalSubsystem::Get();
	if(TickIntervalSubsystem == nullptr)
	{
		return;
	}

	GetOwner()->GetComponents(Components);
	// SkeletalMeshComponent

	TickIntervalSubsystem->RegisterTarget(this);
}

void UUKTickIntervalTargetComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (TickIntervalSubsystem == nullptr)
	{
		return;
	}
	
	TickIntervalSubsystem->UnregisterTarget(this);
	TickIntervalSubsystem = nullptr;
}

void UUKTickIntervalTargetComponent::OnRegister()
{
	Super::OnRegister();

	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}
	
	if (!HasBegunPlay())
	{
		return;
	}

	TickIntervalSubsystem = UKTickIntervalSubsystem::Get();
	if(TickIntervalSubsystem == nullptr)
	{
		return;
	}
	
	TickIntervalSubsystem->RegisterTarget(this);
}

void UUKTickIntervalTargetComponent::OnUnregister()
{
	Super::OnUnregister();

	if (TickIntervalSubsystem == nullptr)
	{
		return;
	}
	
	TickIntervalSubsystem->UnregisterTarget(this);
	TickIntervalSubsystem = nullptr;
}

void UUKTickIntervalTargetComponent::SetTickInterval(float IntervalTime)
{
	GetOwner()->SetActorTickInterval(IntervalTime);
	
	for (UActorComponent* Component : Components)
	{
		Component->SetComponentTickInterval(IntervalTime);
	}
}

void UUKTickIntervalTargetComponent::SetTickEnabled(bool Enabled)
{
	GetOwner()->SetActorTickEnabled(Enabled);

	for (UActorComponent* Component : Components)
	{
		Component->SetComponentTickEnabled(Enabled);
	}
}

