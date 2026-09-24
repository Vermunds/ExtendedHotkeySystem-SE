#include "Serialization.h"

namespace EHKS
{
	void SerializeHotkeys(const std::list<Hotkey*>& a_hotkeyList, std::vector<std::uint32_t>& a_serializedData)
	{
		//Block length
		a_serializedData.push_back(static_cast<std::uint32_t>(a_hotkeyList.size()));
		logger::info("Serializing {} hotkeys...", a_hotkeyList.size());

		for (auto it = a_hotkeyList.begin(); it != a_hotkeyList.end(); ++it)
		{
			Hotkey* hotkey = *it;

			//Hotkey type
			a_serializedData.push_back(static_cast<std::uint32_t>(hotkey->type));

			//DeviceType
			a_serializedData.push_back(static_cast<std::uint32_t>(hotkey->device));

			//KeyMask
			a_serializedData.push_back(hotkey->keyMask);

			//EquipMode
			a_serializedData.push_back(static_cast<std::uint32_t>(hotkey->equipMode));

			//Hotkey data
			if (hotkey->type == Hotkey::HotkeyType::kItem)
			{
				ItemHotkey* itemHotkey = static_cast<ItemHotkey*>(hotkey);
				a_serializedData.push_back(itemHotkey->extraDataId);
			}
			else
			{
				MagicHotkey* magicHotkey = static_cast<MagicHotkey*>(hotkey);
				a_serializedData.push_back(magicHotkey->form->formID);
			}
		}
		logger::info("Successfully serialized {} hotkeys.", a_hotkeyList.size());
	}

	bool DeserializeHotkeys(const std::vector<std::uint32_t>& a_serializedData, std::uint32_t& a_currentIndex, std::list<Hotkey*>& a_hotkeyList)
	{
		if (a_currentIndex >= a_serializedData.size())
		{
			logger::error("Serialized data ended unexpectedly.");
			return false;
		}

		std::uint32_t blockSize = a_serializedData[a_currentIndex++];
		logger::info("Expecting {} hotkeys.", blockSize);

		//Every entry is 5 elements long
		if (a_serializedData.size() - a_currentIndex < static_cast<std::size_t>(blockSize) * 5)
		{
			logger::error("Serialized data is too short for {} hotkeys.", blockSize);
			return false;
		}

		//Iterate through the hotkey entries
		for (std::uint32_t i = 0; i < blockSize; ++i)
		{
			logger::info("Reading hotkey {} data...", i + 1);

			//Read hotkey type
			Hotkey::HotkeyType hotkeyType = static_cast<Hotkey::HotkeyType>(a_serializedData[a_currentIndex++]);

			//Read DeviceType
			RE::INPUT_DEVICE deviceType = static_cast<RE::INPUT_DEVICE>(a_serializedData[a_currentIndex++]);

			//Read KeyMask
			std::uint32_t keyMask = a_serializedData[a_currentIndex++];

			//Read EquipMode
			std::uint32_t equipModeData = a_serializedData[a_currentIndex++];

			//Read hotkey data
			std::uint32_t hotkeyData = a_serializedData[a_currentIndex++];

			if (deviceType != RE::INPUT_DEVICE::kKeyboard && deviceType != RE::INPUT_DEVICE::kMouse && deviceType != RE::INPUT_DEVICE::kGamepad)
			{
				logger::error("Unknown device type: {}. Ignoring hotkey.", static_cast<std::int32_t>(deviceType));
				continue;
			}

			if (keyMask > 255)
			{
				logger::error("Unknown keymask: {}. Ignoring hotkey.", keyMask);
				continue;
			}

			if (equipModeData > static_cast<std::uint32_t>(Hotkey::EquipMode::kRight))
			{
				logger::error("Unknown equip mode: {}. Ignoring hotkey.", equipModeData);
				continue;
			}

			Hotkey::EquipMode equipMode = static_cast<Hotkey::EquipMode>(equipModeData);

			//Check values and create hotkey
			if (hotkeyType == Hotkey::HotkeyType::kMagic)
			{
				std::uint32_t formID = hotkeyData;
				RE::FormID resolvedFormID;
				if (!SKSE::GetSerializationInterface()->ResolveFormID(formID, resolvedFormID))
				{
					logger::error("Unable to resolve formID {:08X}. Ignoring hotkey.", formID);
					continue;
				}
				logger::info("Resolved formID {:08X} to {:08X}", formID, resolvedFormID);

				RE::TESForm* form = RE::TESForm::LookupByID(resolvedFormID);
				if (!form)
				{
					logger::error("Unable to lookup form {:08X}. Ignoring hotkey.", resolvedFormID);
					continue;
				}

				if (form->formType != RE::FormType::Spell && form->formType != RE::FormType::Shout)
				{
					logger::error("Form {:08X} is not a spell or a shout ({}). Ignoring hotkey.", resolvedFormID, form->formType.underlying());
					continue;
				}

				MagicHotkey* hotkey = new MagicHotkey();

				hotkey->type = Hotkey::HotkeyType::kMagic;
				hotkey->form = form;
				hotkey->device = deviceType;
				hotkey->keyMask = keyMask;
				hotkey->equipMode = equipMode;
				a_hotkeyList.emplace_back(hotkey);
			}
			else if (hotkeyType == Hotkey::HotkeyType::kItem)
			{
				std::uint32_t extraDataId = hotkeyData;

				if (extraDataId >= 0xFF)
				{
					logger::error("Invalid extra data id: {}. Ignoring hotkey.", extraDataId);
					continue;
				}

				ItemHotkey* hotkey = new ItemHotkey();

				hotkey->type = Hotkey::HotkeyType::kItem;
				hotkey->extraDataId = static_cast<std::uint8_t>(extraDataId);
				hotkey->device = deviceType;
				hotkey->keyMask = keyMask;
				hotkey->equipMode = equipMode;
				a_hotkeyList.emplace_back(hotkey);
			}
			else
			{
				logger::info("Unknown hotkey type. Ignoring hotkey.");
				continue;
			}
			logger::info("Hotkey {} successfully loaded.", i + 1);
		}

		logger::info("Successfully loaded {} hotkeys.", a_hotkeyList.size());

		return true;
	}

