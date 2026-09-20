#pragma once

#include "Hotkey/Hotkey.h"
#include "Hotkey/ItemHotkey.h"
#include "Hotkey/MagicHotkey.h"

namespace EHKS
{
	class HotkeyManager
	{
	public:
		static Hotkey::HotkeyType GetHotkeyType(RE::TESForm* a_form);
		static HotkeyManager* GetSingleton();

		// Whether a_form goes in either hand, so an equip mode can pick one: one-handed weapons and spells
		static bool CanChooseHand(RE::TESForm* a_form);

		std::uint8_t UpdateHotkeys();  //Return value = next free hotkey slot

		void SetHotkeyExtraData(RE::InventoryEntryData* a_entryData, std::uint8_t a_id);

		ItemHotkey* GetItemHotkey(RE::InventoryEntryData* a_entryData);
		MagicHotkey* GetMagicHotkey(RE::TESForm* a_form);
		MagicHotkey* GetVampireHotkey(RE::TESForm* a_form);

		Hotkey* GetHotkey(RE::INPUT_DEVICE a_deviceType, std::uint32_t a_keyMask);
		MagicHotkey* GetVampireHotkey(RE::INPUT_DEVICE a_deviceType, std::uint32_t a_keyMask);

		//Every hotkey on the button for the player's current form
		std::vector<Hotkey*> GetActiveHotkeys(RE::INPUT_DEVICE a_deviceType, std::uint32_t a_keyMask);

		bool RemoveHotkey(RE::INPUT_DEVICE a_deviceType, std::uint32_t a_keyMask);
		bool RemoveHotkey(const Hotkey* a_hotkey);
		bool RemoveVampireHotkey(RE::INPUT_DEVICE a_deviceType, std::uint32_t a_keyMask);
		bool RemoveVampireHotkey(const Hotkey* a_hotkey);

		// False when a_hotkey no longer exists
		bool SetEquipMode(const Hotkey* a_hotkey, Hotkey::EquipMode a_equipMode);

		ItemHotkey* AddItemHotkey(RE::INPUT_DEVICE a_deviceType, std::uint32_t a_keyMask, RE::InventoryEntryData* a_entryData);
		MagicHotkey* AddMagicHotkey(RE::INPUT_DEVICE a_deviceType, std::uint32_t a_keyMask, RE::TESForm* a_form);
		MagicHotkey* AddVampireHotkey(RE::INPUT_DEVICE a_deviceType, std::uint32_t a_keyMask, RE::TESForm* a_form);

		bool IsMagicFavorited(RE::TESForm* a_form);
		bool IsVampireSpell(RE::TESForm* a_form);
		bool IsPlayerVampire();

		//Serialization
		const std::list<Hotkey*>& GetHotkeys();
		const std::list<Hotkey*>& GetVampireHotkeys();

		//Called after loading done
		void SetHotkeys(std::list<Hotkey*> a_hotkeys, std::list<Hotkey*> a_vampireHotkeys);

	private:
		HotkeyManager() {};
		~HotkeyManager() {};
		HotkeyManager(const HotkeyManager&) = delete;
		HotkeyManager& operator=(const HotkeyManager&) = delete;

		static std::vector<Hotkey*> CollectByKey(const std::list<Hotkey*>& a_source, RE::INPUT_DEVICE a_deviceType, std::uint32_t a_keyMask);

		std::list<Hotkey*> hotkeys;
		std::list<Hotkey*> vampireHotkeys;
	};
}
