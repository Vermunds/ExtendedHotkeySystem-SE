#pragma once
#include "skse64_common/Relocation.h"

namespace Offsets {

	//FavoritesHandler
	//const RelocAddr<uintptr_t*> FavoritesHandler_ProcessButton = 0x8A9D60;
	const RelocAddr<uintptr_t*> FavoritesHandler_CanProcess_IsHotkey_Hook = 0x8AACBF;

	//FavoritesMenu
	//const RelocAddr<uintptr_t*> FavoritesMenu_ProcessButton = 0x878210;
	//const RelocAddr<uintptr_t*> FavoritesMenu_CanProcess = 0x8781C0;

	//EquipManager
	const RelocAddr<uintptr_t*> EquipShout = 0x637CA0;
	const RelocAddr<uintptr_t*> EquipSpell = 0x637BC0;

	//Actor
	const RelocAddr<uintptr_t*> IsEquipped = 0x94B4D0;

	//Misc
	const RelocAddr<uintptr_t*> GetSpellLeftEquipSlot = 0x3315F0;
	const RelocAddr<uintptr_t*> GetSpellRightEquipSlot = 0x331620;
	const RelocAddr<uintptr_t*> PlaySound = 0x8DA860;
}
