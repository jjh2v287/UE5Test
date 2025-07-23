// Copyright 2021 Marcin Swiderski. All Rights Reserved.

#include "TickOptToolkitComponentCustomization.h"
#include "TickOptToolkitNames.h"

#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailGroup.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

TSharedRef<IDetailCustomization> FTickOptToolkitComponentCustomization::MakeInstance()
{
	return MakeShareable(new FTickOptToolkitComponentCustomization());
}

#define MAKE_ATTRIBUTE(MethodName) MakeAttributeRaw(this, &FTickOptToolkitComponentCustomization::MethodName)

void FTickOptToolkitComponentCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	CachedBuilder = &DetailBuilder;
	
	InitComponentsBeingCustomized();
	TickSettingsProperty = DetailBuilder.GetProperty(NAME_TickSettings)->AsArray();
	
	CachePropertyValues();
	
	IDetailCategoryBuilder& TickOptToolkitCategory = DetailBuilder.EditCategory(NAME_TickOptToolkit, FText::GetEmpty(), ECategoryPriority::Important);
	
	// Distance Mode
	TickOptToolkitCategory.AddProperty(NAME_DistanceMode);
	// Sphere
	TickOptToolkitCategory.AddProperty(NAME_SphereRadius)
		.DisplayName(FText::FromString(TEXT("    Radius")))
		.Visibility(MAKE_ATTRIBUTE(ShowForSphere));
	// Box
	TickOptToolkitCategory.AddProperty(NAME_BoxExtents)
		.DisplayName(FText::FromString(TEXT("    Extents")))
		.Visibility(MAKE_ATTRIBUTE(ShowForBox));
	TickOptToolkitCategory.AddProperty(NAME_BoxRotation)
		.DisplayName(FText::FromString(TEXT("    Rotation")))
		.Visibility(MAKE_ATTRIBUTE(ShowForBox));
	// Distance Mode != None
	TickOptToolkitCategory.AddProperty(NAME_BufferSize)
		.IsEnabled(MAKE_ATTRIBUTE(EnableForDistanceMode));
	TickOptToolkitCategory.AddProperty(NAME_MidZoneSizes)
		.IsEnabled(MAKE_ATTRIBUTE(EnableForDistanceMode));
	
	// Visibility Mode
	TickOptToolkitCategory.AddProperty(NAME_VisibilityMode);

	// Tick Control
	TickOptToolkitCategory.AddProperty(NAME_ActorTickControl)
		.IsEnabled(MAKE_ATTRIBUTE(EnableForOwnerStartsWithTickEnabled));
	TickOptToolkitCategory.AddProperty(NAME_ComponentsTickControl);
	TickOptToolkitCategory.AddProperty(NAME_TimelinesTickControl);
	TickOptToolkitCategory.AddProperty(NAME_SyncTimelinesToWorld)
		.IsEnabled(MAKE_ATTRIBUTE(EnableForTimelinesTickControl));
	TickOptToolkitCategory.AddProperty(NAME_ForceExecuteFirstTick);

	// Tick Settings
	TickOptToolkitCategory.AddProperty(NAME_TickSettings)
		.Visibility(EVisibility::Hidden);
	CreateTickSettingsDetails(TickOptToolkitCategory);
}

