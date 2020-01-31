#pragma once

#include "RE/TESForm.h"
#include "RE/InputDevices.h"

#include <vector>

namespace MHK
{
	class HotkeyManager
	{
	public:

		class Hotkey
		{
		public:
			RE::TESForm* item;
			RE::INPUT_DEVICE device;
			UInt32 keyMask;
		};

	private:
		static HotkeyManager* singleton;

		std::vector<Hotkey*> hotkeys;
		std::vector<Hotkey*> vampireHotkeys;

	public:
		static HotkeyManager* GetSingleton();

		void AddHotkey(Hotkey* a_hotkey, bool a_isVampire);
		void RemoveHotkey(Hotkey* a_hotkey, bool a_isVampire);
		Hotkey* GetHotkey(RE::TESForm* a_form, bool a_isVampire);
		Hotkey* GetHotkey(RE::INPUT_DEVICE a_deviceType, UInt32 a_keyMask, bool a_isVampire);
		bool IsFavorited(RE::TESForm* a_form);
		bool IsVampireSpell(RE::TESForm* a_form);

		std::vector<Hotkey*> GetHotkeys();
		std::vector<Hotkey*> GetVampireHotkeys();

		void SetHotkeys(std::vector<Hotkey*> a_hotkeys);
		void SetVampireHotkeys(std::vector<Hotkey*> a_hotkeys);
	};
}