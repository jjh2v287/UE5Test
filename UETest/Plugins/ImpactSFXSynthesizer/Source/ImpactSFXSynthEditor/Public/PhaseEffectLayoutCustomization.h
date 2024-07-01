// Copyright 2023-2024, Le Binh Son, All rights reserved.

#pragma once

#include "IPropertyTypeCustomization.h"

enum class EResidualPhaseGenerator : uint8;
struct FResidualPhaseEffect;

namespace LBSImpactSFXSynth
{
	namespace Editor
	{
		class IMPACTSFXSYNTHEDITOR_API FPhaseEffectLayoutCustomizationBase : public IPropertyTypeCustomization
		{
		public:
			//~ Begin IPropertyTypeCustomization
			virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
			virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
			//~ End IPropertyTypeCustomization

			protected:
			EResidualPhaseGenerator GetPhaseEffectType() const;
			virtual bool IsScalableEffect(const EResidualPhaseGenerator EffectType);
			
			TSharedPtr<IPropertyHandle> EffectTypeHandle;
		};

		class IMPACTSFXSYNTHEDITOR_API FPhaseEffectLayoutCustomization : public FPhaseEffectLayoutCustomizationBase
		{
		public:
			static TSharedRef<IPropertyTypeCustomization> MakeInstance()
			{
				return MakeShared<FPhaseEffectLayoutCustomization>();
			}
		};
	} 
} 
