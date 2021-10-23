#include "Serialization.h"

namespace EHKS
{
	std::vector<std::uint32_t> SerializeHotkeys(std::list<Hotkey*> a_hotkeyList)
	{
		std::vector<std::uint32_t> serializedData;

		//Block length
		serializedData.push_back(static_cast<std::uint32_t>(a_hotkeyList.size()));
		SKSE::log::info("Serializing %d hotkeys...", a_hotkeyList.size());

		for (auto it = a_hotkeyList.begin(); it != a_hotkeyList.end(); ++it)
		{
			Hotkey* hotkey = *it;

			//Hotkey type
			serializedData.push_back(static_cast<std::uint32_t>(hotkey->type));

			//DeviceType
			serializedData.push_back(static_cast<std::uint32_t>(hotkey->device));

			//KeyMask
			serializedData.push_back(hotkey->keyMask);

			//Hotkey data
			if (hotkey->type == Hotkey::HotkeyType::kItem)
			{
				ItemHotkey* itemHotkey = static_cast<ItemHotkey*>(hotkey);
				serializedData.push_back(itemHotkey->extraDataId);
			}
			else
			{
				MagicHotkey* magicHotkey = static_cast<MagicHotkey*>(hotkey);
				serializedData.push_back(magicHotkey->form->formID);
			}

		}
		SKSE::log::info("Successfully serialized %d hotkeys.", a_hotkeyList.size());

		return serializedData;
	}

	std::list<Hotkey*> DeserializeHotkeys(std::vector<std::uint32_t> a_serializedData)
	{
		std::uint32_t currentIndex = 0;
		std::list<Hotkey*> deserializedHotkeys;

		std::uint32_t blockSize = a_serializedData[currentIndex++];
		SKSE::log::info("Expecting %d hotkeys.", blockSize);

		//Iterate through the hotkey entries
		for (std::uint32_t i = 0; i < blockSize; ++i)
		{
			SKSE::log::info("Reading hotkey %d data...", i + 1);

			//Read hotkey type
			Hotkey::HotkeyType hotkeyType = static_cast<Hotkey::HotkeyType>(a_serializedData[currentIndex++]);

			//Read DeviceType
			RE::INPUT_DEVICE deviceType = static_cast<RE::INPUT_DEVICE>(a_serializedData[currentIndex++]);

			//Read KeyMask
			std::uint32_t keyMask = a_serializedData[currentIndex++];

			//Read hotkey data
			std::uint32_t hotkeyData = a_serializedData[currentIndex++];

			//Check values and create hotkey
			if (hotkeyType == Hotkey::HotkeyType::kMagic)
			{
				std::uint32_t formID = hotkeyData;
				RE::FormID resolvedFormID;
				if (!SKSE::GetSerializationInterface()->ResolveFormID(formID, resolvedFormID))
				{
					SKSE::log::error("Unable to resolve formID %x. Ignoring hotkey.", formID);
					continue;
				}
				SKSE::log::info("Resolved formID %.8x to %.8x", formID, resolvedFormID);

				RE::TESForm* form = RE::TESForm::LookupByID(resolvedFormID);
				if (!form)
				{
					SKSE::log::error("Unable to lookup form %x. Ignoring hotkey.", resolvedFormID);
					continue;
				}

				if (form->formType != RE::FormType::Spell && form->formType != RE::FormType::Shout)
				{
					SKSE::log::error("Form %.8x is not a spell or a shout (%x). Ignoring hotkey.", resolvedFormID, form->formType.get());
					continue;
				}
				if (deviceType != RE::INPUT_DEVICE::kKeyboard && deviceType != RE::INPUT_DEVICE::kMouse && deviceType != RE::INPUT_DEVICE::kGamepad)
				{
					SKSE::log::error("Unknown device type: %d. Ignoring hotkey.", deviceType);
					continue;
				}

				if (keyMask > 255)
				{
					SKSE::log::error("Unknown keymask: %d. Ignoring hotkey", keyMask);
					continue;
				}

				MagicHotkey* hotkey = new MagicHotkey();

				hotkey->type = Hotkey::HotkeyType::kMagic;
				hotkey->form = form;
				hotkey->device = deviceType;
				hotkey->keyMask = keyMask;
				deserializedHotkeys.emplace_back(hotkey);
			}
			else if (hotkeyType == Hotkey::HotkeyType::kItem)
			{
				std::uint32_t extraDataId = hotkeyData;

				if (extraDataId >= 0xFF)
				{
					SKSE::log::error("Invalid extra data id: %d. Ignoring hotkey.", extraDataId);
					continue;
				}

				if (deviceType != RE::INPUT_DEVICE::kKeyboard && deviceType != RE::INPUT_DEVICE::kMouse && deviceType != RE::INPUT_DEVICE::kGamepad)
				{
					SKSE::log::error("Unknown device type: %d. Ignoring hotkey.", deviceType);
					continue;
				}

				if (keyMask > 255)
				{
					SKSE::log::error("Unknown keymask: %d. Ignoring hotkey", keyMask);
					continue;
				}

				ItemHotkey* hotkey = new ItemHotkey();

				hotkey->type = Hotkey::HotkeyType::kItem;
				hotkey->extraDataId = static_cast<std::uint8_t>(extraDataId);
				hotkey->device = deviceType;
				hotkey->keyMask = keyMask;
				deserializedHotkeys.emplace_back(hotkey);
			}
			else
			{
				SKSE::log::info("Unknown hotkey type. Ignoring hotkey.");
				continue;
			}
			SKSE::log::info("Hotkey %d successfully loaded.", i + 1);
		}

		SKSE::log::info("Successfully loaded %d hotkeys", deserializedHotkeys.size());

		return deserializedHotkeys;
	}

