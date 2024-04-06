// Fill out your copyright notice in the Description page of Project Settings.


#include "UKPlatformToolUtils.h"

#include "UKPlatformActor.h"
#include "UKPlatformGroupActor.h"
#include "Engine/Selection.h"
#include "Layers/LayersSubsystem.h"

UUKPlatformToolUtils::UUKPlatformToolUtils()
{
	Instance = this;
}

FUKSelectActorPlatformInfo UUKPlatformToolUtils::GetSelectPlatformGroupInfo()
{
	FUKSelectActorPlatformInfo Info;
	
	TArray<AActor*> Actors;
	GEditor->GetSelectedActors()->GetSelectedObjects<AActor>(Actors);
	for (AActor* Actor : Actors)
	{
		AUKPlatformGroupActor* PlatformGroupActor = Cast<AUKPlatformGroupActor>(Actor);
		AUKPlatformActor* PlatformActor = Cast<AUKPlatformActor>(Actor);
		if (PlatformActor)
		{
			Info.PlatformActorNum++;
			if(PlatformActor->PlatformGroupActor != nullptr)
			{
				Info.PlatformActorGroupNum++;
			}
		}
		else if (PlatformGroupActor)
		{
			Info.PlatformActorGroupNum++;
		}
	}

	return  Info;
}

void UUKPlatformToolUtils::PlatformGroup(bool IsBox)
{
	TArray<AUKPlatformActor*> Actors;
	GEditor->GetSelectedActors()->GetSelectedObjects<AUKPlatformActor>(Actors);
	
	ULevel* ActorLevel = nullptr;
	bool bActorsInSameLevel = true;
	TArray<AUKPlatformActor*> FinalActorList;
	
	for (AUKPlatformActor* PlatformActor : Actors)
	{
		if (!ActorLevel)
		{
			ActorLevel = PlatformActor->GetLevel();
		}
		else if (ActorLevel != PlatformActor->GetLevel())
		{
			bActorsInSameLevel = false;
			break;
		}

		FinalActorList.Add(PlatformActor);
	}

	if (bActorsInSameLevel)
	{
		if (FinalActorList.Num() > 0)
		{
			UWorld* World = ActorLevel->OwningWorld;
			
			FActorSpawnParameters SpawnInfo;
			SpawnInfo.OverrideLevel = ActorLevel;
			AUKPlatformGroupActor* GroupActor = World->SpawnActor<AUKPlatformGroupActor>(SpawnInfo);
			FString UniqueID = FString::FormatAsNumber(GroupActor->GetUniqueID());
			GroupActor->SetActorLabel(GroupActor->GetActorLabel()+ TEXT("_") + UniqueID);
			
			bool bActorsInSameFolder = true;
			FName FolderPath;

			for (AUKPlatformActor* FinalActor : FinalActorList)
			{
				GroupActor->Add(*FinalActor);

				if (bActorsInSameFolder)
				{
					if (FolderPath.IsNone())
					{
						FolderPath = FinalActor->GetFolderPath();
					}
					else if (FolderPath != FinalActor->GetFolderPath())
					{
						bActorsInSameFolder = false;
						FolderPath = FName();
					}
				}
			}

			GroupActor->SetFolderPath(FolderPath);
			GroupActor->CenterGroupLocation(IsBox);
			GEditor->NoteSelectionChange();
			
			if (GroupActor)
			{
				GEditor->SelectActor(GroupActor, /*bIsSelected*/true, /*bNotify*/false);
			}
		}
	}
}

void UUKPlatformToolUtils::PlatformUnGroup()
{
	TArray<AUKPlatformActor*> Actors;
	GEditor->GetSelectedActors()->GetSelectedObjects<AUKPlatformActor>(Actors);
	
	ULevel* ActorLevel = nullptr;
	bool bActorsInSameLevel = true;
	TArray<AUKPlatformActor*> FinalActorList;
	
	for (AUKPlatformActor* PlatformActor : Actors)
	{
		if (!ActorLevel)
		{
			ActorLevel = PlatformActor->GetLevel();
		}
		else if (ActorLevel != PlatformActor->GetLevel())
		{
			bActorsInSameLevel = false;
			break;
		}

		if(PlatformActor->PlatformGroupActor)
		{
			FinalActorList.Add(PlatformActor);
		}
	}

	if (bActorsInSameLevel)
	{
		if (FinalActorList.Num() > 0)
		{
			UWorld* World = ActorLevel->OwningWorld;

			for (AUKPlatformActor* PlatformActor : FinalActorList)
			{
				TObjectPtr<AUKPlatformGroupActor> TempPlatformGroupActor = PlatformActor->PlatformGroupActor;
				PlatformActor->PlatformGroupActor->Remove(*PlatformActor);

				if(TempPlatformGroupActor->PlatformActors.IsEmpty())
				{
					UWorld* MyWorld = TempPlatformGroupActor->GetWorld();
					if( MyWorld )
					{
						MyWorld->ModifyLevel(TempPlatformGroupActor->GetLevel());
			
						MarkPackageDirty();

						if(!IsGarbageCollecting())
						{
							FScopedRefreshAllBrowsers LevelRefreshAllBrowsers;

							GEditor->SelectActor( TempPlatformGroupActor, /*bSelected=*/ false, /*bNotify=*/ false );
							ULayersSubsystem* LayersSubsystem = GEditor->GetEditorSubsystem<ULayersSubsystem>();
							LayersSubsystem->DisassociateActorFromLayers( TempPlatformGroupActor );
							MyWorld->EditorDestroyActor( TempPlatformGroupActor, false );			
				
							LevelRefreshAllBrowsers.Request();
						}
					}
				}
			}
		}
	}
}