// Fill out your copyright notice in the Description page of Project Settings.


#include "UKECSManager.h"
#include "UKECSSystemBase.h"

UUKECSManager* UUKECSManager::Instance = nullptr;

void UUKECSManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Instance = this;
    
	// FECSManager의 생성자 코드
	// Index 0은 Invalid Handle용으로 예약
	Entities.AddDefaulted();
}

void UUKECSManager::Deinitialize()
{
	Super::Deinitialize();
	Instance = nullptr;
	
	// 모든 엔티티 정리
	// 단, 이 시점에서는 엔티티를 제거하는 대신 참조만 정리하는 것이 안전할 수 있음
	Entities.Empty();
	FreeEntityIndices.Empty();
	Archetypes.Empty();
	NextEntityGeneration.Empty();
	Systems.Empty();
    
}

TStatId UUKECSManager::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UECSManager, STATGROUP_Tickables);
}

void UUKECSManager::Tick(float DeltaTime)
{
	// 등록된 모든 시스템 업데이트
	for (const TTuple<TSubclassOf<UUKECSSystemBase>, TObjectPtr<UUKECSSystemBase>>& Iter : Systems)
	{
		UUKECSSystemBase* System = Iter.Value.Get();
		if (Iter.Value->IsValidLowLevel() && System)
		{
			System->Tick(DeltaTime, this);
		}
	}
}

FECSEntityHandle UUKECSManager::CreateEntity(const FECSArchetypeComposition& Composition)
{
	FECSArchetype* Archetype = GetOrCreateArchetype(Composition);
	FECSEntityHandle NewHandle = AllocateEntityHandle();

	int32 ChunkIndex, IndexInChunk;
	Archetype->AddEntity(NewHandle, ChunkIndex, IndexInChunk);

	// 엔티티 레코드 업데이트
	check(Entities.IsValidIndex(NewHandle.Index));
	FEntityRecord& Record = Entities[NewHandle.Index];
	Record.Archetype = Archetype;
	Record.ChunkIndex = ChunkIndex;
	Record.IndexInChunk = IndexInChunk;
	Record.Generation = NewHandle.Generation;

	return NewHandle;
}

bool UUKECSManager::DestroyEntity(FECSEntityHandle Entity)
{
	if (!IsEntityValid(Entity)) { return false; }

	FEntityRecord& Record = Entities[Entity.Index];
	FECSArchetype* Archetype = Record.Archetype;
	const int32 ChunkIndex = Record.ChunkIndex;
	const int32 IndexInChunk = Record.IndexInChunk;

	if (Archetype)
	{
		TTuple<FECSEntityHandle, int32, int32> MovedInfo = Archetype->RemoveEntity(ChunkIndex, IndexInChunk);
		FECSEntityHandle MovedEntity = MovedInfo.Get<0>();

		if (MovedEntity.IsValid())
		{
			check(Entities.IsValidIndex(MovedEntity.Index));
			FEntityRecord& MovedRecord = Entities[MovedEntity.Index];
			MovedRecord.ChunkIndex = MovedInfo.Get<1>();
			MovedRecord.IndexInChunk = MovedInfo.Get<2>();
		}
	}

	// 엔티티 레코드 리셋 및 핸들 해제
	Record.Archetype = nullptr;
	Record.ChunkIndex = INDEX_NONE;
	Record.IndexInChunk = INDEX_NONE;
	ReleaseEntityHandle(Entity);

	return true;
}

bool UUKECSManager::AddComponent(FECSEntityHandle Entity, FECSComponentTypeID TypeID, const void* Data)
{
	if (!IsEntityValid(Entity) || TypeID == nullptr) { return false; }

	FEntityRecord& Record = Entities[Entity.Index];
	FECSArchetype* CurrentArchetype = Record.Archetype;
	check(CurrentArchetype);

	if (CurrentArchetype->GetComposition().Contains(TypeID))
	{
		// 이미 컴포넌트가 있다면 데이터만 업데이트 (선택적)
		if (Data)
		{
			void* DestData = CurrentArchetype->GetComponentDataPtr(Record.ChunkIndex, Record.IndexInChunk, TypeID);
			if (DestData)
			{
				TypeID->CopyScriptStruct(DestData, Data);
				return true;
			}
		}
		return false;
	}

	// 새 아키타입 찾기/생성
	FECSArchetypeComposition NewComposition = CurrentArchetype->GetComposition();
	NewComposition.Add(TypeID);
	FECSArchetype* TargetArchetype = GetOrCreateArchetype(NewComposition);

	// 엔티티 이동
	MoveEntityToArchetype(Entity, TargetArchetype);

	// 새 컴포넌트 데이터 설정
	if (Data)
	{
		FEntityRecord& UpdatedRecord = Entities[Entity.Index];
		void* DestData = TargetArchetype->GetComponentDataPtr(UpdatedRecord.ChunkIndex, UpdatedRecord.IndexInChunk,
		                                                      TypeID);
		if (DestData)
		{
			TypeID->CopyScriptStruct(DestData, Data);
		}
	}

	return true;
}

