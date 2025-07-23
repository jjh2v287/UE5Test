// Copyright 2021 Marcin Swiderski. All Rights Reserved.

#include "TickOptToolkitSubsystem.h"
#include "TickOptToolkit.h"
#include "TickOptToolkitTargetComponent.h"
#include "Rob.h"
#include "Algo/RandomShuffle.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/EngineVersionComparison.h"

#if UE_VERSION_OLDER_THAN(5, 5, 0)
ROB_DEFINE_VAR(AActor, LastRenderTime, float);
#else
ROB_DEFINE_VAR(AActor, LastRenderTime, FActorLastRenderTime);
#endif

//
// FTickFocusCache
//

FTickFocusCache::FTickFocusCache(const UTickOptToolkitFocusComponent* Focus)
	: Location(Focus->GetComponentLocation())
	, Forward(Focus->GetComponentQuat().GetForwardVector())
	, Layer(1u << static_cast<uint8>(Focus->GetFocusLayer()))
{}

//
// FTickOptToolkitSphereRangeCache
//

FTickOptToolkitSphereRangeCache::FTickOptToolkitSphereRangeCache(const UTickOptToolkitTargetComponent* Target)
	: Location(Target->GetComponentLocation())
	, LayerMask(Target->GetFocusLayerMask())
{
	UpdateRadius(Target);
}

void FTickOptToolkitSphereRangeCache::UpdateRadius(const UTickOptToolkitTargetComponent* Target)
{
	const int Zone = Target->GetTickZone();
	check(Zone == 0 || Zone == Target->GetMidZonesNum() + 1);

	float Radius = Target->GetSphereRadius();
	if (Zone == 0)
	{
		Radius += Target->GetBufferSize();
	}
	else
	{
		Radius += Target->GetZoneInnerSize(Zone);
	}
	
	RadiusSqr = FMath::Square(Radius * Target->GetDistanceScale());
}

bool FTickOptToolkitSphereRangeCache::IsInRange(const FTickFocusCache& Focus) const
{
	return FVector::DistSquared(Focus.Location, Location) <= RadiusSqr;
}

//
// FTickOptToolkitSphereZoneCache
//

FTickOptToolkitSphereZoneCache::FTickOptToolkitSphereZoneCache(const UTickOptToolkitTargetComponent* Target)
	: Location(Target->GetComponentLocation())
	, LayerMask(Target->GetFocusLayerMask())
{
	UpdateRadius(Target);
}

void FTickOptToolkitSphereZoneCache::UpdateRadius(const UTickOptToolkitTargetComponent* Target)
{
	const uint32 Zone = Target->GetTickZone();
	check(Zone > 0 && Zone <= Target->GetMidZonesNum());
	
	const float InnerRadius = Target->GetSphereRadius() + Target->GetZoneInnerSize(Zone);
	const float OuterRadius = InnerRadius + Target->GetMidZoneSizes()[Zone - 1] + Target->GetBufferSize();
	const float DistanceScale = Target->GetDistanceScale();
	
	InnerRadiusSqr = FMath::Square(InnerRadius * DistanceScale);
	OuterRadiusSqr = FMath::Square(OuterRadius * DistanceScale);
}

int FTickOptToolkitSphereZoneCache::GetZoneDiff(const FTickFocusCache& Focus) const
{
	const float DistSqr = FVector::DistSquared(Focus.Location, Location);
	if (DistSqr <= InnerRadiusSqr)
	{
		return -1;
	}
	if (DistSqr > OuterRadiusSqr)
	{
		return 1;
	}
	return 0;
}

//
// FTickOptToolkitBoxRangeCache
//

FTickOptToolkitBoxRangeCache::FTickOptToolkitBoxRangeCache(const UTickOptToolkitTargetComponent* Target)
	: Location(Target->GetComponentLocation())
	, LayerMask(Target->GetFocusLayerMask())
{
	UpdateExtents(Target);
	FMath::SinCos(&Sin, &Cos, FMath::DegreesToRadians(-Target->GetBoxRotation()));
}

