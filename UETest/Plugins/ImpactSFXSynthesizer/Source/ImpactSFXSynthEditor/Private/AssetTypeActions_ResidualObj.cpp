// Copyright 2023-2024, Le Binh Son, All rights reserved.

#include "AssetTypeActions_ResidualObj.h"

#include "AudioDevice.h"
#include "ContentBrowserMenuContexts.h"
#include "FCustomEditorUtils.h"
#include "ImpactSFXSynthEditor.h"
#include "ResidualDataEditorToolkit.h"
#include "ResidualDataFactory.h"
#include "ResidualObj.h"
#include "ResidualObjFactory.h"
#include "ResidualSynth.h"
#include "Algo/AnyOf.h"
#include "EditorFramework/AssetImportData.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

namespace LBSImpactSFXSynth
{
	namespace Editor
	{
		UClass* FAssetTypeActions_ResidualObj::GetSupportedClass() const
		{
			return UResidualObj::StaticClass();
		}
		
		FColor FAssetTypeActions_ResidualObj::GetTypeColor() const
		{
			return FColor::Red;
		}
	 
		uint32 FAssetTypeActions_ResidualObj::GetCategories()
		{
			if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
			{
				return FAssetToolsModule::GetModule().Get().FindAdvancedAssetCategory(FImpactSFXSynthEditorModule::ImpactSynthCategory);
			}
			else
				return EAssetTypeCategories::Misc;
		}

		void FAssetTypeActions_ResidualObj::GetResolvedSourceFilePaths(const TArray<UObject*>& TypeAssets, TArray<FString>& OutSourceFilePaths) const
		{
			for (auto& Asset : TypeAssets)
			{
				const auto ResidualObj = CastChecked<UResidualObj>(Asset);
				ResidualObj->AssetImportData->ExtractFilenames(OutSourceFilePaths);
			}
		}

		void FAssetTypeActions_ResidualObj::GetActions(const TArray<UObject*>& InObjects, FToolMenuSection& Section)
		{
			auto ResidualObjs = GetTypedWeakObjectPtrs<UResidualObj>(InObjects);

			Section.AddMenuEntry(
				"ResidualObj_CreateResidualData",
				LOCTEXT("ResidualData_Create", "Create ResidualData"),
				LOCTEXT("ResidualData_CreateTooltip", "Create ResidualData from selected objects."),
				FSlateIcon(),
				FUIAction(
				FExecuteAction::CreateSP(this, &FAssetTypeActions_ResidualObj::ExecuteCreateResidualData, ResidualObjs),
				FCanExecuteAction::CreateSP(this, &FAssetTypeActions_ResidualObj::CanExecuteCreateResidualDataCommand, ResidualObjs)
				)
				);
		}

		void FAssetTypeActions_ResidualObj::ExecuteCreateResidualData(TArray<TWeakObjectPtr<UResidualObj>> WeakObjs)
		{
			for (auto ObjIt = WeakObjs.CreateConstIterator(); ObjIt; ++ObjIt)
			{
				UResidualObj* ResidualObj = ObjIt->Get();
				if (ResidualObj)
				{
					const FString DefaultAffix = TEXT("RD_");
					FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");

					// Create the factory used to generate the asset
					UResidualDataFactory* Factory = NewObject<UResidualDataFactory>();
					Factory->StagedObj = ResidualObj; //StagedObj is null after consumed

					// Determine an appropriate name
					FString Name;
					FString PackagePath;
					FString PathName;
					FString ObjName;
					FString Extension;
					FPaths::Split(ResidualObj->GetPackage()->GetName(), PathName, ObjName, Extension);
					ObjName.RemoveFromStart(TEXT("RO_"), ESearchCase::IgnoreCase);
					PathName = FPaths::Combine(PathName, DefaultAffix + ObjName + Extension);
					
					AssetToolsModule.Get().CreateUniqueAssetName(PathName, FString(), PackagePath, Name);

					// create new asset
					AssetToolsModule.Get().CreateAsset(Name, FPackageName::GetLongPackagePath(PackagePath), UResidualData::StaticClass(), Factory);
				}
			}
		}

		bool FAssetTypeActions_ResidualObj::CanExecuteCreateResidualDataCommand(TArray<TWeakObjectPtr<UResidualObj>> WeakObjects)
		{
			return WeakObjects.Num() > 0;
		}

