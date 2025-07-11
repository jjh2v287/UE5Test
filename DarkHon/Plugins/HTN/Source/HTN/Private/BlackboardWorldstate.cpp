// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "BlackboardWorldstate.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "AITypes.h"

#include "HTNTypes.h"

namespace
{
	struct UBlackboardKeyTypeHelper : public UBlackboardKeyType
	{
		static void CopyValuesHelper(UBlackboardKeyType* DestinationKey, UBlackboardComponent& OwnerComp, uint8* DestinationMemory, const UBlackboardKeyType* SourceKey, const uint8* SourceMemory)
		{
			if (ensure(DestinationKey))
			{
				StaticCast<UBlackboardKeyTypeHelper*>(DestinationKey)->CopyValues(OwnerComp, DestinationMemory, SourceKey, SourceMemory);
			}
		}

		static void InitializeMemoryHelper(UBlackboardKeyType* Key, UBlackboardComponent& OwnerComp, uint8* MemoryBlock)
		{
			if (ensure(Key))
			{
				StaticCast<UBlackboardKeyTypeHelper*>(Key)->InitializeMemory(OwnerComp, MemoryBlock);
			}
		}
		
		static void FreeMemoryHelper(UBlackboardKeyType* Key, UBlackboardComponent& OwnerComp, uint8* MemoryBlock)
		{
			if (ensure(Key))
			{
				StaticCast<UBlackboardKeyTypeHelper*>(Key)->FreeMemory(OwnerComp, MemoryBlock);
			}
		}

		FORCEINLINE static bool TestBasicOperationHelper(const UBlackboardKeyType& KeyInstance, const UBlackboardComponent& OwnerComp, const uint8* MemoryBlock, EBasicKeyOperation::Type Type)
		{
			return StaticCast<const UBlackboardKeyTypeHelper*>(&KeyInstance)->TestBasicOperation(OwnerComp, MemoryBlock, Type);
		}

		FORCEINLINE static bool TestArithmeticOperationHelper(const UBlackboardKeyType& KeyInstance, const UBlackboardComponent& OwnerComp, const uint8* MemoryBlock, EArithmeticKeyOperation::Type Type, int32 IntValue, float FloatValue)
		{
			return StaticCast<const UBlackboardKeyTypeHelper*>(&KeyInstance)->TestArithmeticOperation(OwnerComp, MemoryBlock, Type, IntValue, FloatValue);
		}
		
		FORCEINLINE static bool TestTextOperationHelper(const UBlackboardKeyType& KeyInstance, const UBlackboardComponent& OwnerComp, const uint8* MemoryBlock, ETextKeyOperation::Type Type, const FString& StringValue)
		{
			return StaticCast<const UBlackboardKeyTypeHelper*>(&KeyInstance)->TestTextOperation(OwnerComp, MemoryBlock, Type, StringValue);
		}

		static UBlackboardKeyType* MakeInstance(UBlackboardKeyType* const SourceKey, UBlackboardComponent& BlackboardComponent)
		{
			UBlackboardKeyType* const Instance = DuplicateObject(SourceKey, &BlackboardComponent);
			if (!ensure(Instance))
			{
				return nullptr;
			}
			StaticCast<UBlackboardKeyTypeHelper*>(Instance)->bIsInstanced = true;
			return Instance;
		}
	};
	
	// A utility for exposing some protected members of UBlackboardComponent
	struct UBlackboardComponentHelper : public UBlackboardComponent
	{	
		FORCEINLINE static const TArray<uint8>& GetValueMemory(const UBlackboardComponent& BlackboardComponent)
		{
			return StaticCast<const UBlackboardComponentHelper*>(&BlackboardComponent)->ValueMemory;
		}

		FORCEINLINE static TArray<uint8>& GetValueMemory(UBlackboardComponent& BlackboardComponent)
		{
			return StaticCast<UBlackboardComponentHelper*>(&BlackboardComponent)->ValueMemory;
		}
		
		FORCEINLINE static const TArray<uint16>& GetValueMemoryOffsets(const UBlackboardComponent& BlackboardComponent)
		{
			return StaticCast<const UBlackboardComponentHelper*>(&BlackboardComponent)->ValueOffsets;
		}

