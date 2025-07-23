// Copyright 2021 Marcin Swiderski. All Rights Reserved.

#pragma once

#include "TickOptToolkitAnimUpdateRateOptComponent.h"
#include "IDetailCustomization.h"
#include "Layout/Visibility.h"

class FTickOptToolkitAnimUpdateRateOptComponentCustomization : public IDetailCustomization
{
	IDetailLayoutBuilder* CachedBuilder = nullptr;

	TArray<TWeakObjectPtr<UTickOptToolkitAnimUpdateRateOptComponent>> Components;

	TOptional<ETickOptToolkitAnimUROMode> CachedAnimUROMode;

public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	void InitComponentsBeingCustomized();
	void CachePropertyValues();

	template<typename ValueType>
	TOptional<ValueType> GetValueFromComponents(ValueType (UTickOptToolkitAnimUpdateRateOptComponent::*GetValue)() const) const;
	
	TOptional<ETickOptToolkitAnimUROMode> GetAnimUROMode() const;

	EVisibility ShowForScreenSizeMode() const;
	EVisibility ShowForLODMode() const;
};
