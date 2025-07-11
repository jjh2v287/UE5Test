// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "HTNTypes.h"
#include "IHTNEditor.h"

#include "AIGraphEditor.h"
#include "Misc/EngineVersionComparison.h"
#include "Misc/NotifyHook.h"
#include "Toolkits/IToolkitHost.h"

#if !UE_VERSION_OLDER_THAN(5, 0, 0)
#include "UObject/ObjectSaveContext.h"
#endif

// An editor for Hierarchical Task Networks
class HTNEDITOR_API FHTNEditor : public IHTNEditor, public FAIGraphEditor, public FNotifyHook
{
public:
	FHTNEditor();
	virtual ~FHTNEditor();
	
	void InitHTNEditor(const EToolkitMode::Type Mode, const TSharedPtr<class IToolkitHost>& InitToolkitHost, class UHTN* HTN);

	//~ Begin IHTNEditor interface
	virtual class UHTN* GetCurrentHTN() const override;
	virtual class UBlackboardData* GetCurrentBlackboardData() const override;
	virtual void SetCurrentHTN(UHTN* HTN) override;
	// End IHTNEditor interface
	
	//~ Begin FAssetEditorToolkit interface
	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager) override;
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FText GetToolkitName() const override;
	virtual FText GetToolkitToolTipText() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual bool IsPrimaryEditor() const override { return true; }
	virtual void FocusWindow(UObject* ObjectToFocusOn = nullptr) override;
	virtual void FindInContentBrowser_Execute() override;
	// End FAssetEditorToolkit interface 

	//~ Begin IAssetEditorInstance Interface
#if !UE_VERSION_OLDER_THAN(5, 4, 0)
	virtual bool IncludeAssetInRestoreOpenAssetsPrompt(UObject* Asset) const override;
#endif
	//~ End IAssetEditorInstance Interface

	//~ Begin FEditorUndoClient Interface
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;
	// End of FEditorUndoClient

	//~ Begin FNotifyHook Interface
	virtual void NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged) override;
	// End of FNotifyHook

	TWeakPtr<SGraphEditor> GetFocusedGraphPtr() const;

	bool CanAccessHTNMode() const;
	bool CanAccessBlackboardMode() const;
	bool IsBlackboardModeActive() const;
	bool CanEditWithDebuggerActive() const;

	void RestoreHTN();
	void SaveEditedObjectState();
	void OnGraphEditorFocused(const TSharedRef<SGraphEditor>& InGraphEditor);

	void RegisterToolbarTabSpawner(const TSharedRef<class FTabManager>& InTabManager);
	
	// Factory functions for creating widgets of tabs
	TSharedRef<class SWidget> SpawnDetailsWidget();
	TSharedRef<class SWidget> SpawnSearchWidget();
	TSharedRef<class SWidget> SpawnBlackboardDetailsWidget();
	TSharedRef<class SWidget> SpawnBlackboardViewWidget();
	TSharedRef<class SWidget> SpawnBlackboardEditorWidget();

	FORCEINLINE TSharedPtr<class FHTNEditorToolbarBuilder> GetToolbarBuilder() const { return ToolbarBuilder; }
	FORCEINLINE TSharedPtr<class FHTNDebugger> GetDebugger() const { return Debugger; }
	
	void OnEnableBreakpoint();
	bool CanEnableBreakpoint() const;
	void OnToggleBreakpoint();
	bool CanToggleBreakpoint() const;
	void OnDisableBreakpoint();
	bool CanDisableBreakpoint() const;
	void OnAddBreakpoint();
	bool CanAddBreakpoint() const;
	void OnRemoveBreakpoint();
	bool CanRemoveBreakpoint() const;
	
protected:
	// Called when "Save" is clicked for this asset
	virtual void SaveAsset_Execute() override;
	
private:
	void OnSearch();

	virtual void OnSelectedNodesChanged(const TSet<class UObject*>& NewSelection) override;

	// Blackboard editing callbacks and helpers
	int32 GetSelectedBlackboardItemIndex(bool& bIsInherited) const;
	void HandleBlackboardEntrySelected(const struct FBlackboardEntry* BlackboardEntry, bool bIsInherited);
	void HandleBlackboardKeyChanged(class UBlackboardData* InBlackboardData, struct FBlackboardEntry* InKey);
	void RefreshBlackboardViews();

	bool IsPropertyEditable() const;
	void OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent);
#if ENGINE_MAJOR_VERSION < 5
	void OnPackageSaved(const FString& PackageFileName, UObject* Outer);
#else
	void OnPackageSavedWithContext(const FString& PackageFileName, UPackage* Package, FObjectPostSaveContext Context);
#endif

	TSharedRef<class SGraphEditor> CreateGraphEditorWidget(UEdGraph* InGraph);
	FGraphAppearanceInfo GetGraphAppearance() const;
	void OnNodeDoubleClicked(class UEdGraphNode* GraphNode);
	void OnNodeTitleCommitted(const FText& NewText, ETextCommit::Type CommitInfo, UEdGraphNode* NodeBeingChanged);
	bool IsInEditingMode(bool bGraphIsEditable) const;

	void BindCommonCommands();
	void BindDebuggerToolbarCommands();
	
	TSharedPtr<class FDocumentTracker> DocumentTracker;
	TSharedPtr<class FHTNEditorToolbarBuilder> ToolbarBuilder;
	
	// Retained widgets
	TSharedPtr<class IDetailsView> DetailsView;
	TSharedPtr<class SHTNSearch> SearchView;
	TSharedPtr<class IDetailsView> BlackboardDetailsView;
	TSharedPtr<class SHTNBlackboardView> BlackboardView;
	TSharedPtr<class SHTNBlackboardEditor> BlackboardEditor;

	// The debugger
	TSharedPtr<class FHTNDebugger> Debugger;

	// The currently opened items in this editor
	class UHTN* CurrentHTN;
	class UBlackboardData* CurrentBlackboardData;

	// Handle to the registered OnPackageSave delegate
	FDelegateHandle OnPackageSavedDelegateHandle;

public:
	static bool IsPIESimulating();
	static bool IsPIENotSimulating();
	static FText GetLocalizedModeDescription(FName InMode);
	
	// The name given to all instances of this type of editor
	static const FName ToolkitFName;
	
	// Names of editor modes
	static const FName HTNMode;
	static const FName BlackboardMode;
};
