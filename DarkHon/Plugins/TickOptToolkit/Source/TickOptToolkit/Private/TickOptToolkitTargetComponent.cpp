// Copyright 2021 Marcin Swiderski. All Rights Reserved.

#include "TickOptToolkitTargetComponent.h"
#include "TickOptToolkitSubsystem.h"
#include "TickOptToolkitMimicComponent.h"
#include "TickOptToolkit.h"
#include "TimerManager.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Particles/ParticleSystemComponent.h"
#include "Engine/World.h"
#include "Misc/EngineVersionComparison.h"

FTickOptToolkitOptimizationLevel UTickOptToolkitTargetComponent::DefaultOptimizationLevel;

UTickOptToolkitTargetComponent::UTickOptToolkitTargetComponent()
	: Super()
	, Subsystem()
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UTickOptToolkitTargetComponent::BeginPlay()
{
	const UWorld* World = GetWorld();
	const AActor* Owner = GetOwner();

	Subsystem = World->GetSubsystem<UTickOptToolkitSubsystem>();
	CurrentOptimizationLevel = GetCurrentOptimizationLevel();
	
	bWantsOnUpdateTransform = true;
	
	if (!bForced)
	{
		TickZone = DistanceMode != ETickOptToolkitDistanceMode::None ? MidZoneSizes.Num() + 1 : 0;
		bTickVisible = VisibilityMode != ETickByVisibilityMode::None ? false : true;
		Subsystem->RegisterTarget(this);
	}
	else
	{
		TickZone = DistanceMode != ETickOptToolkitDistanceMode::None ? FMath::Clamp(TickZone, 0, MidZoneSizes.Num() + 1) : 0;
		bTickVisible = VisibilityMode != ETickByVisibilityMode::None ? bTickVisible : true;
	}

	bZoneUpdated = DistanceMode != ETickOptToolkitDistanceMode::None;
	bVisUpdated = VisibilityMode != ETickByVisibilityMode::None;
	if (bZoneUpdated || bVisUpdated)
	{
		// Make sure the OnUpdated is called after all BeginPlay functions
		World->GetTimerManager().SetTimerForNextTick(this, &UTickOptToolkitTargetComponent::OnUpdatedInBeginPlay);
	}
	
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

	// Call last, so bHasBegunPlay won't be set earlier
	Super::BeginPlay();
}

void UTickOptToolkitTargetComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	if (bTickOptRegistered)
	{
		Subsystem->UnregisterTarget(this);
	}
	if (IsBalancingTickInterval())
	{
		Subsystem->CancelBalancingTargetTickInterval(this);
	}
	Subsystem = nullptr;
}

void UTickOptToolkitTargetComponent::OnRegister()
{
	Super::OnRegister();

	if (!bTickOptRegistered && !bForced && HasBegunPlay())
	{
		Subsystem->RegisterTarget(this);
	}
}

void UTickOptToolkitTargetComponent::OnUnregister()
{
	Super::OnUnregister();

	if (bTickOptRegistered)
	{
		Subsystem->UnregisterTarget(this);
	}
	if (IsBalancingTickInterval())
	{
		Subsystem->CancelBalancingTargetTickInterval(this);
	}
}

void UTickOptToolkitTargetComponent::OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport)
{
	Super::OnUpdateTransform(UpdateTransformFlags, Teleport);
	
	if (bTickOptRegistered)
	{
		Subsystem->UpdateTargetLocation(this);
	}
}

