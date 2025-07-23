// Copyright 2021 Marcin Swiderski. All Rights Reserved.

#include "TickOptToolkitMimicComponent.h"
#include "TickOptToolkit.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Particles/ParticleSystemComponent.h"

UTickOptToolkitMimicComponent::UTickOptToolkitMimicComponent()
	: Super()
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UTickOptToolkitMimicComponent::BeginPlay()
{
	AActor* Owner = GetOwner();
	
	if (bActorTickControl)
	{
		bActorTickControl = Owner->PrimaryActorTick.bCanEverTick && Owner->PrimaryActorTick.bStartWithTickEnabled && !Owner->IsTemplate();
	}
	
	if (bComponentsTickControl)
	{
		TInlineComponentArray<UActorComponent*> Components;
		Owner->GetComponents(Components);
		for (UActorComponent* Component : Components)
		{
			if (!Component->IsA<UTimelineComponent>() && !Component->IsA<UParticleSystemComponent>() && Component->PrimaryComponentTick.bStartWithTickEnabled)
			{
				AddComponentTickControl(Component);
			}
		}
	}

	if (bTimelinesTickControl)
	{
		TInlineComponentArray<UTimelineComponent*> Timelines;
		Owner->GetComponents(Timelines);
		for (UTimelineComponent* Timeline : Timelines)
		{
			AddTimelineTickControl(Timeline, bSyncTimelinesToWorld);
		}
	}

	// For controllers, handle registering targets automatically
	if (AController* Controller = Cast<AController>(Owner))
	{
		OnNewPawn(Controller->GetPawn());
		Controller->GetOnNewPawnNotifier().AddUObject(this, &UTickOptToolkitMimicComponent::OnNewPawn);
	}

	// Call last, so bHasBegunPlay won't be set earlier
	Super::BeginPlay();
}

void UTickOptToolkitMimicComponent::OnNewPawn(APawn* Pawn)
{
	UnregisterMimicTarget();
	if (Pawn)
	{
		if (UTickOptToolkitTargetComponent* Target = Pawn->FindComponentByClass<UTickOptToolkitTargetComponent>())
		{
			RegisterMimicTarget(Target);
		}
	}
}

void UTickOptToolkitMimicComponent::SetActorTickControl(bool bInActorTickControl)
{
	bActorTickControl = bInActorTickControl;
	if (bActorTickControl && HasBegunPlay())
	{
		if (AActor* Owner = GetOwner())
		{
			bActorTickControl = Owner->PrimaryActorTick.bCanEverTick && !Owner->IsTemplate();
			if (bActorTickControl && MimicTarget)
			{
				bool bTickEnabled;
				float TickInterval;
				MimicTarget->GetCurrentTickSettings(bTickEnabled, TickInterval);
				
				const bool bWasTickEnabled = Owner->PrimaryActorTick.IsTickFunctionEnabled();
				if (bTickEnabled != bWasTickEnabled)
				{
					Owner->PrimaryActorTick.SetTickFunctionEnable(bTickEnabled);
				}
				Owner->PrimaryActorTick.TickInterval = TickInterval; 
			}
		}
		else
		{
			bActorTickControl = false;
		}
	}
}

void UTickOptToolkitMimicComponent::SetComponentsTickControl(bool bInComponentsTickControl)
{
	if (!HasBegunPlay())
	{
		bComponentsTickControl = bInComponentsTickControl;
	}
	else
	{
		UE_LOG(LogTickOptToolkit, Warning, TEXT("Cannot set 'ComponentsTickControl' property after BeginPlay!"));
	}
}

void UTickOptToolkitMimicComponent::SetTimelinesTickControl(bool bInTimelinesTickControl)
{
	if (!HasBegunPlay())
	{
		bTimelinesTickControl = bInTimelinesTickControl;
	}
	else
	{
		UE_LOG(LogTickOptToolkit, Warning, TEXT("Cannot set 'TimelinesTickControl' property after BeginPlay!"));
	}
}

