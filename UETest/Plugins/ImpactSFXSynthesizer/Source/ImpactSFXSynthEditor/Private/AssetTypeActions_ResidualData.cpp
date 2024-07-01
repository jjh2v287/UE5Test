// Copyright 2023-2024, Le Binh Son, All rights reserved.

#include "AssetTypeActions_ResidualData.h"

#include "FCustomEditorUtils.h"
#include "ImpactSFXSynthEditor.h"
#include "ResidualData.h"
#include "ResidualDataEditorToolkit.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

namespace LBSImpactSFXSynth
{
	namespace Editor
	{
		UClass* FAssetTypeActions_ResidualData::GetSupportedClass() const
		{
			return UResidualData::StaticClass();
		}
		
		FColor FAssetTypeActions_ResidualData::GetTypeColor() const
		{
			return FColor::Cyan;
		}
	 
		uint32 FAssetTypeActions_ResidualData::GetCategories()
		{
			if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
			{
				return FAssetToolsModule::GetModule().Get().FindAdvancedAssetCategory(FImpactSFXSynthEditorModule::ImpactSynthCategory);
			}
			else
				return EAssetTypeCategories::Misc;
		}

		void FAssetTypeActions_ResidualData::GetActions(const TArray<UObject*>& InObjects, FToolMenuSection& Section)
		{
			auto Sounds = GetTypedWeakObjectPtrs<UResidualData>(InObjects);

			Section.AddMenuEntry(
				"Residual_PlaySound",
				LOCTEXT("Residual_PlaySound", "Play"),
				LOCTEXT("Residual_PlaySoundTooltip", "Plays the selected residual data."),
				FSlateIcon(),
				FUIAction(
				FExecuteAction::CreateSP(this, &FAssetTypeActions_ResidualData::ExecutePlaySound, Sounds),
				FCanExecuteAction::CreateSP(this, &FAssetTypeActions_ResidualData::CanExecutePlayCommand, Sounds)
				)
				);

			Section.AddMenuEntry(
				"Residual_StopSound",
				LOCTEXT("Residual_StopSound", "Stop"),
				LOCTEXT("Residual_StopSoundTooltip", "Stops the selected residual data."),
				FSlateIcon(),
				FUIAction(
				FExecuteAction::CreateSP(this, &FAssetTypeActions_ResidualData::ExecuteStopSound, Sounds),
				FCanExecuteAction()
				)
				);
		}

		void FAssetTypeActions_ResidualData::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> ToolkitHost)
		{
			EToolkitMode::Type Mode = ToolkitHost.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

			for (UObject* Object : InObjects)
			{
				if (UResidualData* ResidualData = Cast<UResidualData>(Object))
				{
					TSharedRef<FResidualDataEditorToolkit> Editor = MakeShared<FResidualDataEditorToolkit>();
					Editor->Init(Mode, ToolkitHost, ResidualData);
				}
			}
		}

		bool FAssetTypeActions_ResidualData::AssetsActivatedOverride(const TArray<UObject*>& InObjects,
			EAssetTypeActivationMethod::Type ActivationType)
		{
			if(ActivationType == EAssetTypeActivationMethod::Previewed)
			{
				for (UObject* InObject : InObjects)
				{
					if(UResidualData* ResidualData = Cast<UResidualData>(InObject))
					{
						Audio::FAlignedFloatBuffer SynthData;
						if(FResidualDataEditorToolkit::SynthesizePreviewedSound(ResidualData, SynthData))
						{
							//Only play the first valid data
							return FResidualDataEditorToolkit::PlaySynthesizedSound(SynthData);
						}
					}
				}
			}
			
			return FAssetTypeActions_Base::AssetsActivatedOverride(InObjects, ActivationType);
		}

		TSharedPtr<SWidget> FAssetTypeActions_ResidualData::GetThumbnailOverlay(const FAssetData& InAssetData) const
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
				if (UResidualData* ResidualData = Cast<UResidualData>(Path.TryLoad()))
				{
					PlaySound(ResidualData);
				}

				return FReply::Handled();
			};

			auto OnToolTipTextLambda = [ClassName = GetSupportedClass()->GetFName(), Path = InAssetData.ToSoftObjectPath()]()
			{
				return FText::Format(LOCTEXT("PreviewResidualDataIconToolTip_Editor", "Preview {0}"), FText::FromName(ClassName));
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

		bool FAssetTypeActions_ResidualData::CanExecutePlayCommand(TArray<TWeakObjectPtr<UResidualData>> InObjects) const
		{
			return InObjects.Num() == 1;
		}

		void FAssetTypeActions_ResidualData::ExecutePlaySound(TArray<TWeakObjectPtr<UResidualData>> WeakObjects)
		{
			for (auto ObjIt = WeakObjects.CreateConstIterator(); ObjIt; ++ObjIt)
            {
            	UResidualData* ResidualData = (*ObjIt).Get();
            	if (ResidualData)
            	{
            		// Only play the first valid ResidualData
            		PlaySound(ResidualData);
            		break;
            	}
            }
		}

		void FAssetTypeActions_ResidualData::ExecuteStopSound(TArray<TWeakObjectPtr<UResidualData>> WeakObjects)
		{
			StopSound();
		}
		
		void FAssetTypeActions_ResidualData::PlaySound(UResidualData* ResidualData)
		{
			if (ResidualData)
			{
				Audio::FAlignedFloatBuffer SynthData;
				if(FResidualDataEditorToolkit::SynthesizePreviewedSound(ResidualData, SynthData))
					FResidualDataEditorToolkit::PlaySynthesizedSound(SynthData);
			}
			else
			{
				StopSound();
			}
		}

		void FAssetTypeActions_ResidualData::StopSound()
		{
			GEditor->ResetPreviewAudioComponent();
		}
	}
}

#undef LOCTEXT_NAMESPACE
