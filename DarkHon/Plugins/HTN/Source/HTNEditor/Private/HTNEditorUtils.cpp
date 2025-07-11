// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "HTNEditorUtils.h"
#include "HTNGraphNode.h"
#include "HTNGraphNode_Root.h"

#include "Algo/Transform.h"

TArray<UObject*> FHTNEditorUtils::GetSelectionForPropertyEditor(const TSet<UObject*>& Selection)
{
	TArray<UObject*> SelectedObjects;
	Algo::TransformIf(Selection, SelectedObjects, IsValid, [](UObject* Obj) -> UObject*
	{
		if (UHTNGraphNode_Root* const RootNode = Cast<UHTNGraphNode_Root>(Obj))
		{
			return RootNode;
		}

		if (const UHTNGraphNode* const RuntimeNode = Cast<UHTNGraphNode>(Obj))
		{
			return RuntimeNode->NodeInstance;
		}

		return Obj;
	});

	return SelectedObjects;
}
