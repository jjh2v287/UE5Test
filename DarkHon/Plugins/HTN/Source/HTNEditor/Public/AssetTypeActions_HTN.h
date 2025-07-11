// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "AssetTypeActions_Base.h"

// Defines how HTN assets show up in the content browser, what to do when we need to edit one, and how to diff them.
class FAssetTypeActions_HTN : public FAssetTypeActions_Base
{
public:
	// Begin IAssetTypeActions interface
	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override;
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor) override;
	// End IAssetTypeActions interface

	virtual void PerformAssetDiff(UObject* Asset1, UObject* Asset2, const FRevisionInfo& OldRevision, const FRevisionInfo& NewRevision) const override;

private:
	void OpenInDefaultDiffTool(class UHTN* OldHTN, class UHTN* NewHTN) const;
};
