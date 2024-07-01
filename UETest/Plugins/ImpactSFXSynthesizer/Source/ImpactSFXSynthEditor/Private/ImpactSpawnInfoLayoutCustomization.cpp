// Copyright 2023-2024, Le Binh Son, All rights reserved.


#include "ImpactSpawnInfoLayoutCustomization.h"

#include "Editor.h"
#include "IDetailChildrenBuilder.h"
#include "MultiImpactData.h"
#include "PropertyCustomizationHelpers.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SToolTip.h"

#define LOCTEXT_NAMESPACE "ImpactySFXSynthEditor"

namespace LBSImpactSFXSynth
{
	namespace Editor
	{
		void FImpactSpawnInfoLayoutCustomizationBase::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
		{
			HeaderRow
			.NameContent()
			[
				StructPropertyHandle->CreatePropertyNameWidget()
			]
			.ValueContent();
		}

		void FImpactSpawnInfoLayoutCustomizationBase::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
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

			ChildBuilder.AddProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FImpactSpawnInfo, StartTime)).ToSharedRef());
			ChildBuilder.AddProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FImpactSpawnInfo, EndTime)).ToSharedRef());
			ChildBuilder.AddProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FImpactSpawnInfo, DampingRatio)).ToSharedRef());
			
			TSharedRef<IPropertyHandle> UseModalSynthHandleRef = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FImpactSpawnInfo, bUseModalSynth)).ToSharedRef();
			ChildBuilder.AddProperty(UseModalSynthHandleRef);
			bUseModalSynthHandle = UseModalSynthHandleRef;

			const TAttribute<EVisibility> ModalSynthVisibility = TAttribute<EVisibility>::Create([this]()
			{
				return GetModalSynthParamsVisibility();
			});

			ChildBuilder.AddProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FImpactSpawnInfo, ModalStartTime)).ToSharedRef())
						.Visibility(ModalSynthVisibility);
			ChildBuilder.AddProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FImpactSpawnInfo, ModalDuration)).ToSharedRef())
						.Visibility(ModalSynthVisibility);
			ChildBuilder.AddProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FImpactSpawnInfo, ModalAmpScale)).ToSharedRef())
						.Visibility(ModalSynthVisibility);
			ChildBuilder.AddProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FImpactSpawnInfo, ModalAmpScaleRange)).ToSharedRef())
						.Visibility(ModalSynthVisibility);
			ChildBuilder.AddProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FImpactSpawnInfo, ModalDecayScale)).ToSharedRef())
									.Visibility(ModalSynthVisibility);
			ChildBuilder.AddProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FImpactSpawnInfo, ModalDecayScaleRange)).ToSharedRef())
						.Visibility(ModalSynthVisibility);
			ChildBuilder.AddProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FImpactSpawnInfo, ModalPitchShift)).ToSharedRef())
						.Visibility(ModalSynthVisibility);
			ChildBuilder.AddProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FImpactSpawnInfo, ModalPitchShiftRange)).ToSharedRef())
						.Visibility(ModalSynthVisibility);
			
			TSharedRef<IPropertyHandle> UseResidualSynthHandleRef = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FImpactSpawnInfo, bUseResidualSynth)).ToSharedRef();
			ChildBuilder.AddProperty(UseResidualSynthHandleRef);
			bUseResidualSynthHandle = UseResidualSynthHandleRef;

			const TAttribute<EVisibility> ResidualSynthVisibility = TAttribute<EVisibility>::Create([this]()
			{
				return GetResidualSynthParamsVisibility();
			});
			
			ChildBuilder.AddProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FImpactSpawnInfo, ResidualStartTime)).ToSharedRef())
						.Visibility(ResidualSynthVisibility);
			ChildBuilder.AddProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FImpactSpawnInfo, ResidualDuration)).ToSharedRef())
						.Visibility(ResidualSynthVisibility);
			ChildBuilder.AddProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FImpactSpawnInfo, ResidualAmpScale)).ToSharedRef())
						.Visibility(ResidualSynthVisibility);
			ChildBuilder.AddProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FImpactSpawnInfo, ResidualAmpScaleRange)).ToSharedRef())
						.Visibility(ResidualSynthVisibility);
			ChildBuilder.AddProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FImpactSpawnInfo, ResidualPlaySpeed)).ToSharedRef())
						.Visibility(ResidualSynthVisibility);
			ChildBuilder.AddProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FImpactSpawnInfo, ResidualPlaySpeedRange)).ToSharedRef())
						.Visibility(ResidualSynthVisibility);
			ChildBuilder.AddProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FImpactSpawnInfo, ResidualPitchShift)).ToSharedRef())
						.Visibility(ResidualSynthVisibility);
			ChildBuilder.AddProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FImpactSpawnInfo, ResidualPitchShiftRange)).ToSharedRef())
						.Visibility(ResidualSynthVisibility);
		}

		EVisibility FImpactSpawnInfoLayoutCustomizationBase::GetModalSynthParamsVisibility() const
		{
			if (!bUseModalSynthHandle.IsValid())
			{
				return EVisibility::Hidden;
			}

			bool OutValue;
			bUseModalSynthHandle->GetValue(OutValue);
			return (OutValue ? EVisibility::Visible : EVisibility::Hidden);
		}

		EVisibility FImpactSpawnInfoLayoutCustomizationBase::GetResidualSynthParamsVisibility() const
		{
			if (!bUseResidualSynthHandle.IsValid())
			{
				return EVisibility::Hidden;
			}

			bool OutValue;
			bUseResidualSynthHandle->GetValue(OutValue);
			return (OutValue ? EVisibility::Visible : EVisibility::Hidden);
		}
	} 
} 
#undef LOCTEXT_NAMESPACE 
