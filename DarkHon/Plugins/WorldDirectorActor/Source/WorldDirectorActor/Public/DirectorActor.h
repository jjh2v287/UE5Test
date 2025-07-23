// Copyright 2024 BitProtectStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "WDActorComponent.h"
#include "GameFramework/Actor.h"
#include "Runtime/Core/Public/HAL/Runnable.h"
#include "Runtime/Core/Public/HAL/ThreadSafeBool.h"
#include "DirectorActor.generated.h"

USTRUCT(BlueprintType)
struct FWDAStructInBP
{
	GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director Actor Parameters")
	int indexNpc = 0;

	FWDAStructInBP()
	{
	};
};

USTRUCT(BlueprintType)
struct FWDActorStruct
{
	GENERATED_USTRUCT_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Director Actor Struct")
	FVector actorLocation = FVector::ZeroVector;
	UPROPERTY()
	FRotator actorRotation = FRotator::ZeroRotator;
	UPROPERTY()
	FVector actorScale = FVector::ZeroVector;

	UPROPERTY()
	FVector actorSpawnLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Director Actor Struct")
	bool bIsNearActor = false;
	
	UPROPERTY()
	UClass* classActor = nullptr;

	UPROPERTY()
	FString actorUniqName;

	FWDActorStruct()
	{
	};
};

USTRUCT(BlueprintType)
struct FWDAStruct
{
	GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, Category = "All Players Classes")
	TArray<TSubclassOf<AActor>> playersClassesArr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director Actor Parameters")
	float firstLayerRadius = 3000.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director Actor Parameters")
	float layerOffset = 500.f;

	FWDAStruct()
	{
	};
};

UCLASS()
class WORLDDIRECTORACTOR_API ADirectorActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADirectorActor();
// Called every frame
	virtual void Tick(float DeltaTime) override;

	//************************************************************************
	// Component                                                                  
	//************************************************************************

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Component")
	class UInstancedStaticMeshComponent* StaticMeshInstanceComponent;

	//************************************************************************

	bool RegisterActor(AActor *actorRef);

	UFUNCTION(BlueprintPure, Category = "Director Actor Struct")
    int GetBackgroundActorCount() const;

	// Set Enable Director Actor.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director Actor Parameters")
	bool bIsActivate = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director Actor Parameters")
	bool bIsDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director Actor Parameters")
	FWDAStruct directorParameters;

	UFUNCTION(BlueprintImplementableEvent, Category = "Director Actor Parameters")
	void InsertActorInBackground_BP(AActor* actorRef);
	UFUNCTION(BlueprintImplementableEvent, Category = "Director Actor Parameters")
	void RestoreActor_BP(int indexNpc, AActor* actorRef);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void BeginDestroy() override;

	void InsertActorInBackground(AActor* actorRef);

	void ExchangeInformationTimer();

private:

	TArray<FWDActorStruct> allBackgroundActorArr;

	TArray<FWDActorStruct> allThreadActors_Debug;

	TArray<FWDActorStruct> allActorArr_ForBP;

	UPROPERTY()
	TArray<AActor*> allRegisteredActorArr;

	class WorldDirectorActorThread* directorThreadRef;

	FTimerHandle exchangeInformation_Timer;
	float exchangeInformationRate = 0.2;

	int actorsInBackground = 0;

};

// Thread
class WorldDirectorActorThread : public FRunnable
{
public:

	//================================= THREAD =====================================
    
	//Constructor
	WorldDirectorActorThread(AActor *newActor, FWDAStruct setDirectorParameters);
	//Destructor
	virtual ~WorldDirectorActorThread() override;

	//Use this method to kill the thread!!
	void EnsureCompletion();
	//Pause the thread 
	void PauseThread();
	//Continue/UnPause the thread
	void ContinueThread();
	//FRunnable interface.
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;
	bool IsThreadPaused() const;

	//================================= THREAD =====================================

	TArray<FWDActorStruct> GetActorData(TArray<FWDActorStruct> setNewBackgroundActorsArr, TArray<FVector> setPlayersLocationsArr,TArray<FWDActorStruct> &getAllThreadActorsArr);
	
	AActor *ownerActor;

private:

	//================================= THREAD =====================================
	
	//Thread to run the worker FRunnable on
	FRunnableThread* Thread;

	FCriticalSection m_mutex;
	FEvent* m_semaphore;

	int m_chunkCount;
	int m_amount;
	int m_MinInt;
	int m_MaxInt;

	float threadSleepTime = 0.01f;

	//As the name states those members are Thread safe
	FThreadSafeBool m_Kill;
	FThreadSafeBool m_Pause;


	//================================= THREAD =====================================

	TArray<FWDActorStruct> allBackgroundActorsArrTH;

	TArray<FWDActorStruct> canRestoreActorsArrTH;

	class UNavigationSystemV1* navigationSystemTH = nullptr;

	// All found players locations. 
	TArray<FVector> playersLocationsArr;

	FWDAStruct directorParametersTH;

	float threadDeltaTime = 0.f;

	// Skip frame for performance.
	int skipFrameValue = 25;
	int nowFrame = 0;
};