bool UUKECSManager::RemoveComponent(FECSEntityHandle Entity, FECSComponentTypeID TypeID)
{
	if (!IsEntityValid(Entity) || TypeID == nullptr) { return false; }

	FEntityRecord& Record = Entities[Entity.Index];
	FECSArchetype* CurrentArchetype = Record.Archetype;
	check(CurrentArchetype);

	if (!CurrentArchetype->GetComposition().Contains(TypeID)) { return false; }

	// 새 아키타입 찾기/생성
	FECSArchetypeComposition NewComposition = CurrentArchetype->GetComposition();
	NewComposition.ComponentTypes.Remove(TypeID);
	FECSArchetype* TargetArchetype = GetOrCreateArchetype(NewComposition);

	// 엔티티 이동
	MoveEntityToArchetype(Entity, TargetArchetype);

	return true;
}

void* UUKECSManager::GetComponentData(FECSEntityHandle Entity, FECSComponentTypeID TypeID)
{
	if (!IsEntityValid(Entity)) { return nullptr; }
	const FEntityRecord& Record = Entities[Entity.Index];
	return Record.Archetype
		       ? Record.Archetype->GetComponentDataPtr(Record.ChunkIndex, Record.IndexInChunk, TypeID)
		       : nullptr;
}

const void* UUKECSManager::GetComponentDataReadOnly(FECSEntityHandle Entity, FECSComponentTypeID TypeID) const
{
	if (!IsEntityValid(Entity)) { return nullptr; }
	const FEntityRecord& Record = Entities[Entity.Index];
	return Record.Archetype
		       ? Record.Archetype->GetComponentDataPtr(Record.ChunkIndex, Record.IndexInChunk, TypeID)
		       : nullptr;
}

bool UUKECSManager::HasComponent(FECSEntityHandle Entity, FECSComponentTypeID TypeID) const
{
	if (!IsEntityValid(Entity)) { return false; }
	const FEntityRecord& Record = Entities[Entity.Index];
	return Record.Archetype && Record.Archetype->GetComposition().Contains(TypeID);
}

bool UUKECSManager::IsEntityValid(FECSEntityHandle Entity) const
{
	return Entities.IsValidIndex(Entity.Index) && Entities[Entity.Index].Generation == Entity.Generation;
}

void UUKECSManager::ForEachChunk(const TArray<FECSComponentTypeID>& ComponentTypes, TFunctionRef<void(FECSArchetypeChunk& Chunk)> Func)
{
	FECSArchetypeComposition QueryComposition;
	for (FECSComponentTypeID TypeID : ComponentTypes)
	{
		QueryComposition.Add(TypeID);
	}

	for (auto& ArchetypePair : Archetypes)
	{
		FECSArchetype* Archetype = ArchetypePair.Value.Get();

		// 아키타입이 쿼리에 필요한 모든 컴포넌트를 가지고 있는지 확인
		bool bMatch = true;
		for (FECSComponentTypeID RequiredType : ComponentTypes)
		{
			if (!Archetype->GetComposition().Contains(RequiredType))
			{
				bMatch = false;
				break;
			}
		}

		if (bMatch)
		{
			for (const TUniquePtr<FECSArchetypeChunk>& ChunkPtr : Archetype->GetChunks())
			{
				if (ChunkPtr && ChunkPtr->GetEntityCount() > 0)
				{
					Func(*ChunkPtr);
				}
			}
		}
	}
}

void UUKECSManager::AddSystem(TSubclassOf<UUKECSSystemBase> ECSSystemBaseClass)
{
	// 이미 해당 타입의 시스템이 등록되어 있는지 확인
	if (Systems.Contains(ECSSystemBaseClass))
	{
		return;
	}
    
	// 새 시스템 인스턴스 생성
	UUKECSSystemBase* NewSystem = NewObject<UUKECSSystemBase>(this, ECSSystemBaseClass);
	if (NewSystem)
	{
		NewSystem->Register();
		Systems.Add(ECSSystemBaseClass, NewSystem);
	}
}

