// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "HTNStyle.h"

#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleRegistry.h"

TSharedPtr<FSlateStyleSet> FHTNStyle::StyleInstance = nullptr;

void FHTNStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FHTNStyle::Shutdown()
{
	if (StyleInstance.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
		ensure(StyleInstance.IsUnique());
		StyleInstance.Reset();
	}
}

const ISlateStyle& FHTNStyle::Get()
{
	return *StyleInstance;
}

FName FHTNStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("HTNStyle"));
	return StyleSetName;
}

#define IMAGE_BRUSH(RelativePath, ...) FSlateImageBrush(RootToContentDir( RelativePath, TEXT(".png")), __VA_ARGS__)
#define BOX_BRUSH(RelativePath, ...) FSlateBoxBrush(RootToContentDir( RelativePath, TEXT(".png")), __VA_ARGS__)
#define BORDER_BRUSH(RelativePath, ...) FSlateBorderBrush(RootToContentDir( RelativePath, TEXT(".png")), __VA_ARGS__)
#define TTF_FONT(RelativePath, ...) FSlateFontInfo(RootToContentDir( RelativePath, TEXT(".ttf")), __VA_ARGS__)
#define OTF_FONT(RelativePath, ...) FSlateFontInfo(RootToContentDir( RelativePath, TEXT(".otf")), __VA_ARGS__)

FHTNStyle::FStyle::FStyle() : FSlateStyleSet(FHTNStyle::GetStyleSetName())
{
	const FVector2D Icon16x16(16.0f, 16.0f);
	const FVector2D Icon20x20(20.0f, 20.0f);
	const FVector2D Icon40x40(40.0f, 40.0f);

	SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));

	SetContentRoot(IPluginManager::Get().FindPlugin(TEXT("HTN"))->GetBaseDir() / TEXT("Resources"));
	Set("HTNEditor.Common.OpenPluginWindow", new IMAGE_BRUSH(TEXT("ButtonIcon_40x"), Icon40x40));

	SetContentRoot(FPaths::EngineContentDir() / TEXT("Editor/Slate"));
	Set("HTNEditor.Debugger.PausePlaySession", new IMAGE_BRUSH("Icons/icon_pause_40x", Icon40x40));
	Set("HTNEditor.Debugger.PausePlaySession.Small", new IMAGE_BRUSH("Icons/icon_pause_40x", Icon20x20));
	Set("HTNEditor.Debugger.ResumePlaySession", new IMAGE_BRUSH("Icons/icon_simulate_40x", Icon40x40));
	Set("HTNEditor.Debugger.ResumePlaySession.Small", new IMAGE_BRUSH("Icons/icon_simulate_40x", Icon20x20));
	Set("HTNEditor.Debugger.StopPlaySession", new IMAGE_BRUSH("Icons/icon_stop_40x", Icon40x40));
	Set("HTNEditor.Debugger.StopPlaySession.Small", new IMAGE_BRUSH("Icons/icon_stop_40x", Icon20x20));
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef OTF_FONT

TSharedRef<FSlateStyleSet> FHTNStyle::Create()
{
	TSharedRef<FSlateStyleSet> Style = MakeShared<FStyle>();
	return Style;
}