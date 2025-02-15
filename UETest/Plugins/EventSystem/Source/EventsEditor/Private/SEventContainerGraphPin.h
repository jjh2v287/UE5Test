// Copyright 2019 - 2021, butterfly, Event System Plugin, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SWidget.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"
#include "SEventWidget.h"
#include "SGraphPin.h"

class SComboButton;

class SEventContainerGraphPin : public SGraphPin
{
public:
	SLATE_BEGIN_ARGS(SEventContainerGraphPin) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj);

	//~ Begin SGraphPin Interface
	virtual TSharedRef<SWidget>	GetDefaultValueWidget() override;
	//~ End SGraphPin Interface

private:

	/** Refreshes the list of tags displayed on the node. */
	void RefreshTagList();

	/** Parses the Data from the pin to fill in the names of the array. */
	void ParseDefaultValueData();

	/** Callback function to create content for the combo button. */
	TSharedRef<SWidget> GetListContent();

	/** 
	 * Creates SelectedTags List.
	 * @return widget that contains the read only tag names for displaying on the node.
	 */
	TSharedRef<SWidget> SelectedTags();

	/** 
	 * Callback for populating rows of the SelectedTags List View.
	 * @return widget that contains the name of a tag.
	 */
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& OwnerTable);

private:

	// Combo Button for the drop down list.
	TSharedPtr<SComboButton> ComboButton;

	// Tag Container used for the EventWidget.
	TSharedPtr<FEventContainer> TagContainer;

	// Datum uses for the EventWidget.
	TArray<SEventWidget::FEditableEventContainerDatum> EditableContainers;

	// Array of names for the read only display of tag names on the node.
	TArray< TSharedPtr<FString> > TagNames;

	// The List View used to display the read only tag names on the node.
	TSharedPtr<SListView<TSharedPtr<FString>>> TagListView;

	// Tag filter string for the container
	FString FilterString;
};
