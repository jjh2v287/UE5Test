// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "WorldStateProxy.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Rotator.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Name.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_String.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Int.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Enum.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Class.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"

UWorldStateProxy::UWorldStateProxy(const FObjectInitializer& Initializer) : Super(Initializer),
	bIsEditable(true)
{}

bool UWorldStateProxy::CopyValueFrom(const FBlackboardWorldState& SourceWorldState, FBlackboard::FKey KeyID)
{
	if (!bIsEditable)
	{
		UE_VLOG_UELOG(GetOwner(), LogHTN, Error,
			TEXT("Trying to set value on a read-only Worldstate! Key: %s. Worldstates are read-only during plan recheck."),
			*GetKeyName(KeyID).ToString());
		return false;
	}

	if (WorldState.IsValid())
	{
		SourceWorldState.CopyValue(*WorldState, KeyID);
		return true;
	}

	UHTNComponent* const Owner = GetOwner();
	if (ensureMsgf(Owner, TEXT("Trying to set value on a worldstate proxy that has no Owner component assigned.")))
	{
		if (UBlackboardComponent* const BlackboardComponent = Owner->GetBlackboardComponent())
		{
			SourceWorldState.CopyValue(*BlackboardComponent, KeyID);
			return true;
		}
	}

	return false;
}

bool UWorldStateProxy::TestBasicOperation(const UBlackboardKeyType& Key, FBlackboard::FKey KeyID, EBasicKeyOperation::Type Type) const
{	
	if (WorldState.IsValid())
	{
		return WorldState->TestBasicOperation(Key, KeyID, Type);
	}

	const UHTNComponent* const Owner = GetOwner();
	if (ensure(Owner))
	{
		if (const UBlackboardComponent* const BlackboardComponent = Owner->GetBlackboardComponent())
		{
			const uint8* const KeyMemory = BlackboardComponent->GetKeyRawData(KeyID);
			return Key.WrappedTestBasicOperation(*BlackboardComponent, KeyMemory, Type);
		}
	}

	return false;
}

bool UWorldStateProxy::TestArithmeticOperation(const UBlackboardKeyType& Key, FBlackboard::FKey KeyID, EArithmeticKeyOperation::Type Type, int32 IntValue, float FloatValue) const
{
	if (WorldState.IsValid())
	{
		return WorldState->TestArithmeticOperation(Key, KeyID, Type, IntValue, FloatValue);
	}

	const UHTNComponent* const Owner = GetOwner();
	if (ensure(Owner))
	{
		if (const UBlackboardComponent* const BlackboardComponent = Owner->GetBlackboardComponent())
		{
			const uint8* const KeyMemory = BlackboardComponent->GetKeyRawData(KeyID);
			return Key.WrappedTestArithmeticOperation(*BlackboardComponent, KeyMemory, Type, IntValue, FloatValue);
		}
	}

	return false;
}

bool UWorldStateProxy::TestTextOperation(const UBlackboardKeyType& Key, FBlackboard::FKey KeyID, ETextKeyOperation::Type Type, const FString& StringValue) const
{
	if (WorldState.IsValid())
	{
		return WorldState->TestTextOperation(Key, KeyID, Type, StringValue);
	}

	const UHTNComponent* const Owner = GetOwner();
	if (ensure(Owner))
	{
		if (const UBlackboardComponent* const BlackboardComponent = Owner->GetBlackboardComponent())
		{
			const uint8* const KeyMemory = BlackboardComponent->GetKeyRawData(KeyID);
			return Key.WrappedTestTextOperation(*BlackboardComponent, KeyMemory, Type, StringValue);
		}
	}

	return false;
}

