// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKInputComponent.h"
#include "Common/UKCommonLog.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UKInputComponent)

const FUKInputBufferData FUKInputBufferData::EmptyData;

void UUKInputComponent::SetupInputConfig(const UUKInputConfig* InputConfig)
{
	for (const FUKInputAction& Action : InputConfig->AbilityInputActions)
	{
		InputActions.Emplace(Action.InputTag, Action);
	}
	
	for (const FUKInputAction& Action : InputConfig->InteractionInputActions)
	{
		InputActions.Emplace(Action.InputTag, Action);
	}

	BindInputTagActions(InputConfig, this, &ThisClass::OnInputActionTriggered, &ThisClass::OnInputActionCompleted);
}

const FUKInputBufferData& UUKInputComponent::GetInputBufferDataByTag(const FGameplayTag& InputTag) const
{
	const FUKInputBufferData* FindResult = InputBuffer.Find(InputTag);
	return FindResult ? *FindResult : FUKInputBufferData::EmptyData;
}

void UUKInputComponent::OnInputActionTriggered(FGameplayTag InputTag)
{
	if (HeldInputs.Contains(InputTag))
	{
		return;
	}

	TriggeredInputs.Emplace(InputTag);
	AddBuffer(InputTag, InputActions[InputTag]);

	HeldInputs.Emplace(InputTag);
}

void UUKInputComponent::OnInputActionCompleted(FGameplayTag InputTag)
{	
	HeldInputs.Remove(InputTag);
	CompletedInputs.Emplace(InputTag);

	if (const FUKInputBufferData* BufferData = InputBuffer.Find(InputTag))
	{
		if (!BufferData->ExpireTime.IsSet() || BufferData->ReleasePolicy == EUKInputBufferReleasePolicy::RefreshOnInputRelease)
		{
			RemoveBuffer(InputTag);	
		}
	}
}

void UUKInputComponent::AddBuffer(const FGameplayTag& InputTag, const FUKInputAction& Action)
{
	//UK_LOG(Log, "[Input] Buffer Added : [%s]", *InputTag.ToString());
	
	FUKInputBufferData NewData;	
	NewData.ExpirationPolicy = Action.ExpirationPolicy;
	NewData.ReleasePolicy = Action.ReleasePolicy;
	if (Action.bEnableInputBuffer)
	{
		NewData.ExpireTime = GetWorld()->GetTimeSeconds() + Action.InputBufferTime;	
	}
	
	InputBuffer.Emplace(InputTag, NewData);
}

bool UUKInputComponent::RemoveBuffer(const FGameplayTag& InputTag)
{
	//UK_LOG(Log, "[Input] Buffer Removed : [%s]", *InputTag.ToString());
	if (InputBuffer.Contains(InputTag))
	{
		return InputBuffer.Remove(InputTag) != 0;
	}

	return false;
}

bool UUKInputComponent::ConsumeTriggeredInput(const FGameplayTag& InputTag)
{
	const FUKInputBufferData* BufferData = InputBuffer.Find(InputTag);

	if (BufferData == nullptr)
	{
		return false;
	}

	if (!BufferData->ExpireTime.IsSet() || BufferData->ExpirationPolicy == EUKInputBufferExpirationPolicy::RefreshOnSuccessfulActivation)
	{
		RemoveBuffer(InputTag);
	}

	return true;
}

void UUKInputComponent::VisitTriggeredInput(TFunction<void(const FGameplayTag&)>&& Visitor)
{
	for (const FGameplayTag& Tag : TriggeredInputs)
	{
		Visitor(Tag);
	}
}

void UUKInputComponent::VisitBufferedInput(TFunction<bool(const FGameplayTag&, bool)>&& Consumer)
{
	for (const auto& [InputTag, BufferData] : InputBuffer)
	{
 		if (!Consumer(InputTag, HeldInputs.Contains(InputTag)))
		{
			continue;
		}
		
		if (!BufferData.ExpireTime.IsSet() || BufferData.ExpirationPolicy == EUKInputBufferExpirationPolicy::RefreshOnSuccessfulActivation)
		{
			ExpiredInputBuffer.Emplace(InputTag);
		}
	}

	EmptyExpiredInput();
}

void UUKInputComponent::VisitCompletedInput(TFunction<void(const FGameplayTag&)>&& Visitor)
{
	for (const FGameplayTag& Tag : CompletedInputs)
	{
		Visitor(Tag);
	}
}

void UUKInputComponent::UpdateBuffer()
{
	const float CurrentTimeSeconds = GetWorld()->GetTimeSeconds();

	for (const auto& [Key, Value] : InputBuffer)
	{
		if (!Value.ExpireTime.IsSet() || Value.ExpireTime.GetValue() <= CurrentTimeSeconds)
		{
			ExpiredInputBuffer.Emplace(Key);
		}
	}

	EmptyExpiredInput();
	TriggeredInputs.Empty();
	CompletedInputs.Empty();
}

void UUKInputComponent::EmptyExpiredInput()
{
	for (const FGameplayTag& One : ExpiredInputBuffer)
	{
		RemoveBuffer(One);
	}
	
	ExpiredInputBuffer.Empty();
}