// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "DetailCustomizations/WorldstateSetValueContainerDetails.h"

#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Int.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_String.h"
#include "DetailWidgetRow.h"
#include "DetailLayoutBuilder.h"
#include "Editor/EditorPerProjectUserSettings.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "IPropertyUtilities.h"
#include "IDetailChildrenBuilder.h"
#include "Misc/EngineVersionComparison.h"
#include "PropertyCustomizationHelpers.h"
#include "Textures/SlateIcon.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Text/STextBlock.h"

#include "HTNEditor.h"
#include "HTNNode.h"
#include "Utility/WorldstateSetValueContainer.h"
#include "Widgets/Input/SCheckBox.h"

#define LOCTEXT_NAMESPACE "WorldstateSetValueContainerDetails"

TSharedRef<IPropertyTypeCustomization> FWorldstateSetValueContainerDetails::MakeInstance() { return MakeShared<FWorldstateSetValueContainerDetails>(); }

FWorldstateSetValueContainerDetails::FWorldstateSetValueContainerDetails() :
	CachedEnumType(nullptr),
	PropUtils(nullptr)
{}

void FWorldstateSetValueContainerDetails::CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils )
{
	MyStructProperty = StructPropertyHandle;
	PropUtils = StructCustomizationUtils.GetPropertyUtilities().Get();

	CacheBlackboardData();
	CachedIntPropertyHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWorldstateSetValueContainer, IntValue));
	CachedObjectPropertyHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWorldstateSetValueContainer, ObjectValue));
	if (CachedBlackboardKeyIDProperty.IsValid())
	{
		CachedBlackboardKeyIDProperty->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this,
			&FWorldstateSetValueContainerDetails::OnKeyIDChanged
		));
		OnKeyIDChanged();
	}

	HeaderRow.IsEnabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &FWorldstateSetValueContainerDetails::IsEditingEnabled)))
		.NameContent()
		[
			StructPropertyHandle->CreatePropertyNameWidget()
		];
}

