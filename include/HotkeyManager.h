#pragma once

#include "RE/Skyrim.h"

#include <map>

namespace MHK
{
	class HotkeyManager
	{
	public:

		class Hotkey
		{
		public:
			enum class Type
			{
				kInvalid,
				kItem,
				kMagic
			};

			RE::INPUT_DEVICE device;
			UInt32 keyMask;
			Type type = Type::kInvalid;
		};

		class ItemHotkey : public Hotkey
		{
		public:
			UInt8 extraDataId;
		};

		class MagicHotkey : public Hotkey
		{
		public:
			RE::TESForm* form;
		};

		class ControllerHotkey
		{
			Hotkey* hotkey;
		};

	private:
		static HotkeyManager* singleton;

		std::list<Hotkey*> hotkeys;
		std::list<MagicHotkey*> vampireHotkeys;

	public:
		static Hotkey::Type GetHotkeyType(RE::TESForm* a_form);
		static HotkeyManager* GetSingleton();

		UInt8 UpdateHotkeys(); //Return value = next free hotkey slot

		RE::ExtraDataList* GetHotkeyData(ItemHotkey* a_hotkey);
		RE::TESForm* GetBaseForm(ItemHotkey* a_hotkey);
		void SetHotkeyExtraData(RE::InventoryEntryData* a_entryData, UInt8 a_id);

		ItemHotkey* GetItemHotkey(RE::InventoryEntryData* a_entryData);
		MagicHotkey* GetMagicHotkey(RE::TESForm* a_form);
		MagicHotkey* GetVampireHotkey(RE::TESForm* a_form);

		Hotkey* GetHotkey(RE::INPUT_DEVICE a_deviceType, UInt32 a_keyMask);
		MagicHotkey* GetVampireHotkey(RE::INPUT_DEVICE a_deviceType, UInt32 a_keyMask);

		bool RemoveHotkey(RE::INPUT_DEVICE a_deviceType, UInt32 a_keyMask);
		bool RemoveVampireHotkey(RE::INPUT_DEVICE a_deviceType, UInt32 a_keyMask);

		ItemHotkey* AddItemHotkey(RE::INPUT_DEVICE a_deviceType, UInt32 a_keyMask, RE::InventoryEntryData* a_entryData);
		MagicHotkey* AddMagicHotkey(RE::INPUT_DEVICE a_deviceType, UInt32 a_keyMask, RE::TESForm* a_form);
		MagicHotkey* AddVampireHotkey(RE::INPUT_DEVICE a_deviceType, UInt32 a_keyMask, RE::TESForm* a_form);

		bool IsMagicFavorited(RE::TESForm* a_form);
		bool IsVampireSpell(RE::TESForm* a_form);


		//Serialization
		std::list<Hotkey*> GetHotkeys();
		std::list<Hotkey*> GetVampireHotkeys();

		//Called after loading done
		void SetHotkeys(std::list<Hotkey*> a_hotkeys, std::list<Hotkey*> a_vampireHotkeys);
	};
}