		FORCEINLINE static const TArray<TObjectPtr<UBlackboardKeyType>>& GetKeyInstances(const UBlackboardComponent& BlackboardComponent)
		{
			return StaticCast<const UBlackboardComponentHelper*>(&BlackboardComponent)->KeyInstances;
		}

		FORCEINLINE static TArray<TObjectPtr<UBlackboardKeyType>>& GetKeyInstances(UBlackboardComponent& BlackboardComponent)
		{
			return StaticCast<UBlackboardComponentHelper*>(&BlackboardComponent)->KeyInstances;
		}

		static void OnValueChanged(UBlackboardComponent* BlackboardComponent, FBlackboard::FKey KeyID, const FBlackboardEntry& Entry, UBlackboardKeyType* Key, uint16 DataOffset, const uint8* SourceValueMemory)
		{
			UBlackboardComponentHelper* const BB = StaticCast<UBlackboardComponentHelper*>(BlackboardComponent);
			if (!BB)
			{
				return;
			}
			
			BB->NotifyObservers(KeyID);

			UBlackboardData* const BBAsset = BB->GetBlackboardAsset();
			if (BBAsset && BBAsset->HasSynchronizedKeys() && BB->IsKeyInstanceSynced(KeyID))
			{
				UAISystem* const AISystem = UAISystem::GetCurrentSafe(BB->GetWorld());
				for (auto Iter = AISystem->CreateBlackboardDataToComponentsIterator(*BBAsset); Iter; ++Iter)
				{
					UBlackboardComponentHelper* const OtherBB = StaticCast<UBlackboardComponentHelper*>(Iter.Value());
					if (OtherBB && BB->ShouldSyncWithBlackboard(*OtherBB))
					{
						UBlackboardData* const OtherBlackboardAsset = OtherBB->GetBlackboardAsset();
						const FBlackboard::FKey OtherKeyID = OtherBlackboardAsset ? OtherBlackboardAsset->GetKeyID(Entry.EntryName) : FBlackboard::InvalidKey;
						if (OtherKeyID != FBlackboard::InvalidKey)
						{
							UBlackboardKeyType* const OtherKey = Entry.KeyType->HasInstance() ? OtherBB->KeyInstances[OtherKeyID] : Entry.KeyType;
							uint8* const OtherRawData = OtherBB->GetKeyRawData(OtherKeyID) + DataOffset;

							UBlackboardKeyTypeHelper::CopyValuesHelper(OtherKey, *BlackboardComponent, OtherRawData, Key, SourceValueMemory);

							OtherBB->NotifyObservers(OtherKeyID);
						}
					}
				}
			}
		}
	};
}

class FBlackboardWorldStateImpl
{
public:
	
	template<typename SourceType>
	static void InitializeKeys(FBlackboardWorldState& WorldState, const SourceType& Source)
	{
		check(WorldState.BlackboardComponent.IsValid());
		check(WorldState.BlackboardAsset.IsValid());
		check(!WorldState.bIsInitialized);
		
		const auto& SourceMemory = GetValueMemory(Source);
		const TArray<TObjectPtr<UBlackboardKeyType>>& SourceKeyInstances = GetKeyInstances(Source);
		UBlackboardComponent& BlackboardComponent = *WorldState.BlackboardComponent;

		WorldState.ValueMemory.AddZeroed(SourceMemory.Num());
		WorldState.KeyInstances.AddZeroed(SourceKeyInstances.Num());
		for (UBlackboardData* It = WorldState.BlackboardAsset.Get(); It; It = It->Parent)
		{
			for (int32 KeyIndex = 0; KeyIndex < It->Keys.Num(); ++KeyIndex)
			{
				if (UBlackboardKeyType* const KeyType = It->Keys[KeyIndex].KeyType)
				{
					KeyType->PreInitialize(BlackboardComponent);
					
					const bool bKeyHasInstance = KeyType->HasInstance();
					const uint16 MemoryOffset = bKeyHasInstance ? sizeof(FBlackboardInstancedKeyMemory) : 0;
					const FBlackboard::FKey KeyID = KeyIndex + StaticCast<int32>(It->GetFirstKeyID());

					const uint8* const SourceValueMemory = GetKeyRawDataConst(SourceMemory, BlackboardComponent, KeyID) + MemoryOffset;
					UBlackboardKeyType* const SourceKey = bKeyHasInstance ? SourceKeyInstances[KeyID].Get() : KeyType;
					
					uint8* const DestinationRawMemory = GetKeyRawData(WorldState.ValueMemory, BlackboardComponent, KeyID);
					uint8* const DestinationValueMemory = DestinationRawMemory + MemoryOffset;
					UBlackboardKeyType* DestinationKey = KeyType;
					if (bKeyHasInstance)
					{
						DestinationKey = UBlackboardKeyTypeHelper::MakeInstance(SourceKey, BlackboardComponent);
						reinterpret_cast<FBlackboardInstancedKeyMemory*>(DestinationRawMemory)->KeyIdx = KeyID;
						WorldState.KeyInstances[KeyID] = DestinationKey;
					}
					UBlackboardKeyTypeHelper::InitializeMemoryHelper(DestinationKey, BlackboardComponent, DestinationValueMemory);

					UBlackboardKeyTypeHelper::CopyValuesHelper(DestinationKey, BlackboardComponent, DestinationValueMemory, SourceKey, SourceValueMemory);
				}
			}
		}

		WorldState.bIsInitialized = true;
	}

