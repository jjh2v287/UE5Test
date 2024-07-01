// Copyright 2023-2024, Le Binh Son, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class UImpactModalObj;

namespace LBSImpactSFXSynth
{
	namespace Editor
	{
		class IMPACTSFXSYNTHEDITOR_API FAssetTypeActions_ImpactModalObj : public FAssetTypeActions_Base
		{
		public:
			virtual UClass* GetSupportedClass() const override;
			virtual FText GetName() const override{ return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_ImpactModalObj", "Impact Modal Obj"); }
			virtual FColor GetTypeColor() const override;
			virtual uint32 GetCategories() override;
			virtual void GetResolvedSourceFilePaths(const TArray<UObject*>& TypeAssets, TArray<FString>& OutSourceFilePaths) const override;
			virtual bool IsImportedAsset() const override { return true; }
			
			virtual TSharedPtr<SWidget> GetThumbnailOverlay(const FAssetData& InAssetData) const override;
			
			static void PlaySound(UImpactModalObj* ImpactModalData);
		};
	}
}