void FTickOptToolkitBoxRangeCache::UpdateExtents(const UTickOptToolkitTargetComponent* Target)
{
	const int Zone = Target->GetTickZone();
	check(Zone == 0 || Zone == Target->GetMidZonesNum() + 1);

	Extents = Target->GetBoxExtents();
	if (Zone == 0)
	{
		Extents = Extents + Target->GetBufferSize();
	}
	else
	{
		Extents = Extents + Target->GetZoneInnerSize(Zone);
	}
	Extents *= Target->GetDistanceScale();
}

bool FTickOptToolkitBoxRangeCache::IsInRange(const FTickFocusCache& Focus) const
{
	FVector RelPosition = Focus.Location - Location;
	RelPosition = FVector(
		FMath::Abs(Cos * RelPosition.X - Sin * RelPosition.Y),
		FMath::Abs(Sin * RelPosition.X + Cos * RelPosition.Y),
		FMath::Abs(RelPosition.Z));
	return RelPosition.X <= Extents.X && RelPosition.Y <= Extents.Y && RelPosition.Z <= Extents.Z;
}

//
// FTickOptToolkitBoxZoneCache
//

FTickOptToolkitBoxZoneCache::FTickOptToolkitBoxZoneCache(const UTickOptToolkitTargetComponent* Target)
	: Location(Target->GetComponentLocation())
	, LayerMask(Target->GetFocusLayerMask())
{
	UpdateExtents(Target);
	FMath::SinCos(&Sin, &Cos, FMath::DegreesToRadians(-Target->GetBoxRotation()));
}

void FTickOptToolkitBoxZoneCache::UpdateExtents(const UTickOptToolkitTargetComponent* Target)
{
	const uint32 Zone = Target->GetTickZone();
	check(Zone > 0 && Zone <= Target->GetMidZonesNum());

	const float DistanceScale = Target->GetDistanceScale();
	
	InnerExtents = (Target->GetBoxExtents() + Target->GetZoneInnerSize(Zone)) * DistanceScale;
	OuterSize = (Target->GetMidZoneSizes()[Zone - 1] + Target->GetBufferSize()) * DistanceScale;
}

int FTickOptToolkitBoxZoneCache::GetZoneDiff(const FTickFocusCache& Focus) const
{
	FVector RelPosition = Focus.Location - Location;
	RelPosition = FVector(
		FMath::Abs(Cos * RelPosition.X - Sin * RelPosition.Y),
		FMath::Abs(Sin * RelPosition.X + Cos * RelPosition.Y),
		FMath::Abs(RelPosition.Z));
	
	const FVector InnerDiff = RelPosition - InnerExtents;
	if (InnerDiff.X <= 0.0f && InnerDiff.Y <= 0.0f && InnerDiff.Z <= 0.0f)
	{
		return -1;
	}
	if (InnerDiff.X > OuterSize || InnerDiff.Y > OuterSize || InnerDiff.Z > OuterSize)
	{
		return 1;
	}
	return 0;
}

//
// FTickByVisFrontCache
//

FTickByVisFrontCache::FTickByVisFrontCache(const UTickOptToolkitTargetComponent* Target)
	: Location(Target->GetComponentLocation())
	, LayerMask(Target->GetFocusLayerMask())
{}

bool FTickByVisFrontCache::IsVisible(const FTickFocusCache& Focus) const
{
	return FVector::DotProduct(Location - Focus.Location, Focus.Forward) >= 0.0f;
}

//
// UTickOptToolkitSubsystem
//

void UTickOptToolkitSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Settings = UTickOptToolkitSettings::StaticClass()->GetDefaultObject<UTickOptToolkitSettings>();

	BalancingBins.SetNum(FMath::Max(Settings->BalancingFramesNum, 0));
	if (BalancingBins.Num() > 0)
	{
		BalancingBinsPermutation.SetNum(BalancingBins.Num());
		for (int I = 0; I < BalancingBins.Num(); ++I)
		{
			BalancingBinsPermutation[I] = I;
		}
		Algo::RandomShuffle(BalancingBinsPermutation);
		BalancingBinsNextInsert = 0;
		BalancingBinsNextUpdate = 0;
	}
	
	const FConsoleVariableDelegate CVarChangeDelegate = FConsoleVariableDelegate::CreateUObject(this, &UTickOptToolkitSubsystem::OnScalabilityCVarChanged);
	CVarTickOptToolkitTickOptimizationLevel->SetOnChangedCallback(CVarChangeDelegate);
	CVarTickOptToolkitTickDistanceScale->SetOnChangedCallback(CVarChangeDelegate);
}

