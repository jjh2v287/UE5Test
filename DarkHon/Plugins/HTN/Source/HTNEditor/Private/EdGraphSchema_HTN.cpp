// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "EdGraphSchema_HTN.h"

#include "Algo/AnyOf.h"
#include "Algo/Transform.h"
#include "EdGraph/EdGraph.h"
#include "Engine/Blueprint.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "Modules/ModuleManager.h"

#include "HTNEditorModule.h"
#include "HTNEditor.h"
#include "HTNConnectionDrawingPolicy.h"
#include "HTNNode.h"
#include "HTNGraph.h"
#include "HTNTask.h"
#include "HTNDecorator.h"
#include "HTNGraphNode_Decorator.h"
#include "HTNGraphNode_Root.h"
#include "HTNGraphNode_Service.h"
#include "HTNGraphNode_TwoBranches.h"
#include "HTNService.h"
#include "ScopedTransaction.h"
#include "Nodes/HTNNode_SubNetwork.h"
#include "Nodes/HTNNode_TwoBranches.h"
#include "Tasks/HTNTask_EQSQuery.h"

#define LOCTEXT_NAMESPACE "HTNEditorGraphSchema"

int32 UEdGraphSchema_HTN::CurrentCacheRefreshID = 0;

namespace
{
	template<typename T>
	T* MakeNode(UEdGraph& Graph)
	{
		FGraphNodeCreator<T> Creator(Graph);
		T* const Node = Creator.CreateNode();
		Creator.Finalize();

		return Node;
	}
}

void UEdGraphSchema_HTN::CreateDefaultNodesForGraph(UEdGraph& Graph) const
{
	UHTNGraphNode_Root* const Root = MakeNode<UHTNGraphNode_Root>(Graph);
	SetNodeMetaData(Root, FNodeMetadata::DefaultGraphNode);
}

const FPinConnectionResponse UEdGraphSchema_HTN::CanCreateConnection(const UEdGraphPin* PinA, const UEdGraphPin* PinB) const
{
	if (PinA->GetOwningNode() == PinB->GetOwningNode())
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinErrorSameNode", "Both are on the same node"));
	}

	if (PinA->Direction == EGPD_Input && PinB->Direction == EGPD_Input)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinErrorInputToInput", "Can't connect input node to input node"));
	}
	else if (PinB->Direction == EGPD_Output && PinA->Direction == EGPD_Output)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinErrorOutputToOutput", "Can't connect output node to output node"));
	}

	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, LOCTEXT("PinConnect", "Connect nodes"));
}

void UEdGraphSchema_HTN::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	const auto MakeCreateNodeAction = [OwnerOfTemporaries = ContextMenuBuilder.OwnerOfTemporaries]
	(
		FGraphActionListBuilderBase& Builder, 
		const FGraphNodeClassData& RuntimeNodeClassData, 
		const TSubclassOf<UHTNGraphNode> GraphNodeClass = UHTNGraphNode::StaticClass()
	)
	{
		const FText NodeTypeName = FText::FromString(FName::NameToDisplayString(RuntimeNodeClassData.ToString(), false));
		const TSharedPtr<FAISchemaAction_NewNode> AddOpAction = AddNewNodeAction(Builder, 
			RuntimeNodeClassData.GetCategory(), NodeTypeName, FText::GetEmpty());

		UHTNGraphNode* const TemplateNode = NewObject<UHTNGraphNode>(OwnerOfTemporaries, GraphNodeClass);
		TemplateNode->ClassData = RuntimeNodeClassData;
		AddOpAction->NodeTemplate = TemplateNode;

		return AddOpAction;
	};

	FCategorizedGraphActionListBuilder TasksBuilder(TEXT("Tasks"));
	
	const TSharedRef<FGraphNodeClassHelper> ClassCache = IHTNEditorModule::Get().GetClassCache().ToSharedRef();

	TArray<FGraphNodeClassData> NodeClasses;
	ClassCache->GatherClasses(UHTNTask::StaticClass(), NodeClasses);
	TSet<FString> TaskNames;
	Algo::Transform(NodeClasses, TaskNames, &FGraphNodeClassData::GetClassName);

	NodeClasses.Reset();
	ClassCache->GatherClasses(UHTNNode_TwoBranches::StaticClass(), NodeClasses);
	TSet<FString> TwoBranchesNames;
	Algo::Transform(NodeClasses, TwoBranchesNames, &FGraphNodeClassData::GetClassName);

	NodeClasses.Reset();
	ClassCache->GatherClasses(UHTNStandaloneNode::StaticClass(), NodeClasses);
	for (const FGraphNodeClassData& NodeClassData : NodeClasses)
	{
		if (TaskNames.Contains(NodeClassData.GetClassName()))
		{
			MakeCreateNodeAction(TasksBuilder, NodeClassData, UHTNGraphNode::StaticClass());
		}
		else if (TwoBranchesNames.Contains(NodeClassData.GetClassName()))
		{
			MakeCreateNodeAction(ContextMenuBuilder, NodeClassData, UHTNGraphNode_TwoBranches::StaticClass());
		}
		else
		{
			MakeCreateNodeAction(ContextMenuBuilder, NodeClassData, UHTNGraphNode::StaticClass());
		}
	}
	
	ContextMenuBuilder.Append(TasksBuilder);
}

