// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_ResidualData.h"
#include "AssetTypeActions_ResidualObj.h"
#include "AssetTypeActions_ImpactModalObj.h"
#include "AssetTypeActions_MultiImpactData.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogImpactSFXSynthEditor, Log, All);

namespace LBSImpactSFXSynth
{
    namespace Editor
    {
        class FImpactSFXSynthEditorModule : public IModuleInterface
        {
        public:
            static const FName ImpactSynthCategory;;
            
            virtual void StartupModule() override;
            virtual void ShutdownModule() override;
        private:
            TSharedPtr<FAssetTypeActions_ResidualData> ResidualDataTypeActions;
            TSharedPtr<FAssetTypeActions_ResidualObj> ResidualObjTypeActions;
            TSharedPtr<FAssetTypeActions_ImpactModalObj> ImpactModalObjTypeActions;
            TSharedPtr<FAssetTypeActions_MultiImpactData> MultiImpactDataActions;
        
            void RegisterMenus();
        };
    }
}