		TSharedPtr<SWidget> FAssetTypeActions_ResidualObj::GetThumbnailOverlay(const FAssetData& InAssetData) const
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
				if (UResidualObj* ResidualObj = Cast<UResidualObj>(Path.TryLoad()))
				{
					PlaySound(ResidualObj);
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

		void FAssetTypeActions_ResidualObj::PlaySound(UResidualObj* ResidualObj)
		{
			if(ResidualObj == nullptr || ResidualObj->Data.IsEmpty())
				return;
			
			if(FAudioDeviceHandle AudioDevice = GEngine->GetMainAudioDevice())
			{
				const int32 NumChannel = FMath::Min(1, AudioDevice->GetMaxChannels());
				if(NumChannel < 1)
					return;
				
				const int32 HopSize = ResidualObj->GetNumFFT() / 2;
				
				const float PlaySampleRate = AudioDevice->GetSampleRate();
				const auto ResidualDataAssetProxy =  MakeShared<FResidualDataAssetProxy>(ResidualObj);
				FResidualSynth ResidualSynth = FResidualSynth(PlaySampleRate, HopSize, NumChannel, ResidualDataAssetProxy,
													1.f, 1.f, 1.f, -1, 0.f, -1.f);

				Audio::FAlignedFloatBuffer SynthesizedData;
				FResidualSynth::SynthesizeFull(ResidualSynth, SynthesizedData);
				FResidualDataEditorToolkit::PlaySynthesizedSound(SynthesizedData);
			}
		}

		void FResidualObjExtension::RegisterMenus()
		{
			UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("ContentBrowser.AssetContextMenu.SoundWave");
			FToolMenuSection& Section = Menu->FindOrAddSection("GetAssetActions");

			Section.AddDynamicEntry("SoundWaveAssetConversion_CreateResidualObj", FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
			{
				if (const UContentBrowserAssetContextMenuContext* Context = InSection.FindContext<UContentBrowserAssetContextMenuContext>())
				{
					if (!Algo::AnyOf(Context->SelectedAssets, [](const FAssetData& AssetData){ return AssetData.IsInstanceOf<USoundWaveProcedural>(); }))
					{
						const TAttribute<FText> Label = LOCTEXT("SoundWave_CreateResidualObj", "Create Residual Object");
						const TAttribute<FText> ToolTip = LOCTEXT("SoundWave_CreateResidualObjTooltip", "Creates a residual object using the selected sound wave.");
						const FSlateIcon Icon = FSlateIcon();
						const FToolMenuExecuteAction UIAction = FToolMenuExecuteAction::CreateStatic(&FResidualObjExtension::ExecuteCreateResidualObjFromWave);

						InSection.AddMenuEntry("SoundWave_CreateResidualObj", Label, ToolTip, Icon, UIAction);
					}
				}
			}));
		}
		
		void FResidualObjExtension::ExecuteCreateResidualObjFromWave(const FToolMenuContext& MenuContext)
		{
			const FString DefaultAffix = TEXT("RO_");
			FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");

			// Create the factory used to generate the asset
			UResidualObjFactory* Factory = NewObject<UResidualObjFactory>();
	
			// only converts 0th selected object for now (see return statement)
			if (const UContentBrowserAssetContextMenuContext* Context = UContentBrowserAssetContextMenuContext::FindContextWithAssets(MenuContext))
			{
				for (USoundWave* Wave : Context->LoadSelectedObjectsIf<USoundWave>([](const FAssetData& AssetData) { return !AssetData.IsInstanceOf<USoundWaveProcedural>(); }))
				{
					Factory->StagedSoundWave = Wave;

					// Determine an appropriate name
					FString Name;
					FString PackagePath;

					FString PathName;
					FCustomEditorUtils::GetNameAffix(DefaultAffix, Wave->GetPackage(), PathName);
					AssetToolsModule.Get().CreateUniqueAssetName(PathName, FString(), PackagePath, Name);

					// create new asset
					AssetToolsModule.Get().CreateAsset(Name, FPackageName::GetLongPackagePath(PackagePath), UResidualObj::StaticClass(), Factory);
				}
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE