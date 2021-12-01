#pragma once
#include <SKSE/Impl/Stubs.h>

namespace EHKS
{
	class EquipTaskDelegate : public SKSE::detail::TaskDelegate
	{
	public:
		RE::TESForm* item;
		RE::ExtraDataList* extraData;

		virtual void Run() override;
		virtual void Dispose() override;
	};

	class FavoritesHandlerEx : public RE::FavoritesHandler
	{
	public:
		bool ProcessButton_Hook(RE::ButtonEvent* a_event);  // 05

		static void InstallHook();

		using ProcessButton_t = decltype(&RE::FavoritesHandler::ProcessButton);
		static inline REL::Relocation<ProcessButton_t> _ProcessButton;
	};
}
