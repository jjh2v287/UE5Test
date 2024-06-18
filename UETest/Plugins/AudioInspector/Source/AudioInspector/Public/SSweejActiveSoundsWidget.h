// Copyright 2023, SweejTech Ltd. All Rights Reserved.

#pragma once

#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/SListView.h"
#include "FActiveSoundsWidgetRowInfo.h"

class FActiveTimerHandle;

class SActiveSoundListViewRow : public SMultiColumnTableRow<FActiveSoundsWidgetRowInfoPtr>
{
public:

	SLATE_BEGIN_ARGS(SActiveSoundListViewRow)
	{}
	/** The row we're working with to allow us to get information. */
	SLATE_ARGUMENT(FActiveSoundsWidgetRowInfoPtr, RowDataPtr)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView);

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override;

private:

	static void OpenEditorForAsset(UObject* Asset);

	static void SelectAssetInContentBrowser(const UObject* asset);

	FReply OnDoubleClicked(const FGeometry& MyGeometry, const FPointerEvent& PointerEvent);
	FReply OnDoubleClickedSoundClass(const FGeometry& MyGeometry, const FPointerEvent& PointerEvent);
	FReply OnDoubleClickedAttenuation(const FGeometry& MyGeometry, const FPointerEvent& PointerEvent);

	FActiveSoundsWidgetRowInfoPtr RowDataPtr;
};

/**
 * 
 */
class AUDIOINSPECTOR_API SSweejActiveSoundsWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSweejActiveSoundsWidget)
	{}
	SLATE_END_ARGS()

	static const FName NameColumnId;
	static const FName DistanceColumnId;
	static const FName VolumeColumnId;
	static const FName SoundClassColumnId;
	static const FName AttenuationColumnId;
	static const FName AngleToListenerColumnId;
	static const FName AzimuthColumnId;
	static const FName ElevationColumnId;

	static const int32 DistMaxFractionalDigits;
	static const int32 DistMinFractionalDigits;

	static const int32 VolumeMaxFractionalDigits;
	static const int32 VolumeMinFractionalDigits;

	static const int32 AngleToListenerFractionalDigits;

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	void SetTickInterval(float TickInterval);

	void BuildColumns();

	void RefreshColumnVisibility();

protected:

	static bool CompareUObjectsByName(TWeakObjectPtr<UObject> ObjectA, TWeakObjectPtr<UObject> ObjectB);
	static bool CompareFloatValues(float ValueA, float ValueB, EColumnSortMode::Type ColumnSortMode);

	EActiveTimerReturnType ActiveTimerTick(double InCurrentTime, float InDeltaTime);

	TSharedRef<ITableRow> OnGenerateWidgetForList(TSharedPtr<FActiveSoundsWidgetRowInfo> InItem, const TSharedRef<STableViewBase>& OwnerTable);

	void OnFilterTextChanged(const FText& SearchText);

	void OnHiddenColumnsListChangedHandler();

	void UpdateListView();
	void UpdateFilteredItems();

	EColumnSortMode::Type GetColumnSortMode() const { return ColumnSortMode; }

	void OnColumnSortModeChanged(const EColumnSortPriority::Type SortPriority, const FName& ColumnId, const EColumnSortMode::Type InSortMode);

private:

	// View
	TSharedPtr<SHeaderRow> ColumnsHeaderRow;

	TSharedPtr< SListView<FActiveSoundsWidgetRowInfoPtr> > ListView;

	// ViewModel
	TArray< TSharedPtr<FActiveSoundsWidgetRowInfo> > Items;

	// Filtering
	TArray< TSharedPtr<FActiveSoundsWidgetRowInfo> > FilteredItems;

	FText FilterText;

	// Column Sorting
	EColumnSortMode::Type ColumnSortMode = EColumnSortMode::Ascending;
	FName SortColumnId;

	// Timers
	TSharedPtr<FActiveTimerHandle> TickActiveTimerHandle;

};
