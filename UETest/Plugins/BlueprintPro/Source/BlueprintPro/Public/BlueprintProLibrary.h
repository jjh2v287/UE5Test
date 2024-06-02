// BlueprintPro Plugin - Copyright (c) 2023 XUSJTUER. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BlueprintProLibrary.generated.h"

class UDataTable;

// Declare General Log Category
DEFINE_LOG_CATEGORY_STATIC(LogBlueprintProLibrary, Log, All);

UENUM()
enum class EIndexValidResult : uint8
{
	Valid UMETA(DisplayName = "True"),
	Invalid UMETA(DisplayName = "False"),
};

//Object Array filter delegate
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FArrayFilterDelegate, const UObject*, Object);

/**
 *
 */
UCLASS()
class BLUEPRINTPRO_API UBlueprintProLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Calculate the viewport transformation matrix based on the bounding boxes of focus Actors.
	 *
	 * @param Actors	An array of AActor pointers representing focus Actors.
	 * @param Factor	A scaling factor used to adjust the viewport perspective, clamp between 1.0 and 2.0.
	 * @return The viewport transformation matrix converting focus Actors' bounding boxes from world to viewport space.
	 */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Focus Actors", Keywords = "BlueprintPro,Focus,ViewTransform,Actor"), Category = "BlueprintPro|Focus")
	static FTransform GetViewportTransformFromActors(const TArray<AActor*>& Actors, const float Factor = 1.0f);

	/**
	 * Calculate the viewport transformation matrix based on the bounding boxes of focus Actors and Components.
	 *
	 * @param Actors		An array of AActor pointers representing focus Actors.
	 * @param Components	An array of Components pointers representing focus Components.
	 * @param Factor		A scaling factor used to adjust the viewport perspective, clamp between 1.0 and 2.0.
	 * @return The viewport transformation matrix converting focus Actors' bounding boxes from world to viewport space.
	 */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Focus Actors and Components", Keywords = "BlueprintPro,Focus,ViewTransform,Actor"), Category = "BlueprintPro|Focus")
	static FTransform GetViewportTransformFromActorsAndComponents(const TArray<AActor*>& Actors, const TArray<UPrimitiveComponent*>& Components, const float Factor = 1.0f);

	/**
	 * Calculate the viewport transformation matrix based on BoundingBox.
	 *
	 * @param BoundingBox	focus BoundingBox
	 * @param Factor		A scaling factor used to adjust the viewport perspective, clamp between 1.0 and 2.0.
	 * @return The viewport transformation matrix converting focus bounding boxe from world to viewport space.
	 */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Focus BoundingBox", Keywords = "BlueprintPro,Focus,ViewTransform,Actor"), Category = "BlueprintPro|Focus")
	static FTransform GetViewportTransformFromBoundingBox(const FBox& BoundingBox, const float Factor = 1.0f);

