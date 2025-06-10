// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "HTNRuntimeSettings.generated.h"

// Runtime (as opposed to Editor) settings of the HTN plugin.
UCLASS(Config = Engine, DefaultConfig)
class HTN_API UHTNRuntimeSettings : public UObject
{
	GENERATED_BODY()

	UHTNRuntimeSettings();

public:
	// When a new plan is created, Blueprint nodes (and C++ nodes that enabled bCreateNodeInstance)
	// from the HTN graph are duplicated to be used in the plan. This increases the load on Garbage Collection, 
	// especially when making plans frequently. Enabling node instance pooling allows such node instances 
	// to be pooled on the HTNComponent, making it possible to reuse them when creating new plans.
	// 
	// By default, when returning a node instance to the pool after its plan is over,
	// the node's properties and local variables (e.g., inside DoOnce nodes etc.) will be automatically reset 
	// to values from the template node in the HTN asset from which the node instance was created.
	// 
	// This also resets the UObject serial number of node instances when they return to the pool. 
	// This invalidates all WeakObjectPtrs to the node instance, such as those inside delegates that the node 
	// may have subscribed to external events. This effectively unsubscribes the node from anything it might've subscribed to. 
	// 
	// This behavior can be augmented by overriding the OnInstanceReturnedToPool function.
	//
	// This setting can be overridden on a per-node basis via the "Node Instance Pooling Mode" of each HTN node.
	// "Node Instance Pooling Mode" is only visible for nodes that create instances (built in C++ nodes don't).
	UPROPERTY(EditAnywhere, Config, Category = "Node Instance Pooling")
	bool bEnableNodeInstancePooling;
};
