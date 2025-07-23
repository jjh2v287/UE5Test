// Copyright 2021 Marcin Swiderski. All Rights Reserved.

#pragma once

#include "ComponentVisualizer.h"

class FTickOptToolkitComponentVisualizer : public FComponentVisualizer
{
public:
	virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
};
