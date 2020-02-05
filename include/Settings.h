#pragma once

#include <string>
#include <vector>

#include "RE/InputDevices.h"

namespace MHK
{
	class Settings
	{
	private:
		static Settings* singleton;
		Settings();
	public:

		struct Button
		{
			RE::INPUT_DEVICE inputDevice;
			UInt32           id;
		};

		//bool					dualWieldSupport;

		Button					modifierKey;

		bool					useWhiteList;
		std::vector<Button>		whitelist;
		bool					allowOverride;

		bool IsInWhitelist(RE::INPUT_DEVICE a_device, UInt32 a_id);

		static Settings* GetSingleton();
	};

	extern void LoadSettings();
}
