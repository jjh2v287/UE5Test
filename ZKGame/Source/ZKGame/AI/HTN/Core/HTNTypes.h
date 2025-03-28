// CoffeeHTNTypes.h - HTN 시스템에 필요한 데이터 타입 정의

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "HTNTypes.generated.h"

// 월드 상태를 저장하는 구조체
USTRUCT(BlueprintType)
struct FCoffeeWorldState
{
	GENERATED_BODY()

	// 재료 상태
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coffee")
	bool bHaveBeans = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coffee")
	bool bHaveGroundCoffee = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coffee")
	bool bHaveFilter = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coffee")
	bool bHaveWater = true;

	// 기기 상태
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coffee")
	bool bIsGrinderPluggedIn = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coffee")
	bool bIsGrinderReady = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coffee")
	bool bIsMachineClean = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coffee")
	bool bIsFilterInPlace = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coffee")
	bool bAreGroundsInFilter = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coffee")
	bool bIsMachineReady = false;

	// 결과
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coffee")
	bool bHaveBrewedCoffee = false;
};

// HTN 태스크 타입 열거형
UENUM(BlueprintType)
enum class EHTNTaskType : uint8
{
	Primitive,   // 직접 실행 가능한 태스크
	Compound     // 하위 태스크로 분해되는 태스크
};

// HTN 태스크 상태 열거형
UENUM(BlueprintType)
enum class EHTNTaskStatus : uint8
{
	Inactive,    // 아직 실행되지 않음
	InProgress,  // 현재 실행 중
	Succeeded,   // 성공적으로 완료
	Failed       // 실패
};