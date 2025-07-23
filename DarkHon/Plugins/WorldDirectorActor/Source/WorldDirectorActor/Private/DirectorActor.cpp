// Copyright 2024 BitProtectStudio. All Rights Reserved.


#include "DirectorActor.h"
#include "TimerManager.h"
#include "WDActorComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/PointLight.h"
#include "Runtime/Core/Public/HAL/RunnableThread.h"
#include "Engine.h"


// Sets default values
ADirectorActor::ADirectorActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	StaticMeshInstanceComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("StaticMeshInstanceComponent"));
	StaticMeshInstanceComponent->SetupAttachment(RootComponent);
	StaticMeshInstanceComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StaticMeshInstanceComponent->SetGenerateOverlapEvents(false);
	StaticMeshInstanceComponent->SetCastShadow(false);
}

// Called when the game starts or when spawned
void ADirectorActor::BeginPlay()
{
	Super::BeginPlay();

	// Run thread if has Authority.
	if (HasAuthority())
	{
		// Run thread.
		directorThreadRef = nullptr;
		directorThreadRef = new WorldDirectorActorThread(this, directorParameters);

		// Worker timers.
		GetWorldTimerManager().SetTimer(exchangeInformation_Timer, this, &ADirectorActor::ExchangeInformationTimer, exchangeInformationRate, true, exchangeInformationRate);
	}
	else
	{
		bIsActivate = false;
	}
}

void ADirectorActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (directorThreadRef)
	{
		directorThreadRef->EnsureCompletion();
		delete directorThreadRef;
		directorThreadRef = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void ADirectorActor::BeginDestroy()
{
	if (directorThreadRef)
	{
		directorThreadRef->EnsureCompletion();
		delete directorThreadRef;
		directorThreadRef = nullptr;
	}
	Super::BeginDestroy();
}

// Called every frame
void ADirectorActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ADirectorActor::InsertActorInBackground(AActor* actorRef)
{
	if (actorRef != nullptr)
	{
		FWDActorStruct newActor_;
		newActor_.actorLocation = actorRef->GetActorLocation();
		newActor_.actorRotation = actorRef->GetActorRotation();
		newActor_.classActor = actorRef->GetClass();
		newActor_.actorScale = actorRef->GetActorScale3D();

		UWDActorComponent* actorComp_ = Cast<UWDActorComponent>(actorRef->FindComponentByClass(UWDActorComponent::StaticClass()));
		if (actorComp_)
		{
			actorComp_->BroadcastOnPrepareForOptimization();

			// Save uniq name actor.
			if (actorComp_->GetActorUniqName() == "")
			{
				newActor_.actorUniqName = actorRef->GetName();
			}
			else
			{
				newActor_.actorUniqName = actorComp_->GetActorUniqName();
			}
		}

		// Add to background.
		allBackgroundActorArr.Add(newActor_);

		// Add to array for find actor struct in BP.
		allActorArr_ForBP.Add(newActor_);
		InsertActorInBackground_BP(actorRef);

		// Destroy actor.
		actorRef->Destroy();
	}
}

void ADirectorActor::ExchangeInformationTimer()
{
	if (!bIsActivate)
	{
		return;
	}

	TArray<AActor*> allRegPawnsArrTemp_;
	TArray<APawn*> allPlayersFoundArr_;
	TArray<FVector> allPlayersLocationsArr_;

	// Find all players.
	for (int i = 0; i < directorParameters.playersClassesArr.Num(); ++i)
	{
		TArray<AActor*> allActorsOfClassArr_;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), directorParameters.playersClassesArr[i], allActorsOfClassArr_);

		for (int pawnID_ = 0; pawnID_ < allActorsOfClassArr_.Num(); ++pawnID_)
		{
			APawn* newPlayer_ = Cast<APawn>(allActorsOfClassArr_[pawnID_]);
			if (newPlayer_)
			{
				if (!allPlayersFoundArr_.Contains(newPlayer_))
				{
					allPlayersFoundArr_.AddUnique(newPlayer_);
					allPlayersLocationsArr_.Add(newPlayer_->GetActorLocation());
				}
			}
		}
	}

	if (allPlayersFoundArr_.Num() == 0)
	{
		return;
	}

	TArray<bool> isCanSetInBackgroundArr_;
	isCanSetInBackgroundArr_.SetNum(allRegisteredActorArr.Num());

	// Move far Actor to Background.
	for (int registerActorID_ = 0; registerActorID_ < allRegisteredActorArr.Num(); ++registerActorID_)
	{
		bool bIsCanSetToBackground_(true);

		for (int playerID_ = 0; playerID_ < allPlayersLocationsArr_.Num(); ++playerID_)
		{
			FVector playerLocation_ = allPlayersLocationsArr_[playerID_];
			if (IsValid(allRegisteredActorArr[registerActorID_]))
			{
				if ((allRegisteredActorArr[registerActorID_]->GetActorLocation() - playerLocation_).Size() < directorParameters.firstLayerRadius + directorParameters.layerOffset)
				{
					bIsCanSetToBackground_ = false;
				}
			}
		}

		if (bIsCanSetToBackground_ && IsValid(allRegisteredActorArr[registerActorID_]))
		{
			InsertActorInBackground(allRegisteredActorArr[registerActorID_]);
			++actorsInBackground;
		}
		else if (IsValid(allRegisteredActorArr[registerActorID_]))
		{
			allRegPawnsArrTemp_.Add(allRegisteredActorArr[registerActorID_]);
		}
	}

	allRegisteredActorArr = allRegPawnsArrTemp_;

	// Get Actor Data.
	TArray<FWDActorStruct> restoreActor_ = directorThreadRef->GetActorData(allBackgroundActorArr, allPlayersLocationsArr_, allThreadActors_Debug);
	actorsInBackground -= restoreActor_.Num();

	if (bIsDebug)
	{
		if (StaticMeshInstanceComponent->GetInstanceCount() != allThreadActors_Debug.Num())
		{
			StaticMeshInstanceComponent->ClearInstances();
			for (int i = 0; i < allThreadActors_Debug.Num(); ++i)
			{
				FTransform newInstanceTransform_;
				newInstanceTransform_.SetLocation(allThreadActors_Debug[i].actorLocation);
				StaticMeshInstanceComponent->AddInstance(newInstanceTransform_,true);
			}
		}
		else
		{
			for (int i = 0; i < allThreadActors_Debug.Num(); ++i)
			{
				FTransform newInstanceTransform_;
				newInstanceTransform_.SetLocation(allThreadActors_Debug[i].actorLocation);

				StaticMeshInstanceComponent->UpdateInstanceTransform(i, newInstanceTransform_, true, false);
			}
		}
	}
	else
	{
		if (StaticMeshInstanceComponent->GetInstanceCount() > 0)
		{
			StaticMeshInstanceComponent->ClearInstances();
		}
	}

	StaticMeshInstanceComponent->MarkRenderStateDirty();

	allBackgroundActorArr.Empty();

	for (int restoreID_ = 0; restoreID_ < restoreActor_.Num(); ++restoreID_)
	{
		FActorSpawnParameters spawnInfo_;
		spawnInfo_.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		// Save transform.
		FTransform spawnTransform_;
		spawnTransform_.SetLocation(restoreActor_[restoreID_].actorLocation);
		spawnTransform_.SetRotation(restoreActor_[restoreID_].actorRotation.Quaternion());
		spawnTransform_.SetScale3D(restoreActor_[restoreID_].actorScale);

		AActor* restoredActor_ = GetWorld()->SpawnActor<AActor>(restoreActor_[restoreID_].classActor, spawnTransform_, spawnInfo_);
		if (restoredActor_)
		{
			// Fix actor scale.
			restoredActor_->SetActorScale3D(restoreActor_[restoreID_].actorScale);

			UWDActorComponent* actorComp_ = Cast<UWDActorComponent>(restoredActor_->FindComponentByClass(UWDActorComponent::StaticClass()));

			if (actorComp_)
			{
				// Restore uniq name of Actor.
				actorComp_->SetActorUniqName(restoreActor_[restoreID_].actorUniqName);

				actorComp_->BroadcastOnRecoveryFromOptimization();

				for (int findID_ = 0; findID_ < allActorArr_ForBP.Num(); ++findID_)
				{
					if (allActorArr_ForBP[findID_].actorUniqName == restoreActor_[restoreID_].actorUniqName)
					{
						RestoreActor_BP(findID_, restoredActor_);
						allActorArr_ForBP.RemoveAt(findID_);
					}
				}
			}
		}
	}
}

