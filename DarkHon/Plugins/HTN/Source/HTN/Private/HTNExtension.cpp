// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "HTNExtension.h"
#include "HTNComponent.h"

UHTNExtension::UHTNExtension() : 
	bNotifyTick(false)
{}

#if WITH_ENGINE

UWorld* UHTNExtension::GetWorld() const
{
	if (const UObject* const Outer = GetOuter())
	{
		// Special case for extension Blueprints in the editor.
		// This prevents extra "world context object" inputs from showing up in many BP nodes in the BP editor.
		if (Outer->IsA<UPackage>())
		{
			// GetOuter should return a UPackage and its Outer is a UWorld
			return Cast<UWorld>(Outer->GetOuter());
		}

		return Outer->GetWorld();
	}

	return nullptr;
}

#endif

UHTNComponent* UHTNExtension::GetHTNComponent() const
{
	return GetTypedOuter<UHTNComponent>();
}
