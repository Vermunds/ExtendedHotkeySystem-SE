#pragma once

#include "Hotkey/Hotkey.h"
#include "Hotkey/ItemHotkey.h"
#include "Hotkey/MagicHotkey.h"

#include <map>

namespace EHKS
{
	class HotkeyManager
	{
	public:
		static Hotkey::HotkeyType GetHotkeyType(RE::TESForm* a_form);
		static HotkeyManager* GetSingleton();

		std::uint8_t UpdateHotkeys();  //Return value = next free hotkey slot

		void SetHotkeyExtraData(RE::InventoryEntryData* a_entryData, std::uint8_t a_id);

		ItemHotkey* GetItemHotkey(RE::InventoryEntryData* a_entryData);
		MagicHotkey* GetMagicHotkey(RE::TESForm* a_form);
		MagicHotkey* GetVampireHotkey(RE::TESForm* a_form);

		Hotkey* GetHotkey(RE::INPUT_DEVICE a_deviceType, std::uint32_t a_keyMask);
		MagicHotkey* GetVampireHotkey(RE::INPUT_DEVICE a_deviceType, std::uint32_t a_keyMask);

		bool RemoveHotkey(RE::INPUT_DEVICE a_deviceType, std::uint32_t a_keyMask);
		bool RemoveVampireHotkey(RE::INPUT_DEVICE a_deviceType, std::uint32_t a_keyMask);

		ItemHotkey* AddItemHotkey(RE::INPUT_DEVICE a_deviceType, std::uint32_t a_keyMask, RE::InventoryEntryData* a_entryData);
		MagicHotkey* AddMagicHotkey(RE::INPUT_DEVICE a_deviceType, std::uint32_t a_keyMask, RE::TESForm* a_form);
		MagicHotkey* AddVampireHotkey(RE::INPUT_DEVICE a_deviceType, std::uint32_t a_keyMask, RE::TESForm* a_form);

		bool IsMagicFavorited(RE::TESForm* a_form);
		bool IsVampireSpell(RE::TESForm* a_form);

		//Serialization
		std::list<Hotkey*> GetHotkeys();
		std::list<Hotkey*> GetVampireHotkeys();

		//Called after loading done
		void SetHotkeys(std::list<Hotkey*> a_hotkeys, std::list<Hotkey*> a_vampireHotkeys);

	private:
		static HotkeyManager* singleton;

		std::list<Hotkey*> hotkeys;
		std::list<MagicHotkey*> vampireHotkeys;
	};
}
