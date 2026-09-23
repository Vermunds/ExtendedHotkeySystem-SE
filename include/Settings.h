#pragma once

namespace EHKS
{
	// General
	constexpr std::uint32_t ASSIGNMENT_KEY_DEFAULT_VALUE = 29;  // Left control
	constexpr bool ALLOW_DUPLICATES_DEFAULT_VALUE = false;
	constexpr bool DUAL_WIELD_SUPPORT_DEFAULT_VALUE = true;

	// Whitelist
	constexpr bool USE_WHITELIST_DEFAULT_VALUE = true;
	constexpr const char* WHITELIST_DEFAULT_VALUE = "2,3,4,5,6,7,8,9,10,11";  // The number row, 1 to 0
	constexpr bool ENFORCE_WHITELIST_DEFAULT_VALUE = false;

	class Settings
	{
	public:
		struct Button
		{
			RE::INPUT_DEVICE inputDevice;
			std::uint32_t id;
		};

		Button assignmentKey;
		bool allowDuplicates;
		bool dualWieldSupport;

		bool useWhitelist;
		bool enforceWhitelist;

		bool IsInWhitelist(RE::INPUT_DEVICE a_device, std::uint32_t a_id) const;

		std::vector<Button> GetWhitelist() const;
		void SetWhitelist(std::vector<Button> a_whitelist);

		static Settings* GetSingleton();

	private:
		Settings() {};
		~Settings() {};
		Settings(const Settings&) = delete;
		Settings& operator=(const Settings&) = delete;

		std::vector<Button> whitelist;
	};

	void LoadSettings();
	void SaveSettings();
	void RestoreDefaults();
}
