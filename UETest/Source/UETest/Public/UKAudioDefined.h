// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "MetasoundEnumRegistrationMacro.h"

UENUM()
enum class EUKFootStepSoune : int32
{
	Ground = 0,
	Glass,
	Iron,
};

namespace Metasound
{
	DECLARE_METASOUND_ENUM(
		EUKFootStepSoune,
		EUKFootStepSoune::Ground,
		UETEST_API,
		FEnumUKFootStepSouneType,
		FEnumUKFootStepSouneTypeInfo,
		FEnumUKFootStepSouneTypeReadReference,
		FEnumUKFootStepSouneTypeWriteReference);
}

// #define LOCTEXT_NAMESPACE "FEnumTestModule"
// namespace Metasound
// {
// 	enum class EMyEnum
// 	{
// 		SomeEntry = 0,
// 		AnotherEntry,
// 	};
//
// 	DECLARE_METASOUND_ENUM(EMyEnum, EMyEnum::SomeEntry, UETEST_API, FEnumMyEnum, FEnumMyEnumInfo, FEnumMyEnumReadRef, FEnumMyEnumWriteRef);
//
// 	DEFINE_METASOUND_ENUM_BEGIN(EMyEnum, FEnumMyEnum, "MyEnum")
// 		DEFINE_METASOUND_ENUM_ENTRY(EMyEnum::SomeEntry, "SomeEntryDescription", "SomeEntry", "SomeEntryDescriptionTT", "Some Entry"),
// 		DEFINE_METASOUND_ENUM_ENTRY(EMyEnum::AnotherEntry, "AnotherEntryDescription", "AnotherEntry", "AnotherEntryDescriptionTT", "Another Entry"),
// 	DEFINE_METASOUND_ENUM_END()
//
// 	namespace EnumTestNode
// 	{
// 		METASOUND_PARAM(MyEnumPin, "Enum Value", "Set the enum value")
// 	}
//
// 	class FMyEnumTestOperator : public TExecutableOperator<FMyEnumTestOperator>
// 	{
// 	public:
// 		FMyEnumTestOperator(const FOperatorSettings& InSettings, FEnumMyEnumReadRef InMyEnum)
// 			: MyEnumValue(InMyEnum) {}
//
// 		static const FNodeClassMetadata& GetNodeInfo()
// 		{
// 			auto InitNodeInfo = []() -> FNodeClassMetadata
// 				{
// 					FNodeClassMetadata Info;
//
// 					Info.ClassName = { TEXT("UE"), TEXT("Enum Test"), TEXT("Audio") };
// 					Info.MajorVersion = 1;
// 					Info.MinorVersion = 0;
// 					Info.DisplayName = LOCTEXT("EnumTest_DisplayName", "Enum Test");
// 					Info.DefaultInterface = GetVertexInterface();
//
// 					return Info;
// 				};
//
// 			static const FNodeClassMetadata Info = InitNodeInfo();
//
// 			return Info;
// 		}
//
// 		virtual void BindInputs(FInputVertexInterfaceData& InOutVertexData) override
// 		{
// 			using namespace EnumTestNode;
//
// 			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(MyEnumPin), MyEnumValue);
// 		}
//
//
// 		static const FVertexInterface& GetVertexInterface()
// 		{
// 			using namespace EnumTestNode;
//
// 			static const FVertexInterface Interface(
// 				FInputVertexInterface(
// 					TInputDataVertex<FEnumMyEnum>(METASOUND_GET_PARAM_NAME_AND_METADATA(MyEnumPin), (int32)(EMyEnum::SomeEntry))
// 				),
// 				FOutputVertexInterface()
// 			);
//
// 			return Interface;
// 		}
//
//
// 		static TUniquePtr<IOperator> CreateOperator(const Metasound::FCreateOperatorParams& InParams, Metasound::FBuildErrorArray& OutErrors)
// 		{
// 			using namespace EnumTestNode;
//
// 			const FDataReferenceCollection& InputCollection = InParams.InputDataReferences;
// 			const FInputVertexInterface& InputInterface = GetVertexInterface().GetInputInterface();
//
//
// 			FEnumMyEnumReadRef MyEnumRef = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<FEnumMyEnum>(InputInterface, METASOUND_GET_PARAM_NAME(MyEnumPin), InParams.OperatorSettings);
//
// 			return MakeUnique<FMyEnumTestOperator>(InParams.OperatorSettings, MyEnumRef);
// 		}
//
// 		void Execute() {}
//
// 	private:
// 		FEnumMyEnumReadRef MyEnumValue;
// 	};
//
//
// 	class FMyEnumTestNode : public FNodeFacade
// 	{
// 	public:
// 		FMyEnumTestNode(const FNodeInitData& InitData)
// 			: FNodeFacade(InitData.InstanceName, InitData.InstanceID, TFacadeOperatorClass<FMyEnumTestOperator>())
// 		{
//
// 		}
// 	};
//
// 	METASOUND_REGISTER_NODE(FMyEnumTestNode);
// }
// #undef LOCTEXT_NAMESPACE