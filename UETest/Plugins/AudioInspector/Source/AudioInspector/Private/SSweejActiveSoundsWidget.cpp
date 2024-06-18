// Copyright 2023, SweejTech Ltd. All Rights Reserved.


#include "SSweejActiveSoundsWidget.h"
#include "SlateOptMacros.h"

#include "AudioInspectorSettings.h"
#include "AudioInspectorUtils.h"

#include "Audio.h"
#include "Audio/AudioDebug.h"
#include "AudioDeviceManager.h"
#include "AudioDevice.h"

#include "Editor.h"

#include "Subsystems/AssetEditorSubsystem.h"

#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Input/SSearchBox.h"

const FName SSweejActiveSoundsWidget::NameColumnId("Name_Column");
const FName SSweejActiveSoundsWidget::DistanceColumnId("Distance_Column");
const FName SSweejActiveSoundsWidget::VolumeColumnId("Volume_Column");
const FName SSweejActiveSoundsWidget::SoundClassColumnId("SoundClass_Column");
const FName SSweejActiveSoundsWidget::AttenuationColumnId("Attenuation_Column");
const FName SSweejActiveSoundsWidget::AngleToListenerColumnId("AngleToListener_Column");
const FName SSweejActiveSoundsWidget::AzimuthColumnId("Azimuth_Column");
const FName SSweejActiveSoundsWidget::ElevationColumnId("Elevation_Column");

const int32 SSweejActiveSoundsWidget::DistMaxFractionalDigits = 2;
const int32 SSweejActiveSoundsWidget::DistMinFractionalDigits = 2;
const int32 SSweejActiveSoundsWidget::VolumeMaxFractionalDigits = 1;
const int32 SSweejActiveSoundsWidget::VolumeMinFractionalDigits = 1;
const int32 SSweejActiveSoundsWidget::AngleToListenerFractionalDigits = 1;

#define LOCTEXT_NAMESPACE "SSweejActiveSoundsWidget"

void SActiveSoundListViewRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
{
	RowDataPtr = InArgs._RowDataPtr;

	SMultiColumnTableRow<FActiveSoundsWidgetRowInfoPtr>::Construct(FSuperRowType::FArguments(), InOwnerTableView);
}

