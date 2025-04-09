#include "SimpleECS.h"
#include "HAL/PlatformMemory.h"
#include "Misc/AssertionMacros.h"

DEFINE_LOG_CATEGORY(LogSimpleECS);

// --- FECSArchetypeChunk 구현 ---

FECSArchetypeChunk::FECSArchetypeChunk(const FECSArchetypeComposition& InComposition, int32 InCapacity)
	: Composition(InComposition), Capacity(InCapacity), NumEntities(0)
{
	check(Capacity > 0);

	// 1. 엔티티 핸들 배열 메모리 할당 (스마트 포인터 사용)
	EntityHandles = TUniquePtr<FECSEntityHandle[], FMemoryDeleter>(static_cast<FECSEntityHandle*>(FMemory::Malloc(sizeof(FECSEntityHandle) * Capacity)));
	check(EntityHandles.Get());

	// 2. 컴포넌트 데이터 메모리 계산 및 할당 (SoA)
	SIZE_T TotalComponentSize = 0;
	ComponentSizes.Reserve(Composition.ComponentTypes.Num());
	ComponentDataArrays.Reserve(Composition.ComponentTypes.Num());

	// 정렬을 고려하여 각 컴포넌트 배열의 오프셋과 전체 크기 계산
	for (FECSComponentTypeID TypeID : Composition.ComponentTypes)
	{
		const int32 Size = TypeID->GetStructureSize();
		const int32 Alignment = TypeID->GetMinAlignment();
		check(Size > 0);
		ComponentSizes.Add(TypeID, Size);

		TotalComponentSize = Align(TotalComponentSize, Alignment);
		ComponentDataArrays.Add(TypeID, nullptr);
		TotalComponentSize += (SIZE_T)Size * Capacity;
	}

	// 전체 컴포넌트 데이터 메모리 할당 (스마트 포인터 사용)
	if (TotalComponentSize > 0)
	{
		ChunkMemory = TUniquePtr<uint8[], FMemoryDeleter>(static_cast<uint8*>(FMemory::Malloc(TotalComponentSize, DEFAULT_ALIGNMENT)));
		check(ChunkMemory.Get());

		// 각 컴포넌트 배열의 실제 시작 주소 계산 및 저장
		SIZE_T CurrentOffset = 0;
		for (auto It = ComponentDataArrays.CreateIterator(); It; ++It)
		{
			FECSComponentTypeID TypeID = It.Key();
			const int32 Size = ComponentSizes[TypeID];
			const int32 Alignment = TypeID->GetMinAlignment();

			uint8* AlignedPtr = (uint8*)Align((SIZE_T)(ChunkMemory.Get() + CurrentOffset), Alignment);
			It.Value() = AlignedPtr;
			CurrentOffset = (AlignedPtr - ChunkMemory.Get()) + (SIZE_T)Size * Capacity;
		}
	}
}

FECSArchetypeChunk::~FECSArchetypeChunk()
{
	// 엔티티 데이터 소멸
	for (int32 i = NumEntities - 1; i >= 0; --i)
	{
		for (auto It = ComponentDataArrays.CreateConstIterator(); It; ++It)
		{
			FECSComponentTypeID TypeID = It.Key();
			uint8* DataArrayStart = static_cast<uint8*>(It.Value());
			const int32 Size = ComponentSizes[TypeID];
			void* CompData = DataArrayStart + ((SIZE_T)i * Size);
			TypeID->DestroyStruct(CompData);
		}
	}

	// 메모리 해제는 스마트 포인터가 자동으로 처리
}

int32 FECSArchetypeChunk::AddEntity(FECSEntityHandle Entity)
{
	if (IsFull()) { return INDEX_NONE; }

	const int32 Index = NumEntities++;
	EntityHandles[Index] = Entity;

	// 컴포넌트 기본 생성
	for (auto It = ComponentDataArrays.CreateConstIterator(); It; ++It)
	{
		FECSComponentTypeID TypeID = It.Key();
		uint8* DataArrayStart = static_cast<uint8*>(It.Value());
		const int32 Size = ComponentSizes[TypeID];
		void* CompData = DataArrayStart + ((SIZE_T)Index * Size);
		TypeID->InitializeStruct(CompData);
	}
	return Index;
}

