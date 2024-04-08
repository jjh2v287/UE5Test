// Fill out your copyright notice in the Description page of Project Settings.

#include "UETest/Public/UKTickIntervalFocusComponent.h"
#include "UETest/Public/UKTickIntervalSubsystem.h"
#include "Engine/World.h"

// Sets default values for this component's properties
UUKTickIntervalFocusComponent::UUKTickIntervalFocusComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UUKTickIntervalFocusComponent::BeginPlay()
{
	Super::BeginPlay();

	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	TickIntervalSubsystem = UKTickIntervalSubsystem::Get();
	if(TickIntervalSubsystem == nullptr)
	{
		return;
	}

	TickIntervalSettings.Sort([](const FUKTickIntervalSettings A, const FUKTickIntervalSettings B) {
			return A.IntervalDistance > B.IntervalDistance;
		});
	
	TickIntervalSubsystem->RegisterFocus(this);
}

void UUKTickIntervalFocusComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (TickIntervalSubsystem == nullptr)
	{
		return;
	}
	
	TickIntervalSubsystem->UnregisterFocus(this);
	TickIntervalSubsystem = nullptr;
}

void UUKTickIntervalFocusComponent::OnRegister()
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
	
	TickIntervalSubsystem->RegisterFocus(this);
}

void UUKTickIntervalFocusComponent::OnUnregister()
{
	Super::OnUnregister();

	if (TickIntervalSubsystem == nullptr)
	{
		return;
	}
	
	TickIntervalSubsystem->UnregisterFocus(this);
	TickIntervalSubsystem = nullptr;
}

