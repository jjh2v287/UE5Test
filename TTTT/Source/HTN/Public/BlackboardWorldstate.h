// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyAllTypes.h"
#include "UObject/GCObject.h"
#include "HTNTypes.h"

class FBlackboardWorldStateImpl;

// Stores Blackboard values the same way as a BlackboardComponent, but is cheap to copy since it's not a UObject.
// Used to model future states during planning. Also keeps track of which keys were changed since the object's creation.
// Note: to work, it requires the original BlackboardComponent to be alive, 
// so make sure all worldstates are deallocated before their BlackboardCompoent is.
class HTN_API FBlackboardWorldState final : public FGCObject
{	
public:	
	// For internal use only
	FBlackboardWorldState();
	
	FBlackboardWorldState(class UBlackboardComponent& Blackboard);
	virtual ~FBlackboardWorldState();

	// FGCObject implementation
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override;
	// End FGCObject implementation
	
	TSharedRef<FBlackboardWorldState> MakeNext() const;
	void ApplyChangedValues(UBlackboardComponent& BlackboardComponent) const;
	void ApplyChangedValues(FBlackboardWorldState& OtherWorldstate) const;
	void CopyValue(UBlackboardComponent& TargetBlackboard, FBlackboard::FKey KeyID) const;
	void CopyValue(FBlackboardWorldState& TargetWorldstate, FBlackboard::FKey KeyID) const;

	// For vector keys returns the vector itself.
	// For actor keys returns the actor's location and the actor itself.
	// For component keys returns the component's location, its actor, and the component itself.
	FVector GetLocation(const FBlackboardKeySelector& KeySelector, AActor** OutActor = nullptr, UActorComponent** OutComponent = nullptr) const;
	
	template<class TDataClass>
	typename TDataClass::FDataType GetValue(const FName& KeyName) const;

	template<class TDataClass>
	typename TDataClass::FDataType GetValue(FBlackboard::FKey KeyID) const;
	
	template<class TDataClass>
	bool SetValue(const FName& KeyName, typename TDataClass::FDataType Value);

	template<class TDataClass>
	bool SetValue(FBlackboard::FKey KeyID, typename TDataClass::FDataType Value);

	FORCEINLINE void ClearValue(const FName& KeyName) { ClearValue(GetKeyID(KeyName)); }
	void ClearValue(FBlackboard::FKey KeyID);

	// Copies the content from SourceKey to TargetKey and returns true if it worked
	bool CopyKeyValue(FBlackboard::FKey SourceKeyID, FBlackboard::FKey TargetKeyID);

	bool TestBasicOperation(const UBlackboardKeyType& Key, FBlackboard::FKey KeyID, EBasicKeyOperation::Type Type) const;
	bool TestArithmeticOperation(const UBlackboardKeyType& Key, FBlackboard::FKey KeyID, EArithmeticKeyOperation::Type Type, int32 IntValue, float FloatValue) const;
	bool TestTextOperation(const UBlackboardKeyType& Key, FBlackboard::FKey KeyID, ETextKeyOperation::Type Type, const FString& StringValue) const;

	bool IsVectorValueSet(const FName& Name) const;
	bool IsVectorValueSet(FBlackboard::FKey KeyID) const;

	bool GetLocationFromEntry(const FName& KeyName, FVector& ResultLocation) const;
	bool GetLocationFromEntry(FBlackboard::FKey KeyID, FVector& ResultLocation) const;

	bool GetRotationFromEntry(const FName& KeyName, FRotator& ResultRotation) const;
	bool GetRotationFromEntry(FBlackboard::FKey KeyID, FRotator& ResultRotation) const;
	
	FORCEINLINE uint8* GetKeyRawData(const FName& KeyName) { return GetKeyRawData(GetKeyID(KeyName)); }
	uint8* GetKeyRawData(FBlackboard::FKey KeyID);

	FORCEINLINE const uint8* GetKeyRawData(const FName& KeyName) const { return GetKeyRawData(GetKeyID(KeyName)); }
	const uint8* GetKeyRawData(FBlackboard::FKey KeyID) const;

	FORCEINLINE bool IsValidKey(FBlackboard::FKey KeyID) const { check(BlackboardAsset.IsValid()); return KeyID != FBlackboard::InvalidKey && BlackboardAsset->Keys.IsValidIndex(KeyID); }
	FORCEINLINE	FName GetKeyName(FBlackboard::FKey KeyID) const { return BlackboardAsset.IsValid() ? BlackboardAsset->GetKeyName(KeyID) : NAME_None; }
	FORCEINLINE FBlackboard::FKey GetKeyID(const FName& KeyName) const { return BlackboardAsset.IsValid() ? BlackboardAsset->GetKeyID(KeyName) : FBlackboard::InvalidKey; }