bool UWorldStateProxy::GetLocation(const FBlackboardKeySelector& KeySelector, FVector& OutLocation, AActor*& OutActor, UActorComponent*& OutComponent) const
{
	if (KeySelector.SelectedKeyType == UBlackboardKeyType_Vector::StaticClass())
	{
		OutLocation = GetValue<UBlackboardKeyType_Vector>(KeySelector.GetSelectedKeyID());
		OutActor = nullptr;
		OutComponent = nullptr;
		return true;
	}

	if (KeySelector.SelectedKeyType == UBlackboardKeyType_Object::StaticClass())
	{
		UObject* const KeyValue = GetValue<UBlackboardKeyType_Object>(KeySelector.GetSelectedKeyID());
		if (AActor* const TargetActor = Cast<AActor>(KeyValue))
		{
			OutLocation = TargetActor->GetActorLocation();
			OutActor = TargetActor;
			OutComponent = nullptr;
			return true;
		}
		else if (UActorComponent* const ActorComponent = Cast<UActorComponent>(KeyValue))
		{
			OutActor = ActorComponent->GetOwner();
			OutComponent = ActorComponent;

			if (const USceneComponent* const SceneComponent = Cast<const USceneComponent>(KeyValue))
			{
				OutLocation = SceneComponent->GetComponentLocation();
				return true;
			}
			else if (ensure(OutActor))
			{
				OutLocation = OutActor->GetActorLocation();
				return true;
			}
		}
	}

	OutLocation = FAISystem::InvalidLocation;
	OutActor = nullptr;
	OutComponent = nullptr;
	return false;
}

FVector UWorldStateProxy::GetLocation(const FBlackboardKeySelector& KeySelector, AActor** OutActor, UActorComponent** OutComponent) const
{
	if (KeySelector.SelectedKeyType == UBlackboardKeyType_Vector::StaticClass())
	{
		if (OutActor)
		{
			*OutActor = nullptr;
		}

		if (OutComponent)
		{
			*OutComponent = nullptr;
		}

		return GetValue<UBlackboardKeyType_Vector>(KeySelector.GetSelectedKeyID());
	}

	if (KeySelector.SelectedKeyType == UBlackboardKeyType_Object::StaticClass())
	{
		UObject* const KeyValue = GetValue<UBlackboardKeyType_Object>(KeySelector.GetSelectedKeyID());
		if (AActor* const TargetActor = Cast<AActor>(KeyValue))
		{
			if (OutActor)
			{
				*OutActor = TargetActor;
			}

			if (OutComponent)
			{
				*OutComponent = nullptr;
			}

			return TargetActor->GetActorLocation();
		}
		else if (UActorComponent* const ActorComponent = Cast<UActorComponent>(KeyValue))
		{
			AActor* const OwnerActor = ActorComponent->GetOwner();
			if (OutActor)
			{
				*OutActor = OwnerActor;
			}

			if (OutComponent)
			{
				*OutComponent = ActorComponent;
			}

			if (const USceneComponent* const SceneComponent = Cast<const USceneComponent>(KeyValue))
			{
				return SceneComponent->GetComponentLocation();
			}
			else if (ensure(OwnerActor))
			{
				return OwnerActor->GetActorLocation();
			}
		}
	}

	if (OutActor)
	{
		*OutActor = nullptr;
	}

	if (OutComponent)
	{
		*OutComponent = nullptr;
	}

	return FAISystem::InvalidLocation;
}

UObject* UWorldStateProxy::GetValueAsObject (const FName& KeyName) const { return GetValue<UBlackboardKeyType_Object>(KeyName); }
AActor*  UWorldStateProxy::GetValueAsActor  (const FName& KeyName) const { return Cast<AActor>(GetValueAsObject(KeyName)); }
UClass*  UWorldStateProxy::GetValueAsClass  (const FName& KeyName) const { return GetValue<UBlackboardKeyType_Class>(KeyName); }
uint8    UWorldStateProxy::GetValueAsEnum   (const FName& KeyName) const { return GetValue<UBlackboardKeyType_Enum>(KeyName); }
int32    UWorldStateProxy::GetValueAsInt    (const FName& KeyName) const { return GetValue<UBlackboardKeyType_Int>(KeyName); }
float    UWorldStateProxy::GetValueAsFloat  (const FName& KeyName) const { return GetValue<UBlackboardKeyType_Float>(KeyName); }
bool     UWorldStateProxy::GetValueAsBool   (const FName& KeyName) const { return GetValue<UBlackboardKeyType_Bool>(KeyName); }
FString  UWorldStateProxy::GetValueAsString (const FName& KeyName) const { return GetValue<UBlackboardKeyType_String>(KeyName); }
FName    UWorldStateProxy::GetValueAsName   (const FName& KeyName) const { return GetValue<UBlackboardKeyType_Name>(KeyName); }
FVector  UWorldStateProxy::GetValueAsVector (const FName& KeyName) const { return GetValue<UBlackboardKeyType_Vector>(KeyName); }
FRotator UWorldStateProxy::GetValueAsRotator(const FName& KeyName) const { return GetValue<UBlackboardKeyType_Rotator>(KeyName); }

