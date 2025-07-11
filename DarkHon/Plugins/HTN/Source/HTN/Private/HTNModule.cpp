// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "HTNModule.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "HTNTypes.h"

#define LOCTEXT_NAMESPACE "HTNModule"

/// Runtime module implementation
class FHTNModule : public IHTNModule
{
public:
	virtual void StartupModule() override
	{
		// Ensure that there is a key for own location on every blackboard. 
		OnUpdateBlackboardKeysDelegateHandle = UBlackboardData::OnUpdateKeys.AddStatic(&OnUpdateBlackboardKeys);
	}

	virtual void ShutdownModule() override
	{
		if (OnUpdateBlackboardKeysDelegateHandle.IsValid())
		{
			UBlackboardData::OnUpdateKeys.Remove(OnUpdateBlackboardKeysDelegateHandle);
			OnUpdateBlackboardKeysDelegateHandle.Reset();
		}
	}

private:

	static void OnUpdateBlackboardKeys(UBlackboardData* Asset)
	{
		check(IsValid(Asset));
		Asset->UpdatePersistentKey<UBlackboardKeyType_Vector>(FBlackboard::KeySelfLocation);
	}

	FDelegateHandle OnUpdateBlackboardKeysDelegateHandle;
};

IMPLEMENT_GAME_MODULE(FHTNModule, HTN);

#undef LOCTEXT_NAMESPACE