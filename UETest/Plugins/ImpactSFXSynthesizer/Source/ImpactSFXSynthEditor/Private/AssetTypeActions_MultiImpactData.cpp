// Copyright 2023-2024, Le Binh Son, All rights reserved.

#include "AssetTypeActions_MultiImpactData.h"

#include "FCustomEditorUtils.h"
#include "ImpactSFXSynthEditor.h"
#include "MultiImpactData.h"
#include "MultiImpactDataToolkit.h"
#include "ResidualDataEditorToolkit.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

namespace LBSImpactSFXSynth
{
	namespace Editor
	{
		UClass* FAssetTypeActions_MultiImpactData::GetSupportedClass() const
		{
			return UMultiImpactData::StaticClass();
		}
		
		FColor FAssetTypeActions_MultiImpactData::GetTypeColor() const
		{
			return FColor::Cyan;
		}
	 
		uint32 FAssetTypeActions_MultiImpactData::GetCategories()
		{
			if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
			{
				return FAssetToolsModule::GetModule().Get().FindAdvancedAssetCategory(FImpactSFXSynthEditorModule::ImpactSynthCategory);
			}
			else
				return EAssetTypeCategories::Misc;
		}

		void FAssetTypeActions_MultiImpactData::OpenAssetEditor(const TArray<UObject*>& InObjects,
			TSharedPtr<IToolkitHost> ToolkitHost)
		{
			EToolkitMode::Type Mode = ToolkitHost.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

			for (UObject* Object : InObjects)
			{
				if (UMultiImpactData* MultiImpactData = Cast<UMultiImpactData>(Object))
				{
					TSharedRef<FMultiImpactDataToolkit> Editor = MakeShared<FMultiImpactDataToolkit>();
					Editor->Init(Mode, ToolkitHost, MultiImpactData);
				}
			}
		}

		TSharedPtr<SWidget> FAssetTypeActions_MultiImpactData::GetThumbnailOverlay(const FAssetData& InAssetData) const
		{
			TSharedPtr<SBox> Box = SNew(SBox)
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				.Padding(2.0f);

			auto OnGetDisplayPlaybackLambda = [Box, Path = InAssetData.ToSoftObjectPath()]()
			{
				const bool bIsValid = Box.IsValid();
				if (!bIsValid)
					return FCoreStyle::Get().GetBrush(TEXT("NoBrush"));
				
				const bool bIsHovered = Box->IsHovered();
				if (!bIsHovered)
					return FCoreStyle::Get().GetBrush(TEXT("NoBrush"));

				FString IconName = TEXT("MetasoundEditor.Play.Thumbnail");
				if (bIsHovered)
					IconName += TEXT(".Hovered");
				
				return &FCustomEditorUtils::GetSlateBrushSafe(FName(*IconName));
			};

			auto OnClickedLambda = [Path = InAssetData.ToSoftObjectPath()]()
			{
				if (UMultiImpactData* MultiImpactData = Cast<UMultiImpactData>(Path.TryLoad()))
				{
					PlaySound(MultiImpactData);
				}

				return FReply::Handled();
			};

			auto OnToolTipTextLambda = [ClassName = GetSupportedClass()->GetFName(), Path = InAssetData.ToSoftObjectPath()]()
			{
				return FText::Format(LOCTEXT("PreviewImpactModalDataIconToolTip_Editor", "Preview {0}"), FText::FromName(ClassName));
			};
			
			auto OnGetVisibilityLambda = [Box, InAssetData]() -> EVisibility
			{
				if(Box.IsValid() && Box->IsHovered())
				{
					return EVisibility::Visible;
				}
				return EVisibility::Hidden;
			};
			
			TSharedPtr<SButton> Widget;
			SAssignNew(Widget, SButton)
				.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.ToolTipText_Lambda(OnToolTipTextLambda)
				.Cursor(EMouseCursor::Default) // The outer widget can specify a DragHand cursor, so overridden here
				.ForegroundColor(FSlateColor::UseForeground())
				.IsFocusable(false)
				.OnClicked_Lambda(OnClickedLambda)
				.Visibility_Lambda(OnGetVisibilityLambda)
				[
					SNew(SImage)
					.Image_Lambda(OnGetDisplayPlaybackLambda)
				];
			
			Box->SetContent(Widget.ToSharedRef());
			Box->SetVisibility(EVisibility::Visible);
			return Box;
		}

		void FAssetTypeActions_MultiImpactData::PlaySound(UMultiImpactData* MultiImpactData)
		{
			if(!GEditor->CanPlayEditorSound())
				return;
			
			if (MultiImpactData)
			{
				Audio::FAlignedFloatBuffer SynthesizedData;
				if(const FImpactSpawnInfo* SpawnInfo = MultiImpactData->GetCurrentValidPreviewSpawnInfo())
				{
					const float SamplingRate = FMultiImpactDataToolkit::SynthesizeImpact(MultiImpactData, SynthesizedData, *SpawnInfo);
					if(SamplingRate > 0.f)
						FResidualDataEditorToolkit::PlaySynthesizedSound(SynthesizedData);
				}
			}
			else
			{
				GEditor->ResetPreviewAudioComponent();
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
