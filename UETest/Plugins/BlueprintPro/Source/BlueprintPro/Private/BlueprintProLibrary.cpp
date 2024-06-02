// BlueprintPro Plugin - Copyright (c) 2023 XUSJTUER. All Rights Reserved.


#include "BlueprintProLibrary.h"
#include "Kismet/KismetArrayLibrary.h"
#include "GameFramework/Actor.h"
#include "Components/PrimitiveComponent.h"
#include "LevelUtils.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/Level.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"



namespace BlueprintProHelper
{
	template <class T> T* FindFProperty(const UStruct* Owner, FName FieldName)
	{
		// We know that a "none" field won't exist in this Struct
		if (FieldName.IsNone())
		{
			return nullptr;
		}

		// Search by comparing FNames (INTs), not strings
		for (TFieldIterator<T>It(Owner); It; ++It)
		{
			if ((It->GetFName() == FieldName) || (It->GetAuthoredName() == FieldName.ToString()))
			{
				return *It;
			}
		}
		// If we didn't find it, return no field
		return nullptr;
	}
}

//Based on UEditorEngine::MoveViewportCamerasToActor(const TArray<AActor*> &Actors, bool bActiveViewportOnly)
FTransform UBlueprintProLibrary::GetViewportTransformFromActors(const TArray<AActor*>& Actors, const float Factor /*= 1.0f*/)
{
	return GetViewportTransformFromActorsAndComponents(Actors, TArray<UPrimitiveComponent*>(),Factor);
}


FTransform UBlueprintProLibrary::GetViewportTransformFromActorsAndComponents(const TArray<AActor*>& Actors, const TArray<UPrimitiveComponent*>& Components, const float Factor /*= 1.0f*/)
{
	if (Actors.Num() == 0 && Components.Num() == 0)
	{
		return FTransform::Identity;
	}

	TArray<AActor*> InvisLevelActors;

	// Create a bounding volume of all of the selected actors.
	FBox BoundingBox(ForceInit);

	if (Components.Num() > 0)
	{
		// First look at components
		for (UPrimitiveComponent* PrimitiveComponent : Components)
		{
			if (PrimitiveComponent)
			{
				if (!FLevelUtils::IsLevelVisible(PrimitiveComponent->GetComponentLevel()))
				{
					continue;
				}

				// Some components can have huge bounds but are not visible.  Ignore these components unless it is the only component on the actor
				
#if WITH_EDITOR //UE_5.2
				const bool bIgnore = Components.Num() > 1 && PrimitiveComponent->GetIgnoreBoundsForEditorFocus();
#else
				const bool bIgnore = Components.Num() > 1;
#endif

///*#if WITH_EDITOR *///UE_5.3
//				const bool bIgnore = Components.Num() > 1 && PrimitiveComponent->GetIgnoreBoundsForEditorFocus();
////#else
////				const bool bIgnore = Components.Num() > 1;
////#endif

				if (!bIgnore && PrimitiveComponent->IsRegistered())
				{
					BoundingBox += PrimitiveComponent->Bounds.GetBox();
				}

			}
		}
	}
	else
	{
		TSet<AActor*> AlignActors;
		for (AActor* RootActor : Actors)
		{
			if (RootActor)
			{
				// Don't allow moving the viewport cameras to actors in invisible levels
				if (!FLevelUtils::IsLevelVisible(RootActor->GetLevel()))
				{
					InvisLevelActors.Add(RootActor);
					continue;
				}

				AlignActors.Empty(AlignActors.Num());
				AlignActors.Add(RootActor);
#if WITH_EDITOR
				RootActor->EditorGetUnderlyingActors(AlignActors);
#endif
				for (AActor* AlignActor : AlignActors)
				{
					/* const bool bActorIsEmitter = (Cast<AEmitter>(AlignActor) != NULL);

					if (bActorIsEmitter && bCustomCameraAlignEmitter)
					{
						const FVector DefaultExtent(CustomCameraAlignEmitterDistance, CustomCameraAlignEmitterDistance, CustomCameraAlignEmitterDistance);
						const FBox DefaultSizeBox(AlignActor->GetActorLocation() - DefaultExtent, AlignActor->GetActorLocation() + DefaultExtent);
						BoundingBox += DefaultSizeBox;
					}
					else */if (USceneComponent* RootComponent = AlignActor->GetRootComponent())
					{
						TArray<USceneComponent*> SceneComponents;
						RootComponent->GetChildrenComponents(true, SceneComponents);
						SceneComponents.Add(RootComponent);

						bool bHasAtLeastOnePrimitiveComponent = false;
						for (USceneComponent* SceneComponent : SceneComponents)
						{
							UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(SceneComponent);

							if (PrimitiveComponent && PrimitiveComponent->IsRegistered())
							{
								// Some components can have huge bounds but are not visible.  Ignore these components unless it is the only component on the actor 
#if WITH_EDITOR	//UE_5.2
								//ENGINE_MAJOR_VERSION == 5 &&  1<= ENGINE_MINOR_VERSION 
								const bool bIgnore = SceneComponents.Num() > 1 && PrimitiveComponent->GetIgnoreBoundsForEditorFocus();
#else
								const bool bIgnore = SceneComponents.Num() > 1;
#endif
								////UE_5.3
								//const bool bIgnore = SceneComponents.Num() > 1 && PrimitiveComponent->GetIgnoreBoundsForEditorFocus();

								if (!bIgnore)
								{
									//FBox LocalBox(ForceInit);
									//if (GLevelEditorModeTools().ComputeBoundingBoxForViewportFocus(AlignActor, PrimitiveComponent, LocalBox))
									//{
									//	BoundingBox += LocalBox;
									//}
									//else
									//{
									BoundingBox += PrimitiveComponent->Bounds.GetBox();
									//}

									bHasAtLeastOnePrimitiveComponent = true;
								}
							}
						}

						if (!bHasAtLeastOnePrimitiveComponent)
						{
							BoundingBox += RootComponent->GetComponentLocation();
						}

					}
				}
			}
		}
	}

	return GetViewportTransformFromBoundingBox(BoundingBox, Factor);

}