	template<typename DestinationType>
	static void ApplyChangedValues(const FBlackboardWorldState& WorldState, DestinationType& Destination)
	{
		check(WorldState.BlackboardComponent.IsValid());
		check(WorldState.BlackboardAsset.IsValid());

		auto& DestinationMemory = GetValueMemory(Destination);
		const TArray<TObjectPtr<UBlackboardKeyType>>& DestinationKeyInstances = GetKeyInstances(Destination);
		UBlackboardComponent& BlackboardComponent = *WorldState.BlackboardComponent;
		
		for (int32 KeyIndex = 0; KeyIndex < WorldState.ChangedFlags.Num(); ++KeyIndex)
		{
			const FBlackboard::FKey KeyID(KeyIndex);
			if (WorldState.ChangedFlags[KeyID])
			{
				if (const FBlackboardEntry* const Entry = WorldState.BlackboardAsset->GetKey(KeyID))
				{
					if (UBlackboardKeyType* const KeyType = Entry->KeyType)
					{
						const bool bKeyHasInstance = KeyType->HasInstance();
						const uint16 MemoryOffset = bKeyHasInstance ? sizeof(FBlackboardInstancedKeyMemory) : 0;

						const uint8* const SourceValueMemory = GetKeyRawDataConst(WorldState.ValueMemory, BlackboardComponent, KeyID) + MemoryOffset;
						uint8* const DestinationValueMemory = GetKeyRawData(DestinationMemory, BlackboardComponent, KeyID) + MemoryOffset;
						
						UBlackboardKeyType* const SourceKey = bKeyHasInstance ? WorldState.KeyInstances[KeyID].Get() : KeyType;
						UBlackboardKeyType* const DestinationKey = bKeyHasInstance ? DestinationKeyInstances[KeyID].Get() : KeyType;
						UBlackboardKeyTypeHelper::CopyValuesHelper(DestinationKey, BlackboardComponent, DestinationValueMemory, SourceKey, SourceValueMemory);
						NotifyValueChanged(Destination, KeyID, *Entry, DestinationKey, MemoryOffset, DestinationValueMemory);
					}
				}
			}
		}
	}

	template<typename DestinationType>
	static void CopyValueFromWorldstate(const FBlackboardWorldState& WorldState, DestinationType& Destination, FBlackboard::FKey KeyID)
	{
		check(WorldState.BlackboardComponent.IsValid());
		check(WorldState.BlackboardAsset.IsValid());

		auto& DestinationMemory = GetValueMemory(Destination);
		const TArray<TObjectPtr<UBlackboardKeyType>>& DestinationKeyInstances = GetKeyInstances(Destination);
		UBlackboardComponent& BlackboardComponent = *WorldState.BlackboardComponent;

		if (const FBlackboardEntry* const Entry = WorldState.BlackboardAsset->GetKey(KeyID))
		{
			if (UBlackboardKeyType* const KeyType = Entry->KeyType)
			{
				const bool bKeyHasInstance = KeyType->HasInstance();
				const uint16 MemoryOffset = bKeyHasInstance ? sizeof(FBlackboardInstancedKeyMemory) : 0;

				const uint8* const SourceValueMemory = GetKeyRawDataConst(WorldState.ValueMemory, BlackboardComponent, KeyID) + MemoryOffset;
				uint8* const DestinationValueMemory = GetKeyRawData(DestinationMemory, BlackboardComponent, KeyID) + MemoryOffset;

				UBlackboardKeyType* const SourceKey = bKeyHasInstance ? WorldState.KeyInstances[KeyID].Get() : KeyType;
				UBlackboardKeyType* const DestinationKey = bKeyHasInstance ? DestinationKeyInstances[KeyID].Get() : KeyType;
				UBlackboardKeyTypeHelper::CopyValuesHelper(DestinationKey, BlackboardComponent, DestinationValueMemory, SourceKey, SourceValueMemory);
				NotifyValueChanged(Destination, KeyID, *Entry, DestinationKey, MemoryOffset, DestinationValueMemory);
				SetKeyChanged(Destination, KeyID);
			}
		}
	}
	
private:

