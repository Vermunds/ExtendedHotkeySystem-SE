#pragma once

#include "HotkeyManager.h"
#include "SKSE/API.h"

#include <vector>
#include <list>

#define SERIALIZATION_VERSION 2

namespace Serialization
{
	using Hotkey = MHK::HotkeyManager::Hotkey;
	using ItemHotkey = MHK::HotkeyManager::ItemHotkey;
	using MagicHotkey = MHK::HotkeyManager::MagicHotkey;

	std::vector<UInt32> SerializeHotkeys(std::list<Hotkey*> a_hotkeyList);

	std::list<Hotkey*> DeserializeHotkeys(std::vector<UInt32> a_serializedData);

	void SaveCallback(SKSE::SerializationInterface* a_intfc);

	void LoadCallback(SKSE::SerializationInterface* a_intfc);

}