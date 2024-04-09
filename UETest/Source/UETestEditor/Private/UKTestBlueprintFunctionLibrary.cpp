// Fill out your copyright notice in the Description page of Project Settings.


#include "UKTestBlueprintFunctionLibrary.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Texture2D.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "PackageTools.h"
#include "AssetRegistry/AssetRegistryModule.h"

UUKTestBlueprintFunctionLibrary::UUKTestBlueprintFunctionLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

// UTexture2D* UUKTestBlueprintFunctionLibrary::UKRenderTargetCreateStaticTexture2DEditorOnly(UTextureRenderTarget2D* RenderTarget, FString InName, enum TextureCompressionSettings CompressionSettings, enum TextureMipGenSettings MipSettings)
// {
// #if WITH_EDITOR
// 	if (!RenderTarget)
// 	{
// 		return nullptr;
// 	}
// 	else if (!RenderTarget->GetResource())
// 	{
// 		return nullptr;
// 	}
// 	else
// 	{
// 		FString Name;
// 		FString PackageName;
// 		IAssetTools& AssetTools = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
//
// 		//Use asset name only if directories are specified, otherwise full path
// 		if (!InName.Contains(TEXT("/")))
// 		{
// 			FString AssetName = RenderTarget->GetOutermost()->GetName();
// 			const FString SanitizedBasePackageName = UPackageTools::SanitizePackageName(AssetName);
// 			const FString PackagePath = FPackageName::GetLongPackagePath(SanitizedBasePackageName) + TEXT("/");
// 			AssetTools.CreateUniqueAssetName(PackagePath, InName, PackageName, Name);
// 		}
// 		else
// 		{
// 			InName.RemoveFromStart(TEXT("/"));
// 			InName.RemoveFromStart(TEXT("Content/"));
// 			InName.StartsWith(TEXT("Game/")) == true ? InName.InsertAt(0, TEXT("/")) : InName.InsertAt(0, TEXT("/Game/"));
// 			AssetTools.CreateUniqueAssetName(InName, TEXT(""), PackageName, Name);
// 		}
//
// 		UObject* NewObj = nullptr;
//
// 		// create a static 2d texture
// 		NewObj = RenderTarget->ConstructTexture2D(CreatePackage( *PackageName), Name, RenderTarget->GetMaskedFlags() | RF_Public | RF_Standalone, CTF_Default | CTF_AllowMips | CTF_SkipPostEdit, nullptr);
// 		UTexture2D* NewTex = Cast<UTexture2D>(NewObj);
//
// 		if (NewTex != nullptr)
// 		{
// 			// package needs saving
// 			NewObj->MarkPackageDirty();
//
// 			// Update Compression and Mip settings
// 			NewTex->CompressionSettings = CompressionSettings;
// 			NewTex->MipGenSettings = MipSettings;
// 			NewTex->CompressionNoAlpha = true;
// 			NewTex->PostEditChange();
//
// 			// Notify the asset registry
// 			FAssetRegistryModule::AssetCreated(NewObj);
//
// 			return NewTex;
// 		}
// 	}
// #else
// 	FMessageLog("Blueprint").Error(LOCTEXT("Texture2D's cannot be created at runtime.", "RenderTargetCreateStaticTexture2DEditorOnly: Can't create Texture2D at run time. "));
// #endif
// 	return nullptr;
// }