#if WITH_EDITOR
void UTickOptToolkitTargetComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	static FName DistanceModeName = GET_MEMBER_NAME_CHECKED(UTickOptToolkitTargetComponent, DistanceMode);
	static FName VisibilityModeName = GET_MEMBER_NAME_CHECKED(UTickOptToolkitTargetComponent, VisibilityMode);
	static FName MidZoneSizesName = GET_MEMBER_NAME_CHECKED(UTickOptToolkitTargetComponent, MidZoneSizes);
	static FName OptimizationLevelsName = GET_MEMBER_NAME_CHECKED(UTickOptToolkitTargetComponent, OptimizationLevels);

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	const EPropertyChangeType::Type ChangeType = PropertyChangedEvent.ChangeType;
	
	if (PropertyName == MidZoneSizesName)
	{
		const int32 Index = PropertyChangedEvent.GetArrayIndex(MidZoneSizesName.ToString());
		// ReSharper disable once CppDefaultCaseNotHandledInSwitchStatement
		switch (ChangeType)
		{
		case EPropertyChangeType::ArrayAdd:
			for (FTickOptToolkitOptimizationLevel& OptimizationLevel : OptimizationLevels)
			{
				OptimizationLevel.MidZoneSizes.Insert(MidZoneSizes[Index], Index);
			}
			if (DistanceMode != ETickOptToolkitDistanceMode::None)
			{
				TickSettings.Insert(FTickOptToolkitTickSettings(), Index + 1);
				for (FTickOptToolkitOptimizationLevel& OptimizationLevel : OptimizationLevels)
				{
					OptimizationLevel.TickSettings.Insert(FTickOptToolkitTickSettings(), Index + 1);
				}
			}
			break;
		case EPropertyChangeType::ArrayRemove:
			for (FTickOptToolkitOptimizationLevel& OptimizationLevel : OptimizationLevels)
			{
				OptimizationLevel.MidZoneSizes.RemoveAt(Index);
			}
			if (DistanceMode != ETickOptToolkitDistanceMode::None)
			{
				TickSettings.RemoveAt(Index + 1);
				for (FTickOptToolkitOptimizationLevel& OptimizationLevel : OptimizationLevels)
				{
					OptimizationLevel.TickSettings.RemoveAt(Index + 1);
				}
			}
			break;
		case EPropertyChangeType::ArrayClear:
			for (FTickOptToolkitOptimizationLevel& OptimizationLevel : OptimizationLevels)
			{
				OptimizationLevel.MidZoneSizes.Reset();
			}
			if (DistanceMode != ETickOptToolkitDistanceMode::None)
			{
				TickSettings[1] = TickSettings.Last();
				TickSettings.SetNum(2);
				for (FTickOptToolkitOptimizationLevel& OptimizationLevel : OptimizationLevels)
				{
					OptimizationLevel.TickSettings[1] = OptimizationLevel.TickSettings.Last();
					OptimizationLevel.TickSettings.SetNum(2);
				}
			}
			break;
		}
	}
	
	if (PropertyName == DistanceModeName || PropertyName == VisibilityModeName || PropertyName == MidZoneSizesName)
	{
		const uint32 ZonesNum = GetZonesNum();
		TickSettings.SetNum(ZonesNum);
		for (FTickOptToolkitOptimizationLevel& OptimizationLevel : OptimizationLevels)
		{
			OptimizationLevel.TickSettings.SetNum(ZonesNum);
		}
	}
	
	if (PropertyName == OptimizationLevelsName && ChangeType == EPropertyChangeType::ArrayAdd)
	{
		const int32 Index = PropertyChangedEvent.GetArrayIndex(OptimizationLevelsName.ToString());
		if (OptimizationLevels.IsValidIndex(Index))
		{
			FTickOptToolkitOptimizationLevel& Level = OptimizationLevels[Index];
			Level.SphereRadius = SphereRadius;
			Level.BoxExtents = BoxExtents;
			Level.BufferSize = BufferSize;
			Level.MidZoneSizes = MidZoneSizes;
			Level.TickSettings = TickSettings;
		}
	}
	
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UTickOptToolkitTargetComponent::SetDistanceMode(ETickOptToolkitDistanceMode InDistanceMode)
{
	if (!HasBegunPlay())
	{
		DistanceMode = InDistanceMode;
	}
	else
	{
		UE_LOG(LogTickOptToolkit, Warning, TEXT("Cannot set 'DistanceMode' property after BeginPlay!"));
	}
}

void UTickOptToolkitTargetComponent::SetVisibilityMode(ETickByVisibilityMode InVisibilityMode)
{
	if (!HasBegunPlay())
	{
		VisibilityMode = InVisibilityMode;
	}
	else
	{
		UE_LOG(LogTickOptToolkit, Warning, TEXT("Cannot set 'VisibilityMode' property after BeginPlay!"));
	}
}

