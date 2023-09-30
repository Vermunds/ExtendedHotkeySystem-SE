#include "HotkeyManager.h"

namespace EHKS
{
	void HotkeyManager::SetHotkeyExtraData(RE::InventoryEntryData* a_entryData, std::uint8_t a_id)
	{
		//Remove all existing hotkey extra data
		for (RE::BSSimpleList<RE::ExtraDataList*>::iterator it = a_entryData->extraLists->begin(); it != a_entryData->extraLists->end(); ++it)
		{
			RE::ExtraDataList* extraDataEntry = *it;
			if (extraDataEntry->HasType(RE::ExtraDataType::kHotkey))
			{
				RE::ExtraHotkey* extraHotkey = static_cast<RE::ExtraHotkey*>(extraDataEntry->GetByType(RE::ExtraDataType::kHotkey));
				extraHotkey->hotkey = RE::ExtraHotkey::Hotkey::kUnbound;
			}
		}

		//Attach the extradata to the first one.
		if (a_entryData->extraLists->front()->HasType(RE::ExtraDataType::kHotkey))
		{
			RE::ExtraHotkey* extraHotkey = static_cast<RE::ExtraHotkey*>(a_entryData->extraLists->front()->GetByType(RE::ExtraDataType::kHotkey));
			extraHotkey->hotkey = static_cast<RE::ExtraHotkey::Hotkey>(a_id);
		}
		else
		{
			RE::ExtraHotkey* extraHotkey = RE::BSExtraData::Create<RE::ExtraHotkey>();
			extraHotkey->hotkey = static_cast<RE::ExtraHotkey::Hotkey>(a_id);
			a_entryData->extraLists->front()->Add(extraHotkey);
		}
	}

	Hotkey::HotkeyType HotkeyManager::GetHotkeyType(RE::TESForm* a_form)
	{
		using FormType = RE::FormType;

		switch (a_form->formType.get())
		{
		case (FormType::Spell):
		case (FormType::Shout):
			return Hotkey::HotkeyType::kMagic;
		default:
			return Hotkey::HotkeyType::kItem;
		}
	}

	HotkeyManager* HotkeyManager::GetSingleton()
	{
		static HotkeyManager singleton;
		return &singleton;
	}

	std::uint8_t HotkeyManager::UpdateHotkeys()
	{
		RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
		RE::TESObjectREFR::InventoryItemMap inventory = player->GetInventory();

		std::uint8_t result = 0;
		std::vector<std::uint8_t> occupiedSlots;
		std::list<RE::InventoryEntryData*> hotkeyedItems;

		//Iterate inventory
		for (RE::TESObjectREFR::InventoryItemMap::iterator it = inventory.begin(); it != inventory.end(); ++it)
		{
			RE::InventoryEntryData* entryData = it->second.second.get();

			if (!entryData->extraLists)
			{
				continue;
			}

			//Iterate InventoryEntryData->extraLists
			for (RE::BSSimpleList<RE::ExtraDataList*>::iterator it2 = entryData->extraLists->begin(); it2 != entryData->extraLists->end(); ++it2)
			{
				RE::ExtraDataList* extraDataEntry = *it2;

				if (extraDataEntry->HasType(RE::ExtraDataType::kHotkey))
				{
					RE::ExtraHotkey* extraHotkey = static_cast<RE::ExtraHotkey*>(extraDataEntry->GetByType(RE::ExtraDataType::kHotkey));

					if (extraHotkey->hotkey == RE::ExtraHotkey::Hotkey::kUnbound)
					{
						continue;
					}
					else
					{
						hotkeyedItems.emplace_back(entryData);
						break;
					}
				}
			}
		}

		//Iterate hotkeys
		std::list<Hotkey*>::iterator it = this->hotkeys.begin();
		while (it != this->hotkeys.end())
		{
			Hotkey* hotkey = *it;

			bool isHotkeyValid = false;

			//Item hotkeys
			if (hotkey->type == Hotkey::HotkeyType::kItem)
			{
				ItemHotkey* itemHotkey = static_cast<ItemHotkey*>(hotkey);

				//Iterate hotkeyedItems
				for (std::list<RE::InventoryEntryData*>::iterator it2 = hotkeyedItems.begin(); it2 != hotkeyedItems.end(); ++it2)
				{
					RE::InventoryEntryData* entryData = *it2;

					bool found = false;

					//Iterate InventoryEntryData->extraLists
					for (RE::BSSimpleList<RE::ExtraDataList*>::iterator it3 = entryData->extraLists->begin(); it3 != entryData->extraLists->end(); ++it3)
					{
						RE::ExtraDataList* extraDataEntry = *it3;

						if (extraDataEntry->HasType(RE::ExtraDataType::kHotkey))
						{
							RE::ExtraHotkey* extraHotkey = static_cast<RE::ExtraHotkey*>(extraDataEntry->GetByType(RE::ExtraDataType::kHotkey));
							if (extraHotkey->hotkey != RE::ExtraHotkey::Hotkey::kUnbound)
							{
								if (!found)
								{
									found = true;
									if (itemHotkey->extraDataId == static_cast<std::uint8_t>(extraHotkey->hotkey.get()))
									{
										//Hotkey matches!
										isHotkeyValid = true;
										occupiedSlots.emplace_back(itemHotkey->extraDataId);
									}
								}
								else
								{
									//Duplicate data, invalidate it
									extraHotkey->hotkey = RE::ExtraHotkey::Hotkey::kUnbound;
								}
							}
						}
					}
				}
			}
			//Magic hotkeys
			else if (hotkey->type == Hotkey::HotkeyType::kMagic)
			{
				MagicHotkey* magicHotkey = static_cast<MagicHotkey*>(hotkey);

				if (IsMagicFavorited(magicHotkey->form))
				{
					isHotkeyValid = true;
				}
			}

			if (!isHotkeyValid)
			{
				//Invalid hotkey, remove it
				it = this->hotkeys.erase(it);
				continue;
			}
			++it;
		}

		//Check vampire hotkeys
		std::list<MagicHotkey*>::iterator vampire_it = this->vampireHotkeys.begin();
		while (vampire_it != this->vampireHotkeys.end())
		{
			MagicHotkey* hotkey = *vampire_it;
			if (!IsVampireSpell(hotkey->form))
			{
				vampire_it = this->vampireHotkeys.erase(vampire_it);
				continue;
			}
			++vampire_it;
		}

		std::sort(occupiedSlots.begin(), occupiedSlots.end());

		//Get free slot
		for (std::vector<std::uint8_t>::iterator it2 = occupiedSlots.begin(); it2 != occupiedSlots.end(); ++it2)
		{
			std::uint8_t value = *it2;

			if (result == 0xFF)
			{
				//No space left
				return result;
			}

			if (result == value)
			{
				++result;
				continue;
			}
		}
		return result;
	}

