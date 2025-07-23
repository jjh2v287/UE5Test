// Copyright 2021 Marcin Swiderski. All Rights Reserved.

#include "TickOptToolkitFocusComponent.h"
#include "TickOptToolkitSubsystem.h"
#include "Engine/World.h"

UTickOptToolkitFocusComponent::UTickOptToolkitFocusComponent()
	: Super()
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UTickOptToolkitFocusComponent::BeginPlay()
{
	Super::BeginPlay();

	if (const UWorld* World = GetWorld())
	{
		Subsystem = World->GetSubsystem<UTickOptToolkitSubsystem>();
		Subsystem->RegisterFocus(this);
	}
}

void UTickOptToolkitFocusComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (Subsystem)
	{
		Subsystem->UnregisterFocus(this);
		Subsystem = nullptr;
	}
}

void UTickOptToolkitFocusComponent::OnRegister()
{
	Super::OnRegister();

	if (HasBegunPlay())
	{
		if (const UWorld* World = GetWorld())
		{
			Subsystem = World->GetSubsystem<UTickOptToolkitSubsystem>();
			Subsystem->RegisterFocus(this);
		}
	}
}

void UTickOptToolkitFocusComponent::OnUnregister()
{
	Super::OnUnregister();
	
	if (Subsystem)
	{
		Subsystem->UnregisterFocus(this);
		Subsystem = nullptr;
	}
}

bool UTickOptToolkitFocusComponent::ShouldTrack_Implementation()
{
	if (GetNetMode() == NM_Client)
	{
		if (const AActor* Owner = GetOwner())
		{
			return Owner->HasLocalNetOwner();
		}
		return false;
	}
	return true;
}
