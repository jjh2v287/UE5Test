// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "Layout/Visibility.h"
#include "IDetailCustomization.h"

class IDetailLayoutBuilder;
class IPropertyHandle;
class SWidget;
class UBlackboardData;

class FHTNBlackboardDecoratorDetails : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	// IDetailCustomization interface
	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailLayout ) override;

	bool IsEditingEnabled() const;
	
private:
	void CacheBlackboardData(IDetailLayoutBuilder& DetailLayout);

	EVisibility GetIntValueVisibility() const;
	EVisibility GetFloatValueVisibility() const;
	EVisibility GetStringValueVisibility() const;
	EVisibility GetEnumValueVisibility() const;
	EVisibility GetBasicOpVisibility() const;
	EVisibility GetArithmeticOpVisibility() const;
	EVisibility GetTextOpVisibility() const;

	void OnKeyIDChanged();

	void OnEnumValueComboChange(int32 Index);
	TSharedRef<SWidget> OnGetEnumValueContent() const;
	FText GetCurrentEnumValueDesc() const;

	TSharedPtr<IPropertyHandle> IntValueProperty;
	TSharedPtr<IPropertyHandle> KeyIDProperty;
	
	TSubclassOf<class UBlackboardKeyType> CachedKeyType;
	UEnum* CachedCustomObjectType;
	uint8 CachedOperationType;
	
	TArray<FString> EnumPropValues;
	TWeakObjectPtr<UBlackboardData> CachedBlackboardAsset;

	class IPropertyUtilities* PropUtils;
};