	void SaveCallback(SKSE::SerializationInterface* a_intfc)
	{
		HotkeyManager* hotkeyManager = HotkeyManager::GetSingleton();

		std::vector<std::uint32_t> serializedData;
		SerializeHotkeys(hotkeyManager->GetHotkeys(), serializedData);
		SerializeHotkeys(hotkeyManager->GetVampireHotkeys(), serializedData);

		if (!a_intfc->OpenRecord('EHS~', SERIALIZATION_VERSION))
		{
			logger::error("Failed to open record for serialized data!");
			return;
		}

		for (auto& elem : serializedData)
		{
			if (!a_intfc->WriteRecordData(&elem, sizeof(elem)))
			{
				logger::error("Failed to write data for serialized data element!");
				return;
			}
		}

		logger::info("Hotkeys saved successfully.");
	}

	void LoadCallback(SKSE::SerializationInterface* a_intfc)
	{
		HotkeyManager* hotkeyManager = HotkeyManager::GetSingleton();

		std::vector<std::uint32_t> serializedData;

		std::uint32_t type;
		std::uint32_t version;
		std::uint32_t length;

		bool success = true;

		while (a_intfc->GetNextRecordInfo(type, version, length))
		{
			if (type != 'EHS~')
			{
				logger::error("Unrecognized signature type!");
				success = false;
				break;
			}

			if (version != SERIALIZATION_VERSION)
			{
				logger::error("Saved data is incompatible! Ignoring.");
				success = false;
				break;
			}

			for (std::uint32_t i = 0; i < length; i += sizeof(std::uint32_t))
			{
				std::uint32_t elem;
				if (!a_intfc->ReadRecordData(&elem, sizeof(elem)))
				{
					logger::error("Failed to load hotkey data element!");
					success = false;
					break;
				}
				serializedData.push_back(elem);
			}

			if (!success)
			{
				break;
			}
		}

		std::list<Hotkey*> hotkeys;
		std::list<Hotkey*> vampireHotkeys;

		if (success && !serializedData.empty())
		{
			std::uint32_t currentIndex = 0;
			if (DeserializeHotkeys(serializedData, currentIndex, hotkeys) && DeserializeHotkeys(serializedData, currentIndex, vampireHotkeys))
			{
				logger::info("Hotkeys loaded successfully.");
			}
			else
			{
				success = false;
			}
		}
		else if (success)
		{
			logger::info("No saved hotkey data found.");
		}

		if (!success)
		{
			//Loading failed, ignore everything.
			for (Hotkey* hotkey : hotkeys)
			{
				delete hotkey;
			}
			for (Hotkey* hotkey : vampireHotkeys)
			{
				delete hotkey;
			}
			hotkeyManager->SetHotkeys(std::list<Hotkey*>(), std::list<Hotkey*>());
			return;
		}

		hotkeyManager->SetHotkeys(hotkeys, vampireHotkeys);
	}
}
