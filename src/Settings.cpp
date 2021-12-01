#include "Settings.h"

#include "SimpleIni.h"
#include <sstream>
#include <string>
#include <vector>

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

		CSimpleIniA ini;
		ini.SetUnicode();
		ini.LoadFile(".\\Data\\SKSE\\Plugins\\ExtendedHotkeySystem.ini");

		//GENERAL
		//settings->dualWieldSupport = ini.GetBoolValue("GENERAL", "bDualWieldSupport", false, false);
		//ini.SetBoolValue("GENERAL", "bDualWieldSupport", settings->dualWieldSupport, "# Allows you to equip the same weapon to the left hand if it's already equipped to the right (instead of unequipping it)\n# Only works with the same weapons and enchantments.As a rule of thumb : if it stacks in the inventory, it will work, otherwise no.\n# Default value is false (disabled)", true);

		std::uint32_t modifierKey = static_cast<std::uint32_t>(ini.GetLongValue("GENERAL", "iModifierKey", 29));
		settings->modifierKey = GetButtonObj(modifierKey);
		ini.SetLongValue("GENERAL", "iModifierKey", modifierKey, "# The modifier key you have to press when assigning hotkeys in the favorites menu.\n# Requires a DirectInput scan code of the key you want to use.See the included scancodes.txt file for a list of buttons.\n# Example: iUnequipAllKeyCode = 45 is the the 'X' button.\n# Default value is 29, which is the left control key.", false, true);

		//WHITELIST
		settings->useWhiteList = ini.GetBoolValue("WHITELIST", "bEnableWhitelist", true);
		ini.SetBoolValue("WHITELIST", "bEnableWhitelist", settings->useWhiteList, "# Enable or disable the button whitelist. If enabled, only whitelisted buttons can be set as a hotkey.\n# You don't have to hold down the modifier key to assign these hotkeys.\n# Default is 0 (disabled, modifier key is needed)", true);

		std::string whitelistStr = ini.GetValue("WHITELIST", "sWhitelist", "2,3,4,5,6,7,8,9,10,11");
		std::stringstream ss(whitelistStr);
		std::vector<Button> whitelist;
		while (ss.good())
		{
			std::string substr;
			getline(ss, substr, ',');
			whitelist.push_back(GetButtonObj(std::stoi(substr)));
		}
		settings->whitelist = whitelist;
		ini.SetValue("WHITELIST", "sWhitelist", whitelistStr.c_str(), "# The list of buttons that can be set as hotkey.\n# Requires a DirectInput scan code of the key you want to use.See the included scancodes.txt file for a list of buttons.\n# Separate the entries with commas(, ) do not use spaces or any other characters!\n# Example: sWhitelist = 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 (these are the numeric buttons from 0 to 9)", true);

		settings->allowOverride = ini.GetBoolValue("WHITELIST", "bAllowWhitelistOverride", true);
		ini.SetBoolValue("WHITELIST", "bAllowWhitelistOverride", settings->allowOverride, "# If enabled you can still use the Ctrl + hotkey combination to assign a hotkey outside of the whitelist.", true);
		ini.SaveFile(".\\Data\\SKSE\\Plugins\\ExtendedHotkeySystem.ini");
	}
}
