// Copyright 2023-2024, Le Binh Son, All rights reserved.

#include "FCustomEditorUtils.h"

#include "Styling/SlateStyleRegistry.h"

namespace LBSImpactSFXSynth
{
	namespace Editor
	{
		void FCustomEditorUtils::GetNameAffix(const FString& DefaultAffix, const UPackage* Package, FString& PathName)
		{
			FString WaveName;
			FString Extension;
			FPaths::Split(Package->GetName(), PathName, WaveName, Extension);
			PathName = FPaths::Combine(PathName, DefaultAffix + WaveName + Extension);
		}

		const FSlateBrush& FCustomEditorUtils::GetSlateBrushSafe(FName InName)
		{
			const ISlateStyle* MetaSoundStyle = FSlateStyleRegistry::FindSlateStyle("MetaSoundStyle");
			if (ensureMsgf(MetaSoundStyle, TEXT("Missing slate style 'MetaSoundStyle'")))
			{
				const FSlateBrush* Brush = MetaSoundStyle->GetBrush(InName);
				if (ensureMsgf(Brush, TEXT("Missing brush '%s'"), *InName.ToString()))
				{
					return *Brush;
				}
			}

			if (const FSlateBrush* NoBrush = FAppStyle::GetBrush("NoBrush"))
			{
				return *NoBrush;
			}

			static const FSlateBrush NullBrush;
			return NullBrush;
		}
	}
}
