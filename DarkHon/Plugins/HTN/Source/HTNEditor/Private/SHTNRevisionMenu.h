// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Delegates/Delegate.h"
#include "ISourceControlOperation.h"
#include "ISourceControlProvider.h"
#include "Input/Reply.h"
#include "Layout/Visibility.h"
#include "SourceControlOperations.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class FUpdateStatus;
class SVerticalBox;
struct FRevisionInfo;

// The dropown that shows up when you click on the "Diff" combo box in the toolbar of the HTN Editor.
// Lets you select from multiple past revisions.
class SHTNRevisionMenu : public SCompoundWidget
{
	DECLARE_DELEGATE_OneParam(FOnRevisionSelected, const FRevisionInfo&)

public:
	SLATE_BEGIN_ARGS(SHTNRevisionMenu)
		: _bIncludeLocalRevision(false)
	{}
		SLATE_ARGUMENT(bool, bIncludeLocalRevision)
		SLATE_EVENT(FOnRevisionSelected, OnRevisionSelected)
	SLATE_END_ARGS()

	~SHTNRevisionMenu();

	void Construct(const FArguments& InArgs, const class UHTN* HTN);

private: 
	enum class ESourceControlQueryState
	{
		NotQueried,
		QueryInProgress,
		Queried,
	};

	// Delegate used to determine the visibility 'in progress' widgets
	EVisibility GetInProgressVisibility() const;
	// Delegate used to determine the visibility of the cancel button
	EVisibility GetCancelButtonVisibility() const;

	// Delegate used to cancel a source control operation in progress 
	FReply OnCancelButtonClicked() const;
	// Callback for when the source control operation is complete
	void OnSourceControlQueryComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult);

	bool bIncludeLocalRevision;
	FOnRevisionSelected OnRevisionSelected;
	// The name of the file we want revision info for
	FString Filename;
	// The box we are using to display our menu
	TSharedPtr<SVerticalBox> MenuBox;
	// The source control operation in progress
	TSharedPtr<FUpdateStatus, ESPMode::ThreadSafe> SourceControlQueryOp;
	// The state of the SCC query
	ESourceControlQueryState SourceControlQueryState = ESourceControlQueryState::NotQueried;
};
