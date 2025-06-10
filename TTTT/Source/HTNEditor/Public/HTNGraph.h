// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "AIGraph.h"
#include "HTNGraph.generated.h"

/// The editor graph for a Hierarchical Task Network
UCLASS()
class UHTNGraph : public UAIGraph
{
	GENERATED_BODY()
	
public:
	virtual void OnCreated() override;
	virtual void UpdateAsset(int32 UpdateFlags = 0) override;
	virtual void OnSubNodeDropped() override;
	void OnSave();
	void OnBlackboardChanged();
	class UHTNGraphNode* SpawnGraphNode(class UHTNStandaloneNode* Node, bool bSelectNewNode = true);
	class UHTNGraphNode_Root* FindRootNode() const;
	
protected:

#if WITH_EDITOR
	virtual void PostEditUndo() override;
#endif

	// When there are nodes in the asset that don't have graph nodes
	// E.g. when opening an HTN asset but the graph is missing
	void SpawnMissingGraphNodes();
};