	void SaveCallback(SKSE::SerializationInterface* a_intfc)
	{
		HotkeyManager* hotkeyManager = HotkeyManager::GetSingleton();

		std::list<Hotkey*> hotkeys = hotkeyManager->GetHotkeys();
		std::list<Hotkey*> vampireHotkeys = hotkeyManager->GetVampireHotkeys();

		std::vector<std::uint32_t> serializedData = SerializeHotkeys(hotkeys);
		std::vector<std::uint32_t> serializedVampireData = SerializeHotkeys(vampireHotkeys);

		if (!a_intfc->OpenRecord('VERS', 1)) {
			SKSE::log::error("Failed to open record for serialized data!");
		}
		else {
			std::uint32_t version = SERIALIZATION_VERSION;
			if (!a_intfc->WriteRecordData(&version, sizeof(version))) {
				SKSE::log::error("Failed to write version data!");
			}
		}

		if (!a_intfc->OpenRecord('KBHK', 1)) {
			SKSE::log::error("Failed to open record for serialized data!");
		}
		else {
			for (auto& elem : serializedData) {
				if (!a_intfc->WriteRecordData(&elem, sizeof(elem))) {
					SKSE::log::error("Failed to write data for serialized data element!");
					break;
				}
			}	
		}
		if (!a_intfc->OpenRecord('VAMP', 1)) {
			SKSE::log::error("Failed to open record for serialized data!");
		}
		else {
			for (auto& elem : serializedVampireData) {
				if (!a_intfc->WriteRecordData(&elem, sizeof(elem))) {
					SKSE::log::error("Failed to write data for serialized data element!");
					break;
				}
			}
		}
		SKSE::log::info("Hotkeys saved successfully.");
	}

	void LoadCallback(SKSE::SerializationInterface* a_intfc)
	{
		HotkeyManager* hotkeyManager = HotkeyManager::GetSingleton();

		std::vector<std::uint32_t> serializedData;
		std::vector<std::uint32_t> serializedVampireData;

		std::list<Hotkey*> hotkeys;
		std::list<Hotkey*> vampireHotkeys;

		std::uint32_t type;
		std::uint32_t version;
		std::uint32_t length;

		bool success = true;

		while (a_intfc->GetNextRecordInfo(type, version, length)) {
			switch (type)
			{
				case 'VERS':
				{
					std::uint32_t versionData;
					if (!a_intfc->ReadRecordData(&versionData, sizeof(versionData))) {
						SKSE::log::error("Failed to load version info!");
						success = false;
						break;
					}
					if (versionData != SERIALIZATION_VERSION)
					{
						SKSE::log::error("Saved data is incompatible! Ignoring.");
						success = false;
						break;
					}
					break;
				}
				case 'KBHK':
				{
					for (std::uint32_t i = 0; i < length; i += sizeof(std::uint32_t)) {
						std::uint32_t elem;
						if (!a_intfc->ReadRecordData(&elem, sizeof(elem))) {
							SKSE::log::error("Failed to load hotkey data element!");
							success = false;
							break;
						}
						else {
							serializedData.push_back(elem);
						}
					}
					break;
				}
				case 'VAMP':
				{
					for (std::uint32_t i = 0; i < length; i += sizeof(std::uint32_t)) {
						std::uint32_t elem;
						if (!a_intfc->ReadRecordData(&elem, sizeof(elem))) {
							SKSE::log::error("Failed to load hotkey data element!");
							success = false;
							break;
						}
						else {
							serializedVampireData.push_back(elem);
						}
					}
					break;
				}
				default:
				{
					SKSE::log::error("Unrecognized signature type!");
					success = false;
					break;
				}
			}
		}

		if (!success)
		{
			//Loading failed, ignore everything.
			hotkeyManager->SetHotkeys(std::list<Hotkey*>(), std::list<Hotkey*>());
			return;
		}

		if (serializedData.size() > 0)
		{
			hotkeys = DeserializeHotkeys(serializedData);
			SKSE::log::info("Hotkeys loaded successfully.");
		}
		else
		{
			SKSE::log::info("No saved hotkey data found.");
		}

		if (serializedVampireData.size() > 0)
		{
			vampireHotkeys = DeserializeHotkeys(serializedVampireData);
			SKSE::log::info("Vampire hotkeys loaded successfully.");
		}
		else
		{
			SKSE::log::info("No saved vampire hotkey data found.");
		}
		hotkeyManager->SetHotkeys(hotkeys, vampireHotkeys);
	}
}
