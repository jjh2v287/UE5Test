// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LandscapeTestActor.generated.h"


namespace UKStringUtil
{
	inline const FString Empty;
	
	inline FString GetActorDisplayName(const FStringView InActorPath)
	{
		FString ActorPath{ InActorPath.GetData() };
		{
			const TCHAR* Delim{ TEXT("_C") };
			if (const int32 Index{ InActorPath.Find(Delim)}; Index != INDEX_NONE)
			{
				ActorPath.RemoveAt(Index, ActorPath.Len() - Index, EAllowShrinking::No);
				return ActorPath;
			}
		}
		{
			TArray<FString> SplitPath;
			const TCHAR* Delim{ TEXT("_") };
			ActorPath.ParseIntoArray(SplitPath, Delim);
			if (!SplitPath.IsEmpty() && SplitPath.Last().IsNumeric())
			{
				ActorPath.RemoveFromEnd(Delim + SplitPath.Last());
			}
		}
		
		return ActorPath;
	}

	inline FString GetActorUID(const FStringView InActorPath)
	{
		static const FString UIDString{TEXT("_UAID_")};
		if (const int32 Index{UE::String::FindLast(InActorPath, UIDString)}; Index != INDEX_NONE)
		{
			return FString{InActorPath.RightChop(Index + UIDString.Len())};
		}
		return GetActorDisplayName(InActorPath);
	}
	
	inline FString GetActorUID(const AActor* Actor)
	{
		return IsValid(Actor) ? GetActorUID(Actor->GetName()) : Empty;
	}
}

UCLASS()
class DARKHON_API ALandscapeTestActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ALandscapeTestActor();

#if WITH_EDITOR
	virtual void PostActorCreated() override; // 에디터에서 새로 배치/복제
	virtual void PostLoad() override;         // 디스크 로드(에디터/쿠킹 경로)
#endif
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category="Stream")
	static FString GetStableKey(const AActor* A); // 대안: 패키지명 키

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(DisplayPriority = -1))
	FName ActorUID{ NAME_None };
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stream")
	FGuid PersistentId; // 에디터 배치 시 미리 지정 가능, 런타임 스폰 시 NewGuid()
};
