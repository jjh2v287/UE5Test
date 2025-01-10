// Copyright Kong Studios, Inc. All Rights Reserved.


#include "UKAudioVolume.h"
#include "Components/BoxComponent.h"
#include "Components/BrushComponent.h"
#include "BodySetupEnums.h"
#include "Engine/Polys.h"
#include "Engine/BrushBuilder.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicsEngine/BodySetup.h"

AUKAudioVolume::AUKAudioVolume()
{
	PrimaryActorTick.bCanEverTick = true;

	BoxComp = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComp"));
	BoxComp->SetupAttachment(RootComponent);
	BoxComp->CanCharacterStepUpOn = ECB_No;
	BoxComp->SetCollisionProfileName(TEXT("OverlapOnlyPlayer"));
	BoxComp->SetGenerateOverlapEvents(true);
}

void AUKAudioVolume::BeginPlay()
{
	Super::BeginPlay();
	SetActorTickEnabled(false);
	InitializeWallPlanes();
	if (BoxComp)
	{
		if (BoxComp->OnComponentBeginOverlap.IsBound())
		{
			BoxComp->OnComponentBeginOverlap.RemoveAll(this);
			BoxComp->OnComponentEndOverlap.RemoveAll(this);
		}
		BoxComp->OnComponentBeginOverlap.AddDynamic(this, &AUKAudioVolume::OnBoxBeginOverlap);
		BoxComp->OnComponentEndOverlap.AddDynamic(this, &AUKAudioVolume::OnBoxEndOverlap);
	}
}

void AUKAudioVolume::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	if (BoxComp)
	{
		BoxComp->OnComponentBeginOverlap.RemoveAll(this);
		BoxComp->OnComponentEndOverlap.RemoveAll(this);
	}
}

void AUKAudioVolume::OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor != this)
	{
		SetActorTickEnabled(true);
	}
}

void AUKAudioVolume::OnBoxEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor && OtherActor != this)
	{
		SetActorTickEnabled(false);
	}
}

void AUKAudioVolume::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	const float Length = GetDistanceToWalls(Pawn->GetActorLocation());

	UKismetSystemLibrary::PrintString(this, FString::SanitizeFloat(Length), true, true, FLinearColor::Black,0.0f);
}

FVector AUKAudioVolume::GetBoxExtent() const
{
	FVector BoxExtent = FVector(100.0f, 100.0f, 100.0f);
	if (GetBrushComponent() && GetBrushComponent()->BrushBodySetup && !GetBrushComponent()->BrushBodySetup->AggGeom.ConvexElems.IsEmpty())
	{
		BoxExtent = GetBrushComponent()->BrushBodySetup->AggGeom.ConvexElems[0].ElemBox.Max;
	}
	
	return  BoxExtent;
}

void AUKAudioVolume::InitializeWallPlanes()
{
	const FTransform BoxTransform = GetBrushComponent()->GetComponentTransform();
	const FVector Forward = BoxTransform.GetUnitAxis(EAxis::X);
	const FVector Right = BoxTransform.GetUnitAxis(EAxis::Y);
	const FVector Origin = BoxTransform.GetLocation();
	const FVector ActorScale = GetActorScale();
	const FVector BoxExtent = GetBoxExtent() * ActorScale;

	CachedWallPlanes.Empty();
	CachedWallPlanes.Add(FPlane(Origin + Forward * BoxExtent.X, Forward));
	CachedWallPlanes.Add(FPlane(Origin - Forward * BoxExtent.X, -Forward));
	CachedWallPlanes.Add(FPlane(Origin + Right * BoxExtent.Y, Right));
	CachedWallPlanes.Add(FPlane(Origin - Right * BoxExtent.Y, -Right));
}

float AUKAudioVolume::GetDistanceToPlane(const FPlane& Plane, const FVector& Location)
{
	return FMath::Abs(Plane.PlaneDot(Location));
}

#if WITH_EDITOR
void AUKAudioVolume::PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);
}

void AUKAudioVolume::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FVector BoxExtent = GetBoxExtent();
	BoxComp->SetBoxExtent(BoxExtent);
}

void AUKAudioVolume::EditorReplacedActor(AActor* OldActor)
{
	Super::EditorReplacedActor(OldActor);

	if (AAudioVolume* OldAudioVolume = Cast<AAudioVolume>(OldActor))
	{
		FTransform OldTransform = OldAudioVolume->GetActorTransform();
		this->SetActorTransform(OldTransform);
		SetPriority(OldAudioVolume->GetPriority());
		SetEnabled(OldAudioVolume->GetEnabled());
		SetReverbSettings(OldAudioVolume->GetReverbSettings());
		SetInteriorSettings(OldAudioVolume->GetInteriorSettings());
		SetSubmixSendSettings(OldAudioVolume->GetSubmixSendSettings());
		SetSubmixOverrideSettings(OldAudioVolume->GetSubmixOverrideSettings());
		
		if (GetBrushComponent() && GetBrushComponent()->BrushBodySetup && OldAudioVolume->GetBrushComponent() && OldAudioVolume->GetBrushComponent()->BrushBodySetup)
		{
			GetBrushComponent()->Brush->Polys->Element.Reset();
			GetBrushComponent()->Brush->Polys->Element.Append(OldAudioVolume->GetBrushComponent()->Brush->Polys->Element);
			GetBrushComponent()->Brush->Nodes.Reset();
			GetBrushComponent()->Brush->Nodes.Append(OldAudioVolume->GetBrushComponent()->Brush->Nodes);
			GetBrushComponent()->BrushBodySetup->CopyBodyPropertiesFrom(OldAudioVolume->GetBrushComponent()->BrushBodySetup);
			if(OldAudioVolume->BrushBuilder != nullptr)
			{
				BrushBuilder = DuplicateObject<UBrushBuilder>(OldAudioVolume->BrushBuilder, this);
			}
			Brush->Modify(false);
			Brush->EmptyModel(1, 0);
			Brush->BuildBound();
		}

		ReregisterAllComponents();
		const FVector BoxExtent = GetBoxExtent();
		BoxComp->SetBoxExtent(BoxExtent);
	}
}
#endif // WITH_EDITOR

float AUKAudioVolume::GetDistanceToWalls(const FVector& Location) const
{
	if (!IsLocationInside(Location))
	{
		return 0.0f;
	}

	float MinDistance = MAX_FLT;
	for (const FPlane& Plane : CachedWallPlanes)
	{
		float Distance = GetDistanceToPlane(Plane, Location);
		MinDistance = FMath::Min(MinDistance, Distance);
	}
	return MinDistance;
}

bool AUKAudioVolume::IsLocationInside(const FVector& Location) const
{
	const FTransform BoxTransform = GetBrushComponent()->GetComponentTransform();
	const FVector LocalPoint = BoxTransform.InverseTransformPosition(Location);
	const FVector BoxExtent = GetBoxExtent();
	
	return FMath::Abs(LocalPoint.X) <= BoxExtent.X &&
		   FMath::Abs(LocalPoint.Y) <= BoxExtent.Y &&
		   FMath::Abs(LocalPoint.Z) <= BoxExtent.Z;
}