void FWorldstateSetValueContainerDetails::CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	const auto MakeVisibility = [&](const TArray<TSubclassOf<UBlackboardKeyType>, TInlineAllocator<2>>& BlackboardKeyTypes) -> TAttribute<EVisibility>
	{
		return TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this,
			&FWorldstateSetValueContainerDetails::GetPropertyEditorVisibility, BlackboardKeyTypes
		));
	};

	StructBuilder
		.AddProperty(StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWorldstateSetValueContainer, FloatValue)).ToSharedRef())
		.Visibility(MakeVisibility({ UBlackboardKeyType_Float::StaticClass() }));

	StructBuilder
		.AddProperty(CachedIntPropertyHandle.ToSharedRef())
		.Visibility(MakeVisibility({ UBlackboardKeyType_Int::StaticClass() }));

	StructBuilder
		.AddProperty(StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWorldstateSetValueContainer, StringValue)).ToSharedRef())
		.Visibility(MakeVisibility({ UBlackboardKeyType_String::StaticClass() }));

	StructBuilder
		.AddProperty(StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWorldstateSetValueContainer, VectorValue)).ToSharedRef())
		.Visibility(MakeVisibility({ UBlackboardKeyType_Vector::StaticClass() }));

	StructBuilder
		.AddProperty(StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWorldstateSetValueContainer, RotatorValue)).ToSharedRef())
		.Visibility(MakeVisibility({ UBlackboardKeyType_Rotator::StaticClass() }));

	StructBuilder
		.AddProperty(StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWorldstateSetValueContainer, NameValue)).ToSharedRef())
		.Visibility(MakeVisibility({ UBlackboardKeyType_Name::StaticClass() }));

	StructBuilder
		.AddProperty(CachedObjectPropertyHandle.ToSharedRef())
		.Visibility(MakeVisibility({ UBlackboardKeyType_Object::StaticClass() }))
		.CustomWidget()
		.NameContent()
		[
			CachedObjectPropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MinDesiredWidth(250.0f)
		[
			SNew(SObjectPropertyEntryBox)
			.PropertyHandle(CachedObjectPropertyHandle)
			.ThumbnailPool(StructCustomizationUtils.GetThumbnailPool())
			// Filtering by class is done here instead of using AllowedClass, 
			// because the allowed class may change when reassigning to a different blackboard key.
			.OnShouldFilterAsset(FOnShouldFilterAsset::CreateSP(this, &FWorldstateSetValueContainerDetails::OnShouldFilterObjectAsset))
		];

	StructBuilder
		.AddProperty(CachedObjectPropertyHandle.ToSharedRef())
		.Visibility(MakeVisibility({ UBlackboardKeyType_Class::StaticClass() }))
		.CustomWidget()
		.NameContent()
		[
			CachedObjectPropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MinDesiredWidth(250.0f)
		[
			SNew(SClassPropertyEntryBox)
			.ShowTreeView(true)
			.SelectedClass_Lambda([Handle = CachedObjectPropertyHandle]() -> const UClass*
			{
				const UObject* Object = nullptr;
				return Handle->GetValue(Object) == FPropertyAccess::Success ?
					Cast<UClass>(Object) : 
					nullptr;
			})
			.OnSetClass(FOnSetClass::CreateSP(this, &FWorldstateSetValueContainerDetails::OnSetClassValue))
		];

	StructBuilder
		.AddProperty(CachedIntPropertyHandle.ToSharedRef())
		.Visibility(MakeVisibility({ UBlackboardKeyType_Bool::StaticClass() }))
		.CustomWidget()
		.NameContent()
		[
			CachedIntPropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			SNew(SCheckBox)
			.IsChecked_Lambda([Handle = CachedIntPropertyHandle]() -> ECheckBoxState
			{
				int32 Value = 0;
				return Handle->GetValue(Value) != FPropertyAccess::Success ? ECheckBoxState::Undetermined :
					Value != 0 ? ECheckBoxState::Checked :
					ECheckBoxState::Unchecked;
			})
			.OnCheckStateChanged_Lambda([Handle = CachedIntPropertyHandle](ECheckBoxState NewState)
			{
				Handle->SetValue(NewState == ECheckBoxState::Checked ? 1 : 0);
			})
		];

	StructBuilder
		.AddProperty(CachedIntPropertyHandle.ToSharedRef())
		.Visibility(MakeVisibility({ UBlackboardKeyType_Enum::StaticClass(), UBlackboardKeyType_NativeEnum::StaticClass() }))
		.CustomWidget()
		.NameContent()
		[
			CachedIntPropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			SNew(SComboButton)
			.OnGetMenuContent(this, &FWorldstateSetValueContainerDetails::OnGetEnumValueContent)
			.ContentPadding(FMargin(2.0f, 2.0f))
			.ButtonContent()
			[
				SNew(STextBlock)
				.Text(this, &FWorldstateSetValueContainerDetails::GetCurrentEnumValueDesc)
				.Font(IDetailLayoutBuilder::GetDetailFont())
			]
		];
}

void FWorldstateSetValueContainerDetails::CacheBlackboardData()
{	
	TArray<UObject*> MyObjects;
	MyStructProperty->GetOuterObjects(MyObjects);
	for (int32 ObjectIdx = 0; ObjectIdx < MyObjects.Num(); ObjectIdx++)
	{
		if (UBlackboardData* const BlackboardAsset = FindBlackboardAsset(MyObjects[ObjectIdx]))
		{
			CachedBlackboardAsset = BlackboardAsset;
			break;
		}
	}

	CachedBlackboardKeyIDProperty = FindBlackboardKeySelectorProperty();
}

UBlackboardData* FWorldstateSetValueContainerDetails::FindBlackboardAsset(UObject* InObj) const
{
	for (UObject* Object = InObj; Object; Object = Object->GetOuter())
	{
		if (UHTNNode* const Node = Cast<UHTNNode>(Object))
		{
			return Node->GetBlackboardAsset();
		}
	}

	return nullptr;
}

void FWorldstateSetValueContainerDetails::OnKeyIDChanged()
{
	CachedEnumType = nullptr;
	CachedEnumNames.Reset();
	CachedBaseClass = nullptr;

	const UBlackboardKeyType* const KeyType = GetBlackboardKeyType();
	if (KeyType)
	{
		if (const UBlackboardKeyType_Enum* const EnumKeyType = Cast<UBlackboardKeyType_Enum>(KeyType))
		{
			CachedEnumType = EnumKeyType->EnumType;
		}
		else if (const UBlackboardKeyType_NativeEnum* const NativeEnumKeyType = Cast<UBlackboardKeyType_NativeEnum>(KeyType))
		{
			CachedEnumType = NativeEnumKeyType->EnumType;
		}
	}

	if (CachedEnumType.IsValid())
	{
		for (int32 I = 0; I < CachedEnumType->NumEnums() - 1; ++I)
		{
			CachedEnumNames.Add(CachedEnumType->GetDisplayNameTextByIndex(I).ToString());
		}

		int32 CurrentIntValue = 0;
		if (CachedIntPropertyHandle.IsValid() && CachedIntPropertyHandle->GetValue(CurrentIntValue) == FPropertyAccess::Success)
		{
			if (!CachedEnumNames.IsValidIndex(CurrentIntValue))
			{
				CachedIntPropertyHandle->SetValue(0);
			}
		}
	}
	else if (const UBlackboardKeyType_Class* const ClassKeyType = Cast<UBlackboardKeyType_Class>(KeyType))
	{
		CachedBaseClass = ClassKeyType->BaseClass ? ClassKeyType->BaseClass.Get() : UObject::StaticClass();
		UObject* CurrentClassObjectValue = nullptr;
		if (CachedObjectPropertyHandle->GetValue(CurrentClassObjectValue) == FPropertyAccess::Success)
		{
			const UClass* const CurrentClassValue = Cast<UClass>(CurrentClassObjectValue);
			if (!CurrentClassValue || !CurrentClassValue->IsChildOf(CachedBaseClass.Get()))
			{
				CachedObjectPropertyHandle->SetValue(StaticCast<UObject*>(nullptr));
			}
		}
	}
	else if (const UBlackboardKeyType_Object* const ObjectKeyType = Cast<UBlackboardKeyType_Object>(KeyType))
	{
		CachedBaseClass = ObjectKeyType->BaseClass ? ObjectKeyType->BaseClass.Get() : UObject::StaticClass();
		UObject* CurrentObjectValue = nullptr;
		if (CachedObjectPropertyHandle->GetValue(CurrentObjectValue) == FPropertyAccess::Success)
		{
			if (CurrentObjectValue && !CurrentObjectValue->IsA(CachedBaseClass.Get()))
			{
				CachedObjectPropertyHandle->SetValue(StaticCast<UObject*>(nullptr));
			}
		}
	}
}

UBlackboardKeyType* FWorldstateSetValueContainerDetails::GetBlackboardKeyType() const
{
	// FBlackboardKeySelector::SelectedKeyID was changed from a uint8 to int32 in 5.2
#if UE_VERSION_OLDER_THAN(5, 2, 0)
	uint8 KeyIndex;
#else
	int32 KeyIndex;
#endif
	if (CachedBlackboardKeyIDProperty.IsValid() && CachedBlackboardKeyIDProperty->GetValue(KeyIndex) == FPropertyAccess::Success)
	{
		if (CachedBlackboardAsset.IsValid())
		{
			const FBlackboard::FKey KeyID(KeyIndex);
			if (const FBlackboardEntry* const KeyEntry = CachedBlackboardAsset->GetKey(KeyID))
			{
				return KeyEntry->KeyType;
			}
		}
	}

	return nullptr;
}

EVisibility FWorldstateSetValueContainerDetails::GetPropertyEditorVisibility(const TArray<TSubclassOf<UBlackboardKeyType>, TInlineAllocator<2>> BlackboardKeyTypes) const
{
	if (const UBlackboardKeyType* const KeyType = GetBlackboardKeyType())
	{
		if (BlackboardKeyTypes.ContainsByPredicate([=](TSubclassOf<UBlackboardKeyType> Class) { return KeyType->IsA(Class); }))
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}

bool FWorldstateSetValueContainerDetails::OnShouldFilterObjectAsset(const FAssetData& AssetData) const
{
	if (const UClass* const BaseClass = CachedBaseClass.Get())
	{
		if (const UClass* const AssetClass = AssetData.GetClass())
		{
			return !AssetClass->IsChildOf(BaseClass);
		}
	}

	return true;
}

void FWorldstateSetValueContainerDetails::OnSetClassValue(const UClass* NewClass) const
{
	const UClass* const Class = NewClass && (!CachedBaseClass.IsValid() || NewClass->IsChildOf(CachedBaseClass.Get())) ?
		NewClass : 
		nullptr;

	if (CachedObjectPropertyHandle.IsValid())
	{
		CachedObjectPropertyHandle->SetValue(StaticCast<const UObject*>(Class));
	}
}

TSharedPtr<IPropertyHandle> FWorldstateSetValueContainerDetails::FindBlackboardKeySelectorProperty() const
{
	static const FName BlackboardKeyName(TEXT("BlackboardKey"));
	static const FName SelectedKeyIDName(TEXT("SelectedKeyID"));
	for (TSharedPtr<IPropertyHandle> Handle = MyStructProperty; Handle.IsValid(); Handle = Handle->GetParentHandle())
	{
		const TSharedPtr<IPropertyHandle> BlackboardKeyHandle = Handle->GetChildHandle(BlackboardKeyName, /*bRecurse=*/true);
		if (BlackboardKeyHandle.IsValid())
		{
			TSharedPtr<IPropertyHandle> BlackboardKeyIDHandle = BlackboardKeyHandle->GetChildHandle(SelectedKeyIDName, /*bRecurse=*/false);
			if (BlackboardKeyIDHandle.IsValid())
			{
				return BlackboardKeyIDHandle;
			}
		}
	}

	return nullptr;
}

bool FWorldstateSetValueContainerDetails::IsEditingEnabled() const
{
	return FHTNEditor::IsPIENotSimulating() && PropUtils->IsPropertyEditingEnabled(); 
}

TSharedRef<SWidget> FWorldstateSetValueContainerDetails::OnGetEnumValueContent() const
{
	FMenuBuilder MenuBuilder(/*bInShouldCloseWindowAfterMenuSelection=*/true, /*InCommandList=*/nullptr);

	for (int32 I = 0; I < CachedEnumNames.Num(); ++I)
	{
		MenuBuilder.AddMenuEntry(
			FText::FromString(CachedEnumNames[I]), 
			TAttribute<FText>(), 
			FSlateIcon(), 
			FExecuteAction::CreateLambda([Handle = CachedIntPropertyHandle, I]() { Handle->SetValue(I); })
		);
	}

	return MenuBuilder.MakeWidget();
}

FText FWorldstateSetValueContainerDetails::GetCurrentEnumValueDesc() const
{
	if (CachedEnumType.IsValid())
	{
		int32 CurrentIntValue = INDEX_NONE;
		if (CachedIntPropertyHandle->GetValue(CurrentIntValue) == FPropertyAccess::Success)
		{
			if (CachedEnumNames.IsValidIndex(CurrentIntValue))
			{
				return FText::FromString(CachedEnumNames[CurrentIntValue]);
			}
		}
	}

	return FText::GetEmpty();
}

#undef LOCTEXT_NAMESPACE
