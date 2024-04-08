// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UKMapCaptureBox.h"
#include "Components/ArrowComponent.h"
#include "GameFramework/Actor.h"
#include "Components/SceneCaptureComponent2D.h"
// #include "UKMapHelper.h"
#include "UKMapCaptureActor.generated.h"

UENUM(BlueprintType)
enum class EUKRenderTargetResolution : uint8
{
	Res_512			UMETA(DisplayName = "512x"),
	Res_1024		UMETA(DisplayName = "1024x"),
	Res_2048		UMETA(DisplayName = "2048x"),
	Res_4096		UMETA(DisplayName = "4096x"),
	Res_8192		UMETA(DisplayName = "8192x"),
};

USTRUCT(BlueprintType)
struct FUKCaptureMapData
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture")
	float OrthoWidth;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture")
	FVector RootLocation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture")
	FVector RelativeLocation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture")
	FVector BoxSize;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture")
	FString Path;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture")
	FString Name;
};

USTRUCT(BlueprintType)
struct FUKCaptureMapEnclosingBox
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture")
	FVector BoxLocation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture")
	FVector BoxSize;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture")
	TMap<FName, FUKCaptureMapData> CaptureMapDatas;
};

USTRUCT(BlueprintType)
struct FUKCaptureMapInfos
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture")
	TMap<FString, FUKCaptureMapEnclosingBox> EnclosingBoxDatas;
};

UCLASS(BlueprintType)
class UE5TEST_API UUKCaptureMapInfoDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture")
	float MaxOrthoWidth = 1000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture")
	TMap<FString, FUKCaptureMapInfos> CaptureMapInfos;
};

UCLASS()
class UE5TEST_API AUKMapCaptureActor : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UArrowComponent> ArrowComponent = nullptr;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<USceneCaptureComponent2D> SceneCaptureComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Capture")
	FString CaptureTexturePath = TEXT("/Game/MapCapture/CaptureTexture/");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Capture")
	FString CaptureTextureNamae = TEXT("Map-");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Capture")
	FString CaptureDataPath = TEXT("/Game/MapCapture/");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Capture")
	FString CaptureDataName = TEXT("MapCaptureDataAsset");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Capture")
	FString EnclosingBoxName = TEXT("World");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture")
	EUKRenderTargetResolution SegmentResolution = EUKRenderTargetResolution::Res_1024;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Capture Grid")
	int CaptureGridHalfWidth = 100;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Capture Grid")
	int CaptureGridHalfHeight = 100;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Capture Grid")
	int CaptureGridCountX = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Capture Grid")
	int CaptureGridCountY = 1;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Capture")
	int CaptureSizeAllWidth = 1;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Capture")
	int CaptureSizeAllHeight = 1;
#if WITH_EDITOR
	TMap<uint32, TWeakObjectPtr<AUKMapCaptureBox>> CaptureBoxs;
#endif
	
public:	
	// Sets default values for this actor's properties
	explicit AUKMapCaptureActor(const FObjectInitializer& ObjectInitializer);
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }
	virtual void OnConstruction(const FTransform& Transform) override;
	
#if WITH_EDITOR
	void UpdateSceneCaptureBox();
	void CreateCaptureBox();
	void GenerateEnclosingBox(FVector& NewBoxCenter, FVector& NewBoxExtent);
	void FindEnclosingBox(const TArray<FVector>& BoxCenters, const TArray<FVector>& BoxExtents, FVector& NewEnclosingBoxCenter, FVector& NewEnclosingBoxExtent);
#endif

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void Destroyed() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Capture")
	void CapturePlay(class UTextureRenderTarget2D* TextureTarget);

	UFUNCTION(BlueprintCallable, Category = "Capture")
	TArray<FUKCaptureMapData> GenerateCaptureMapInfo();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Capture Grid")
	void SettingCaptureBoxGridLocation();

	void CreateDataAssetPackage() const;
};
