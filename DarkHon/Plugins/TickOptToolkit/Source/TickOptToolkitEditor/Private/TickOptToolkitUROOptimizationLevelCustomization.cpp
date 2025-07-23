// Copyright 2021 Marcin Swiderski. All Rights Reserved.

#include "TickOptToolkitUROOptimizationLevelCustomization.h"
#include "TickOptToolkitNames.h"

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"

TSharedRef<IPropertyTypeCustomization> FTickOptToolkitUROOptimizationLevelCustomization::MakeInstance()
{
	return MakeShareable(new FTickOptToolkitUROOptimizationLevelCustomization());
}

#define MAKE_ATTRIBUTE(MethodName) MakeAttributeRaw(this, &FTickOptToolkitUROOptimizationLevelCustomization::MethodName)

void FTickOptToolkitUROOptimizationLevelCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	CachedProperty = PropertyHandle;
	CachedComponentProperty = PropertyHandle->GetParentHandle()->GetParentHandle();
	CachedPropertyUtilities = CustomizationUtils.GetPropertyUtilities();
	
	HeaderRow
	.NameContent() [ PropertyHandle->CreatePropertyNameWidget() ]
	.ValueContent()
	[
		SNew(STextBlock)
		.Text(this, &FTickOptToolkitUROOptimizationLevelCustomization::GetOptimizationLevelText)
	];
}

void FTickOptToolkitUROOptimizationLevelCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	CachedBuilder = &ChildBuilder;
	
	InitComponentsBeingCustomized();
	CachePropertyValues();

	ChildBuilder.AddProperty(CachedProperty->GetChildHandle(NAME_FramesSkippedScreenSizeThresholds).ToSharedRef())
		.Visibility(MAKE_ATTRIBUTE(ShowForScreenSizeMode));
	ChildBuilder.AddProperty(CachedProperty->GetChildHandle(NAME_LODToFramesSkipped).ToSharedRef())
		.Visibility(MAKE_ATTRIBUTE(ShowForLODMode));

	ChildBuilder.AddProperty(CachedProperty->GetChildHandle(NAME_NonRenderedFramesSkipped).ToSharedRef());
	ChildBuilder.AddProperty(CachedProperty->GetChildHandle(NAME_MaxFramesSkippedForInterpolation).ToSharedRef());
}

#undef MAKE_ATTRIBUTE

void FTickOptToolkitUROOptimizationLevelCustomization::InitComponentsBeingCustomized()
{
	Components.Reset();

	TArray<UObject*> CustomizedObjects;
	CachedProperty->GetOuterObjects(CustomizedObjects);
	for (UObject* Customized : CustomizedObjects)
	{
		TWeakObjectPtr<UTickOptToolkitAnimUpdateRateOptComponent> Component = Cast<UTickOptToolkitAnimUpdateRateOptComponent>(Customized);
		if (Component.IsValid())
		{
			Components.Add(Component);
		}
	}
}

void FTickOptToolkitUROOptimizationLevelCustomization::CachePropertyValues()
{
	CachedComponentProperty->GetChildHandle(NAME_AnimUpdateRateOptimizationsMode)->SetOnPropertyValueChanged(
		FSimpleDelegate::CreateLambda([this]()
		{
			CachedAnimUROMode = GetAnimUROMode();
		}));
	CachedAnimUROMode = GetAnimUROMode();
}

FText FTickOptToolkitUROOptimizationLevelCustomization::GetOptimizationLevelText() const
{
	const int32 OptimizationLevel = CachedProperty->GetIndexInArray() + 1;
	return FText::FromString(FString::Printf(TEXT("Opt Level %d"), OptimizationLevel));
}

template<typename ValueType>
TOptional<ValueType> FTickOptToolkitUROOptimizationLevelCustomization::GetValueFromComponents(ValueType (UTickOptToolkitAnimUpdateRateOptComponent::*GetValue)() const) const
{
	TOptional<ValueType> Value;
	for (const TWeakObjectPtr<UTickOptToolkitAnimUpdateRateOptComponent>& Component : Components)
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

TOptional<ETickOptToolkitAnimUROMode> FTickOptToolkitUROOptimizationLevelCustomization::GetAnimUROMode() const
{
	return GetValueFromComponents(&UTickOptToolkitAnimUpdateRateOptComponent::GetAnimUpdateRateOptimizationsMode);
}

EVisibility FTickOptToolkitUROOptimizationLevelCustomization::ShowForScreenSizeMode() const
{
	if (CachedAnimUROMode == ETickOptToolkitAnimUROMode::ScreenSize)
	{
		return EVisibility::Visible;
	}
	return EVisibility::Hidden;
}

EVisibility FTickOptToolkitUROOptimizationLevelCustomization::ShowForLODMode() const
{
	if (CachedAnimUROMode == ETickOptToolkitAnimUROMode::LOD || CachedAnimUROMode == ETickOptToolkitAnimUROMode::MinLOD)
	{
		return EVisibility::Visible;
	}
	return EVisibility::Hidden;
}
