// Copyright 2023-2024, Le Binh Son, All rights reserved.


#include "ImpactSynthCurve.h"
#include "Curves/CurveFloat.h"

void FImpactSynthCurve::CacheCurve()
{
	if (CurveSource == EImpactSynthCurveSource::Shared && CurveShared)
	{
		CurveCustom = CurveShared->FloatCurve;
	}

	// Always clear shared curve regardless to avoid
	// holding a reference to the UObject when never
	// expected to be used.
	CurveShared = nullptr;
}