TSharedRef<SWidget> SActiveSoundListViewRow::GenerateWidgetForColumn(const FName& InColumnName)
{
	FText Text;
	FSlateColor TextColor = FSlateColor::UseForeground();

	auto DoubleClickedHandlerFunc = &SActiveSoundListViewRow::OnDoubleClicked;

	if (InColumnName == SSweejActiveSoundsWidget::NameColumnId)
	{
		Text = FText::FromName(RowDataPtr->Name);
	}
	else if (InColumnName == SSweejActiveSoundsWidget::DistanceColumnId)
	{
		if (!RowDataPtr->AtOrigin)
		{
			FNumberFormattingOptions numberFormat = FNumberFormattingOptions();
			numberFormat.SetMaximumFractionalDigits(SSweejActiveSoundsWidget::DistMaxFractionalDigits);
			numberFormat.SetMinimumFractionalDigits(SSweejActiveSoundsWidget::DistMinFractionalDigits);

			Text = FText::AsNumber(RowDataPtr->Distance, &numberFormat);
		}
		else
		{
			// If the sound is at the origin, distance is meaningless. Note as a special case in a different color
			Text = LOCTEXT("Distance_Origin", "Origin");
			TextColor = FSlateColor(FColor::Turquoise);
		}
	}
	else if (InColumnName == SSweejActiveSoundsWidget::VolumeColumnId)
	{
		FNumberFormattingOptions volumeNumberFormat = FNumberFormattingOptions();
		volumeNumberFormat.SetMaximumFractionalDigits(SSweejActiveSoundsWidget::VolumeMaxFractionalDigits);
		volumeNumberFormat.SetMinimumFractionalDigits(SSweejActiveSoundsWidget::VolumeMinFractionalDigits);

		float VolumeDecibels = Audio::ConvertToDecibels(RowDataPtr->Volume);

		Text = FText::AsNumber(VolumeDecibels, &volumeNumberFormat);

		switch (GetDefault<UAudioInspectorSettings>()->VolumeColoringMode)
		{
		default:
		case EAudioInspectorVolumeColoringMode::None:
			TextColor = FSlateColor::UseForeground();
			break;

		case EAudioInspectorVolumeColoringMode::Relevance:
			TextColor = FSlateColor(AudioInspectorUtils::GetRelevanceColorForVolumeDecibels(VolumeDecibels));
			break;

		case EAudioInspectorVolumeColoringMode::VUMeter:
			TextColor = FSlateColor(AudioInspectorUtils::GetVUMeterColorForVolumeDecibels(VolumeDecibels));
			break;
		}
	}
	else if (InColumnName == SSweejActiveSoundsWidget::SoundClassColumnId)
	{
		if (RowDataPtr->SoundClass.IsValid())
		{
			Text = FText::FromName(RowDataPtr->SoundClass->GetFName());
		}

		DoubleClickedHandlerFunc = &SActiveSoundListViewRow::OnDoubleClickedSoundClass;
	}
	else if (InColumnName == SSweejActiveSoundsWidget::AttenuationColumnId)
	{
		if (RowDataPtr->Sound->AttenuationSettings)
		{
			Text = FText::FromName(RowDataPtr->Sound->AttenuationSettings->GetFName());
		}

		DoubleClickedHandlerFunc = &SActiveSoundListViewRow::OnDoubleClickedAttenuation;
	}
	else if (InColumnName == SSweejActiveSoundsWidget::AngleToListenerColumnId)
	{
		FNumberFormattingOptions AngleToListenerNumberFormat = FNumberFormattingOptions();
		AngleToListenerNumberFormat.SetMaximumFractionalDigits(SSweejActiveSoundsWidget::AngleToListenerFractionalDigits);
		AngleToListenerNumberFormat.SetMinimumFractionalDigits(SSweejActiveSoundsWidget::AngleToListenerFractionalDigits);

		Text = FText::AsNumber(RowDataPtr->AngleToListener, &AngleToListenerNumberFormat);
	}
	else if (InColumnName == SSweejActiveSoundsWidget::AzimuthColumnId)
	{
		FNumberFormattingOptions NumberFormat = FNumberFormattingOptions();
		NumberFormat.SetMaximumFractionalDigits(SSweejActiveSoundsWidget::AngleToListenerFractionalDigits);
		NumberFormat.SetMinimumFractionalDigits(SSweejActiveSoundsWidget::AngleToListenerFractionalDigits);

		Text = FText::AsNumber(RowDataPtr->AzimuthAndElevation.X, &NumberFormat);
	}
	else if (InColumnName == SSweejActiveSoundsWidget::ElevationColumnId)
	{
		FNumberFormattingOptions NumberFormat = FNumberFormattingOptions();
		NumberFormat.SetMaximumFractionalDigits(SSweejActiveSoundsWidget::AngleToListenerFractionalDigits);
		NumberFormat.SetMinimumFractionalDigits(SSweejActiveSoundsWidget::AngleToListenerFractionalDigits);

		Text = FText::AsNumber(RowDataPtr->AzimuthAndElevation.Y, &NumberFormat);
	}

	return SNew(STextBlock)
		.Text(Text)
		.ColorAndOpacity(TextColor)
		.Margin(FMargin(4.0f, 0.0f)) // Horizontal margin to make the text readable if columns run together
		.OnDoubleClicked(this, DoubleClickedHandlerFunc);
}

void SActiveSoundListViewRow::OpenEditorForAsset(UObject* Asset)
{
	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Asset);
}

void SActiveSoundListViewRow::SelectAssetInContentBrowser(const UObject* Asset)
{
	if (Asset)
	{
		const TArray<FAssetData>& Assets = { Asset };
		FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		ContentBrowserModule.Get().SyncBrowserToAssets(Assets);
	}
}

FReply SActiveSoundListViewRow::OnDoubleClicked(const FGeometry& MyGeometry, const FPointerEvent& PointerEvent)
{
	if (RowDataPtr->Sound.IsValid())
	{
		// When the user double clicks a row, open the default editor for the sound asset
		OpenEditorForAsset(RowDataPtr->Sound.Get());
	}
	
	return FReply::Handled();
}

FReply SActiveSoundListViewRow::OnDoubleClickedSoundClass(const FGeometry& MyGeometry, const FPointerEvent& PointerEvent)
{
	if (RowDataPtr->SoundClass.IsValid())
	{
		// When the user double clicks the soundclass value, open its default editor
		OpenEditorForAsset(RowDataPtr->SoundClass.Get());
	}

	return FReply::Handled();
}