	template<typename ValueMemoryArrayType>
	static const uint8* GetKeyRawDataConst(const ValueMemoryArrayType& ValueMemory, const UBlackboardComponent& Blackboard, FBlackboard::FKey KeyID)
	{
		if (ValueMemory.Num())
		{
			const TArray<uint16>& MemoryOffsets = UBlackboardComponentHelper::GetValueMemoryOffsets(Blackboard);
			if (MemoryOffsets.IsValidIndex(KeyID))
			{
				check(ValueMemory.IsValidIndex(MemoryOffsets[KeyID]));
				return ValueMemory.GetData() + MemoryOffsets[KeyID];
			}
		}

		checkNoEntry();
		return nullptr;
	}

	template<typename ValueMemoryArrayType>
	static uint8* GetKeyRawData(ValueMemoryArrayType& ValueMemory, const UBlackboardComponent& Blackboard, FBlackboard::FKey KeyID)
	{
		if (ValueMemory.Num())
		{
			const TArray<uint16>& MemoryOffsets = UBlackboardComponentHelper::GetValueMemoryOffsets(Blackboard);
			if (MemoryOffsets.IsValidIndex(KeyID))
			{
				check(ValueMemory.IsValidIndex(MemoryOffsets[KeyID]));
				return ValueMemory.GetData() + MemoryOffsets[KeyID];
			}
		}

		checkNoEntry();
		return nullptr;
	}

	FORCEINLINE static const TArray<uint8>& GetValueMemory(const UBlackboardComponent& BlackboardComponent)
	{
		return UBlackboardComponentHelper::GetValueMemory(BlackboardComponent);
	}

	FORCEINLINE static TArray<uint8>& GetValueMemory(UBlackboardComponent& BlackboardComponent)
	{
		return UBlackboardComponentHelper::GetValueMemory(BlackboardComponent);
	}

	FORCEINLINE static const auto& GetValueMemory(const FBlackboardWorldState& WorldState)
	{
		return WorldState.ValueMemory;
	}

	FORCEINLINE static auto& GetValueMemory(FBlackboardWorldState& WorldState)
	{
		return WorldState.ValueMemory;
	}

	FORCEINLINE static const TArray<TObjectPtr<UBlackboardKeyType>>& GetKeyInstances(const UBlackboardComponent& BlackboardComponent)
	{
		return UBlackboardComponentHelper::GetKeyInstances(BlackboardComponent);
	}

	FORCEINLINE static TArray<TObjectPtr<UBlackboardKeyType>>& GetKeyInstances(UBlackboardComponent& BlackboardComponent)
	{
		return UBlackboardComponentHelper::GetKeyInstances(BlackboardComponent);
	}

	FORCEINLINE static const TArray<TObjectPtr<UBlackboardKeyType>>& GetKeyInstances(const FBlackboardWorldState& WorldState)
	{
		return WorldState.KeyInstances;
	}

	FORCEINLINE static TArray<TObjectPtr<UBlackboardKeyType>>& GetKeyInstances(FBlackboardWorldState& WorldState)
	{
		return WorldState.KeyInstances;
	}

	FORCEINLINE static void NotifyValueChanged(UBlackboardComponent& BlackboardComponent, FBlackboard::FKey KeyID, const FBlackboardEntry& Entry, UBlackboardKeyType* DestinationKey, uint16 MemoryOffset, const uint8* SourceValueMemory)
	{
		UBlackboardComponentHelper::OnValueChanged(&BlackboardComponent, KeyID, Entry, DestinationKey, MemoryOffset, SourceValueMemory);
	}