//Based on FEditorViewportClient::FocusViewportOnBox
FTransform UBlueprintProLibrary::GetViewportTransformFromBoundingBox(const FBox& BoundingBox, const float Factor /*= 1.0f*/)
{
	const FVector Position = BoundingBox.GetCenter();
	float Radius = FMath::Max<FVector::FReal>(BoundingBox.GetExtent().Size(), 10.f);

	float AspectRatio = 0;
	FVector2D ViewportSize;
	GWorld->GetGameViewport()->GetViewportSize(ViewportSize);
	if (ViewportSize.X > 0 && ViewportSize.Y > 0)
	{
		AspectRatio = ViewportSize.X / ViewportSize.Y;
	}

	if (AspectRatio > 1.0f)
	{
		Radius *= AspectRatio;
	}

	UWorld* World =
#if WITH_EDITOR
		GIsEditor ? GWorld :
#endif // WITH_EDITOR
		(GEngine->GetWorldContexts().Num() > 0 ? GEngine->GetWorldContexts()[0].World() : nullptr);

	APlayerCameraManager* PlayerCameraManager = World->GetFirstPlayerController()->PlayerCameraManager;

	/**
	 * Now that we have a adjusted radius, we are taking half of the viewport's FOV,
	 * converting it to radians, and then figuring out the camera's distance from the center
	 * of the bounding sphere using some simple trig.  Once we have the distance, we back up
	 * along the camera's forward vector from the center of the sphere, and set our new view location.
	 */

	const float factor = FMath::Clamp(Factor, 1.0f, 2.0f);

	const float HalfFOVRadians = FMath::DegreesToRadians(PlayerCameraManager->GetFOVAngle() / 2.0f);
	//const float HalfFOVRadians = FMath::DegreesToRadians(ViewFOV / 2.0f);
	const float DistanceFromSphere = Radius / FMath::Tan(HalfFOVRadians);
	//FVector CameraOffsetVector = ViewTransform.GetRotation().Vector() * -DistanceFromSphere;
	FVector CameraOffsetVector = World->GetFirstPlayerController()->GetControlRotation().Vector() * -DistanceFromSphere;

	FVector ViewPosition = Position + factor * CameraOffsetVector;
	FRotator ViewRotation = UKismetMathLibrary::FindLookAtRotation(Position + CameraOffsetVector, Position);
	FTransform ViewTransform(ViewRotation, ViewPosition, FVector::OneVector);

	return ViewTransform;

}


