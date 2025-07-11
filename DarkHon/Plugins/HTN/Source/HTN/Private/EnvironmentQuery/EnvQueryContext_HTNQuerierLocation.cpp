// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "EnvironmentQuery/EnvQueryContext_HTNQuerierLocation.h"

#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"
#include "VisualLogger/VisualLogger.h"

#include "HTNBlueprintLibrary.h"
#include "WorldStateProxy.h"
#include "Utility/HTNHelpers.h"

void UEnvQueryContext_HTNQuerierLocation::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	if (AActor* const QueryOwner = Cast<AActor>(QueryInstance.Owner.Get()))
	{
		FVector Location = FAISystem::InvalidLocation;

		const bool bIsPlanTimeQuery = QueryInstance.NamedParams.FindRef(FHTNNames::IsPlanTimeQuery) != 0.0f;
		if (UWorldStateProxy* const WorldStateProxy = FHTNHelpers::GetWorldStateProxy(QueryOwner, bIsPlanTimeQuery))
		{
			Location = WorldStateProxy->GetValueAsVector(FBlackboard::KeySelfLocation);
		}
		else
		{
			UE_LOG(LogEQS, Log, TEXT("EnvQueryContext_HTNQuerierLocation: couldn't get the current worldstate of query owner."));
		}

		if (!FAISystem::IsValidLocation(Location))
		{
			UE_LOG(LogEQS, Log, TEXT("EnvQueryContext_HTNQuerierLocation: couldn't get location from worldstate, using query owner's actual location."));
			UE_CVLOG(GET_AI_CONFIG_VAR(bAllowControllersAsEQSQuerier) == false && Cast<AController>(QueryOwner), QueryOwner, LogEQS, Warning, TEXT("Using Controller as query's owner is dangerous since Controller's location is usually not what you expect it to be!"));
			Location = QueryOwner->GetActorLocation();
		}

		if (FAISystem::IsValidLocation(Location))
		{
			UEnvQueryItemType_Point::SetContextHelper(ContextData, Location);
		}
	}
}
