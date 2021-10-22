#pragma once

#include "HotkeyManager.h"

#include <vector>
#include <list>

#define SERIALIZATION_VERSION 3

namespace EHKS
{
	std::vector<std::uint32_t> SerializeHotkeys(std::list<Hotkey*> a_hotkeyList);

	std::list<Hotkey*> DeserializeHotkeys(std::vector<std::uint32_t> a_serializedData);

	void SaveCallback(SKSE::SerializationInterface* a_intfc);

	void LoadCallback(SKSE::SerializationInterface* a_intfc);

}