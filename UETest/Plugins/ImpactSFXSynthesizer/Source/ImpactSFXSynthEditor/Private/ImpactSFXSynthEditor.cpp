// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#include "ImpactSFXSynthEditor.h"
#include "ICurveEditorModule.h"
#include "ImpactCurveLayoutCuztomizationBase.h"
#include "ImpactSpawnInfoLayoutCustomization.h"
#include "ImpactSynthCurveEditorModel.h"
#include "PhaseEffectLayoutCustomization.h"
#include "PropertyEditorModule.h"

DEFINE_LOG_CATEGORY(LogImpactSFXSynthEditor);

#define LOCTEXT_NAMESPACE "FImpactSFXSynthEditorModule"

namespace LBSImpactSFXSynth
{
	namespace Editor
	{
		
		const FName FImpactSFXSynthEditorModule::ImpactSynthCategory = FName(TEXT("ImpactSFXSynthCat"));
		
		void FImpactSFXSynthEditorModule::StartupModule()
		{
			FAssetToolsModule::GetModule().Get().RegisterAdvancedAssetCategory(ImpactSynthCategory,
																				FText::FromString("ImpactSFXSynth"));
			
			FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
			PropertyModule.RegisterCustomPropertyTypeLayout
			(
				"ImpactSynthCurve",
				FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FImpactCurveCustomization::MakeInstance)
			);
			PropertyModule.RegisterCustomPropertyTypeLayout
			(
				"ResidualPhaseEffect",
				FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FPhaseEffectLayoutCustomization::MakeInstance)
			);			
			PropertyModule.RegisterCustomPropertyTypeLayout
			(
				"ImpactSpawnInfo",
				FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FImpactSpawnInfoLayoutCustomization::MakeInstance)
			);
			
			ICurveEditorModule& CurveEditorModule = FModuleManager::LoadModuleChecked<ICurveEditorModule>(TEXT("CurveEditor"));
			FImpactSynthCurveModel::WaveTableViewId = CurveEditorModule.RegisterView(FOnCreateCurveEditorView::CreateStatic(
				[](TWeakPtr<FCurveEditor> WeakCurveEditor) -> TSharedRef<SCurveEditorView>
				{
					return SNew(SViewStacked, WeakCurveEditor);
				}
			));
			
			ResidualDataTypeActions = MakeShared<FAssetTypeActions_ResidualData>();
			FAssetToolsModule::GetModule().Get().RegisterAssetTypeActions(ResidualDataTypeActions.ToSharedRef());
			
			ResidualObjTypeActions = MakeShared<FAssetTypeActions_ResidualObj>();
			FAssetToolsModule::GetModule().Get().RegisterAssetTypeActions(ResidualObjTypeActions.ToSharedRef());

			ImpactModalObjTypeActions = MakeShared<FAssetTypeActions_ImpactModalObj>();
			FAssetToolsModule::GetModule().Get().RegisterAssetTypeActions(ImpactModalObjTypeActions.ToSharedRef());
			
			MultiImpactDataActions = MakeShared<FAssetTypeActions_MultiImpactData>();
			FAssetToolsModule::GetModule().Get().RegisterAssetTypeActions(MultiImpactDataActions.ToSharedRef());

			UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FImpactSFXSynthEditorModule::RegisterMenus));
		}

		void FImpactSFXSynthEditorModule::ShutdownModule()
		{
			if (!FModuleManager::Get().IsModuleLoaded("AssetTools"))
				return;
			
			FAssetToolsModule::GetModule().Get().UnregisterAssetTypeActions(ResidualDataTypeActions.ToSharedRef());
			FAssetToolsModule::GetModule().Get().UnregisterAssetTypeActions(ResidualObjTypeActions.ToSharedRef());
			FAssetToolsModule::GetModule().Get().UnregisterAssetTypeActions(ImpactModalObjTypeActions.ToSharedRef());
			FAssetToolsModule::GetModule().Get().UnregisterAssetTypeActions(MultiImpactDataActions.ToSharedRef());
		}

		void FImpactSFXSynthEditorModule::RegisterMenus()
		{
			FToolMenuOwnerScoped MenuOwner(this);
			FResidualObjExtension::RegisterMenus();
		}
	}
}

IMPLEMENT_MODULE(LBSImpactSFXSynth::Editor::FImpactSFXSynthEditorModule, ImpactSFXSynthEditor)
#undef LOCTEXT_NAMESPACE