public://Array

	/*
	 *Tests if IndexToTest is valid, i.e. greater than or equal to zero, and less than the number of elements in TargetArray.
	 *
	 *@param	TargetArray		Array to use for the IsValidIndex test
	 *@param	Index		The Index, that we want to test for being valid
	 *@return	True if the Index is Valid, i.e. greater than or equal to zero, and less than the number of elements in TargetArray.
	*/
	UFUNCTION(BlueprintCallable, CustomThunk, meta = (DisplayName = "Is Valid Index ?", ArrayParm = "TargetArray", Keywords = "array,index,valid", Index = 0, ExpandEnumAsExecs = "Result", BlueprintThreadSafe), Category = "Utilities|Array")
	static void Array_IsValidIndex(const TArray<int32>& TargetArray, int32 Index, EIndexValidResult& Result);

	/*
	 *	Slice array into sub array using the given index and length.
	 *
	 *	@param TargetArray	The array to Slices from
	 *	@param Index		Position of the first element
	 *	@param Length		Number of elements in the slice
	 *	@param OutArray		Another slice to copy
	*/
	UFUNCTION(BlueprintCallable, CustomThunk, meta = (DisplayName = "Slice", CompactNodeTitle = "SLICE", ArrayParm = "TargetArray", AutoCreateRefTerm = "TargetArray", ArrayTypeDependentParams = "TargetArray,OutArray", Index = "0", Length = "1"), Category = "Utilities|Array")
	static void Array_Slice(const TArray<int32>& TargetArray, int32 Index, int32 Length, TArray<int32>& OutArray);


	/*
	*Filter an array based on filter function of object.
	*
	*@param	Object		The owner of function
	*@param	FilterBy	Filter function name
						The signature of the filter function should be equivalent to the following:
						bool cmp(const Type1 &a); return value should named "ReturnValue"(bool)
	*@param	TargetArray	 The array to filter from
	*@return	An array containing only those members which meet the filterBy condition.
	*/
	UFUNCTION(BlueprintCallable, CustomThunk, meta = (DisplayName = "Filter Array By Function", CompactNodeTitle = "FILTER", ArrayParm = "TargetArray,FilteredArray", ArrayTypeDependentParams = "TargetArray,FilteredArray", AutoCreateRefTerm = "FilteredArray", DefaultToSelf = "Object", AdvancedDisplay = "Object"), Category = "Utilities|Array")
	static void Array_Filter(const UObject* Object, const FName FilterBy, const TArray<int32>& TargetArray, TArray<int32>& FilteredArray);

	/*
	 *Filter an UObject array 
	 *
	 *@param	TargetArray		The array to filter from
	 *@param	Event	Filter object function	
	 *@return	FilteredArray An array containing only those members which meet the filterBy condition.
	*/
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Filter Objects"), Category = "Utilities|Array")
	static void FilterObejcts(const TArray<UObject*>& TargetArray,  FArrayFilterDelegate Event, TArray<UObject*>& FilteredArray);

	/*
	*Max of Array based on CompareBy function(Overloads Operator "<") of object.
	*
	*@param	TargetArray		Target array to find max
	*@param	FunctionOwner	The owner of  CompareBy function,(DefaultToSelf)
	*@param	FunctionName	Function name of overloads operator "<" function
						The signature of the comparison function should be equivalent to the following:
						bool cmp(const Type1 &a, const Type2 &b); return value should named "ReturnValue"(bool)
	*@param	IndexOfMaxValue		Index of max element of array
	*@param	MaxValue		Max member of array
	*/
	UFUNCTION(BlueprintCallable, CustomThunk, meta = (DisplayName = "Max Array Elem", CompactNodeTitle = "MAX", ArrayParm = "TargetArray", ArrayTypeDependentParams = "MaxValue", AutoCreateRefTerm = "MaxValue", DefaultToSelf = "FunctionOwner", AdvancedDisplay = "FunctionOwner"), Category = "Utilities|Array")
	static void Array_Max(const TArray<int32>& TargetArray, UObject* FunctionOwner, const FName FunctionName, int32& IndexOfMaxValue, int32& MaxValue);

	/*
	*Min of Array based on CompareBy function(Overloads Operator "<") of object.
	*
	*@param	TargetArray		Target array to find min
	*@param	FunctionOwner	The owner of  CompareBy function,(DefaultToSelf)
	*@param	FunctionName	Function name of overloads operator "<" function
						The signature of the comparison function should be equivalent to the following:
						bool cmp(const Type1 &a, const Type2 &b); return value should named "ReturnValue"(bool)
	*@param	IndexOfMinValue		Index of Min element of array
	*@param	MinValue		Min member of array
	*/
	UFUNCTION(BlueprintCallable, CustomThunk, meta = (DisplayName = "Min Array Elem", CompactNodeTitle = "MIN", ArrayParm = "TargetArray", ArrayTypeDependentParams = "MinValue", AutoCreateRefTerm = "MinValue", DefaultToSelf = "FunctionOwner", AdvancedDisplay = "FunctionOwner"), Category = "Utilities|Array")
	static void Array_Min(const TArray<int32>& TargetArray, UObject* FunctionOwner, const FName FunctionName, int32& IndexOfMinValue, int32& MinValue);

	/** Sorts a Reference of the given float array. */
	UFUNCTION(BlueprintCallable, Category = "Utilities|Array", meta = (DisplayName = "Array Sort (Int)", KeyWords = "array,sort,int", CompactNodeTitle = "SORT"))
	static TArray<int32>& SortIntArray(UPARAM(ref) TArray<int32>& TargetArray, const bool bAscending = true);

	/** Sorts a Reference of the given float array. */
	UFUNCTION(BlueprintCallable, Category = "Utilities|Array", meta = (DisplayName = "Array Sort (Float)", KeyWords = "array,sort,float", CompactNodeTitle = "SORT"))
	static TArray<float>& SortFloatArray(UPARAM(ref) TArray<float>& TargetArray, const bool bAscending = true);

	/**
	 *	Generic sort array by function based on quicksort algorithm.
	 * @param TargetArray,  The array to sort
	 * @param FunctionOwner,  The object that owns sort function, default to self
	 * @param FunctionName, array elem campare function name, sort function must have 3 parameters.
	   2 input vlaue(type: same to array memember) ,1 return value(bool) ,return parameter must be named "ReturnValue"
	*/
	UFUNCTION(BlueprintCallable, CustomThunk, meta = (DisplayName = "Array Sort By Function", Keywords = "Array,Sort,Wildcard", CompactNodeTitle = "SORT", ArrayParm = "TargetArray", AutoCreateRefTerm = "TargetArray", DefaultToSelf = "FunctionOwner", AdvancedDisplay = "FunctionOwner"), Category = "Utilities|Array")
	static void Array_QuickSort(const TArray<int32>& TargetArray, UObject* FunctionOwner, FName FunctionName);


	/** Generic sort array by property based on quicksort algorithm.
	*
	*	@param	TargetArray	Target array to sort
	*	@param	PropertyName	Name is the variable to sort by for struct or object array. Otherwise, the parameter is ignored.
	*	@param	bAscending	If true, sort by ascending order.
	*/
	UFUNCTION(BlueprintCallable, CustomThunk, meta = (ArrayParm = "TargetArray", BlueprintInternalUseOnly = "true"), Category = "Utilities|Array")
	static void Array_SortV2(const TArray<int32>& TargetArray, FName PropertyName, bool bAscending);

