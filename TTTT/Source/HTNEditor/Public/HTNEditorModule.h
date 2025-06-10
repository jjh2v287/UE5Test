// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "Toolkits/AssetEditorToolkit.h"

extern const FName HTNEditorAppIdentifier;

// Interface for the HTN editor module
class IHTNEditorModule : public IModuleInterface, public IHasMenuExtensibility, public IHasToolBarExtensibility
{
public:
	virtual TSharedRef<class IHTNEditor> CreateHTNEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, class UHTN* HTN) = 0;
	virtual TSharedPtr<struct FGraphNodeClassHelper> GetClassCache() = 0;

	static inline IHTNEditorModule& Get()
	{
		return FModuleManager::LoadModuleChecked<IHTNEditorModule>(TEXT("HTNEditor"));
	}
};
