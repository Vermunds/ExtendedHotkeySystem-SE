#include "RE/TESDataHandler.h"
#include "RE/TESFile.h"

#include "Serialization.h"

#include "SKSE/API.h"

namespace Serialization
{
	std::vector<UInt32> SerializeHotkeys(std::vector<Hotkey*> a_hotkeyList)
	{
		RE::TESDataHandler* dataHandler = RE::TESDataHandler::GetSingleton();

		std::vector<UInt32> serializedData;

		//Block length
		serializedData.push_back(a_hotkeyList.size());
		_MESSAGE("Serializing %d hotkeys...", a_hotkeyList.size());

		for (auto it = a_hotkeyList.begin(); it != a_hotkeyList.end(); ++it)
		{
			Hotkey* hotkey = *it;

			//FormID
			serializedData.push_back(hotkey->item->formID);

			//DeviceType
			serializedData.push_back(static_cast<UInt32>(hotkey->device));

			//KeyMask
			serializedData.push_back(hotkey->keyMask);

		}
		_MESSAGE("Successfully serialized %d hotkeys.", a_hotkeyList.size());

		return serializedData;
	}

	std::vector<Hotkey*> DeserializeHotkeys(std::vector<UInt32> a_serializedData)
	{
		RE::TESDataHandler* dataHandler = RE::TESDataHandler::GetSingleton();

		UInt32 currentIndex = 0;
		std::vector<Hotkey*> deserializedHotkeys;

		UInt32 blockSize = a_serializedData[currentIndex++];
		_MESSAGE("Expecting %d hotkeys.", blockSize);

		//Iterate through the hotkey entries
		for (UInt32 i = 0; i < blockSize; ++i)
		{
			_MESSAGE("Reading hotkey %d data...", i + 1);

			//Read FormID
			UInt32 formID = a_serializedData[currentIndex++];

			//Read DeviceType
			RE::INPUT_DEVICE deviceType = static_cast<RE::INPUT_DEVICE>(a_serializedData[currentIndex++]);

			//Read KeyMask
			UInt32 keyMask = a_serializedData[currentIndex++];
			_MESSAGE("FormID: %.8x, DeviceType: %d, Keymask: %d", formID, deviceType, keyMask);

			//Check values and create hotkey
			RE::FormID resolvedFormID;
			if (!SKSE::GetSerializationInterface()->ResolveFormID(formID, resolvedFormID))
			{
				_ERROR("Unable to resolve formID %x. Ignoring hotkey.", formID);
				continue;
			}
			_MESSAGE("Resolved formID %.8x to %.8x", formID, resolvedFormID);

			RE::TESForm* form = RE::TESForm::LookupByID(resolvedFormID);
			if (!form)
			{
				_ERROR("Unable to lookup form %x. Ignoring hotkey.", resolvedFormID);
				continue;
			}

			if (form->formType != RE::FormType::Armor && form->formType != RE::FormType::Weapon &&
				form->formType != RE::FormType::Light && form->formType != RE::FormType::AlchemyItem &&
				form->formType != RE::FormType::Ingredient && form->formType != RE::FormType::Spell &&
				form->formType != RE::FormType::Shout && form->formType != RE::FormType::Ammo &&
				form->formType != RE::FormType::Scroll)
			{
				_ERROR("Form %.8x has an unknown FormType (%x). Ignoring hotkey.", resolvedFormID, form->formType);
				continue;
			}

			if (deviceType != RE::INPUT_DEVICE::kKeyboard && deviceType != RE::INPUT_DEVICE::kMouse)
			{
				_ERROR("Unknown device type: %d. Ignoring hotkey.", deviceType);
				continue;
			}

			if (keyMask > 255)
			{
				_ERROR("Unknown keymask: %d. Ignoring hotkey", keyMask);
				continue;
			}

			Hotkey* hotkey = new Hotkey();

			hotkey->item = form;
			hotkey->device = deviceType;
			hotkey->keyMask = keyMask;
			deserializedHotkeys.emplace_back(hotkey);
			_MESSAGE("Hotkey %d successfully loaded.", i + 1);
		}

		_MESSAGE("Successfully loaded %d hotkeys", deserializedHotkeys.size());

		return deserializedHotkeys;
	}

	void SaveCallback(SKSE::SerializationInterface* a_intfc)
	{
		MHK::HotkeyManager* hotkeyManager = MHK::HotkeyManager::GetSingleton();

		std::vector<Hotkey*> hotkeys = hotkeyManager->GetHotkeys();
		std::vector<Hotkey*> vampireHotkeys = hotkeyManager->GetVampireHotkeys();
		std::vector<UInt32> serializedData = Serialization::SerializeHotkeys(hotkeys);
		std::vector<UInt32> serializedVampireData = Serialization::SerializeHotkeys(vampireHotkeys);

		if (!a_intfc->OpenRecord('KBHK', 1)) {
			_ERROR("Failed to open record for serialized data!");
		}
		else {
			for (auto& elem : serializedData) {
				if (!a_intfc->WriteRecordData(&elem, sizeof(elem))) {
					_ERROR("Failed to write data for serialized data element!");
					break;
				}
			}	
		}
		if (!a_intfc->OpenRecord('VAMP', 1)) {
			_ERROR("Failed to open record for serialized data!");
		}
		else {
			for (auto& elem : serializedVampireData) {
				if (!a_intfc->WriteRecordData(&elem, sizeof(elem))) {
					_ERROR("Failed to write data for serialized data element!");
					break;
				}
			}
		}
		_MESSAGE("Hotkeys saved successfully.");
	}

	void LoadCallback(SKSE::SerializationInterface* a_intfc)
	{
		MHK::HotkeyManager* hotkeyManager = MHK::HotkeyManager::GetSingleton();

		std::vector<UInt32> serializedData;
		std::vector<UInt32> serializedVampireData;

		UInt32 type;
		UInt32 version;
		UInt32 length;
		while (a_intfc->GetNextRecordInfo(type, version, length)) {
			switch (type)
			{
				case 'KBHK':
				{
					for (UInt32 i = 0; i < length; i += sizeof(UInt32)) {
						UInt32 elem;
						if (!a_intfc->ReadRecordData(&elem, sizeof(elem))) {
							_ERROR("Failed to load hotkey data element!");
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
					for (UInt32 i = 0; i < length; i += sizeof(UInt32)) {
						UInt32 elem;
						if (!a_intfc->ReadRecordData(&elem, sizeof(elem))) {
							_ERROR("Failed to load hotkey data element!");
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
					_ERROR("Unrecognized signature type!");
					break;
				}
			}
		}

		if (serializedData.size() > 0)
		{
			std::vector<Hotkey*> hotkeys = Serialization::DeserializeHotkeys(serializedData);
			hotkeyManager->SetHotkeys(hotkeys);

			_MESSAGE("Hotkeys loaded successfully.");
		}
		else
		{
			_MESSAGE("No saved hotkey data found.");
		}

		if (serializedVampireData.size() > 0)
		{
			std::vector<Hotkey*> hotkeys = Serialization::DeserializeHotkeys(serializedVampireData);
			hotkeyManager->SetVampireHotkeys(hotkeys);

			_MESSAGE("Vampire hotkeys loaded successfully.");
		}
		else
		{
			_MESSAGE("No saved vampire hotkey data found.");
		}
	}
}