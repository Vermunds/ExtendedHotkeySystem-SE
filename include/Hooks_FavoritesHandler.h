#pragma once

namespace EHKS
{
	class FavoritesHandlerEx : public RE::FavoritesHandler
	{
	public:
		bool ProcessButton_Hook(RE::ButtonEvent* a_event);	// 05

		static void InstallHook();

		using ProcessButton_t = decltype(&RE::FavoritesHandler::ProcessButton);
		static inline REL::Relocation<ProcessButton_t> _ProcessButton;
	};
}