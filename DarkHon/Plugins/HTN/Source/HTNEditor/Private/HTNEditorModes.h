// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once 

#include "CoreMinimal.h"
#include "Framework/Docking/TabManager.h"
#include "WorkflowOrientedApp/WorkflowTabManager.h"
#include "WorkflowOrientedApp/ApplicationMode.h"

// Application mode for main htn editing mode
class FHTNEditorApplicationMode : public FApplicationMode
{
public:
	FHTNEditorApplicationMode(TSharedPtr<class FHTNEditor> InHTNEditor);

	virtual void RegisterTabFactories(TSharedPtr<class FTabManager> InTabManager) override;
	virtual void PreDeactivateMode() override;
	virtual void PostActivateMode() override;

protected:
	TWeakPtr<class FHTNEditor> HTNEditor;

	// Set of spawnable tabs in behavior tree editing mode
	FWorkflowAllowedTabSet HTNEditorTabFactories;
};

// Application mode for blackboard editing mode
class FHTNBlackboardEditorApplicationMode : public FApplicationMode
{
public:
	FHTNBlackboardEditorApplicationMode(TSharedPtr<class FHTNEditor> InHTNEditor);

	virtual void RegisterTabFactories(TSharedPtr<class FTabManager> InTabManager) override;

protected:
	TWeakPtr<class FHTNEditor> HTNEditor;

	// Set of spawnable tabs in blackboard mode
	FWorkflowAllowedTabSet BlackboardTabFactories;
};