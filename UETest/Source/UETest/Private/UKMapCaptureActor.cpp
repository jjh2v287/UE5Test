// Fill out your copyright notice in the Description page of Project Settings.


#include "UETest/Public/UKMapCaptureActor.h"

#include "DrawDebugHelpers.h"
#include "MathUtil.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "UETest/Public/UKMapCaptureBox.h"
#include "AssetRegistry/AssetData.h"
#include "UObject/SavePackage.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "UObject/Package.h"

// Sets default values
AUKMapCaptureActor::AUKMapCaptureActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	ArrowComponent = ObjectInitializer.CreateDefaultSubobject<UArrowComponent>(this, TEXT("Arrow"));
	ArrowComponent->SetArrowSize(3.0f);
	RootComponent = ArrowComponent;

	SceneCaptureComponent = ObjectInitializer.CreateDefaultSubobject<USceneCaptureComponent2D>(this, TEXT("SceneCapture"));
	SceneCaptureComponent->ProjectionType = ECameraProjectionMode::Orthographic;
	SceneCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	SceneCaptureComponent->SetupAttachment(RootComponent);
}

void AUKMapCaptureActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	const UWorld* EditorWorld = GetWorld();
	if (!EditorWorld)
	{
		return;
	}

	if (EditorWorld->IsGameWorld())
	{
		return;
	}
	
#if WITH_EDITOR
	UpdateSceneCaptureBox();
#endif
}

// Called when the game starts or when spawned
void AUKMapCaptureActor::BeginPlay()
{
	Super::BeginPlay();
}

void AUKMapCaptureActor::Destroyed()
{
	Super::Destroyed();
#if WITH_EDITOR
	for (auto It = CaptureBoxs.CreateIterator(); It; ++It)
	{
		It.Value()->Destroy();
		It.RemoveCurrent();
	}
#endif
}

#if WITH_EDITOR
void AUKMapCaptureActor::UpdateSceneCaptureBox()
{
	UWorld* EditorWorld = GetWorld();

	// Delete all capture boxes that do not belong to the capture actor
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(EditorWorld, AUKMapCaptureBox::StaticClass(), FoundActors);
	for (AActor* Actor : FoundActors)
	{
		AUKMapCaptureBox* MyActor = Cast<AUKMapCaptureBox>(Actor);
		if (MyActor == nullptr)
		{
			continue;
		}

		bool bNotChild = this != MyActor->GetAttachParentActor();
		if(bNotChild)
		{
			MyActor->Destroy();
		}
	}

	// Delete if it belongs to a capture actor but not a management container
	TArray<AActor*> OutActors;
	GetAttachedActors(OutActors);
	for (AActor* AttachedActor : OutActors)
	{
		if (AttachedActor == nullptr)
		{
			continue;
		}

		TWeakObjectPtr<AUKMapCaptureBox> addBox = Cast<AUKMapCaptureBox>(AttachedActor);
		if(!addBox.IsValid())
		{
			continue;
		}
		
		bool bIsAddNeed = true;
		for (auto TempActor : CaptureBoxs)
		{
			if(TempActor.Value.Get() == AttachedActor)
			{
				bIsAddNeed = false;
				break;
			}
		}

		const bool bIsContains = CaptureBoxs.Contains(addBox->GetUniqueID());
		if(bIsAddNeed && !bIsContains)
		{
			CaptureBoxs.Emplace(addBox->GetUniqueID(), addBox);
		}
	}

	// When a capture box is deleted externally, it is removed from the container.
	for (auto It = CaptureBoxs.CreateIterator(); It; ++It)
	{
		if(!It.Value().IsValid())
		{
			It.RemoveCurrent();
		}
	}
}

