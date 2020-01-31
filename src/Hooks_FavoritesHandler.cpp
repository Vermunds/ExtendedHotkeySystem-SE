#include "skse64_common/SafeWrite.h"

#include "RE/Skyrim.h"

#include "SKSE/API.h"

#include "Offsets.h"
#include "HotkeyManager.h"
#include "Settings.h"

namespace Hooks_FavoritesHandler
{
	bool isUnequipping = false;

	uintptr_t ProcessButton_Original_ptr;

	bool IsPlayerVampire()
	{
		RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
		RE::BGSDefaultObjectManager* objManager = RE::BGSDefaultObjectManager::GetSingleton();
		RE::TESRace* vampireRace = static_cast<RE::TESRace*>(objManager->GetObject(RE::DEFAULT_OBJECT::kVampireRace));
		return player->race == vampireRace;
	}

	bool CanDualWield(RE::PlayerCharacter* a_player, RE::TESObjectWEAP* a_weapon)
	{
		if (a_weapon->IsOneHandedAxe() || a_weapon->IsOneHandedDagger() || a_weapon->IsOneHandedMace() || a_weapon->IsOneHandedSword())
		{
			RE::TESObjectREFR::InventoryItemMap inventory = a_player->GetInventory();

			RE::TESObjectREFR::InventoryItemMap::iterator it = inventory.begin();

			// Iterate over the map using iterator
			while (it != inventory.end())
			{
				if (it->first == static_cast<RE::TESBoundObject*>(a_weapon))
				{
					if (it->second.first >= 2)
					{
						return true;
					}
					else
					{
						return false;
					}
				}
				++it;
			}
		}
		return false;
	}