void UTickOptToolkitMimicComponent::SetSyncTimelinesToWorld(bool bInSyncTimelinesToWorld)
{
	if (!HasBegunPlay())
	{
		bSyncTimelinesToWorld = bInSyncTimelinesToWorld;
	}
	else
	{
		UE_LOG(LogTickOptToolkit, Warning, TEXT("Cannot set 'SyncTimelinesToWorld' property after BeginPlay!"));
	}
}

void UTickOptToolkitMimicComponent::RegisterMimicTarget(UTickOptToolkitTargetComponent* Target)
{
	if (MimicTarget)
	{
		UnregisterMimicTarget();
	}

	MimicTarget = Target;
	if (MimicTarget)
	{
		MimicTarget->AddMimic(this);
	}
}

void UTickOptToolkitMimicComponent::UnregisterMimicTarget()
{
	if (MimicTarget)
	{
		MimicTarget->RemoveMimic(this);
	}
	MimicTarget = nullptr;
}

void UTickOptToolkitMimicComponent::AddComponentTickControl(UActorComponent* Component)
{
	const AActor* Owner = GetOwner();
	if (Owner && Component && Component->GetOwner() == Owner && Component->PrimaryComponentTick.bCanEverTick && !Component->IsTemplate())
	{
		if (Component->GetClass()->GetFName() == NAME_NiagaraComponent)
		{
			UE_LOG(LogTickOptToolkit, Warning, TEXT("Niagara Particle System cannot have it's tick controled. Please use Niagara scalability settings instead."))
		}
		else if (Component->IsA<UParticleSystemComponent>())
		{
			UE_LOG(LogTickOptToolkit, Warning, TEXT("Cascade Particle System cannot have it's tick controled. Please use Cascade LOD settings instead."))
		}
		else
		{
			TickControlledComponents.AddUnique(Component);

			if (HasBegunPlay() && MimicTarget)
			{
				bool bTickEnabled;
				float TickInterval;
				MimicTarget->GetCurrentTickSettings(bTickEnabled, TickInterval);
				
				const bool bWasTickEnabled = Component->PrimaryComponentTick.IsTickFunctionEnabled();
				if (bTickEnabled != bWasTickEnabled)
				{
					Component->PrimaryComponentTick.SetTickFunctionEnable(bTickEnabled);
				}
				Component->PrimaryComponentTick.TickInterval = TickInterval;
			}
		}
	}
}

void UTickOptToolkitMimicComponent::RemoveComponentTickControl(UActorComponent* Component)
{
	TickControlledComponents.RemoveAllSwap([Component](const UActorComponent* Entry)
	{
		return Entry == Component;
	});
}

void UTickOptToolkitMimicComponent::AddTimelineTickControl(UTimelineComponent* Timeline, bool bSyncToWorld)
{
	const AActor* Owner = GetOwner();
	if (Owner && Timeline && Timeline->GetOwner() == Owner && Timeline->PrimaryComponentTick.bCanEverTick && !Timeline->IsTemplate())
	{
		bSyncToWorld = bSyncToWorld && Timeline->IsLooping();
		TickControlledTimelines.AddUnique({Timeline, bSyncToWorld});

		if (HasBegunPlay() && MimicTarget)
		{
			bool bTickEnabled;
			float TickInterval;
			MimicTarget->GetCurrentTickSettings(bTickEnabled, TickInterval);
		
			const bool bWasTickEnabled = Timeline->PrimaryComponentTick.IsTickFunctionEnabled();
			if (bTickEnabled != bWasTickEnabled)
			{
				Timeline->PrimaryComponentTick.SetTickFunctionEnable(bTickEnabled && Timeline->IsPlaying());
			}
			Timeline->PrimaryComponentTick.TickInterval = TickInterval;
		}
		
		if (bSyncToWorld && GetWorld())
		{
			const float WorldTime = GetWorld()->GetTimeSeconds();
			UTickOptToolkitTargetComponent::SyncTimelineToWorldTime(Timeline, WorldTime);
		}
	}
}

void UTickOptToolkitMimicComponent::RemoveTimelineTickControl(UTimelineComponent* Timeline)
{
	RemoveComponentTickControl(Timeline);
}