	FORCEINLINE static void NotifyValueChanged(FBlackboardWorldState& WorldState, FBlackboard::FKey KeyID, const FBlackboardEntry& Entry, UBlackboardKeyType* DestinationKey, uint16 MemoryOffset, const uint8* SourceValueMemory)
	{
		// No need to notify FBlackboardWorldState
	}

	FORCEINLINE static void SetKeyChanged(UBlackboardComponent& BlackboardComponent, FBlackboard::FKey KeyID)
	{
		// Not available on BlackboardComponent.
	}

	FORCEINLINE static void SetKeyChanged(FBlackboardWorldState& WorldState, FBlackboard::FKey KeyID)
	{
		WorldState.SetKeyChanged(KeyID);
	}
};

FBlackboardWorldState::FBlackboardWorldState() :
	bIsInitialized(false)
{}

FBlackboardWorldState::FBlackboardWorldState(UBlackboardComponent& Blackboard) :
	BlackboardComponent(&Blackboard),
	BlackboardAsset(Blackboard.GetBlackboardAsset()),
	bIsInitialized(false)
{
	check(BlackboardComponent.IsValid());
	check(BlackboardAsset.IsValid());
	
	FBlackboardWorldStateImpl::InitializeKeys(*this, Blackboard);
}

FBlackboardWorldState::~FBlackboardWorldState()
{
	DestroyValues();
}

void FBlackboardWorldState::AddReferencedObjects(FReferenceCollector& Collector)
{
	if (bIsInitialized)
	{
		Collector.AddReferencedObjects(KeyInstances);
	}
}

FString FBlackboardWorldState::GetReferencerName() const
{
	return TEXT("FBlackboardWorldState");
}

TSharedRef<FBlackboardWorldState> FBlackboardWorldState::MakeNext() const
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("FBlackboardWorldState::MakeNext"), STAT_AI_HTN_WorldStateMakeNext, STATGROUP_AI_HTN);
	
	check(BlackboardComponent.IsValid());
	check(BlackboardAsset.IsValid());
	
	const TSharedRef<FBlackboardWorldState> NextWorldstate = MakeShared<FBlackboardWorldState>();
	NextWorldstate->BlackboardComponent = BlackboardComponent;
	NextWorldstate->BlackboardAsset = BlackboardAsset;
	FBlackboardWorldStateImpl::InitializeKeys(*NextWorldstate, *this);
	
	return NextWorldstate;
}

void FBlackboardWorldState::ApplyChangedValues(UBlackboardComponent& Blackboard) const
{
	FBlackboardWorldStateImpl::ApplyChangedValues(*this, Blackboard);
}

void FBlackboardWorldState::ApplyChangedValues(FBlackboardWorldState& OtherWorldstate) const
{
	if (&OtherWorldstate != this)
	{
		FBlackboardWorldStateImpl::ApplyChangedValues(*this, OtherWorldstate);
	}
}

void FBlackboardWorldState::CopyValue(UBlackboardComponent& TargetBlackboard, FBlackboard::FKey KeyID) const
{
	FBlackboardWorldStateImpl::CopyValueFromWorldstate(*this, TargetBlackboard, KeyID);
}

void FBlackboardWorldState::CopyValue(FBlackboardWorldState& TargetWorldstate, FBlackboard::FKey KeyID) const
{
	if (&TargetWorldstate != this)
	{
		FBlackboardWorldStateImpl::CopyValueFromWorldstate(*this, TargetWorldstate, KeyID);
	}
}

FString FBlackboardWorldState::DescribeKeyValue(const FName& KeyName, EBlackboardDescription::Type Type) const
{
	return DescribeKeyValue(GetKeyID(KeyName), Type);
}