void UBlueprintProLibrary::Array_IsValidIndex(const TArray<int32>& TargetArray, int32 Index, EIndexValidResult& Result)
{
	// We should never hit these!  They're stubs to avoid NoExport on the class.  Call the Generic* equivalent instead
	check(0);
}

void UBlueprintProLibrary::Array_Slice(const TArray<int32>& TargetArray, int32 Index, int32 Length, TArray<int32>& OutArray)
{
	// We should never hit these!  They're stubs to avoid NoExport on the class.  Call the Generic* equivalent instead
	check(0);
}

void UBlueprintProLibrary::Array_Filter(const UObject* Object, const FName FilterBy, const TArray<int32>& TargetArray, TArray<int32>& FilteredArray)
{
	// We should never hit these!  They're stubs to avoid NoExport on the class.  Call the Generic* equivalent instead
	check(0);
}

void UBlueprintProLibrary::FilterObejcts(const TArray<UObject*>& TargetArray, FArrayFilterDelegate Event, TArray<UObject*>& FilteredArray)
{
	FilteredArray.Empty();

	for (UObject* Object : TargetArray)
	{
		bool bReturnValue = Event.Execute(Object);
		if (bReturnValue)
		{
			FilteredArray.Add(Object);
		}
	}
}

void UBlueprintProLibrary::Array_Max(const TArray<int32>& TargetArray, UObject* FunctionOwner, const FName FunctionName, int32& IndexOfMaxValue, int32& MaxValue)
{
	// We should never hit these!  They're stubs to avoid NoExport on the class.  Call the Generic* equivalent instead
	check(0);
}

void UBlueprintProLibrary::Array_Min(const TArray<int32>& TargetArray, UObject* FunctionOwner, const FName FunctionName, int32& IndexOfMinValue, int32& MinValue)
{
	// We should never hit these!  They're stubs to avoid NoExport on the class.  Call the Generic* equivalent instead
	check(0);
}

void UBlueprintProLibrary::GenericArray_IsValidIndex(void* ArrayAddr, FArrayProperty* ArrayProperty, int32 Index, EIndexValidResult& Result)
{
	bool bValidIndex = UKismetArrayLibrary::GenericArray_IsValidIndex(ArrayAddr, ArrayProperty, Index);
	Result = bValidIndex ? EIndexValidResult::Valid : EIndexValidResult::Invalid;
}

void UBlueprintProLibrary::GenericArray_Slice(void* TargetArray, const FArrayProperty* ArrayProp, int32 Index, int32 Length, void* OutArray)
{
	if (TargetArray && OutArray)
	{
		FScriptArrayHelper TargetArrayHelper(ArrayProp, TargetArray);
		FScriptArrayHelper OutArrayHelper(ArrayProp, OutArray);

		if (TargetArrayHelper.Num() <= 0 || TargetArrayHelper.Num() <= Index || Length <= 0)
		{
			return;
		}
		if (Index < 0)
		{
			Index = 0;
		}
		if (TargetArrayHelper.Num() - Index < Length)
		{
			Length = TargetArrayHelper.Num() - Index;
		}
		/** Based on UKismetArrayLibrary::GenericArray_Append() */
		FProperty* InnerProp = ArrayProp->Inner;
		OutArrayHelper.AddValues(Length);
		for (int32 i = 0, j = Index; (j < TargetArrayHelper.Num() && (j < Index + Length)); ++i, ++j)
		{
			InnerProp->CopySingleValueToScriptVM(OutArrayHelper.GetRawPtr(i), TargetArrayHelper.GetRawPtr(j));
		}
	}
}


