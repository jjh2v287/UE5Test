// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// default 
DECLARE_LOG_CATEGORY_EXTERN(LogUK, Log, All);
// UK GameAbility
DECLARE_LOG_CATEGORY_EXTERN(LogUKGA, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogUKData, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogUKMarker, Log, All);

// UK UI
DECLARE_LOG_CATEGORY_EXTERN(LogUKUI, Log, All);

#if PLATFORM_WINDOWS
	#define __UK_FUNCTION__ TEXT(__FUNCTION__)
#else
	#define __UK_FUNCTION__ TEXT(__FILE__)
#endif

#define LOG_COMMON(LogType, Verbosity, Format, ...) UE_LOG(LogType, Verbosity, TEXT("[%s(%d)] ") TEXT(Format), __UK_FUNCTION__, __LINE__, ##__VA_ARGS__)
#define UK_LOG(Verbosity, Format, ...) UE_LOG(LogUK, Verbosity, TEXT("[%s(%d)] ") TEXT(Format), __UK_FUNCTION__, __LINE__, ##__VA_ARGS__)

#define UK_LOG_TICK(Format, ...) \
{ \
UE_LOG(LogUK, Display, TEXT(Format), ##__VA_ARGS__); \
GEngine->AddOnScreenDebugMessage(-1, -1.f, FColor::Green, FString::Printf(TEXT(Format), ##__VA_ARGS__)); \
}