// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "EnvironmentQuery/EnvQueryContext_HTNBlueprintBase.h"

#include "BlueprintNodeHelpers.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"
#include "EnvironmentQuery/EnvQueryManager.h"

#include "Utility/HTNHelpers.h"

UEnvQueryContext_HTNBlueprintBase::UEnvQueryContext_HTNBlueprintBase(const FObjectInitializer& Initializer) : Super(Initializer),
	bIsPlanTimeQuery(false)
{
#define IS_IMPLEMENTED(FunctionName) \
	(BlueprintNodeHelpers::HasBlueprintFunction(GET_FUNCTION_NAME_CHECKED(UEnvQueryContext_HTNBlueprintBase, FunctionName), *this, *StaticClass()))
	const bool bImplementsProvideSingleActor = IS_IMPLEMENTED(ProvideSingleActor);
	const bool bImplementsProvideSingleLocation = IS_IMPLEMENTED(ProvideSingleLocation);
	const bool bImplementsProvideActorSet = IS_IMPLEMENTED(ProvideActorsSet);
	const bool bImplementsProvideLocationsSet = IS_IMPLEMENTED(ProvideLocationsSet);
#undef IS_IMPLEMENTED

	if (bImplementsProvideSingleActor)
	{
		CallMode = SingleActor;
	}

	if (bImplementsProvideSingleLocation)
	{
		CallMode = SingleLocation;
	}

	if (bImplementsProvideActorSet)
	{
		CallMode = ActorSet;
	}

	if (bImplementsProvideLocationsSet)
	{
		CallMode = LocationSet;
	}
}

UWorld* UEnvQueryContext_HTNBlueprintBase::GetWorld() const
{
	check(GetOuter());
	if (UEnvQueryManager* const EnvQueryManager = Cast<UEnvQueryManager>(GetOuter()))
	{
		return EnvQueryManager->GetWorld();
	}

	// Outer should always be UEnvQueryManager* in the game, which implements GetWorld() and therefore makes this
	// a correct world context.  However, in the editor the outer is /Script/AIModule (at compile time), which
	// does not explicitly implement GetWorld().  For that reason, calling GetWorld() generically in that case on the
	// AIModule calls to the base implementation, which causes a blueprint compile warning in the Editor
	// which states that the function isn't safe to call on this (due to requiring WorldContext which it doesn't
	// provide). Simply returning nullptr in this case fixes those erroneous blueprint compile warnings.
	return nullptr;
}

void UEnvQueryContext_HTNBlueprintBase::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	UObject* const QuerierObject = QueryInstance.Owner.Get();
	if (!QuerierObject || CallMode == InvalidCallMode)
	{
		return;
	}
	
	TempQuerierActor = Cast<AActor>(QueryInstance.Owner);
	bIsPlanTimeQuery = QueryInstance.NamedParams.FindRef(FHTNNames::IsPlanTimeQuery);

	switch (CallMode)
	{
	case SingleActor:
	{
		AActor* ResultingActor = nullptr;
		ProvideSingleActor(QuerierObject, TempQuerierActor, ResultingActor);
		UEnvQueryItemType_Actor::SetContextHelper(ContextData, ResultingActor);
	}
	break;
	case SingleLocation:
	{
		FVector ResultingLocation = FAISystem::InvalidLocation;
		ProvideSingleLocation(QuerierObject, TempQuerierActor, ResultingLocation);
		UEnvQueryItemType_Point::SetContextHelper(ContextData, ResultingLocation);
	}
	break;
	case ActorSet:
	{
		TArray<AActor*> ActorSet;
		ProvideActorsSet(QuerierObject, TempQuerierActor, ActorSet);
		UEnvQueryItemType_Actor::SetContextHelper(ContextData, ActorSet);
	}
	break;
	case LocationSet:
	{
		TArray<FVector> LocationSet;
		ProvideLocationsSet(QuerierObject, TempQuerierActor, LocationSet);
		UEnvQueryItemType_Point::SetContextHelper(ContextData, LocationSet);
	}
	break;
	default:
		break;
	}
}

UWorldStateProxy* UEnvQueryContext_HTNBlueprintBase::GetWorldState() const
{
	if (TempQuerierActor)
	{
		return FHTNHelpers::GetWorldStateProxy(TempQuerierActor, bIsPlanTimeQuery);
	}

	return nullptr;
}

FVector UEnvQueryContext_HTNBlueprintBase::GetQuerierLocation() const
{
	if (UWorldStateProxy* const WorldStateProxy = GetWorldState())
	{
		return WorldStateProxy->GetSelfLocation();
	}
	
	if (TempQuerierActor)
	{
		return TempQuerierActor->GetActorLocation();
	}

	return FAISystem::InvalidLocation;
}
