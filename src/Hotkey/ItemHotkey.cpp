#include "Hotkey/ItemHotkey.h"

namespace EHKS
{
	RE::TESForm* ItemHotkey::GetBaseForm()
	{
		RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
		RE::TESObjectREFR::InventoryItemMap inventory = player->GetInventory();

		//Iterate inventory
		for (RE::TESObjectREFR::InventoryItemMap::iterator it = inventory.begin(); it != inventory.end(); ++it) {
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

					if (static_cast<std::uint8_t>(extraHotkey->hotkey.get()) != this->extraDataId)
					{
						//Hotkey does not match
						continue;
					}
					return entryData->object;
				}
			}
		}

		return nullptr;
	}

	RE::ExtraDataList* ItemHotkey::GetExtraData()
	{
		RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
		RE::TESObjectREFR::InventoryItemMap inventory = player->GetInventory();

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

					if (static_cast<std::uint8_t>(extraHotkey->hotkey.get()) != this->extraDataId)
					{
						//Hotkey does not match
						continue;
					}
					//Hotkey matches
					return extraDataEntry;
				}
			}
		}

		return nullptr;
	}
}