void UBlueprintProLibrary::GenericArray_Filter(UObject* Object, UFunction* FilterFunction, const FArrayProperty* ArrayProp, void* SrcArrayAddr, void* FilterArrayAddr)
{
	//check input parameters
	if (!Object || !FilterFunction || !SrcArrayAddr)
	{
		return;
	}
	// filter function return property
	FBoolProperty* ReturnProp = CastField<FBoolProperty>(FilterFunction->GetReturnProperty());
	if (!ReturnProp)
	{
		/// The return Property of filter function must be bool and named "ReturnValue"
		UE_LOG(LogBlueprintProLibrary, Error, TEXT("Function:%s, Line:%d, Message: Ensure return value of the Filter Function is a variable of type bool called returnValue !"), *FString(__FUNCTION__), __LINE__);
		return;
	}

	// Get function parameter list
	TArray<FProperty*> ParamterList;
	for (TFieldIterator<FProperty> It(FilterFunction); It; ++It)
	{
		FProperty* FuncParameter = *It;
		/// Get filter function parameters
		ParamterList.Emplace(FuncParameter);
	}
	/// Make sure the first input parameters of filter function is same to array inner
	if (!ParamterList[0]->SameType(ArrayProp->Inner))
	{
		/// The  property of 1st input parameter of filter function must be same as array member
		UE_LOG(LogBlueprintProLibrary, Error, TEXT("Function:%s, Line:%d, Message: Ensure that the type of the input parameter of Filter Function is the same as that of the array element !"), *FString(__FUNCTION__), __LINE__);
		return;
	}

	FScriptArrayHelper ArrayHelper(ArrayProp, SrcArrayAddr);
	FScriptArrayHelper FilterArray(ArrayProp, FilterArrayAddr);

	FProperty* InnerProp = ArrayProp->Inner;
	const int32 PropertySize = InnerProp->ElementSize * InnerProp->ArrayDim;
	// filter function parameters address, 1 input parameter(array item) and 1 return parameter (bool)
	uint8* FilterParamsAddr = (uint8*)FMemory::Malloc(PropertySize + 1);
	for (int32 i = 0; i < ArrayHelper.Num(); i++)
	{
		FMemory::Memzero(FilterParamsAddr, PropertySize + 1);
		// get array member and assign value to filter function input parameter
		InnerProp->CopyCompleteValueFromScriptVM(FilterParamsAddr, ArrayHelper.GetRawPtr(i));
		//process filter function
		Object->ProcessEvent(FilterFunction, FilterParamsAddr);
		if (ReturnProp && ReturnProp->GetPropertyValue(FilterParamsAddr + PropertySize))
		{
			// add item to filter array
			UKismetArrayLibrary::GenericArray_Add(FilterArrayAddr, ArrayProp, ArrayHelper.GetRawPtr(i));
		}
	}
	// relesed memory
	FMemory::Free(FilterParamsAddr);
}


