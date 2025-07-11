#include "Utility/WorldstateSetValueContainer.h"
#include "BehaviorTree/Blackboard/BlackboardKeyAllTypes.h"

bool FWorldstateSetValueContainer::SetValue(FBlackboardWorldState& WorldState, const FBlackboardKeySelector& KeySelector) const
{
	using HandlerType = decltype(&FWorldstateSetValueContainer::SetValueBool);

#define SET_VALUE_HANDLER(TYPE) { UBlackboardKeyType_##TYPE::StaticClass(), &FWorldstateSetValueContainer::SetValue##TYPE }
	static const TMap<UClass*, HandlerType> KeyClassToHandlerMap
	{
		SET_VALUE_HANDLER(Bool),
		SET_VALUE_HANDLER(Int),
		SET_VALUE_HANDLER(Float),
		SET_VALUE_HANDLER(Enum),
		SET_VALUE_HANDLER(NativeEnum),
		SET_VALUE_HANDLER(String),
		SET_VALUE_HANDLER(Name),
		SET_VALUE_HANDLER(Vector),
		SET_VALUE_HANDLER(Rotator),
		SET_VALUE_HANDLER(Class),
		SET_VALUE_HANDLER(Object)
	};
#undef SET_VALUE_HANDLER

	if (const HandlerType* const Handler = KeyClassToHandlerMap.Find(KeySelector.SelectedKeyType))
	{
		return (this->**Handler)(WorldState, KeySelector);
	}

	return false;
}

FString FWorldstateSetValueContainer::GetValueDescription(const UBlackboardData* BlackboardAsset, FBlackboard::FKey KeyID) const
{
	if (BlackboardAsset)
	{
		if (const FBlackboardEntry* const BlackboardEntry = BlackboardAsset->GetKey(KeyID))
		{
			return GetValueDescription(BlackboardEntry->KeyType);
		}
	}

	return TEXT("Could not resolve blackboard key");
}

FString FWorldstateSetValueContainer::GetValueDescription(const UBlackboardKeyType* ValueType) const
{
	if (ValueType->IsA<UBlackboardKeyType_Bool>())
	{
		return IntValue != 0 ? TEXT("true") : TEXT("false");
	}
	else if (ValueType->IsA<UBlackboardKeyType_Int>())
	{
		return FString::FromInt(IntValue);
	}
	else if (ValueType->IsA<UBlackboardKeyType_Float>())
	{
		return FString::Printf(TEXT("%f"), FloatValue);
;	}
	else if (ValueType->IsA<UBlackboardKeyType_String>())
	{
		return StringValue;
	}
	else if (ValueType->IsA<UBlackboardKeyType_Name>())
	{
		return NameValue.ToString();
	}
	else if (ValueType->IsA<UBlackboardKeyType_Vector>())
	{
		return VectorValue.ToString();
	}
	else if (ValueType->IsA<UBlackboardKeyType_Rotator>())
	{
		return RotatorValue.ToString();
	}
	else if (ValueType->IsA<UBlackboardKeyType_Object>())
	{
		return GetNameSafe(ObjectValue);
	}
	else if (ValueType->IsA<UBlackboardKeyType_Class>())
	{
		return GetNameSafe(Cast<UClass>(ObjectValue));
	}
	else if (const UBlackboardKeyType_Enum* const EnumKeyType = Cast<UBlackboardKeyType_Enum>(ValueType))
	{
		if (UEnum* const EnumType = EnumKeyType->EnumType)
		{
			return EnumType->GetDisplayNameTextByIndex(IntValue).ToString();
		}
	}
	else if (const UBlackboardKeyType_NativeEnum* const NativeEnumKeyType = Cast<UBlackboardKeyType_NativeEnum>(ValueType))
	{
		if (UEnum* const NativeEnumType = NativeEnumKeyType->EnumType)
		{
			return NativeEnumType->GetDisplayNameTextByIndex(IntValue).ToString();
		}
	}

	return FString::Printf(TEXT("Unknown blackboard key type: %s"), 
		*GetNameSafe(ValueType ? ValueType->GetClass() : nullptr)
	);
}

#define SET_VALUE_IMPLEMENTATION(TYPE) \
bool FWorldstateSetValueContainer::SetValue##TYPE(FBlackboardWorldState& WorldState, const FBlackboardKeySelector& KeySelector) const \
{ \
	return WorldState.SetValue<UBlackboardKeyType_##TYPE>(KeySelector.GetSelectedKeyID(), TYPE##Value); \
};

SET_VALUE_IMPLEMENTATION(Int)
SET_VALUE_IMPLEMENTATION(Float)
SET_VALUE_IMPLEMENTATION(String)
SET_VALUE_IMPLEMENTATION(Name)
SET_VALUE_IMPLEMENTATION(Vector)
SET_VALUE_IMPLEMENTATION(Rotator)
SET_VALUE_IMPLEMENTATION(Object)

bool FWorldstateSetValueContainer::SetValueBool(FBlackboardWorldState& WorldState, const FBlackboardKeySelector& KeySelector) const
{
	return WorldState.SetValue<UBlackboardKeyType_Bool>(
		KeySelector.GetSelectedKeyID(),
		StaticCast<UBlackboardKeyType_Bool::FDataType>(IntValue)
	);
};

bool FWorldstateSetValueContainer::SetValueEnum(FBlackboardWorldState& WorldState, const FBlackboardKeySelector& KeySelector) const
{
	return WorldState.SetValue<UBlackboardKeyType_Enum>(
		KeySelector.GetSelectedKeyID(),
		StaticCast<UBlackboardKeyType_Enum::FDataType>(IntValue)
	);
};

bool FWorldstateSetValueContainer::SetValueNativeEnum(FBlackboardWorldState& WorldState, const FBlackboardKeySelector& KeySelector) const
{
	return WorldState.SetValue<UBlackboardKeyType_NativeEnum>(
		KeySelector.GetSelectedKeyID(),
		StaticCast<UBlackboardKeyType_NativeEnum::FDataType>(IntValue)
	);
};

bool FWorldstateSetValueContainer::SetValueClass(FBlackboardWorldState& WorldState, const FBlackboardKeySelector& KeySelector) const
{
	return WorldState.SetValue<UBlackboardKeyType_Class>(
		KeySelector.GetSelectedKeyID(),
		Cast<UClass>(ObjectValue)
	);
};

#undef SET_VALUE_IMPLEMENTATION