bool UTickOptToolkitSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UTickOptToolkitSubsystem::RegisterFocus(UTickOptToolkitFocusComponent* Focus)
{
	check(Focus->FocusIndex == INDEX_NONE);

	Focus->FocusIndex = Focuses.Num();
	Focuses.Add(Focus);
}

void UTickOptToolkitSubsystem::UnregisterFocus(UTickOptToolkitFocusComponent* Focus)
{
	check(Focus->FocusIndex != INDEX_NONE);

	const int Index = Focus->FocusIndex;
	Focus->FocusIndex = INDEX_NONE;

	Focuses.RemoveAtSwap(Index);
	if (Index < Focuses.Num())
	{
		Focuses[Index]->FocusIndex = Index;
	}
}

void UTickOptToolkitSubsystem::RegisterTarget(UTickOptToolkitTargetComponent* Target)
{
	if (Target->bTickOptRegistered)
	{
		UE_LOG(LogTickOptToolkit, Warning, TEXT("Trying to register already component '%s'!"), *Target->GetName())
		return;
	}
	Target->bTickOptRegistered = true;
	
	switch (Target->DistanceMode)
	{
	case ETickOptToolkitDistanceMode::None:
		break;
		
	case ETickOptToolkitDistanceMode::Sphere:
		if (IsRange(Target))
		{
			AddSphereRangeTarget(Target);
		}
		else
		{
			AddSphereZoneTarget(Target);
		}
		break;
		
	case ETickOptToolkitDistanceMode::Box:
		if (IsRange(Target))
		{
			AddBoxRangeTarget(Target);
		}
		else
		{
			AddBoxZoneTarget(Target);
		}
		break;
	}

	switch (Target->VisibilityMode)
	{
	case ETickByVisibilityMode::None:
		break;

	case ETickByVisibilityMode::Front:
		AddVisFrontTarget(Target);
		break;

	case ETickByVisibilityMode::Rendered:
		AddVisRenderedTarget(Target);
		break;
	}
}

void UTickOptToolkitSubsystem::UnregisterTarget(UTickOptToolkitTargetComponent* Target)
{
	if (!Target->bTickOptRegistered)
	{
		UE_LOG(LogTickOptToolkit, Warning, TEXT("Trying to unregister not registered component '%s'!"), *Target->GetName())
		return;
	}
	Target->bTickOptRegistered = false;

	switch (Target->DistanceMode)
	{
	case ETickOptToolkitDistanceMode::None:
		break;
		
	case ETickOptToolkitDistanceMode::Sphere:
		if (IsRange(Target))
		{
			RemoveSphereRangeTarget(Target);
		}
		else
		{
			RemoveSphereZoneTarget(Target);
		}
		break;

	case ETickOptToolkitDistanceMode::Box:
		if (IsRange(Target))
		{
			RemoveBoxRangeTarget(Target);
		}
		else
		{
			RemoveBoxZoneTarget(Target);
		}
		break;
	}

	switch (Target->VisibilityMode)
	{
	case ETickByVisibilityMode::None:
		break;

	case ETickByVisibilityMode::Front:
		RemoveVisFrontTarget(Target);
		break;

	case ETickByVisibilityMode::Rendered:
		RemoveVisRenderedTarget(Target);
		break;
	}

	UpdatedTargets.RemoveSingleSwap(Target);
}

