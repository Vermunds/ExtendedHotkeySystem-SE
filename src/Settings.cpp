#include "Settings.h"

#include <ModConfigUI/Binding.h>

#include <SimpleIni.h>

namespace
{
	constexpr const char* INI_PATH = R"(.\Data\SKSE\Plugins\ExtendedHotkeySystem.ini)";

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

	bool Settings::IsInWhitelist(RE::INPUT_DEVICE a_device, std::uint32_t a_id) const
	{
		for (std::vector<Button>::const_iterator it = this->whitelist.begin(); it != this->whitelist.end(); ++it)
		{
			if (it->id == a_id && it->inputDevice == a_device)
			{
				return true;
			}
		}
		return false;
	}

	std::vector<Settings::Button> Settings::GetWhitelist() const
	{
		return this->whitelist;
	}

	void Settings::SetWhitelist(std::vector<Button> a_whitelist)
	{
		this->whitelist = std::move(a_whitelist);
	}

	// The INI stores a button the way every other mod does, as one number in the unified key space
	// SKSE::InputMap defines. ModConfigUI speaks the same space, so the split into a device and an id
	// is its to make: this used to subtract 0xFF from a mouse button where the space starts them at
	// 0x100, which left every mouse button in the INI reading back as the one after it.
	Settings::Button GetButtonObj(std::uint32_t a_key)
	{
		ModConfigUI::ButtonBinding binding{ a_key };
		return Settings::Button{ binding.GetDevice(), binding.GetDeviceId() };
	}

	std::uint32_t GetButtonKey(const Settings::Button& a_button)
	{
		return ModConfigUI::ButtonBinding::FromEvent(a_button.inputDevice, a_button.id).key;
	}

	std::vector<Settings::Button> ParseWhitelist(const std::string& a_whitelistStr)
	{
		std::stringstream ss(a_whitelistStr);
		std::vector<Settings::Button> whitelist;
		while (ss.good())
		{
			std::string substr;
			getline(ss, substr, ',');

			// The file is hand edited often enough that a stray comma or a typo shouldn't take the mod
			// down. Anything that isn't a number the unified key space knows about is dropped.
			std::uint32_t key = 0;
			std::from_chars_result parsed = std::from_chars(substr.data(), substr.data() + substr.size(), key);
			if (parsed.ec != std::errc{})
			{
				if (!substr.empty())
				{
					SKSE::log::warn("Ignoring '{}' in sWhitelist, it is not a button number.", substr);
				}
				continue;
			}

			Settings::Button button = GetButtonObj(key);
			if (button.inputDevice == RE::INPUT_DEVICES::kNone)
			{
				SKSE::log::warn("Ignoring {} in sWhitelist, it is not a button.", key);
				continue;
			}

			whitelist.push_back(button);
		}
		return whitelist;
	}

