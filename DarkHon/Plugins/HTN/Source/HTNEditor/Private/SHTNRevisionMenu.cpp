// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "SHTNRevisionMenu.h"

#include "HTN.h"
#include "HTNAppStyle.h"

#include "Framework/Commands/UIAction.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "HAL/PlatformMath.h"
#include "IAssetTypeActions.h"
#include "ISourceControlModule.h"
#include "ISourceControlRevision.h"
#include "ISourceControlState.h"
#include "Misc/AssertionMacros.h"
#include "Misc/Attribute.h"
#include "SlotBase.h"
#include "SourceControlHelpers.h"
#include "SourceControlOperations.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SHTNRevisionMenu"

namespace
{
	int32 FindLatestRevisionNumber(const ISourceControlState& SourceControlState)
	{
		int32 Result = 0;
		for (int32 HistoryIndex = 0; HistoryIndex < SourceControlState.GetHistorySize(); ++HistoryIndex)
		{
			if (const TSharedPtr<ISourceControlRevision, ESPMode::ThreadSafe> Revision = SourceControlState.GetHistoryItem(HistoryIndex))
			{
				if (Revision->GetRevisionNumber() > Result)
				{
					Result = Revision->GetRevisionNumber();
				}
			}
		}
		return Result;
	}
}

SHTNRevisionMenu::~SHTNRevisionMenu()
{
	// Cancel any operation if this widget is destroyed while in progress
	if (SourceControlQueryState == ESourceControlQueryState::QueryInProgress)
	{
		ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
		if (SourceControlQueryOp.IsValid() && SourceControlProvider.CanCancelOperation(SourceControlQueryOp.ToSharedRef()))
		{
			SourceControlProvider.CancelOperation(SourceControlQueryOp.ToSharedRef());
		}
	}
}

void SHTNRevisionMenu::Construct(const FArguments& InArgs, const UHTN* HTN)
{
	bIncludeLocalRevision = InArgs._bIncludeLocalRevision;
	OnRevisionSelected = InArgs._OnRevisionSelected;

	ChildSlot	
	[
		SAssignNew(MenuBox, SVerticalBox)
		+SVerticalBox::Slot()
		[
			SNew(SBorder)
			.Visibility(this, &SHTNRevisionMenu::GetInProgressVisibility)
			.BorderImage(FHTNAppStyle::GetBrush("Menu.Background"))
			.Content()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SThrobber)
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(2.0f, 0.0f, 4.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("DiffMenuOperationInProgress", "Updating history..."))
				]
				+SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.Visibility(this, &SHTNRevisionMenu::GetCancelButtonVisibility)
					.OnClicked(this, &SHTNRevisionMenu::OnCancelButtonClicked)
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Center)
					.Content()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("DiffMenuCancelButton", "Cancel"))
					]
				]
			]
		]
	];

	if (HTN)
	{
		SourceControlQueryState = ESourceControlQueryState::QueryInProgress;

		Filename = SourceControlHelpers::PackageFilename(HTN->GetPathName());

		// Make sure the history info is up to date
		SourceControlQueryOp = ISourceControlOperation::Create<FUpdateStatus>();
		SourceControlQueryOp->SetUpdateHistory(true);
		ISourceControlModule::Get().GetProvider().Execute(SourceControlQueryOp.ToSharedRef(), 
			Filename, EConcurrency::Asynchronous, 
			FSourceControlOperationComplete::CreateSP(this, &SHTNRevisionMenu::OnSourceControlQueryComplete));
	}
}

