#include "Utility/HTNLocationSource.h"
#include "HTN.h"
#include "WorldStateProxy.h"

#include "AITypes.h"
#include "AI/Navigation/NavAgentInterface.h"
#include "Algo/RemoveIf.h"
#include "UObject/Package.h"

FHTNLocationSource::FHTNLocationSource() :
	LocationExtractionMethod(EHTNLocationExtractionMethod::Location),
	CustomSource(nullptr),
	Offset(ForceInitToZero)
{}

void FHTNLocationSource::InitializeBlackboardKeySelector(UObject* Owner, FName PropertyName)
{
	BlackboardKey.AddObjectFilter(Owner, PropertyName, AActor::StaticClass());
	BlackboardKey.AddVectorFilter(Owner, PropertyName);
}

void FHTNLocationSource::InitializeFromAsset(const UHTN& Asset)
{
	if (const UBlackboardData* const BBAsset = Asset.BlackboardAsset)
	{
		BlackboardKey.ResolveSelectedKey(*BBAsset);
	}
	else
	{
		UE_LOG(LogHTN, Warning, TEXT("Can't initialize HTNLocationSource due to missing blackboard data."));
		BlackboardKey.InvalidateResolvedKey();
	}
}

FString FHTNLocationSource::ToString() const
{
	TStringBuilder<1024> SB;

	SB << BlackboardKey.SelectedKeyName.ToString();
	if (LocationExtractionMethod != EHTNLocationExtractionMethod::Location)
	{
		SB << TEXT(" (");
		if (LocationExtractionMethod == EHTNLocationExtractionMethod::Custom)
		{
			if (!IsValid(CustomSource))
			{
				SB << TEXT("invalid");
			}
			else
			{
#if WITH_EDITORONLY_DATA
				SB << CustomSource->GetClass()->GetDisplayNameText().ToString();
#else
				SB << GetNameSafe(CustomSource->GetClass());
#endif
			}
		}
		else
		{
			SB << UEnum::GetDisplayValueAsText(LocationExtractionMethod).ToString();
		}
			
		SB << TEXT(")");
	}
	if (Offset != FVector::ZeroVector)
	{
		SB << TEXT(" + ") << Offset.ToCompactString();
	}

	return SB.ToString();
}

int32 FHTNLocationSource::GetLocations(TArray<FVector>& OutLocations, const AActor* Owner, const UWorldStateProxy& WorldStateProxy) const
{
	const int32 OldNumLocations = OutLocations.Num();

	switch (LocationExtractionMethod)
	{
	case EHTNLocationExtractionMethod::Location:
		OutLocations.Add(WorldStateProxy.GetLocation(BlackboardKey));
		break;
	case EHTNLocationExtractionMethod::NavAgentLocation:
		if (const INavAgentInterface* const NavAgent = Cast<INavAgentInterface>(WorldStateProxy.GetValueAsActor(BlackboardKey.SelectedKeyName)))
		{
			OutLocations.Add(NavAgent->GetNavAgentLocation());
		}
		else
		{
			OutLocations.Add(WorldStateProxy.GetLocation(BlackboardKey));
		}
		break;
	case EHTNLocationExtractionMethod::ActorEyesViewPoint:
		if (const AActor* const Actor = WorldStateProxy.GetValueAsActor(BlackboardKey.SelectedKeyName))
		{
			FVector ViewLocation;
			FRotator ViewRotation;
			Actor->GetActorEyesViewPoint(ViewLocation, ViewRotation);
			OutLocations.Add(ViewLocation);
		}
		else
		{
			OutLocations.Add(WorldStateProxy.GetLocation(BlackboardKey));
		}
		break;
	case EHTNLocationExtractionMethod::Custom:
		if (IsValid(CustomSource))
		{
			CustomSource->ProvideLocations(OutLocations, Owner, &WorldStateProxy, BlackboardKey);
		}
		break;
	default:
		checkNoEntry();
		break;
	}

	const int32 NumAddedLocations = OutLocations.Num() - OldNumLocations;
	ensure(NumAddedLocations >= 0);
	if (NumAddedLocations > 0)
	{
		FVector* const OnePastExistingLocations = OutLocations.GetData() + OldNumLocations;

		// Remove invalid items, but don't touch ones that were already in the array.
		const TArrayView<FVector> AddedLocations = MakeArrayView(OnePastExistingLocations, NumAddedLocations);
		const int32 NumAddedLocationsRemaining = Algo::StableRemoveIf(AddedLocations, [](const FVector& Location) { 
			return !FAISystem::IsValidLocation(Location); });
		OutLocations.SetNum(OldNumLocations + NumAddedLocationsRemaining, HTN_DISALLOW_SHRINKING);

		// Apply the Offset to the remaining added locations.
		// OnePastExistingLocations is still valid because SetNum did not reallocate due to bAllowShrinking=false.
		const int32 NumValidLocationsAdded = OutLocations.Num() - OldNumLocations;
		const TArrayView<FVector> AddedValidLocations = MakeArrayView(OnePastExistingLocations, NumValidLocationsAdded);
		for (FVector& AddedValidLocation : AddedValidLocations)
		{
			AddedValidLocation += Offset;
		}
		return NumValidLocationsAdded;
	}

	return 0;
}

FVector FHTNLocationSource::GetLocation(const AActor* Owner, const UWorldStateProxy& WorldStateProxy) const
{
	TArray<FVector> Locations;
	if (GetLocations(Locations, Owner, WorldStateProxy) > 0)
	{
		return Locations[0];
	}

	return FAISystem::InvalidLocation;
}

// Make sure we can add BP nodes that require a world (e.g., debug drawing) even when in editor.
UWorld* UHTNCustomLocationSource::GetWorld() const
{
	if (GetOuter())
	{
		// Special case for when we're in the editor
		if (Cast<UPackage>(GetOuter()))
		{
			// GetOuter should return a UPackage and its Outer is a UWorld
			return Cast<UWorld>(GetOuter()->GetOuter());
		}

		return GetOuter()->GetWorld();
	}

	return nullptr;
}

void UHTNCustomLocationSource::ProvideLocations_Implementation(TArray<FVector>& OutLocations,
	const AActor* Owner, const UWorldStateProxy* WorldStateProxy, const FBlackboardKeySelector& BlackboardKeySelector) const
{
}
