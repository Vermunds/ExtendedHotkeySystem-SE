#pragma once
#include "RE/NiPoint2.h"
#include "RE/NiPlane.h"

namespace MHK
{
	UInt32 GetGamepadIconIndex(UInt32 scanCode);
	bool IsPointInPlane(RE::NiPoint2 a_point, RE::NiPoint2 a_planePos, RE::NiPoint2 a_planeEnd);
}