void UTickOptToolkitTargetComponent::SetSphereRadius(float InRadius)
{
	if (!HasBegunPlay())
	{
		SphereRadius = InRadius;
	}
	else
	{
		UE_LOG(LogTickOptToolkit, Warning, TEXT("Cannot set 'SphereRadius' property after BeginPlay!"));
	}
}

void UTickOptToolkitTargetComponent::SetBoxExtents(FVector InExtents)
{
	if (!HasBegunPlay())
	{
		BoxExtents = InExtents;
	}
	else
	{
		UE_LOG(LogTickOptToolkit, Warning, TEXT("Cannot set 'BoxExtents' property after BeginPlay!"));
	}
}

void UTickOptToolkitTargetComponent::SetBoxRotation(float InRotation)
{
	if (!HasBegunPlay())
	{
		BoxRotation = InRotation;
	}
	else
	{
		UE_LOG(LogTickOptToolkit, Warning, TEXT("Cannot set 'BoxRotation' property after BeginPlay!"));
	}
}

void UTickOptToolkitTargetComponent::SetBufferSize(float InBufferSize)
{
	if (!HasBegunPlay())
	{
		BufferSize = InBufferSize;
	}
	else
	{
		UE_LOG(LogTickOptToolkit, Warning, TEXT("Cannot set 'BufferSize' property after BeginPlay!"));
	}
}

void UTickOptToolkitTargetComponent::SetMidZoneSizes(const TArray<float>& InMidZoneSizes)
{
	if (!HasBegunPlay())
	{
		if (MidZoneSizes.Num() != InMidZoneSizes.Num())
		{
			UE_LOG(LogTickOptToolkit, Warning, TEXT("Cannot change size of 'MidZoneSizes' using setter!"));
		}
		const uint32 Num = FMath::Min(MidZoneSizes.Num(), InMidZoneSizes.Num());
		for (uint32 I = 0; I < Num; ++I)
		{
			MidZoneSizes[I] = InMidZoneSizes[I];
		}
	}
	else
	{
		UE_LOG(LogTickOptToolkit, Warning, TEXT("Cannot set 'MidZoneSizes' property after BeginPlay!"));
	}
}

float UTickOptToolkitTargetComponent::GetZoneInnerSize(int InZone) const
{
	float Size = 0.0f;
	const TArray<float>& OptLevelMidZoneSizes = TOT_OPT_LEVEL_GET(MidZoneSizes);
	for (int ZI = 0; ZI < InZone - 1; ++ZI)
	{
		Size += OptLevelMidZoneSizes[ZI];
	}
	return Size;
}

uint32 UTickOptToolkitTargetComponent::GetZonesNum() const
{
	if (DistanceMode != ETickOptToolkitDistanceMode::None)
	{
		return GetMidZonesNum() + 2;
	}
	if (VisibilityMode != ETickByVisibilityMode::None)
	{
		return 1;
	}
	return 0;
}

void UTickOptToolkitTargetComponent::SetActorTickControl(bool bInActorTickControl)
{
	bActorTickControl = bInActorTickControl;
	if (bActorTickControl && HasBegunPlay())
	{
		if (AActor* Owner = GetOwner())
		{
			bActorTickControl = Owner->PrimaryActorTick.bCanEverTick && !Owner->IsTemplate();
			if (bActorTickControl)
			{
				bool bTickEnabled;
				float TickInterval;
				GetCurrentTickSettings(bTickEnabled, TickInterval);
				
				const bool bWasTickEnabled = Owner->PrimaryActorTick.IsTickFunctionEnabled();
				if (bTickEnabled != bWasTickEnabled)
				{
					Owner->PrimaryActorTick.SetTickFunctionEnable(bTickEnabled);
				}
				Owner->PrimaryActorTick.TickInterval = TickInterval;
				if (TickInterval > 0.0f && !IsBalancingTickInterval())
				{
					Subsystem->BalanceTargetTickInterval(this);
				} 
			}
		}
		else
		{
			bActorTickControl = false;
		}
	}
}