void FTickOptToolkitComponentCustomization::CreateTickSettingsDetails(IDetailCategoryBuilder& TickOptToolkitCategory)
{
	IDetailGroup& TickSettingsGroup = TickOptToolkitCategory.AddGroup(NAME_TickSettings, FText::FromString(TEXT("Tick Settings")), false, true);

	if (CachedZonesNum.IsSet())
	{
		TickSettingsGroup.HeaderRow()
		.NameContent()
		[
			SNew(SBox)
			.Padding(FMargin(0.0f, 4.0f))
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Tick Settings")))
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.ToolTipText(CachedBuilder->GetProperty(NAME_TickSettings)->GetToolTipText())
			]
		]
		.ValueContent()
		[
			SNew(SVerticalBox)

			+SVerticalBox::Slot()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.Padding(0.0f, 1.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Enabled / Interval")))
				.Font(IDetailLayoutBuilder::GetDetailFontBold())
			]

			+SVerticalBox::Slot()
			.Padding(0.0f, 1.0f)
			[
				SNew(SHorizontalBox)

				+SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Visible")))
					.Font(IDetailLayoutBuilder::GetDetailFontBold())
				]
				
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Hidden")))
					.Font(IDetailLayoutBuilder::GetDetailFontBold())
				]
			]
		];
		
		const uint32 ZonesNum = CachedZonesNum.GetValue();
		for (uint32 Zone = 0; Zone < ZonesNum; ++Zone)
		{
			FString RowName;
			if (ZonesNum > 1)
			{
				const TCHAR* Suffix = Zone == 0 ? TEXT(" (Inner)")
					: Zone == ZonesNum - 1 ? TEXT(" (Outer)")
					: TEXT("");
				RowName = FString::Printf(TEXT("Zone %d%s"), Zone, Suffix);
			}
			else
			{
				RowName = TEXT("Visibility");
			}
		
			TickSettingsGroup.AddWidgetRow()
			.NameContent()
			[
				SNew(STextBlock)
				.Text(FText::FromString(RowName))
				.Font(IDetailLayoutBuilder::GetDetailFont())
			]
			.ValueContent()
			[
				SNew(SHorizontalBox)
				
				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SCheckBox)
					.OnCheckStateChanged(this, &FTickOptToolkitComponentCustomization::OnZoneTickEnabledChanged, Zone, true)
					.IsChecked(this, &FTickOptToolkitComponentCustomization::IsZoneTickEnabledChecked, Zone, true)
					.ToolTipText(FText::FromString(TEXT("Tick enabled when visible")))
				]

				+SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SNew(SNumericEntryBox<float>)
					.AllowSpin(true)
					.MinSliderValue(0.0f)
					.MaxSliderValue(1.0f)
					.OnValueChanged(this, &FTickOptToolkitComponentCustomization::OnZoneTickIntervalChanged, Zone, true)
					.OnValueCommitted(this, &FTickOptToolkitComponentCustomization::OnZoneTickIntervalCommitted, Zone, true)
					.Value(this, &FTickOptToolkitComponentCustomization::GetZoneTickInterval, Zone, true)
					.ToolTipText(FText::FromString(TEXT("Tick interval when visible")))
				]

				+SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(1.0f, 0.0)
				
				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SCheckBox)
					.OnCheckStateChanged(this, &FTickOptToolkitComponentCustomization::OnZoneTickEnabledChanged, Zone, false)
					.IsChecked(this, &FTickOptToolkitComponentCustomization::IsZoneTickEnabledChecked, Zone, false)
					.IsEnabled(MAKE_ATTRIBUTE(EnableForVisibilityMode))
					.ToolTipText(FText::FromString(TEXT("Tick enabled when hidden")))
				]

				+SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SNew(SNumericEntryBox<float>)
					.AllowSpin(true)
					.MinSliderValue(0.0f)
					.MaxSliderValue(1.0f)
					.OnValueChanged(this, &FTickOptToolkitComponentCustomization::OnZoneTickIntervalChanged, Zone, false)
					.OnValueCommitted(this, &FTickOptToolkitComponentCustomization::OnZoneTickIntervalCommitted, Zone, false)
					.Value(this, &FTickOptToolkitComponentCustomization::GetZoneTickInterval, Zone, false)
					.IsEnabled(MAKE_ATTRIBUTE(EnableForVisibilityMode))
					.ToolTipText(FText::FromString(TEXT("Tick interval when hidden")))
				]
			];
		}
	}
	else
	{
		TickSettingsGroup.HeaderRow()
		.NameContent()
		[
			SNew(SBox)
			.Padding(FMargin(0.0f, 4.0f))
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Tick Settings")))
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.ToolTipText(CachedBuilder->GetProperty(NAME_TickSettings)->GetToolTipText())
			]
		]
		.ValueContent()
		[
			SNew(SBox)
			.Padding(4.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Multiple Values")))
				.Font(IDetailLayoutBuilder::GetDetailFont())
			]
		];
	}
}

#undef MAKE_ATTRIBUTE

void FTickOptToolkitComponentCustomization::InitComponentsBeingCustomized()
{
	Components.Reset();
	
	TArray<TWeakObjectPtr<UObject>> CustomizedObjects;
	CachedBuilder->GetObjectsBeingCustomized(CustomizedObjects);
	for (const TWeakObjectPtr<UObject>& Customized : CustomizedObjects)
	{
		TWeakObjectPtr<UTickOptToolkitTargetComponent> Component = Cast<UTickOptToolkitTargetComponent>(Customized);
		if (Component.IsValid())
		{
			Components.Add(Component);
		}
	}
}

