#pragma once

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSimpleECS, Log, All);

// --- 기본 타입 정의 ---

/** 엔티티 핸들 (Index + Generation) */
struct FECSEntityHandle
{
	int32 Index = INDEX_NONE;
	int32 Generation = 0;

	FECSEntityHandle() = default;

	FECSEntityHandle(int32 InIndex, int32 InGeneration) : Index(InIndex), Generation(InGeneration)
	{
	}

	bool IsValid() const { return Index != INDEX_NONE && Generation > 0; }

	void Reset()
	{
		Index = INDEX_NONE;
		Generation = 0;
	}

	bool operator==(const FECSEntityHandle& Other) const
	{
		return Index == Other.Index && Generation == Other.Generation;
	}

	friend uint32 GetTypeHash(const FECSEntityHandle& Handle)
	{
		return HashCombine(GetTypeHash(Handle.Index), GetTypeHash(Handle.Generation));
	}
};

/** FMemory를 사용하는 메모리 관리를 위한 커스텀 삭제자 */
struct FMemoryDeleter
{
	void operator()(void* Ptr) const
	{
		FMemory::Free(Ptr);
	}
};

/** 컴포넌트 타입 식별자 (UScriptStruct* 사용) */
using FECSComponentTypeID = const UScriptStruct*;

/** 아키타입의 컴포넌트 구성 */
struct FECSArchetypeComposition
{
	TSet<FECSComponentTypeID> ComponentTypes;

	void Add(FECSComponentTypeID Type) { ComponentTypes.Add(Type); }
	bool Contains(FECSComponentTypeID Type) const { return ComponentTypes.Contains(Type); }

	bool operator==(const FECSArchetypeComposition& Other) const
	{
		return ComponentTypes.Num() == Other.ComponentTypes.Num() && ComponentTypes.Includes(Other.ComponentTypes);
	}

	friend uint32 GetTypeHash(const FECSArchetypeComposition& Comp)
	{
		uint32 Hash = 0;
		for (FECSComponentTypeID Type : Comp.ComponentTypes)
		{
			Hash = HashCombine(Hash, GetTypeHash(Type));
		}
		return Hash;
	}
};

// --- 전방 선언 ---
class FECSArchetype;
class FECSArchetypeChunk;
class UUKECSManager;

/** 엔티티 레코드 */
struct FEntityRecord
{
	FECSArchetype* Archetype = nullptr; // 약한 참조 - 소유권 없음
	int32 ChunkIndex = INDEX_NONE;
	int32 IndexInChunk = INDEX_NONE;
	int32 Generation = 0;
};

//				--- 청크 클래스 ---
/*			데이터를 SoA 방식으로 저장하는 메모리 청크
			청크 (Chunk for Arch_Move) - Capacity: N Entities
+---------------------------------------------------------------------------+
| Entity Handles Array (FECSEntityHandle*)                                  |
| E[0] | E[1] | E[2] | ... | E[N-1]                                         |
+---------------------------------------------------------------------------+
| Position Component Array (FPositionComponent*)                            | <-- ComponentDataArrays[Position]이 여기를 가리킴
| P[0] | P[1] | P[2] | ... | P[N-1]                                         |
+---------------------------------------------------------------------------+
| Velocity Component Array (FVelocityComponent*)                            | <-- ComponentDataArrays[Velocity]가 여기를 가리킴
| V[0] | V[1] | V[2] | ... | V[N-1]                                         |
+---------------------------------------------------------------------------+
1. 하나의 청크는 여러 엔티티(최대 N개)의 데이터를 담습니다.
2. 먼저 모든 엔티티의 핸들(E[0]~E[N-1])이 연속으로 저장됩니다.
3. 그다음, 모든 엔티티의 Position 컴포넌트(P[0]~P[N-1])가 연속으로 저장됩니다.
4. 그다음, 모든 엔티티의 Velocity 컴포넌트(V[0]~V[N-1])가 연속으로 저장됩니다.
5. ComponentDataArrays 맵은 각 컴포넌트 배열(Position Component Array, Velocity Component Array)의 시작 메모리 주소를 저장하고 있습니다.
6. 엔티티 E[i]의 Position 데이터는 P[i]에, Velocity 데이터는 V[i]에 저장됩니다.
*/
class FECSArchetypeChunk
{
public:
	FECSArchetypeChunk(const FECSArchetypeComposition& InComposition, int32 InCapacity);
	~FECSArchetypeChunk();

