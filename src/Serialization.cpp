#include "RE/TESDataHandler.h"
#include "RE/TESFile.h"

#include "Serialization.h"

#include "SKSE/API.h"

namespace Serialization
{
	std::vector<UInt32> SerializeHotkeys(std::list<Hotkey*> a_hotkeyList)
	{
		RE::TESDataHandler* dataHandler = RE::TESDataHandler::GetSingleton();

		std::vector<UInt32> serializedData;

		//Block length
		serializedData.push_back(a_hotkeyList.size());
		_MESSAGE("Serializing %d hotkeys...", a_hotkeyList.size());

		for (auto it = a_hotkeyList.begin(); it != a_hotkeyList.end(); ++it)
		{
			Hotkey* hotkey = *it;

			//Hotkey type
			serializedData.push_back((UInt32)(hotkey->type));

			//DeviceType
			serializedData.push_back(static_cast<UInt32>(hotkey->device));

			//KeyMask
			serializedData.push_back(hotkey->keyMask);

			//Hotkey data
			if (hotkey->type == Hotkey::Type::kItem)
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
		_MESSAGE("Successfully serialized %d hotkeys.", a_hotkeyList.size());

		return serializedData;
	}

	std::list<Hotkey*> DeserializeHotkeys(std::vector<UInt32> a_serializedData)
	{
		RE::TESDataHandler* dataHandler = RE::TESDataHandler::GetSingleton();

		UInt32 currentIndex = 0;
		std::list<Hotkey*> deserializedHotkeys;

		UInt32 blockSize = a_serializedData[currentIndex++];
		_MESSAGE("Expecting %d hotkeys.", blockSize);

		//Iterate through the hotkey entries
		for (UInt32 i = 0; i < blockSize; ++i)
		{
			_MESSAGE("Reading hotkey %d data...", i + 1);

			//Read hotkey type
			Hotkey::Type hotkeyType = (Hotkey::Type)(a_serializedData[currentIndex++]);

			//Read DeviceType
			RE::INPUT_DEVICE deviceType = static_cast<RE::INPUT_DEVICE>(a_serializedData[currentIndex++]);

			//Read KeyMask
			UInt32 keyMask = a_serializedData[currentIndex++];

			//Read hotkey data
			UInt32 hotkeyData = a_serializedData[currentIndex++];

			//Check values and create hotkey
			if (hotkeyType == Hotkey::Type::kMagic)
			{
				UInt32 formID = hotkeyData;
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

				if (form->formType != RE::FormType::Spell && form->formType != RE::FormType::Shout)
				{
					_ERROR("Form %.8x is not a spell or a shout (%x). Ignoring hotkey.", resolvedFormID, form->formType);
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

				MagicHotkey* hotkey = new MagicHotkey();

				hotkey->type = Hotkey::Type::kMagic;
				hotkey->form = form;
				hotkey->device = deviceType;
				hotkey->keyMask = keyMask;
				deserializedHotkeys.emplace_back(hotkey);
			}
			else if (hotkeyType == Hotkey::Type::kItem)
			{
				UInt32 extraDataId = hotkeyData;

				if (extraDataId >= 0xFF)
				{
					_ERROR("Invalid extra data id: %d. Ignoring hotkey.", extraDataId);
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

				ItemHotkey* hotkey = new ItemHotkey();

				hotkey->type = Hotkey::Type::kItem;
				hotkey->extraDataId = extraDataId;
				hotkey->device = deviceType;
				hotkey->keyMask = keyMask;
				deserializedHotkeys.emplace_back(hotkey);
			}
			else
			{
				_MESSAGE("Unknown hotkey type. Ignoring hotkey.");
				continue;
			}
			_MESSAGE("Hotkey %d successfully loaded.", i + 1);
		}

		_MESSAGE("Successfully loaded %d hotkeys", deserializedHotkeys.size());

		return deserializedHotkeys;
	}

	void SaveCallback(SKSE::SerializationInterface* a_intfc)
	{
		MHK::HotkeyManager* hotkeyManager = MHK::HotkeyManager::GetSingleton();

		std::list<Hotkey*> hotkeys = hotkeyManager->GetHotkeys();
		std::list<Hotkey*> vampireHotkeys = hotkeyManager->GetVampireHotkeys();

		std::vector<UInt32> serializedData = Serialization::SerializeHotkeys(hotkeys);
		std::vector<UInt32> serializedVampireData = Serialization::SerializeHotkeys(vampireHotkeys);

		if (!a_intfc->OpenRecord('VERS', 1)) {
			_ERROR("Failed to open record for serialized data!");
		}
		else {
			UInt32 version = SERIALIZATION_VERSION;
			if (!a_intfc->WriteRecordData(&version, sizeof(version))) {
				_ERROR("Failed to write version data!");
			}
		}

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

		std::list<Hotkey*> hotkeys;
		std::list<Hotkey*> vampireHotkeys;

		UInt32 type;
		UInt32 version;
		UInt32 length;

		bool success = true;

		while (a_intfc->GetNextRecordInfo(type, version, length)) {
			switch (type)
			{
				case 'VERS':
				{
					UInt32 version;
					if (!a_intfc->ReadRecordData(&version, sizeof(version))) {
						_ERROR("Failed to load version info!");
						success = false;
						break;
					}
					if (version != SERIALIZATION_VERSION)
					{
						_ERROR("Saved data is incompatible! Ignoring.");
						success = false;
						break;
					}
					break;
				}
				case 'KBHK':
				{
					for (UInt32 i = 0; i < length; i += sizeof(UInt32)) {
						UInt32 elem;
						if (!a_intfc->ReadRecordData(&elem, sizeof(elem))) {
							_ERROR("Failed to load hotkey data element!");
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
					for (UInt32 i = 0; i < length; i += sizeof(UInt32)) {
						UInt32 elem;
						if (!a_intfc->ReadRecordData(&elem, sizeof(elem))) {
							_ERROR("Failed to load hotkey data element!");
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
					_ERROR("Unrecognized signature type!");
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
			hotkeys = Serialization::DeserializeHotkeys(serializedData);
			_MESSAGE("Hotkeys loaded successfully.");
		}
		else
		{
			_MESSAGE("No saved hotkey data found.");
		}

		if (serializedVampireData.size() > 0)
		{
			vampireHotkeys = Serialization::DeserializeHotkeys(serializedVampireData);
			_MESSAGE("Vampire hotkeys loaded successfully.");
		}
		else
		{
			_MESSAGE("No saved vampire hotkey data found.");
		}
		hotkeyManager->SetHotkeys(hotkeys, vampireHotkeys);
	}
}