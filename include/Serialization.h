#pragma once

#include "HotkeyManager.h"
#include "SKSE/API.h"

#include <vector>

namespace Serialization
{
	using Hotkey = MHK::HotkeyManager::Hotkey;

	std::vector<UInt32> SerializeHotkeys(std::vector<Hotkey*> a_hotkeyList);

	std::vector<Hotkey*> DeserializeHotkeys(std::vector<UInt32> a_serializedData);

	void SaveCallback(SKSE::SerializationInterface* a_intfc);

	void LoadCallback(SKSE::SerializationInterface* a_intfc);

}