void UTickOptToolkitTargetComponent::SetComponentsTickControl(bool bInComponentsTickControl)
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

void UTickOptToolkitTargetComponent::SetTimelinesTickControl(bool bInTimelinesTickControl)
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

void UTickOptToolkitTargetComponent::SetSyncTimelinesToWorld(bool bInSyncTimelinesToWorld)
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

void UTickOptToolkitTargetComponent::SetForceExecuteFirstTick(bool bInForceExecuteFirstTick)
{
	if (!HasBegunPlay())
	{
		bForceExecuteFirstTick = bInForceExecuteFirstTick;
	}
	else
	{
		UE_LOG(LogTickOptToolkit, Warning, TEXT("Cannot set 'ForceExecuteFirstTick' property after BeginPlay!"));
	}
}

void UTickOptToolkitTargetComponent::SetFocusLayerMask(int InFocusLayerMask)
{
	FocusLayerMask = InFocusLayerMask;
	if (bTickOptRegistered)
	{
		Subsystem->UpdateTargetLayerMask(this);
	}
}

void UTickOptToolkitTargetComponent::SetDisableDistanceScale(bool bInDisableDistanceScale)
{
	if (!HasBegunPlay())
	{
		bDisableDistanceScale = bInDisableDistanceScale;
	}
	else
	{
		UE_LOG(LogTickOptToolkit, Warning, TEXT("Cannot set 'bDisableDistanceScale' property after BeginPlay!"));
	}
}

void UTickOptToolkitTargetComponent::SetTickSettings(const TArray<FTickOptToolkitTickSettings>& InTickSettings)
{
	if (!HasBegunPlay())
	{
		if (TickSettings.Num() != InTickSettings.Num())
		{
			UE_LOG(LogTickOptToolkit, Warning, TEXT("Cannot change size of 'TickSettings' using setter!"));
		}
		const uint32 Num = FMath::Min(TickSettings.Num(), InTickSettings.Num());
		for (uint32 I = 0; I < Num; ++I)
		{
			TickSettings[I] = InTickSettings[I];
		}
	}
	else
	{
		UE_LOG(LogTickOptToolkit, Warning, TEXT("Cannot set 'TickSettings' property after BeginPlay!"));
	}
}

void UTickOptToolkitTargetComponent::Force(int InZone, bool bInVisible)
{
	bForced = true;
	if (bTickOptRegistered)
	{
		Subsystem->UnregisterTarget(this);
	}
	
	InZone = DistanceMode != ETickOptToolkitDistanceMode::None ? FMath::Clamp(InZone, 0, TOT_OPT_LEVEL_GET(MidZoneSizes).Num() + 1) : 0;
	bInVisible = VisibilityMode != ETickByVisibilityMode::None ? bInVisible : true;

	bZoneUpdated = TickZone != InZone;
	bVisUpdated = bTickVisible != bInVisible;
	
	TickZone = InZone;
	bTickVisible = bInVisible;
	
	if (HasBegunPlay() && (bZoneUpdated || bVisUpdated))
	{
		OnUpdated();
	}
}

void UTickOptToolkitTargetComponent::Release()
{
	bForced = false;
	if (!bTickOptRegistered && HasBegunPlay() && IsRegistered())
	{
		Subsystem->RegisterTarget(this);
	}
}

void UTickOptToolkitTargetComponent::AddComponentTickControl(UActorComponent* Component)
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

			if (HasBegunPlay())
			{
				bool bTickEnabled;
				float TickInterval;
				GetCurrentTickSettings(bTickEnabled, TickInterval);
				
				const bool bWasTickEnabled = Component->PrimaryComponentTick.IsTickFunctionEnabled();
				if (bTickEnabled != bWasTickEnabled)
				{
					Component->PrimaryComponentTick.SetTickFunctionEnable(bTickEnabled);
				}
				Component->PrimaryComponentTick.TickInterval = TickInterval;
				if (TickInterval > 0.0f && !IsBalancingTickInterval())
				{
					Subsystem->BalanceTargetTickInterval(this);
				}
			}
		}
	}
}

