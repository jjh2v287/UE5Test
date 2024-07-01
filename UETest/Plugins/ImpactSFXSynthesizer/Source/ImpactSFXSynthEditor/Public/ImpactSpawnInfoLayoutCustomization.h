// Copyright 2023-2024, Le Binh Son, All rights reserved.

#pragma once

#include "IPropertyTypeCustomization.h"

enum class EResidualPhaseGenerator : uint8;
struct FResidualPhaseEffect;

namespace LBSImpactSFXSynth
{
	namespace Editor
	{
		class IMPACTSFXSYNTHEDITOR_API FImpactSpawnInfoLayoutCustomizationBase : public IPropertyTypeCustomization
		{
		public:
			//~ Begin IPropertyTypeCustomization
			virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
			virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
			//~ End IPropertyTypeCustomization

			protected:
			TSharedPtr<IPropertyHandle> bUseModalSynthHandle;
			TSharedPtr<IPropertyHandle> bUseResidualSynthHandle;

			EVisibility GetModalSynthParamsVisibility() const;
			EVisibility GetResidualSynthParamsVisibility() const;
		};

		class IMPACTSFXSYNTHEDITOR_API FImpactSpawnInfoLayoutCustomization : public FImpactSpawnInfoLayoutCustomizationBase
		{
		public:
			static TSharedRef<IPropertyTypeCustomization> MakeInstance()
			{
				return MakeShared<FImpactSpawnInfoLayoutCustomization>();
			}
		};
	} 
} 
