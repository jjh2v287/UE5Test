// Copyright 2019 - 2021, butterfly, Event System Plugin, All Rights Reserved.

#pragma once

#include "KismetPins/SGraphPinStructInstance.h"
#include "SEventWidget.h"

template <typename ItemType> class SListView;

/** Pin that represents a single gameplay tag, overrides the generic struct widget because tags have their own system for saving changes */
class SEventGraphPin : public SGraphPinStructInstance
{
public:
	SLATE_BEGIN_ARGS(SEventGraphPin) {}
	SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj);

protected:
	//~ Begin SGraphPinStructInstance Interface
	virtual void ParseDefaultValueData() override;
	virtual void SaveDefaultValueData() override;
	virtual void RefreshCachedData() override;
	virtual TSharedRef<SWidget> GetEditContent() override;
	virtual TSharedRef<SWidget> GetDescriptionContent() override;
	//~ End SGraphPin Interface 

	/**
	 * Callback for populating rows of the SelectedTags List View.
	 * @return widget that contains the name of a tag.
	 */
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& OwnerTable);

	// Tag Container used for the GameplayTagWidget.
	TSharedPtr<FEventContainer> TagContainer;

	// Datum uses for the GameplayTagWidget.
	TArray<SEventWidget::FEditableEventContainerDatum> EditableContainers;

	// Array of names for the read only display of tag names on the node.
	TArray< TSharedPtr<FString> > TagNames;

	// The List View used to display the read only tag names on the node.
	TSharedPtr<SListView<TSharedPtr<FString>>> TagListView;

	// Filter the list of available tags
	FString FilterString;
};

