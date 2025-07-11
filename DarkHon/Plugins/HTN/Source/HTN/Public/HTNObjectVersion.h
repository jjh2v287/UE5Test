// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Misc/Guid.h"

// Custom serialization version for fixups when loading assets from older versions of the HTN plugin.
struct HTN_API FHTNObjectVersion
{
	enum Type
	{
		// Before any version changes were made
		BeforeCustomVersionWasAdded = 0,

		// Before this, decorators used to have "check condition on plan recheck" on by default.
		DisableDecoratorRecheckByDefault,

		// Before this, some tasks (Success, Wait, EQS) used to have a default cost of 100.
		DisableDefaultCostOnSomeTasks,

		// Ways to modify the compared locations were added to HTNDecorator_DistanceCheck.
		MakeDistanceCheckMoreFlexible,

		// UHTNService_ReplanIfLocationChanges now has the whole FHTNReplanSettings struct instead of 
		// just a couple of individual variables that go into that struct.
		AddReplanSettingsStructToHTNServiceReplanIfLocationChanges,

		// Made HTNDecorator_DistanceCheck use a FFloatRange instead of MinDistance/MaxDistance variables.
		MakeDistanceCheckUseFloatRange,

		// -----<new versions can be added above this line>-------------------------------------------------
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};

	// The GUID for this custom version number
	const static FGuid GUID;

	FHTNObjectVersion() = delete;
};
