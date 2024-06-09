// Copyright 2019 - 2021, butterfly, Event System Plugin, All Rights Reserved.

#include "SEventGraphPin.h"
#include "EdGraph/EdGraphSchema.h"
#include "EventPinUtilities.h"
#include "Framework/Views/TableViewMetadata.h"
#include "EventContainer.h"
#include "Widgets/Views/SListView.h"

#define LOCTEXT_NAMESPACE "EventGraphPin"

void SEventGraphPin::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
{
	TagContainer = MakeShareable(new FEventContainer());
	SGraphPin::Construct(SGraphPin::FArguments(), InGraphPinObj);
}

void SEventGraphPin::ParseDefaultValueData()
{
	FilterString = EventPinUtilities::ExtractTagFilterStringFromGraphPin(GraphPinObj);

	FEventInfo Event;

	// Read using import text, but with serialize flag set so it doesn't always throw away invalid ones
	//Event.FromExportString(GraphPinObj->GetDefaultAsString(), PPF_SerializedAsImportText);
	Event.FromExportString(GraphPinObj->GetDefaultAsString());

	if (Event.IsValid())
	{
		TagContainer->AddTag(Event);
	}
}

TSharedRef<SWidget> SEventGraphPin::GetEditContent()
{
	EditableContainers.Empty();
	EditableContainers.Add(SEventWidget::FEditableEventContainerDatum(GraphPinObj->GetOwningNode(), TagContainer.Get()));

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.MaxHeight(400)
		[
			SNew(SEventWidget, EditableContainers)
			.OnTagChanged(this, &SEventGraphPin::SaveDefaultValueData)
			.TagContainerName(TEXT("SEventGraphPin"))
			.Visibility(this, &SGraphPin::GetDefaultValueVisibility)
			.MultiSelect(false)
			.Filter(FilterString)
		];
}

TSharedRef<SWidget> SEventGraphPin::GetDescriptionContent()
{
	RefreshCachedData();

	SAssignNew(TagListView, SListView<TSharedPtr<FString>>)
		.ListItemsSource(&TagNames)
		.SelectionMode(ESelectionMode::None)
		.OnGenerateRow(this, &SEventGraphPin::OnGenerateRow);

	return TagListView->AsShared();
}

TSharedRef<ITableRow> SEventGraphPin::OnGenerateRow(TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow< TSharedPtr<FString> >, OwnerTable)
		[
			SNew(STextBlock).Text(FText::FromString(*Item.Get()))
		];
}

void SEventGraphPin::RefreshCachedData()
{
	// Clear the list
	TagNames.Empty();

	// Add tags to list
	FString TagName;
	if (TagContainer.IsValid())
	{
		for (auto It = TagContainer->CreateConstIterator(); It; ++It)
		{
			TagName = It->ToString();
			TagNames.Add(MakeShareable(new FString(TagName)));
		}
	}

	// Refresh the slate list
	if (TagListView.IsValid())
	{
		TagListView->RequestListRefresh();
	}
}

void SEventGraphPin::SaveDefaultValueData()
{
	RefreshCachedData();

	// Set Pin Data
	FString TagString;
	FString TagName;
	if (TagNames.Num() > 0 && TagNames[0].IsValid())
	{
		TagName = *TagNames[0];
		TagString = TEXT("");
		//TagString = TEXT("(");
		TagString += TEXT("TagName=\"");
		TagString += *TagNames[0].Get();
		TagString += TEXT("\"");
		//TagString += TEXT(")");
	}
	FString CurrentDefaultValue = GraphPinObj->GetDefaultAsString();
	if (CurrentDefaultValue.IsEmpty() || CurrentDefaultValue == TEXT("(TagName=\"\")"))
	{
		CurrentDefaultValue = FString(TEXT(""));
	}
	if (!CurrentDefaultValue.Equals(TagName))
	{
		GraphPinObj->GetSchema()->TrySetDefaultValue(*GraphPinObj, TagName);
	}

}

#undef LOCTEXT_NAMESPACE