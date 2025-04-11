// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UKECSEntity.h"
#include "Subsystems/WorldSubsystem.h"
#include "UKECSManager.generated.h"

class UUKECSSystemBase;

// --- ECS 매니저 클래스 ---
/** ECS 월드를 관리하는 중앙 클래스 */
UCLASS()
class ZKGAME_API UUKECSManager : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	static UUKECSManager* Get() { return Instance; }

	// USubsystem implementation Begin
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	// USubsystem implementation End

	// FTickableGameObject implementation Begin
	virtual TStatId GetStatId() const override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return true; }
	virtual bool IsTickableInEditor() const override { return false; }
	// FTickableGameObject implementation End

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

	/** 쿼리 실행 함수 */
	void ForEachChunk(const TArray<FECSComponentTypeID>& ComponentTypes, TFunctionRef<void(FECSArchetypeChunk& Chunk)> Func);

	void AddSystem(TSubclassOf<UUKECSSystemBase> ECSSystemBaseClass);

private:
	FECSArchetype* GetOrCreateArchetype(const FECSArchetypeComposition& Composition);
	void MoveEntityToArchetype(FECSEntityHandle Entity, FECSArchetype* TargetArchetype);

	FECSEntityHandle AllocateEntityHandle();
	void ReleaseEntityHandle(FECSEntityHandle Handle);

	static UUKECSManager* Instance;
	
	TArray<FEntityRecord> Entities;
	TArray<int32> FreeEntityIndices;
	TMap<uint32, TUniquePtr<FECSArchetype>> Archetypes;
	TMap<int32, int32> NextEntityGeneration;

	UPROPERTY(Transient)
	TMap<TSubclassOf<UUKECSSystemBase>, TObjectPtr<UUKECSSystemBase>> Systems;
};