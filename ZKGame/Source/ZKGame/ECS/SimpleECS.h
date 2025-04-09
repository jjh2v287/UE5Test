#pragma once

#include "CoreMinimal.h" // FVector 등 기본 타입 사용을 위해 포함
#include "Containers/Set.h"
#include "Containers/Map.h"
#include "Containers/Array.h"
#include "Logging/LogMacros.h"
#include "SimpleECS.generated.h"

// 로그 카테고리 정의
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

	bool IsValid() const
	{
		return Index != INDEX_NONE && Generation > 0;
	}

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

	FString ToString() const
	{
		return FString::Printf(TEXT("E(%d,%d)"), Index, Generation);
	}
};

/** 모든 컴포넌트의 기본 구조체 */
USTRUCT(BlueprintType)
struct FECSComponentBase
{
	GENERATED_BODY()
	virtual ~FECSComponentBase() = default;
};

/** 컴포넌트 타입 식별자 (UScriptStruct* 사용) */
using FECSComponentTypeID = const UScriptStruct*;

/** 아키타입의 컴포넌트 구성 (간단하게 TSet 사용) */
struct FECSArchetypeComposition
{
	TSet<FECSComponentTypeID> ComponentTypes;

	void Add(FECSComponentTypeID Type)
	{
		ComponentTypes.Add(Type);
	}
	
	bool Contains(FECSComponentTypeID Type) const
	{
		return ComponentTypes.Contains(Type);
	}

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

	FString ToString() const
	{
		FString S = TEXT("{");
		for (FECSComponentTypeID Type : ComponentTypes) { S += GetNameSafe(Type) + TEXT(" "); }
		S.TrimEndInline();
		S += TEXT("}");
		return S;
	}
};

// --- 전방 선언 ---
class FECSArchetype;
class FECSArchetypeChunk;

/** 엔티티가 어느 아키타입, 어느 청크, 어느 위치에 있는지 기록 */
struct FEntityRecord
{
	FECSArchetype* Archetype = nullptr;
	int32 ChunkIndex = INDEX_NONE;
	int32 IndexInChunk = INDEX_NONE;
	int32 Generation = 0; // 현재 엔티티의 Generation
};

// --- 청크 클래스 ---
/** 데이터를 SoA 방식으로 저장하는 메모리 청크 */
class FECSArchetypeChunk
{
public:
	FECSArchetypeChunk(const FECSArchetypeComposition& InComposition, int32 InCapacity);
	~FECSArchetypeChunk();

	// 복사/이동 불가
	FECSArchetypeChunk(const FECSArchetypeChunk&) = delete;
	FECSArchetypeChunk& operator=(const FECSArchetypeChunk&) = delete;

	int32 AddEntity(FECSEntityHandle Entity);
	FECSEntityHandle RemoveEntity(int32 IndexInChunk); // 제거된 위치로 이동된 엔티티 핸들 반환

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

	// 쿼리용 데이터 접근
	int32 GetCapacity() const { return Capacity; }

	TArrayView<FECSEntityHandle> GetEntityHandleArrayView()
	{
		return TArrayView<FECSEntityHandle>(EntityHandles, NumEntities);
	}

	void* GetComponentRawDataArray(FECSComponentTypeID TypeID) { return ComponentDataArrays.FindRef(TypeID); }

	const void* GetComponentRawDataArray(FECSComponentTypeID TypeID) const
	{
		return ComponentDataArrays.FindRef(TypeID);
	}

private:
	const FECSArchetypeComposition Composition;
	const int32 Capacity;
	int32 NumEntities = 0;

	// SoA 데이터 저장
	FECSEntityHandle* EntityHandles = nullptr; // 엔티티 핸들 배열
	TMap<FECSComponentTypeID, void*> ComponentDataArrays; // 컴포넌트 타입별 데이터 배열 시작 주소
	TMap<FECSComponentTypeID, int32> ComponentSizes; // 컴포넌트 타입별 크기
	uint8* ChunkMemory = nullptr; // 전체 컴포넌트 데이터 메모리 블록
};

// --- 아키타입 클래스 ---
/** 동일한 컴포넌트 구성을 가진 엔티티 그룹 */
class FECSArchetype
{
public:
	FECSArchetype(const FECSArchetypeComposition& InComposition, class FECSManager* InManager);
	~FECSArchetype();

	// 복사/이동 불가
	FECSArchetype(const FECSArchetype&) = delete;
	FECSArchetype& operator=(const FECSArchetype&) = delete;