void UWorldStateProxy::SetValueAsObject (const FName& KeyName, UObject* Value) { SetValue<UBlackboardKeyType_Object>(KeyName, Value); }
void UWorldStateProxy::SetValueAsClass  (const FName& KeyName, UClass* Value) { SetValue<UBlackboardKeyType_Class>(KeyName, Value); }
void UWorldStateProxy::SetValueAsEnum   (const FName& KeyName, uint8 Value) { SetValue<UBlackboardKeyType_Enum>(KeyName, Value); }
void UWorldStateProxy::SetValueAsInt    (const FName& KeyName, int32 Value) { SetValue<UBlackboardKeyType_Int>(KeyName, Value); }
void UWorldStateProxy::SetValueAsFloat  (const FName& KeyName, float Value) { SetValue<UBlackboardKeyType_Float>(KeyName, Value); }
void UWorldStateProxy::SetValueAsBool   (const FName& KeyName, bool Value) { SetValue<UBlackboardKeyType_Bool>(KeyName, Value); }
void UWorldStateProxy::SetValueAsString (const FName& KeyName, FString Value) { SetValue<UBlackboardKeyType_String>(KeyName, Value); }
void UWorldStateProxy::SetValueAsName   (const FName& KeyName, FName Value) { SetValue<UBlackboardKeyType_Name>(KeyName, Value); }
void UWorldStateProxy::SetValueAsVector (const FName& KeyName, FVector Value) { SetValue<UBlackboardKeyType_Vector>(KeyName, Value); }
void UWorldStateProxy::SetValueAsRotator(const FName& KeyName, FRotator Value) { SetValue<UBlackboardKeyType_Rotator>(KeyName, Value); }

bool UWorldStateProxy::IsVectorValueSet(const FName& KeyName) const
{
	if (WorldState.IsValid())
	{
		return WorldState->IsVectorValueSet(KeyName);
	}
	else
	{
		const UHTNComponent* const Owner = GetOwner();
		if (ensure(Owner))
		{
			if (const UBlackboardComponent* const BlackboardComponent = Owner->GetBlackboardComponent())
			{
				return BlackboardComponent->IsVectorValueSet(KeyName);
			}
		}
	}

	return false;
}

bool UWorldStateProxy::GetLocationFromEntry(const FName& KeyName, FVector& ResultLocation) const
{
	if (WorldState.IsValid())
	{
		return WorldState->GetLocationFromEntry(KeyName, ResultLocation);
	}
	else
	{
		const UHTNComponent* const Owner = GetOwner();
		if (ensure(Owner))
		{
			if (const UBlackboardComponent* const BlackboardComponent = Owner->GetBlackboardComponent())
			{
				return BlackboardComponent->GetLocationFromEntry(KeyName, ResultLocation);
			}
		}
	}

	return false;
}

bool UWorldStateProxy::GetRotationFromEntry(const FName& KeyName, FRotator& ResultRotation) const
{
	if (WorldState.IsValid())
	{
		return WorldState->GetRotationFromEntry(KeyName, ResultRotation);
	}
	else
	{
		const UHTNComponent* const Owner = GetOwner();
		if (ensure(Owner))
		{
			if (const UBlackboardComponent* const BlackboardComponent = Owner->GetBlackboardComponent())
			{
				return BlackboardComponent->GetRotationFromEntry(KeyName, ResultRotation);
			}
		}
	}

	return false;
}

