// Copyright 2021 Marcin Swiderski. All Rights Reserved.

#include "TickOptToolkitAnimUpdateRateOptimizationComponentCustomization.h"
#include "TickOptToolkitNames.h"

#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"

TSharedRef<IDetailCustomization> FTickOptToolkitAnimUpdateRateOptComponentCustomization::MakeInstance()
{
	return MakeShareable(new FTickOptToolkitAnimUpdateRateOptComponentCustomization());
}

#define MAKE_ATTRIBUTE(MethodName) MakeAttributeRaw(this, &FTickOptToolkitAnimUpdateRateOptComponentCustomization::MethodName)

void FTickOptToolkitAnimUpdateRateOptComponentCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	CachedBuilder = &DetailBuilder;

	InitComponentsBeingCustomized();
	CachePropertyValues();

	IDetailCategoryBuilder& TickOptToolkitCategory = DetailBuilder.EditCategory(NAME_TickOptToolkit, FText::GetEmpty(), ECategoryPriority::Important);

	TickOptToolkitCategory.AddProperty(NAME_AnimUpdateRateOptimizationsMode);
	TickOptToolkitCategory.AddProperty(NAME_FramesSkippedScreenSizeThresholds)
		.DisplayName(FText::FromString(TEXT("    Frames Skipped Screen Size Thresholds")))
		.Visibility(MAKE_ATTRIBUTE(ShowForScreenSizeMode));
	TickOptToolkitCategory.AddProperty(NAME_LODToFramesSkipped)
		.DisplayName(FText::FromString(TEXT("    LOD to Frames Skipped")))
		.Visibility(MAKE_ATTRIBUTE(ShowForLODMode));

	TickOptToolkitCategory.AddProperty(NAME_NonRenderedFramesSkipped);
	TickOptToolkitCategory.AddProperty(NAME_MaxFramesSkippedForInterpolation);
	TickOptToolkitCategory.AddProperty(NAME_ForceEnableAnimUpdateRateOptimization);
}

#undef MAKE_ATTRIBUTE

void FTickOptToolkitAnimUpdateRateOptComponentCustomization::InitComponentsBeingCustomized()
{
	Components.Reset();

	TArray<TWeakObjectPtr<UObject>> SelectedObjects;
	CachedBuilder->GetObjectsBeingCustomized(SelectedObjects);
	for (const TWeakObjectPtr<UObject>& Selected : SelectedObjects)
	{
		TWeakObjectPtr<UTickOptToolkitAnimUpdateRateOptComponent> Component = Cast<UTickOptToolkitAnimUpdateRateOptComponent>(Selected);
		if (Component.IsValid())
		{
			Components.Add(Component);
		}
	}
}

void FTickOptToolkitAnimUpdateRateOptComponentCustomization::CachePropertyValues()
{
	CachedBuilder->GetProperty(NAME_AnimUpdateRateOptimizationsMode)->SetOnPropertyValueChanged(
		FSimpleDelegate::CreateLambda([this]()
		{
			CachedAnimUROMode = GetAnimUROMode();
		}));
	CachedAnimUROMode = GetAnimUROMode();
}

template<typename ValueType>
TOptional<ValueType> FTickOptToolkitAnimUpdateRateOptComponentCustomization::GetValueFromComponents(ValueType (UTickOptToolkitAnimUpdateRateOptComponent::*GetValue)() const) const
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

TOptional<ETickOptToolkitAnimUROMode> FTickOptToolkitAnimUpdateRateOptComponentCustomization::GetAnimUROMode() const
{
	return GetValueFromComponents(&UTickOptToolkitAnimUpdateRateOptComponent::GetAnimUpdateRateOptimizationsMode);
}

EVisibility FTickOptToolkitAnimUpdateRateOptComponentCustomization::ShowForScreenSizeMode() const
{
	if (CachedAnimUROMode == ETickOptToolkitAnimUROMode::ScreenSize)
	{
		return EVisibility::Visible;
	}
	return EVisibility::Hidden;
}

EVisibility FTickOptToolkitAnimUpdateRateOptComponentCustomization::ShowForLODMode() const
{
	if (CachedAnimUROMode == ETickOptToolkitAnimUROMode::LOD || CachedAnimUROMode == ETickOptToolkitAnimUROMode::MinLOD)
	{
		return EVisibility::Visible;
	}
	return EVisibility::Hidden;
}