void UTickOptToolkitMimicComponent::OnUpdated(bool bTickEnabled, float TickInterval, int TickZone, bool bZoneUpdated, bool bTickVisible, bool bVisUpdated)
{
	if (bActorTickControl || TickControlledComponents.Num() > 0 || TickControlledTimelines.Num() > 0)
	{
		bool bTickEnabledChanged = false;
	
		if (bActorTickControl)
		{
			AActor* Owner = GetOwner();
			const bool bWasTickEnabled = Owner->PrimaryActorTick.IsTickFunctionEnabled();
			if (bTickEnabled != bWasTickEnabled)
			{
				Owner->PrimaryActorTick.SetTickFunctionEnable(bTickEnabled);
				bTickEnabledChanged = true;
			}
			Owner->PrimaryActorTick.TickInterval = TickInterval;
		}

		for (int I = 0; I < TickControlledComponents.Num(); ++I)
		{
			UActorComponent* Component = TickControlledComponents[I];
			if (Component == nullptr)
			{
				TickControlledComponents.RemoveAtSwap(I--);
				continue;
			}

			const bool bWasTickEnabled = Component->PrimaryComponentTick.IsTickFunctionEnabled();
			if (bTickEnabled != bWasTickEnabled)
			{
				Component->PrimaryComponentTick.SetTickFunctionEnable(bTickEnabled);
				bTickEnabledChanged = true;
			}
			Component->PrimaryComponentTick.TickInterval = TickInterval;
		}
		
		const float WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
		for (int I = 0; I < TickControlledTimelines.Num(); ++I)
		{
			const FTickControlledTimeline& Controlled = TickControlledTimelines[I];
			if (Controlled.Timeline == nullptr)
			{
				TickControlledTimelines.RemoveAtSwap(I--);
				continue;
			}

			const bool bWasTickEnabled = Controlled.Timeline->PrimaryComponentTick.IsTickFunctionEnabled();
			if (bTickEnabled != bWasTickEnabled)
			{
				Controlled.Timeline->PrimaryComponentTick.SetTickFunctionEnable(bTickEnabled && Controlled.Timeline->IsPlaying());
				bTickEnabledChanged = true;
			}
			Controlled.Timeline->PrimaryComponentTick.TickInterval = TickInterval;

			if (Controlled.bSyncTimelineToWorld && !bWasTickEnabled && bTickEnabled)
			{
				UTickOptToolkitTargetComponent::SyncTimelineToWorldTime(Controlled.Timeline, WorldTime);
			}
		}

		if (bTickEnabledChanged && OnTickEnabledChanged.IsBound())
		{
			OnTickEnabledChanged.Broadcast(bTickEnabled);
		}
	}
	
	if (OnTickChanged.IsBound())
	{
		OnTickChanged.Broadcast(bZoneUpdated, bVisUpdated, TickZone, bTickVisible);
	}
	if (bZoneUpdated && OnTickZoneChanged.IsBound())
	{
		OnTickZoneChanged.Broadcast(TickZone);
	}
	if (bVisUpdated && OnTickVisibilityChanged.IsBound())
	{
		OnTickVisibilityChanged.Broadcast(bTickVisible);
	}
}

void UTickOptToolkitMimicComponent::OnBalanceTickInterval(float TickInterval)
{	
	if (bActorTickControl)
	{
		AActor* Owner = GetOwner();
		Owner->PrimaryActorTick.UpdateTickIntervalAndCoolDown(TickInterval);
	}

	for (int I = 0; I < TickControlledComponents.Num(); ++I)
	{
		UActorComponent* Component = TickControlledComponents[I];
		if (Component == nullptr)
		{
			TickControlledComponents.RemoveAtSwap(I--);
			continue;
		}
		Component->PrimaryComponentTick.UpdateTickIntervalAndCoolDown(TickInterval);
	}
	
	for (int I = 0; I < TickControlledTimelines.Num(); ++I)
	{
		const FTickControlledTimeline& Controlled = TickControlledTimelines[I];
		if (Controlled.Timeline == nullptr)
		{
			TickControlledTimelines.RemoveAtSwap(I--);
			continue;
		}
		Controlled.Timeline->PrimaryComponentTick.UpdateTickIntervalAndCoolDown(TickInterval);
	}
}
