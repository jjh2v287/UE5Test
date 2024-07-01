// Copyright 2023-2024, Le Binh Son, All rights reserved.

#pragma once

#include "IPropertyTypeCustomization.h"

enum class EImpactSynthCurveSource : uint8;
struct FImpactSynthCurve;

namespace LBSImpactSFXSynth
{
	namespace Editor
	{
		class IMPACTSFXSYNTHEDITOR_API FImpactCurveCustomizationBase : public IPropertyTypeCustomization
		{
		public:
			//~ Begin IPropertyTypeCustomization
			virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
			virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
			//~ End IPropertyTypeCustomization

			protected:
			virtual TSet<EImpactSynthCurveSource> GetSupportedCurves() const;
			
			EImpactSynthCurveSource GetCurveSource() const;

			TSharedPtr<IPropertyHandle> CurveHandle;

		private:
			void CustomizeCurveSelector(IDetailChildrenBuilder& ChildBuilder);

			TMap<FString, FName> CurveDisplayStringToNameMap;
		};

		class IMPACTSFXSYNTHEDITOR_API FImpactCurveCustomization : public FImpactCurveCustomizationBase
		{
		public:
			static TSharedRef<IPropertyTypeCustomization> MakeInstance()
			{
				return MakeShared<FImpactCurveCustomization>();
			}
		};
	} 
} 