	int32 AddEntity(FECSEntityHandle Entity);
	FECSEntityHandle RemoveEntity(int32 IndexInChunk);

	void* GetComponentDataPtr(FECSComponentTypeID TypeID, int32 IndexInChunk);
	const void* GetComponentDataPtr(FECSComponentTypeID TypeID, int32 IndexInChunk) const;

	template <typename T>
	T* GetComponentDataPtr(int32 IndexInChunk)
	{
		return static_cast<T*>(GetComponentDataPtr(T::StaticStruct(), IndexInChunk));
	}

	template <typename T>
	const T* GetComponentDataPtr(int32 IndexInChunk) const
	{
		return static_cast<const T*>(GetComponentDataPtr(T::StaticStruct(), IndexInChunk));
	}

	FECSEntityHandle GetEntityHandle(int32 IndexInChunk) const;
	int32 GetEntityCount() const { return NumEntities; }
	bool IsFull() const { return NumEntities >= Capacity; }

	void* GetComponentRawDataArray(FECSComponentTypeID TypeID) { return ComponentDataArrays.FindRef(TypeID); }

	const void* GetComponentRawDataArray(FECSComponentTypeID TypeID) const
	{
		return ComponentDataArrays.FindRef(TypeID);
	}

private:
	const FECSArchetypeComposition Composition;
	const int32 Capacity;
	int32 NumEntities = 0;

	// SoA 데이터 저장 (스마트 포인터 적용)
	TUniquePtr<FECSEntityHandle[], FMemoryDeleter> EntityHandles;
	TMap<FECSComponentTypeID, void*> ComponentDataArrays; // 청크 메모리 내부 포인터
	TMap<FECSComponentTypeID, int32> ComponentSizes;
	TUniquePtr<uint8[], FMemoryDeleter> ChunkMemory;
};

// --- 아키타입 클래스 ---
/** 동일한 컴포넌트 구성을 가진 엔티티 그룹 */
class FECSArchetype
{
public:
	FECSArchetype(const FECSArchetypeComposition& InComposition, class UUKECSManager* InManager);
	~FECSArchetype() = default; // 스마트 포인터로 인해 청크 소멸은 자동 처리됨

	void AddEntity(FECSEntityHandle Entity, int32& OutChunkIndex, int32& OutIndexInChunk);
	TTuple<FECSEntityHandle, int32, int32> RemoveEntity(int32 ChunkIndex, int32 IndexInChunk);

	void* GetComponentDataPtr(int32 ChunkIndex, int32 IndexInChunk, FECSComponentTypeID TypeID);
	const void* GetComponentDataPtr(int32 ChunkIndex, int32 IndexInChunk, FECSComponentTypeID TypeID) const;

	const FECSArchetypeComposition& GetComposition() const { return Composition; }
	const TArray<TUniquePtr<FECSArchetypeChunk>>& GetChunks() const { return Chunks; }

private:
	FECSArchetypeChunk* FindAvailableChunkOrCreate();
	FECSArchetypeChunk* AllocateNewChunk();

	const FECSArchetypeComposition Composition;
	class UUKECSManager* Manager = nullptr; // 약한 참조 - 소유권 없음
	TArray<TUniquePtr<FECSArchetypeChunk>> Chunks; // 스마트 포인터 적용
	int32 ChunkCapacity = 128;
};