	void EquipItem(RE::TESForm* a_item)
	{
		RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
		RE::ActorEquipManager* em = RE::ActorEquipManager::GetSingleton();

		MHK::Settings* settings = MHK::Settings::GetSingleton();

		//Function definitions
		bool (*IsEquipped)(void*, void*, RE::Actor*, RE::TESForm*) = reinterpret_cast<bool (*)(void*, void*, RE::Actor*, RE::TESForm*)>(Offsets::IsEquipped.GetUIntPtr()); //Papyrus function

		RE::BGSEquipSlot* (*GetLeftHandEquipSlot)() = reinterpret_cast<RE::BGSEquipSlot * (*)()>(Offsets::GetSpellLeftEquipSlot.GetUIntPtr());
		RE::BGSEquipSlot* (*GetRightHandEquipSlot)() = reinterpret_cast<RE::BGSEquipSlot * (*)()>(Offsets::GetSpellRightEquipSlot.GetUIntPtr());

		void(*EquipSpell)(RE::ActorEquipManager*, RE::Actor*, RE::TESForm*, RE::BGSEquipSlot*) = reinterpret_cast<void(*)(RE::ActorEquipManager*, RE::Actor*, RE::TESForm*, RE::BGSEquipSlot*)>(Offsets::EquipSpell.GetUIntPtr()); //Member of EquipManager
		void(*EquipShout)(RE::ActorEquipManager*, RE::Actor*, RE::TESForm*) = reinterpret_cast<void(*)(RE::ActorEquipManager*, RE::Actor*, RE::TESForm*)>(Offsets::EquipShout.GetUIntPtr()); //Member of EquipManager

		void(*PlaySound)(const char*) = reinterpret_cast<void(*)(const char*)>(Offsets::PlaySound.GetUIntPtr());

		switch (a_item->formType)
		{
		case RE::FormType::Armor:
		{
			RE::TESObjectARMO* item = static_cast<RE::TESObjectARMO*>(a_item);

			if (IsEquipped(nullptr, nullptr, player, item))
			{
				em->UnequipItem(player, item, nullptr, 1, item->GetEquipSlot(), true, false, true, false);
			}
			else
			{
				em->EquipItem(player, a_item, nullptr, 1, item->GetEquipSlot(), true, false, true, false);
			}
			break;
		}
		case RE::FormType::Weapon:
		{
			RE::TESObjectWEAP* item = static_cast<RE::TESObjectWEAP*>(a_item);
			if (settings->dualWieldSupport && CanDualWield(player, item))
			{
				if (item == player->currentProcess->GetEquippedRightHand() && item == player->currentProcess->GetEquippedLeftHand())
				{
					//Unequip from left
					em->UnequipItem(player, item, nullptr, 1, GetLeftHandEquipSlot(), true, false, true, false);
					break;
				}
				else if (item == player->currentProcess->GetEquippedRightHand() && item != player->currentProcess->GetEquippedLeftHand())
				{
					//Equip to left hand
					em->EquipItem(player, item, nullptr, 1, GetLeftHandEquipSlot(), true, false, true, false);
					break;
				}

				//Not equipped to the right -> continue
			}

			if (item == player->currentProcess->GetEquippedLeftHand() || item == player->currentProcess->GetEquippedRightHand())
			{
				//Item already equipped
				em->UnequipItem(player, item, nullptr, 1, item->GetEquipSlot(), true, false, true, false);
			}
			else
			{
				em->EquipItem(player, item, nullptr, 1, item->GetEquipSlot(), true, false, true, false);
			}
			break;
		}
		case RE::FormType::Light:
		{
			RE::TESObjectLIGH* item = static_cast<RE::TESObjectLIGH*>(a_item);
			if (a_item == player->currentProcess->GetEquippedLeftHand() || a_item == player->currentProcess->GetEquippedRightHand())
			{
				//Item already equipped
				em->UnequipItem(player, item, nullptr, 1, item->GetEquipSlot(), true, false, true, false);
			}
			else
			{
				em->EquipItem(player, item, nullptr, 1, item->GetEquipSlot(), true, false, true, false);
			}
			break;
		}
		case RE::FormType::AlchemyItem:
		{
			RE::AlchemyItem* item = static_cast<RE::AlchemyItem*>(a_item);
			em->EquipItem(player, item, nullptr, 1, item->GetEquipSlot(), true, false, true, false);
			break;
		}
		case RE::FormType::Ingredient:
		{
			RE::IngredientItem* item = static_cast<RE::IngredientItem*>(a_item);
			em->EquipItem(player, item, nullptr, 1, item->GetEquipSlot(), true, false, true, false);
			break;
		}
		case RE::FormType::Spell:
		{
			RE::SpellItem* item = static_cast<RE::SpellItem*>(a_item);

			if (player->selectedSpells[RE::PlayerCharacter::SlotTypes::kLeftHand] != item)
			{
				//Equip spell to left hand
				EquipSpell(em, player, item, GetLeftHandEquipSlot());
			}
			else if (player->selectedSpells[RE::PlayerCharacter::SlotTypes::kRightHand] != item)
			{
				//Equip spell to right hand
				EquipSpell(em, player, item, GetRightHandEquipSlot());
			}
			return; //Nothing to equip
		}
		case RE::FormType::Shout:
		{
			RE::TESShout* item = static_cast<RE::TESShout*>(a_item);
			EquipShout(em, player, item);
			break;
		}
		case RE::FormType::Ammo:
		{
			RE::TESAmmo* item = static_cast<RE::TESAmmo*>(a_item);
			if (IsEquipped(nullptr, nullptr, player, item))
			{
				em->UnequipItem(player, a_item, nullptr, 1, nullptr, true, false, true, false);
			}
			else
			{
				em->EquipItem(player, a_item, nullptr, 1, nullptr, true, false, true, false);
			}
			break;
		}
		case RE::FormType::Scroll:
		{
			RE::ScrollItem* item = static_cast<RE::ScrollItem*>(a_item);
			em->EquipItem(player, a_item, nullptr, 1, nullptr, true, false, true, false);
			break;
		}
		}
		PlaySound("UIFavorite");
	}

