// Fill out your copyright notice in the Description page of Project Settings.

#include "UKTickIntervalSubsystem.h"
#include "TimerManager.h"
#include "Camera/CameraComponent.h"

void UKTickIntervalSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Instance = this;

	TickIntervalThreadRef = nullptr;
	TickIntervalThreadRef = new TickIntervalThread();
}

void UKTickIntervalSubsystem::Deinitialize()
{
	Super::Deinitialize();

	if (TickIntervalThreadRef)
	{
		TickIntervalThreadRef->EnsureCompletion();
		delete TickIntervalThreadRef;
		TickIntervalThreadRef = nullptr;
	}
	
	Instance = nullptr;
}

ETickableTickType UKTickIntervalSubsystem::GetTickableTickType() const
{
	if (IsTemplate())
	{
		return ETickableTickType::Never;
	}
	return ETickableTickType::Always;
}

void UKTickIntervalSubsystem::Tick(float DeltaTime)
{
	UpdateTargetActorTick(DeltaTime);
}

void UKTickIntervalSubsystem::RegisterFocus(UUKTickIntervalFocusComponent* Focus)
{
	if(Focus == nullptr)
	{
		return;
	}

	check(Focus->FocusIndex == INDEX_NONE);
	
	Focus->FocusIndex = Focuses.Num();
	Focuses.Emplace(Focus);

	FUKFocusTickIntervalInfo Info;
	Info.Owner = Focus->GetOwner();
	Info.ActorLocation = Focus->GetOwner()->GetActorLocation();
	Info.Index = Focus->FocusIndex;
	Info.TickIntervalSettings = Focus->TickIntervalSettings;
	FocusTickIntervalInfos.Emplace(Info);
}

void UKTickIntervalSubsystem::UnregisterFocus(UUKTickIntervalFocusComponent* Focus)
{
	if(Focus == nullptr)
	{
		return;
	}
	
	check(Focus->FocusIndex != INDEX_NONE);

	const int32 Index = Focus->FocusIndex;
	Focus->FocusIndex = INDEX_NONE;

	Focuses.RemoveAtSwap(Index);
	if (Index < Focuses.Num())
	{
		Focuses[Index]->FocusIndex = Index;
	}

	FocusTickIntervalInfos.RemoveAtSwap(Index);
	if (Index < FocusTickIntervalInfos.Num())
	{
		FocusTickIntervalInfos[Index].Index = Index;
	}
}

void UKTickIntervalSubsystem::RegisterTarget(UUKTickIntervalTargetComponent* Target)
{
	if(Target == nullptr)
	{
		return;
	}

	check(Target->TargetIndex == INDEX_NONE);
	
	Target->TargetIndex = Targets.Num();
	Targets.Emplace(Target);

	FUKTargetTickIntervalInfo Info;
	Info.Owner = Target->GetOwner();
	Info.ActorLocation = Target->GetOwner()->GetActorLocation();
	Info.Index = Target->TargetIndex;
	Info.TickIntervalLayer = Target->TickIntervalLayer;
	TargetTickIntervalInfos.Emplace(Info);
}

void UKTickIntervalSubsystem::UnregisterTarget(UUKTickIntervalTargetComponent* Target)
{
	if(Target == nullptr)
	{
		return;
	}
	
	check(Target->TargetIndex != INDEX_NONE);

	const int Index = Target->TargetIndex;
	Target->TargetIndex = INDEX_NONE;

	Targets.RemoveAtSwap(Index);
	if (Index < Targets.Num())
	{
		Targets[Index]->TargetIndex = Index;
	}

	TargetTickIntervalInfos.RemoveAtSwap(Index);
	if (Index < TargetTickIntervalInfos.Num())
	{
		TargetTickIntervalInfos[Index].Index = Index;
	}
}