void UWorldStateProxy::ClearValue(const FName& KeyName)
{
	if (ensureMsgf(bIsEditable, TEXT("Trying to set value on a non-editable WorldState!")))
	{
		if (WorldState.IsValid())
		{
			WorldState->ClearValue(KeyName);
		}
		else
		{
			UHTNComponent* const Owner = GetOwner();
			if (ensure(Owner))
			{
				if (UBlackboardComponent* const BlackboardComponent = Owner->GetBlackboardComponent())
				{
					BlackboardComponent->ClearValue(KeyName);
				}
			}
		}
	}
}

bool UWorldStateProxy::CopyKeyValue(const FName& SourceKeyName, const FName& TargetKeyName)
{
	return CopyKeyValue(GetKeyID(SourceKeyName), GetKeyID(TargetKeyName));
}

bool UWorldStateProxy::CopyKeyValue(FBlackboard::FKey SourceKeyID, FBlackboard::FKey TargetKeyID)
{
	if (ensureMsgf(bIsEditable, TEXT("Trying to set value on a non-editable WorldState!")))
	{
		if (WorldState.IsValid())
		{
			return WorldState->CopyKeyValue(SourceKeyID, TargetKeyID);
		}

		UHTNComponent* const Owner = GetOwner();
		if (ensure(Owner))
		{
			if (UBlackboardComponent* const BlackboardComponent = Owner->GetBlackboardComponent())
			{
				return BlackboardComponent->CopyKeyValue(SourceKeyID, TargetKeyID);
			}
		}
	}

	return false;
}

FString UWorldStateProxy::DescribeKeyValue(FBlackboard::FKey KeyID, EBlackboardDescription::Type Mode) const
{
	if (WorldState.IsValid())
	{
		return WorldState->DescribeKeyValue(KeyID, Mode);
	}

	const UHTNComponent* const Owner = GetOwner();
	if (ensure(Owner))
	{
		if (const UBlackboardComponent* const BlackboardComponent = Owner->GetBlackboardComponent())
		{
			return BlackboardComponent->DescribeKeyValue(KeyID, Mode);
		}
	}

	return FString();
}

FName UWorldStateProxy::GetKeyName(FBlackboard::FKey KeyID) const
{
	const UHTNComponent* const Owner = GetOwner();
	if (ensure(Owner))
	{
		if (const UBlackboardComponent* const BlackboardComponent = Owner->GetBlackboardComponent())
		{
			return BlackboardComponent->GetKeyName(KeyID);
		}
	}

	return NAME_None;
}

FString UWorldStateProxy::DescribeKeyValue(const FName& KeyName, EBlackboardDescription::Type Mode) const
{
	return DescribeKeyValue(GetKeyID(KeyName), Mode);
}

FBlackboard::FKey UWorldStateProxy::GetKeyID(const FName& KeyName) const
{
	const UHTNComponent* const Owner = GetOwner();
	if (ensure(Owner))
	{
		if (const UBlackboardComponent* const BlackboardComponent = Owner->GetBlackboardComponent())
		{
			return BlackboardComponent->GetKeyID(KeyName);
		}
	}

	return FBlackboard::InvalidKey;
}

FGuardWorldStateProxy::FGuardWorldStateProxy(UWorldStateProxy& Proxy) :
	Proxy(Proxy),
	PrevWorldState(Proxy.WorldState),
	bPrevIsEditable(Proxy.bIsEditable)
{}

FGuardWorldStateProxy::FGuardWorldStateProxy(UWorldStateProxy& Proxy, TSharedPtr<FBlackboardWorldState> WorldState, bool bIsEditable) :
	FGuardWorldStateProxy(Proxy)
{
	Proxy.WorldState = WorldState;
	Proxy.bIsEditable = bIsEditable;
}

FGuardWorldStateProxy::~FGuardWorldStateProxy()
{
	Proxy.WorldState = PrevWorldState;
	Proxy.bIsEditable = bPrevIsEditable;
}