void UBlueprintProLibrary::GenericArray_Max(void* TargetArrayAddr, FArrayProperty* ArrayProp, UObject* OwnerObject, UFunction* CompareByFunction, int32& FoundIndex, void* MaxElemAddr, bool bGetMax/*= true*/)
{

	if (!IsValidComparator(CompareByFunction, ArrayProp))
	{
		UE_LOG(LogBlueprintProLibrary, Error, TEXT("Function:%s, Line:%d, ERROR: This function %s can be used for comparing the size of array elements "), *FString(__FUNCTION__), __LINE__, *CompareByFunction->GetFullName());
		return;
	}

	//Get return property of CompareBy function.
	FBoolProperty* ReturnProp = CastField<FBoolProperty>(CompareByFunction->GetReturnProperty());

	FScriptArrayHelper ArrayHelper(ArrayProp, TargetArrayAddr);
	FProperty* InnerProp = ArrayProp->Inner;
	const int32 PropertySize = InnerProp->ElementSize * InnerProp->ArrayDim;

	if (ArrayHelper.Num() == 0)
	{
		FoundIndex = INDEX_NONE;
		InnerProp->InitializeValue(MaxElemAddr);
		UE_LOG(LogBlueprintProLibrary, Log, TEXT("GenericArray_Max/Min -> Target array is empty."));
		return;
	}
	// CompareBy function parameters address, 2 input parameter(array item) and 1 return parameter (bool)
	uint8* FuncParamsAddr = (uint8*)FMemory::Malloc(CompareByFunction->ParmsSize); /// note:ParamsSize = PropertySize* 2 +1
	FoundIndex = 0;

	FMemory::Memzero(FuncParamsAddr, CompareByFunction->ParmsSize);
	// assign value to first parameter of CompareBy funcion.
	InnerProp->CopyCompleteValueFromScriptVM(FuncParamsAddr, ArrayHelper.GetRawPtr(FoundIndex));

	for (int32 i = 1; i < ArrayHelper.Num(); i++)
	{
		// assign value to second parameter of CompareBy funcion.
		InnerProp->CopyCompleteValueFromScriptVM(FuncParamsAddr + PropertySize, ArrayHelper.GetRawPtr(i));
		//process custom CompareBy function
		OwnerObject->ProcessEvent(CompareByFunction, FuncParamsAddr);
		if (bGetMax)
		{
			if (ReturnProp && ReturnProp->GetPropertyValue(FuncParamsAddr + PropertySize * 2))
			{
				// first param ->max value, second param->array[i]
				FoundIndex = i;
				InnerProp->CopyCompleteValueFromScriptVM(FuncParamsAddr, ArrayHelper.GetRawPtr(FoundIndex));
			}
		}
		else
		{
			if (ReturnProp && !ReturnProp->GetPropertyValue(FuncParamsAddr + PropertySize * 2))
			{
				// first param ->min value, second param->array[i]
				FoundIndex = i;
				InnerProp->CopyCompleteValueFromScriptVM(FuncParamsAddr, ArrayHelper.GetRawPtr(FoundIndex));
			}
		}
	}
	InnerProp->CopyCompleteValueFromScriptVM(MaxElemAddr, ArrayHelper.GetRawPtr(FoundIndex));
	// release memory
	FMemory::Free(FuncParamsAddr);
}


TArray<int32>& UBlueprintProLibrary::SortIntArray(TArray<int32>& TargetArray, const bool bAscending /*= true*/)
{
	if (bAscending)
	{
		TargetArray.Sort([](const int32 A, const int32 B)
			{
				return A < B;
			});
	}
	else
	{
		TargetArray.Sort([](const int32 A, const int32 B)
			{
				return A > B;
			});
	}
	return TargetArray;
}

TArray<float>& UBlueprintProLibrary::SortFloatArray(TArray<float>& TargetArray, const bool bAscending /*= true*/)
{
	if (bAscending)
	{
		TargetArray.Sort([](const float A, const float B)
			{
				return A < B;
			});
	}
	else
	{
		TargetArray.Sort([](const float A, const float B)
			{
				return A > B;
			});
	}
	return TargetArray;
}

void UBlueprintProLibrary::Array_QuickSort(const TArray<int32>& TargetArray, UObject* FunctionOwner, FName FunctionName)
{
	// We should never hit these!  They're stubs to avoid NoExport on the class.  Call the Generic* equivalent instead
	check(0);
	return;
}