void UTickOptToolkitSubsystem::UpdateTargetLocation(const UTickOptToolkitTargetComponent* Target)
{
	check(Target->bTickOptRegistered);

	const int Index = Target->TickByZoneIndex;
	if (Index != INDEX_NONE)
	{
		const FVector Location = Target->GetComponentLocation();

		switch (Target->DistanceMode)
		{
		case ETickOptToolkitDistanceMode::None:
			break;
		
		case ETickOptToolkitDistanceMode::Sphere:
			if (IsRange(Target))
			{
				SphereRangeCaches[Index].Location = Location;
			}
			else
			{
				SphereZoneCaches[Index].Location = Location;
			}
			break;
		
		case ETickOptToolkitDistanceMode::Box:
			if (IsRange(Target))
			{
				BoxRangeCaches[Index].Location = Location;
			}
			else
			{
				BoxZoneCaches[Index].Location = Location;
			}
			break;
		}
	}
}

void UTickOptToolkitSubsystem::UpdateTargetLayerMask(const UTickOptToolkitTargetComponent* Target)
{
	check(Target->bTickOptRegistered);

	const uint16 LayerMask = Target->GetFocusLayerMask();

	const int ZoneIndex = Target->TickByZoneIndex;
	if (ZoneIndex != INDEX_NONE)
	{
		switch (Target->DistanceMode)
		{
		case ETickOptToolkitDistanceMode::None:
			break;
		
		case ETickOptToolkitDistanceMode::Sphere:
			if (IsRange(Target))
			{
				SphereRangeCaches[ZoneIndex].LayerMask = LayerMask;
			}
			else
			{
				SphereZoneCaches[ZoneIndex].LayerMask = LayerMask;
			}
			break;
		
		case ETickOptToolkitDistanceMode::Box:
			if (IsRange(Target))
			{
				BoxRangeCaches[ZoneIndex].LayerMask = LayerMask;
			}
			else
			{
				BoxZoneCaches[ZoneIndex].LayerMask = LayerMask;
			}
			break;
		}
	}
	
	const int VisIndex = Target->TickByVisIndex;
	if (VisIndex != INDEX_NONE && Target->VisibilityMode == ETickByVisibilityMode::Front)
	{
		VisFrontCaches[VisIndex].LayerMask = LayerMask; 
	}
}

void UTickOptToolkitSubsystem::BalanceTargetTickInterval(UTickOptToolkitTargetComponent* Target)
{
	if (BalancingBins.Num() > 0)
	{
		Target->BalancingBinIndex = BalancingBinsPermutation[BalancingBinsNextInsert]; 
		BalancingBins[Target->BalancingBinIndex].Add(Target);
		BalancingBinsNextInsert = (BalancingBinsNextInsert + 1) % BalancingBins.Num();
	}
}

void UTickOptToolkitSubsystem::CancelBalancingTargetTickInterval(UTickOptToolkitTargetComponent* Target)
{
	check(Target->IsBalancingTickInterval());
	BalancingBins[Target->BalancingBinIndex].RemoveSingleSwap(Target);
	Target->BalancingBinIndex = INDEX_NONE;
}

ETickableTickType UTickOptToolkitSubsystem::GetTickableTickType() const
{
	if (IsTemplate())
	{
		return ETickableTickType::Never;
	}
	return ETickableTickType::Always;
}

TStatId UTickOptToolkitSubsystem::GetStatId() const
{
	return GET_STATID(STAT_TickOptToolkit);
}

template <bool bMultipleFocuses, bool bSupportLayers, typename CacheType>
void UTickOptToolkitSubsystem::UpdateTargetsInRange(const TTickFocusCachesArray& FocusCaches, const TArray<UTickOptToolkitTargetComponent*>& Targets, const TArray<CacheType>& Caches, TArray<bool>& bInRange)
{
	for (int I = 0; I < Targets.Num(); ++I)
	{
		const CacheType& Cache = Caches[I];
		
		bool bInRangeNew = (!bSupportLayers || (FocusCaches[0].Layer & Cache.LayerMask) != 0) ? Cache.IsInRange(FocusCaches[0]) : false;
		if (bMultipleFocuses)
		{
			for (int FI = 1; FI < FocusCaches.Num(); ++FI)
			{
				if (!bSupportLayers || (FocusCaches[FI].Layer & Cache.LayerMask) != 0)
				{
					bInRangeNew = bInRangeNew || Cache.IsInRange(FocusCaches[FI]);
				}
			}
		}
			
		if (bInRangeNew != bInRange[I])
		{
			bInRange[I] = bInRangeNew;
			UpdateZone(Targets[I], bInRangeNew ? -1 : 1);
		}
	}
}