void AUKMapCaptureActor::CreateCaptureBox()
{
	UWorld* EditorWorld = GetWorld();
	int32 MaxCount = 0;
	static FActorSpawnParameters SpawnParams;
	SpawnParams.Name = MakeUniqueObjectName(EditorWorld, AUKMapCaptureBox::StaticClass(), "CaptureBox");
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
	
	if(CaptureBoxs.Num() < (CaptureGridCountX * CaptureGridCountY))
	{
		MaxCount = (CaptureGridCountX * CaptureGridCountY) - CaptureBoxs.Num();
	}

	for(int32 i=0; i<MaxCount; ++i)
	{
		AUKMapCaptureBox* CaptureBox = EditorWorld->SpawnActor<AUKMapCaptureBox>(AUKMapCaptureBox::StaticClass(), FVector::Zero(), FRotator::ZeroRotator, SpawnParams);
		if(CaptureBox)
		{
			CaptureBox->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
			CaptureBox->SetActorLocation(FVector::Zero());
			CaptureBox->SetActorRelativeLocation(FVector::Zero());
			CaptureBoxs.Emplace(CaptureBox->GetUniqueID(), CaptureBox);
		}
	}
}

void AUKMapCaptureActor::GenerateEnclosingBox(FVector& NewBoxCenter, FVector& NewBoxExtent)
{
	TArray<FVector> BoxCenters;
	TArray<FVector> BoxExtents;
	for (auto dsf : CaptureBoxs)
	{
		BoxCenters.Emplace(dsf.Value.Get()->GetActorLocation());
		BoxExtents.Emplace(dsf.Value.Get()->GetBoxExtent());
	}

	FindEnclosingBox(BoxCenters, BoxExtents, NewBoxCenter, NewBoxExtent);
}

void AUKMapCaptureActor::FindEnclosingBox(const TArray<FVector>& BoxCenters, const TArray<FVector>& BoxExtents, FVector& NewEnclosingBoxCenter, FVector& NewEnclosingBoxExtent)
{
	if(BoxCenters.IsEmpty() || BoxExtents.IsEmpty())
	{
		return;
	}
	
	FVector MinPoint = BoxCenters[0] - BoxExtents[0];
	FVector MaxPoint = BoxCenters[0] + BoxExtents[0];

	for (int i = 1; i < BoxCenters.Num(); ++i) {
		FVector CurrentMin = BoxCenters[i] - BoxExtents[i];
		FVector CurrentMax = BoxCenters[i] + BoxExtents[i];

		MinPoint.X = FMath::Min(MinPoint.X, CurrentMin.X);
		MinPoint.Y = FMath::Min(MinPoint.Y, CurrentMin.Y);
		MinPoint.Z = FMath::Min(MinPoint.Z, CurrentMin.Z);

		MaxPoint.X = FMath::Max(MaxPoint.X, CurrentMax.X);
		MaxPoint.Y = FMath::Max(MaxPoint.Y, CurrentMax.Y);
		MaxPoint.Z = FMath::Max(MaxPoint.Z, CurrentMax.Z);
	}

	NewEnclosingBoxCenter = (MinPoint + MaxPoint) / 2;
	NewEnclosingBoxExtent = (MaxPoint - MinPoint) / 2;
}
#endif

// Called every frame
void AUKMapCaptureActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const UWorld* EditorWorld = GetWorld();
	if (!EditorWorld)
	{
		return;
	}

	if (EditorWorld->IsGameWorld())
	{
		return;
	}
	
#if WITH_EDITOR
	UpdateSceneCaptureBox();
	FVector NewBoxCenter;
	FVector NewBoxExtent;
	GenerateEnclosingBox(NewBoxCenter, NewBoxExtent);
	DrawDebugBox(GetWorld(), NewBoxCenter, NewBoxExtent, FColor::Black, false, 0.0f, 0 ,10.0f);
#endif

}

