// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKAudioVolume.h"
#include "Components/BoxComponent.h"
#include "Components/BrushComponent.h"
#include "Model.h"
#include "Engine/Polys.h"
#include "Engine/BrushBuilder.h"
#include "PhysicsEngine/BodySetup.h"
#include "Subsystems/SoundMananger/UKAudioEngineSubsystem.h"

AUKAudioVolume::AUKAudioVolume()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	BoxComp = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComp"));
	BoxComp->SetupAttachment(RootComponent);
	BoxComp->CanCharacterStepUpOn = ECB_No;
	BoxComp->SetCollisionProfileName(TEXT("OverlapOnlyPlayer"));
	BoxComp->SetGenerateOverlapEvents(true);
}

void AUKAudioVolume::BeginPlay()
{
	Super::BeginPlay();
	
	InitializeWallPlanes();

	BoxComp->OnComponentBeginOverlap.AddDynamic(this, &AUKAudioVolume::OnBoxBeginOverlap);
	BoxComp->OnComponentEndOverlap.AddDynamic(this, &AUKAudioVolume::OnBoxEndOverlap);
}

void AUKAudioVolume::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	BoxComp->OnComponentBeginOverlap.RemoveAll(this);
	BoxComp->OnComponentEndOverlap.RemoveAll(this);

	OverlapActor.Reset();
	SetActorTickEnabled(false);
}

void AUKAudioVolume::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float DistanceToWalls = GetDistanceToWalls(OverlapActor->GetActorLocation());
	const float NewRainAttenuation = FMath::GetMappedRangeValueClamped(FVector2D{ 0.0f, RainAttenuationDistance }, FVector2D{ 1.0f, 0.0f }, DistanceToWalls);
	UUKAudioEngineSubsystem::Get()->RainAttenuation = NewRainAttenuation;
}

FVector AUKAudioVolume::GetBoxExtent() const
{
	FVector BoxExtent = FVector(100.0f, 100.0f, 100.0f);
	const UBrushComponent* BrushComp = GetBrushComponent();
	if (BrushComp && BrushComp->BrushBodySetup && !BrushComp->BrushBodySetup->AggGeom.ConvexElems.IsEmpty())
	{
		BoxExtent = BrushComp->BrushBodySetup->AggGeom.ConvexElems[0].ElemBox.Max;
	}
	
	return BoxExtent;
}

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
	
	return FMath::Abs(LocalPoint.X) <= BoxExtent.X && FMath::Abs(LocalPoint.Y) <= BoxExtent.Y && FMath::Abs(LocalPoint.Z) <= BoxExtent.Z;
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

void AUKAudioVolume::OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor != this)
	{
		OverlapActor = OtherActor;
		SetActorTickEnabled(true);
	}
}

void AUKAudioVolume::OnBoxEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor && OtherActor != this)
	{
		OverlapActor.Reset();
		SetActorTickEnabled(false);
		
		UUKAudioEngineSubsystem::Get()->RainAttenuation = 1.0f;
	}
}

#if WITH_EDITOR
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

		UBrushComponent* BrushComp = GetBrushComponent();
		const UBrushComponent* OldBrushComp = OldAudioVolume->GetBrushComponent();
		if (BrushComp && BrushComp->BrushBodySetup && OldBrushComp && OldBrushComp->BrushBodySetup)
		{
			BrushComp->Brush->Polys->Element.Reset();
			BrushComp->Brush->Polys->Element.Append(OldBrushComp->Brush->Polys->Element);
			BrushComp->Brush->Nodes.Reset();
			BrushComp->Brush->Nodes.Append(OldBrushComp->Brush->Nodes);
			BrushComp->BrushBodySetup->CopyBodyPropertiesFrom(OldBrushComp->BrushBodySetup);
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