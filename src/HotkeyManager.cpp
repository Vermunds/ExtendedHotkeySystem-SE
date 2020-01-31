#include "skse64/GameData.h"	//MagicFavorites
#include "RE/Skyrim.h"

#include "HotkeyManager.h"

namespace MHK
{
	using Hotkey = MHK::HotkeyManager::Hotkey;

	HotkeyManager* HotkeyManager::singleton = nullptr;

	HotkeyManager* HotkeyManager::GetSingleton()
	{
		if (singleton)
		{
			return singleton;
		}
		singleton = new HotkeyManager();
		return singleton;
	}

	void HotkeyManager::AddHotkey(Hotkey* a_hotkey, bool a_isVampire)
	{
		std::vector<Hotkey*>* hotkeyList;
		hotkeyList = a_isVampire ? &(this->vampireHotkeys) : &(this->hotkeys);

		auto it = hotkeyList->begin();
		while (it != hotkeyList->end())
		{
			Hotkey* hotkey = *it;

			if (hotkey)
			{
				if (hotkey->device == a_hotkey->device && hotkey->keyMask == a_hotkey->keyMask)
				{
					//_MESSAGE("Removing duplicate hotkey (deviceType: %d, keyMask: %d, formID: %.8x).", hotkey->device, hotkey->keyMask, hotkey->item->formID);
					it = hotkeyList->erase(it);
					continue;
				}
				else if (hotkey->item == a_hotkey->item)
				{
					//_MESSAGE("Removing duplicate hotkey (deviceType: %d, keyMask: %d, formID: %.8x).", hotkey->device, hotkey->keyMask, hotkey->item->formID);
					it = hotkeyList->erase(it);
					continue;
				}
			}
			++it;
		}
		hotkeyList->emplace_back(a_hotkey);
	}

	void HotkeyManager::RemoveHotkey(Hotkey* a_hotkey, bool a_isVampire)
	{
		std::vector<Hotkey*>* hotkeyList;

		hotkeyList = a_isVampire ? &(this->vampireHotkeys) : &(this->hotkeys);

		auto it = hotkeyList->begin();
		while (it != hotkeyList->end())
		{
			Hotkey* hotkey = *it;

			if (hotkey == a_hotkey)
			{
				hotkeyList->erase(it);
				return;
			}
			++it;
		}
	}

	Hotkey* HotkeyManager::GetHotkey(RE::TESForm* a_form, bool a_isVampire)
	{
		std::vector<Hotkey*>* hotkeyList;

		hotkeyList = a_isVampire ? &(this->vampireHotkeys) : &(this->hotkeys);

		auto it = hotkeyList->begin();
		while (it != hotkeyList->end())
		{
			Hotkey* hotkey = *it;

			if (hotkey->item == a_form)
			{
				if (!a_isVampire && IsFavorited(hotkey->item))
				{
					return hotkey;
				}
				else if (a_isVampire && IsVampireSpell(hotkey->item))
				{
					return hotkey;
				}
				else
				{
					//_MESSAGE("Removing hotkey (deviceType: %d, keyMask: %d, formID: %.8x). The player does not have the item or it is not favorited.", hotkey->device, hotkey->keyMask, hotkey->item->formID);
					hotkeyList->erase(it);
					return nullptr;
				}
			}
			++it;
		}
		return nullptr;
	}

	Hotkey* HotkeyManager::GetHotkey(RE::INPUT_DEVICE a_deviceType, UInt32 a_keyMask, bool a_isVampire)
	{
		std::vector<Hotkey*>* hotkeyList;

		hotkeyList = a_isVampire ? &(this->vampireHotkeys) : &(this->hotkeys);		

		auto it = hotkeyList->begin();
		while (it != hotkeyList->end())
		{
			Hotkey* hotkey = *it;

			if (hotkey->device == a_deviceType && hotkey->keyMask == a_keyMask)
			{
				if (!a_isVampire && IsFavorited(hotkey->item))
				{
					return hotkey;
				}
				else if (a_isVampire && IsVampireSpell(hotkey->item))
				{
					return hotkey;
				}
				else
				{
					//_MESSAGE("Removing hotkey (deviceType: %d, keyMask: %d, formID: %.8x). The player does not have the item or it is not favorited.", hotkey->device, hotkey->keyMask, hotkey->item->formID);
					hotkeyList->erase(it);
					return nullptr;
				}
			}
			++it;
		}
		return nullptr;
	}

	bool HotkeyManager::IsFavorited(RE::TESForm* a_form)
	{
		RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
		RE::BSSimpleList<RE::InventoryEntryData*>* entries = player->GetInventoryChanges()->entryList;

		if (a_form->Is(RE::FormType::Spell) || a_form->Is(RE::FormType::Shout))
		{
			//skse func
			MagicFavorites* magicFavorites = MagicFavorites::GetSingleton();

			return magicFavorites && magicFavorites->IsFavorited((TESForm*)a_form);
		}

		RE::BSSimpleList<RE::InventoryEntryData*>::iterator i;
		for (i = entries->begin(); i != entries->end(); ++i)
		{
			RE::InventoryEntryData* entry = *i;

			if (entry->object->formID == a_form->formID)
			{
				//Player have the item
				RE::BSSimpleList<RE::ExtraDataList*>* extraList = entry->extraLists;
				RE::BSSimpleList<RE::ExtraDataList*>::iterator j;
				for (j = extraList->begin(); j != extraList->end(); ++j)
				{
					RE::ExtraDataList* extraData = *j;

					if (extraData->HasType(RE::ExtraDataType::kHotkey))
					{
						//Item is favorited
						IsVampireSpell(a_form);
						return true;
					}
				}
			}
		}
		//Player does not have the item or it is not favorited
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

	std::vector<Hotkey*> HotkeyManager::GetHotkeys()
	{
		return this->hotkeys;
	}

	std::vector<Hotkey*> HotkeyManager::GetVampireHotkeys()
	{
		return this->vampireHotkeys;
	}

	void HotkeyManager::SetHotkeys(std::vector<Hotkey*> a_hotkeys)
	{
		this->hotkeys.clear();
		this->hotkeys = a_hotkeys;
	}
	void HotkeyManager::SetVampireHotkeys(std::vector<Hotkey*> a_hotkeys)
	{
		this->vampireHotkeys.clear();
		this->vampireHotkeys = a_hotkeys;
	}
}