void UTickOptToolkitTargetComponent::RemoveComponentTickControl(UActorComponent* Component)
{
	TickControlledComponents.RemoveAllSwap([Component](const UActorComponent* Entry)
	{
		return Entry == Component;
	});
}

void UTickOptToolkitTargetComponent::AddTimelineTickControl(UTimelineComponent* Timeline, bool bSyncToWorld)
{
	const AActor* Owner = GetOwner();
	if (Owner && Timeline && Timeline->GetOwner() == Owner && Timeline->PrimaryComponentTick.bCanEverTick && !Timeline->IsTemplate())
	{
		bSyncToWorld = bSyncToWorld && Timeline->IsLooping();
		TickControlledTimelines.AddUnique({Timeline, bSyncToWorld});

		if (HasBegunPlay())
		{
			bool bTickEnabled;
			float TickInterval;
			GetCurrentTickSettings(bTickEnabled, TickInterval);
		
			const bool bWasTickEnabled = Timeline->PrimaryComponentTick.IsTickFunctionEnabled();
			if (bTickEnabled != bWasTickEnabled)
			{
				Timeline->PrimaryComponentTick.SetTickFunctionEnable(bTickEnabled && Timeline->IsPlaying());
			}
			Timeline->PrimaryComponentTick.TickInterval = TickInterval;
			if (TickInterval > 0.0f && !IsBalancingTickInterval())
			{
				Subsystem->BalanceTargetTickInterval(this);
			}
		}
		
		if (bSyncToWorld && GetWorld())
		{
			const float WorldTime = GetWorld()->GetTimeSeconds();
			SyncTimelineToWorldTime(Timeline, WorldTime);
		}
	}
}

void UTickOptToolkitTargetComponent::RemoveTimelineTickControl(UTimelineComponent* Timeline)
{
	RemoveComponentTickControl(Timeline);
}

void UTickOptToolkitTargetComponent::AddMimic(UTickOptToolkitMimicComponent* Mimic)
{
	if (Mimic)
	{
		Mimics.AddUnique(Mimic);

		if (HasBegunPlay() && Mimic->HasBegunPlay())
		{
			bool bTickEnabled;
			float TickInterval;
			GetCurrentTickSettings(bTickEnabled, TickInterval);
		
			Mimic->OnUpdated(bTickEnabled, TickInterval,
				TickZone, DistanceMode != ETickOptToolkitDistanceMode::None,
				bTickVisible, VisibilityMode != ETickByVisibilityMode::None);
		}
	}
}

void UTickOptToolkitTargetComponent::RemoveMimic(UTickOptToolkitMimicComponent* Mimic)
{
	Mimics.Remove(Mimic);
}

