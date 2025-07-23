// Copyright 2021 Marcin Swiderski. All Rights Reserved.

#include "TickOptToolkitOptimizationLevelCustomization.h"
#include "TickOptToolkitNames.h"

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailGroup.h"
#include "IPropertyUtilities.h"
#include "SNameComboBox.h"
#include "DeviceProfiles/DeviceProfile.h"
#include "Widgets/Input/SNumericEntryBox.h"

TSharedRef<IPropertyTypeCustomization> FTickOptToolkitOptimizationLevelCustomization::MakeInstance()
{
	return MakeShareable(new FTickOptToolkitOptimizationLevelCustomization());
}

#define MAKE_ATTRIBUTE(MethodName) MakeAttributeRaw(this, &FTickOptToolkitOptimizationLevelCustomization::MethodName)

void FTickOptToolkitOptimizationLevelCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	CachedProperty = PropertyHandle;
	CachedComponentProperty = PropertyHandle->GetParentHandle()->GetParentHandle();
	CachedPropertyUtilities = CustomizationUtils.GetPropertyUtilities();
	
	HeaderRow
	.NameContent() [ PropertyHandle->CreatePropertyNameWidget() ]
	.ValueContent()
	[
		SNew(STextBlock)
		.Text(this, &FTickOptToolkitOptimizationLevelCustomization::GetOptimizationLevelText)
	];
}

void FTickOptToolkitOptimizationLevelCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	CachedBuilder = &ChildBuilder;
	
	InitComponentsBeingCustomized();
	
	TickSettingsProperty = CachedProperty->GetChildHandle(NAME_TickSettings)->AsArray();
	
	CachePropertyValues();

	// Sphere
	ChildBuilder.AddProperty(CachedProperty->GetChildHandle(NAME_SphereRadius).ToSharedRef())
		.Visibility(MAKE_ATTRIBUTE(ShowForSphere));
	// Box
	ChildBuilder.AddProperty(CachedProperty->GetChildHandle(NAME_BoxExtents).ToSharedRef())
		.Visibility(MAKE_ATTRIBUTE(ShowForBox));
	// Distance Mode != None
	ChildBuilder.AddProperty(CachedProperty->GetChildHandle(NAME_BufferSize).ToSharedRef())
		.IsEnabled(MAKE_ATTRIBUTE(EnableForDistanceMode));
	ChildBuilder.AddProperty(CachedProperty->GetChildHandle(NAME_MidZoneSizes).ToSharedRef())
		.IsEnabled(MAKE_ATTRIBUTE(EnableForDistanceMode));
	
	// Tick Settings
	CreateTickSettingsDetails(ChildBuilder);
}

