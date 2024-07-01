// Copyright 2023-2024, Le Binh Son, All rights reserved.
#include "AssetTypeActions_ImpactModalObj.h"

#include "AudioDevice.h"
#include "FCustomEditorUtils.h"
#include "ImpactSFXSynthEditor.h"
#include "ImpactModalObj.h"
#include "ModalSynth.h"
#include "ResidualDataEditorToolkit.h"
#include "EditorFramework/AssetImportData.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

namespace LBSImpactSFXSynth
{
	namespace Editor
	{
		UClass* FAssetTypeActions_ImpactModalObj::GetSupportedClass() const
		{
			return UImpactModalObj::StaticClass();
		}
		
		FColor FAssetTypeActions_ImpactModalObj::GetTypeColor() const
		{
			return FColor::Red;
		}
	 
		uint32 FAssetTypeActions_ImpactModalObj::GetCategories()
		{
			if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
			{
				return FAssetToolsModule::GetModule().Get().FindAdvancedAssetCategory(FImpactSFXSynthEditorModule::ImpactSynthCategory);
			}
			else
				return EAssetTypeCategories::Misc;
		}

		void FAssetTypeActions_ImpactModalObj::GetResolvedSourceFilePaths(const TArray<UObject*>& TypeAssets, TArray<FString>& OutSourceFilePaths) const
		{
			for (auto& Asset : TypeAssets)
			{
				const auto ImpactModalObj = CastChecked<UImpactModalObj>(Asset);
				ImpactModalObj->AssetImportData->ExtractFilenames(OutSourceFilePaths);
			}
		}

		TSharedPtr<SWidget> FAssetTypeActions_ImpactModalObj::GetThumbnailOverlay(const FAssetData& InAssetData) const
		{
			//Impact sounds are short so we don't implement a stop function here to keep logic simple
			//User can always stop sound through right click command
			
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
				if (UImpactModalObj* ImpactModalData = Cast<UImpactModalObj>(Path.TryLoad()))
				{
					PlaySound(ImpactModalData);
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

		void FAssetTypeActions_ImpactModalObj::PlaySound(UImpactModalObj* ImpactModalData)
		{
			if(!GEditor->CanPlayEditorSound())
				return;
			
			if (ImpactModalData)
			{
				const FImpactModalObjAssetProxyPtr& ProxyPtr = ImpactModalData->GetProxy();
				if(FAudioDeviceHandle AudioDevice = GEngine->GetMainAudioDevice())
				{
					TArray<float> SynthData;
					FModalSynth ModalSynth = FModalSynth(AudioDevice->GetSampleRate(), ProxyPtr);
					FModalSynth::SynthesizeFullOneChannel(ModalSynth, ProxyPtr, SynthData);
					FResidualDataEditorToolkit::PlaySynthesizedSound(SynthData);
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