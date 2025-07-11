// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "HTNGraph.h"
#include "Algo/Find.h"
#include "Algo/RemoveIf.h"
#include "Containers/Queue.h"
#include "EdGraph/EdGraphPin.h"
#include "HTNGraphNode_Root.h"
#include "Misc/ScopeExit.h"

#include "HTN.h"
#include "HTNDecorator.h"
#include "HTNGraphNode_Decorator.h"
#include "HTNGraphNode_Service.h"
#include "HTNGraphNode_TwoBranches.h"
#include "HTNStandaloneNode.h"
#include "HTNService.h"
#include "Nodes/HTNNode_TwoBranches.h"

namespace
{
	// For sorting nodes by y position top-to-bottom, otherwise by x position left-to-right.
	struct FCompareNodeYLocation
	{
		FORCEINLINE bool operator()(const UEdGraphPin& A, const UEdGraphPin& B) const
		{
			const UEdGraphNode* const NodeA = A.GetOwningNode();
			const UEdGraphNode* const NodeB = B.GetOwningNode();
			check(NodeA);
			check(NodeB);

			if (NodeA->NodePosY == NodeB->NodePosY)
			{
				return NodeA->NodePosX < NodeB->NodePosX;
			}

			return NodeA->NodePosY < NodeB->NodePosY;
		}
	};

}

void UHTNGraph::OnCreated()
{
	Super::OnCreated();
	SpawnMissingGraphNodes();
}

