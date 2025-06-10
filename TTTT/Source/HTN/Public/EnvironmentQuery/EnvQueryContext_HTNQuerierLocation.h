// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "EnvQueryContext_HTNQuerierLocation.generated.h"

// An EQS context that returns the location of the querier.
// If the querier is currently planning, returns the location the querier will have at this point in the plan.
// Allows for querying from future locations, which is useful for planning.
UCLASS()
class HTN_API UEnvQueryContext_HTNQuerierLocation : public UEnvQueryContext
{
	GENERATED_BODY()
	
public:
	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;
};