FString FBlackboardWorldState::DescribeKeyValue(FBlackboard::FKey KeyID, EBlackboardDescription::Type Mode) const
{
	if (!BlackboardComponent.IsValid() || !BlackboardAsset.IsValid())
	{
		return FString();
	}

	FString Description;
	
	if (const FBlackboardEntry* Key = BlackboardAsset->GetKey(KeyID))
	{
		const uint8* const ValueData = GetKeyRawData(KeyID);
		const FString ValueDesc = Key->KeyType && ValueData ?
			*Key->KeyType->WrappedDescribeValue(*BlackboardComponent, ValueData) :
			TEXT("empty");

		if (Mode == EBlackboardDescription::OnlyValue)
		{
			Description = ValueDesc;
		}
		else if (Mode == EBlackboardDescription::KeyWithValue)
		{
			Description = FString::Printf(TEXT("%s: %s"), *Key->EntryName.ToString(), *ValueDesc);
		}
		else // Mode == EBlackboardDescription::DetailedKeyWithValue
		{
			const FString CommonTypePrefix = UBlackboardKeyType::StaticClass()->GetName().AppendChar(TEXT('_'));
			const FString FullKeyType = Key->KeyType ? GetNameSafe(Key->KeyType->GetClass()) : FString();
			const FString DescKeyType = FullKeyType.StartsWith(CommonTypePrefix) ? FullKeyType.RightChop(CommonTypePrefix.Len()) : FullKeyType;

			Description = FString::Printf(TEXT("%s [%s]: %s"), *Key->EntryName.ToString(), *DescKeyType, *ValueDesc);
		}
	}

	return Description;
}

bool FBlackboardWorldState::WasKeyChanged(FBlackboard::FKey KeyID) const
{
	return ChangedFlags.IsValidIndex(KeyID) && ChangedFlags[KeyID];
}

bool FBlackboardWorldState::HasAnyKeyChanged() const
{
	if (ensure(BlackboardAsset.IsValid()))
	{
		for (UBlackboardData* It = BlackboardAsset.Get(); It; It = It->Parent)
		{
			const FBlackboard::FKey FirstKeyIndex = It->GetFirstKeyID();
			const int32 EndIndex = StaticCast<int32>(FirstKeyIndex) + It->Keys.Num();
			for (int32 KeyIndex = FirstKeyIndex; KeyIndex < EndIndex; ++KeyIndex)
			{
				const FBlackboard::FKey KeyID(KeyIndex);
				if (WasKeyChanged(KeyID))
				{
					return true;
				}
			}
		}
	}

	return false;
}

void FBlackboardWorldState::SetKeyChanged(FBlackboard::FKey KeyID, bool bWasChanged)
{
	if (!ChangedFlags.IsValidIndex(KeyID))
	{
		ChangedFlags.Add(false, StaticCast<int32>(KeyID) + 1 - ChangedFlags.Num());
	}
	ChangedFlags[KeyID] = bWasChanged;
}

bool FBlackboardWorldState::IsCompatible(const FBlackboardWorldState& Other) const
{
	return BlackboardComponent == Other.BlackboardComponent && 
		BlackboardAsset == Other.BlackboardAsset && 
		BlackboardComponent.IsValid() && 
		BlackboardAsset.IsValid();
}

void FBlackboardWorldState::DestroyValues()
{
	if (!ensureMsgf(BlackboardComponent.IsValid() || !BlackboardAsset.IsValid(), TEXT("Could not destroy key values in a worldstate because the original blackboard component is no longer valid.")))
	{
		return;
	}

	for (UBlackboardData* It = BlackboardAsset.Get(); It; It = It->Parent)
	{
		for (int32 KeyIndex = 0; KeyIndex < It->Keys.Num(); KeyIndex++)
		{
			if (UBlackboardKeyType* const KeyType = It->Keys[KeyIndex].KeyType)
			{
				const FBlackboard::FKey KeyID = KeyIndex + StaticCast<int32>(It->GetFirstKeyID());
				uint8* const KeyValueRawMemory = GetKeyRawData(KeyID);
				if (ensure(KeyValueRawMemory))
				{
					uint8* const KeyValueMemory = KeyType->HasInstance() ? 
						KeyValueRawMemory + sizeof(FBlackboardInstancedKeyMemory) :
						KeyValueRawMemory;
					
					UBlackboardKeyType* const Key = KeyType->HasInstance() ? KeyInstances[KeyID].Get() : KeyType;
					if (ensure(Key))
					{
						UBlackboardKeyTypeHelper::FreeMemoryHelper(Key, *BlackboardComponent, KeyValueMemory);
					}
				}
			}
		}
	}

	ValueMemory.Reset();
	KeyInstances.Reset();
}

