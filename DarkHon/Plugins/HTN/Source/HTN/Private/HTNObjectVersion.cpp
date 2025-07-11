// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "HTNObjectVersion.h"
#include "Serialization/CustomVersion.h"

const FGuid FHTNObjectVersion::GUID(0xC9E2F84A, 0xF6384E76, 0x941EB4F2, 0xE952E7DD);
FCustomVersionRegistration GRegisterHTNObjectVersion(FHTNObjectVersion::GUID, FHTNObjectVersion::LatestVersion, TEXT("HTNObjectVer"));
