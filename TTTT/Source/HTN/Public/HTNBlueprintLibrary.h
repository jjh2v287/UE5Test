// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "HTNTypes.h"
#include "Tasks/HTNTask_BlueprintBase.h"
#include "Utility/HTNLocationSource.h"

#include "AIController.h"

#include "HTNBlueprintLibrary.generated.h"

UCLASS()
class HTN_API UHTNBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	// Sets up the given AIController to use an HTNComponent as its BrainComponent.
	// Won't work if the AIController already has a BrainComponent that isn't an HTNComponent.
	// If the AIController already has an HTNComponent as its BrainComponent, simply returns that.
	UFUNCTION(BlueprintCallable, Category = "AI|HTN", Meta = (DefaultToSelf = "AIController"))
	static UHTNComponent* SetUpHTNComponent(AAIController* AIController);

	UFUNCTION(BlueprintCallable, Category = "AI|HTN", Meta = (DefaultToSelf = "AIController"))
	static bool RunHTN(AAIController* AIController, class UHTN* HTNAsset);

	// Find all Components in the world of the specified class. The components
	// This is a slow operation, use with caution (e.g., do not use every frame).
	// @param OutComponents Output array of Components of the specified class.
	// @param ComponentClass Class of Component to find. Must be specified or the result array will be empty.
	UFUNCTION(BlueprintCallable, Category = "AI|HTN", Meta = (WorldContext = "WorldContextObject", DeterminesOutputType = "ComponentClass", DynamicOutputParam = "OutComponents"))
	static void GetAllComponentsOfClass(TArray<UActorComponent*>& OutComponents, const UObject* WorldContextObject, 
		TSubclassOf<UActorComponent> ComponentClass);

	// Find the closest N components of the specified class to the given location.
	// This is a slow operation, use with caution (e.g., do not use every frame).
	// @param OutComponents Output array of Components of the specified class, in order of increasing distance from Location (closest first).
	// @param ComponentClass Class of Actor to find. Must be specified or the result array will be empty.
	// @param Location Location to find the closest components to.
	// @param MaxNumComponents Maximum number of components to find. If there are fewer components of the specified class in the world, all of them will be returned.
	UFUNCTION(BlueprintCallable, Category = "AI|HTN", Meta = (WorldContext = "WorldContextObject", DeterminesOutputType = "ComponentClass", DynamicOutputParam = "OutComponents"))
	static void GetClosestComponentsOfClass(TArray<UActorComponent*>& OutComponents, const UObject* WorldContextObject, 
		TSubclassOf<UActorComponent> ComponentClass, const FVector& Location, int32 MaxNumComponents = 10);
};