template <bool bMultipleFocuses, bool bSupportLayers, typename CacheType>
void UTickOptToolkitSubsystem::UpdateTargetsZoneDiff(const TTickFocusCachesArray& FocusCaches, const TArray<UTickOptToolkitTargetComponent*>& Targets, const TArray<CacheType>& Caches)
{
	for (int I = 0; I < Targets.Num(); ++I)
	{
		const CacheType& Cache = Caches[I];
			
		int ZoneDiff = (!bSupportLayers || (FocusCaches[0].Layer & Cache.LayerMask) != 0) ? Cache.GetZoneDiff(FocusCaches[0]) : 0;
		if (bMultipleFocuses)
		{
			for (int FI = 1; FI < FocusCaches.Num(); ++FI)
			{
				if (!bSupportLayers || (FocusCaches[FI].Layer & Cache.LayerMask) != 0)
				{
					ZoneDiff = FMath::Min(ZoneDiff, Cache.GetZoneDiff(FocusCaches[FI]));
				}
			}
		}
			
		if (ZoneDiff != 0)
		{
			UpdateZone(Targets[I], ZoneDiff);
		}
	}
}

template <bool bMultipleFocuses, bool bSupportLayers, typename CacheType>
void UTickOptToolkitSubsystem::UpdateTargetsVisibility(const TTickFocusCachesArray& FocusCaches, const TArray<UTickOptToolkitTargetComponent*>& Targets, const TArray<CacheType>& Caches, TArray<bool>& bVisible)
{
	for (int I = 0; I < Targets.Num(); ++I)
	{
		const CacheType& Cache = Caches[I];
			
		bool bVisibleNew = (!bSupportLayers || (FocusCaches[0].Layer & Cache.LayerMask) != 0) ? Cache.IsVisible(FocusCaches[0]) : false;
		if (bMultipleFocuses)
		{
			for (int FI = 1; FI < FocusCaches.Num(); ++FI)
			{
				if (!bSupportLayers || (FocusCaches[FI].Layer & Cache.LayerMask) != 0)
				{
					bVisibleNew = bVisibleNew || Cache.IsVisible(FocusCaches[FI]);
				}
			}
		}
			
		if (bVisibleNew != bVisible[I])
		{
			bVisible[I] = bVisibleNew;
			UpdateVisibility(Targets[I], bVisibleNew);
		}
	}
}

void UTickOptToolkitSubsystem::UpdateRenderVisibility()
{
	if (const UWorld* World = GetWorld())
	{
		const float RenderTimeThreshold = FMath::Max(0.1f, World->GetDeltaSeconds() + KINDA_SMALL_NUMBER);
		const float RenderTimeTest = World->GetTimeSeconds() - RenderTimeThreshold; 
			
		for (int I = 0; I < VisRenderedTargets.Num(); ++I)
		{
#if UE_VERSION_OLDER_THAN(5, 6, 0)
			const bool bVisibleNew = *VisRenderedLastRenderTime[I] >= RenderTimeTest;
#else
			const bool bVisibleNew = VisRenderedLastRenderTime[I]->GetLastRenderTime() >= RenderTimeTest;
#endif
			if (bVisibleNew != bVisibleRendered[I])
			{
				bVisibleRendered[I] = bVisibleNew;
				UpdateVisibility(VisRenderedTargets[I], bVisibleNew);
			}
		}
	}	
}

