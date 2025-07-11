// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWidget.h"
#include "UnrealClient.h"
#include "IPropertyTypeCustomization.h"

class FWorldstateSetValueContainerDetails : public IPropertyTypeCustomization
{ 
public:
	// Makes a new instance of this detail layout class for a specific detail view requesting it
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	FWorldstateSetValueContainerDetails();

	// IPropertyTypeCustomization interface
	virtual void CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils ) override;
	virtual void CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils ) override;

private:
	void CacheBlackboardData();
	TSharedPtr<IPropertyHandle> FindBlackboardKeySelectorProperty() const;
	class UBlackboardData* FindBlackboardAsset(UObject* InObj) const;
	void OnKeyIDChanged();
	class UBlackboardKeyType* GetBlackboardKeyType() const;
	EVisibility GetPropertyEditorVisibility(TArray<TSubclassOf<UBlackboardKeyType>, TInlineAllocator<2>> BlackboardKeyTypes) const;
	bool OnShouldFilterObjectAsset(const FAssetData& AssetData) const;
	void OnSetClassValue(const UClass* NewClass) const;

	bool IsEditingEnabled() const;

	TSharedRef<SWidget> OnGetEnumValueContent() const;
	FText GetCurrentEnumValueDesc() const;

	TWeakObjectPtr<class UBlackboardData> CachedBlackboardAsset;
	
	TSharedPtr<IPropertyHandle> MyStructProperty;
	TSharedPtr<IPropertyHandle> CachedBlackboardKeyIDProperty;
	TSharedPtr<IPropertyHandle> CachedIntPropertyHandle;
	TSharedPtr<IPropertyHandle> CachedObjectPropertyHandle;

	TWeakObjectPtr<UEnum> CachedEnumType;
	TArray<FString> CachedEnumNames;
	TWeakObjectPtr<UClass> CachedBaseClass;

	class IPropertyUtilities* PropUtils;
};
