// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "DetailCustomizations/HTNBlackboardDecoratorDetails.h"
#include "Misc/Attribute.h"
#include "Misc/EngineVersionComparison.h"
#include "Layout/Margin.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "SlateOptMacros.h"
#include "Textures/SlateIcon.h"
#include "Framework/Commands/UIAction.h"
#include "Widgets/SWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Input/SComboButton.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailPropertyRow.h"
#include "IPropertyUtilities.h"
#include "DetailCategoryBuilder.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Enum.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_NativeEnum.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Int.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"

#include "HTNNode.h"
#include "Decorators/HTNDecorator_Blackboard.h"
#include "HTNEditor.h"

#define LOCTEXT_NAMESPACE "HTNBlackboardDecoratorDetails"

TSharedRef<IDetailCustomization> FHTNBlackboardDecoratorDetails::MakeInstance()
{
	return MakeShareable( new FHTNBlackboardDecoratorDetails );
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void FHTNBlackboardDecoratorDetails::CustomizeDetails( IDetailLayoutBuilder& DetailLayout )
{
	PropUtils = &DetailLayout.GetPropertyUtilities().Get();
	
	CacheBlackboardData(DetailLayout);
	const bool bIsEnabled = CachedBlackboardAsset.IsValid();
	const TAttribute<bool> PropertyEditCheck = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &FHTNBlackboardDecoratorDetails::IsEditingEnabled));

	IDetailCategoryBuilder& BBCategory = DetailLayout.EditCategory("Blackboard");
	IDetailPropertyRow& KeySelectorRow = BBCategory.AddProperty(DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UHTNDecorator_Blackboard, BlackboardKey)));
	KeySelectorRow.IsEnabled(bIsEnabled);

	KeyIDProperty = DetailLayout.GetProperty(FName(TEXT("BlackboardKey.SelectedKeyID")), UHTNDecorator_BlackboardBase::StaticClass());
	if (KeyIDProperty.IsValid())
	{
		FSimpleDelegate OnKeyChangedDelegate = FSimpleDelegate::CreateSP( this, &FHTNBlackboardDecoratorDetails::OnKeyIDChanged );
		KeyIDProperty->SetOnPropertyValueChanged(OnKeyChangedDelegate);
		OnKeyIDChanged();
	}

#if WITH_EDITORONLY_DATA

	IDetailPropertyRow& BasicOpRow = BBCategory.AddProperty(DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UHTNDecorator_Blackboard, BasicOperation)));
	BasicOpRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FHTNBlackboardDecoratorDetails::GetBasicOpVisibility)));
	BasicOpRow.IsEnabled(PropertyEditCheck);
	
	IDetailPropertyRow& ArithmeticOpRow = BBCategory.AddProperty(DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UHTNDecorator_Blackboard, ArithmeticOperation)));
	ArithmeticOpRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FHTNBlackboardDecoratorDetails::GetArithmeticOpVisibility)));
	ArithmeticOpRow.IsEnabled(PropertyEditCheck);

	IDetailPropertyRow& TextOpRow = BBCategory.AddProperty(DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UHTNDecorator_Blackboard, TextOperation)));
	TextOpRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FHTNBlackboardDecoratorDetails::GetTextOpVisibility)));
	TextOpRow.IsEnabled(PropertyEditCheck);

#endif // WITH_EDITORONLY_DATA

	IntValueProperty = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UHTNDecorator_Blackboard, IntValue));
	IDetailPropertyRow& IntValueRow = BBCategory.AddProperty(IntValueProperty);
	IntValueRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FHTNBlackboardDecoratorDetails::GetIntValueVisibility)));
	IntValueRow.IsEnabled(PropertyEditCheck);

	IDetailPropertyRow& FloatValueRow = BBCategory.AddProperty(DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UHTNDecorator_Blackboard, FloatValue)));
	FloatValueRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FHTNBlackboardDecoratorDetails::GetFloatValueVisibility)));
	FloatValueRow.IsEnabled(PropertyEditCheck);

	IDetailPropertyRow& StringValueRow = BBCategory.AddProperty(DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UHTNDecorator_Blackboard, StringValue)));
	StringValueRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FHTNBlackboardDecoratorDetails::GetStringValueVisibility)));
	StringValueRow.IsEnabled(PropertyEditCheck);

	IDetailPropertyRow& EnumValueRow = BBCategory.AddProperty(IntValueProperty);
	EnumValueRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FHTNBlackboardDecoratorDetails::GetEnumValueVisibility)));
	EnumValueRow.IsEnabled(PropertyEditCheck);
	EnumValueRow.CustomWidget()
		.NameContent()
		[
			IntValueProperty->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			SNew(SComboButton)
			.OnGetMenuContent(this, &FHTNBlackboardDecoratorDetails::OnGetEnumValueContent)
 			.ContentPadding(FMargin( 2.0f, 2.0f ))
			.ButtonContent()
			[
				SNew(STextBlock) 
				.Text(this, &FHTNBlackboardDecoratorDetails::GetCurrentEnumValueDesc)
				.Font(IDetailLayoutBuilder::GetDetailFont())
			]
		];
}