const FPinConnectionResponse UEdGraphSchema_HTN::CanMergeNodes(const UEdGraphNode* A, const UEdGraphNode* B) const
{
	if (A == B)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Both are the same node"));
	}

	if (FHTNEditor::IsPIESimulating())
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Can't edit during a Play in Editor session."));
	}
	
	static const auto CanMergeSubNodeIntoNode = [](TSubclassOf<UHTNGraphNode> SubNodeType, const UEdGraphNode* Target) -> bool
	{
		if (const UHTNGraphNode* const GraphNode = Cast<UHTNGraphNode>(Target))
		{
			if (GraphNode->IsA(SubNodeType))
			{
				return true;
			}

			if (GraphNode->IsA(UHTNGraphNode_Root::StaticClass()))
			{
				return true;
			}

			return GraphNode->NodeInstance && GraphNode->NodeInstance->IsA(UHTNStandaloneNode::StaticClass());
		}

		return false;
	};
	
	if (
		(Cast<UHTNGraphNode_Decorator>(A) && CanMergeSubNodeIntoNode(UHTNGraphNode_Decorator::StaticClass(), B)) ||
		(Cast<UHTNGraphNode_Service>(A) && CanMergeSubNodeIntoNode(UHTNGraphNode_Service::StaticClass(), B))
	)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, TEXT(""));
	}

	return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT(""));
}

FConnectionDrawingPolicy* UEdGraphSchema_HTN::CreateConnectionDrawingPolicy(
	int32 InBackLayerID, int32 InFrontLayerID,
	float InZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements,
	UEdGraph* InGraphObj
) const
{
	return new FHTNConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements, InGraphObj);
}

void UEdGraphSchema_HTN::GetAssetsGraphHoverMessage(const TArray<FAssetData>& Assets, const UEdGraph* HoverGraph, FString& OutTooltipText, bool& OutOkIcon) const
{
	OutOkIcon = Algo::AnyOf(Assets, [](const FAssetData& AssetData) -> bool 
	{ 
		UObject* const Asset = AssetData.GetAsset();
		if (const UHTN* const Subnetwork = Cast<UHTN>(Asset))
		{
			return true;
		}
		else if (const UEnvQuery* const EnvQuery = Cast<UEnvQuery>(Asset))
		{
			return true;
		}
		else if (const TSubclassOf<UHTNStandaloneNode> NodeClass = Cast<UClass>(Asset))
		{
			return true;
		}
		else if (const UBlueprint* const Blueprint = Cast<UBlueprint>(Asset))
		{
			const TSubclassOf<UHTNStandaloneNode> BlueprintNodeClass(Blueprint->GeneratedClass);
			if (BlueprintNodeClass)
			{
				return true;
			}
		}

		return false;
	});
}