	FString DescribeKeyValue(const FName& KeyName, EBlackboardDescription::Type Type) const;
	FString DescribeKeyValue(FBlackboard::FKey KeyID, EBlackboardDescription::Type Mode) const;
	
	bool WasKeyChanged(FBlackboard::FKey KeyID) const;
	bool HasAnyKeyChanged() const;
	
	bool IsCompatible(const FBlackboardWorldState& Other) const;

private:
	friend FBlackboardWorldStateImpl;
	
	void SetKeyChanged(FBlackboard::FKey KeyID, bool bWasChanged = true);
	void DestroyValues();

	TWeakObjectPtr<class UBlackboardComponent> BlackboardComponent;
	TWeakObjectPtr<class UBlackboardData> BlackboardAsset;

	TArray<uint8, TInlineAllocator<256>> ValueMemory;

	// The key instances are gc-owned by the UBlackboardComponent
	TArray<TObjectPtr<UBlackboardKeyType>> KeyInstances;

	// Whether or not a given key was changed on this worldstate.
	TBitArray<> ChangedFlags;

	bool bIsInitialized : 1;
};

template <class TDataClass>
typename TDataClass::FDataType FBlackboardWorldState::GetValue(const FName& KeyName) const
{
	const FBlackboard::FKey KeyID = GetKeyID(KeyName);
	return GetValue<TDataClass>(KeyID);
}

template <class TDataClass>
typename TDataClass::FDataType FBlackboardWorldState::GetValue(FBlackboard::FKey KeyID) const
{
	const FBlackboardEntry* const EntryInfo = BlackboardAsset.IsValid() ? BlackboardAsset->GetKey(KeyID) : nullptr;
	if (!EntryInfo || !EntryInfo->KeyType || EntryInfo->KeyType->GetClass() != TDataClass::StaticClass())
	{
		return TDataClass::InvalidValue;
	}

	UBlackboardKeyType* const KeyOb = EntryInfo->KeyType->HasInstance() ? KeyInstances[KeyID] : EntryInfo->KeyType;
	const uint16 DataOffset = EntryInfo->KeyType->HasInstance() ? sizeof(FBlackboardInstancedKeyMemory) : 0;

	const uint8* const RawData = GetKeyRawData(KeyID) + DataOffset;
	return RawData ? TDataClass::GetValue(StaticCast<TDataClass*>(KeyOb), RawData) : TDataClass::InvalidValue;
}

template <class TDataClass>
bool FBlackboardWorldState::SetValue(const FName& KeyName, typename TDataClass::FDataType Value)
{
	const FBlackboard::FKey KeyID = GetKeyID(KeyName);
	return SetValue<TDataClass>(KeyID, Value);
}

template <class TDataClass>
bool FBlackboardWorldState::SetValue(FBlackboard::FKey KeyID, typename TDataClass::FDataType Value)
{
	const FBlackboardEntry* EntryInfo = BlackboardAsset.IsValid() ? BlackboardAsset->GetKey(KeyID) : nullptr;
	if (!EntryInfo || !EntryInfo->KeyType || EntryInfo->KeyType->GetClass() != TDataClass::StaticClass())
	{
		return false;
	}

	const uint16 DataOffset = EntryInfo->KeyType->HasInstance() ? sizeof(FBlackboardInstancedKeyMemory) : 0;
	if (uint8* const RawData = GetKeyRawData(KeyID) + DataOffset)
	{
		UBlackboardKeyType* const KeyOb = EntryInfo->KeyType->HasInstance() ? KeyInstances[KeyID] : EntryInfo->KeyType;
		TDataClass::SetValue(StaticCast<TDataClass*>(KeyOb), RawData, Value);
		// Intentionally marking the key as changed even though it might have been set to the same value it had before.
		SetKeyChanged(KeyID);
		
		return true;
	}
	

	return false;
}

FORCEINLINE bool FBlackboardWorldState::IsVectorValueSet(const FName& KeyName) const
{
	return IsVectorValueSet(GetKeyID(KeyName));
}

FORCEINLINE bool FBlackboardWorldState::GetLocationFromEntry(const FName& KeyName, FVector& ResultLocation) const
{
	return GetLocationFromEntry(GetKeyID(KeyName), ResultLocation);
}

FORCEINLINE bool FBlackboardWorldState::GetRotationFromEntry(const FName& KeyName, FRotator& ResultRotation) const
{
	return GetRotationFromEntry(GetKeyID(KeyName), ResultRotation);
}