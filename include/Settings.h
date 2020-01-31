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

		bool					dualWieldSupport; //ok

		Button					modifierKey; //ok

		bool					useWhiteList; //ok
		std::vector<Button>		whitelist; //ok
		bool					allowOverride; //ok

		bool IsInWhitelist(RE::INPUT_DEVICE a_device, UInt32 a_id);

		static Settings* GetSingleton();
	};

	extern void LoadSettings();
}
