// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "SHTNBlackboardView.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SWidget.h"

DECLARE_LOG_CATEGORY_EXTERN(LogBlackboardEditor, Warning, All);

// Delegate used to determine whether the Blackboard mode is active
DECLARE_DELEGATE_RetVal(bool, FOnIsBlackboardModeActive);

// Displays and edits blackboard entries
class SHTNBlackboardEditor : public SHTNBlackboardView
{
public:
	SLATE_BEGIN_ARGS(SHTNBlackboardEditor) {}

		SLATE_EVENT(FOnEntrySelected, OnEntrySelected)
		SLATE_EVENT(FOnGetDebugKeyValue, OnGetDebugKeyValue)
		SLATE_EVENT(FOnGetDisplayCurrentState, OnGetDisplayCurrentState)
		SLATE_EVENT(FOnIsDebuggerReady, OnIsDebuggerReady)
		SLATE_EVENT(FOnIsDebuggerPaused, OnIsDebuggerPaused)
		SLATE_EVENT(FOnGetDebugTimeStamp, OnGetDebugTimeStamp)
		SLATE_EVENT(FOnBlackboardKeyChanged, OnBlackboardKeyChanged)
		SLATE_EVENT(FOnIsBlackboardModeActive, OnIsBlackboardModeActive)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedRef<FUICommandList> InCommandList, UBlackboardData* InBlackboardData);

private:
	virtual void FillContextMenu(FMenuBuilder& MenuBuilder) const override;
	void FillToolbar(FToolBarBuilder& ToolbarBuilder) const;
	virtual TSharedPtr<FExtender> GetToolbarExtender(TSharedRef<FUICommandList> ToolkitCommands) const override;

	// Create the menu for creating a new blackboard entry
	TSharedRef<class SWidget> HandleCreateNewEntryMenu() const;

	bool CanCreateNewEntry() const;
	bool CanDeleteEntry() const;
	bool CanRenameEntry() const;

	void HandleCreateEntry(UClass* InClass);
	void HandleDeleteEntry();
	void HandleRenameEntry();

	FName MakeUniqueKeyName(const UClass& Class) const;

	// Delegate used to determine whether the Blackboard mode is active
	FOnIsBlackboardModeActive OnIsBlackboardModeActive;
};
