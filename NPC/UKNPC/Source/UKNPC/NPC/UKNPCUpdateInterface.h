// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "UKNPCUpdateInterface.generated.h"

UINTERFACE(MinimalAPI)
class UUKNPCUpdateInterface : public UInterface
{
	GENERATED_BODY()
};

class UKNPC_API IUKNPCUpdateInterface
{
	GENERATED_BODY()

public:
	virtual void ManualTick(float DeltaTime) = 0;
};