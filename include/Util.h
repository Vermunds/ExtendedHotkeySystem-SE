#pragma once
#include "RE/BSFixedString.h"

namespace MHK
{
	UInt32 GetGamepadIconIndex(UInt32 scanCode);
	bool IsVanillaHotkey(RE::BSFixedString a_userEvent);
}