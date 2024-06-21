// Copyright 2023 Davide Socol. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MetasoundBuilderInterface.h"
#include "MetasoundDataReferenceCollection.h"
#include "MetasoundExecutableOperator.h"
#include "MetasoundFacade.h"
#include "MetasoundNode.h"
#include "MetasoundNodeInterface.h"
#include "MetasoundOperatorInterface.h"
#include "MetasoundTime.h"
#include "MetasoundTrigger.h"
#include "MetasoundNodeRegistrationMacro.h"
#include "MetasoundParamHelper.h"
#include "MetasoundPrimitives.h"
#include "MetasoundStandardNodesNames.h"
#include "MetasoundStandardNodesCategories.h"
#include "MetasoundVertex.h"

namespace Metasound
{
	class MUSICEXTRANODES_API MelodyPlayerNode : public FNodeFacade
	{
	public:
		MelodyPlayerNode(const FNodeInitData& InInitData);

		virtual ~MelodyPlayerNode() = default;
	};
}
