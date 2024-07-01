// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#pragma once

#include "MetasoundDataReference.h"
#include "MetasoundDataTypeRegistrationMacro.h"
#include "MultiImpactData.h"

namespace Metasound
{
	// Forward declare ReadRef
	class FMultiImpactData;
	typedef TDataReadReference<FMultiImpactData> FMultiImpactDataReadRef;
	
	// Metasound data type that holds onto a weak ptr. Mostly used as a placeholder until we have a proper proxy type.
	class IMPACTSFXSYNTH_API FMultiImpactData
	{
		FMultiImpactDataAssetProxyPtr Proxy;
		
	public:
	
		FMultiImpactData()
			: Proxy(nullptr)
		{
		}
	
		FMultiImpactData(const FMultiImpactData& InOther)
			: Proxy(InOther.Proxy)
		{
		}
	
		FMultiImpactData& operator=(const FMultiImpactData& InOther)
		{
			Proxy = InOther.Proxy;
			return *this;
		}
		
		FMultiImpactData& operator=(FMultiImpactData&& InOther)
		{
			Proxy = MoveTemp(InOther.Proxy);
			return *this;
		}
	
		FMultiImpactData(const TSharedPtr<Audio::IProxyData>& InInitData)
		{
			if (InInitData.IsValid())
			{
				if (InInitData->CheckTypeCast<FMultiImpactDataAssetProxy>())
				{
					Proxy = MakeShared<FMultiImpactDataAssetProxy, ESPMode::ThreadSafe>(InInitData->GetAs<FMultiImpactDataAssetProxy>());
				}
			}
		}
	
		bool IsValid() const
		{
			return Proxy.IsValid();
		}
	
		const FMultiImpactDataAssetProxyPtr& GetProxy() const
		{
			return Proxy;
		}
	
		const FMultiImpactDataAssetProxy* operator->() const
		{
			return Proxy.Get();
		}
	
		FMultiImpactDataAssetProxy* operator->()
		{
			return Proxy.Get();
		}
	};
	
	DECLARE_METASOUND_DATA_REFERENCE_TYPES(FMultiImpactData, IMPACTSFXSYNTH_API, FMultiImpactDataTypeInfo, FMultiImpactDataReadRef, FMultiImpactDataWriteRef)
}