void AUKMapCaptureActor::CreateDataAssetPackage() const
{
	const FString LoadAssetName = CaptureDataPath + CaptureDataName + "." + CaptureDataName;
	const FString PackageName = CaptureDataPath + CaptureDataName;
	const FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	if (FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(LoadAssetName, false, true); !AssetData.IsValid())
	{
		UPackage* Package = nullptr;
		Package = CreatePackage(*PackageName);

		if(Package)
		{
			UUKCaptureMapInfoDataAsset* DataAsset = nullptr;
			Package->FullyLoad();

			DataAsset = NewObject<UUKCaptureMapInfoDataAsset>(Package, *CaptureDataName, RF_Public | RF_Standalone | RF_MarkAsRootSet);

			Package->MarkPackageDirty();
			FAssetRegistryModule::AssetCreated(DataAsset);
			FSavePackageArgs SaveArgs;
			SaveArgs.TopLevelFlags = EObjectFlags::RF_Public | EObjectFlags::RF_Standalone;
			SaveArgs.SaveFlags = SAVE_NoError;
			UPackage::SavePackage(Package, DataAsset, *PackageFileName, SaveArgs);
		}
	}
}

void AUKMapCaptureActor::CapturePlay(class UTextureRenderTarget2D* TextureTarget)
{
	if (GetWorld()->IsGameWorld())
	{
		return;
	}
	
#if WITH_EDITOR
	// DataAsset Create
	CreateDataAssetPackage();

	// DataAsset Load
	const FString LoadAssetName = CaptureDataPath + CaptureDataName + "." + CaptureDataName;
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(LoadAssetName, false, true);
	UUKCaptureMapInfoDataAsset* DataAsset = nullptr;
	UPackage* Package = nullptr;
	
	if (AssetData.IsValid())
	{
		// Load
		DataAsset = Cast<UUKCaptureMapInfoDataAsset> (AssetData.GetAsset());
		if(DataAsset)
		{
			Package = FindPackage(nullptr, *AssetData.PackageName.ToString());

			if(Package)
			{
				Package->FullyLoad();
			}
		}
	}
	
	// Start Set Data
	const FString MapName = GetWorld()->GetMapName();
	FVector NewBoxCenter;
	FVector NewBoxExtent;
	GenerateEnclosingBox(NewBoxCenter, NewBoxExtent);

	auto DataAssetMapInfos = DataAsset->CaptureMapInfos.Find(MapName);
	if(DataAssetMapInfos == nullptr)
	{
		DataAsset->CaptureMapInfos.Emplace(MapName, FUKCaptureMapInfos());
		DataAssetMapInfos = DataAsset->CaptureMapInfos.Find(MapName);
	}

	// Set EnclosingBox
	auto EnclosingBoxDatas = DataAssetMapInfos->EnclosingBoxDatas.Find(EnclosingBoxName);
	if(EnclosingBoxDatas == nullptr)
	{
		FUKCaptureMapEnclosingBox EnclosingBox;
		EnclosingBox.BoxLocation = NewBoxCenter;
		EnclosingBox.BoxSize = NewBoxExtent;
		DataAssetMapInfos->EnclosingBoxDatas.Emplace(EnclosingBoxName, EnclosingBox);
		EnclosingBoxDatas = DataAssetMapInfos->EnclosingBoxDatas.Find(EnclosingBoxName);
	}
	else
	{
		EnclosingBoxDatas->BoxLocation = NewBoxCenter;
		EnclosingBoxDatas->BoxSize = NewBoxExtent;
	}
	
	TArray<FUKCaptureMapData> Info = GenerateCaptureMapInfo();
	uint32 idx = EnclosingBoxDatas->CaptureMapDatas.Num();
	for (auto Element : Info)
	{
		// Capture Camera OrthoWidth Setting
		SceneCaptureComponent->OrthoWidth = Element.OrthoWidth;
		
		// Capture Camera Settint RelativeLocation
		SceneCaptureComponent->SetRelativeLocation(Element.RelativeLocation + FVector(0.0f, 0.0f, Element.BoxSize.Z));

		// Capture
		SceneCaptureComponent->TextureTarget = TextureTarget;
		UKismetRenderingLibrary::ClearRenderTarget2D(this, SceneCaptureComponent->TextureTarget, FLinearColor::Black);
		SceneCaptureComponent->CaptureScene();

		// Texture Save Asset
		auto NewTexture2D = UKismetRenderingLibrary::RenderTargetCreateStaticTexture2DEditorOnly(SceneCaptureComponent->TextureTarget, CaptureTexturePath + Element.Name, TextureCompressionSettings::TC_Default, TextureMipGenSettings::TMGS_FromTextureGroup);
		Element.Name = NewTexture2D->GetName();

		FString DataName = EnclosingBoxName + "-" + FString::FromInt(idx++);
		EnclosingBoxDatas->CaptureMapDatas.Emplace(*DataName, Element);
	}
	// Capture Camera Reset Location
	const FVector FinalLocation = FVector(0.0f, 0.0f, CaptureGridHalfHeight);
	SceneCaptureComponent->SetRelativeLocation(FinalLocation);
	
	//
	// Package->MarkPackageDirty();
	TArray<FAssetData> SavedAssets = {AssetData};
	FAssetRegistryModule::AssetsSaved(MoveTemp(SavedAssets));

	// DataAsset Save
	FString packageFileName = FPackageName::LongPackageNameToFilename(*AssetData.PackageName.ToString(), FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = EObjectFlags::RF_Public | EObjectFlags::RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;
	UPackage::SavePackage(Package, DataAsset, *packageFileName, SaveArgs);
#endif
}

TArray<FUKCaptureMapData> AUKMapCaptureActor::GenerateCaptureMapInfo()
{
	TArray<FUKCaptureMapData> NewVectors;
	
#if WITH_EDITOR
	for (const auto Element : CaptureBoxs)
	{
		FUKCaptureMapData info;
		info.BoxSize = Element.Value->GetBoxExtent();
		info.OrthoWidth = FMathf::Max(info.BoxSize.X * 2.0f, info.BoxSize.Y * 2.0f);
		info.RootLocation = GetActorLocation();
		info.RelativeLocation = Element.Value->GetRootComponent()->GetRelativeLocation();
		info.Name = CaptureTextureNamae + Element.Value->GetActorLabel();
		info.Path = CaptureTexturePath;
		NewVectors.Emplace(info);
	}
#endif
	return NewVectors;
}

void AUKMapCaptureActor::SettingCaptureBoxGridLocation()
{
	const UWorld* EditorWorld = GetWorld();
	if (!EditorWorld)
	{
		return;
	}
	
	if (EditorWorld->IsGameWorld())
	{
		return;
	}
	
	
#if WITH_EDITOR
	CreateCaptureBox();
	
	CaptureSizeAllWidth = (CaptureGridHalfWidth * 2) * CaptureGridCountX;
	CaptureSizeAllHeight = (CaptureGridHalfWidth * 2) * CaptureGridCountY;
	const float StartX = ((CaptureSizeAllWidth * 0.5f)  - CaptureGridHalfWidth);
	const float StartY = -((CaptureSizeAllHeight * 0.5f)  - CaptureGridHalfWidth);
	int32 CurrentIdx = 0;
	auto It = CaptureBoxs.CreateIterator();
	
	for(int32 x=0; x<CaptureGridCountX; ++x)
	{
		for(int32 y=0; y<CaptureGridCountY; ++y)
		{
			if(CaptureBoxs.Num() <= CurrentIdx)
			{
				return;
			}
			
			const float Xpos = StartX - ((CaptureGridHalfWidth * 2) * x);
			const float Ypos = StartY + ((CaptureGridHalfWidth * 2) * y);
			const FVector CellPos = FVector(Xpos, Ypos, 0.0f);
			const FVector BoxSize = FVector(CaptureGridHalfWidth, CaptureGridHalfWidth, CaptureGridHalfHeight);
			const FString Name = FString(FString::Printf(TEXT("%03d_%03d"), x,y));
			
			It.Value()->SetActorRelativeLocation(CellPos);
			It.Value()->SetBoxExtent(BoxSize);
			It.Value()->SetActorLabel(Name);
			CurrentIdx++;
			++It;
		}
	}
#endif
}