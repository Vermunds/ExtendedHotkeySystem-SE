#include "skse64_common/SafeWrite.h"

#include "RE/Skyrim.h"

#include "SKSE/API.h"

#include "Offsets.h"
#include "HotkeyManager.h"
#include "Settings.h"
#include "Util.h"

namespace Hooks_FavoritesHandler
{
	uintptr_t ProcessButton_Original_ptr;

	bool IsPlayerVampire()
	{
		RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
		RE::BGSDefaultObjectManager* objManager = RE::BGSDefaultObjectManager::GetSingleton();
		RE::TESRace* vampireRace = static_cast<RE::TESRace*>(objManager->GetObject(RE::DEFAULT_OBJECT::kVampireRace));
		return player->race == vampireRace;
	}

	void EquipItem(RE::TESForm* a_item, RE::ExtraDataList* a_extraData)
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
				em->UnequipItem(player, item, a_extraData, 1, item->GetEquipSlot(), true, false, true, false);
			}
			else
			{
				em->EquipItem(player, a_item, a_extraData, 1, item->GetEquipSlot(), true, false, true, false);
			}
			break;
		}
		case RE::FormType::Weapon:
		{
			RE::TESObjectWEAP* item = static_cast<RE::TESObjectWEAP*>(a_item);
			if (item == player->currentProcess->GetEquippedLeftHand() || item == player->currentProcess->GetEquippedRightHand())
			{
				//Item already equipped
				em->UnequipItem(player, item, nullptr, 1, item->GetEquipSlot(), true, false, true, false);
			}
			else
			{
				em->EquipItem(player, item, a_extraData, 1, item->GetEquipSlot(), true, false, true, false);
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
				em->EquipItem(player, item, a_extraData, 1, item->GetEquipSlot(), true, false, true, false);
			}
			break;
		}
		case RE::FormType::AlchemyItem:
		{
			RE::AlchemyItem* item = static_cast<RE::AlchemyItem*>(a_item);
			em->EquipItem(player, item, a_extraData, 1, item->GetEquipSlot(), true, false, true, false);
			break;
		}
		case RE::FormType::Ingredient:
		{
			RE::IngredientItem* item = static_cast<RE::IngredientItem*>(a_item);
			em->EquipItem(player, item, a_extraData, 1, item->GetEquipSlot(), true, false, true, false);
			break;
		}
		case RE::FormType::Spell:
		{
			RE::SpellItem* item = static_cast<RE::SpellItem*>(a_item);
			if (item->IsTwoHanded())
			{
				EquipSpell(em, player, item, item->GetEquipSlot());
			}
			else
			{
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
				em->EquipItem(player, a_item, a_extraData, 1, nullptr, true, false, true, false);
			}
			break;
		}
		case RE::FormType::Scroll:
		{
			RE::ScrollItem* item = static_cast<RE::ScrollItem*>(a_item);
			em->EquipItem(player, a_item, a_extraData, 1, nullptr, true, false, true, false);
			break;
		}
		}
		PlaySound("UIFavorite");
	}

	bool ProcessButton_Hook(RE::FavoritesHandler* a_this, RE::ButtonEvent* a_event)
	{
		using MHK::HotkeyManager;

		RE::UserEvents* userEvents = RE::UserEvents::GetSingleton();

		RE::BSInputDeviceManager* inputDeviceManager = RE::BSInputDeviceManager::GetSingleton();

		if (a_event->userEvent == userEvents->favorites)
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
			HotkeyManager::Hotkey* hotkey = nullptr;
			
			if (a_event->device == RE::INPUT_DEVICE::kGamepad && !MHK::IsVanillaHotkey(a_event->userEvent))
			{
				return false;
			}

			bool isVampire = IsPlayerVampire();
			isVampire ? hotkey = hotkeyManager->GetVampireHotkey(a_event->device, a_event->idCode) : hotkey = hotkeyManager->GetHotkey(a_event->device, a_event->idCode);

			if (hotkey)
			{
				//Update hotkeys and check again
				hotkeyManager->UpdateHotkeys();

				if (isVampire ? hotkey = hotkeyManager->GetVampireHotkey(a_event->device, a_event->idCode) : hotkey = hotkeyManager->GetHotkey(a_event->device, a_event->idCode))
				{
					if (hotkey->type == HotkeyManager::Hotkey::Type::kItem)
					{
						HotkeyManager::ItemHotkey* itemHotkey = static_cast<HotkeyManager::ItemHotkey*>(hotkey);
						RE::TESForm* baseForm = hotkeyManager->GetBaseForm(itemHotkey);
						if (baseForm)
						{
							RE::ExtraDataList* extraData = hotkeyManager->GetHotkeyData(itemHotkey);
							EquipItem(baseForm, extraData);
							return true;
						}
					}
					else
					{
						HotkeyManager::MagicHotkey* magicHotkey = static_cast<HotkeyManager::MagicHotkey*>(hotkey);
						EquipItem(magicHotkey->form, nullptr);
						return true;
					}
				}
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

		if (idm->IsGamepadEnabled() && a_event->device == RE::INPUT_DEVICE::kGamepad)
		{
			if (a_event->eventType == RE::INPUT_EVENT_TYPE::kButton && a_event->HasIDCode())
			{
				RE::UserEvents* userEvents = RE::UserEvents::GetSingleton();
				RE::ButtonEvent* evn = static_cast<RE::ButtonEvent*>(a_event);

				if (MHK::IsVanillaHotkey(evn->userEvent) && evn->IsUp())
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
				HotkeyManager::Hotkey* hotkey = hotkeyManager->GetHotkey(evn->device, evn->idCode);

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