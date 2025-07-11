// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace FHTNEditorUtils
{
	// Given a collection of graph nodes selected in the editor, 
	// returns the objects that should be selected for editing in the Details tab.
	// In most cases those are the underlying HTNNode objects.
	TArray<UObject*> GetSelectionForPropertyEditor(const TSet<UObject*>& Selection);
}