void UHTNGraph::UpdateAsset(int32 UpdateFlags)
{
	if (bLockUpdates)
	{
		return;
	}
	
	UHTN* const HTNAsset = GetTypedOuter<UHTN>();
	if (!ensure(HTNAsset))
	{
		return;
	}
	
	const auto EnsureOwnedByHTNAsset = [HTNAsset](UObject* Object)
	{
		if (Object && HTNAsset && Cast<UHTN>(Object->GetOuter()) != HTNAsset)
		{
			Object->Rename(nullptr, HTNAsset);
		}
	};

	UEdGraphPin::ResolveAllPinReferences();

	// Graph nodes cleanup
	for (int32 I = 0; I < Nodes.Num(); ++I)
	{
		if (UHTNGraphNode* const HTNGraphNode = Cast<UHTNGraphNode>(Nodes[I]))
		{
			// Make sure subnodes know their parent node.
			HTNGraphNode->ParentNode = nullptr;
			for (UHTNGraphNode_Decorator* const DecoratorGraphNode : HTNGraphNode->Decorators)
			{
				DecoratorGraphNode->ParentNode = HTNGraphNode;
			}
			for (UHTNGraphNode_Service* const ServiceGraphNode : HTNGraphNode->Services)
			{
				ServiceGraphNode->ParentNode = HTNGraphNode;
			}

			EnsureOwnedByHTNAsset(HTNGraphNode->NodeInstance);
			
			if (UHTNNode* const HTNNode = Cast<UHTNNode>(HTNGraphNode->NodeInstance))
			{
				HTNNode->InitializeFromAsset(*HTNAsset);
#if USE_HTN_DEBUGGER
				HTNNode->NodeIndexInGraph = I;
#endif
			}

			// Sort the pin connections to be in the same order as they appear in the editor.
			for (UEdGraphPin* const Pin : HTNGraphNode->Pins)
			{
				if (Pin)
				{
					Pin->LinkedTo.Sort(FCompareNodeYLocation());
				}
			}
		}
	}
	
	static const auto GetNextNodes = [](UAIGraphNode* GraphNode, TArray<TObjectPtr<UHTNStandaloneNode>>& OutNodes)
	{
		static const auto GetOwningHTNNode = [](UEdGraphPin& Pin) -> UHTNStandaloneNode*
		{
			UAIGraphNode* const GraphNode = CastChecked<UAIGraphNode>(Pin.GetOwningNode());
			UHTNStandaloneNode* const Node = GraphNode->NodeInstance ? 
				CastChecked<UHTNStandaloneNode>(GraphNode->NodeInstance) :
				nullptr;
			return Node;
		};

		static const auto RemoveNullPins = [](TArray<UEdGraphPin*>& Pins)
		{
			Pins.SetNum(Algo::RemoveIf(Pins, [](UEdGraphPin* const Pin) -> bool { return !Pin; }));
		};

		OutNodes.Reset();

		UHTNNode_TwoBranches* const TwoBranchesNode = Cast<UHTNNode_TwoBranches>(GraphNode->NodeInstance);
		bool bNeedsToSetNumPrimaryNodes = TwoBranchesNode != nullptr;
		if (bNeedsToSetNumPrimaryNodes)
		{
			TwoBranchesNode->NumPrimaryNodes = 0;
		}
			
		RemoveNullPins(GraphNode->Pins);
		for (UEdGraphPin* const Pin : GraphNode->Pins)
		{
			if (Pin->Direction == EGPD_Output)
			{
				RemoveNullPins(Pin->LinkedTo);
				if (Pin->LinkedTo.Num())
				{
					for (UEdGraphPin* const ConnectedPin : Pin->LinkedTo)
					{
						if (UHTNStandaloneNode* const ConnectedNode = GetOwningHTNNode(*ConnectedPin))
						{
							OutNodes.Add(ConnectedNode);
						}
					}
					
					if (bNeedsToSetNumPrimaryNodes)
					{
						TwoBranchesNode->NumPrimaryNodes = OutNodes.Num();
					}
				}
				bNeedsToSetNumPrimaryNodes = false;
			}
		}
	};

	const auto GetDecorators = [&](UEdGraphNode* GraphNode, TArray<TObjectPtr<UHTNDecorator>>& OutNodes)
	{
		OutNodes.Reset();
		
		if (UHTNGraphNode* const HTNGraphNode = Cast<UHTNGraphNode>(GraphNode))
		{
			for (UHTNGraphNode_Decorator* const DecoratorGraphNode : HTNGraphNode->Decorators)
			{
				UHTNDecorator* const Decorator = Cast<UHTNDecorator>(DecoratorGraphNode->NodeInstance);
				if (ensure(Decorator))
				{
					EnsureOwnedByHTNAsset(Decorator);
					OutNodes.Add(Decorator);
				}
			}
		}
	};

	const auto GetServices = [&](UEdGraphNode* GraphNode, TArray<TObjectPtr<UHTNService>>& OutNodes)
	{
		OutNodes.Reset();

		if (UHTNGraphNode* const HTNGraphNode = Cast<UHTNGraphNode>(GraphNode))
		{
			for (UHTNGraphNode_Service* const ServiceGraphNode : HTNGraphNode->Services)
			{
				UHTNService* const Service = Cast<UHTNService>(ServiceGraphNode->NodeInstance);
				if (ensure(Service))
				{
					EnsureOwnedByHTNAsset(Service);
					OutNodes.Add(Service);
				}
			}
		}
	};

	UHTNGraphNode_Root* const RootGraphNode = FindRootNode();
	if (ensure(RootGraphNode))
	{
		GetNextNodes(RootGraphNode, HTNAsset->StartNodes);
		GetDecorators(RootGraphNode, HTNAsset->RootDecorators);
		GetServices(RootGraphNode, HTNAsset->RootServices);
		
		for (UEdGraphNode* const BaseClassGraphNode : Nodes)
		{
			if (UHTNGraphNode* const GraphNode = Cast<UHTNGraphNode>(BaseClassGraphNode))
			{
				if (UHTNStandaloneNode* const Node = Cast<UHTNStandaloneNode>(GraphNode->NodeInstance))
				{
					GetNextNodes(GraphNode, Node->NextNodes);
					GetDecorators(GraphNode, Node->Decorators);
					GetServices(GraphNode, Node->Services);
				}
			}
		}

		RemoveOrphanedNodes();
	}

	OnBlackboardChanged();
}

