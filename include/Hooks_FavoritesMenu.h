#pragma once

namespace EHKS
{
	class FavoritesMenuEx : public RE::FavoritesMenu
	{
	public:
		RE::UI_MESSAGE_RESULTS ProcessMessage_Hook(RE::UIMessage& a_message); // 04
		void AdvanceMovie_Hook(float a_interval, std::uint32_t a_currentTime);  // 05

		bool CanProcess_Hook(RE::InputEvent* a_event); // 01
		bool ProcessButton_Hook(RE::ButtonEvent* a_event); // 05

		void UpdateHotkeyIcons(bool a_controllerMode);
		static void InstallHook();

		using AdvanceMovie_t = decltype(&RE::FavoritesMenu::AdvanceMovie);
		static inline REL::Relocation<AdvanceMovie_t> _AdvanceMovie;

		using ProcessMessage_t = decltype(&RE::FavoritesMenu::ProcessMessage);
		static inline REL::Relocation<ProcessMessage_t> _ProcessMessage;

		using CanProcess_t = decltype(&RE::FavoritesMenu::CanProcess);
		static inline REL::Relocation<CanProcess_t> _CanProcess;

		using ProcessButton_t = decltype(&RE::FavoritesMenu::ProcessButton);
		static inline REL::Relocation<ProcessButton_t> _ProcessButton;
	};
}

