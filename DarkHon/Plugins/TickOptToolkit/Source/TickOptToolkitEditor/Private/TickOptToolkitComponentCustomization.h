// Copyright 2021 Marcin Swiderski. All Rights Reserved.

#pragma once

#include "TickOptToolkitTargetComponent.h"
#include "IDetailCustomization.h"
#include "PropertyHandle.h"
#include "Layout/Visibility.h"

class IDetailCategoryBuilder;
class IDetailPropertyRow;

class FTickOptToolkitComponentCustomization : public IDetailCustomization
{
	IDetailLayoutBuilder* CachedBuilder = nullptr;

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
	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	void CreateTickSettingsDetails(IDetailCategoryBuilder& TickOptToolkitCategory);
	
	void InitComponentsBeingCustomized();
	void CachePropertyValues();

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
	TOptional<bool> GetActorTickControl() const;
	TOptional<bool> GetComponentsTickControl() const;
	TOptional<bool> GetTimelinesTickControl() const;
	TOptional<bool> DoesOwnerStartWithTickEnabled() const;

	bool EnableForDistanceMode() const;
	bool EnableForVisibilityMode() const;
	
	EVisibility ShowForSphere() const;
	EVisibility ShowForBox() const;

	bool EnableForOwnerStartsWithTickEnabled() const;
	bool EnableForTimelinesTickControl() const;
};