void UHTNGraph::OnSubNodeDropped()
{
	Super::OnSubNodeDropped();
	UpdateAsset();
}

void UHTNGraph::OnSave()
{
	UpdateAsset();
}

void UHTNGraph::OnBlackboardChanged()
{
	UHTN* const HTNAsset = GetTypedOuter<UHTN>();
	if (!HTNAsset)
	{
		return;
	}

	const auto InitializeInstanceFromAsset = [HTNAsset](const UHTNGraphNode* HTNGraphNode)
	{
		if (HTNGraphNode)
		{
			if (UHTNNode* const HTNNode = Cast<UHTNNode>(HTNGraphNode->NodeInstance))
			{
				HTNNode->InitializeFromAsset(*HTNAsset);
			}
		}
	};
	
	for (const UEdGraphNode* const GraphNode : Nodes)
	{
		if (const UHTNGraphNode* const HTNGraphNode = Cast<UHTNGraphNode>(GraphNode))
		{
			InitializeInstanceFromAsset(HTNGraphNode);
			for (UHTNGraphNode_Decorator* const DecoratorGraphNode : HTNGraphNode->Decorators)
			{
				InitializeInstanceFromAsset(DecoratorGraphNode);
			}
			for (UHTNGraphNode_Service* const ServiceGraphNode : HTNGraphNode->Services)
			{
				InitializeInstanceFromAsset(ServiceGraphNode);
			}
		}
	}
}

UHTNGraphNode* UHTNGraph::SpawnGraphNode(UHTNStandaloneNode* Node, bool bSelectNewNode)
{
	if (!Node)
	{
		return nullptr;
	}

	const auto GetGraphNodeClassFor = [](UHTNNode* Node) -> TSubclassOf<UHTNGraphNode>
	{
		if (Node->IsA(UHTNNode_TwoBranches::StaticClass()))
		{
			return UHTNGraphNode_TwoBranches::StaticClass();
		}

		return UHTNGraphNode::StaticClass();
	};

	const TSubclassOf<UHTNGraphNode> GraphNodeClass = GetGraphNodeClassFor(Node);
	UHTNGraphNode* const HTNGraphNode = CastChecked<UHTNGraphNode>(CreateNode(GraphNodeClass, bSelectNewNode));
	HTNGraphNode->CreateNewGuid();
	HTNGraphNode->PostPlacedNewNode();
	if (HTNGraphNode->Pins.Num() == 0)
	{
		HTNGraphNode->AllocateDefaultPins();
	}

	HTNGraphNode->NodeInstance = Node;
	return HTNGraphNode;
}

UHTNGraphNode_Root* UHTNGraph::FindRootNode() const
{
	UHTNGraphNode_Root* RootNode;
	Nodes.FindItemByClass<UHTNGraphNode_Root>(&RootNode);
	return RootNode;
}

#if WITH_EDITOR
void UHTNGraph::PostEditUndo()
{
	Super::PostEditUndo();
	
	UpdateAsset();
	Modify();
}
#endif

