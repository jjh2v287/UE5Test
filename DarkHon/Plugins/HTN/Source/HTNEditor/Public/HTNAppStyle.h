// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "Misc/EngineVersionComparison.h"

#if UE_VERSION_OLDER_THAN(5, 1, 0)
#include "EditorStyleSet.h"
#define FHTNAppStyle FEditorStyle
#else
#include "Styling/AppStyle.h"
#define FHTNAppStyle FAppStyle
#endif

FORCEINLINE FName FHTNAppStyleGetAppStyleSetName()
{
#if UE_VERSION_OLDER_THAN(5, 1, 0)
	return FEditorStyle::GetStyleSetName();
#else
	return FAppStyle::GetAppStyleSetName();
#endif
}
