// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "HTNTypes.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "HTNLocationSource.generated.h"

UENUM()
enum class EHTNLocationExtractionMethod : uint8
{
	// The location in the key or the location of the actor in the key.
	Location,
	// The location on the ground of the actor in the key.
	NavAgentLocation,
	// What is returned by the Actor->GetActorEyesViewPoint() function (typically the pawn's head).
	ActorEyesViewPoint,
	// Returned by an instance of UHTNCustomLocationSource.
	Custom
};

// A helper for providing a location (or multiple locations) to an HTN node. 
// This is more flexible than using a Blackboard Key Selector as it allows for 
// getting the head location, the navigaiton location, or getting locations via custom code.
USTRUCT(BlueprintType)
struct HTN_API FHTNLocationSource
{
	GENERATED_BODY()

	FHTNLocationSource();
	
	// To be called in the constructor of the node that contains this struct. 
	// See UHTNDecorator_DistanceCheck for an example.
	void InitializeBlackboardKeySelector(UObject* Owner, FName PropertyName);
	void InitializeFromAsset(const class UHTN& Asset);
	FString ToString() const;

	// Produces a number of locations (zero or one, unless a custom source is used) from a worldstate proxy.
	int32 GetLocations(TArray<FVector>& OutLocations, const AActor* Owner, const class UWorldStateProxy& WorldStateProxy) const;

	// A helper that calls GetLocations and returns the first item if it exists (InvalidLocation otherwise).
	FVector GetLocation(const AActor* Owner, const class UWorldStateProxy& WorldStateProxy) const;

	UPROPERTY(EditAnywhere, Category = "HTN|LocationSource")
	FBlackboardKeySelector BlackboardKey;

	// How should the location be extracted from the Blackboard Key.
	UPROPERTY(EditAnywhere, Category = "HTN|LocationSource")
	EHTNLocationExtractionMethod LocationExtractionMethod;

	// Make subclasses of HTNCustomLocationSource to provide custom logic for location sources.
	UPROPERTY(EditAnywhere, Instanced, Category = "HTN|LocationSource", Meta = (EditConditionHides, EditCondition = "LocationExtractionMethod == EHTNLocationExtractionMethod::Custom"))
	TObjectPtr<class UHTNCustomLocationSource> CustomSource;

	// Offset to be applied to the extracted location.
	UPROPERTY(EditAnywhere, Category = "HTN|LocationSource")
	FVector Offset;
};

// Make a subclass of this and assign to the CustomSource property of a FHTNLocationSource to use custom logic for providing locations for HTN nodes.
UCLASS(Abstract, BlueprintType, Blueprintable, ClassGroup = AI, DefaultToInstanced, EditInlineNew)
class HTN_API UHTNCustomLocationSource : public UObject
{
	GENERATED_BODY()

public:
	UWorld* GetWorld() const override;

	UFUNCTION(BlueprintNativeEvent, Category = "HTN|LocationSource")
	void ProvideLocations(TArray<FVector>& OutLocations, 
		const AActor* Owner, const class UWorldStateProxy* WorldStateProxy, const FBlackboardKeySelector& BlackboardKeySelector) const;
};
