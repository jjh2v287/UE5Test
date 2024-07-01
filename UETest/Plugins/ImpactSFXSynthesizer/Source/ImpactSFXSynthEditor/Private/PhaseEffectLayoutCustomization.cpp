// Copyright 2023-2024, Le Binh Son, All rights reserved.


#include "PhaseEffectLayoutCustomization.h"

#include "Editor.h"
#include "IDetailChildrenBuilder.h"
#include "ResidualData.h"
#include "PropertyCustomizationHelpers.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SToolTip.h"

#define LOCTEXT_NAMESPACE "ImpactySFXSynthEditor"

namespace LBSImpactSFXSynth
{
	namespace Editor
	{
		void FPhaseEffectLayoutCustomizationBase::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
		{
			HeaderRow
			.NameContent()
			[
				StructPropertyHandle->CreatePropertyNameWidget()
			]
			.ValueContent();
		}

		void FPhaseEffectLayoutCustomizationBase::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
		{
			uint32 NumChildren;
			StructPropertyHandle->GetNumChildren(NumChildren);

			FProperty* Property = nullptr;
			StructPropertyHandle->GetValue(Property);

			TMap<FName, TSharedPtr<IPropertyHandle>> PropertyHandles;

			for (uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex)
			{
				TSharedRef<IPropertyHandle> ChildHandle = StructPropertyHandle->GetChildHandle(ChildIndex).ToSharedRef();
				const FName PropertyName = ChildHandle->GetProperty()->GetFName();
				PropertyHandles.Add(PropertyName, ChildHandle);
			}

			TSharedRef<IPropertyHandle> EffectTypeHandleRef = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FResidualPhaseEffect, EffectType)).ToSharedRef();
			ChildBuilder.AddProperty(EffectTypeHandleRef);
			EffectTypeHandle = EffectTypeHandleRef;
			
			TSharedRef<IPropertyHandle> EffectScaleHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FResidualPhaseEffect, EffectScale)).ToSharedRef();
			ChildBuilder.AddProperty(EffectScaleHandle)
				.EditCondition(TAttribute<bool>::Create([this]()
				{
					return IsScalableEffect(GetPhaseEffectType());
				}), nullptr)
				.Visibility(TAttribute<EVisibility>::Create([this]()
				{
					return IsScalableEffect(GetPhaseEffectType()) ? EVisibility::Visible : EVisibility::Hidden;
				}));
		}

		EResidualPhaseGenerator FPhaseEffectLayoutCustomizationBase::GetPhaseEffectType() const
		{
			if (!EffectTypeHandle.IsValid())
			{
				return EResidualPhaseGenerator::Random;
			}

			uint8 CurveValue = static_cast<uint8>(EResidualPhaseGenerator::Random);
			EffectTypeHandle->GetValue(CurveValue);
			return static_cast<EResidualPhaseGenerator>(CurveValue);
		}

		bool FPhaseEffectLayoutCustomizationBase::IsScalableEffect(const EResidualPhaseGenerator EffectType)
		{
			return EffectType == EResidualPhaseGenerator::IncrementByFreq;
		}
	} 
} 
#undef LOCTEXT_NAMESPACE 
