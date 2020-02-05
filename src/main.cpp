#include "skse64_common/BranchTrampoline.h" 
#include "skse64_common/skse_version.h"
#include "skse64/PluginAPI.h"

#include "version.h"
#include "Settings.h"
#include "HotkeyManager.h"
#include "Serialization.h"
#include "Hooks_FavoritesMenu.h"
#include "Hooks_FavoritesHandler.h"

#include "RE/UI.h"

#include "SKSE/API.h"

extern "C" {
	bool SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
	{
		SKSE::Logger::OpenRelative(FOLDERID_Documents, L"\\My Games\\Skyrim Special Edition\\SKSE\\ExtendedHotkeySystem.log");
		SKSE::Logger::SetPrintLevel(SKSE::Logger::Level::kDebugMessage);
		SKSE::Logger::SetFlushLevel(SKSE::Logger::Level::kDebugMessage);
		SKSE::Logger::UseLogStamp(true);

		_MESSAGE("Extended Hotkey System v%s", MOREHOTKEYS_VERSION_VERSTRING);

		a_info->infoVersion = PluginInfo::kInfoVersion;
		a_info->name = "Extended Hotkey System";
		a_info->version = 1;

		if (a_skse->IsEditor()) {
			_FATALERROR("Loaded in editor, marking as incompatible!\n");
			return false;
		}

		if (a_skse->RuntimeVersion() != RUNTIME_VERSION_1_5_97) {
			_FATALERROR("Unsupported runtime version %08X!\n", a_skse->RuntimeVersion());
			return false;
		}

		if (SKSE::AllocTrampoline(1024 * 8))
		{
			_MESSAGE("Branch trampoline creation successful");
		}
		else {
			_FATALERROR("Branch trampoline creation failed!\n");
			return false;
		}

		return true;
	}

	bool SKSEPlugin_Load(SKSE::LoadInterface* a_skse)
	{
		if (!SKSE::Init(a_skse)) {
			_FATALERROR("SKSE init failed!");
			return false;
		}

		auto serialization = SKSE::GetSerializationInterface();
		if (!serialization)
		{
			_FATALERROR("Couldn't get serialization interface!\n");
			return false;
		}
		serialization->SetUniqueID('EHKS');
		serialization->SetSaveCallback(Serialization::SaveCallback);
		serialization->SetLoadCallback(Serialization::LoadCallback);
		
		MHK::LoadSettings();

		Hooks_FavoritesHandler::InstallHooks();
		Hooks_FavoritesMenu::InstallHooks();
		_MESSAGE("Plugin successfully loaded.");

		return true;
	}
};