FReply SActiveSoundListViewRow::OnDoubleClickedAttenuation(const FGeometry& MyGeometry, const FPointerEvent& PointerEvent)
{
	if (RowDataPtr->Sound.IsValid() && RowDataPtr->Sound->AttenuationSettings)
	{
		// When the user double clicks the soundclass value, open its default editor
		OpenEditorForAsset(RowDataPtr->Sound->AttenuationSettings.Get());
	}

	return FReply::Handled();
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SSweejActiveSoundsWidget::Construct(const FArguments& InArgs)
{
	FilterText = FText::GetEmpty();

	SortColumnId = DistanceColumnId;
	ColumnSortMode = EColumnSortMode::Ascending;

	ColumnsHeaderRow = SNew(SHeaderRow)
						.CanSelectGeneratedColumn(true)
						.OnHiddenColumnsListChanged_Raw(this, &SSweejActiveSoundsWidget::OnHiddenColumnsListChangedHandler);

	BuildColumns();

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SSearchBox)
			.HintText(LOCTEXT("Filter", "Filter"))
			.OnTextChanged(this, &SSweejActiveSoundsWidget::OnFilterTextChanged)
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SAssignNew(ListView, SListView< FActiveSoundsWidgetRowInfoPtr >)
			.ItemHeight(24)
			.SelectionMode(ESelectionMode::None)
			.HeaderRow(ColumnsHeaderRow)
			.ListItemsSource(&FilteredItems)
			.OnGenerateRow(this, &SSweejActiveSoundsWidget::OnGenerateWidgetForList)
		]
	];

	// Use the loaded value for update interval
	float UpdateInterval = GetDefault<UAudioInspectorSettings>()->ActiveSoundsUpdateInterval;

	// Cache the handle to the timer so we can reregister if the interval changes
	TickActiveTimerHandle = RegisterActiveTimer(UpdateInterval, FWidgetActiveTimerDelegate::CreateSP(this, &SSweejActiveSoundsWidget::ActiveTimerTick));
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SSweejActiveSoundsWidget::SetTickInterval(float TickInterval)
{
	// Re-register the timer when there is a new tick interval set

	if (TickActiveTimerHandle.IsValid())
	{
		UnRegisterActiveTimer(TickActiveTimerHandle.ToSharedRef());
	}

	TickActiveTimerHandle = RegisterActiveTimer(TickInterval, FWidgetActiveTimerDelegate::CreateSP(this, &SSweejActiveSoundsWidget::ActiveTimerTick));
}