void FTickOptToolkitOptimizationLevelCustomization::CreateTickSettingsDetails(IDetailChildrenBuilder& ChildBuilder)
{
	IDetailGroup& TickSettingsGroup = ChildBuilder.AddGroup(NAME_TickSettings, FText::FromString(TEXT("Tick Settings")));

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
				.ToolTipText(CachedProperty->GetChildHandle(NAME_TickSettings)->GetToolTipText())
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
					.OnCheckStateChanged(this, &FTickOptToolkitOptimizationLevelCustomization::OnZoneTickEnabledChanged, Zone, true)
					.IsChecked(this, &FTickOptToolkitOptimizationLevelCustomization::IsZoneTickEnabledChecked, Zone, true)
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
					.OnValueChanged(this, &FTickOptToolkitOptimizationLevelCustomization::OnZoneTickIntervalChanged, Zone, true)
					.OnValueCommitted(this, &FTickOptToolkitOptimizationLevelCustomization::OnZoneTickIntervalCommitted, Zone, true)
					.Value(this, &FTickOptToolkitOptimizationLevelCustomization::GetZoneTickInterval, Zone, true)
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
					.OnCheckStateChanged(this, &FTickOptToolkitOptimizationLevelCustomization::OnZoneTickEnabledChanged, Zone, false)
					.IsChecked(this, &FTickOptToolkitOptimizationLevelCustomization::IsZoneTickEnabledChecked, Zone, false)
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
					.OnValueChanged(this, &FTickOptToolkitOptimizationLevelCustomization::OnZoneTickIntervalChanged, Zone, false)
					.OnValueCommitted(this, &FTickOptToolkitOptimizationLevelCustomization::OnZoneTickIntervalCommitted, Zone, false)
					.Value(this, &FTickOptToolkitOptimizationLevelCustomization::GetZoneTickInterval, Zone, false)
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
				.ToolTipText(CachedProperty->GetChildHandle(NAME_TickSettings)->GetToolTipText())
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

void FTickOptToolkitOptimizationLevelCustomization::InitComponentsBeingCustomized()
{
	Components.Reset();
	
	TArray<UObject*> CustomizedObjects;
	CachedProperty->GetOuterObjects(CustomizedObjects);
	for (UObject* Customized : CustomizedObjects)
	{
		TWeakObjectPtr<UTickOptToolkitTargetComponent> Component = Cast<UTickOptToolkitTargetComponent>(Customized);
		if (Component.IsValid())
		{
			Components.Add(Component);
		}
	}
}

void FTickOptToolkitOptimizationLevelCustomization::CachePropertyValues()
{
	const FSimpleDelegate OnZonesNumChanged = FSimpleDelegate::CreateLambda([this]()
	{
		const TOptional<uint32> OldZonesNum = CachedZonesNum;
		CachedZonesNum = GetZonesNum();
		if (CachedZonesNum != OldZonesNum)
		{
			CachedPropertyUtilities->ForceRefresh();
		}
	});
	
	CachedComponentProperty->GetChildHandle(NAME_MidZoneSizes)->SetOnPropertyValueChanged(OnZonesNumChanged);
	CachedComponentProperty->GetChildHandle(NAME_DistanceMode)->SetOnPropertyValueChanged(OnZonesNumChanged);
	CachedComponentProperty->GetChildHandle(NAME_VisibilityMode)->SetOnPropertyValueChanged(OnZonesNumChanged);
	CachedZonesNum = GetZonesNum();

	CachedComponentProperty->GetChildHandle(NAME_DistanceMode)->SetOnPropertyValueChanged(
		FSimpleDelegate::CreateLambda([this]()
		{
			CachedDistanceMode = GetDistanceMode();
		}));
	CachedDistanceMode = GetDistanceMode();
	
	CachedComponentProperty->GetChildHandle(NAME_VisibilityMode)->SetOnPropertyValueChanged(
		FSimpleDelegate::CreateLambda([this]()
		{
			CachedVisibilityMode = GetVisibilityMode();
		}));
	CachedVisibilityMode = GetVisibilityMode();
}

FText FTickOptToolkitOptimizationLevelCustomization::GetOptimizationLevelText() const
{
	const int32 OptimizationLevel = CachedProperty->GetIndexInArray() + 1;
	return FText::FromString(FString::Printf(TEXT("Opt Level %d"), OptimizationLevel));
}

TOptional<uint32> FTickOptToolkitOptimizationLevelCustomization::GetTickSettingsNum() const
{
	uint32 TickSettingsNum;
	if (TickSettingsProperty->GetNumElements(TickSettingsNum) == FPropertyAccess::Success)
	{
		return TickSettingsNum;
	}
	return {};
}

void FTickOptToolkitOptimizationLevelCustomization::OnZoneTickEnabledChanged(ECheckBoxState NewValue, uint32 Zone, bool bVisible) const
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

ECheckBoxState FTickOptToolkitOptimizationLevelCustomization::IsZoneTickEnabledChecked(uint32 Zone, bool bVisible) const
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

void FTickOptToolkitOptimizationLevelCustomization::OnZoneTickIntervalChanged(float Interval, uint32 Zone, bool bVisible)
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

void FTickOptToolkitOptimizationLevelCustomization::OnZoneTickIntervalCommitted(float Interval, ETextCommit::Type, uint32 Zone, bool bVisible)
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

TOptional<float> FTickOptToolkitOptimizationLevelCustomization::GetZoneTickInterval(uint32 Zone, bool bVisible) const
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
TOptional<ValueType> FTickOptToolkitOptimizationLevelCustomization::GetValueFromComponents(ValueType (UTickOptToolkitTargetComponent::*GetValue)() const) const
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

template<typename ValueType>
TOptional<ValueType> GetChildValue(const TSharedPtr<IPropertyHandle> PropertyHandle, const FName& ChildName)
{
	if (PropertyHandle.IsValid())
	{
		const TSharedPtr<IPropertyHandle> ChildHandle = PropertyHandle->GetChildHandle(ChildName);
		if (ChildHandle.IsValid())
		{
			ValueType Value;
			if (ChildHandle->GetValue(Value) == FPropertyAccess::Success)
			{
				return Value;
			}
		}
	}
	return {};
}

TOptional<uint32> GetChildNumElements(const TSharedPtr<IPropertyHandle> PropertyHandle, const FName& ChildName)
{
	if (PropertyHandle.IsValid())
	{
		const TSharedPtr<IPropertyHandle> ChildHandle = PropertyHandle->GetChildHandle(ChildName);
		if (ChildHandle.IsValid())
		{
			const TSharedPtr<IPropertyHandleArray> ChildArrayHandle = ChildHandle->AsArray();
			if (ChildArrayHandle.IsValid())
			{
				uint32 NumElements;
				if (ChildArrayHandle->GetNumElements(NumElements) == FPropertyAccess::Success)
				{
					return NumElements;
				}
			}
		}
	}
	return {};
}

TOptional<uint32> FTickOptToolkitOptimizationLevelCustomization::GetZonesNum() const
{
	return GetValueFromComponents(&UTickOptToolkitTargetComponent::GetZonesNum);
}

TOptional<ETickOptToolkitDistanceMode> FTickOptToolkitOptimizationLevelCustomization::GetDistanceMode() const
{
	return GetValueFromComponents(&UTickOptToolkitTargetComponent::GetDistanceMode);
}

TOptional<ETickByVisibilityMode> FTickOptToolkitOptimizationLevelCustomization::GetVisibilityMode() const
{
	return GetValueFromComponents(&UTickOptToolkitTargetComponent::GetVisibilityMode);
}

bool FTickOptToolkitOptimizationLevelCustomization::EnableForDistanceMode() const
{
	return !(CachedDistanceMode == ETickOptToolkitDistanceMode::None);
}

bool FTickOptToolkitOptimizationLevelCustomization::EnableForVisibilityMode() const
{
	return !(CachedVisibilityMode == ETickByVisibilityMode::None);
}

EVisibility FTickOptToolkitOptimizationLevelCustomization::ShowForSphere() const
{
	if (CachedDistanceMode == ETickOptToolkitDistanceMode::Sphere)
	{
		return EVisibility::Visible;
	}
	return EVisibility::Hidden;
}

EVisibility FTickOptToolkitOptimizationLevelCustomization::ShowForBox() const
{
	if (CachedDistanceMode == ETickOptToolkitDistanceMode::Box)
	{
		return EVisibility::Visible;
	}
	return EVisibility::Hidden;
}