void UTickOptToolkitSubsystem::Tick(float DeltaTime)
{
	TTickFocusCachesArray FocusCaches;
	for (UTickOptToolkitFocusComponent* Focus : Focuses)
	{
		if (Focus->ShouldTrack())
		{
			FocusCaches.Emplace(Focus);
		}
	}
	
	if (FocusCaches.Num() > 0)
	{
#define UpdateTargets(bMultipleFocuses, bSupportLayers) \
	UpdateTargetsInRange<bMultipleFocuses, bSupportLayers>(FocusCaches, SphereRangeTargets, SphereRangeCaches, bSpheresInRange); \
	UpdateTargetsZoneDiff<bMultipleFocuses, bSupportLayers>(FocusCaches, SphereZoneTargets, SphereZoneCaches); \
	UpdateTargetsInRange<bMultipleFocuses, bSupportLayers>(FocusCaches, BoxRangeTargets, BoxRangeCaches, bBoxesInRange); \
	UpdateTargetsZoneDiff<bMultipleFocuses, bSupportLayers>(FocusCaches, BoxZoneTargets, BoxZoneCaches); \
	UpdateTargetsVisibility<bMultipleFocuses, bSupportLayers>(FocusCaches, VisFrontTargets, VisFrontCaches, bVisibleFront)
		
		if (Settings->bSupportFocusLayers)
		{
			if (FocusCaches.Num() == 1)
			{
				UpdateTargets(false, true);
			}
			else
			{
				UpdateTargets(true, true);
			}
		}
		else
		{
			if (FocusCaches.Num() == 1)
			{
				UpdateTargets(false, false);
			}
			else
			{
				UpdateTargets(true, false);
			}
		}
#undef UpdateTargets

		UpdateRenderVisibility();
	
		for (UTickOptToolkitTargetComponent* Target : UpdatedTargets)
		{
			if (Target->bZoneUpdated)
			{
				UpdateZoneCache(Target);
			}
			Target->OnUpdated();
		}
		UpdatedTargets.Reset();
	}

	if (BalancingBins.Num() > 0)
	{
		for (UTickOptToolkitTargetComponent* Target : BalancingBins[BalancingBinsNextUpdate])
		{
			Target->OnBalanceTickInterval();
			Target->BalancingBinIndex = INDEX_NONE;
		}
		BalancingBins[BalancingBinsNextUpdate].Reset();
		BalancingBinsNextUpdate = (BalancingBinsNextUpdate + 1) % BalancingBins.Num();
	}
}

void UTickOptToolkitSubsystem::OnScalabilityCVarChanged(IConsoleVariable*)
{
	for (UTickOptToolkitTargetComponent* Target : SphereRangeTargets)
	{
		Target->CurrentOptimizationLevel = Target->GetCurrentOptimizationLevel(true);
		SphereRangeCaches[Target->TickByZoneIndex].UpdateRadius(Target);
	}
	for (UTickOptToolkitTargetComponent* Target : SphereZoneTargets)
	{
		Target->CurrentOptimizationLevel = Target->GetCurrentOptimizationLevel(true);
		SphereZoneCaches[Target->TickByZoneIndex].UpdateRadius(Target);
	}
	for (UTickOptToolkitTargetComponent* Target : BoxRangeTargets)
	{
		Target->CurrentOptimizationLevel = Target->GetCurrentOptimizationLevel(true);
		BoxRangeCaches[Target->TickByZoneIndex].UpdateExtents(Target);
	}
	for (UTickOptToolkitTargetComponent* Target : BoxZoneTargets)
	{
		Target->CurrentOptimizationLevel = Target->GetCurrentOptimizationLevel(true);
		BoxZoneCaches[Target->TickByZoneIndex].UpdateExtents(Target);
	}
}

bool UTickOptToolkitSubsystem::IsRange(const UTickOptToolkitTargetComponent* Target)
{
	return Target->TickZone == 0 || Target->TickZone == Target->MidZoneSizes.Num() + 1;
}

void UTickOptToolkitSubsystem::AddSphereRangeTarget(UTickOptToolkitTargetComponent* Target)
{
	check(Target->TickByZoneIndex == INDEX_NONE);
	
	Target->TickByZoneIndex = SphereRangeTargets.Num();
	SphereRangeTargets.Add(Target);
	SphereRangeCaches.Add(Target);
	bSpheresInRange.Add(Target->TickZone == 0);
}

void UTickOptToolkitSubsystem::RemoveSphereRangeTarget(UTickOptToolkitTargetComponent* Target)
{
	check(Target->TickByZoneIndex != INDEX_NONE);
	
	const int Index = Target->TickByZoneIndex;
	Target->TickByZoneIndex = INDEX_NONE;
	
	SphereRangeTargets.RemoveAtSwap(Index);
	SphereRangeCaches.RemoveAtSwap(Index);
	bSpheresInRange.RemoveAtSwap(Index);
	if (Index < SphereRangeTargets.Num())
	{
		SphereRangeTargets[Index]->TickByZoneIndex = Index;
	}
}