EVisibility SHTNRevisionMenu::GetInProgressVisibility() const
{
	return (SourceControlQueryState == ESourceControlQueryState::QueryInProgress) ? 
		EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SHTNRevisionMenu::GetCancelButtonVisibility() const
{
	ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
	return SourceControlQueryOp.IsValid() && SourceControlProvider.CanCancelOperation(SourceControlQueryOp.ToSharedRef()) ? 
		EVisibility::Visible : EVisibility::Collapsed;
}

FReply SHTNRevisionMenu::OnCancelButtonClicked() const
{
	if (SourceControlQueryOp.IsValid())
	{
		ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
		SourceControlProvider.CancelOperation(SourceControlQueryOp.ToSharedRef());
	}

	return FReply::Handled();
}

void SHTNRevisionMenu::OnSourceControlQueryComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult)
{
	check(SourceControlQueryOp == InOperation);

	// Add pop-out menu for each revision
	FMenuBuilder MenuBuilder(/*bInShouldCloseWindowAfterMenuSelection =*/true, /*InCommandList =*/nullptr);

	MenuBuilder.BeginSection("AddDiffRevision", LOCTEXT("Revisions", "Revisions"));
	if (bIncludeLocalRevision)
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("LocalRevision", "Local"), 
			LOCTEXT("LocalRevisionToolTip", "The current copy you have saved to disk (locally)"), 
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([OnRevisionSelectedDelegate = OnRevisionSelected]()
		{
			OnRevisionSelectedDelegate.ExecuteIfBound(FRevisionInfo::InvalidRevision());
		})));
	}

	if (InResult == ECommandResult::Succeeded)
	{
		// Get the cached state
		ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
		FSourceControlStatePtr SourceControlState = SourceControlProvider.GetState(Filename, EStateCacheUsage::Use);

		if (SourceControlState.IsValid() && SourceControlState->GetHistorySize() > 0)
		{
			// Figure out the highest revision # (so we can label it "Depot")
			const int32 LatestRevision = FindLatestRevisionNumber(*SourceControlState);

			for (int32 HistoryIndex = 0; HistoryIndex < SourceControlState->GetHistorySize(); HistoryIndex++)
			{
				if (TSharedPtr<ISourceControlRevision, ESPMode::ThreadSafe> Revision = SourceControlState->GetHistoryItem(HistoryIndex))
				{
					FInternationalization& I18N = FInternationalization::Get();

					FFormatNamedArguments Args;
					Args.Add(TEXT("CheckInNumber"), FText::AsNumber(Revision->GetCheckInIdentifier(), nullptr, I18N.GetInvariantCulture()));
					Args.Add(TEXT("Revision"), FText::FromString(Revision->GetRevision()));
					Args.Add(TEXT("UserName"), FText::FromString(Revision->GetUserName()));
					Args.Add(TEXT("DateTime"), FText::AsDate(Revision->GetDate()));
					Args.Add(TEXT("ChanglistDescription"), FText::FromString(Revision->GetDescription()));
					FText ToolTipText;
					if (ISourceControlModule::Get().GetProvider().UsesChangelists())
					{
						ToolTipText = FText::Format(LOCTEXT("ChangelistToolTip", "CL #{CheckInNumber} {UserName} \n{DateTime} \n{ChanglistDescription}"), Args);
					}
					else
					{
						ToolTipText = FText::Format(LOCTEXT("RevisionToolTip", "{Revision} {UserName} \n{DateTime} \n{ChanglistDescription}"), Args);
					}

					const FText Label = Revision->GetRevisionNumber() == LatestRevision ?
						LOCTEXT("Depo", "Depot") : 
						FText::Format(LOCTEXT("RevisionNumber", "Revision {0}"),
							FText::AsNumber(Revision->GetRevisionNumber(), nullptr, I18N.GetInvariantCulture()));

					const FRevisionInfo RevisionInfo
					{ 
						Revision->GetRevision(), 
						Revision->GetCheckInIdentifier(), 
						Revision->GetDate() 
					};
					MenuBuilder.AddMenuEntry(
						TAttribute<FText>(Label), ToolTipText, FSlateIcon(),
						FUIAction(FExecuteAction::CreateLambda([RevisionInfo, OnRevisionSelectedDelegate = OnRevisionSelected]()
					{
						OnRevisionSelectedDelegate.ExecuteIfBound(RevisionInfo);
					})));
				}
			}
		}
		else if (!bIncludeLocalRevision)
		{
			// Show 'empty' item in toolbar
			MenuBuilder.AddMenuEntry(LOCTEXT("NoRevisonHistory", "No revisions found"),
				FText(), FSlateIcon(), FUIAction());
		}
	}
	else if (!bIncludeLocalRevision)
	{
		// Show 'empty' item in toolbar
		MenuBuilder.AddMenuEntry(LOCTEXT("NoRevisonHistory", "No revisions found"),
			FText(), FSlateIcon(), FUIAction());
	}

	MenuBuilder.EndSection();
	MenuBox->AddSlot() 
	[
		MenuBuilder.MakeWidget(nullptr, 500)
	];

	SourceControlQueryOp.Reset();
	SourceControlQueryState = ESourceControlQueryState::Queried;
}

#undef LOCTEXT_NAMESPACE
