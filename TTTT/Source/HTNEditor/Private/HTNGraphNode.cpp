// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "HTNGraphNode.h"

#include "EdGraphSchema_HTN.h"
#include "HTNDecorator.h"
#include "HTNGraph.h"
#include "HTNGraphPinCategories.h"
#include "HTNGraphNode_Decorator.h"
#include "HTNGraphNode_Service.h"
#include "HTNNode.h"
#include "HTNService.h"
#include "HTNStandaloneNode.h"
#include "HTNTypes.h"
#include "Utility/HTNHelpers.h"

#include "GraphDiffControl.h"
#include "ToolMenuDelegates.h"
#include "ToolMenu.h"
#include "SGraphEditorActionMenuAI.h"

#define LOCTEXT_NAMESPACE "HTNGraph"

UHTNGraphNode::UHTNGraphNode(const FObjectInitializer& Initializer) : Super(Initializer),
	bDebuggerMarkCurrentlyActive(false),
	bDebuggerMarkCurrentlyExecuting(false),
	bHasBreakpoint(false),
	bIsBreakpointEnabled(false),
	bHighlightDueToSearch(false)
{}

void UHTNGraphNode::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, FHTNGraphPinCategories::MultipleNodesAllowed, TEXT("In"));
	CreatePin(EGPD_Output, FHTNGraphPinCategories::MultipleNodesAllowed, TEXT("Out"));
}

FText UHTNGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (UHTNNode* const Node = Cast<UHTNNode>(NodeInstance))
	{
		return FText::FromString(Node->GetNodeName());
	}
	
	if (!ClassData.GetClassName().IsEmpty())
	{
		FString StoredClassName = ClassData.GetClassName();
		StoredClassName.RemoveFromEnd(TEXT("_C"));
		return FText::Format(NSLOCTEXT("AIGraph", "NodeClassError", "Class {0} not found, make sure it's saved!"), FText::FromString(StoredClassName));
	}

	return Super::GetNodeTitle(TitleType);
}

bool UHTNGraphNode::CanCreateUnderSpecifiedSchema(const UEdGraphSchema* DesiredSchema) const
{
	return Cast<UEdGraphSchema_HTN>(DesiredSchema) != nullptr;
}

void UHTNGraphNode::FindDiffs(UEdGraphNode* OtherNode, FDiffResults& Results)
{
	Super::FindDiffs(OtherNode, Results);

	// This function is mostly a copypaste of UBehaviorTreeGraphNode::FindDiffs
	// Differences in the nodes themselves were already collected in Super::FindDiffs.
	// This collects the differences in decorators and services.

	UHTNGraphNode* OtherHTNGraphNode = Cast<UHTNGraphNode>(OtherNode);
	if (!OtherHTNGraphNode)
	{
		return;
	}

	const auto DiffSubNodes = [&Results](const FText& NodeTypeDisplayName, const auto& LhsSubNodes, const auto& RhsSubNodes)
	{
		TArray<FGraphDiffControl::FNodeMatch> NodeMatches;
		TSet<const UEdGraphNode*> MatchedRhsNodes;

		FGraphDiffControl::FNodeDiffContext AdditiveDiffContext;
		AdditiveDiffContext.NodeTypeDisplayName = NodeTypeDisplayName;
		AdditiveDiffContext.bIsRootNode = false;

		// March through the all the nodes in the rhs and look for matches 
		for (UEdGraphNode* RhsSubNode : RhsSubNodes)
		{
			FGraphDiffControl::FNodeMatch NodeMatch;
			NodeMatch.NewNode = RhsSubNode;

			const auto DoPass = [&](bool bExactOnly)
			{
				for (UEdGraphNode* LhsSubNode : LhsSubNodes)
				{
					if (FGraphDiffControl::IsNodeMatch(LhsSubNode, RhsSubNode, bExactOnly, &NodeMatches))
					{
						NodeMatch.OldNode = LhsSubNode;
						break;
					}
				}
			};

			// Do two passes, exact and soft
			DoPass(true);
			if (!NodeMatch.IsValid())
			{
				DoPass(false);
			}

			// If we found a corresponding node in the lhs graph, track it (so we can prevent future matches with the same nodes)
			if (NodeMatch.IsValid())
			{
				NodeMatches.Add(NodeMatch);
				MatchedRhsNodes.Add(NodeMatch.OldNode);
			}

			NodeMatch.Diff(AdditiveDiffContext, Results);
		}

		FGraphDiffControl::FNodeDiffContext SubtractiveDiffContext = AdditiveDiffContext;
		SubtractiveDiffContext.DiffMode = FGraphDiffControl::EDiffMode::Subtractive;
		SubtractiveDiffContext.DiffFlags = FGraphDiffControl::EDiffFlags::NodeExistance;

		// Go through the lhs nodes to catch ones that may have been missing from the rhs graph
		for (UEdGraphNode* LhsSubNode : LhsSubNodes)
		{
			// if this node has already been matched, move on
			if (!LhsSubNode || MatchedRhsNodes.Find(LhsSubNode))
			{
				continue;
			}

			// There can't be a matching node in RhsGraph because it would have been found above
			FGraphDiffControl::FNodeMatch NodeMatch;
			NodeMatch.NewNode = LhsSubNode;

			NodeMatch.Diff(SubtractiveDiffContext, Results);
		}
	};

	DiffSubNodes(LOCTEXT("DecoratorDiffDisplayName", "Decorator"), Decorators, OtherHTNGraphNode->Decorators);
	DiffSubNodes(LOCTEXT("ServiceDiffDisplayName", "Service"), Services, OtherHTNGraphNode->Services);
}