void UBlueprintProLibrary::Array_SortV2(const TArray<int32>& TargetArray, FName PropertyName, bool bAscending)
{
	// We should never hit this!  stubs to avoid NoExport on the class.
	check(0);
}

void UBlueprintProLibrary::GenericArray_QuickSort(UObject* OwnerObject, UFunction* ComparedBy, void* TargetArray, FArrayProperty* ArrayProp)
{
	if (!ComparedBy || !OwnerObject || !TargetArray)
	{
		return;
	}
	else if (!IsValidComparator(ComparedBy, ArrayProp))
	{
		return;
	}
	// Optimal
	UKismetArrayLibrary::GenericArray_Shuffle(TargetArray, ArrayProp);

	// Begin sort array
	FScriptArrayHelper ArrayHelper(ArrayProp, TargetArray);

	if (ArrayHelper.Num() < 2)
	{
		return;
	}
	else
	{
		FProperty* InnerProp = ArrayProp->Inner;
		QuickSort_Recursive(OwnerObject, ComparedBy, InnerProp, ArrayHelper, 0, ArrayHelper.Num() - 1);
	}
}

void UBlueprintProLibrary::QuickSort_Recursive(UObject* OwnerObject, UFunction* ComparedBy, FProperty* InnerProp, FScriptArrayHelper& ArrayHelper, int32 Low, int32 High)
{
	if (Low < High)
	{
		int32 Pivot = QuickSort_Partition(OwnerObject, ComparedBy, InnerProp, ArrayHelper, Low, High);
		QuickSort_Recursive(OwnerObject, ComparedBy, InnerProp, ArrayHelper, Low, Pivot - 1);
		QuickSort_Recursive(OwnerObject, ComparedBy, InnerProp, ArrayHelper, Pivot + 1, High);
	}
}

int32 UBlueprintProLibrary::QuickSort_Partition(UObject* OwnerObject, UFunction* ComparedBy, FProperty* InnerProp, FScriptArrayHelper& ArrayHelper, int32 Low, int32 High)
{
	const int32 PropertySize = InnerProp->ElementSize * InnerProp->ArrayDim;
	FBoolProperty* ReturnProp = CastField<FBoolProperty>(ComparedBy->GetReturnProperty());

	// CompareBy function parameters address, 2 input parameter(array item) and 1 return parameter (bool)
	uint8* FuncParamsAddr = (uint8*)FMemory::Malloc(ComparedBy->ParmsSize); /// note:ParamsSize = PropertySize* 2 +1

	FMemory::Memzero(FuncParamsAddr, ComparedBy->ParmsSize);

	/** Based on quick sort of stl */
	InnerProp->CopyCompleteValueFromScriptVM(FuncParamsAddr, ArrayHelper.GetRawPtr(High)); //Params1: LastSmallElem = Array[High]

	int32 i = Low - 1;

	for (int32 j = Low; j < High; j++)
	{
		InnerProp->CopyCompleteValueFromScriptVM(FuncParamsAddr + PropertySize, ArrayHelper.GetRawPtr(j)); // Param2: Array[i]
		OwnerObject->ProcessEvent(ComparedBy, FuncParamsAddr);
		if (ReturnProp && ReturnProp->GetPropertyValue(FuncParamsAddr + PropertySize * 2))
		{
			i++;
			// InnerProp->CopyCompleteValueFromScriptVM(FuncParamsAddr, ArrayHelper.GetRawPtr(j)); // copy min elem to Param1
			ArrayHelper.SwapValues(i, j);
		}
	}
	ArrayHelper.SwapValues(i + 1, High);
	// release memory
	FMemory::Free(FuncParamsAddr);

	return i + 1;

}