	bool ProcessButton_Hook(RE::FavoritesHandler* a_this, RE::ButtonEvent* a_event)
	{
		using MHK::HotkeyManager;

		RE::UserEvents* inStr = RE::UserEvents::GetSingleton();

		RE::BSInputDeviceManager* inputDeviceManager = RE::BSInputDeviceManager::GetSingleton();

		if (inputDeviceManager->IsGamepadEnabled())
		{
			bool(*ProcessButton_Original)(RE::FavoritesHandler*, RE::ButtonEvent*);
			ProcessButton_Original = reinterpret_cast<bool(*)(RE::FavoritesHandler*, RE::ButtonEvent*)>(ProcessButton_Original_ptr);;
			return ProcessButton_Original(a_this, a_event);
		}

		if (a_event->userEvent == inStr->favorites)
		{
			RE::UIMessageQueue* messageQueue = RE::UIMessageQueue::GetSingleton();
			RE::UI* ui = RE::UI::GetSingleton();
			RE::InterfaceStrings* strHolder = RE::InterfaceStrings::GetSingleton();

			if (!ui->IsMenuOpen(strHolder->favoritesMenu))
			{
				messageQueue->AddMessage(strHolder->favoritesMenu, RE::UI_MESSAGE_TYPE::kShow, 0);
				return true;
			}
		}

		if (a_event->eventType == RE::INPUT_EVENT_TYPE::kButton)
		{
			HotkeyManager* hotkeyManager = HotkeyManager::GetSingleton();
			HotkeyManager::Hotkey* hotkey = hotkeyManager->GetHotkey(a_event->device, a_event->idCode, IsPlayerVampire());

			if (hotkey)
			{
				EquipItem(hotkey->item);
				return true;
			}
		}
		return false;
	}

	//Hooks the function that looks up if the button is a hotkey or not in FavoritesHandler::CanProcess, so we don't have to hook that
	UInt8 IsHotkey_Hook(RE::InputEvent* a_event)
	{
		RE::UI* ui = RE::UI::GetSingleton();
		if (ui->GameIsPaused())
		{
			return 0xFF; //false
		}

		RE::MenuControls* mc = RE::MenuControls::GetSingleton();
		if (mc->beastForm && !IsPlayerVampire())
		{
			return 0xFF; //false
		}

		RE::BSInputDeviceManager* idm = RE::BSInputDeviceManager::GetSingleton();

		if (idm->IsGamepadEnabled())
		{
			if (a_event->eventType == RE::INPUT_EVENT_TYPE::kButton && a_event->HasIDCode())
			{
				RE::UserEvents* userEvents = RE::UserEvents::GetSingleton();
				RE::ButtonEvent* evn = static_cast<RE::ButtonEvent*>(a_event);

				if (evn->userEvent == userEvents->favorites ||
					evn->userEvent == userEvents->hotkey1 || evn->userEvent == userEvents->hotkey2 ||
					evn->userEvent == userEvents->hotkey3 || evn->userEvent == userEvents->hotkey4 ||
					evn->userEvent == userEvents->hotkey5 || evn->userEvent == userEvents->hotkey6 ||
					evn->userEvent == userEvents->hotkey7 || evn->userEvent == userEvents->hotkey8)
				{
					return 1; //true
				}
			}
			return 0xFF; //false
		}

		if (a_event->eventType == RE::INPUT_EVENT_TYPE::kButton && a_event->HasIDCode())
		{
			RE::ButtonEvent* evn = static_cast<RE::ButtonEvent*>(a_event);

			if (evn->IsUp())
			{
				if (evn->userEvent == RE::UserEvents::GetSingleton()->favorites)
				{
					return 1; //true
				}

				using MHK::HotkeyManager;

				HotkeyManager* hotkeyManager = HotkeyManager::GetSingleton();
				HotkeyManager::Hotkey* hotkey = hotkeyManager->GetHotkey(evn->device, evn->idCode, IsPlayerVampire()); //TODO

				if (hotkey)
				{
					return 1; //true
				}
			}
		}
		return 0xFF; //false
	}

	void InstallHooks()
	{
		SKSE::GetTrampoline()->Write6Call(Offsets::FavoritesHandler_CanProcess_IsHotkey_Hook.GetUIntPtr(), (uintptr_t)IsHotkey_Hook);
		UInt8 codes[] = { 0x90, 0x90, 0x90, 0x90, 0x90 };
		SafeWriteBuf(Offsets::FavoritesHandler_CanProcess_IsHotkey_Hook.GetUIntPtr() + 0x6, codes, sizeof(codes));

		RelocAddr<uintptr_t*> ProcessButtonPtr = RE::Offset::FavoritesHandler::Vtbl + (0x5 * 0x8);

		ProcessButton_Original_ptr = *(reinterpret_cast<uintptr_t*>(ProcessButtonPtr.GetUIntPtr()));

		SafeWrite64(ProcessButtonPtr.GetUIntPtr(), (uintptr_t)ProcessButton_Hook);
	}
}