void UTickOptToolkitSubsystem::AddSphereZoneTarget(UTickOptToolkitTargetComponent* Target)
{
	check(Target->TickByZoneIndex == INDEX_NONE);
	
	Target->TickByZoneIndex = SphereZoneTargets.Num();
	SphereZoneTargets.Add(Target);
	SphereZoneCaches.Add(Target);
}

void UTickOptToolkitSubsystem::RemoveSphereZoneTarget(UTickOptToolkitTargetComponent* Target)
{
	check(Target->TickByZoneIndex != INDEX_NONE);
	
	const int Index = Target->TickByZoneIndex;
	Target->TickByZoneIndex = INDEX_NONE;
	
	SphereZoneTargets.RemoveAtSwap(Index);
	SphereZoneCaches.RemoveAtSwap(Index);
	if (Index < SphereZoneTargets.Num())
	{
		SphereZoneTargets[Index]->TickByZoneIndex = Index;
	}
}

void UTickOptToolkitSubsystem::AddBoxRangeTarget(UTickOptToolkitTargetComponent* Target)
{
	check(Target->TickByZoneIndex == INDEX_NONE);
	
	Target->TickByZoneIndex = BoxRangeTargets.Num();
	BoxRangeTargets.Add(Target);
	BoxRangeCaches.Add(Target);
	bBoxesInRange.Add(Target->TickZone == 0);
}

void UTickOptToolkitSubsystem::RemoveBoxRangeTarget(UTickOptToolkitTargetComponent* Target)
{
	check(Target->TickByZoneIndex != INDEX_NONE);
	
	const int Index = Target->TickByZoneIndex;
	Target->TickByZoneIndex = INDEX_NONE;

	BoxRangeTargets.RemoveAtSwap(Index);
	BoxRangeCaches.RemoveAtSwap(Index);
	bBoxesInRange.RemoveAtSwap(Index);
	if (Index < BoxRangeTargets.Num())
	{
		BoxRangeTargets[Index]->TickByZoneIndex = Index;
	}
}

void UTickOptToolkitSubsystem::AddBoxZoneTarget(UTickOptToolkitTargetComponent* Target)
{
	check(Target->TickByZoneIndex == INDEX_NONE);
	
	Target->TickByZoneIndex = BoxZoneTargets.Num();
	BoxZoneTargets.Add(Target);
	BoxZoneCaches.Add(Target);
}

void UTickOptToolkitSubsystem::RemoveBoxZoneTarget(UTickOptToolkitTargetComponent* Target)
{
	check(Target->TickByZoneIndex != INDEX_NONE);
	
	const int Index = Target->TickByZoneIndex;
	Target->TickByZoneIndex = INDEX_NONE;

	BoxZoneTargets.RemoveAtSwap(Index);
	BoxZoneCaches.RemoveAtSwap(Index);
	if (Index < BoxZoneTargets.Num())
	{
		BoxZoneTargets[Index]->TickByZoneIndex = Index;
	}
}

void UTickOptToolkitSubsystem::AddVisFrontTarget(UTickOptToolkitTargetComponent* Target)
{
	check(Target->TickByVisIndex == INDEX_NONE);
	
	Target->TickByVisIndex = VisFrontTargets.Num();
	VisFrontTargets.Add(Target);
	VisFrontCaches.Add(Target);
	bVisibleFront.Add(Target->bTickVisible);
}

void UTickOptToolkitSubsystem::RemoveVisFrontTarget(UTickOptToolkitTargetComponent* Target)
{
	check(Target->TickByVisIndex != INDEX_NONE);
	
	const int Index = Target->TickByVisIndex;
	Target->TickByVisIndex = INDEX_NONE;

	VisFrontTargets.RemoveAtSwap(Index);
	VisFrontCaches.RemoveAtSwap(Index);
	bVisibleFront.RemoveAtSwap(Index);
	if (Index < VisFrontTargets.Num())
	{
		VisFrontTargets[Index]->TickByVisIndex = Index;
	}
}