void UTickOptToolkitTargetComponent::OnUpdated()
{
	bool bTickEnabled;
	float TickInterval;
	GetCurrentTickSettings(bTickEnabled, TickInterval);
	
	if (bActorTickControl || TickControlledComponents.Num() > 0 || TickControlledTimelines.Num() > 0)
	{
		bool bTickEnabledChanged = false;
		bool bTickIntervalChanged = false;
	
		if (bActorTickControl)
		{
			AActor* Owner = GetOwner();
			const bool bWasTickEnabled = Owner->PrimaryActorTick.IsTickFunctionEnabled();
			if (bTickEnabled != bWasTickEnabled)
			{
				Owner->PrimaryActorTick.SetTickFunctionEnable(bTickEnabled);
				bTickEnabledChanged = true;
			}
			
			bTickIntervalChanged |= Owner->PrimaryActorTick.TickInterval != TickInterval;
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
			
			bTickIntervalChanged |= Component->PrimaryComponentTick.TickInterval != TickInterval;
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

			bTickIntervalChanged |= Controlled.Timeline->PrimaryComponentTick.TickInterval != TickInterval;
			Controlled.Timeline->PrimaryComponentTick.TickInterval = TickInterval;

			if (Controlled.bSyncTimelineToWorld && !bWasTickEnabled && bTickEnabled)
			{
				SyncTimelineToWorldTime(Controlled.Timeline, WorldTime);
			}
		}

		if (bTickEnabledChanged && OnTickEnabledChanged.IsBound())
		{
			OnTickEnabledChanged.Broadcast(bTickEnabled);
		}

		if (bTickIntervalChanged && bTickEnabled && TickInterval > 0.0f && !IsBalancingTickInterval() && Subsystem)
		{
			Subsystem->BalanceTargetTickInterval(this);
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

	for (int I = 0; I < Mimics.Num(); ++I)
	{
		UTickOptToolkitMimicComponent* Mimic = Mimics[I];
		if (Mimic == nullptr)
		{
			Mimics.RemoveAtSwap(I--);
			continue;
		}
		Mimic->OnUpdated(bTickEnabled, TickInterval, TickZone, bZoneUpdated, bTickVisible, bVisUpdated);
	}
	
	bZoneUpdated = false;
	bVisUpdated = false;
}

void UTickOptToolkitTargetComponent::OnBalanceTickInterval()
{
	bool bTickEnabled;
	float TickInterval;
	GetCurrentTickSettings(bTickEnabled, TickInterval);

	if (bTickEnabled && TickInterval > 0.0f)
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
		
		for (int I = 0; I < Mimics.Num(); ++I)
		{
			UTickOptToolkitMimicComponent* Mimic = Mimics[I];
			if (Mimic == nullptr)
			{
				Mimics.RemoveAtSwap(I--);
				continue;
			}
			Mimic->OnBalanceTickInterval(TickInterval);
		}
	}
}

void UTickOptToolkitTargetComponent::OnUpdatedInBeginPlay()
{
#if UE_VERSION_OLDER_THAN(5, 0, 0)
	if (!IsPendingKill() && (bZoneUpdated || bVisUpdated))
#else
	if (IsValid(this) && (bZoneUpdated || bVisUpdated))
#endif
	{
		ForceExecuteFirstTick();
		OnUpdated();
	}
}

void UTickOptToolkitTargetComponent::ForceExecuteFirstTick()
{	
	if (bForceExecuteFirstTick && (bActorTickControl || TickControlledComponents.Num() > 0 || TickControlledTimelines.Num() > 0))
	{
		const TArray<FTickOptToolkitTickSettings>& OptLevelTickSettings = TOT_OPT_LEVEL_GET(TickSettings); 
		const FTickOptToolkitTickSettings& Settings = TickZone < OptLevelTickSettings.Num()
			? OptLevelTickSettings[TickZone]
			: FTickOptToolkitTickSettings();
		const bool bTickEnabled = bTickVisible ? Settings.bEnabledVisible : Settings.bEnabledHidden;
		if (!bTickEnabled)
		{
			const float DeltaTime = GetWorld()->DeltaTimeSeconds;
			const FGraphEventRef NullGraphEvent;
				
			if (bActorTickControl)
			{
				AActor* Owner = GetOwner();
				Owner->PrimaryActorTick.ExecuteTick(DeltaTime, LEVELTICK_All, ENamedThreads::GameThread, NullGraphEvent);
			}

			for (UActorComponent* Component : TickControlledComponents)
			{
				Component->PrimaryComponentTick.ExecuteTick(DeltaTime, LEVELTICK_All, ENamedThreads::GameThread, NullGraphEvent);
			}
			
			for (const FTickControlledTimeline& Controlled : TickControlledTimelines)
			{
				if (Controlled.Timeline->IsPlaying())
				{
					Controlled.Timeline->PrimaryComponentTick.ExecuteTick(DeltaTime, LEVELTICK_All, ENamedThreads::GameThread, NullGraphEvent);;
				}
			}
		}
	}
}

#if WITH_EDITOR
bool UTickOptToolkitTargetComponent::DoesOwnerStartWithTickEnabled() const
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		if (const UBlueprintGeneratedClass* Class = Cast<UBlueprintGeneratedClass>(GetOuter()))
		{
			Owner = Cast<AActor>(Class->GetDefaultObject());
		}
	}
	if (Owner)
	{
		return Owner->PrimaryActorTick.bStartWithTickEnabled;
	}
	// Return true here, so it'll work nicely with subclassing
	return true;
}
#endif // WITH_EDITOR