bool UBlueprintProLibrary::IsValidComparator(UFunction* ComparedBy, FArrayProperty* ArrayProp)
{
	// check CompareBy function's parameter number
	if ((!ComparedBy || (ComparedBy->NumParms != 3)))
	{
		UE_LOG(LogBlueprintProLibrary, Error, TEXT("Function:%s, Line:%d, ERROR: CompareBy function should only have 3 parameters !"), *FString(__FUNCTION__), __LINE__);
		return false;
	}

	//Get return property of CompareBy function.
	FBoolProperty* ReturnProp = CastField<FBoolProperty>(ComparedBy->GetReturnProperty());
	if (!ReturnProp)
	{
		/// The return Property of max function must be bool and named "ReturnValue"
		UE_LOG(LogBlueprintProLibrary, Error, TEXT("Function:%s, Line:%d, ERROR: Return value of CompareBy function should be bool type, and named ReturnValue !"), *FString(__FUNCTION__), __LINE__);
		return false;
	}

	// Get all parameters of CompareBy function
	TArray<FProperty*> ParamterList;
	for (TFieldIterator<FProperty> It(ComparedBy); It; ++It)
	{
		FProperty* FuncParameter = *It;
		ParamterList.Emplace(FuncParameter);
	}

	// Make sure the first/second input parameters of Compare function is same to array inner
	if (!ParamterList[0]->SameType(ArrayProp->Inner) || !ParamterList[1]->SameType(ArrayProp->Inner))
	{
		/// The  property of 1st input parameter of max function must be same as array member
		UE_LOG(LogBlueprintProLibrary, Error, TEXT("Function:%s, Line:%d, ERROR: The parameters type of CompareBy should be same to array member."), *FString(__FUNCTION__), __LINE__);
		return false;
	}
	return true;
}

void UBlueprintProLibrary::GenericArraySortProperty(void* TargetArray, FArrayProperty* ArrayProp, FName PropertyName, bool bAscending)
{
	if (!TargetArray)
	{
		return;
	}
	// Optimal
	UKismetArrayLibrary::GenericArray_Shuffle(TargetArray, ArrayProp);
	FScriptArrayHelper ArrayHelper(ArrayProp, TargetArray);

	FProperty* SortProp = nullptr;
	if (ArrayHelper.Num() < 2)
	{
		return;
	}
	else if (const FObjectProperty* ObjectProp = CastField<const FObjectProperty>(ArrayProp->Inner))
	{
		SortProp = FindFProperty<FProperty>(ObjectProp->PropertyClass, PropertyName);
	}
	else if (const FStructProperty* StructProp = CastField<const FStructProperty>(ArrayProp->Inner))
	{
		SortProp = BlueprintProHelper::FindFProperty<FProperty>(StructProp->Struct, PropertyName);
	}
	else
	{
		SortProp = ArrayProp->Inner;
	}

	if (SortProp)
	{
		QuickSortRecursiveByProperty(ArrayHelper, ArrayProp->Inner, SortProp, 0, ArrayHelper.Num() - 1, bAscending);
	}
}

void UBlueprintProLibrary::QuickSortRecursiveByProperty(FScriptArrayHelper& ArrayHelper, FProperty* InnerProp, FProperty* SortProp, int32 Low, int32 High, bool bAscending)
{
	if (Low < High)
	{
		int32 Pivot = QuickSortPartitionByProperty(ArrayHelper, InnerProp, SortProp, Low, High, bAscending);

		QuickSortRecursiveByProperty(ArrayHelper, InnerProp, SortProp, Low, Pivot - 1, bAscending);
		QuickSortRecursiveByProperty(ArrayHelper, InnerProp, SortProp, Pivot + 1, High, bAscending);
	}
}

int32 UBlueprintProLibrary::QuickSortPartitionByProperty(FScriptArrayHelper& ArrayHelper, FProperty* InnerProp, FProperty* SortProp, int32 Low, int32 High, bool bAscending)
{
	int32 i = Low - 1;

	for (int32 j = Low; j < High; j++)
	{
		if (CompareArrayItemsByProperty(ArrayHelper, InnerProp, SortProp, j, High, bAscending))
		{
			i++;
			ArrayHelper.SwapValues(i, j);
		}
	}
	ArrayHelper.SwapValues(i + 1, High);
	return i + 1;
}