void SSweejActiveSoundsWidget::BuildColumns()
{
	const UAudioInspectorSettings* Settings = GetDefault<UAudioInspectorSettings>();

	if (ColumnsHeaderRow && Settings)
	{
		ColumnsHeaderRow->ClearColumns();

		// Name Column
		ColumnsHeaderRow->AddColumn(
			SHeaderRow::Column(NameColumnId)
			.DefaultLabel(LOCTEXT("ColumnTitle_SoundName", "Name"))
			.FillWidth(0.7f)
			.SortMode(this, &SSweejActiveSoundsWidget::GetColumnSortMode)
			.OnSort(this, &SSweejActiveSoundsWidget::OnColumnSortModeChanged)
		);

		// Distance Column
		ColumnsHeaderRow->AddColumn(
			SHeaderRow::Column(DistanceColumnId)
			.DefaultLabel(LOCTEXT("ColumnTitle_Distance", "Distance"))
			.HAlignCell(EHorizontalAlignment::HAlign_Right)
			.FillWidth(0.15f)
			.SortMode(this, &SSweejActiveSoundsWidget::GetColumnSortMode)
			.OnSort(this, &SSweejActiveSoundsWidget::OnColumnSortModeChanged)
		);

		// Volume Column
		ColumnsHeaderRow->AddColumn(
			SHeaderRow::Column(VolumeColumnId)
			.DefaultLabel(LOCTEXT("ColumnTitle_Volume_dB", "Volume (dB)"))
			.HAlignCell(EHorizontalAlignment::HAlign_Right)
			.FillWidth(0.15f)
			.SortMode(this, &SSweejActiveSoundsWidget::GetColumnSortMode)
			.OnSort(this, &SSweejActiveSoundsWidget::OnColumnSortModeChanged)
		);

		// Sound Class
		ColumnsHeaderRow->AddColumn(
			SHeaderRow::Column(SoundClassColumnId)
			.DefaultLabel(LOCTEXT("ColumnTitle_SoundClass", "Sound Class"))
			.HAlignCell(EHorizontalAlignment::HAlign_Right)
			.FillWidth(0.15f)
			.SortMode(this, &SSweejActiveSoundsWidget::GetColumnSortMode)
			.OnSort(this, &SSweejActiveSoundsWidget::OnColumnSortModeChanged)
		);

		// Attenuation Asset
		ColumnsHeaderRow->AddColumn(
			SHeaderRow::Column(AttenuationColumnId)
			.DefaultLabel(LOCTEXT("ColumnTitle_Attenuation", "Attenuation"))
			.HAlignCell(EHorizontalAlignment::HAlign_Right)
			.FillWidth(0.15f)
			.SortMode(this, &SSweejActiveSoundsWidget::GetColumnSortMode)
			.OnSort(this, &SSweejActiveSoundsWidget::OnColumnSortModeChanged)
		);

		// Angle To Listener
		ColumnsHeaderRow->AddColumn(
			SHeaderRow::Column(AngleToListenerColumnId)
			.DefaultLabel(LOCTEXT("ColumnTitle_AngleToListener", "Angle To Listener"))
			.HAlignCell(EHorizontalAlignment::HAlign_Right)
			.FillWidth(0.15f)
			.SortMode(this, &SSweejActiveSoundsWidget::GetColumnSortMode)
			.OnSort(this, &SSweejActiveSoundsWidget::OnColumnSortModeChanged)
		);

		// Azimuth
		ColumnsHeaderRow->AddColumn(
			SHeaderRow::Column(AzimuthColumnId)
			.DefaultLabel(LOCTEXT("ColumnTitle_Azimuth", "Azimuth"))
			.HAlignCell(EHorizontalAlignment::HAlign_Right)
			.FillWidth(0.15f)
			.SortMode(this, &SSweejActiveSoundsWidget::GetColumnSortMode)
			.OnSort(this, &SSweejActiveSoundsWidget::OnColumnSortModeChanged)
		);
		
		// Elevation
		ColumnsHeaderRow->AddColumn(
			SHeaderRow::Column(ElevationColumnId)
			.DefaultLabel(LOCTEXT("ColumnTitle_Elevation", "Elevation"))
			.HAlignCell(EHorizontalAlignment::HAlign_Right)
			.FillWidth(0.15f)
			.SortMode(this, &SSweejActiveSoundsWidget::GetColumnSortMode)
			.OnSort(this, &SSweejActiveSoundsWidget::OnColumnSortModeChanged)
		);
	}
}

void SSweejActiveSoundsWidget::RefreshColumnVisibility()
{
	const UAudioInspectorSettings* Settings = GetDefault<UAudioInspectorSettings>();
	if (ColumnsHeaderRow && Settings)
	{
		ColumnsHeaderRow->SetShowGeneratedColumn(NameColumnId, Settings->ShowNameColumn);
		ColumnsHeaderRow->SetShowGeneratedColumn(DistanceColumnId, Settings->ShowDistanceColumn);
		ColumnsHeaderRow->SetShowGeneratedColumn(VolumeColumnId, Settings->ShowVolumeColumn);
		ColumnsHeaderRow->SetShowGeneratedColumn(SoundClassColumnId, Settings->ShowSoundClassColumn);
		ColumnsHeaderRow->SetShowGeneratedColumn(AttenuationColumnId, Settings->ShowAttenuationColumn);
		ColumnsHeaderRow->SetShowGeneratedColumn(AngleToListenerColumnId, Settings->ShowAngleToListenerColumn);
		ColumnsHeaderRow->SetShowGeneratedColumn(AzimuthColumnId, Settings->ShowAzimuthColumn);
		ColumnsHeaderRow->SetShowGeneratedColumn(ElevationColumnId, Settings->ShowElevationColumn);
	}
}

bool SSweejActiveSoundsWidget::CompareUObjectsByName(TWeakObjectPtr<UObject> ObjectA, TWeakObjectPtr<UObject> ObjectB)
{
	if (!ObjectA.IsValid()) return false;
	if (!ObjectB.IsValid()) return true;

	return ObjectA->GetFName().Compare(ObjectA->GetFName()) > 0;
}

bool SSweejActiveSoundsWidget::CompareFloatValues(float ValueA, float ValueB, EColumnSortMode::Type ColumnSortMode)
{
	switch (ColumnSortMode)
	{
	case EColumnSortMode::Ascending: return ValueA < ValueB;
	default:
	case EColumnSortMode::Descending: return ValueA >= ValueB;
	}
}