void UKTickIntervalSubsystem::UpdateTargetActorTick(float DeltaTime)
{
	static const bool IsThread = true;
#pragma region Thread
	if(IsThread)
	{
		const TArray<FUKTargetTickIntervalInfo> Infos = TickIntervalThreadRef->GetActorData(FocusTickIntervalInfos, TargetTickIntervalInfos);
		for (const FUKTargetTickIntervalInfo& Info : Infos)
		{
			TargetTickIntervalInfos[Info.Index].TickIntervalLayer = Info.TickIntervalLayer;
			TargetTickIntervalInfos[Info.Index].TickIntervalTime = Info.TickIntervalTime;
			Targets[Info.Index]->TickIntervalLayer = Info.TickIntervalLayer;
		
			Targets[Info.Index]->SetTickInterval(Info.TickIntervalTime);
		}
	}
#pragma endregion Thread

#pragma region NO Thread
	if(!IsThread)
	{
		for (const UUKTickIntervalFocusComponent* Focuse : Focuses)
		{
			FVector FocusLocation = Focuse->GetOwner()->GetActorLocation();
			for (UUKTickIntervalTargetComponent* TargetInfo : Targets)
			{
				FVector TargetLocation = TargetInfo->GetOwner()->GetActorLocation();
				float Distance = FVector::Distance(FocusLocation, TargetLocation);
				TTuple<int32, float, bool> TimeAndEnable = GetTimeAndEnableToDistance(Focuse->TickIntervalSettings, Distance);

				TargetInfo->SetTickInterval(TimeAndEnable.Get<1>());
			}
		}
	}
#pragma endregion NO Thread
}

const TTuple<int32, float, bool> UKTickIntervalSubsystem::GetTimeAndEnableToDistance(const TArray<FUKTickIntervalSettings>& TickIntervalSettings, const float Distance)
{
	TTuple<int32, float, bool> TimeAndEnable = {-1, 0.0f, true};

	int32 index = 0;
	for (const FUKTickIntervalSettings& OptLevel : TickIntervalSettings)
	{
		if(Distance >= OptLevel.IntervalDistance)
		{
			TimeAndEnable.Get<0>() = index;
			TimeAndEnable.Get<1>() = OptLevel.IntervalTime;
			TimeAndEnable.Get<2>() = OptLevel.EnabledVisible;
			break;
		}
		index++;
	}
	
	return TimeAndEnable;
}

#pragma region TickIntervalThread
TickIntervalThread::TickIntervalThread()
{
	m_Kill = false;
	m_Pause = false;

	m_semaphore = FGenericPlatformProcess::GetSynchEventFromPool(false);

	Thread = FRunnableThread::Create(this, TEXT("TickIntervalThread_FSomeRunnable"), 0, TPri_Lowest);
}

TickIntervalThread::~TickIntervalThread()
{
	if (m_semaphore)
	{
		FGenericPlatformProcess::ReturnSynchEventToPool(m_semaphore);
		m_semaphore = nullptr;
	}

	if (Thread)
	{
		delete Thread;
		Thread = nullptr;
	}
}

bool TickIntervalThread::Init()
{
	return FRunnable::Init();
}

