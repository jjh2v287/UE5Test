// Copyright 2021 Marcin Swiderski. All Rights Reserved.

#pragma once

#include "TickOptToolkitTargetComponent.h"
#include "IPropertyTypeCustomization.h"
#include "PropertyHandle.h"

class IDetailCategoryBuilder;
class IDetailPropertyRow;

class FTickOptToolkitOptimizationLevelCustomization : public IPropertyTypeCustomization
{
	IDetailChildrenBuilder* CachedBuilder = nullptr;
	
	TSharedPtr<IPropertyHandle> CachedProperty;
	TSharedPtr<IPropertyHandle> CachedComponentProperty;
	TSharedPtr<IPropertyUtilities> CachedPropertyUtilities;
	
	TArray<TWeakObjectPtr<UTickOptToolkitTargetComponent>> Components;
	TSharedPtr<IPropertyHandleArray> TickSettingsProperty;
	
	TOptional<uint32> CachedZonesNum;
	TOptional<ETickOptToolkitDistanceMode> CachedDistanceMode;
	TOptional<ETickByVisibilityMode> CachedVisibilityMode;
	TOptional<bool> bCachedActorTickControl;
	TOptional<bool> bCachedComponentsTickControl;
	TOptional<bool> bCachedTimelinesTickControl;
	TOptional<bool> bCachedOwnerStartsWithTickEnabled;

	struct FTickIntervalsChange
	{
		uint32 Zone;
		bool bVisible;
		float Interval;
	};
	TArray<FTickIntervalsChange> ChangedTickIntervals;
	
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

private:
	void CreateTickSettingsDetails(IDetailChildrenBuilder& ChildBuilder);
	
	void InitComponentsBeingCustomized();
	void CachePropertyValues();

	FText GetOptimizationLevelText() const;
	
	TOptional<uint32> GetTickSettingsNum() const;
	
	void OnZoneTickEnabledChanged(ECheckBoxState NewValue, uint32 Zone, bool bVisible) const;
	ECheckBoxState IsZoneTickEnabledChecked(uint32 Zone, bool bVisible) const;

	void OnZoneTickIntervalChanged(float Interval, uint32 Zone, bool bVisible);
	void OnZoneTickIntervalCommitted(float Interval, ETextCommit::Type, uint32 Zone, bool bVisible);
	TOptional<float> GetZoneTickInterval(uint32 Zone, bool bVisible) const;
	
	template<typename ValueType>
	TOptional<ValueType> GetValueFromComponents(ValueType (UTickOptToolkitTargetComponent::*GetValue)() const) const;

	TOptional<uint32> GetZonesNum() const;
	TOptional<ETickOptToolkitDistanceMode> GetDistanceMode() const;
	TOptional<ETickByVisibilityMode> GetVisibilityMode() const;
	
	bool EnableForDistanceMode() const;
	bool EnableForVisibilityMode() const;
	
	EVisibility ShowForSphere() const;
	EVisibility ShowForBox() const;
};
