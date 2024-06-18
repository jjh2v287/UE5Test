// Copyright SweejTech Ltd. All Rights Reserved.

#include "AudioInspectorStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FAudioInspectorStyle::StyleInstance = nullptr;

void FAudioInspectorStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FAudioInspectorStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FAudioInspectorStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("AudioInspectorStyle"));
	return StyleSetName;
}

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);

TSharedRef< FSlateStyleSet > FAudioInspectorStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("AudioInspectorStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("AudioInspector")->GetBaseDir() / TEXT("Resources"));

	Style->Set("AudioInspector.OpenPluginWindow", new IMAGE_BRUSH_SVG(TEXT("st-audio-inspector-icon-white-red"), Icon20x20));
	Style->Set("AudioInspector.WindowIcon", new IMAGE_BRUSH_SVG(TEXT("st-audio-inspector-icon-white-red"), Icon20x20));

	return Style;
}

void FAudioInspectorStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FAudioInspectorStyle::Get()
{
	return *StyleInstance;
}