UCLASS(Meta = (RestrictedToClasses = "HTNNode"))
class HTN_API UHTNNodeLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Gets the worldstate of the owner of this HTN node.
	// WARNING: this function only works for Blueprint nodes!
	UFUNCTION(BlueprintPure, Category = "AI|HTN", Meta = (HidePin="Node", DefaultToSelf="Node"))
	static class UWorldStateProxy* GetOwnersWorldState(const UHTNNode* Node);

	UFUNCTION(BlueprintCallable, Category = "AI|HTN|LocationSource", Meta = (HidePin = "Node", DefaultToSelf = "Node", ExpandEnumAsExecs = "OutResult"))
	static FVector GetSingleLocationFromHTNLocationSource(EHTNReturnValueValidity& OutResult,
		const UHTNNode* Node, const FHTNLocationSource& LocationSource);

	UFUNCTION(BlueprintCallable, Category = "AI|HTN|LocationSource", Meta = (HidePin = "Node", DefaultToSelf = "Node", ExpandEnumAsExecs = "OutResult"))
	static void GetMultipleLocationsFromHTNLocationSource(EHTNReturnValueValidity& OutResult, TArray<FVector>& OutLocations,
		const UHTNNode* Node, const FHTNLocationSource& LocationSource);

	// If the given key contains a Vector, returns it.
	// If the given key contains an Actor, returns its location and the actor itself.
	// If the given key contains a Component, returns its location, its actor, and the component itself.
    // Otherwise returns false.
	UFUNCTION(BlueprintCallable, Category="AI|HTN", Meta = (HidePin="Node", DefaultToSelf="Node"))
	static bool GetLocationFromWorldState(UHTNNode* Node, const FBlackboardKeySelector& KeySelector,
		FVector& OutLocation, AActor*& OutActor, UActorComponent*& OutComponent);

	// A helper returning the value of SelfLocation key in the worldstate.
	// This key is automatically added to every blackboard asset used for HTN planning and is automatically updated.
	UFUNCTION(BlueprintPure, Category = "AI|HTN", Meta = (HidePin="Node", DefaultToSelf="Node"))
	static FVector GetSelfLocationFromWorldState(UHTNNode* Node);
	
	UFUNCTION(BlueprintPure, Category="AI|HTN", Meta = (HidePin="Node", DefaultToSelf="Node"))
	static UObject* GetWorldStateValueAsObject(UHTNNode* Node, const FBlackboardKeySelector& Key);

	UFUNCTION(BlueprintPure, Category="AI|HTN", Meta = (HidePin="Node", DefaultToSelf="Node"))
	static AActor* GetWorldStateValueAsActor(UHTNNode* Node, const FBlackboardKeySelector& Key);

	UFUNCTION(BlueprintPure, Category="AI|HTN", Meta = (HidePin="Node", DefaultToSelf="Node"))
	static UClass* GetWorldStateValueAsClass(UHTNNode* Node, const FBlackboardKeySelector& Key);

	UFUNCTION(BlueprintPure, Category="AI|HTN", Meta = (HidePin="Node", DefaultToSelf="Node"))
	static uint8 GetWorldStateValueAsEnum(UHTNNode* Node, const FBlackboardKeySelector& Key);

	UFUNCTION(BlueprintPure, Category="AI|HTN", Meta = (HidePin="Node", DefaultToSelf="Node"))
	static int32 GetWorldStateValueAsInt(UHTNNode* Node, const FBlackboardKeySelector& Key);

	UFUNCTION(BlueprintPure, Category="AI|HTN", Meta = (HidePin="Node", DefaultToSelf="Node"))
	static float GetWorldStateValueAsFloat(UHTNNode* Node, const FBlackboardKeySelector& Key);

	UFUNCTION(BlueprintPure, Category="AI|HTN", Meta = (HidePin="Node", DefaultToSelf="Node"))
	static bool GetWorldStateValueAsBool(UHTNNode* Node, const FBlackboardKeySelector& Key);

	UFUNCTION(BlueprintPure, Category="AI|HTN", Meta = (HidePin="Node", DefaultToSelf="Node"))
	static FString GetWorldStateValueAsString(UHTNNode* Node, const FBlackboardKeySelector& Key);

	UFUNCTION(BlueprintPure, Category="AI|HTN", Meta = (HidePin="Node", DefaultToSelf="Node"))
	static FName GetWorldStateValueAsName(UHTNNode* Node, const FBlackboardKeySelector& Key);

	UFUNCTION(BlueprintPure, Category="AI|HTN", Meta = (HidePin="Node", DefaultToSelf="Node"))
	static FVector GetWorldStateValueAsVector(UHTNNode* Node, const FBlackboardKeySelector& Key);

	UFUNCTION(BlueprintPure, Category ="AI|HTN", Meta = (HidePin="Node", DefaultToSelf="Node"))
	static FRotator GetWorldStateValueAsRotator(UHTNNode* Node, const FBlackboardKeySelector& Key);

	UFUNCTION(BlueprintCallable, Category = "AI|HTN", Meta = (HidePin = "Node", DefaultToSelf = "Node"))
	static void SetWorldStateSelfLocation(UHTNNode* Node, const FVector& NewSelfLocation);

	UFUNCTION(BlueprintCallable, Category="AI|HTN", Meta = (HidePin="Node", DefaultToSelf="Node"))
	static void SetWorldStateValueAsObject(UHTNNode* Node, const FBlackboardKeySelector& Key, UObject* Value);

	UFUNCTION(BlueprintCallable, Category="AI|HTN", Meta = (HidePin="Node", DefaultToSelf="Node"))
	static void SetWorldStateValueAsClass(UHTNNode* Node, const FBlackboardKeySelector& Key, UClass* Value);

	UFUNCTION(BlueprintCallable, Category="AI|HTN", Meta = (HidePin="Node", DefaultToSelf="Node"))
	static void SetWorldStateValueAsEnum(UHTNNode* Node, const FBlackboardKeySelector& Key, uint8 Value);

	UFUNCTION(BlueprintCallable, Category="AI|HTN", Meta = (HidePin="Node", DefaultToSelf="Node"))
	static void SetWorldStateValueAsInt(UHTNNode* Node, const FBlackboardKeySelector& Key, int32 Value);

	UFUNCTION(BlueprintCallable, Category="AI|HTN", Meta = (HidePin="Node", DefaultToSelf="Node"))
	static void SetWorldStateValueAsFloat(UHTNNode* Node, const FBlackboardKeySelector& Key, float Value);

	UFUNCTION(BlueprintCallable, Category="AI|HTN", Meta = (HidePin="Node", DefaultToSelf="Node"))
	static void SetWorldStateValueAsBool(UHTNNode* Node, const FBlackboardKeySelector& Key, bool Value);

	UFUNCTION(BlueprintCallable, Category="AI|HTN", Meta = (HidePin="Node", DefaultToSelf="Node"))
	static void SetWorldStateValueAsString(UHTNNode* Node, const FBlackboardKeySelector& Key, FString Value);

	UFUNCTION(BlueprintCallable, Category="AI|HTN", Meta = (HidePin="Node", DefaultToSelf="Node"))
	static void SetWorldStateValueAsName(UHTNNode* Node, const FBlackboardKeySelector& Key, FName Value);

	UFUNCTION(BlueprintCallable, Category="AI|HTN", Meta = (HidePin="Node", DefaultToSelf="Node"))
	static void SetWorldStateValueAsVector(UHTNNode* Node, const FBlackboardKeySelector& Key, FVector Value);

	UFUNCTION(BlueprintCallable, Category="AI|HTN", Meta = (HidePin = "Node", DefaultToSelf = "Node"))
	static void SetWorldStateValueAsRotator(UHTNNode* Node, const FBlackboardKeySelector& Key, FRotator Value);

	// Resets indicated value to "not set" value, based on the key type
	UFUNCTION(BlueprintCallable, Category="AI|HTN", Meta = (HidePin = "Node", DefaultToSelf = "Node"))
	static void ClearWorldStateValue(UHTNNode* Node, const FBlackboardKeySelector& Key);

	// Copies the value of the SourceKey into the DestinationKey and returns true if succeeded.
	UFUNCTION(BlueprintCallable, Category = "AI|HTN", Meta = (HidePin = "Node", DefaultToSelf = "Node", ReturnDisplayName = "Success"))
	static bool CopyWorldStateValue(UHTNNode* Node, const FBlackboardKeySelector& SourceKey, const FBlackboardKeySelector& TargetKey);
};
