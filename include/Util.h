#pragma once

namespace EHKS
{
	std::uint32_t GetGamepadIconIndex(std::uint32_t a_scanCode);
	bool IsVanillaHotkey(RE::BSFixedString a_userEvent);
}