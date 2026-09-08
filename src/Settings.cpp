#include "Settings.h"

#include <SimpleIni.h>

#include <sstream>
#include <string>
#include <vector>

namespace
{
	void IniSection(CSimpleIniA& a_ini, const char* a_section, const char* a_comment = nullptr)
	{
		a_ini.SetValue(a_section, nullptr, nullptr, a_comment);
		SKSE::log::info("[{}]", a_section);
	}

	bool IniGetBool(CSimpleIniA& a_ini, const char* a_section, const char* a_key, bool a_default, const char* a_comment = nullptr)
	{
		bool val = a_ini.GetBoolValue(a_section, a_key, a_default);
		a_ini.SetBoolValue(a_section, a_key, val, a_comment, true);
		SKSE::log::info("  {}: {}", a_key, val);
		return val;
	}

	std::uint32_t IniGetUInt(CSimpleIniA& a_ini, const char* a_section, const char* a_key, std::uint32_t a_default, const char* a_comment = nullptr)
	{
		std::uint32_t val = static_cast<std::uint32_t>(a_ini.GetLongValue(a_section, a_key, a_default));
		a_ini.SetLongValue(a_section, a_key, val, a_comment, false, true);
		SKSE::log::info("  {}: {}", a_key, val);
		return val;
	}

	std::string IniGetString(CSimpleIniA& a_ini, const char* a_section, const char* a_key, const char* a_default, const char* a_comment = nullptr)
	{
		std::string val = a_ini.GetValue(a_section, a_key, a_default);
		a_ini.SetValue(a_section, a_key, val.c_str(), a_comment, true);
		SKSE::log::info("  {}: {}", a_key, val);
		return val;
	}
}

namespace EHKS
{
	Settings* Settings::GetSingleton()
	{
		static Settings singleton;
		return &singleton;
	}

	bool Settings::IsInWhitelist(RE::INPUT_DEVICE a_device, std::uint32_t a_id)
	{
		for (auto it = this->whitelist.begin(); it != this->whitelist.end(); ++it)
		{
			Button button = *it;
			if (button.id == a_id && button.inputDevice == a_device)
			{
				return true;
			}
		}
		return false;
	}

	Settings::Button& GetButtonObj(std::uint32_t a_id)
	{
		using Button = Settings::Button;

		Button* button = new Button();
		if (a_id > 0xFF)
		{
			button->inputDevice = RE::INPUT_DEVICE::kMouse;
			button->id = a_id - 0xFF;
		}
		else
		{
			button->inputDevice = RE::INPUT_DEVICE::kKeyboard;
			button->id = a_id;
		}
		return *button;
	}

	void LoadSettings()
	{
		using Button = Settings::Button;

		Settings* settings = Settings::GetSingleton();

		constexpr const char* iniPath = R"(.\Data\SKSE\Plugins\ExtendedHotkeySystem.ini)";

		CSimpleIniA ini;
		ini.SetUnicode();
		ini.LoadFile(iniPath);

		SKSE::log::info("Loading settings from: {}", std::filesystem::absolute(iniPath).string());

		IniSection(ini, "GENERAL");
		//settings->dualWieldSupport = IniGetBool(ini, "GENERAL", "bDualWieldSupport", false, "# Allows you to equip the same weapon to the left hand if it's already equipped to the right (instead of unequipping it)\n# Only works with the same weapons and enchantments.As a rule of thumb : if it stacks in the inventory, it will work, otherwise no.\n# Default value is false (disabled)");

		std::uint32_t modifierKey = IniGetUInt(ini, "GENERAL", "iModifierKey", 29, "# The modifier key you have to press when assigning hotkeys in the favorites menu.\n# Requires a DirectInput scan code of the key you want to use.See the included scancodes.txt file for a list of buttons.\n# Example: iUnequipAllKeyCode = 45 is the the 'X' button.\n# Default value is 29, which is the left control key.");
		settings->modifierKey = GetButtonObj(modifierKey);

		IniSection(ini, "WHITELIST");
		settings->useWhiteList = IniGetBool(ini, "WHITELIST", "bEnableWhitelist", true, "# Enable or disable the button whitelist. If enabled, only whitelisted buttons can be set as a hotkey.\n# You don't have to hold down the modifier key to assign these hotkeys.\n# Default is 0 (disabled, modifier key is needed)");

		std::string whitelistStr = IniGetString(ini, "WHITELIST", "sWhitelist", "2,3,4,5,6,7,8,9,10,11", "# The list of buttons that can be set as hotkey.\n# Requires a DirectInput scan code of the key you want to use.See the included scancodes.txt file for a list of buttons.\n# Separate the entries with commas(, ) do not use spaces or any other characters!\n# Example: sWhitelist = 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 (these are the numeric buttons from 0 to 9)");
		std::stringstream ss(whitelistStr);
		std::vector<Button> whitelist;
		while (ss.good())
		{
			std::string substr;
			getline(ss, substr, ',');
			whitelist.push_back(GetButtonObj(std::stoi(substr)));
		}
		settings->whitelist = whitelist;

		settings->allowOverride = IniGetBool(ini, "WHITELIST", "bAllowWhitelistOverride", true, "# If enabled you can still use the Ctrl + hotkey combination to assign a hotkey outside of the whitelist.");

		SKSE::log::info("Settings loaded.");

		ini.SaveFile(iniPath);
	}
}
