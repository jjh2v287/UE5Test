// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DrawDebugHelpers.h"
#include "Math/Vector2D.h"
#include "InputCoreTypes.h"
#include "UKTestBlueprintFunctionLibrary.generated.h"

class UTexture2D;
/**
 * 
 */
UCLASS()
// class UETEST_API UUKTestBlueprintFunctionLibrary final : public UBlueprintFunctionLibrary
class UUKTestBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()
	
public:

	// UFUNCTION(BlueprintCallable, meta = (DisplayName = "Render Target Create Static Texture Editor Only", Keywords = "Create Static Texture from Render Target", UnsafeDuringActorConstruction = "true"), Category = Game)
	// UFUNCTION(BlueprintCallable, Category = "UKGame|Actor")
	// static ENGINE_API UTexture2D* UKRenderTargetCreateStaticTexture2DEditorOnly(UTextureRenderTarget2D* RenderTarget, FString Name = "Texture", enum TextureCompressionSettings CompressionSettings = TC_Default, enum TextureMipGenSettings MipSettings = TMGS_FromTextureGroup);
};