bool UBlueprintProLibrary::CompareArrayItemsByProperty(FScriptArrayHelper& ArrayHelper, FProperty* InnerProp, FProperty* SortProp, int32 j, int32 High, bool bAscending)
{
	bool bGreater = false;
	void* LeftValueAddr = nullptr;
	void* RightValueAddr = nullptr;
	if (const FObjectProperty* ObjectProp = CastField<const FObjectProperty>(InnerProp))
	{
		UObject* LeftObject = ObjectProp->GetObjectPropertyValue(ArrayHelper.GetRawPtr(j));
		UObject* RightObject = ObjectProp->GetObjectPropertyValue(ArrayHelper.GetRawPtr(High));

		LeftValueAddr = SortProp->ContainerPtrToValuePtr<void>(LeftObject);
		RightValueAddr = SortProp->ContainerPtrToValuePtr<void>(RightObject);
	}
	else
	{
		LeftValueAddr = SortProp->ContainerPtrToValuePtr<void>(ArrayHelper.GetRawPtr(j));
		RightValueAddr = SortProp->ContainerPtrToValuePtr<void>(ArrayHelper.GetRawPtr(High));
	}

	if (const FNumericProperty* NumericProp = CastField<const FNumericProperty>(SortProp))
	{
		if (NumericProp->IsFloatingPoint())
		{
			bGreater = NumericProp->GetFloatingPointPropertyValue(LeftValueAddr) < NumericProp->GetFloatingPointPropertyValue(RightValueAddr);
		}
		else if (NumericProp->IsInteger())
		{
			bGreater = NumericProp->GetSignedIntPropertyValue(LeftValueAddr) < NumericProp->GetSignedIntPropertyValue(RightValueAddr);
		}
	}
	else if (const FBoolProperty* BoolProp = CastField<const FBoolProperty>(SortProp))
	{
		bGreater = !BoolProp->GetPropertyValue(LeftValueAddr) && BoolProp->GetPropertyValue(RightValueAddr);
	}
	else if (const FNameProperty* NameProp = CastField<const FNameProperty>(SortProp))
	{
		bGreater = NameProp->GetPropertyValue(LeftValueAddr).ToString() < NameProp->GetPropertyValue(RightValueAddr).ToString();
	}
	else if (const FStrProperty* StringProp = CastField<const FStrProperty>(SortProp))
	{
		bGreater = (StringProp->GetPropertyValue(LeftValueAddr) < StringProp->GetPropertyValue(RightValueAddr));
	}
	else if (const FTextProperty* TextProp = CastField<const FTextProperty>(SortProp))
	{
		bGreater = (TextProp->GetPropertyValue(LeftValueAddr).ToString() < TextProp->GetPropertyValue(RightValueAddr).ToString());
	}

	return bGreater == bAscending;
}



void UBlueprintProLibrary::SetDataTableRowFromName(UDataTable* Table, FName RowName, const int32& RowData)
{
	// We should never hit this!  stubs to avoid NoExport on the class.
	check(0);
}

bool UBlueprintProLibrary::Generic_SetDataTableRowFromName(const UDataTable* Table, FName RowName, void* OutRowPtr)
{
	bool bFoundRow = false;

	//if (OutRowPtr && Table)
	//{
	//	void* RowPtr = Table->FindRowUnchecked(RowName);

	//	if (RowPtr != nullptr)
	//	{
	//		const UScriptStruct* StructType = Table->GetRowStruct();

	//		if (StructType != nullptr)
	//		{
	//			StructType->CopyScriptStruct(OutRowPtr, RowPtr);
	//			bFoundRow = true;
	//		}
	//	}
	//}

	return bFoundRow;
}