FECSEntityHandle FECSArchetypeChunk::RemoveEntity(int32 IndexInChunk)
{
	check(IndexInChunk >= 0 && IndexInChunk < NumEntities);

	const int32 LastIndex = NumEntities - 1;
	FECSEntityHandle MovedEntity = FECSEntityHandle();

	// 제거될 위치의 컴포넌트 소멸
	for (auto It = ComponentDataArrays.CreateConstIterator(); It; ++It)
	{
		uint8* DataArrayStart = static_cast<uint8*>(It.Value());
		FECSComponentTypeID TypeID = It.Key();
		const int32 Size = ComponentSizes[TypeID];
		void* CompData = DataArrayStart + (SIZE_T)IndexInChunk * Size;
		TypeID->DestroyStruct(CompData);
	}

	// 마지막 엔티티가 아니라면, 마지막 엔티티 데이터를 현재 위치로 이동 (Swap & Pop)
	if (IndexInChunk != LastIndex)
	{
		EntityHandles[IndexInChunk] = EntityHandles[LastIndex];
		MovedEntity = EntityHandles[IndexInChunk];

		for (auto It = ComponentDataArrays.CreateConstIterator(); It; ++It)
		{
			uint8* DataArrayStart = static_cast<uint8*>(It.Value());
			FECSComponentTypeID TypeID = It.Key();
			const int32 Size = ComponentSizes[TypeID];
			void* Dest = DataArrayStart + (SIZE_T)IndexInChunk * Size;
			const void* Src = DataArrayStart + (SIZE_T)LastIndex * Size;
			FMemory::Memcpy(Dest, Src, Size);
		}
	}

	NumEntities--;
	return MovedEntity;
}

void* FECSArchetypeChunk::GetComponentDataPtr(FECSComponentTypeID TypeID, int32 IndexInChunk)
{
	check(IndexInChunk >= 0 && IndexInChunk < NumEntities);
	void** Ptr = ComponentDataArrays.Find(TypeID);
	if (Ptr && *Ptr)
	{
		const int32 Size = ComponentSizes[TypeID];
		return static_cast<uint8*>(*Ptr) + (SIZE_T)IndexInChunk * Size;
	}
	return nullptr;
}

const void* FECSArchetypeChunk::GetComponentDataPtr(FECSComponentTypeID TypeID, int32 IndexInChunk) const
{
	return const_cast<FECSArchetypeChunk*>(this)->GetComponentDataPtr(TypeID, IndexInChunk);
}

FECSEntityHandle FECSArchetypeChunk::GetEntityHandle(int32 IndexInChunk) const
{
	check(IndexInChunk >= 0 && IndexInChunk < NumEntities);
	return EntityHandles[IndexInChunk];
}

// --- FECSArchetype 구현 ---

FECSArchetype::FECSArchetype(const FECSArchetypeComposition& InComposition, FECSManager* InManager)
	: Composition(InComposition), Manager(InManager)
{
	check(Manager);
	AllocateNewChunk();
}

void FECSArchetype::AddEntity(FECSEntityHandle Entity, int32& OutChunkIndex, int32& OutIndexInChunk)
{
	FECSArchetypeChunk* TargetChunk = FindAvailableChunkOrCreate();
	OutIndexInChunk = TargetChunk->AddEntity(Entity);

	// 청크 배열에서 인덱스 찾기
	for (int32 i = 0; i < Chunks.Num(); ++i)
	{
		if (Chunks[i].Get() == TargetChunk)
		{
			OutChunkIndex = i;
			break;
		}
	}
	check(OutChunkIndex != INDEX_NONE);
}

TTuple<FECSEntityHandle, int32, int32> FECSArchetype::RemoveEntity(int32 ChunkIndex, int32 IndexInChunk)
{
	check(Chunks.IsValidIndex(ChunkIndex));
	FECSArchetypeChunk* Chunk = Chunks[ChunkIndex].Get();
	FECSEntityHandle MovedEntity = Chunk->RemoveEntity(IndexInChunk);

	if (MovedEntity.IsValid())
	{
		return MakeTuple(MovedEntity, ChunkIndex, IndexInChunk);
	}
	return MakeTuple(FECSEntityHandle(), INDEX_NONE, INDEX_NONE);
}