void UHTNGraph::SpawnMissingGraphNodes()
{
	UHTN* const HTNAsset = GetTypedOuter<UHTN>();
	if (!HTNAsset)
	{
		return;
	}

	UHTNGraphNode_Root* const RootGraphNode = FindRootNode();
	if (!RootGraphNode)
	{
		return;
	}

	const bool bWasLocked = bLockUpdates;
	LockUpdates();
	ON_SCOPE_EXIT { if (!bWasLocked) UnlockUpdates(); };

	const auto FindGraphNode = [&](UHTNStandaloneNode* Node) -> UHTNGraphNode*
	{
		if (!Node)
		{
			return RootGraphNode;
		}

#if ENGINE_MAJOR_VERSION <= 4
		UEdGraphNode** const GraphNode = Nodes.FindByPredicate([Node](UEdGraphNode* GraphNode) -> bool
#else
		TObjectPtr<UEdGraphNode>* const GraphNode = Nodes.FindByPredicate([Node](TObjectPtr<UEdGraphNode> GraphNode) -> bool
#endif
		{
			if (UHTNGraphNode* const HTNGraphNode = Cast<UHTNGraphNode>(GraphNode))
			{
				return HTNGraphNode->NodeInstance == Node;
			}
			return false;
		});

		if (GraphNode)
		{
			return CastChecked<UHTNGraphNode>(*GraphNode);
		}

		return nullptr;
	};

	struct FTraversalContext
	{
		UHTNStandaloneNode* Node = nullptr;
		UHTNGraphNode* PrecedingGraphNode = nullptr;
		int32 Depth = 0;
	};

	TArray<int32> DepthToVerticalOffset;
	const auto GetVerticalOffsetAtDepth = [&DepthToVerticalOffset](int32 Depth) -> int32
	{
		return DepthToVerticalOffset.IsValidIndex(Depth) ? DepthToVerticalOffset[Depth] : 0;
	};
	const auto SetVerticalOffsetAtDepth = [&DepthToVerticalOffset](int32 Depth, int32 NewOffset)
	{
		if (ensure(Depth >= 0))
		{
			if (Depth >= DepthToVerticalOffset.Num())
			{
				DepthToVerticalOffset.SetNumZeroed(Depth + 1);
			}
			DepthToVerticalOffset[Depth] = NewOffset;
		}
	};

	static const auto FindPin = [](const UHTNGraphNode& GraphNode, EEdGraphPinDirection Dir, int32 DesiredIndex = 0) -> UEdGraphPin*
	{
		int32 Index = 0;
		for (UEdGraphPin* const Pin : GraphNode.Pins)
		{
			if (Pin->Direction == Dir)
			{
				if (Index == DesiredIndex)
				{
					return Pin;
				}
				++Index;
			}
		}

		return nullptr;
	};

	TSet<UHTNStandaloneNode*> VisitedNodes { nullptr };
	TQueue<FTraversalContext> Frontier;
	
	Frontier.Enqueue({});
	while (!Frontier.IsEmpty())
	{
		FTraversalContext CurrentContext;
		Frontier.Dequeue(CurrentContext);
		
		// Spawn graph node
		UHTNGraphNode* CurrentGraphNode = FindGraphNode(CurrentContext.Node);
		if (!CurrentGraphNode)
		{
			CurrentGraphNode = SpawnGraphNode(CurrentContext.Node);
			if (ensure(CurrentGraphNode))
			{
				const UHTNGraphNode* const PrecedingGraphNode = CurrentContext.PrecedingGraphNode;
				check(PrecedingGraphNode);
				CurrentGraphNode->NodePosX = PrecedingGraphNode->NodePosX + 400;
				CurrentGraphNode->NodePosY = FMath::Max(GetVerticalOffsetAtDepth(CurrentContext.Depth), PrecedingGraphNode->NodePosY);
			}
		}

		const bool bIsRootNode = CurrentGraphNode == RootGraphNode;
		check(bIsRootNode || CurrentContext.Node);
		
		// Keep track of height at current depth
		if (!bIsRootNode && ensure(CurrentGraphNode))
		{
			const int32 NumSubNodes = CurrentContext.Node->Decorators.Num() + CurrentContext.Node->Services.Num();
			SetVerticalOffsetAtDepth(CurrentContext.Depth, CurrentGraphNode->NodePosY + 100 + (1 + NumSubNodes) * 75);
		}
			
		// Add following nodes to the frontier
		const TArray<UHTNStandaloneNode*>& NextNodes = bIsRootNode ? HTNAsset->StartNodes : CurrentContext.Node->NextNodes;
		for (UHTNStandaloneNode* const NextNode : NextNodes)
		{
			if (!VisitedNodes.Contains(NextNode))
			{
				VisitedNodes.Add(NextNode);
				Frontier.Enqueue({ NextNode, CurrentGraphNode, CurrentContext.Depth + 1 });
			}
		}
	}

	const auto ConnectPinToNodes = [&](UEdGraphPin* const OutputPin, TArrayView<TObjectPtr<UHTNStandaloneNode>> NextNodes)
	{
		if (!OutputPin)
		{
			return;
		}

		for (UHTNStandaloneNode* const NextNode : NextNodes)
		{
			if (UHTNGraphNode* NextNodeGraphNode = FindGraphNode(NextNode))
			{
				UEdGraphPin* const InputPin = FindPin(*NextNodeGraphNode, EGPD_Input);
				OutputPin->MakeLinkTo(InputPin);
			}
		}
	};

	// Make connections between nodes
	for (UEdGraphNode* const GraphNode : Nodes)
	{
		if (UHTNGraphNode* const HTNGraphNode = Cast<UHTNGraphNode>(GraphNode))
		{			
			const bool bIsRootNode = HTNGraphNode == RootGraphNode;
			UHTNStandaloneNode* const CurrentNode = Cast<UHTNStandaloneNode>(HTNGraphNode->NodeInstance);
			if (bIsRootNode || CurrentNode)
			{
				// Add pin connections
				if (UHTNNode_TwoBranches* const CurrentNodeTwoBranches = Cast<UHTNNode_TwoBranches>(CurrentNode))
				{
					ConnectPinToNodes(FindPin(*HTNGraphNode, EGPD_Output, 0), CurrentNodeTwoBranches->GetPrimaryNextNodes());
					ConnectPinToNodes(FindPin(*HTNGraphNode, EGPD_Output, 1), CurrentNodeTwoBranches->GetSecondaryNextNodes());
				}
				else
				{
					ConnectPinToNodes(FindPin(*HTNGraphNode, EGPD_Output), bIsRootNode ? HTNAsset->StartNodes : CurrentNode->NextNodes);
				}
			}
		}
	}

	const auto SpawnMissingSubNodes = [&](UHTNGraphNode& GraphNode)
	{
		const bool bIsRootNode = &GraphNode == RootGraphNode;
		const UHTNStandaloneNode* const NodeInstance = Cast<UHTNStandaloneNode>(GraphNode.NodeInstance);
		if (!bIsRootNode && !NodeInstance)
		{
			return;
		}

		const TArray<UHTNDecorator*>& Decorators = bIsRootNode ? HTNAsset->RootDecorators : NodeInstance->Decorators;
		for (UHTNDecorator* const Decorator : Decorators)
		{
			const bool bIsMissing = !GraphNode.Decorators.ContainsByPredicate([Decorator](UHTNGraphNode_Decorator* DecoratorGraphNode) 
			{
				return DecoratorGraphNode && DecoratorGraphNode->NodeInstance == Decorator;
			});
			if (bIsMissing)
			{
				UHTNGraphNode_Decorator* const DecoratorNode = NewObject<UHTNGraphNode_Decorator>(this);
				DecoratorNode->NodeInstance = Decorator;
				GraphNode.AddSubNode(DecoratorNode, this);
			}
		}

		const TArray<UHTNService*>& Services = bIsRootNode ? HTNAsset->RootServices : NodeInstance->Services;
		for (UHTNService* const Service : Services)
		{
			const bool bIsMissing = !GraphNode.Services.ContainsByPredicate([Service](UHTNGraphNode_Service* ServiceGraphNode)
			{
				return ServiceGraphNode && ServiceGraphNode->NodeInstance == Service;
			});
			if (bIsMissing)
			{
				UHTNGraphNode_Service* const ServiceNode = NewObject<UHTNGraphNode_Service>(this);
				ServiceNode->NodeInstance = Service;
				GraphNode.AddSubNode(ServiceNode, this);
			}
		}
	};

	for (UEdGraphNode* const GraphNode : Nodes)
	{
		if (UHTNGraphNode* const HTNGraphNode = Cast<UHTNGraphNode>(GraphNode))
		{
			SpawnMissingSubNodes(*HTNGraphNode);
		}
	}
}