EActiveTimerReturnType SSweejActiveSoundsWidget::ActiveTimerTick(double InCurrentTime, float InDeltaTime)
{
	// Always empty the list because we don't want stale sounds hanging around if the audio device has been stopped
	Items.Empty();
	FilteredItems.Empty();

	// Early out if there's no game running
	if (!GEngine)
	{
		return EActiveTimerReturnType::Continue;
	}

	// Early out if no audio device
	FAudioDeviceHandle AudioDevice = GEngine->GetAudioDeviceManager()->GetActiveAudioDevice();
	if (!AudioDevice)
	{
		return EActiveTimerReturnType::Continue;
	}

	const TArray<FActiveSound*>& ActiveSounds = AudioDevice->GetActiveSounds();

	// Refresh the source items every tick
	for (int32 i = 0; i < ActiveSounds.Num(); i++)
	{
		FActiveSound* ActiveSound = ActiveSounds[i];
		check(ActiveSound);

		float AngleToListener = 0.0f;
		FVector2D AzimuthAndElevation = FVector2D::ZeroVector;

		FTransform ListenerTransform;
		if (AudioDevice->GetListenerTransform(ActiveSound->GetClosestListenerIndex(), ListenerTransform))
		{
			FTransform& SoundTransform = ActiveSound->Transform;
			FVector SoundToListenerVector = ListenerTransform.GetLocation() - SoundTransform.GetLocation();
			if (SoundToListenerVector.Normalize())
			{
				FVector SoundForward = SoundTransform.GetUnitAxis(EAxis::X);
				FVector SoundSide = SoundTransform.GetUnitAxis(EAxis::Y);
				FVector SoundUp = SoundTransform.GetUnitAxis(EAxis::Z);

				AngleToListener = FMath::RadiansToDegrees(FMath::Acos(FVector3f::DotProduct(FVector3f(SoundToListenerVector), FVector3f(SoundForward))));
				AzimuthAndElevation = FMath::RadiansToDegrees(FMath::GetAzimuthAndElevation(FVector(SoundToListenerVector), SoundForward, SoundSide, SoundUp));
			}
		}

		FActiveSoundsWidgetRowInfoPtr RowInfo = MakeShareable(new FActiveSoundsWidgetRowInfo());
		RowInfo->Sound = ActiveSound->GetSound();
		RowInfo->SoundClass = ActiveSound->GetSoundClass();
		RowInfo->Name = ActiveSound->GetSound()->GetFName();
		RowInfo->Distance = AudioDevice->GetDistanceToNearestListener(ActiveSound->LastLocation);
		RowInfo->AngleToListener = AngleToListener;
		RowInfo->AzimuthAndElevation = FVector2f(AzimuthAndElevation);
		RowInfo->AtOrigin = ActiveSound->LastLocation.IsZero();
		RowInfo->Volume = 0.0f;
		
		for (const TPair<UPTRINT, FWaveInstance*>& Pair : ActiveSound->GetWaveInstances())
		{
			check(Pair.Value);
			RowInfo->Volume = FMath::Max(RowInfo->Volume, Pair.Value->GetVolumeWithDistanceAndOcclusionAttenuation() * Pair.Value->GetDynamicVolume());
		}

		Items.Add(RowInfo);
	}

	// Derive the property to sort by from which column header has been clicked, and take direction into account
	Items.Sort([this](FActiveSoundsWidgetRowInfoPtr a, FActiveSoundsWidgetRowInfoPtr b) {

		// Sort by distance
		if (SortColumnId == DistanceColumnId)
		{
			return CompareFloatValues(a->Distance, b->Distance, ColumnSortMode);
		}

		// Sort by volume
		if (SortColumnId == VolumeColumnId)
		{
			return CompareFloatValues(a->Volume, b->Volume, ColumnSortMode);
		}

		// Sort by name
		if (SortColumnId == NameColumnId)
		{
			switch (ColumnSortMode)
			{
			case EColumnSortMode::Ascending: return a->Name.Compare(b->Name) > 0;
			default:
			case EColumnSortMode::Descending: return a->Name.Compare(b->Name) <= 0;
			}
		}

		// Sort by Sound Class name
		if (SortColumnId == SoundClassColumnId)
		{
			switch (ColumnSortMode)
			{
			case EColumnSortMode::Ascending: return CompareUObjectsByName(a->SoundClass, b->SoundClass);
			default:
			case EColumnSortMode::Descending: return !CompareUObjectsByName(a->SoundClass, b->SoundClass);
			}
		}

		// Sort by Attenuation
		if (SortColumnId == AttenuationColumnId)
		{
			switch (ColumnSortMode)
			{
			case EColumnSortMode::Ascending: return CompareUObjectsByName(a->Sound->AttenuationSettings, b->Sound->AttenuationSettings);
			default:
			case EColumnSortMode::Descending: return !CompareUObjectsByName(a->Sound->AttenuationSettings, b->Sound->AttenuationSettings);
			}
		}

		// Sort by angle to listener
		if (SortColumnId == AngleToListenerColumnId)
		{
			return CompareFloatValues(a->AngleToListener, b->AngleToListener, ColumnSortMode);
		}

		// Sort by Azimuth
		if (SortColumnId == AzimuthColumnId)
		{
			return CompareFloatValues(FMath::Abs(a->AzimuthAndElevation.X), FMath::Abs(b->AzimuthAndElevation.X), ColumnSortMode);
		}

		// Sort by Elevation
		if (SortColumnId == ElevationColumnId)
		{
			return CompareFloatValues(FMath::Abs(a->AzimuthAndElevation.Y), FMath::Abs(b->AzimuthAndElevation.Y), ColumnSortMode);
		}

		// Fall back on distance ascending
		return a->Distance > b->Distance;
	});

	UpdateFilteredItems();

	UpdateListView();

	return EActiveTimerReturnType::Continue;
}

