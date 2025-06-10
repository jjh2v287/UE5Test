// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "EnvironmentQuery/EQSParametrizedQueryExecutionRequestHTN.h"

#include "VisualLogger/VisualLogger.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Int.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"

#include "HTN.h"
#include "BlackboardWorldstate.h"
#include "HTNPlan.h"

int32 FEQSParametrizedQueryExecutionRequestHTN::Execute(AActor& QueryOwner, const FBlackboardWorldState& WorldState, const FQueryFinishedSignature& QueryFinishedDelegate) const
{
	UEnvQuery* Template = QueryTemplate;
	if (bUseBBKeyForQueryTemplate)
	{
		Template = GetQueryTemplateFromWorldState(QueryOwner, WorldState);
	}

	if (Template)
	{
		FEnvQueryRequest QueryRequest(Template, &QueryOwner);
		PopulateDynamicParameters(QueryRequest, WorldState);
		return QueryRequest.Execute(RunMode, QueryFinishedDelegate);
	}

	return INDEX_NONE;
}

TSharedPtr<FEnvQueryResult> FEQSParametrizedQueryExecutionRequestHTN::ExecuteInstant(AActor& QueryOwner, const FBlackboardWorldState& WorldState) const
{
	UEnvQuery* Template = QueryTemplate;
	if (bUseBBKeyForQueryTemplate)
	{
		Template = GetQueryTemplateFromWorldState(QueryOwner, WorldState);
	}

	if (Template)
	{
		FEnvQueryRequest QueryRequest(Template, &QueryOwner);
		PopulateDynamicParameters(QueryRequest, WorldState);

		UWorld* const World = QueryOwner.GetWorld();
		QueryRequest.SetWorldOverride(World);
		
		if (UEnvQueryManager* const EnvQueryManager = UEnvQueryManager::GetCurrent(World))
		{
			return EnvQueryManager->RunInstantQuery(QueryRequest, RunMode);
		}

		UE_LOG(LogEQS, Warning, TEXT("Missing EQS manager!"));
		return nullptr;
	}

	return nullptr;
}

UEnvQuery* FEQSParametrizedQueryExecutionRequestHTN::GetQueryTemplateFromWorldState(AActor& QueryOwner, const FBlackboardWorldState& WorldState) const
{
	UObject* const QueryTemplateObject = WorldState.GetValue<UBlackboardKeyType_Object>(EQSQueryBlackboardKey.GetSelectedKeyID());
	UEnvQuery* const Template = Cast<UEnvQuery>(QueryTemplateObject);
	
	UE_CVLOG(Template == nullptr, &QueryOwner, LogHTN, Warning,
		TEXT("Trying to run EQS query configured to use BB key, but indicated key (%s) doesn't contain EQS template pointer"),
		*EQSQueryBlackboardKey.SelectedKeyName.ToString()
	);

	return Template;
}

void FEQSParametrizedQueryExecutionRequestHTN::PopulateDynamicParameters(FEnvQueryRequest& QueryRequest, const FBlackboardWorldState& WorldState) const
{
	QueryRequest.SetBoolParam(FHTNNames::IsPlanTimeQuery, true);
	
	for (const FAIDynamicParam& RuntimeParam : QueryConfig)
	{
		// Check if given param requires runtime resolve, like reading from BB
		if (RuntimeParam.BBKey.IsSet())
		{
			// Grab info from BB
			switch (RuntimeParam.ParamType)
			{
			case EAIParamType::Float:
			{
				const float Value = WorldState.GetValue<UBlackboardKeyType_Float>(RuntimeParam.BBKey.GetSelectedKeyID());
				QueryRequest.SetFloatParam(RuntimeParam.ParamName, Value);
			}
			break;
			case EAIParamType::Int:
			{
				const int32 Value = WorldState.GetValue<UBlackboardKeyType_Int>(RuntimeParam.BBKey.GetSelectedKeyID());
				QueryRequest.SetIntParam(RuntimeParam.ParamName, Value);
			}
			break;
			case EAIParamType::Bool:
			{
				const bool Value = WorldState.GetValue<UBlackboardKeyType_Bool>(RuntimeParam.BBKey.GetSelectedKeyID());
				QueryRequest.SetBoolParam(RuntimeParam.ParamName, Value);
			}
			break;
			default:
				checkNoEntry();
				break;
			}
		}
		else
		{
			switch (RuntimeParam.ParamType)
			{
			case EAIParamType::Float:
			{
				QueryRequest.SetFloatParam(RuntimeParam.ParamName, RuntimeParam.Value);
			}
			break;
			case EAIParamType::Int:
			{
				QueryRequest.SetIntParam(RuntimeParam.ParamName, RuntimeParam.Value);
			}
			break;
			case EAIParamType::Bool:
			{
				QueryRequest.SetBoolParam(RuntimeParam.ParamName, RuntimeParam.Value > 0);
			}
			break;
			default:
				checkNoEntry();
				break;
			}
		}
	}
}