private://Array

	// Is valid array index
	DECLARE_FUNCTION(execArray_IsValidIndex)
	{
		Stack.MostRecentProperty = nullptr;
		Stack.StepCompiledIn<FArrayProperty>(NULL);
		void* ArrayAddr = Stack.MostRecentPropertyAddress;
		FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Stack.MostRecentProperty);
		if (!ArrayProperty)
		{
			Stack.bArrayContextFailed = true;
			return;
		}
		P_GET_PROPERTY(FIntProperty, IndexToTest);
		P_GET_ENUM_REF(EIndexValidResult, Result);
		P_FINISH;

		P_NATIVE_BEGIN;
		UBlueprintProLibrary::GenericArray_IsValidIndex(ArrayAddr, ArrayProperty, IndexToTest, Result);
		P_NATIVE_END;
	}

	// 	static void Array_Slice(const TArray<int32>& TargetArray, int32 From, int32 To, TArray<int32>& OutArray);
	DECLARE_FUNCTION(execArray_Slice)
	{
		Stack.MostRecentProperty = nullptr;
		Stack.StepCompiledIn<FArrayProperty>(NULL);
		void* TargetArrayAddr = Stack.MostRecentPropertyAddress;
		FArrayProperty* TargetArrayProp = CastField<FArrayProperty>(Stack.MostRecentProperty);
		if (!TargetArrayProp)
		{
			Stack.bArrayContextFailed = true;
			return;
		}

		P_GET_PROPERTY(FIntProperty, Index);
		P_GET_PROPERTY(FIntProperty, Length);


		// Retrieve the out array
		Stack.MostRecentProperty = nullptr;
		Stack.StepCompiledIn<FArrayProperty>(NULL);
		void* OutArrayAddr = Stack.MostRecentPropertyAddress;
		FArrayProperty* OutArrayProp = CastField<FArrayProperty>(Stack.MostRecentProperty);
		if (!OutArrayProp)
		{
			Stack.bArrayContextFailed = true;
			return;
		}

		if (!TargetArrayProp->SameType(OutArrayProp))
		{
			return;
		}

		P_FINISH;
		P_NATIVE_BEGIN;
		UBlueprintProLibrary::GenericArray_Slice(TargetArrayAddr, TargetArrayProp, Index, Length, OutArrayAddr);
		P_NATIVE_END;
	}

	// filter array
	DECLARE_FUNCTION(execArray_Filter)
	{
		P_GET_OBJECT(UObject, OwnerObject);
		P_GET_PROPERTY(FNameProperty, FilterBy);

		//Find filter function
		UFunction* const FilterFunction = OwnerObject->FindFunction(FilterBy);
		// Fitler function must have two parameters(1 input / 1 output)
		if (!FilterFunction || (FilterFunction->NumParms != 2))
		{
			UE_LOG(LogTemp, Warning, TEXT("Tooltip -> Array_Filter -> Please check filter function %s "), *FilterBy.ToString());
			return;
		}

		// Get target array  address and ArrayProperty
		Stack.MostRecentProperty = nullptr;
		Stack.StepCompiledIn<FArrayProperty>(NULL);
		void* SrcArrayAddr = Stack.MostRecentPropertyAddress;
		FArrayProperty* SrcArrayProperty = CastField<FArrayProperty>(Stack.MostRecentProperty);
		if (!SrcArrayProperty)
		{
			Stack.bArrayContextFailed = true;
			return;
		}

		// Get filtered array address and arrayproperty
		Stack.MostRecentProperty = nullptr;
		Stack.StepCompiledIn<FArrayProperty>(NULL);
		void* FilterArrayAddr = Stack.MostRecentPropertyAddress;
		FArrayProperty* FilterArrayProperty = CastField<FArrayProperty>(Stack.MostRecentProperty);
		if (!FilterArrayProperty)
		{
			Stack.bArrayContextFailed = true;
			return;
		}
		P_FINISH;

		P_NATIVE_BEGIN;
		// ScrArrayProperty is equal to FilterArrayProperty
		UBlueprintProLibrary::GenericArray_Filter(OwnerObject, FilterFunction, SrcArrayProperty, SrcArrayAddr, FilterArrayAddr);
		P_NATIVE_END;
	}

	// array max
	DECLARE_FUNCTION(execArray_Max)
	{
		Stack.MostRecentProperty = nullptr;
		Stack.StepCompiledIn<FArrayProperty>(NULL);
		void* TargetArrayAddr = Stack.MostRecentPropertyAddress;
		FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Stack.MostRecentProperty);
		if (!ArrayProperty)
		{
			Stack.bArrayContextFailed = true;
			return;
		}

		P_GET_OBJECT(UObject, OwnerObject);
		P_GET_PROPERTY(FNameProperty, CompareByName);
		P_GET_PROPERTY_REF(FIntProperty, OutMaxIndex);

		// find custom compare by function
		UFunction* CompareByFunction = OwnerObject->FindFunction(CompareByName);
		if (!CompareByFunction)
		{
			UE_LOG(LogBlueprintProLibrary, Warning, TEXT("Array_Max -> Please input your custom compare function name to MAX blueprint node."));
			return;
		}

		// Since NewItem isn't really an int, step the stack manually
		const FProperty* InnerProp = ArrayProperty->Inner;
		const int32 PropertySize = InnerProp->ElementSize * InnerProp->ArrayDim;
		void* StorageSpace = FMemory_Alloca(PropertySize);
		InnerProp->InitializeValue(StorageSpace);

		Stack.MostRecentPropertyAddress = NULL;
		Stack.StepCompiledIn<FProperty>(StorageSpace);
		void* ItemPtr = (Stack.MostRecentPropertyAddress != NULL && Stack.MostRecentProperty->GetClass() == InnerProp->GetClass()) ? Stack.MostRecentPropertyAddress : StorageSpace;

		P_FINISH;
		P_NATIVE_BEGIN;
		UBlueprintProLibrary::GenericArray_Max(TargetArrayAddr, ArrayProperty, OwnerObject, CompareByFunction, OutMaxIndex, ItemPtr, true);
		P_NATIVE_END;
		InnerProp->DestroyValue(StorageSpace);
	}

	// array min
	DECLARE_FUNCTION(execArray_Min)
	{
		Stack.MostRecentProperty = nullptr;
		Stack.StepCompiledIn<FArrayProperty>(NULL);
		void* TargetArrayAddr = Stack.MostRecentPropertyAddress;
		FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Stack.MostRecentProperty);
		if (!ArrayProperty)
		{
			Stack.bArrayContextFailed = true;
			return;
		}

		P_GET_OBJECT(UObject, OwnerObject);
		P_GET_PROPERTY(FNameProperty, CompareByName);
		P_GET_PROPERTY_REF(FIntProperty, OutMinIndex);

		// find custom compare by function
		UFunction* CompareByFunction = OwnerObject->FindFunction(CompareByName);
		if (!CompareByFunction)
		{
			UE_LOG(LogBlueprintProLibrary, Warning, TEXT("Array_Min -> Please input your custom compare function name to MIN blueprint node."));
			return;
		}

		// Since NewItem isn't really an int, step the stack manually
		const FProperty* InnerProp = ArrayProperty->Inner;
		const int32 PropertySize = InnerProp->ElementSize * InnerProp->ArrayDim;
		void* StorageSpace = FMemory_Alloca(PropertySize);
		InnerProp->InitializeValue(StorageSpace);

		Stack.MostRecentPropertyAddress = NULL;
		Stack.StepCompiledIn<FProperty>(StorageSpace);
		void* ItemPtr = (Stack.MostRecentPropertyAddress != NULL && Stack.MostRecentProperty->GetClass() == InnerProp->GetClass()) ? Stack.MostRecentPropertyAddress : StorageSpace;

		P_FINISH;
		P_NATIVE_BEGIN;
		GenericArray_Max(TargetArrayAddr, ArrayProperty, OwnerObject, CompareByFunction, OutMinIndex, ItemPtr, false);
		P_NATIVE_END;
		InnerProp->DestroyValue(StorageSpace);
	}


	// Is it a valid array index ?
	static void GenericArray_IsValidIndex(void* ArrayAddr, FArrayProperty* ArrayProperty, int32 Index, EIndexValidResult& Result);

	// slice elements of array from FromIndex to ToIndex
	static void GenericArray_Slice(void* TargetArray, const FArrayProperty* ArrayProp, int32 Index, int32 Length, void* OutArray);

	// filter wildcard array
	static void GenericArray_Filter(UObject* Object, UFunction* FilterFunction, const FArrayProperty* ArrayProp, void* SrcArrayAddr, void* FilterArrayAddr);

	// Max/Min of Array
	static void GenericArray_Max(void* TargetArrayAddr, FArrayProperty* ArrayProp, UObject* OwnerObject, UFunction* CompareByFunction, int32& MaxIndex, void* MaxElemAddr, bool bGetMax = true);


	static void GenericArray_QuickSort(UObject* OwnerObject, UFunction* ComparedBy, void* TargetArray, FArrayProperty* ArrayProp);

	DECLARE_FUNCTION(execArray_QuickSort)
	{
		Stack.MostRecentProperty = nullptr;
		Stack.StepCompiledIn<FArrayProperty>(NULL);
		void* ArrayAAddr = Stack.MostRecentPropertyAddress;
		FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Stack.MostRecentProperty);
		if (!ArrayProperty)
		{
			Stack.bArrayContextFailed = true;
			return;
		}

		P_GET_OBJECT(UObject, OwnerObject);
		P_GET_PROPERTY(FNameProperty, FunctionName);
		if (!OwnerObject)
		{
			return;
		}
		UFunction* const ElemCompareFunction = OwnerObject->FindFunction(FunctionName);
		if ((!ElemCompareFunction || (ElemCompareFunction->NumParms != 3)))
		{
			return;
		}

		P_FINISH;

		P_NATIVE_BEGIN;
		GenericArray_QuickSort(OwnerObject, ElemCompareFunction, ArrayAAddr, ArrayProperty);
		P_NATIVE_END;
	}


	// generic quick sort array by property
	static void GenericArraySortProperty(void* TargetArray, FArrayProperty* ArrayProp, FName PropertyName, bool bAscending);

	// sort array by property
	DECLARE_FUNCTION(execArray_SortV2)
	{
		Stack.MostRecentProperty = nullptr;
		Stack.StepCompiledIn<FArrayProperty>(NULL);
		void* ArrayAddr = Stack.MostRecentPropertyAddress;
		FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Stack.MostRecentProperty);
		if (!ArrayProperty)
		{
			Stack.bArrayContextFailed = true;
			return;
		}
		P_GET_PROPERTY(FNameProperty, PropertyName);
		P_GET_UBOOL(bAscending);
		P_FINISH;

		P_NATIVE_BEGIN;
		UBlueprintProLibrary::GenericArraySortProperty(ArrayAddr, ArrayProperty, PropertyName, bAscending);
		P_NATIVE_END;
	}


	static void QuickSort_Recursive(UObject* OwnerObject, UFunction* ComparedBy, FProperty* InnerProp, FScriptArrayHelper& ArrayHelper, int32 Low, int32 High);

	static int32 QuickSort_Partition(UObject* OwnerObject, UFunction* ComparedBy, FProperty* InnerProp, FScriptArrayHelper& ArrayHelper, int32 Low, int32 High);

	// Check if a function can be used for comparing the size of array element
	static bool IsValidComparator(UFunction* ComparedBy, FArrayProperty* ArrayProp);

	// Low  --> Starting index,  High  --> Ending index
	static void QuickSortRecursiveByProperty(FScriptArrayHelper& ArrayHelper, FProperty* InnerProp, FProperty* SortProp, int32 Low, int32 High, bool bAscending);

	// swapping items in place and partitioning the section of an array
	static int32 QuickSortPartitionByProperty(FScriptArrayHelper& ArrayHelper, FProperty* InnerProp, FProperty* SortProp, int32 Low, int32 High, bool bAscending);

	// generic compare two element of array by property
	static bool CompareArrayItemsByProperty(FScriptArrayHelper& ArrayHelper, FProperty* InnerProp, FProperty* SortProp, int32 j, int32 High, bool bAscending);

	// internal function of GenericArray_Sort
	static bool GenericArray_SortCompare(const FProperty* LeftProperty, void* LeftValuePtr, const FProperty* RightProperty, void* RightValuePtr);


