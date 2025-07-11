// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "BehaviorTree/BlackboardAssetProvider.h"
#include "HTN.generated.h"

// A Hierarchical Task Network asset
UCLASS(BlueprintType)
class HTN_API UHTN : public UObject, public IBlackboardAssetProvider
{
	GENERATED_BODY()

public:
	// IBlackboardAssetProvider
	virtual UBlackboardData* GetBlackboardAsset() const override;

	// The nodes that begin from the root.
	UPROPERTY()
	TArray<TObjectPtr<class UHTNStandaloneNode>> StartNodes;

	UPROPERTY()
	TArray<TObjectPtr<class UHTNDecorator>> RootDecorators;

	UPROPERTY()
	TArray<TObjectPtr<class UHTNService>> RootServices; 

#if WITH_EDITORONLY_DATA
	// The editor graph associated with this HTN.
	UPROPERTY()
	TObjectPtr<class UEdGraph> HTNGraph;

	// Contextual info stored on editor close. Viewport location, zoom level etc.
	UPROPERTY()
	TArray<FEditedDocumentInfo> LastEditedDocuments;
#endif

	// Blackboard asset for this HTN.
	UPROPERTY()
	TObjectPtr<class UBlackboardData> BlackboardAsset;
};