#include "UKECSEntity.h"
#include "Misc/AssertionMacros.h"
#include "UObject/Class.h"

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

FECSArchetype::FECSArchetype(const FECSArchetypeComposition& InComposition, UUKECSManager* InManager)
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