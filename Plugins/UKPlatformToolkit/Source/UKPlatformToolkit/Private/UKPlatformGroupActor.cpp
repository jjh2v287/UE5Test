// Fill out your copyright notice in the Description page of Project Settings.


#include "UKPlatformGroupActor.h"
#include "UKPlatformActor.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"

AUKPlatformGroupActor::AUKPlatformGroupActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	UArrowComponent* ArrowComponent = ObjectInitializer.CreateDefaultSubobject<UArrowComponent>(this, TEXT("Arrow"));
	ArrowComponent->SetArrowSize(3.0f);
	RootComponent = ArrowComponent;
}

void AUKPlatformGroupActor::BeginPlay()
{
	Super::BeginPlay();

	PlatformActorsTickEnabled(false);

	// Remove Duplicates
	TSet<TObjectPtr<class AUKPlatformActor>> TempSet(PlatformActors);
	PlatformActors.Empty();
	PlatformActors.Append(TempSet.Array());
	
	if(BoxComponent)
	{
		BoxComponent->OnComponentBeginOverlap.RemoveDynamic(this, &AUKPlatformGroupActor::BoxBeginOverlap);
		BoxComponent->OnComponentEndOverlap.RemoveDynamic(this, &AUKPlatformGroupActor::BoxEndOverlap);
		BoxComponent->OnComponentBeginOverlap.AddDynamic(this, &AUKPlatformGroupActor::BoxBeginOverlap);
		BoxComponent->OnComponentEndOverlap.AddDynamic(this, &AUKPlatformGroupActor::BoxEndOverlap);
	}

	if(SphereComponent)
	{
		SphereComponent->OnComponentBeginOverlap.RemoveDynamic(this, &AUKPlatformGroupActor::BoxBeginOverlap);
		SphereComponent->OnComponentEndOverlap.RemoveDynamic(this, &AUKPlatformGroupActor::BoxEndOverlap);
		SphereComponent->OnComponentBeginOverlap.AddDynamic(this, &AUKPlatformGroupActor::BoxBeginOverlap);
		SphereComponent->OnComponentEndOverlap.AddDynamic(this, &AUKPlatformGroupActor::BoxEndOverlap);
	}
}

void AUKPlatformGroupActor::Destroyed()
{
	Super::Destroyed();

	for(int32 ActorIndex = 0; ActorIndex < PlatformActors.Num(); ++ActorIndex)
	{
		AUKPlatformActor* InActorPtr = Cast<AUKPlatformActor>(PlatformActors[ActorIndex]);
		if(InActorPtr != nullptr)
		{
#if WITH_EDITOR
			InActorPtr->SetDebugText(TEXT(""));
#endif
			InActorPtr->PlatformGroupActor = nullptr;
		}
	}
	PlatformActors.Empty();
}

void AUKPlatformGroupActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
#if WITH_EDITOR
	// if (!GetWorld()->IsGameWorld())
	// {
	// 	for(int32 ActorIndex = 0; ActorIndex < PlatformActors.Num(); ++ActorIndex)
	// 	{
	// 		if(PlatformActors[ActorIndex])
	// 		{
	// 			FBox ActorBox = PlatformActors[ActorIndex]->GetComponentsBoundingBox( true );
	// 			// DrawDebugSolidBox(GetWorld(), ActorBox);
	// 		}
	// 	}
	// }
#endif
}

void AUKPlatformGroupActor::PlatformActorsTickEnabled(bool Enabled)
{
	for(int32 ActorIndex = 0; ActorIndex < PlatformActors.Num(); ++ActorIndex)
	{
		AUKPlatformActor* InActorPtr = Cast<AUKPlatformActor>(PlatformActors[ActorIndex]);
		if(InActorPtr != nullptr)
		{
			InActorPtr->ActorTickEnabled(Enabled);
		}
	}
}

void AUKPlatformGroupActor::BoxBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& OverlapInfo)
{
	PlatformActorsTickEnabled(true);
}

void AUKPlatformGroupActor::BoxEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	PlatformActorsTickEnabled(false);
}