void UHTNGraphNode::DiffProperties(UStruct* StructA, UStruct* StructB, uint8* DataA, uint8* DataB, FDiffResults& Results, FDiffSingleResult& Diff) const
{
	const auto GetHTN = [](const UStruct* Struct, const uint8* Data) -> const UHTN*
	{
		if (Data && Struct && Struct->IsA<UClass>())
		{
			return reinterpret_cast<const UObject*>(Data)->GetTypedOuter<UHTN>();
		}

		return nullptr;
	};
	DiffPropertiesInternal(Results, Diff, TEXT(""), 
		GetHTN(StructA, DataA), StructA, DataA,
		GetHTN(StructB, DataB), StructB, DataB);
}

void UHTNGraphNode::GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	if (Cast<UHTNStandaloneNode>(NodeInstance))
	{
		AddContextMenuActionsForAddingDecorators(Menu, TEXT("HTNGraphNode"), Context);
		AddContextMenuActionsForAddingServices(Menu, TEXT("HTNGraphNode"), Context);
	}
}

FText UHTNGraphNode::GetDescription() const
{
	if (const UHTNNode* const HTNNode = Cast<const UHTNNode>(NodeInstance))
	{
		return FText::FromString(HTNNode->GetStaticDescription());
	}

	return Super::GetDescription();
}

void UHTNGraphNode::InitializeInstance()
{
	if (UHTNNode* const HTNNode = Cast<UHTNNode>(NodeInstance))
	{
		if (UHTN* const HTNAsset = HTNNode->GetTypedOuter<UHTN>())
		{
			HTNNode->InitializeFromAsset(*HTNAsset);
			//HTNNode->OnNodeCreated();
		}
	}
}

void UHTNGraphNode::OnSubNodeAdded(UAIGraphNode* SubNode)
{
	if (UHTNGraphNode_Decorator* const DecoratorNode = Cast<UHTNGraphNode_Decorator>(SubNode))
	{
		Decorators.Add(DecoratorNode);
	}
	else if (UHTNGraphNode_Service* const ServiceNode = Cast<UHTNGraphNode_Service>(SubNode))
	{
		Services.Add(ServiceNode);
	}
}

