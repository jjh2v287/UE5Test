// Copyright 2019 - 2021, butterfly, Event System Plugin, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Layout/Visibility.h"
#include "Widgets/SWidget.h"
#include "Editor/PropertyEditor/Public/IPropertyTypeCustomization.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"
#include "SEventWidget.h"
#include "EditorUndoClient.h"

class IPropertyHandle;

/** Customization for the gameplay tag container struct */
class FEventContainerCustomization : public IPropertyTypeCustomization, public FEditorUndoClient
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShareable(new FEventContainerCustomization);
	}

	~FEventContainerCustomization();

	/** Overridden to show an edit button to launch the gameplay tag editor */
	virtual void CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	
	/** Overridden to do nothing */
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override {}

	//~ Begin FEditorUndoClient Interface
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;
	//~ End FEditorUndoClient Interface

private:

	/** Called when the clear all button is clicked; Clears all selected tags in the container*/
	FReply OnClearAllButtonClicked();

	/** Returns the visibility of the "clear all" button (collapsed when there are no tags) */
	EVisibility GetClearAllVisibility() const;

	/** Returns the visibility of the tags list box (collapsed when there are no tags) */
	EVisibility GetTagsListVisibility() const;

	/** Returns the selected tags list widget*/
	TSharedRef<SWidget> ActiveTags();

	/** Updates the list of selected tags*/
	void RefreshTagList();

	/** On Generate Row Delegate */
	TSharedRef<ITableRow> MakeListViewWidget(TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& OwnerTable);

	/** Build List of Editable Containers */
	void BuildEditableContainerList();

	/** Callback function to create content for the combo button. */
	TSharedRef<SWidget> GetListContent();

	void OnEventListMenuOpenStateChanged(bool bIsOpened);

	/** Cached property handle */
	TSharedPtr<IPropertyHandle> StructPropertyHandle;

	/** The array of containers this objects has */
	TArray<SEventWidget::FEditableEventContainerDatum> EditableContainers;

	/** List of tag names selected in the tag containers*/
	TArray< TSharedPtr<FString> > TagNames;

	/** The TagList, kept as a member so we can update it later */
	TSharedPtr<SListView<TSharedPtr<FString>>> TagListView;

	TSharedPtr<class SComboButton> EditButton;

	TWeakPtr<class SEventWidget> LastTagWidget;

	void OnTagDoubleClicked(FString TagName);
	FReply OnRemoveTagClicked(FString TagName);

	FReply OnSingleTagMouseButtonPressed(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, FString TagName);
	void OnSingleTagSearchForReferences(FString TagName);
	void OnWholeContainerSearchForReferences();
};