	void LoadSettings()
	{
		using Button = Settings::Button;

		Settings* settings = Settings::GetSingleton();

		CSimpleIniA ini;
		ini.SetUnicode();
		ini.LoadFile(INI_PATH);

		SKSE::log::info("Loading settings from: {}", std::filesystem::absolute(INI_PATH).string());

		IniSection(ini, "GENERAL");
		//settings->dualWieldSupport = IniGetBool(ini, "GENERAL", "bDualWieldSupport", false, "# Allows you to equip the same weapon to the left hand if it's already equipped to the right (instead of unequipping it)\n# Only works with the same weapons and enchantments.As a rule of thumb : if it stacks in the inventory, it will work, otherwise no.\n# Default value is false (disabled)");

		std::uint32_t assignmentKey = IniGetUInt(ini, "GENERAL", "iAssignmentKey", ASSIGNMENT_KEY_DEFAULT_VALUE, "# The assignment key you have to press when assigning hotkeys in the favorites menu.\n# Requires a DirectInput scan code of the key you want to use.See the included scancodes.txt file for a list of buttons.\n# Example: iAssignmentKey = 45 is the the 'X' button.\n# Default value is 29, which is the left control key.");
		settings->assignmentKey = GetButtonObj(assignmentKey);

		settings->allowDuplicates = IniGetBool(ini, "GENERAL", "bAllowDuplicates", ALLOW_DUPLICATES_DEFAULT_VALUE, "# If enabled, assigning a button that is already in use keeps the old hotkey as well.\n# Pressing the button then equips or unequips all of its items together.\n# Default is false (disabled, the old hotkey is replaced)");

		IniSection(ini, "WHITELIST");
		settings->useWhitelist = IniGetBool(ini, "WHITELIST", "bUseWhitelist", USE_WHITELIST_DEFAULT_VALUE, "# Enable or disable the button whitelist. If enabled, only whitelisted buttons can be set as a hotkey.\n# You don't have to hold down the assignment key to assign these hotkeys.\n# Default is true (enabled)");

		std::string whitelistStr = IniGetString(ini, "WHITELIST", "sWhitelist", WHITELIST_DEFAULT_VALUE, "# The list of buttons that can be set as hotkey.\n# Requires a DirectInput scan code of the key you want to use.See the included scancodes.txt file for a list of buttons.\n# Separate the entries with commas(, ) do not use spaces or any other characters!\n# Example: sWhitelist = 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 (these are the numeric buttons from 0 to 9)");
		settings->SetWhitelist(ParseWhitelist(whitelistStr));

		settings->enforceWhitelist = IniGetBool(ini, "WHITELIST", "bEnforceWhitelist", ENFORCE_WHITELIST_DEFAULT_VALUE, "# If enabled, the assignment key + hotkey combination can no longer assign a hotkey outside of the whitelist.\n# Default is false (disabled)");

		SKSE::log::info("Settings loaded.");

		ini.SaveFile(INI_PATH);
	}

	void SaveSettings()
	{
		Settings* settings = Settings::GetSingleton();

		// Read back in first: the file holds the comments and the settings the menu can't change yet,
		// and writing it from scratch would drop all of them.
		CSimpleIniA ini;
		ini.SetUnicode();

		SI_Error result = ini.LoadFile(INI_PATH);
		if (result < 0)
		{
			SKSE::log::error("Failed to open {} for writing, the settings were not saved.", INI_PATH);
			return;
		}

		ini.SetLongValue("GENERAL", "iAssignmentKey", static_cast<long>(GetButtonKey(settings->assignmentKey)), nullptr, false, true);
		ini.SetBoolValue("GENERAL", "bAllowDuplicates", settings->allowDuplicates, nullptr, true);
		ini.SetBoolValue("WHITELIST", "bUseWhitelist", settings->useWhitelist, nullptr, true);
		ini.SetBoolValue("WHITELIST", "bEnforceWhitelist", settings->enforceWhitelist, nullptr, true);

		// Written the way the file has always held it: the unified key numbers, comma separated.
		std::vector<Settings::Button> whitelist = settings->GetWhitelist();
		std::string whitelistStr;
		for (std::vector<Settings::Button>::const_iterator it = whitelist.begin(); it != whitelist.end(); ++it)
		{
			if (!whitelistStr.empty())
			{
				whitelistStr += ',';
			}
			whitelistStr += std::to_string(GetButtonKey(*it));
		}
		ini.SetValue("WHITELIST", "sWhitelist", whitelistStr.c_str(), nullptr, true);

		result = ini.SaveFile(INI_PATH);
		if (result < 0)
		{
			SKSE::log::error("Failed to save the settings to {}.", INI_PATH);
			return;
		}

		SKSE::log::info("Settings saved.");
	}

	void RestoreDefaults()
	{
		Settings* settings = Settings::GetSingleton();

		settings->assignmentKey = GetButtonObj(ASSIGNMENT_KEY_DEFAULT_VALUE);
		settings->allowDuplicates = ALLOW_DUPLICATES_DEFAULT_VALUE;
		settings->useWhitelist = USE_WHITELIST_DEFAULT_VALUE;
		settings->enforceWhitelist = ENFORCE_WHITELIST_DEFAULT_VALUE;
		settings->SetWhitelist(ParseWhitelist(WHITELIST_DEFAULT_VALUE));

		SaveSettings();
	}
}
