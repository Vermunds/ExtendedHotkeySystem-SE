#pragma once

#include "HotkeyManager.h"

namespace EHKS
{
	constexpr std::uint32_t SERIALIZATION_VERSION = 4;

	void SerializeHotkeys(const std::list<Hotkey*>& a_hotkeyList, std::vector<std::uint32_t>& a_serializedData);
	bool DeserializeHotkeys(const std::vector<std::uint32_t>& a_serializedData, std::uint32_t& a_currentIndex, std::list<Hotkey*>& a_hotkeyList);

	void SaveCallback(SKSE::SerializationInterface* a_intfc);
	void LoadCallback(SKSE::SerializationInterface* a_intfc);
}
