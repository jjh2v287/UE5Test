// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#pragma once

#include "MetasoundDataReference.h"
#include "MetasoundDataTypeRegistrationMacro.h"
#include "ImpactModalObj.h"

namespace Metasound
{
	// Forward declare ReadRef
	class FImpactModalObj;
	typedef TDataReadReference<FImpactModalObj> FImpactModalObjReadRef;
	
	// Metasound data type that holds onto a weak ptr. Mostly used as a placeholder until we have a proper proxy type.
	class IMPACTSFXSYNTH_API FImpactModalObj
	{
		FImpactModalObjAssetProxyPtr Proxy;
		
	public:
	
		FImpactModalObj()
			: Proxy(nullptr)
		{
		}
	
		FImpactModalObj(const FImpactModalObj& InOther)
			: Proxy(InOther.Proxy)
		{
		}
	
		FImpactModalObj& operator=(const FImpactModalObj& InOther)
		{
			Proxy = InOther.Proxy;
			return *this;
		}
	
		FImpactModalObj& operator=(FImpactModalObj&& InOther)
		{
			Proxy = MoveTemp(InOther.Proxy);
			return *this;
		}
	
		FImpactModalObj(const TSharedPtr<Audio::IProxyData>& InInitData)
		{
			if (InInitData.IsValid())
			{
				if (InInitData->CheckTypeCast<FImpactModalObjAssetProxy>())
				{
					Proxy = MakeShared<FImpactModalObjAssetProxy, ESPMode::ThreadSafe>(InInitData->GetAs<FImpactModalObjAssetProxy>());
				}
			}
		}
	
		bool IsValid() const
		{
			return Proxy.IsValid();
		}
	
		const FImpactModalObjAssetProxyPtr& GetProxy() const
		{
			return Proxy;
		}
	
		const FImpactModalObjAssetProxy* operator->() const
		{
			return Proxy.Get();
		}
	
		FImpactModalObjAssetProxy* operator->()
		{
			return Proxy.Get();
		}
	};
	
	DECLARE_METASOUND_DATA_REFERENCE_TYPES(FImpactModalObj, IMPACTSFXSYNTH_API, FImpactModalObjTypeInfo, FImpactModalObjReadRef, FImpactModalObjWriteRef)
}