	ItemHotkey* HotkeyManager::GetItemHotkey(RE::InventoryEntryData* a_entryData)
	{
		if (!a_entryData->extraLists)
		{
			return nullptr;
		}

		//Iterate hotkeys
		for (std::list<Hotkey*>::iterator it = this->hotkeys.begin(); it != this->hotkeys.end(); ++it)
		{
			Hotkey* hotkey = *it;

			if (hotkey->type == Hotkey::HotkeyType::kItem)
			{
				ItemHotkey* itemHotkey = static_cast<ItemHotkey*>(hotkey);

				//Iterate InventoryEntryData->extraLists
				for (RE::BSSimpleList<RE::ExtraDataList*>::iterator it2 = a_entryData->extraLists->begin(); it2 != a_entryData->extraLists->end(); ++it2)
				{
					RE::ExtraDataList* extraDataEntry = *it2;

					if (extraDataEntry->HasType(RE::ExtraDataType::kHotkey))
					{
						RE::ExtraHotkey* extraHotkey = static_cast<RE::ExtraHotkey*>(extraDataEntry->GetByType(RE::ExtraDataType::kHotkey));

						if (itemHotkey->extraDataId == static_cast<std::uint8_t>(extraHotkey->hotkey.get()))
						{
							//Hotkey matches
							return itemHotkey;
						}
					}
				}
			}
		}
		return nullptr;
	}

	MagicHotkey* HotkeyManager::GetMagicHotkey(RE::TESForm* a_form)
	{
		for (std::list<Hotkey*>::iterator it = this->hotkeys.begin(); it != this->hotkeys.end(); ++it)
		{
			Hotkey* hotkey = *it;
			if (hotkey->type == Hotkey::HotkeyType::kMagic)
			{
				MagicHotkey* magicHotkey = static_cast<MagicHotkey*>(hotkey);
				if (magicHotkey->form == a_form)
				{
					return magicHotkey;
				}
			}
		}
		return nullptr;
	}

	MagicHotkey* HotkeyManager::GetVampireHotkey(RE::TESForm* a_form)
	{
		for (std::list<MagicHotkey*>::iterator it = this->vampireHotkeys.begin(); it != this->vampireHotkeys.end(); ++it)
		{
			MagicHotkey* hotkey = *it;
			if (hotkey->form == a_form)
			{
				return hotkey;
			}
		}
		return nullptr;
	}