uint32 TickIntervalThread::Run()
{
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

			// Data Copy
			m_mutex.Lock();
			TArray<FUKFocusTickIntervalInfo> FocusInfosTemp = FocusTickIntervalInfos;
			TArray<FUKTargetTickIntervalInfo> TargetInfosTemp = TargetTickIntervalInfos;
			TArray<FUKTargetTickIntervalInfo> NewCluster;
			FocusTickIntervalInfos.Empty();
			TargetTickIntervalInfos.Empty();
			m_mutex.Unlock();

			// Data Calculate
			for (int FocusID = 0; FocusID < FocusInfosTemp.Num(); ++FocusID)
			{
				TArray<FUKTickIntervalSettings>& Settings = FocusInfosTemp[FocusID].TickIntervalSettings;
				FVector FocusLocation = FocusInfosTemp[FocusID].ActorLocation;
				if(FocusInfosTemp[FocusID].Owner != nullptr)
				{
					FocusLocation = FocusInfosTemp[FocusID].Owner->GetActorLocation();
				}
				
				for (int TargetID = 0; TargetID < TargetInfosTemp.Num(); ++TargetID)
				{
					FVector TargetLocation = TargetInfosTemp[TargetID].ActorLocation;
					if(TargetInfosTemp[TargetID].Owner != nullptr)
					{
						TargetLocation = TargetInfosTemp[TargetID].Owner->GetActorLocation();
					}
					
					float Distance = FVector::Distance(FocusLocation, TargetLocation);
					const TTuple<int32, float, bool> TimeAndEnable = GetTimeAndEnableToDistance(Settings, Distance);
					
					if(TimeAndEnable.Get<0>() != TargetInfosTemp[TargetID].TickIntervalLayer)
					{
						TargetInfosTemp[TargetID].TickIntervalLayer = TimeAndEnable.Get<0>();
						TargetInfosTemp[TargetID].TickIntervalTime = TimeAndEnable.Get<1>();
						NewCluster.Emplace(TargetInfosTemp[TargetID]);
					}
				}
			}

			// Save data.
			m_mutex.Lock();
			TickIntervalInfoCluster.Append(NewCluster);
			m_mutex.Unlock();

			threadDeltaTime = FTimespan(FPlatformTime::Cycles64() - timePlatform).GetTotalSeconds();

			threadDeltaTime += threadSleepTime;

			//A little sleep between the chunks (So CPU will rest a bit -- (may be omitted))
			FPlatformProcess::Sleep(threadSleepTime);
		}
	}
	return 0;
}

void TickIntervalThread::Stop()
{
	FRunnable::Stop();

	m_Kill = true;
	m_Pause = false;

	if (m_semaphore)
	{
		m_semaphore->Trigger();
	}
}

void TickIntervalThread::Exit()
{
	FRunnable::Exit();
}

bool TickIntervalThread::IsThreadPaused() const
{
	return (bool)m_Pause;
}

void TickIntervalThread::EnsureCompletion()
{
	Stop();

	if (Thread)
	{
		Thread->WaitForCompletion();
	}
}

void TickIntervalThread::PauseThread()
{
	m_Pause = true;
}

void TickIntervalThread::ContinueThread()
{
	m_Pause = false;

	if (m_semaphore)
	{
		m_semaphore->Trigger();
	}
}

const TArray<FUKTargetTickIntervalInfo> TickIntervalThread::GetActorData(TArray<FUKFocusTickIntervalInfo> FocusInfos, TArray<FUKTargetTickIntervalInfo> TargetInfos)
{
	// Data Copy
	m_mutex.Lock();
	FocusTickIntervalInfos = FocusInfos;
	TargetTickIntervalInfos = TargetInfos;
	TArray<FUKTargetTickIntervalInfo> CopyData = TickIntervalInfoCluster;
	TickIntervalInfoCluster.Empty();
	m_mutex.Unlock();

	return CopyData;
}

const TTuple<int32, float, bool> TickIntervalThread::GetTimeAndEnableToDistance(const TArray<FUKTickIntervalSettings>& TickIntervalSettings, const float Distance)
{
	TTuple<int32, float, bool> TimeAndEnable = {-1, 0.0f, true};

	int32 index = 0;
	for (const FUKTickIntervalSettings& OptLevel : TickIntervalSettings)
	{
		if(Distance >= OptLevel.IntervalDistance)
		{
			TimeAndEnable.Get<0>() = index;
			TimeAndEnable.Get<1>() = OptLevel.IntervalTime;
			TimeAndEnable.Get<2>() = OptLevel.EnabledVisible;
			break;
		}
		index++;
	}
	
	return TimeAndEnable;
}
#pragma endregion TickIntervalThread