void UTickOptToolkitSubsystem::AddVisRenderedTarget(UTickOptToolkitTargetComponent* Target)
{
	check(Target->TickByVisIndex == INDEX_NONE);
	
	Target->TickByVisIndex = VisRenderedTargets.Num();
	VisRenderedTargets.Add(Target);
#if UE_VERSION_OLDER_THAN(5, 5, 0)
	VisRenderedLastRenderTime.Add(&(Target->GetOwner()->*RobAccess(AActor, LastRenderTime)));
#elif UE_VERSION_OLDER_THAN(5, 6, 0)
	VisRenderedLastRenderTime.Add(&(Target->GetOwner()->*RobAccess(AActor, LastRenderTime)).LastRenderTime);
#else
	VisRenderedLastRenderTime.Add(&(Target->GetOwner()->*RobAccess(AActor, LastRenderTime)));
#endif
	bVisibleRendered.Add(Target->bTickVisible);
}

void UTickOptToolkitSubsystem::RemoveVisRenderedTarget(UTickOptToolkitTargetComponent* Target)
{
	check(Target->TickByVisIndex != INDEX_NONE);
	
	const int Index = Target->TickByVisIndex;
	Target->TickByVisIndex = INDEX_NONE;

	VisRenderedTargets.RemoveAtSwap(Index);
	VisRenderedLastRenderTime.RemoveAtSwap(Index);
	bVisibleRendered.RemoveAtSwap(Index);
	if (Index < VisRenderedTargets.Num())
	{
		VisRenderedTargets[Index]->TickByVisIndex = Index;
	}
}

void UTickOptToolkitSubsystem::UpdateZone(UTickOptToolkitTargetComponent* Target, int ZoneDiff)
{
	Target->TickZone += ZoneDiff;
	Target->bZoneUpdated = true;
	UpdatedTargets.Add(Target);
	
	check(Target->TickZone >= 0 && Target->TickZone <= Target->MidZoneSizes.Num() + 1);
}

void UTickOptToolkitSubsystem::UpdateVisibility(UTickOptToolkitTargetComponent* Target, bool bVisible)
{
	Target->bTickVisible = bVisible;
	Target->bVisUpdated = true;
	if (!Target->bZoneUpdated)
	{
		UpdatedTargets.Add(Target);
	}
}

void UTickOptToolkitSubsystem::UpdateZoneCache(UTickOptToolkitTargetComponent* Target)
{
	const int Index = Target->TickByZoneIndex;
	const int MidZonesNum = Target->MidZoneSizes.Num(); 
	switch (Target->DistanceMode)
	{
	case ETickOptToolkitDistanceMode::None:
		break;

	case ETickOptToolkitDistanceMode::Sphere:
		if (MidZonesNum == 0)
		{
			SphereRangeCaches[Index].UpdateRadius(Target);
		}
		else if (Target->TickZone == 0 || Target->TickZone == MidZonesNum + 1)
		{
			RemoveSphereZoneTarget(Target);
			AddSphereRangeTarget(Target);
		}
		else if (Index < SphereRangeTargets.Num() && SphereRangeTargets[Index] == Target)
		{
			RemoveSphereRangeTarget(Target);
			AddSphereZoneTarget(Target);
		}
		else
		{
			SphereZoneCaches[Index].UpdateRadius(Target);
		}
		break;

	case ETickOptToolkitDistanceMode::Box:
		if (MidZonesNum == 0)
		{
			BoxRangeCaches[Index].UpdateExtents(Target);
		}
		else if (Target->TickZone == 0 || Target->TickZone == MidZonesNum + 1)
		{
			RemoveBoxZoneTarget(Target);
			AddBoxRangeTarget(Target);
		}
		else if (Index < BoxRangeTargets.Num() && BoxRangeTargets[Index] == Target)
		{
			RemoveBoxRangeTarget(Target);
			AddBoxZoneTarget(Target);
		}
		else
		{
			BoxZoneCaches[Index].UpdateExtents(Target);
		}
		break;
	}
}