#if WITH_EDITOR
void AUKPlatformGroupActor::PreEditChange(FProperty* PropertyThatWillChange)
{
	Super::PreEditChange(PropertyThatWillChange);

	if(PropertyThatWillChange == nullptr)
	{
		return;	
	}

	const FName PropertyName = PropertyThatWillChange->GetFName();
	bool IsVariableName = PropertyName == GET_MEMBER_NAME_CHECKED(AUKPlatformGroupActor, PlatformActors);
	
	if (IsVariableName)
	{
		PreEditChangePrimaryPlatformActors.Empty();
		PreEditChangePrimaryPlatformActors.Append(PlatformActors);
	}
}

void AUKPlatformGroupActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if(PropertyChangedEvent.Property == nullptr)
	{
		return;	
	}

	const FName PropertyName = PropertyChangedEvent.Property->GetFName();
	bool IsVariableName = PropertyName == GET_MEMBER_NAME_CHECKED(AUKPlatformGroupActor, PlatformActors);
	
	if (IsVariableName)
	{
		bool IsSame = PreEditChangePrimaryPlatformActors.Num() == PlatformActors.Num();
		bool IsDelete = PreEditChangePrimaryPlatformActors.Num() > PlatformActors.Num();
		if(IsDelete)
		{
			for (auto Actor : PlatformActors)
			{
				PreEditChangePrimaryPlatformActors.Remove(Actor);
			}

			for (auto Actor : PreEditChangePrimaryPlatformActors)
			{
				if(Actor)
				{
					Actor->Modify();
					Actor->SetDebugText(TEXT(""));
					Actor->PlatformGroupActor = nullptr;
				}
			}
		}
		else if(IsSame)
		{
			for (int32 i=0; i<PreEditChangePrimaryPlatformActors.Num(); ++i)
			{
				AUKPlatformActor* PreActor = PreEditChangePrimaryPlatformActors[i];
				AUKPlatformActor* PostActor = PlatformActors[i];

				if(PreActor == nullptr && PostActor->PlatformGroupActor == nullptr)
				{
					// Delete
					PostActor->Modify();
					PostActor->PlatformGroupActor = this;
					PostActor->SetDebugText(this->GetActorLabel());
				}
				else if(PostActor == nullptr && PreActor->PlatformGroupActor == nullptr)
				{
					// Add
					PreActor->Modify();
					PreActor->PlatformGroupActor = nullptr;
					PreActor->SetDebugText(TEXT(""));
				}
			}

			for (int32 i=PlatformActors.Num()-1; i >= 0; --i)
			{
				// 이미 그룹인 플랫폼 액터를 선택 했을때
				AUKPlatformActor* PostActor = PlatformActors[i];
				if(PostActor && PostActor->PlatformGroupActor && PostActor->PlatformGroupActor != this)
				{
					PlatformActors[i] = nullptr;
				}
			}
		}
		PreEditChangePrimaryPlatformActors.Empty();
	}
}
#endif

void AUKPlatformGroupActor::Add(AActor& InActor)
{
#if WITH_EDITOR
	Modify();
	AUKPlatformActor* InActorPtr = Cast<AUKPlatformActor>(&InActor);
	if(InActorPtr)
	{
		PlatformActors.AddUnique(InActorPtr);
		InActorPtr->Modify();
		InActorPtr->PlatformGroupActor = this;
		InActorPtr->SetDebugText(this->GetActorLabel());
	}
#else
	AUKPlatformActor* InActorPtr = Cast<AUKPlatformActor>(&InActor);
	if(InActorPtr)
	{
		PlatformActors.AddUnique(InActorPtr);
		InActorPtr->PlatformGroupActor = this;
	}
#endif
}

