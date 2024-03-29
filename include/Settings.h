#pragma once

#include <string>
#include <vector>

namespace EHKS
{
	class Settings
	{
	public:
		struct Button
		{
			RE::INPUT_DEVICE inputDevice;
			std::uint32_t id;
		};

		//bool					dualWieldSupport;

		Button modifierKey;

		bool useWhiteList;
		std::vector<Button> whitelist;
		bool allowOverride;

		bool IsInWhitelist(RE::INPUT_DEVICE a_device, std::uint32_t a_id);

		static Settings* GetSingleton();

	private:
		Settings(){};
		~Settings(){};
		Settings(const Settings&) = delete;
		Settings& operator=(const Settings&) = delete;
	};

	extern void LoadSettings();
}
