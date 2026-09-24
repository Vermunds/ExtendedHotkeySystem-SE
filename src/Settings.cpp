#include "Settings.h"

#include <ModConfigUI/Binding.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <SimpleIni.h>

namespace
{
	constexpr const char* INI_PATH = R"(.\Data\SKSE\Plugins\ExtendedHotkeySystem.ini)";

	void IniSection(CSimpleIniA& a_ini, const char* a_section, const char* a_comment = nullptr)
	{
		a_ini.SetValue(a_section, nullptr, nullptr, a_comment);
		logger::info("[{}]", a_section);
	}

	bool IniGetBool(CSimpleIniA& a_ini, const char* a_section, const char* a_key, bool a_default, const char* a_comment = nullptr)
	{
		bool val = a_ini.GetBoolValue(a_section, a_key, a_default);
		a_ini.SetBoolValue(a_section, a_key, val, a_comment, true);
		logger::info("  {}: {}", a_key, val);
		return val;
	}

	std::uint32_t IniGetUInt(CSimpleIniA& a_ini, const char* a_section, const char* a_key, std::uint32_t a_default, const char* a_comment = nullptr)
	{
		std::uint32_t val = static_cast<std::uint32_t>(a_ini.GetLongValue(a_section, a_key, a_default));
		a_ini.SetLongValue(a_section, a_key, val, a_comment, false, true);
		logger::info("  {}: {}", a_key, val);
		return val;
	}

	std::string IniGetString(CSimpleIniA& a_ini, const char* a_section, const char* a_key, const char* a_default, const char* a_comment = nullptr)
	{
		std::string val = a_ini.GetValue(a_section, a_key, a_default);
		a_ini.SetValue(a_section, a_key, val.c_str(), a_comment, true);
		logger::info("  {}: {}", a_key, val);
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
					logger::warn("Ignoring '{}' in sWhitelist, it is not a button number.", substr);
				}
				continue;
			}

			Settings::Button button = GetButtonObj(key);
			if (button.inputDevice == RE::INPUT_DEVICES::kNone)
			{
				logger::warn("Ignoring {} in sWhitelist, it is not a button.", key);
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

		logger::info("Loading settings from: {}", std::filesystem::absolute(INI_PATH).string());

		IniSection(ini, "GENERAL");
		std::uint32_t assignmentKey = IniGetUInt(ini, "GENERAL", "iAssignmentKey", ASSIGNMENT_KEY_DEFAULT_VALUE, "# Hold this key while clicking an item in the Favorites menu to assign it a hotkey.\n# The value is a button code: keyboard keys use their DirectInput scan code, mouse buttons start at 256 and controller buttons at 266. The easiest way to change it is the in-game settings menu.\n# Example: iAssignmentKey = 45 is the 'X' key.\n# Default value is 29, which is the left control key.");
		settings->assignmentKey = GetButtonObj(assignmentKey);

		settings->allowDuplicates = IniGetBool(ini, "GENERAL", "bAllowDuplicates", ALLOW_DUPLICATES_DEFAULT_VALUE, "# When enabled, assigning a button that is already in use keeps its old hotkey too, instead of replacing it.\n# Pressing the button then equips or unequips all of them together.\n# Default is false (disabled)");

		settings->dualWieldSupport = IniGetBool(ini, "GENERAL", "bDualWieldSupport", DUAL_WIELD_SUPPORT_DEFAULT_VALUE, "# When enabled, if the weapon is already in one hand and you carry another, the hotkey equips that to the other hand instead of unequipping it.\n# This only works for items that are stacked in the inventory.\n# Default is true (enabled)");

		IniSection(ini, "WHITELIST");
		settings->useWhitelist = IniGetBool(ini, "WHITELIST", "bUseWhitelist", USE_WHITELIST_DEFAULT_VALUE, "# Whitelisted buttons assign a hotkey on their own, without holding the assignment key.\n# Everything else still needs the assignment key.\n# Default is true (enabled)");

		std::string whitelistStr = IniGetString(ini, "WHITELIST", "sWhitelist", WHITELIST_DEFAULT_VALUE, "# The list of whitelisted buttons, as button codes (see iAssignmentKey). Separate the entries with commas, without spaces.\n# Example: sWhitelist = 2,3,4,5,6,7,8,9,10,11 (the number row, from 1 to 0)");
		settings->SetWhitelist(ParseWhitelist(whitelistStr));

		settings->enforceWhitelist = IniGetBool(ini, "WHITELIST", "bEnforceWhitelist", ENFORCE_WHITELIST_DEFAULT_VALUE, "# When enabled, only whitelisted buttons can be assigned.\n# The assignment key is disabled, and its hint is hidden in the Favorites menu.\n# Default is false (disabled)");

		logger::info("Settings loaded.");

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
			logger::error("Failed to open {} for writing, the settings were not saved.", INI_PATH);
			return;
		}

		ini.SetLongValue("GENERAL", "iAssignmentKey", static_cast<long>(GetButtonKey(settings->assignmentKey)), nullptr, false, true);
		ini.SetBoolValue("GENERAL", "bAllowDuplicates", settings->allowDuplicates, nullptr, true);
		ini.SetBoolValue("GENERAL", "bDualWieldSupport", settings->dualWieldSupport, nullptr, true);
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
			logger::error("Failed to save the settings to {}.", INI_PATH);
			return;
		}

		logger::info("Settings saved.");
	}

	void RestoreDefaults()
	{
		Settings* settings = Settings::GetSingleton();

		settings->assignmentKey = GetButtonObj(ASSIGNMENT_KEY_DEFAULT_VALUE);
		settings->allowDuplicates = ALLOW_DUPLICATES_DEFAULT_VALUE;
		settings->dualWieldSupport = DUAL_WIELD_SUPPORT_DEFAULT_VALUE;
		settings->useWhitelist = USE_WHITELIST_DEFAULT_VALUE;
		settings->enforceWhitelist = ENFORCE_WHITELIST_DEFAULT_VALUE;
		settings->SetWhitelist(ParseWhitelist(WHITELIST_DEFAULT_VALUE));

		SaveSettings();
	}
}