void FTickOptToolkitComponentCustomization::CachePropertyValues()
{
	const FSimpleDelegate OnZonesNumChanged = FSimpleDelegate::CreateLambda([this]()
	{
		const TOptional<uint32> OldZonesNum = CachedZonesNum;
		CachedZonesNum = GetZonesNum();
		if (CachedZonesNum != OldZonesNum)
		{
			CachedBuilder->ForceRefreshDetails();
		}
	});
	
	CachedBuilder->GetProperty(NAME_MidZoneSizes)->SetOnPropertyValueChanged(OnZonesNumChanged);
	CachedBuilder->GetProperty(NAME_DistanceMode)->SetOnPropertyValueChanged(OnZonesNumChanged);
	CachedBuilder->GetProperty(NAME_VisibilityMode)->SetOnPropertyValueChanged(OnZonesNumChanged);
	CachedZonesNum = GetZonesNum();
	
	CachedBuilder->GetProperty(NAME_DistanceMode)->SetOnPropertyValueChanged(
		FSimpleDelegate::CreateLambda([this]()
		{
			CachedDistanceMode = GetDistanceMode();
		}));
	CachedDistanceMode = GetDistanceMode();
	
	CachedBuilder->GetProperty(NAME_VisibilityMode)->SetOnPropertyValueChanged(
		FSimpleDelegate::CreateLambda([this]()
		{
			CachedVisibilityMode = GetVisibilityMode();
		}));
	CachedVisibilityMode = GetVisibilityMode();
	
	CachedBuilder->GetProperty(NAME_ActorTickControl)->SetOnPropertyValueChanged(
		FSimpleDelegate::CreateLambda([this]()
		{
			bCachedActorTickControl = GetActorTickControl();
		}));
	bCachedActorTickControl = GetActorTickControl();

	CachedBuilder->GetProperty(NAME_ComponentsTickControl)->SetOnPropertyValueChanged(
		FSimpleDelegate::CreateLambda([this]()
		{
			bCachedComponentsTickControl = GetComponentsTickControl();
		}));
	bCachedComponentsTickControl = GetComponentsTickControl();
	
	CachedBuilder->GetProperty(NAME_TimelinesTickControl)->SetOnPropertyValueChanged(
		FSimpleDelegate::CreateLambda([this]()
		{
			bCachedTimelinesTickControl = GetTimelinesTickControl();
		}));
	bCachedTimelinesTickControl = GetTimelinesTickControl();

	bCachedOwnerStartsWithTickEnabled = DoesOwnerStartWithTickEnabled();
}

TOptional<uint32> FTickOptToolkitComponentCustomization::GetTickSettingsNum() const
{
	uint32 TickSettingsNum;
	if (TickSettingsProperty->GetNumElements(TickSettingsNum) == FPropertyAccess::Success)
	{
		return TickSettingsNum;
	}
	return {};
}

void FTickOptToolkitComponentCustomization::OnZoneTickEnabledChanged(ECheckBoxState NewValue, uint32 Zone, bool bVisible) const
{
	TOptional<uint32> TickSettingsNum = GetTickSettingsNum();
	if (TickSettingsNum.IsSet() && Zone < TickSettingsNum.GetValue())
	{
		if (NewValue != ECheckBoxState::Undetermined)
		{
			const TSharedRef<IPropertyHandle> ZoneProperty = TickSettingsProperty->GetElement(Zone);
			const TSharedPtr<IPropertyHandle> EnabledProperty = ZoneProperty->GetChildHandle(bVisible ? NAME_EnabledVisible : NAME_EnabledHidden);
			EnabledProperty->SetValue(NewValue == ECheckBoxState::Checked);
		}
	}
}

ECheckBoxState FTickOptToolkitComponentCustomization::IsZoneTickEnabledChecked(uint32 Zone, bool bVisible) const
{
	TOptional<uint32> TickSettingsNum = GetTickSettingsNum();
	if (TickSettingsNum.IsSet() && Zone < TickSettingsNum.GetValue())
	{
		const TSharedRef<IPropertyHandle> ZoneProperty = TickSettingsProperty->GetElement(Zone);
		const TSharedPtr<IPropertyHandle> EnabledProperty = ZoneProperty->GetChildHandle(bVisible ? NAME_EnabledVisible : NAME_EnabledHidden);

		bool Value;
		if (EnabledProperty->GetValue(Value) == FPropertyAccess::Success)
		{
			return Value ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
		}
	}
	return ECheckBoxState::Undetermined;
}

void FTickOptToolkitComponentCustomization::OnZoneTickIntervalChanged(float Interval, uint32 Zone, bool bVisible)
{
	if (FTickIntervalsChange* Change = ChangedTickIntervals.FindByPredicate([Zone, bVisible](const FTickIntervalsChange& Elem) { return Elem.Zone == Zone && Elem.bVisible == bVisible; }))
	{
		Change->Interval = Interval;
	}
	else
	{
		ChangedTickIntervals.Add({ Zone, bVisible, Interval });
	}
}

void FTickOptToolkitComponentCustomization::OnZoneTickIntervalCommitted(float Interval, ETextCommit::Type, uint32 Zone, bool bVisible)
{
	TOptional<uint32> TickSettingsNum = GetTickSettingsNum();
	if (TickSettingsNum.IsSet() && Zone < TickSettingsNum.GetValue())
	{
		const TSharedRef<IPropertyHandle> ZoneProperty = TickSettingsProperty->GetElement(Zone);
		const TSharedPtr<IPropertyHandle> IntervalProperty = ZoneProperty->GetChildHandle(bVisible ? NAME_IntervalVisible : NAME_IntervalHidden);
		IntervalProperty->SetValue(Interval);
	}

	ChangedTickIntervals.RemoveAll([Zone, bVisible](const FTickIntervalsChange& Elem) { return Elem.Zone == Zone && Elem.bVisible == bVisible; });
}

