// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Delegates/Delegate.h"
#include "GraphEditor.h"
#include "HAL/Platform.h"
#include "IAssetTypeActions.h"
#include "Input/Reply.h"
#include "Misc/EngineVersionComparison.h"
#include "Types/SlateEnums.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class FUICommandList;
class ITableRow;
class SBorder;
class STableViewBase;
class SWidget;
class UEdGraph;
class UEdGraphPin;
struct FDiffSingleResult;
template <typename ItemType> class SListView;

// The diff view that shows the differences between two HTNs.
// It is meant to fill an entire window.
class SHTNDiff : public SCompoundWidget
{
public:

	// Delegate for default Diff tool
	DECLARE_DELEGATE_TwoParams(FOpenInDefaultDiffTool, class UHTN*, class UHTN*);

	SLATE_BEGIN_ARGS(SHTNDiff) {}
		SLATE_ARGUMENT(class UHTN*, HTNOld)
		SLATE_ARGUMENT(class UHTN*, HTNNew)
		SLATE_ARGUMENT(struct FRevisionInfo, OldRevision)
		SLATE_ARGUMENT(struct FRevisionInfo, NewRevision)
		SLATE_ARGUMENT(bool, ShowAssetNames)
		SLATE_EVENT(FOpenInDefaultDiffTool, OpenInDefaultDiffTool)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:

	// A panel used to display an HTN. There is one such panel for each of the two revisions being compared.
	struct FHTNDiffPanel
	{
		FHTNDiffPanel();

		// Generates a panel with a graph view
		TSharedRef<SWidget> CreateGraphWidget(
#if UE_VERSION_OLDER_THAN(5, 1, 0)
			UEdGraph* const GraphToDiff);
#else
			TSharedPtr<TArray<FDiffSingleResult>> DiffResults);
#endif

		// Generates a panel that contains a details view of the selected node
		TSharedRef<SWidget> CreateDetailsWidget();

		// Returns a text describing the revision this diff panel is showing
		FText GetRevisionDescription() const;

		// Called when user hits keyboard shortcut to copy nodes
		void CopySelectedNodes();

		// Called When graph node gains focus
		void OnSelectionChanged( const FGraphPanelSelectionSet& Selection );

		// Delegate to say if a node property should be editable
		bool IsPropertyEditable();

		// Gets whatever nodes are selected in the Graph Editor
		FGraphPanelSelectionSet GetSelectedNodes() const;

		bool CanCopyNodes() const;

		// The HTN that owns the graph we are showing
		class UHTN* HTN;

		// Revision information for this HTN
		FRevisionInfo RevisionInfo;

		// The graph editor which does the work of displaying the graph
		TWeakPtr<class SGraphEditor> GraphEditor;

		// If we should show a name identifying which asset this panel is displaying
		bool bShowAssetName;
	
		// Command list for this diff panel
		TSharedPtr<FUICommandList> GraphEditorCommands;

		// Property View 
		TSharedPtr<class IDetailsView> DetailsView;
	};

	using FSharedDiffOnGraph = TSharedPtr<struct FHTNDiffResultItem>;

	void OnOpenInDefaultDiffTool();

	TSharedRef<SWidget> GenerateDiffListWidget();
	TSharedRef<SWidget> GenerateDiffPanels();

	void BuildDiffSourceArray();

	void NextDiff();
	void PrevDiff();

	int32 GetCurrentDiffIndex();

	void OnSelectionChanged(FSharedDiffOnGraph Item, ESelectInfo::Type SelectionType);
	TSharedRef<ITableRow> OnGenerateRow(FSharedDiffOnGraph Item, const TSharedRef<STableViewBase>& OwnerTable);
	SGraphEditor* GetGraphEditorForGraph(UEdGraph* Graph) const;

	// Get the image to show for the toggle lock option
	FSlateIcon GetLockViewImage() const;

	// Get the image to show for the toggle split view mode option
	FSlateIcon GetSplitViewModeImage() const;

	// User toggles the option to lock the views between the two graphs
	void OnToggleLockView();

	// User toggles the option to change the split view mode betwwen vertical and horizontal
	void OnToggleSplitViewMode();

	// Reset the graph editor, called when user switches graphs to display
	void ResetGraphEditors();

	// Delegate to call when user wishes to view the defaults
	FOpenInDefaultDiffTool OpenInDefaultDiffTool;

	// The 2 panels showing the two HTNs we will be comparing
	FHTNDiffPanel PanelOld;
	FHTNDiffPanel PanelNew;

	// If the two views should be locked
	bool bLockViews = true;

	// If the view on Graph Mode should be divided vertically instead of horizontally
	bool bVerticalSplitGraphMode = true;

	TSharedPtr<SSplitter> DiffGraphSplitter;

	// Source for list view 
	TArray<FSharedDiffOnGraph> DiffListSource;

	// result from FGraphDiffControl::DiffGraphs
	TSharedPtr<TArray<FDiffSingleResult>> FoundDiffs;

	// Key commands processed by this widget
	TSharedPtr<FUICommandList> KeyCommands;

	// ListView of differences
	TSharedPtr<SListView<FSharedDiffOnGraph>> DiffList;

	// The last pin the user clicked on
	UEdGraphPin* LastPinTarget = nullptr;

	// The last other pin the user clicked on
	UEdGraphPin* LastOtherPinTarget = nullptr;
};