void FBlackboardWorldState::ClearValue(FBlackboard::FKey KeyID)
{
	check(BlackboardComponent.IsValid());
	check(BlackboardAsset.IsValid());
	
	if (const FBlackboardEntry* const EntryInfo = BlackboardAsset->GetKey(KeyID))
	{
		if (const UBlackboardKeyType* const KeyType = EntryInfo->KeyType)
		{
			if (uint8* const RawData = GetKeyRawData(KeyID))
			{
				KeyType->WrappedClear(*BlackboardComponent, RawData);
				SetKeyChanged(KeyID);
			}
		}
	}
}

bool FBlackboardWorldState::CopyKeyValue(FBlackboard::FKey SourceKeyID, FBlackboard::FKey TargetKeyID)
{
	if (!BlackboardComponent.IsValid() || !BlackboardAsset.IsValid())
	{
		return false;
	}

	// Copy only when values are initialized
	if (ValueMemory.Num() == 0)
	{
		return false;
	}

	// No source key
	const FBlackboardEntry* const SourceValueEntryInfo = BlackboardAsset->GetKey(SourceKeyID);
	if (!SourceValueEntryInfo || !SourceValueEntryInfo->KeyType)
	{
		return false;
	}

	// No target key
	const FBlackboardEntry* const TargetValueEntryInfo = BlackboardAsset->GetKey(TargetKeyID);
	if (!TargetValueEntryInfo || !TargetValueEntryInfo->KeyType)
	{
		return false;
	}

	// Key types don't match
	if (SourceValueEntryInfo->KeyType->GetClass() != TargetValueEntryInfo->KeyType->GetClass())
	{
		return false;
	}

	const bool bKeyHasInstance = SourceValueEntryInfo->KeyType->HasInstance();
	const uint16 MemDataOffset = bKeyHasInstance ? sizeof(FBlackboardInstancedKeyMemory) : 0;

	const UBlackboardKeyType* const SourceKeyOb = bKeyHasInstance ? KeyInstances[SourceKeyID] : SourceValueEntryInfo->KeyType;
	const uint8* SourceValueData = GetKeyRawData(SourceKeyID) + MemDataOffset;

	UBlackboardKeyType* const TargetKeyOb = bKeyHasInstance ? KeyInstances[TargetKeyID] : TargetValueEntryInfo->KeyType;
	uint8* const TargetValueData = GetKeyRawData(TargetKeyID) + MemDataOffset;

	UBlackboardKeyTypeHelper::CopyValuesHelper(TargetKeyOb, *BlackboardComponent, TargetValueData, SourceKeyOb, SourceValueData);
	SetKeyChanged(TargetKeyID);

	return true;
}

bool FBlackboardWorldState::TestBasicOperation(const UBlackboardKeyType& Key, FBlackboard::FKey KeyID, EBasicKeyOperation::Type Type) const
{
	check(BlackboardComponent.IsValid());
	
	const uint8* const RawMemory = GetKeyRawData(KeyID);
	if (ensure(RawMemory))
	{
		if (Key.HasInstance())
		{
			const UBlackboardKeyType* const KeyInstance = KeyInstances.IsValidIndex(KeyID) ? KeyInstances[KeyID] : nullptr;
			if (ensure(KeyInstance))
			{
				const uint8* KeyValueMemory = RawMemory + sizeof(FBlackboardInstancedKeyMemory);
				return UBlackboardKeyTypeHelper::TestBasicOperationHelper(*KeyInstance, *BlackboardComponent, KeyValueMemory, Type);
			}
		}
		else
		{
			return UBlackboardKeyTypeHelper::TestBasicOperationHelper(Key, *BlackboardComponent, RawMemory, Type);
		}
	}

	return false;
}

bool FBlackboardWorldState::TestArithmeticOperation(const UBlackboardKeyType& Key, FBlackboard::FKey KeyID,
	EArithmeticKeyOperation::Type Type, int32 IntValue, float FloatValue) const
{
	check(BlackboardComponent.IsValid());

	const uint8* const RawMemory = GetKeyRawData(KeyID);
	if (ensure(RawMemory))
	{
		if (Key.HasInstance())
		{
			const UBlackboardKeyType* const KeyInstance = KeyInstances.IsValidIndex(KeyID) ? KeyInstances[KeyID] : nullptr;
			if (ensure(KeyInstance))
			{
				const uint8* KeyValueMemory = RawMemory + sizeof(FBlackboardInstancedKeyMemory);
				return UBlackboardKeyTypeHelper::TestArithmeticOperationHelper(*KeyInstance, *BlackboardComponent, KeyValueMemory, Type, IntValue, FloatValue);
			}
		}
		else
		{
			return UBlackboardKeyTypeHelper::TestArithmeticOperationHelper(Key, *BlackboardComponent, RawMemory, Type, IntValue, FloatValue);
		}
	}

	return false;
}

