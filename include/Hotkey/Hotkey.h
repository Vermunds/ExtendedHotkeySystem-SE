#pragma once

namespace EHKS
{
	class Hotkey
	{
	public:
		enum class HotkeyType : std::uint32_t
		{
			kInvalid = 0,
			kItem = 1,
			kMagic = 2
		};

		RE::INPUT_DEVICE device = RE::INPUT_DEVICES::kNone;
		std::uint32_t keyMask = 0;
		HotkeyType type = HotkeyType::kInvalid;
	};
}