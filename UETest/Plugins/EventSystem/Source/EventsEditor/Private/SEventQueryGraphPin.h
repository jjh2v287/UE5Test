// Copyright 2019 - 2021, butterfly, Event System Plugin, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "SGraphPin.h"
#include "SEventQueryWidget.h"

class SComboButton;

class SEventQueryGraphPin : public SGraphPin
{
public:
	SLATE_BEGIN_ARGS(SEventQueryGraphPin) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj);

	//~ Begin SGraphPin Interface
	virtual TSharedRef<SWidget>	GetDefaultValueWidget() override;
	//~ End SGraphPin Interface

private:

	/** Parses the Data from the pin to fill in the names of the array. */
	void ParseDefaultValueData();

	/** Callback function to create content for the combo button. */
	TSharedRef<SWidget> GetListContent();

	void OnQueryChanged();

	/**
	 * Creates SelectedTags List.
	 * @return widget that contains the read only tag names for displaying on the node.
	 */
	TSharedRef<SWidget> QueryDesc();

private:

	// Combo Button for the drop down list.
	TSharedPtr<SComboButton> ComboButton;

	// Tag Container used for the EventWidget.
	TSharedPtr<FEventQuery> TagQuery;

	FString TagQueryExportText;

	// Datum uses for the EventWidget.
	TArray<SEventQueryWidget::FEditableEventQueryDatum> EditableQueries;

	FText GetQueryDescText() const;

	FString QueryDescription;
};