bool ADirectorActor::RegisterActor(AActor* actorRef)
{
	bool bIsReg_ = false;
	if (actorRef)
	{
		allRegisteredActorArr.Add(actorRef);
		bIsReg_ = true;
	}
	return bIsReg_;
}

int ADirectorActor::GetBackgroundActorCount() const
{
	return actorsInBackground;
}

WorldDirectorActorThread::WorldDirectorActorThread(AActor* newActor, FWDAStruct setDirectorParameters)
{
	m_Kill = false;
	m_Pause = false;

	//Initialize FEvent (as a cross platform (Confirmed Mac/Windows))
	m_semaphore = FGenericPlatformProcess::GetSynchEventFromPool(false);

	ownerActor = newActor;
	directorParametersTH = setDirectorParameters;

	Thread = FRunnableThread::Create(this, (TEXT("%s_FSomeRunnable"), *newActor->GetName()), 0, TPri_Lowest);
}

WorldDirectorActorThread::~WorldDirectorActorThread()
{
	if (m_semaphore)
	{
		//Cleanup the FEvent
		FGenericPlatformProcess::ReturnSynchEventToPool(m_semaphore);
		m_semaphore = nullptr;
	}

	if (Thread)
	{
		//Cleanup the worker thread
		delete Thread;
		Thread = nullptr;
	}
}

void WorldDirectorActorThread::EnsureCompletion()
{
	Stop();

	if (Thread)
	{
		Thread->WaitForCompletion();
	}
}

