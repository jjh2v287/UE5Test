// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "HTNTypes.h"
#include "Misc/StringBuilder.h"
#include <atomic>

HTN_API DEFINE_LOG_CATEGORY(LogHTN);
HTN_API DEFINE_LOG_CATEGORY(LogHTNCurrentPlan);

DEFINE_STAT(STAT_AI_HTN_Tick);
DEFINE_STAT(STAT_AI_HTN_Planning);
DEFINE_STAT(STAT_AI_HTN_Execution);
DEFINE_STAT(STAT_AI_HTN_Cleanup);
DEFINE_STAT(STAT_AI_HTN_StopHTN);
DEFINE_STAT(STAT_AI_HTN_NodeInstantiation);
DEFINE_STAT(STAT_AI_HTN_NodeInstanceInitialization);
DEFINE_STAT(STAT_AI_HTN_NumAITask_MakeHTNPlan);
DEFINE_STAT(STAT_AI_HTN_NumProducedPlans);
DEFINE_STAT(STAT_AI_HTN_NumNodeInstancesCreated);

EHTNDecoratorTestResult CombineHTNDecoratorTestResults(EHTNDecoratorTestResult Lhs, EHTNDecoratorTestResult Rhs, bool bShouldPass)
{
	if (Lhs == EHTNDecoratorTestResult::NotTested)
	{
		return Rhs;
	}

	if (Rhs == EHTNDecoratorTestResult::NotTested)
	{
		return Lhs;
	}

	if (bShouldPass)
	{
		// Combine like AND, so all must pass for the result to pass.
		return FMath::Min(Lhs, Rhs);
	}
	else
	{
		// Combine like OR, so at least one must pass for the result to pass.
		return FMath::Max(Lhs, Rhs);
	}
}

bool HTNDecoratorTestResultToBool(EHTNDecoratorTestResult TestResult, bool bShouldPass)
{
	return TestResult != (bShouldPass ? EHTNDecoratorTestResult::Failed : EHTNDecoratorTestResult::Passed);
}

FHTNPlanInstanceID::FHTNPlanInstanceID() :
	ID(0)
{}

FHTNPlanInstanceID::FHTNPlanInstanceID(EGenerateNewIDType)
{
	static std::atomic<uint64> NextID { StaticCast<uint64>(1) };

	ID = ++NextID;

	// Check for the next-to-impossible event that we wrap round to 0, because 0 is the invalid ID.
	if (ID == 0)
	{
		ID = ++NextID;
	}
}

FString HTNLockFlagsToString(EHTNLockFlags Flags)
{
	TStringBuilder<1024> SB;

	SB << "(";

	bool bFirst = true;
	for (EHTNLockFlags Flag : TEnumRange<EHTNLockFlags>())
	{
		if (EnumHasAnyFlags(Flags, Flag))
		{	
			if (bFirst)
			{
				bFirst = false;
			}
			else
			{
				SB << TEXT(" | ");
			}

			SB << TEXT("'") << UEnum::GetDisplayValueAsText(Flag).ToString() << TEXT("'");
		}
	}

	SB << ")";

	return *SB;
}

FHTNScopedLock::FHTNScopedLock(EHTNLockFlags& LockFlags, EHTNLockFlags FlagToAdd) :
	LockFlags(LockFlags),
	FlagToAdd(FlagToAdd),
	bHadFlag(EnumHasAnyFlags(LockFlags, FlagToAdd))
{
	LockFlags |= FlagToAdd;
}

FHTNScopedLock::~FHTNScopedLock()
{
	if (!bHadFlag)
	{
		LockFlags &= ~FlagToAdd;
	}
}

const FHTNReplanParameters FHTNReplanParameters::Default = {};

FHTNReplanParameters::FHTNReplanParameters() :
	bForceAbortPlan(false),
	bForceRestartActivePlanning(false),
	bForceDeferToNextFrame(false),
	bReplanOutermostPlanInstance(false),
	bForceReplan(false),
	bMakeNewPlanRegardlessOfSubPlanSettings(true),
	PlanningType(EHTNPlanningType::Normal)
{}

FString FHTNReplanParameters::ToCompactString() const
{
	TStringBuilder<2048> SB;

	SB << TEXT("(DebugReason='") << DebugReason << TEXT("'");

#define HTN_DESCRIBE_BOOL_IF_NON_DEFAULT(Field) if (Field != Default.Field) { SB << TEXT(", ") << TEXT(#Field) << TEXT("=") << (Field ? TEXT("True") : TEXT("False")); }
	HTN_DESCRIBE_BOOL_IF_NON_DEFAULT(bForceAbortPlan);
	HTN_DESCRIBE_BOOL_IF_NON_DEFAULT(bForceRestartActivePlanning);
	HTN_DESCRIBE_BOOL_IF_NON_DEFAULT(bForceDeferToNextFrame);
	HTN_DESCRIBE_BOOL_IF_NON_DEFAULT(bReplanOutermostPlanInstance);
	HTN_DESCRIBE_BOOL_IF_NON_DEFAULT(bForceReplan);
	HTN_DESCRIBE_BOOL_IF_NON_DEFAULT(bMakeNewPlanRegardlessOfSubPlanSettings);
#undef HTN_DESCRIBE_BOOL_IF_NON_DEFAULT

	if (PlanningType != Default.PlanningType)
	{
		SB << TEXT(", PlanningType=") << UEnum::GetDisplayValueAsText(PlanningType).ToString();
	}

	SB << TEXT(")");

	return SB.ToString();
}

FString FHTNReplanParameters::ToTitleString() const
{
	if (PlanningType == EHTNPlanningType::TryToAdjustCurrentPlan)
	{
		return  bReplanOutermostPlanInstance ? TEXT("Try To Adjust Top-Level Plan") : TEXT("Try To Adjust Plan");
	}

	return bReplanOutermostPlanInstance ? TEXT("Replan Top-Level Plan") : TEXT("Replan");
}

FString FHTNReplanParameters::ToDetailedString() const
{
	TStringBuilder<2048> SB;

	const auto BoolToString = [](bool bBool) -> FString { return bBool ? TEXT("True") : TEXT("False"); };

	SB << TEXT("DebugReason: ") << DebugReason;
	SB << TEXT("\nForceAbortPlan: ") << BoolToString(bForceAbortPlan);
	SB << TEXT("\nForceRestartActivePlanning: ") << BoolToString(bForceRestartActivePlanning);
	SB << TEXT("\nForceDeferToNextFrame: ") << BoolToString(bForceDeferToNextFrame);
	SB << TEXT("\nReplanOutermostPlanInstance: ") << BoolToString(bReplanOutermostPlanInstance);
	SB << TEXT("\nForceReplan: ") << BoolToString(bForceReplan);
	SB << TEXT("\nMakeNewPlanRegardlessOfSubPlanSettings: ") << BoolToString(bMakeNewPlanRegardlessOfSubPlanSettings);
	SB << TEXT("\nPlanningType: ") << UEnum::GetDisplayValueAsText(PlanningType).ToString();

	return SB.ToString();
}

bool FHTNSubNodeGroup::CanDecoratorConditionsInterruptPlan() const
{
	return bIsIfNodeFalseBranch ? bCanConditionsInterruptFalseBranch : bCanConditionsInterruptTrueBranch;
}