	void AddEntity(FECSEntityHandle Entity, int32& OutChunkIndex, int32& OutIndexInChunk);
	// 제거된 위치로 이동된 엔티티 핸들 및 새 위치 정보 반환
	TTuple<FECSEntityHandle, int32, int32> RemoveEntity(int32 ChunkIndex, int32 IndexInChunk);

	void* GetComponentDataPtr(int32 ChunkIndex, int32 IndexInChunk, FECSComponentTypeID TypeID);
	const void* GetComponentDataPtr(int32 ChunkIndex, int32 IndexInChunk, FECSComponentTypeID TypeID) const;

	const FECSArchetypeComposition& GetComposition() const { return Composition; }
	const TArray<FECSArchetypeChunk*>& GetChunks() const { return Chunks; }
	int32 GetEntityCount() const;

private:
	FECSArchetypeChunk* FindAvailableChunkOrCreate();
	FECSArchetypeChunk* AllocateNewChunk();

	const FECSArchetypeComposition Composition;
	class FECSManager* Manager = nullptr; // ECS 매니저 참조
	TArray<FECSArchetypeChunk*> Chunks;
	int32 ChunkCapacity = 128; // 예시: 청크당 엔티티 수
};

// --- ECS 매니저 클래스 ---
/** ECS 월드를 관리하는 중앙 클래스 */
class FECSManager
{
public:
	FECSManager();
	~FECSManager();

	// 복사/이동 불가
	FECSManager(const FECSManager&) = delete;
	FECSManager& operator=(const FECSManager&) = delete;

	FECSEntityHandle CreateEntity(const FECSArchetypeComposition& Composition);
	bool DestroyEntity(FECSEntityHandle Entity);

	bool AddComponent(FECSEntityHandle Entity, FECSComponentTypeID TypeID, const void* Data = nullptr);
	bool RemoveComponent(FECSEntityHandle Entity, FECSComponentTypeID TypeID);

	void* GetComponentData(FECSEntityHandle Entity, FECSComponentTypeID TypeID);
	const void* GetComponentDataReadOnly(FECSEntityHandle Entity, FECSComponentTypeID TypeID) const;
	bool HasComponent(FECSEntityHandle Entity, FECSComponentTypeID TypeID) const;

	template <typename T>
	T* GetComponentData(FECSEntityHandle Entity)
	{
		return static_cast<T*>(GetComponentData(Entity, T::StaticStruct()));
	}

	template <typename T>
	const T* GetComponentDataReadOnly(FECSEntityHandle Entity) const
	{
		return static_cast<const T*>(GetComponentDataReadOnly(Entity, T::StaticStruct()));
	}

	template <typename T>
	bool HasComponent(FECSEntityHandle Entity) const
	{
		return HasComponent(Entity, T::StaticStruct());
	}

	bool IsEntityValid(FECSEntityHandle Entity) const;

	/** 쿼리 실행 함수: 특정 컴포넌트들을 모두 포함하는 아키타입의 청크들을 순회 */
	void ForEachChunk(const TArray<FECSComponentTypeID>& ComponentTypes, TFunctionRef<void(FECSArchetypeChunk& Chunk)> Func);

private:
	FECSArchetype* GetOrCreateArchetype(const FECSArchetypeComposition& Composition);
	void MoveEntityToArchetype(FECSEntityHandle Entity, FECSArchetype* TargetArchetype);

	FECSEntityHandle AllocateEntityHandle();
	void ReleaseEntityHandle(FECSEntityHandle Handle);

	TArray<FEntityRecord> Entities; // 엔티티 정보 저장
	TArray<int32> FreeEntityIndices; // 재사용 가능한 엔티티 인덱스
	TMap<uint32, FECSArchetype*> Archetypes; // 아키타입 구성 해시 -> 아키타입 맵
	TMap<int32, int32> NextEntityGeneration; // 엔티티 Generation 관리
};

/*---------- ddd ----------*/
/**
 * @brief 위치를 나타내는 예시 컴포넌트입니다.
 */
USTRUCT(BlueprintType)
struct ZKGAME_API FPositionComponent : public FECSComponentBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ECS")
	FVector Position = FVector::ZeroVector;
};

/**
 * @brief 속도를 나타내는 예시 컴포넌트입니다.
 */
USTRUCT(BlueprintType)
struct ZKGAME_API FVelocityComponent : public FECSComponentBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ECS")
	FVector Velocity = FVector::ZeroVector;
};

/**
 * @brief 이동 속도를 정의하는 예시 컴포넌트입니다.
 */
USTRUCT(BlueprintType)
struct ZKGAME_API FMovementSpeedComponent : public FECSComponentBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ECS")
	float Speed = 100.0f;
};