void UHTNGraphNode::OnSubNodeRemoved(UAIGraphNode* SubNode)
{
	if (UHTNGraphNode_Decorator* const DecoratorNode = Cast<UHTNGraphNode_Decorator>(SubNode))
	{
		Decorators.Remove(DecoratorNode);
	}
	else if (UHTNGraphNode_Service* const ServiceNode = Cast<UHTNGraphNode_Service>(SubNode))
	{
		Services.Remove(ServiceNode);
	}
}

void UHTNGraphNode::RemoveAllSubNodes()
{
	Super::RemoveAllSubNodes();
	Decorators.Reset();
	Services.Reset();
}

int32 UHTNGraphNode::FindSubNodeDropIndex(UAIGraphNode* SubNode) const
{
	const int32 SubIndex = SubNodes.IndexOfByKey(SubNode) + 1;

	UHTNGraphNode_Decorator* const DecoratorNode = Cast<UHTNGraphNode_Decorator>(SubNode);
	const int32 DecoratorIndex = (DecoratorNode ? Decorators.IndexOfByKey(DecoratorNode) : -1) + 1;

	UHTNGraphNode_Service* const ServiceNode = Cast<UHTNGraphNode_Service>(SubNode);
	const int32 ServiceIndex = (ServiceNode ? Services.IndexOfByKey(ServiceNode) : -1) + 1;

	const int32 CombinedIdx = (SubIndex & 0xff) | ((DecoratorIndex & 0xff) << 8) | ((ServiceIndex & 0xff) << 16);
	return CombinedIdx;
}

void UHTNGraphNode::InsertSubNodeAt(UAIGraphNode* SubNode, int32 DropIndex)
{
	const int32 SubNodeIndex = (DropIndex & 0xff) - 1;
	const int32 DecoratorIndex = ((DropIndex >> 8) & 0xff) - 1;
	const int32 ServiceIndex = ((DropIndex >> 16) & 0xff) - 1;

	if (SubNodeIndex >= 0)
	{
		SubNodes.Insert(SubNode, SubNodeIndex);
	}
	else
	{
		SubNodes.Add(SubNode);
	}

	if (UHTNGraphNode_Decorator* const DecoratorNode = Cast<UHTNGraphNode_Decorator>(SubNode))
	{
		if (DecoratorIndex >= 0)
		{
			Decorators.Insert(DecoratorNode, DecoratorIndex);
		}
		else
		{
			Decorators.Add(DecoratorNode);
		}
	}
	else if (UHTNGraphNode_Service* const ServiceNode = Cast<UHTNGraphNode_Service>(SubNode))
	{
		if (ServiceIndex >= 0)
		{
			Services.Insert(ServiceNode, ServiceIndex);
		}
		else
		{
			Services.Add(ServiceNode);
		}
	}
}

#if WITH_EDITOR
void UHTNGraphNode::PostEditUndo()
{
	Super::PostEditUndo();

	if (UHTNGraphNode* const MyParentNode = Cast<UHTNGraphNode>(ParentNode))
	{
		if (UHTNDecorator* const Decorator = Cast<UHTNDecorator>(NodeInstance))
		{
			UHTNGraphNode_Decorator* const DecoratorGraphNode = Cast<UHTNGraphNode_Decorator>(this);
			check(DecoratorGraphNode);
			MyParentNode->Decorators.AddUnique(DecoratorGraphNode);
		}
		else if (UHTNService* const Service = Cast<UHTNService>(NodeInstance))
		{
			UHTNGraphNode_Service* const ServiceGraphNode = Cast<UHTNGraphNode_Service>(this);
			check(ServiceGraphNode);
			MyParentNode->Services.AddUnique(ServiceGraphNode);
		}
	}
}
#endif

UHTNGraph* UHTNGraphNode::GetHTNGraph() { return CastChecked<UHTNGraph>(GetGraph()); }

FName UHTNGraphNode::GetIconName() const
{
	UHTNNode* const Node = Cast<UHTNNode>(NodeInstance);
	return Node ? Node->GetNodeIconName() : FName("BTEditor.Graph.BTNode.Icon");
}

