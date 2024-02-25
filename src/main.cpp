#include "Serialization.h"
#include "Settings.h"
#include "version.h"

#include "Hooks_FavoritesHandler.h"
#include "Hooks_FavoritesMenu.h"
#include "TaskQueue.h"

extern "C"
{
	DLLEXPORT SKSE::PluginVersionData SKSEPlugin_Version = []() {
		SKSE::PluginVersionData v{};
		v.PluginVersion(REL::Version{ Version::MAJOR, Version::MINOR, Version::PATCH, 0 });
		v.PluginName(Version::NAME);
		v.AuthorName(Version::AUTHOR);
		v.UsesAddressLibrary();
		v.UsesStructsPost629();
		v.CompatibleVersions({ SKSE::RUNTIME_SSE_1_6_640, SKSE::RUNTIME_SSE_LATEST_AE });
		return v;
	}();

	DLLEXPORT bool SKSEPlugin_Load(SKSE::LoadInterface* a_skse)
	{
		assert(SKSE::log::log_directory().has_value());
		auto path = SKSE::log::log_directory().value() / std::filesystem::path(Version::NAME.data() + ".log"s);
		auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path.string(), true);
		auto log = std::make_shared<spdlog::logger>("global log", std::move(sink));

		log->set_level(spdlog::level::trace);
		log->flush_on(spdlog::level::trace);

		spdlog::set_default_logger(std::move(log));
		spdlog::set_pattern("%g(%#): [%^%l%$] %v", spdlog::pattern_time_type::local);

		SKSE::log::info("{} v{} -({})", Version::FORMATTED_NAME, Version::STRING, __TIMESTAMP__);

		SKSE::AllocTrampoline(1 << 5, true);
		SKSE::Init(a_skse);

		auto serialization = SKSE::GetSerializationInterface();
		if (!serialization)
		{
			return false;
		}
		serialization->SetUniqueID('EHKS');
		serialization->SetSaveCallback(EHKS::SaveCallback);
		serialization->SetLoadCallback(EHKS::LoadCallback);

		EHKS::LoadSettings();
		SKSE::log::info("Settings loaded.");

		EHKS::LoadSettings();

		EHKS::FavoritesHandlerEx::InstallHook();
		EHKS::FavoritesMenuEx::InstallHook();
		EHKS::TaskQueue::InstallHook();

		SKSE::log::info("Hooks installed.");

		return true;
	}
};