TSharedRef<ITableRow> SSweejActiveSoundsWidget::OnGenerateWidgetForList(TSharedPtr<FActiveSoundsWidgetRowInfo> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SActiveSoundListViewRow, OwnerTable)
		.RowDataPtr(InItem);
}

void SSweejActiveSoundsWidget::OnFilterTextChanged(const FText& SearchText)
{
	FilterText = SearchText;
	
	UpdateFilteredItems();

	UpdateListView();
}

void SSweejActiveSoundsWidget::OnHiddenColumnsListChangedHandler()
{
	if (UAudioInspectorSettings* Settings = GetMutableDefault<UAudioInspectorSettings>();
		Settings && ColumnsHeaderRow)
	{
		TArray<FName> HiddenColumnIDs = ColumnsHeaderRow->GetHiddenColumnIds();

		Settings->ShowNameColumn = !HiddenColumnIDs.Contains(NameColumnId);
		Settings->ShowDistanceColumn = !HiddenColumnIDs.Contains(DistanceColumnId);
		Settings->ShowVolumeColumn = !HiddenColumnIDs.Contains(VolumeColumnId);
		Settings->ShowSoundClassColumn = !HiddenColumnIDs.Contains(SoundClassColumnId);
		Settings->ShowAttenuationColumn = !HiddenColumnIDs.Contains(AttenuationColumnId);
		Settings->ShowAngleToListenerColumn = !HiddenColumnIDs.Contains(AngleToListenerColumnId);
		Settings->ShowAzimuthColumn = !HiddenColumnIDs.Contains(AzimuthColumnId);
		Settings->ShowElevationColumn = !HiddenColumnIDs.Contains(ElevationColumnId);

		Settings->SaveConfig();
	}
}

void SSweejActiveSoundsWidget::UpdateListView()
{
	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();
	}
}

void SSweejActiveSoundsWidget::UpdateFilteredItems()
{
	const FString& FilterTextStr = FilterText.ToString();

	// If there is a string to filter by
	if (!FilterTextStr.IsEmpty())
	{
		// Empty the filtered items vm
		FilteredItems.Empty();

		// Filter active sound items by checking whether their name contains the filter text
		FilteredItems = Items.FilterByPredicate([&FilterTextStr](const FActiveSoundsWidgetRowInfoPtr Candidate) 
			{
				return Candidate->Name.ToString().Contains(FilterTextStr);
			});
	}
	else
	{
		// If no filter string, assign all items to the filtered list
		FilteredItems = Items;
	}
}

void SSweejActiveSoundsWidget::OnColumnSortModeChanged(const EColumnSortPriority::Type SortPriority, const FName& ColumnId, const EColumnSortMode::Type InSortMode)
{
	SortColumnId = ColumnId;
	ColumnSortMode = InSortMode;
}

#undef LOCTEXT_NAMESPACE