FECSArchetype* UUKECSManager::GetOrCreateArchetype(const FECSArchetypeComposition& Composition)
{
	const uint32 Hash = GetTypeHash(Composition);
	TUniquePtr<FECSArchetype>* Found = Archetypes.Find(Hash);
	if (Found)
	{
		if ((*Found)->GetComposition() == Composition)
		{
			return Found->Get();
		}
	}

	TUniquePtr<FECSArchetype> NewArchetype = MakeUnique<FECSArchetype>(Composition, this);
	FECSArchetype* Result = NewArchetype.Get();
	Archetypes.Add(Hash, MoveTemp(NewArchetype));
	return Result;
}

void UUKECSManager::MoveEntityToArchetype(FECSEntityHandle Entity, FECSArchetype* TargetArchetype)
{
	check(IsEntityValid(Entity));
	check(TargetArchetype);

	FEntityRecord& Record = Entities[Entity.Index];
	FECSArchetype* SourceArchetype = Record.Archetype;
	const int32 SourceChunkIndex = Record.ChunkIndex;
	const int32 SourceIndexInChunk = Record.IndexInChunk;

	if (SourceArchetype == TargetArchetype) { return; }

	// 1. 새 아키타입에 엔티티 추가
	int32 TargetChunkIndex, TargetIndexInChunk;
	TargetArchetype->AddEntity(Entity, TargetChunkIndex, TargetIndexInChunk);

	// 2. 공통 컴포넌트 데이터 복사
	const FECSArchetypeComposition& SourceComp = SourceArchetype->GetComposition();
	const FECSArchetypeComposition& TargetComp = TargetArchetype->GetComposition();
	for (FECSComponentTypeID TypeID : SourceComp.ComponentTypes)
	{
		if (TargetComp.Contains(TypeID))
		{
			void* SrcData = SourceArchetype->GetComponentDataPtr(SourceChunkIndex, SourceIndexInChunk, TypeID);
			void* DestData = TargetArchetype->GetComponentDataPtr(TargetChunkIndex, TargetIndexInChunk, TypeID);
			if (SrcData && DestData)
			{
				TypeID->CopyScriptStruct(DestData, SrcData);
			}
		}
	}

	// 3. 이전 아키타입에서 엔티티 제거 및 이동된 엔티티 처리
	TTuple<FECSEntityHandle, int32, int32> MovedInfo = SourceArchetype->RemoveEntity(
		SourceChunkIndex, SourceIndexInChunk);
	FECSEntityHandle MovedEntity = MovedInfo.Get<0>();
	if (MovedEntity.IsValid())
	{
		FEntityRecord& MovedRecord = Entities[MovedEntity.Index];
		MovedRecord.ChunkIndex = MovedInfo.Get<1>();
		MovedRecord.IndexInChunk = MovedInfo.Get<2>();
	}

	// 4. 현재 엔티티 레코드 업데이트
	Record.Archetype = TargetArchetype;
	Record.ChunkIndex = TargetChunkIndex;
	Record.IndexInChunk = TargetIndexInChunk;
}

FECSEntityHandle UUKECSManager::AllocateEntityHandle()
{
	int32 Index;
	if (FreeEntityIndices.Num() > 0)
	{
		Index = FreeEntityIndices.Pop();
	}
	else
	{
		Index = Entities.AddDefaulted(); // 새 슬롯 할당
	}
	// Generation 관리 (0은 유효하지 않음)
	int32 Generation = NextEntityGeneration.FindOrAdd(Index, 1);
	if (Generation == 0)
	{
		Generation = 1;
	}
	NextEntityGeneration[Index] = Generation; // 다음 번호를 위해 저장 (Release에서 증가시킴)

	return FECSEntityHandle(Index, Generation);
}

void UUKECSManager::ReleaseEntityHandle(FECSEntityHandle Handle)
{
	if (Entities.IsValidIndex(Handle.Index))
	{
		int32& NextGen = NextEntityGeneration.FindChecked(Handle.Index);
		NextGen++;
		if (NextGen == 0) { NextGen = 1; } // 0 방지
		FreeEntityIndices.Add(Handle.Index);
	}
}