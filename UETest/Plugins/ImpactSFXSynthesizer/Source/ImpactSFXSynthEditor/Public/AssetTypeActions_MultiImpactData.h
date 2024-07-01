// Copyright 2023-2024, Le Binh Son, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class UMultiImpactData;

namespace LBSImpactSFXSynth
{
	namespace Editor
	{
		class IMPACTSFXSYNTHEDITOR_API FAssetTypeActions_MultiImpactData : public FAssetTypeActions_Base
		{
		public:
			virtual UClass* GetSupportedClass() const override;
			virtual FText GetName() const override{ return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_MultiImpactData", "Multi Impact Data"); }
			virtual FColor GetTypeColor() const override;
			virtual uint32 GetCategories() override;
			
			virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> ToolkitHost) override;

			virtual TSharedPtr<SWidget> GetThumbnailOverlay(const FAssetData& InAssetData) const override;
			
			static void PlaySound(UMultiImpactData* MultiImpactData);
		};
	}
}