bool FBlackboardWorldState::TestTextOperation(const UBlackboardKeyType& Key, FBlackboard::FKey KeyID,
	ETextKeyOperation::Type Type, const FString& StringValue) const
{
	check(BlackboardComponent.IsValid());

	const uint8* const RawMemory = GetKeyRawData(KeyID);
	if (ensure(RawMemory))
	{
		if (Key.HasInstance())
		{
			const UBlackboardKeyType* const KeyInstance = KeyInstances.IsValidIndex(KeyID) ? KeyInstances[KeyID] : nullptr;
			if (ensure(KeyInstance))
			{
				const uint8* KeyValueMemory = RawMemory + sizeof(FBlackboardInstancedKeyMemory);
				return UBlackboardKeyTypeHelper::TestTextOperationHelper(*KeyInstance, *BlackboardComponent, KeyValueMemory, Type, StringValue);
			}
		}
		else
		{
			return UBlackboardKeyTypeHelper::TestTextOperationHelper(Key, *BlackboardComponent, RawMemory, Type, StringValue);
		}
	}

	return false;
}

bool FBlackboardWorldState::IsVectorValueSet(FBlackboard::FKey KeyID) const
{
	const FVector VectorValue = GetValue<UBlackboardKeyType_Vector>(KeyID);
	return VectorValue != FAISystem::InvalidLocation;
}

bool FBlackboardWorldState::GetLocationFromEntry(FBlackboard::FKey KeyID, FVector& ResultLocation) const
{
	if (BlackboardComponent.IsValid() && BlackboardAsset.IsValid())
	{
		const TArray<uint16>& MemoryOffsets = UBlackboardComponentHelper::GetValueMemoryOffsets(*BlackboardComponent);
		if (MemoryOffsets.IsValidIndex(KeyID))
		{
			const FBlackboardEntry* const EntryInfo = BlackboardAsset->GetKey(KeyID);
			if (EntryInfo && EntryInfo->KeyType)
			{
				const uint8* const ValueData = ValueMemory.GetData() + MemoryOffsets[KeyID];
				return EntryInfo->KeyType->WrappedGetLocation(*BlackboardComponent, ValueData, ResultLocation);
			}
		}
	}

	return false;
}

bool FBlackboardWorldState::GetRotationFromEntry(FBlackboard::FKey KeyID, FRotator& ResultRotation) const
{
	if (BlackboardComponent.IsValid() && BlackboardAsset.IsValid())
	{
		const TArray<uint16>& MemoryOffsets = UBlackboardComponentHelper::GetValueMemoryOffsets(*BlackboardComponent);
		if (MemoryOffsets.IsValidIndex(KeyID))
		{
			const FBlackboardEntry* const EntryInfo = BlackboardAsset->GetKey(KeyID);
			if (EntryInfo && EntryInfo->KeyType)
			{
				const uint8* const ValueData = ValueMemory.GetData() + MemoryOffsets[KeyID];
				return EntryInfo->KeyType->WrappedGetRotation(*BlackboardComponent, ValueData, ResultRotation);
			}
		}
	}

	return false;
}

uint8* FBlackboardWorldState::GetKeyRawData(FBlackboard::FKey KeyID)
{
	if (ValueMemory.Num())
	{
		const TArray<uint16>& MemoryOffsets = UBlackboardComponentHelper::GetValueMemoryOffsets(*BlackboardComponent);
		if (ensure(MemoryOffsets.Num()) && MemoryOffsets.IsValidIndex(KeyID))
		{
			check(ValueMemory.IsValidIndex(MemoryOffsets[KeyID]));
			return ValueMemory.GetData() + MemoryOffsets[KeyID];
		}
	}

	return nullptr;
}

const uint8* FBlackboardWorldState::GetKeyRawData(FBlackboard::FKey KeyID) const
{
	return const_cast<FBlackboardWorldState*>(this)->GetKeyRawData(KeyID);
}

FVector FBlackboardWorldState::GetLocation(const FBlackboardKeySelector& KeySelector, AActor** OutActor, UActorComponent** OutComponent) const
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