	Hotkey* HotkeyManager::GetHotkey(RE::INPUT_DEVICE a_deviceType, std::uint32_t a_keyMask)
	{
		for (std::list<Hotkey*>::iterator it = this->hotkeys.begin(); it != this->hotkeys.end(); ++it)
		{
			Hotkey* hotkey = *it;
			if (hotkey->device == a_deviceType && hotkey->keyMask == a_keyMask)
			{
				return hotkey;
			}
		}
		return nullptr;
	}

	MagicHotkey* HotkeyManager::GetVampireHotkey(RE::INPUT_DEVICE a_deviceType, std::uint32_t a_keyMask)
	{
		for (std::list<MagicHotkey*>::iterator it = this->vampireHotkeys.begin(); it != this->vampireHotkeys.end(); ++it)
		{
			MagicHotkey* hotkey = *it;
			if (hotkey->device == a_deviceType && hotkey->keyMask == a_keyMask)
			{
				return hotkey;
			}
		}
		return nullptr;
	}

	bool HotkeyManager::RemoveHotkey(RE::INPUT_DEVICE a_deviceType, std::uint32_t a_keyMask)
	{
		for (std::list<Hotkey*>::iterator it = this->hotkeys.begin(); it != this->hotkeys.end(); ++it)
		{
			Hotkey* hotkey = *it;
			if (hotkey->device == a_deviceType && hotkey->keyMask == a_keyMask)
			{
				if (hotkey->type == Hotkey::HotkeyType::kItem)
				{
					RE::ExtraDataList* extraData = static_cast<ItemHotkey*>(hotkey)->GetExtraData();
					if (extraData)
					{
						RE::ExtraHotkey* extraHotkey = static_cast<RE::ExtraHotkey*>(extraData->GetByType(RE::ExtraDataType::kHotkey));
						if (extraHotkey)
						{
							extraHotkey->hotkey = RE::ExtraHotkey::Hotkey::kUnbound;  //0xFF
						}
					}
				}
				this->hotkeys.erase(it);
				return true;
			}
		}
		return false;
	}

	bool HotkeyManager::RemoveVampireHotkey(RE::INPUT_DEVICE a_deviceType, std::uint32_t a_keyMask)
	{
		for (std::list<MagicHotkey*>::iterator it = this->vampireHotkeys.begin(); it != this->vampireHotkeys.end(); ++it)
		{
			Hotkey* hotkey = *it;
			if (hotkey->device == a_deviceType && hotkey->keyMask == a_keyMask)
			{
				this->vampireHotkeys.erase(it);
				return true;
			}
		}
		return false;
	}

	ItemHotkey* HotkeyManager::AddItemHotkey(RE::INPUT_DEVICE a_deviceType, std::uint32_t a_keyMask, RE::InventoryEntryData* a_entryData)
	{
		std::uint8_t slot = UpdateHotkeys();
		if (slot == 0xFF)
		{
			return nullptr;
		}

		ItemHotkey* existingItemHotkey = GetItemHotkey(a_entryData);
		if (existingItemHotkey)
		{
			if (existingItemHotkey->device == a_deviceType && existingItemHotkey->keyMask == a_keyMask)
			{
				//Unassigning, don't create a new hotkey;
				RemoveHotkey(existingItemHotkey->device, existingItemHotkey->keyMask);
				return nullptr;
			}
			RemoveHotkey(existingItemHotkey->device, existingItemHotkey->keyMask);
		}

		Hotkey* existingHotkey = GetHotkey(a_deviceType, a_keyMask);
		if (existingHotkey)
		{
			RemoveHotkey(existingHotkey->device, existingHotkey->keyMask);
		}

		ItemHotkey* hotkey = new ItemHotkey();
		hotkey->device = a_deviceType;
		hotkey->keyMask = a_keyMask;
		hotkey->type = Hotkey::HotkeyType::kItem;
		hotkey->extraDataId = slot;
		SetHotkeyExtraData(a_entryData, slot);
		this->hotkeys.emplace_back(hotkey);

		return hotkey;
	}

