// Copyright 2023-2024, Le Binh Son, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class UResidualData;

namespace LBSImpactSFXSynth
{
	namespace Editor
	{
		class IMPACTSFXSYNTHEDITOR_API FAssetTypeActions_ResidualData : public FAssetTypeActions_Base
		{
		public:
			virtual UClass* GetSupportedClass() const override;
			virtual FText GetName() const override{ return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_ResidualData", "Residual Data"); }
			virtual FColor GetTypeColor() const override;
			virtual uint32 GetCategories() override;
			virtual void GetActions(const TArray<UObject*>& InObjects, struct FToolMenuSection& Section) override;
			
			virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> ToolkitHost) override;
			virtual bool AssetsActivatedOverride(const TArray<UObject*>& InObjects, EAssetTypeActivationMethod::Type ActivationType) override;

			virtual TSharedPtr<SWidget> GetThumbnailOverlay(const FAssetData& InAssetData) const override;
			
			static void PlaySound(UResidualData* ResidualData);
			static void StopSound();
			
		private:
			bool CanExecutePlayCommand(TArray<TWeakObjectPtr<UResidualData>> InObjects) const;

			void ExecutePlaySound(TArray<TWeakObjectPtr<UResidualData>> WeakObjects);
			void ExecuteStopSound(TArray<TWeakObjectPtr<UResidualData>> WeakObjects);
		};

		class IMPACTSFXSYNTHEDITOR_API FResidualDataExtension
		{
		public:
			static void RegisterMenus();
			static void ExecuteCreateResidualDataFromResidualObj(const struct FToolMenuContext& MenuContext);
		};
	}
}