void UEdGraphSchema_HTN::DroppedAssetsOnGraph(const TArray<FAssetData>& Assets, const FVector2D& GraphPosition, UEdGraph* Graph) const
{
	UHTNGraph* const HTNGraph = CastChecked<UHTNGraph>(Graph);
	UHTN* const HTN = HTNGraph->GetTypedOuter<UHTN>();
	if (!ensureAlways(HTN))
	{
		return;
	}

	constexpr int32 Spacing = 100;
	FVector2D CurrentPosition = GraphPosition;
	const auto PlaceNode = [&](UHTNStandaloneNode* Node)
	{
		UHTNGraphNode* const GraphNode = HTNGraph->SpawnGraphNode(Node);
		if (ensureAlways(GraphNode))
		{
			GraphNode->NodePosX = CurrentPosition.X;
			GraphNode->NodePosY = CurrentPosition.Y;

			CurrentPosition.Y += Spacing;
		}
	};

	TArray<UHTN*> Subnetworks;
	TArray<UEnvQuery*> EnvQueries;
	TArray<TSubclassOf<UHTNStandaloneNode>> NodeClasses;
	for (const FAssetData& AssetData : Assets)
	{
		UObject* const Asset = AssetData.GetAsset();
		if (UHTN* const Subnetwork = Cast<UHTN>(Asset))
		{
			Subnetworks.Add(Subnetwork);
		}
		else if (UEnvQuery* const EnvQuery = Cast<UEnvQuery>(Asset))
		{
			EnvQueries.Add(EnvQuery);
		}
		else if (const TSubclassOf<UHTNStandaloneNode> NodeClass = Cast<UClass>(Asset))
		{
			NodeClasses.Add(NodeClass);
		}
		else if (UBlueprint* const Blueprint = Cast<UBlueprint>(Asset))
		{
			const TSubclassOf<UHTNStandaloneNode> BlueprintNodeClass(Blueprint->GeneratedClass);
			if (BlueprintNodeClass)
			{
				NodeClasses.Add(BlueprintNodeClass);
			}
		}
	}

	if (!Subnetworks.IsEmpty() || !EnvQueries.IsEmpty() || !NodeClasses.IsEmpty())
	{
		const FScopedTransaction Transaction(LOCTEXT("HTNEditorDropAssets", "HTN Editor: Drag and Drop Assets"));
		HTN->Modify();
		HTNGraph->Modify();

		for (UHTN* const Subnetwork : Subnetworks)
		{
			UHTNNode_SubNetwork* const SubnetworkNode = NewObject<UHTNNode_SubNetwork>(HTN, NAME_None, RF_Transactional);
			if (ensureAlways(SubnetworkNode))
			{
				SubnetworkNode->HTN = Subnetwork;
				PlaceNode(SubnetworkNode);
			}
		}

		for (UEnvQuery* const EnvQuery : EnvQueries)
		{
			UHTNTask_EQSQuery* const EQSNode = NewObject<UHTNTask_EQSQuery>(HTN, NAME_None, RF_Transactional);
			if (ensureAlways(EQSNode))
			{
				EQSNode->EQSRequest.QueryTemplate = EnvQuery;
				PlaceNode(EQSNode);
			}
		}

		for (const TSubclassOf<UHTNStandaloneNode>& NodeClass : NodeClasses)
		{
			UHTNStandaloneNode* const Node = NewObject<UHTNStandaloneNode>(HTN, NodeClass, NAME_None, RF_Transactional);	
			if (ensureAlways(Node))
			{
				PlaceNode(Node);
			}
		}
	}
}

bool UEdGraphSchema_HTN::IsCacheVisualizationOutOfDate(int32 InVisualizationCacheID) const { return InVisualizationCacheID != CurrentCacheRefreshID; }
int32 UEdGraphSchema_HTN::GetCurrentVisualizationCacheID() const { return CurrentCacheRefreshID; }
void UEdGraphSchema_HTN::ForceVisualizationCacheClear() const { ++CurrentCacheRefreshID; }

void UEdGraphSchema_HTN::GetSubNodeClasses(int32 SubNodeFlags, TArray<FGraphNodeClassData>& ClassData, UClass*& GraphNodeClass) const
{
	const TSharedRef<FGraphNodeClassHelper> ClassCache = IHTNEditorModule::Get().GetClassCache().ToSharedRef();
	switch (StaticCast<EHTNSubNodeType>(SubNodeFlags))
	{
	case EHTNSubNodeType::Decorator:
		ClassCache->GatherClasses(UHTNDecorator::StaticClass(), ClassData);
		GraphNodeClass = UHTNGraphNode_Decorator::StaticClass();
		break;
	case EHTNSubNodeType::Service:
		ClassCache->GatherClasses(UHTNService::StaticClass(), ClassData);
		GraphNodeClass = UHTNGraphNode_Service::StaticClass();
		break;
	default:
		checkNoEntry();
	}
}

#undef LOCTEXT_NAMESPACE