	MagicHotkey* HotkeyManager::AddMagicHotkey(RE::INPUT_DEVICE a_deviceType, std::uint32_t a_keyMask, RE::TESForm* a_form)
	{
		UpdateHotkeys();
		MagicHotkey* existingMagicHotkey = GetMagicHotkey(a_form);
		if (existingMagicHotkey)
		{
			if (existingMagicHotkey->device == a_deviceType && existingMagicHotkey->keyMask == a_keyMask)
			{
				//Unassigning, don't create a new hotkey;
				RemoveHotkey(existingMagicHotkey->device, existingMagicHotkey->keyMask);
				return nullptr;
			}
			RemoveHotkey(existingMagicHotkey->device, existingMagicHotkey->keyMask);
		}

		Hotkey* existingHotkey = GetHotkey(a_deviceType, a_keyMask);
		if (existingHotkey)
		{
			RemoveHotkey(existingHotkey->device, existingHotkey->keyMask);
		}

		MagicHotkey* hotkey = new MagicHotkey();
		hotkey->device = a_deviceType;
		hotkey->keyMask = a_keyMask;
		hotkey->type = Hotkey::HotkeyType::kMagic;
		hotkey->form = a_form;
		this->hotkeys.emplace_back(hotkey);

		return hotkey;
	}

	MagicHotkey* HotkeyManager::AddVampireHotkey(RE::INPUT_DEVICE a_deviceType, std::uint32_t a_keyMask, RE::TESForm* a_form)
	{
		UpdateHotkeys();
		if (!IsVampireSpell(a_form))
		{
			return nullptr;
		}

		MagicHotkey* existingVampireHotkey = GetVampireHotkey(a_form);
		if (existingVampireHotkey)
		{
			if (existingVampireHotkey->device == a_deviceType && existingVampireHotkey->keyMask == a_keyMask)
			{
				//Unassigning, don't create a new hotkey;
				RemoveVampireHotkey(existingVampireHotkey->device, existingVampireHotkey->keyMask);
				return nullptr;
			}
			RemoveVampireHotkey(existingVampireHotkey->device, existingVampireHotkey->keyMask);
		}

		Hotkey* existingHotkey = GetVampireHotkey(a_deviceType, a_keyMask);
		if (existingHotkey)
		{
			RemoveVampireHotkey(existingHotkey->device, existingHotkey->keyMask);
		}

		MagicHotkey* hotkey = new MagicHotkey();
		hotkey->device = a_deviceType;
		hotkey->keyMask = a_keyMask;
		hotkey->type = Hotkey::HotkeyType::kMagic;
		hotkey->form = a_form;
		this->vampireHotkeys.emplace_back(hotkey);

		return hotkey;
	}

	bool HotkeyManager::IsMagicFavorited(RE::TESForm* a_form)
	{
		RE::MagicFavorites* magicFavorites = RE::MagicFavorites::GetSingleton();

		for (RE::BSTArray<RE::TESForm*>::iterator it2 = magicFavorites->spells.begin(); it2 != magicFavorites->spells.end(); ++it2)
		{
			RE::TESForm* form = *it2;

			if (form == a_form)
			{
				return true;
			}
		}
		return false;
	}

	bool HotkeyManager::IsVampireSpell(RE::TESForm* a_form)
	{
		if (a_form->Is(RE::FormType::Spell))
		{
			RE::BGSDefaultObjectManager* objManager = RE::BGSDefaultObjectManager::GetSingleton();
			RE::BGSListForm* formList = static_cast<RE::BGSListForm*>(objManager->GetObject(RE::DEFAULT_OBJECT::kVampireSpells));
			return formList->HasForm(a_form);
		}
		return false;
	}

	std::list<Hotkey*> HotkeyManager::GetHotkeys()
	{
		return this->hotkeys;
	}

	std::list<Hotkey*> HotkeyManager::GetVampireHotkeys()
	{
		std::list<Hotkey*> hotkeys;
		for (std::list<MagicHotkey*>::iterator it = this->vampireHotkeys.begin(); it != this->vampireHotkeys.end(); ++it)
		{
			Hotkey* hotkey = *it;
			hotkeys.push_back(static_cast<Hotkey*>(hotkey));
		}
		return hotkeys;
	}

	void HotkeyManager::SetHotkeys(std::list<Hotkey*> a_hotkeys, std::list<Hotkey*> a_vampireHotkeys)
	{
		this->hotkeys = a_hotkeys;
		std::list<MagicHotkey*> vampireHotkeys;

		for (std::list<Hotkey*>::iterator it = a_vampireHotkeys.begin(); it != a_vampireHotkeys.end(); ++it)
		{
			Hotkey* hotkey = *it;
			if (hotkey->type == Hotkey::HotkeyType::kMagic)
			{
				vampireHotkeys.push_back(static_cast<MagicHotkey*>(hotkey));
			}
		}

		this->vampireHotkeys = vampireHotkeys;

		UpdateHotkeys();
	}
}
