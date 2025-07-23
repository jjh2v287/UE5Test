// Copyright 2021 Marcin Swiderski. All Rights Reserved.

#pragma once

#include "TickOptToolkitAnimUpdateRateOptComponent.h"
#include "IPropertyTypeCustomization.h"
#include "PropertyHandle.h"

class IDetailCategoryBuilder;
class IDetailPropertyRow;

class FTickOptToolkitUROOptimizationLevelCustomization : public IPropertyTypeCustomization
{
	IDetailChildrenBuilder* CachedBuilder = nullptr;
	
	TSharedPtr<IPropertyHandle> CachedProperty;
	TSharedPtr<IPropertyHandle> CachedComponentProperty;
	TSharedPtr<IPropertyUtilities> CachedPropertyUtilities;
	
	TArray<TWeakObjectPtr<UTickOptToolkitAnimUpdateRateOptComponent>> Components;

	TOptional<ETickOptToolkitAnimUROMode> CachedAnimUROMode;
	
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

private:
	void InitComponentsBeingCustomized();
	void CachePropertyValues();

	FText GetOptimizationLevelText() const;
	
	template<typename ValueType>
	TOptional<ValueType> GetValueFromComponents(ValueType (UTickOptToolkitAnimUpdateRateOptComponent::*GetValue)() const) const;
	
	TOptional<ETickOptToolkitAnimUROMode> GetAnimUROMode() const;

	EVisibility ShowForScreenSizeMode() const;
	EVisibility ShowForLODMode() const;
};
