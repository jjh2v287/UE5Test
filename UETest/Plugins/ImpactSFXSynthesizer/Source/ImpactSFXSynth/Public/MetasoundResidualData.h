// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#pragma once

#include "MetasoundDataReference.h"
#include "MetasoundDataTypeRegistrationMacro.h"
#include "ResidualData.h"

namespace Metasound
{
	
	// Forward declare ReadRef
	class FResidualData;
	typedef TDataReadReference<FResidualData> FResidualDataReadRef;
	
	// Metasound data type that holds onto a weak ptr. Mostly used as a placeholder until we have a proper proxy type.
	class IMPACTSFXSYNTH_API FResidualData
	{
		FResidualDataAssetProxyPtr Proxy;
		
	public:
	
		FResidualData()
			: Proxy(nullptr)
		{
		}
	
		FResidualData(const FResidualData& InOther)
			: Proxy(InOther.Proxy)
		{
		}
	
		FResidualData& operator=(const FResidualData& InOther)
		{
			Proxy = InOther.Proxy;
			return *this;
		}
	
		FResidualData& operator=(FResidualData&& InOther)
		{
			Proxy = MoveTemp(InOther.Proxy);
			return *this;
		}
	
		FResidualData(const TSharedPtr<Audio::IProxyData>& InInitData)
		{
			if (InInitData.IsValid())
			{
				if (InInitData->CheckTypeCast<FResidualDataAssetProxy>())
				{
					Proxy = MakeShared<FResidualDataAssetProxy, ESPMode::ThreadSafe>(InInitData->GetAs<FResidualDataAssetProxy>());
				}
			}
		}
	
		bool IsValid() const
		{
			return Proxy.IsValid();
		}
	
		const FResidualDataAssetProxyPtr& GetProxy() const
		{
			return Proxy;
		}
	
		const FResidualDataAssetProxy* operator->() const
		{
			return Proxy.Get();
		}
	
		FResidualDataAssetProxy* operator->()
		{
			return Proxy.Get();
		}
	};
	
	DECLARE_METASOUND_DATA_REFERENCE_TYPES(FResidualData, IMPACTSFXSYNTH_API, FResidualDataTypeInfo, FResidualDataReadRef, FResidualDataWriteRef)
}