void AUKPlatformGroupActor::Remove(AActor& InActor)
{
#if WITH_EDITOR
	AUKPlatformActor* InActorPtr = Cast<AUKPlatformActor>(&InActor);
	if(PlatformActors.Contains(InActorPtr))
	{
		Modify();
		PlatformActors.Remove(InActorPtr);
		InActorPtr->Modify();
		InActorPtr->SetDebugText(TEXT(""));
		InActorPtr->PlatformGroupActor = nullptr;
	}

	if(PlatformActors.IsEmpty())
	{
		UWorld* MyWorld = GetWorld();
		if( MyWorld )
		{
			MyWorld->ModifyLevel(GetLevel());
			MyWorld->EditorDestroyActor( this, false );
			
		}
	}
#else
	AUKPlatformActor* InActorPtr = Cast<AUKPlatformActor>(&InActor);
	if(PlatformActors.Contains(InActorPtr))
	{
		PlatformActors.Remove(InActorPtr);
		InActorPtr->PlatformGroupActor = nullptr;
	}

	if(PlatformActors.IsEmpty())
	{
		UWorld* MyWorld = GetWorld();
		if( MyWorld )
		{
			MyWorld->DestroyActor( this );
		}
	}	
#endif
}

void AUKPlatformGroupActor::CenterGroupLocation(bool IsBox /*= true*/)
{
#if WITH_EDITOR
	Modify();
#endif
	
	FVector MinVector;
	FVector MaxVector;
	
	GetBoundingVectorsForGroup(MinVector, MaxVector );

	FVector BoxCenter = (MinVector + MaxVector) * 0.5f;
	FVector BoxExtent = (MaxVector - MinVector) * 0.5f;

	SetActorLocation(BoxCenter, false);
	GenerateCollision(IsBox, BoxExtent);
}

void AUKPlatformGroupActor::GetBoundingVectorsForGroup(FVector& OutVectorMin, FVector& OutVectorMax)
{
	OutVectorMin = FVector(BIG_NUMBER);
	OutVectorMax = FVector(-BIG_NUMBER);
	
	for(int32 ActorIndex = 0; ActorIndex < PlatformActors.Num(); ++ActorIndex)
	{
		AActor* Actor = PlatformActors[ActorIndex];
		
		FBox ActorBox = Actor->GetComponentsBoundingBox( true );

		// MinVector
		OutVectorMin.X = FMath::Min<FVector::FReal>( ActorBox.Min.X, OutVectorMin.X );
		OutVectorMin.Y = FMath::Min<FVector::FReal>( ActorBox.Min.Y, OutVectorMin.Y );
		OutVectorMin.Z = FMath::Min<FVector::FReal>( ActorBox.Min.Z, OutVectorMin.Z );
		// MaxVector
		OutVectorMax.X = FMath::Max<FVector::FReal>( ActorBox.Max.X, OutVectorMax.X );
		OutVectorMax.Y = FMath::Max<FVector::FReal>( ActorBox.Max.Y, OutVectorMax.Y );
		OutVectorMax.Z = FMath::Max<FVector::FReal>( ActorBox.Max.Z, OutVectorMax.Z );
	}
}

void AUKPlatformGroupActor::GenerateCollision(bool IsBox, FVector Extent)
{
	if(IsBox)
	{
		if(SphereComponent)
		{
			SphereComponent->UnregisterComponent();
			SphereComponent->DestroyComponent();
			SphereComponent = nullptr;
		}
		
		if(BoxComponent == nullptr)
		{
			BoxComponent = NewObject<UBoxComponent>(this);
			BoxComponent->SetBoxExtent(Extent);
			BoxComponent->SetCanEverAffectNavigation(false);
			BoxComponent->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
			BoxComponent->SetGenerateOverlapEvents(true);
			BoxComponent->SetLineThickness(20.0f);
			BoxComponent->ShapeColor = FColor::Black;
			BoxComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			BoxComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
			BoxComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			BoxComponent->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
			BoxComponent->RegisterComponent();
		}
	}
	else
	{
		if(BoxComponent)
		{
			BoxComponent->UnregisterComponent();
			BoxComponent->DestroyComponent();
			BoxComponent = nullptr;
		}
		
		if(SphereComponent == nullptr)
		{
			SphereComponent = NewObject<USphereComponent>(this);
			SphereComponent->SetSphereRadius(FMathf::Max3(Extent.X, Extent.Y, Extent.Z));
			SphereComponent->SetCanEverAffectNavigation(false);
			SphereComponent->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
			SphereComponent->SetGenerateOverlapEvents(true);
			SphereComponent->ShapeColor = FColor::Black;
			SphereComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			SphereComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
			SphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			SphereComponent->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
			SphereComponent->RegisterComponent();
		}
	}
}