public: //DataTable
	/** Get a Row from a DataTable given a RowName */
	//UFUNCTION(BlueprintCallable, CustomThunk, meta = (CustomStructureParam = "RowData", BlueprintInternalUseOnly = "true"), Category = "DataTable")
	static void SetDataTableRowFromName(UDataTable* Table, FName RowName, const int32& RowData);

private:

	static bool Generic_SetDataTableRowFromName(const UDataTable* Table, FName RowName, void* OutRowPtr);

	/** Based on UDataTableFunctionLibrary::GetDataTableRow */
	DECLARE_FUNCTION(execSetDataTableRowFromName)
	{
		P_GET_OBJECT(UDataTable, DataTable);
		P_GET_PROPERTY(FNameProperty, RowName);

		//Get input wildcard single variable
		Stack.Step(Stack.Object, NULL);
		FStructProperty* StructProperty = CastField<FStructProperty>(Stack.MostRecentProperty);
		void* StructPtr = Stack.MostRecentPropertyAddress;

		//P_GET_UBOOL_REF(bSuccessful);
		P_FINISH;
		P_NATIVE_BEGIN;
		//Generic_SetDataTableRowFromName(DataTable, RowName, StructProperty, StructPtr, bSuccessful);
		P_NATIVE_END;
	}



};
