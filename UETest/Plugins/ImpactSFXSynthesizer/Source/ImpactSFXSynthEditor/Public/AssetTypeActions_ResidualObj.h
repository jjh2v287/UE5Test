// Copyright 2023-2024, Le Binh Son, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class UResidualObj;
class UResidualData;

namespace LBSImpactSFXSynth
{
	namespace Editor
	{
		class IMPACTSFXSYNTHEDITOR_API FAssetTypeActions_ResidualObj : public FAssetTypeActions_Base
		{
		public:
			virtual UClass* GetSupportedClass() const override;
			virtual FText GetName() const override{ return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_ResidualObj", "Residual Obj"); }
			virtual FColor GetTypeColor() const override;
			virtual uint32 GetCategories() override;
			virtual void GetResolvedSourceFilePaths(const TArray<UObject*>& TypeAssets, TArray<FString>& OutSourceFilePaths) const override;
			virtual bool IsImportedAsset() const override { return true; }
			
			virtual void GetActions(const TArray<UObject*>& InObjects, struct FToolMenuSection& Section) override;
			
			virtual TSharedPtr<SWidget> GetThumbnailOverlay(const FAssetData& InAssetData) const override;

			static void PlaySound(UResidualObj* ResidualObj);
			
		private:
			void ExecuteCreateResidualData(TArray<TWeakObjectPtr<UResidualObj>> WeakObjs);
			bool CanExecuteCreateResidualDataCommand(TArray<TWeakObjectPtr<UResidualObj>> WeakObjects);
		};

		class IMPACTSFXSYNTHEDITOR_API FResidualObjExtension
		{
		public:
			static void RegisterMenus();
			static void ExecuteCreateResidualObjFromWave(const struct FToolMenuContext& MenuContext);
		};

	}
}
