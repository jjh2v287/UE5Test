// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WorkflowOrientedApp/WorkflowCentricApplication.h"

class HTNEDITOR_API IHTNEditor : public FWorkflowCentricApplication
{
public:
	virtual class UHTN* GetCurrentHTN() const = 0;
	virtual class UBlackboardData* GetCurrentBlackboardData() const = 0;
	virtual void SetCurrentHTN(UHTN* HTN) = 0;
};
