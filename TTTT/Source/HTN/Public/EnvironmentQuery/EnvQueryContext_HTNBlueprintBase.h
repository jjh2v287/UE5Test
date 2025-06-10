// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "EnvironmentQuery/EnvQueryContext.h"
#include "WorldStateProxy.h"

#include "EnvQueryContext_HTNBlueprintBase.generated.h"

// Base class for blueprint environment query contexts that work with HTN planning.
// Use for query contexts which might be used during planning.
// Exactly the same as UEnvQueryContext_BlueprintBase, but provides the GetWorldState function
// that you can use to get the correct worldstate from inside the Provide... functions
// These may still be used for regular (non-planning) EQS queries, but the GetWorldState function will only work
// when querying from an actor that actually uses HTN AI.
UCLASS(Abstract, Blueprintable)
class HTN_API UEnvQueryContext_HTNBlueprintBase : public UEnvQueryContext
{
	GENERATED_BODY()
	
public:
	UEnvQueryContext_HTNBlueprintBase(const FObjectInitializer& Initializer);
	
	// We need to implement GetWorld() so that blueprint functions which use a hidden WorldContextObject* will work properly.
	virtual UWorld* GetWorld() const override;
	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;

	// If currently planning, returns a proxy to the currently considered worldstate.
	// Otherwise returns a proxy to the querier's blackboard.
	UFUNCTION(BlueprintPure, Category = "AI|HTN")
	UWorldStateProxy* GetWorldState() const;

	// If the querier has a current worldstate (see GetWorldState), returns the value of the SelfLocation key from that worldstate.
	// Otherwise returns the actual location of the querier.
	UFUNCTION(BlueprintPure, Category = "AI|HTN")
	FVector GetQuerierLocation() const;

	UFUNCTION(BlueprintImplementableEvent)
	void ProvideSingleActor(UObject* QuerierObject, AActor* QuerierActor, AActor*& ResultingActor) const;

	UFUNCTION(BlueprintImplementableEvent)
	void ProvideSingleLocation(UObject* QuerierObject, AActor* QuerierActor, FVector& ResultingLocation) const;

	UFUNCTION(BlueprintImplementableEvent)
	void ProvideActorsSet(UObject* QuerierObject, AActor* QuerierActor, TArray<AActor*>& ResultingActorsSet) const;
	
	UFUNCTION(BlueprintImplementableEvent)
	void ProvideLocationsSet(UObject* QuerierObject, AActor* QuerierActor, TArray<FVector>& ResultingLocationSet) const;

private:
	enum ECallMode
	{
		InvalidCallMode,
		SingleActor,
		SingleLocation,
		ActorSet,
		LocationSet
	};

	ECallMode CallMode;

	UPROPERTY(transient)
	mutable TObjectPtr<AActor> TempQuerierActor;
	mutable uint8 bIsPlanTimeQuery : 1;
};
