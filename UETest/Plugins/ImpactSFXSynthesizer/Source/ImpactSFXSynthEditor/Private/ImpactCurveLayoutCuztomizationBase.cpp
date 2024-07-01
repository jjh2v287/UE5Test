// Copyright 2023-2024, Le Binh Son, All rights reserved.

#include "ImpactCurveLayoutCuztomizationBase.h"
#include "Editor.h"
#include "IDetailChildrenBuilder.h"
#include "ImpactSynthCurve.h"
#include "PropertyCustomizationHelpers.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SToolTip.h"

#define LOCTEXT_NAMESPACE "ImpactySFXSynthEditor"

namespace LBSImpactSFXSynth
{
	namespace Editor
	{
		void FImpactCurveCustomizationBase::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
		{
			HeaderRow
			.NameContent()
			[
				StructPropertyHandle->CreatePropertyNameWidget()
			]
			.ValueContent();
		}

		void FImpactCurveCustomizationBase::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
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
			
			CurveHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FImpactSynthCurve, CurveSource)).ToSharedRef();
			CustomizeCurveSelector(ChildBuilder);

			TSharedRef<IPropertyHandle> SharedCurveHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FImpactSynthCurve, CurveShared)).ToSharedRef();
			ChildBuilder.AddProperty(SharedCurveHandle)
				.EditCondition(TAttribute<bool>::Create([this]() { return GetCurveSource() == EImpactSynthCurveSource::Shared; }), nullptr)
				.Visibility(TAttribute<EVisibility>::Create([this]() { return GetCurveSource() == EImpactSynthCurveSource::Shared ? EVisibility::Visible : EVisibility::Hidden; }));
		}

		void FImpactCurveCustomizationBase::CustomizeCurveSelector(IDetailChildrenBuilder& ChildBuilder)
		{
			check(CurveHandle.IsValid());

			auto GetAll = [this, SupportedCurves = GetSupportedCurves()](TArray<TSharedPtr<FString>>& OutNames, TArray<TSharedPtr<SToolTip>>& OutTooltips, TArray<bool>&)
			{
				CurveDisplayStringToNameMap.Reset();
				UEnum* Enum = StaticEnum<EImpactSynthCurveSource>();
				check(Enum);

				for (int32 i = 0; i < Enum->NumEnums(); ++i)
				{
					EImpactSynthCurveSource Curve = static_cast<EImpactSynthCurveSource>(Enum->GetValueByIndex(i));
					if (SupportedCurves.Contains(Curve))
					{
						TSharedPtr<FString> DisplayString = MakeShared<FString>(Enum->GetDisplayNameTextByIndex(i).ToString());

						const FName Name = Enum->GetNameByIndex(i);
						CurveDisplayStringToNameMap.Add(*DisplayString.Get(), Name);

						OutTooltips.Emplace(SNew(SToolTip).Text(Enum->GetToolTipTextByIndex(i)));
						OutNames.Emplace(MoveTemp(DisplayString));
					}
				}
			};

			auto GetValue = [this]()
			{
				UEnum* Enum = StaticEnum<EImpactSynthCurveSource>();
				check(Enum);

				uint8 IntValue;
				if (CurveHandle->GetValue(IntValue) == FPropertyAccess::Success)
				{
					return Enum->GetDisplayNameTextByValue(IntValue).ToString();
				}

				return Enum->GetDisplayNameTextByValue(static_cast<int32>(EImpactSynthCurveSource::Count)).ToString();
			};

			auto SelectedValue = [this](const FString& InSelected)
			{
				UEnum* Enum = StaticEnum<EImpactSynthCurveSource>();
				check(Enum);

				const FName& Name = CurveDisplayStringToNameMap.FindChecked(InSelected);
				const uint8 Value = static_cast<uint8>(Enum->GetValueByName(Name, EGetByNameFlags::ErrorIfNotFound));
				ensure(CurveHandle->SetValue(Value) == FPropertyAccess::Success);
			};

			static const FText CurveDisplayName = LOCTEXT("ImpactSynthCurveProperty", "Modification Curve");
			ChildBuilder.AddCustomRow(CurveDisplayName)
				.NameContent()
				[
					CurveHandle->CreatePropertyNameWidget()
				]
				.ValueContent()
				.MinDesiredWidth(150.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
				[
					SNew(SBox)
					[
						PropertyCustomizationHelpers::MakePropertyComboBox(
							CurveHandle,
							FOnGetPropertyComboBoxStrings::CreateLambda(GetAll),
							FOnGetPropertyComboBoxValue::CreateLambda(GetValue),
							FOnPropertyComboBoxValueSelected::CreateLambda(SelectedValue)
						)
					]
				]
			];
		}

		EImpactSynthCurveSource FImpactCurveCustomizationBase::GetCurveSource() const
		{
			if (!CurveHandle.IsValid())
			{
				return EImpactSynthCurveSource::None;
			}

			uint8 CurveValue = static_cast<uint8>(EImpactSynthCurveSource::None);
			CurveHandle->GetValue(CurveValue);
			return static_cast<EImpactSynthCurveSource>(CurveValue);
		}

		TSet<EImpactSynthCurveSource> FImpactCurveCustomizationBase::GetSupportedCurves() const
		{
			UEnum* Enum = StaticEnum<EImpactSynthCurveSource>();
			check(Enum);

			TSet<EImpactSynthCurveSource> Curves;
			static const int64 MaxValue = static_cast<int64>(EImpactSynthCurveSource::Count);
			for (int32 i = 0; i < Enum->NumEnums(); ++i)
			{
				if (Enum->GetValueByIndex(i) < MaxValue)
				{
					if (!Enum->HasMetaData(TEXT("Hidden"), i))
					{
						Curves.Add(static_cast<EImpactSynthCurveSource>(Enum->GetValueByIndex(i)));
					}
				}
			}

			return Curves;
		}
	} 
} 
#undef LOCTEXT_NAMESPACE 