TOptional<float> FTickOptToolkitComponentCustomization::GetZoneTickInterval(uint32 Zone, bool bVisible) const
{
	if (const FTickIntervalsChange* Change = ChangedTickIntervals.FindByPredicate([Zone, bVisible](const FTickIntervalsChange& Elem) { return Elem.Zone == Zone && Elem.bVisible == bVisible; }))
	{
		return Change->Interval;
	}

	TOptional<uint32> TickSettingsNum = GetTickSettingsNum();
	if (TickSettingsNum.IsSet() && Zone < TickSettingsNum.GetValue())
	{
		const TSharedRef<IPropertyHandle> ZoneProperty = TickSettingsProperty->GetElement(Zone);
		const TSharedPtr<IPropertyHandle> IntervalProperty = ZoneProperty->GetChildHandle(bVisible ? NAME_IntervalVisible : NAME_IntervalHidden);

		float Interval;
		if (IntervalProperty->GetValue(Interval) == FPropertyAccess::Success)
		{
			return Interval;
		}
	}
	return {};
}

template<typename ValueType>
TOptional<ValueType> FTickOptToolkitComponentCustomization::GetValueFromComponents(ValueType (UTickOptToolkitTargetComponent::*GetValue)() const) const
{
	TOptional<ValueType> Value;
	for (const TWeakObjectPtr<UTickOptToolkitTargetComponent>& Component : Components)
	{
		if (Component.IsValid())
		{
			if (!Value.IsSet())
			{
				Value = (Component.Get()->*GetValue)();
			}
			else if (Value != (Component.Get()->*GetValue)())
			{
				Value.Reset();
				break;
			}
		}
	}
	return Value;
}

TOptional<uint32> FTickOptToolkitComponentCustomization::GetZonesNum() const
{
	return GetValueFromComponents(&UTickOptToolkitTargetComponent::GetZonesNum);
}

TOptional<ETickOptToolkitDistanceMode> FTickOptToolkitComponentCustomization::GetDistanceMode() const
{
	return GetValueFromComponents(&UTickOptToolkitTargetComponent::GetDistanceMode);
}

TOptional<ETickByVisibilityMode> FTickOptToolkitComponentCustomization::GetVisibilityMode() const
{
	return GetValueFromComponents(&UTickOptToolkitTargetComponent::GetVisibilityMode);
}

TOptional<bool> FTickOptToolkitComponentCustomization::GetActorTickControl() const
{
	return GetValueFromComponents(&UTickOptToolkitTargetComponent::IsActorTickControl);
}

TOptional<bool> FTickOptToolkitComponentCustomization::GetComponentsTickControl() const
{
	return GetValueFromComponents(&UTickOptToolkitTargetComponent::IsComponentsTickControl);
}

TOptional<bool> FTickOptToolkitComponentCustomization::GetTimelinesTickControl() const
{
	return GetValueFromComponents(&UTickOptToolkitTargetComponent::IsTimelinesTickControl);
}

TOptional<bool> FTickOptToolkitComponentCustomization::DoesOwnerStartWithTickEnabled() const
{
	return GetValueFromComponents(&UTickOptToolkitTargetComponent::DoesOwnerStartWithTickEnabled);
}

bool FTickOptToolkitComponentCustomization::EnableForDistanceMode() const
{
	return !(CachedDistanceMode == ETickOptToolkitDistanceMode::None);
}

bool FTickOptToolkitComponentCustomization::EnableForVisibilityMode() const
{
	return !(CachedVisibilityMode == ETickByVisibilityMode::None);
}

EVisibility FTickOptToolkitComponentCustomization::ShowForSphere() const
{
	if (CachedDistanceMode == ETickOptToolkitDistanceMode::Sphere)
	{
		return EVisibility::Visible;
	}
	return EVisibility::Hidden;
}

EVisibility FTickOptToolkitComponentCustomization::ShowForBox() const
{
	if (CachedDistanceMode == ETickOptToolkitDistanceMode::Box)
	{
		return EVisibility::Visible;
	}
	return EVisibility::Hidden;
}

bool FTickOptToolkitComponentCustomization::EnableForOwnerStartsWithTickEnabled() const
{
	return bCachedOwnerStartsWithTickEnabled == true;
}

bool FTickOptToolkitComponentCustomization::EnableForTimelinesTickControl() const
{
	return bCachedTimelinesTickControl == true;
}