void UHTNGraphNode::ClearBreakpoints()
{
	bHasBreakpoint = false;
	bIsBreakpointEnabled = false;
}

void UHTNGraphNode::ClearDebugFlags()
{
	DebuggerPlanEntries.Reset();
	bDebuggerMarkCurrentlyActive = false;
	bDebuggerMarkCurrentlyExecuting = false;
}

bool UHTNGraphNode::IsInFutureOfDebuggedPlan() const
{
	return DebuggerPlanEntries.ContainsByPredicate([&](const FDebuggerPlanEntry& ToEntry) -> bool
	{
		return ToEntry.bIsInFutureOfPlan;
	});
}

void UHTNGraphNode::AddContextMenuActionsForAddingDecorators(UToolMenu* Menu, const FName SectionName, UGraphNodeContextMenuContext* Context) const
{
	FToolMenuSection& Section = Menu->FindOrAddSection(SectionName);
	Section.AddSubMenu(
		TEXT("AddDecorator"),
		LOCTEXT("AddDecorator", "Add Decorator..."),
		LOCTEXT("AddDecoratorTooltip", "Adds new decorator as a subnode"),
		FNewToolMenuDelegate::CreateUObject(this, &UHTNGraphNode::CreateAddDecoratorSubMenu, 
			const_cast<UEdGraph*>(Context->Graph.Get())
		)
	);
}

void UHTNGraphNode::CreateAddDecoratorSubMenu(UToolMenu* Menu, UEdGraph* Graph) const
{
	const TSharedRef<SGraphEditorActionMenuAI> Widget =
		SNew(SGraphEditorActionMenuAI)
		.GraphObj(Graph)
		.GraphNode(const_cast<UHTNGraphNode*>(this))
		.SubNodeFlags(StaticCast<int32>(EHTNSubNodeType::Decorator))
		.AutoExpandActionMenu(true);

	FToolMenuSection& Section = Menu->FindOrAddSection(TEXT("Section"));
	Section.AddEntry(FToolMenuEntry::InitWidget(TEXT("DecoratorWidget"), Widget, FText(), true));
}

void UHTNGraphNode::AddContextMenuActionsForAddingServices(UToolMenu* Menu, const FName SectionName, UGraphNodeContextMenuContext* Context) const
{
	FToolMenuSection& Section = Menu->FindOrAddSection(SectionName);
	Section.AddSubMenu(
		TEXT("AddService"),
		LOCTEXT("AddService", "Add Service..."),
		LOCTEXT("AddServiceTooltip", "Adds new service as a subnode"),
		FNewToolMenuDelegate::CreateUObject(this, &UHTNGraphNode::CreateAddServiceSubMenu, 
			const_cast<UEdGraph*>(Context->Graph.Get())
		)
	);
}

void UHTNGraphNode::CreateAddServiceSubMenu(UToolMenu* Menu, UEdGraph* Graph) const
{
	const TSharedRef<SGraphEditorActionMenuAI> Widget =
		SNew(SGraphEditorActionMenuAI)
		.GraphObj(Graph)
		.GraphNode(const_cast<UHTNGraphNode*>(this))
		.SubNodeFlags(StaticCast<int32>(EHTNSubNodeType::Service))
		.AutoExpandActionMenu(true);

	FToolMenuSection& Section = Menu->FindOrAddSection(TEXT("Section"));
	Section.AddEntry(FToolMenuEntry::InitWidget(TEXT("ServiceWidget"), Widget, FText(), true));
}