void* FECSArchetype::GetComponentDataPtr(int32 ChunkIndex, int32 IndexInChunk, FECSComponentTypeID TypeID)
{
	check(Chunks.IsValidIndex(ChunkIndex));
	return Chunks[ChunkIndex]->GetComponentDataPtr(TypeID, IndexInChunk);
}

const void* FECSArchetype::GetComponentDataPtr(int32 ChunkIndex, int32 IndexInChunk, FECSComponentTypeID TypeID) const
{
	check(Chunks.IsValidIndex(ChunkIndex));
	return Chunks[ChunkIndex]->GetComponentDataPtr(TypeID, IndexInChunk);
}

FECSArchetypeChunk* FECSArchetype::FindAvailableChunkOrCreate()
{
	for (int32 i = Chunks.Num() - 1; i >= 0; --i)
	{
		if (!Chunks[i]->IsFull())
		{
			return Chunks[i].Get();
		}
	}
	return AllocateNewChunk();
}

FECSArchetypeChunk* FECSArchetype::AllocateNewChunk()
{
	TUniquePtr<FECSArchetypeChunk> NewChunk = MakeUnique<FECSArchetypeChunk>(Composition, ChunkCapacity);
	FECSArchetypeChunk* Result = NewChunk.Get();
	Chunks.Add(MoveTemp(NewChunk));
	return Result;
}

// --- FECSManager 구현 ---

FECSManager::FECSManager()
{
	// Index 0은 Invalid Handle용으로 예약
	Entities.AddDefaulted();
}

FECSEntityHandle FECSManager::CreateEntity(const FECSArchetypeComposition& Composition)
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

bool FECSManager::DestroyEntity(FECSEntityHandle Entity)
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

bool FECSManager::AddComponent(FECSEntityHandle Entity, FECSComponentTypeID TypeID, const void* Data)
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

bool FECSManager::RemoveComponent(FECSEntityHandle Entity, FECSComponentTypeID TypeID)
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

void* FECSManager::GetComponentData(FECSEntityHandle Entity, FECSComponentTypeID TypeID)
{
	if (!IsEntityValid(Entity)) { return nullptr; }
	const FEntityRecord& Record = Entities[Entity.Index];
	return Record.Archetype
		       ? Record.Archetype->GetComponentDataPtr(Record.ChunkIndex, Record.IndexInChunk, TypeID)
		       : nullptr;
}

const void* FECSManager::GetComponentDataReadOnly(FECSEntityHandle Entity, FECSComponentTypeID TypeID) const
{
	if (!IsEntityValid(Entity)) { return nullptr; }
	const FEntityRecord& Record = Entities[Entity.Index];
	return Record.Archetype
		       ? Record.Archetype->GetComponentDataPtr(Record.ChunkIndex, Record.IndexInChunk, TypeID)
		       : nullptr;
}

bool FECSManager::HasComponent(FECSEntityHandle Entity, FECSComponentTypeID TypeID) const
{
	if (!IsEntityValid(Entity)) { return false; }
	const FEntityRecord& Record = Entities[Entity.Index];
	return Record.Archetype && Record.Archetype->GetComposition().Contains(TypeID);
}

bool FECSManager::IsEntityValid(FECSEntityHandle Entity) const
{
	return Entities.IsValidIndex(Entity.Index) && Entities[Entity.Index].Generation == Entity.Generation;
}

void FECSManager::ForEachChunk(const TArray<FECSComponentTypeID>& ComponentTypes,
                               TFunctionRef<void(FECSArchetypeChunk& Chunk)> Func)
{
	FECSArchetypeComposition QueryComposition;
	for (FECSComponentTypeID TypeID : ComponentTypes) { QueryComposition.Add(TypeID); }

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

FECSArchetype* FECSManager::GetOrCreateArchetype(const FECSArchetypeComposition& Composition)
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

void FECSManager::MoveEntityToArchetype(FECSEntityHandle Entity, FECSArchetype* TargetArchetype)
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

FECSEntityHandle FECSManager::AllocateEntityHandle()
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

void FECSManager::ReleaseEntityHandle(FECSEntityHandle Handle)
{
	if (Entities.IsValidIndex(Handle.Index))
	{
		int32& NextGen = NextEntityGeneration.FindChecked(Handle.Index);
		NextGen++;
		if (NextGen == 0) { NextGen = 1; } // 0 방지
		FreeEntityIndices.Add(Handle.Index);
	}
}