bool FHTNBlackboardDecoratorDetails::IsEditingEnabled() const
{
	return FHTNEditor::IsPIENotSimulating() && PropUtils->IsPropertyEditingEnabled();
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void FHTNBlackboardDecoratorDetails::CacheBlackboardData(IDetailLayoutBuilder& DetailLayout)
{
	TArray<TWeakObjectPtr<UObject> > MyOuters;
	DetailLayout.GetObjectsBeingCustomized(MyOuters);

	CachedBlackboardAsset.Reset();
	for (int32 i = 0; i < MyOuters.Num(); i++)
	{
		UHTNNode* NodeOb = Cast<UHTNNode>(MyOuters[i].Get());
		if (NodeOb)
		{
			CachedBlackboardAsset = NodeOb->GetBlackboardAsset();
			break;
		}
	}
}

void FHTNBlackboardDecoratorDetails::OnKeyIDChanged()
{
	CachedOperationType = EBlackboardKeyOperation::Basic;
	CachedKeyType = nullptr;

	UBlackboardData* Blackboard = CachedBlackboardAsset.Get();
	if (Blackboard == nullptr)
	{
		return;
	}

	// FBlackboardKeySelector::SelectedKeyID was changed from a uint8 to int32 in 5.2
#if UE_VERSION_OLDER_THAN(5, 2, 0)
	uint8 KeyIndex;
#else
	int32 KeyIndex;
#endif
	FPropertyAccess::Result Result = KeyIDProperty->GetValue(KeyIndex);
	if (Result != FPropertyAccess::Success)
	{
		return;
	}

	const FBlackboard::FKey KeyID(KeyIndex);
	const FBlackboardEntry* KeyEntry = Blackboard->GetKey(KeyID);
	if (KeyEntry && KeyEntry->KeyType)
	{
		CachedKeyType = KeyEntry->KeyType->GetClass();
		CachedOperationType = KeyEntry->KeyType->GetTestOperation();
	}
	else
	{
		CachedKeyType = nullptr;
		CachedOperationType = 0;
	}

	// Special handling of enum type: cache all names for combo box (display names)
	CachedCustomObjectType =
		CachedKeyType == UBlackboardKeyType_Enum::StaticClass() ? StaticCast<UBlackboardKeyType_Enum*>(KeyEntry->KeyType)->EnumType :
		CachedKeyType == UBlackboardKeyType_NativeEnum::StaticClass() ? StaticCast<UBlackboardKeyType_NativeEnum*>(KeyEntry->KeyType)->EnumType :
		nullptr;

	EnumPropValues.Reset();
	if (CachedCustomObjectType)
	{
		for (int32 i = 0; i < CachedCustomObjectType->NumEnums() - 1; i++)
		{
			FString DisplayedName = CachedCustomObjectType->GetDisplayNameTextByIndex(i).ToString();
			EnumPropValues.Add(DisplayedName);
		}
	}
}

TSharedRef<SWidget> FHTNBlackboardDecoratorDetails::OnGetEnumValueContent() const
{
	FMenuBuilder MenuBuilder(true, nullptr);

	for (int32 i = 0; i < EnumPropValues.Num(); i++)
	{
		FUIAction ItemAction( FExecuteAction::CreateSP( const_cast<FHTNBlackboardDecoratorDetails*>(this), &FHTNBlackboardDecoratorDetails::OnEnumValueComboChange, i ) );
		MenuBuilder.AddMenuEntry( FText::FromString( EnumPropValues[i] ), TAttribute<FText>(), FSlateIcon(), ItemAction);
	}

	return MenuBuilder.MakeWidget();
}

FText FHTNBlackboardDecoratorDetails::GetCurrentEnumValueDesc() const
{
	FPropertyAccess::Result Result = FPropertyAccess::Fail;
	int32 CurrentIntValue = INDEX_NONE;

	if (CachedCustomObjectType)
	{	
		Result = IntValueProperty->GetValue(CurrentIntValue);
	}

	return (Result == FPropertyAccess::Success && EnumPropValues.IsValidIndex(CurrentIntValue))
		? FText::FromString(EnumPropValues[CurrentIntValue])
		: FText::GetEmpty();
}

void FHTNBlackboardDecoratorDetails::OnEnumValueComboChange(int32 Index)
{
	IntValueProperty->SetValue(Index);
}

EVisibility FHTNBlackboardDecoratorDetails::GetIntValueVisibility() const
{
	return (CachedKeyType == UBlackboardKeyType_Int::StaticClass()) ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility FHTNBlackboardDecoratorDetails::GetFloatValueVisibility() const
{
	return (CachedKeyType == UBlackboardKeyType_Float::StaticClass()) ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility FHTNBlackboardDecoratorDetails::GetStringValueVisibility() const
{
	return (CachedOperationType == EBlackboardKeyOperation::Text) ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility FHTNBlackboardDecoratorDetails::GetEnumValueVisibility() const
{
	if (CachedKeyType == UBlackboardKeyType_Enum::StaticClass() ||
		CachedKeyType == UBlackboardKeyType_NativeEnum::StaticClass())
	{
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;
}

EVisibility FHTNBlackboardDecoratorDetails::GetBasicOpVisibility() const
{
	return (CachedOperationType == EBlackboardKeyOperation::Basic) ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility FHTNBlackboardDecoratorDetails::GetArithmeticOpVisibility() const
{
	return (CachedOperationType == EBlackboardKeyOperation::Arithmetic) ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility FHTNBlackboardDecoratorDetails::GetTextOpVisibility() const
{
	return (CachedOperationType == EBlackboardKeyOperation::Text) ? EVisibility::Visible : EVisibility::Collapsed;
}

#undef LOCTEXT_NAMESPACE