void WorldDirectorActorThread::PauseThread()
{
	m_Pause = true;
}

void WorldDirectorActorThread::ContinueThread()
{
	m_Pause = false;

	if (m_semaphore)
	{
		//Here is a FEvent signal "Trigger()" -> it will wake up the thread.
		m_semaphore->Trigger();
	}
}

bool WorldDirectorActorThread::Init()
{
	return true;
}

uint32 WorldDirectorActorThread::Run()
{
	//Initial wait before starting
	FPlatformProcess::Sleep(0.5f);

	threadDeltaTime = 0.f;

	while (!m_Kill)
	{
		if (m_Pause)
		{
			//FEvent->Wait(); will "sleep" the thread until it will get a signal "Trigger()"
			m_semaphore->Wait();

			if (m_Kill)
			{
				return 0;
			}
		}
		else
		{
			uint64 const timePlatform = FPlatformTime::Cycles64();

			m_mutex.Lock();

			TArray<FWDActorStruct> allBackgroundActorsArrTemp_ = allBackgroundActorsArrTH;
			TArray<FVector> playersLocationsArrTemp_ = playersLocationsArr;

			allBackgroundActorsArrTH.Empty();

			TArray<FWDActorStruct> stayInBackgroundActorsArrMem_;
			TArray<FWDActorStruct> restoreActorsArr_;
			TArray<FWDActorStruct> nearActors_;

			m_mutex.Unlock();

			// Check distance between players and background Actors. And sort by layers.
			for (int checkActorID_ = 0; checkActorID_ < allBackgroundActorsArrTemp_.Num(); ++checkActorID_)
			{
				bool sorted_(false);
				bool bIsSelectSecondLayer_(false);

				for (int playerLocID_ = 0; playerLocID_ < playersLocationsArrTemp_.Num(); ++playerLocID_)
				{
					float distanceToActor_ = (allBackgroundActorsArrTemp_[checkActorID_].actorLocation - playersLocationsArrTemp_[playerLocID_]).Size();

					// Check distance and sort by layers.
					if (distanceToActor_ < directorParametersTH.firstLayerRadius)
					{
						restoreActorsArr_.Add(allBackgroundActorsArrTemp_[checkActorID_]);
						sorted_ = true;
						break;
					}
					else //if (distanceToActor_ < directorParametersTH.secondLayerRadius)
					{
						bIsSelectSecondLayer_ = true;
					}
				}

				if (bIsSelectSecondLayer_ && !sorted_)
				{
					allBackgroundActorsArrTemp_[checkActorID_].bIsNearActor = true;
					nearActors_.Add(allBackgroundActorsArrTemp_[checkActorID_]);
					sorted_ = true;
				}
				else if (!sorted_) // If not sorted by layers.
				{
					allBackgroundActorsArrTemp_[checkActorID_].bIsNearActor = false;
					stayInBackgroundActorsArrMem_.Add(allBackgroundActorsArrTemp_[checkActorID_]);
				}
			}

			if (nowFrame > skipFrameValue)
			{
				// Reset skip frame.
				nowFrame = 0;
			}
			else
			{
				++nowFrame;
			}

			// Append array for return data actor.
			stayInBackgroundActorsArrMem_.Append(nearActors_);

			m_mutex.Lock();

			// Save data.
			allBackgroundActorsArrTH.Append(stayInBackgroundActorsArrMem_);

			canRestoreActorsArrTH.Append(restoreActorsArr_);

			m_mutex.Unlock();

			threadDeltaTime = FTimespan(FPlatformTime::Cycles64() - timePlatform).GetTotalSeconds();

			threadDeltaTime += threadSleepTime;

			//A little sleep between the chunks (So CPU will rest a bit -- (may be omitted))
			FPlatformProcess::Sleep(threadSleepTime);
		}
	}
	return 0;
}

void WorldDirectorActorThread::Stop()
{
	m_Kill = true; //Thread kill condition "while (!m_Kill){...}"
	m_Pause = false;

	if (m_semaphore)
	{
		//We shall signal "Trigger" the FEvent (in case the Thread is sleeping it shall wake up!!)
		m_semaphore->Trigger();
	}
}

bool WorldDirectorActorThread::IsThreadPaused() const
{
	return (bool)m_Pause;
}

TArray<FWDActorStruct> WorldDirectorActorThread::GetActorData(TArray<FWDActorStruct> setNewBackgroundActorsArr, TArray<FVector> setPlayersLocationsArr, TArray<FWDActorStruct>& getAllThreadActorsArr)
{
	m_mutex.Lock();

	allBackgroundActorsArrTH.Append(setNewBackgroundActorsArr);

	getAllThreadActorsArr = allBackgroundActorsArrTH;

	playersLocationsArr = setPlayersLocationsArr;

	TArray<FWDActorStruct> canRestoreActorsTemp_ = canRestoreActorsArrTH;
	canRestoreActorsArrTH.Empty();

	m_mutex.Unlock();

	return canRestoreActorsTemp_;
}