void UHTNGraphNode::DiffPropertiesInternal(FDiffResults& OutResults, FDiffSingleResult& OutDiff, const FString& DiffPrefix, 
	const UHTN* HTNA, const UStruct* StructA, const uint8* DataA,
	const UHTN* HTNB, const UStruct* StructB, const uint8* DataB) const
{
	const auto GetReferencedObject = [](const FProperty* Property, const uint8* PropertyData) -> const UObject*
	{
		if (const FObjectPropertyBase* const ObjectProp = CastField<FObjectPropertyBase>(Property))
		{
			return ObjectProp->GetObjectPropertyValue_InContainer(PropertyData);
		}

		return nullptr;
	};

	const auto GetPropertyStruct = [](const FProperty* Property) -> const UStruct*
	{
		if (const FStructProperty* const StructProp = CastField<FStructProperty>(Property))
		{
			if (StructProp->Struct)
			{
				return StructProp->Struct;
			}
		}

		return nullptr;
	};

	// Run through all the properties in the first struct
	for (const FProperty* const Prop : TFieldRange<FProperty>(StructA))
	{
		const FProperty* PropB = StructB->FindPropertyByName(Prop->GetFName());
		if (!PropB || Prop->GetClass() != PropB->GetClass())
		{
			// Skip if properties don't match.
			continue;
		}

		// Skip properties we cant see
		if (!Prop->HasAnyPropertyFlags(CPF_Edit | CPF_BlueprintVisible) ||
			Prop->HasAnyPropertyFlags(CPF_Transient) ||
			Prop->HasAnyPropertyFlags(CPF_DisableEditOnInstance) ||
			Prop->IsA(FDelegateProperty::StaticClass()) ||
			Prop->IsA(FMulticastDelegateProperty::StaticClass()))
		{
			continue;
		}

		// If both properties refer to the HTN asset they're in, count them as the same.
		// This is because the two HTNs are only technically different but represent different revisions of the same HTN.
		const UObject* const ReferencedObjectA = GetReferencedObject(Prop, DataA);
		const UObject* const ReferencedObjectB = GetReferencedObject(PropB, DataB);
		if (ReferencedObjectA == HTNA && ReferencedObjectB == HTNB)
		{
			continue;
		}

		const uint8* const PropertyDataA = Prop->ContainerPtrToValuePtr<uint8>(DataA);
		const uint8* const PropertyDataB = PropB->ContainerPtrToValuePtr<uint8>(DataB);
		const auto GetPropertyPrefix = [&]() { return FString::Printf(TEXT("%s%s."), *DiffPrefix, *Prop->GetName()); };

		// If both values are subobjects of their respective HTNs, diff them recursively.
		if (ReferencedObjectA && ReferencedObjectA->IsInOuter(HTNA) && ReferencedObjectB && ReferencedObjectB->IsInOuter(HTNB))
		{
			DiffPropertiesInternal(OutResults, OutDiff, GetPropertyPrefix(),
				HTNA, ReferencedObjectA->GetClass(), reinterpret_cast<const uint8*>(ReferencedObjectA),
				HTNB, ReferencedObjectB->GetClass(), reinterpret_cast<const uint8*>(ReferencedObjectB));
			continue;
		}

		// If both values are structs, diff them recursively.
		if (const UStruct* PropertyStructA = GetPropertyStruct(Prop))
		{
			if (!FHTNHelpers::GetStructsToPrintDirectly().Contains(PropertyStructA))
			{
				if (const UStruct* const PropertyStructB = GetPropertyStruct(PropB))
				{
					DiffPropertiesInternal(OutResults, OutDiff, GetPropertyPrefix(),
						HTNA, PropertyStructA, PropertyDataA,
						HTNB, PropertyStructB, PropertyDataB);
					continue;
				}
			}
		}

		// Compare the two values by string.
		const FString ValueStringA = GetPropertyNameAndValueForDiff(Prop, PropertyDataA);
		const FString ValueStringB = GetPropertyNameAndValueForDiff(PropB, PropertyDataB);
		if (ValueStringA != ValueStringB)
		{
			// Only bother setting up the display data if we're storing the result
			if (OutResults.CanStoreResults())
			{
				OutDiff.DisplayString = FText::Format(LOCTEXT("DIF_NodePropertyFmt", "Property Changed: {0}{1} "), 
					FText::FromString(DiffPrefix),
					FText::FromString(Prop->GetName()));
			}
			OutResults.Add(OutDiff);
		}
	}
}

#undef LOCTEXT_NAMESPACE
