// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#pragma once

#include "Engine/Attenuation.h"
#include "IAudioExtensionPlugin.h"
#include "IAudioParameterInterfaceRegistry.h"

// Defines a MetaSound interface that inputs Modal Spatialization Interface Params
namespace ModalSpatialParameterInterface
{
	const extern FName Name;

	namespace Inputs
	{
		const extern FName EnableHRTF;
		const extern FName RoomSize;
		const extern FName Absorption;
		const extern FName OpenRoomFactor;
		const extern FName EchoVar;
	} 

	Audio::FParameterInterfacePtr GetInterface();
	void RegisterInterface();
} 
