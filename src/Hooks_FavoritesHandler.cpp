#include "Hooks_FavoritesHandler.h"

#include "HotkeyManager.h"
#include "Settings.h"
#include "TaskQueue.h"
#include "Util.h"

namespace EHKS
{
	bool IsPlayerVampire()
	{
		RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
		RE::BGSDefaultObjectManager* objManager = RE::BGSDefaultObjectManager::GetSingleton();
		if (objManager->IsInitialized())
		{
			RE::TESRace* vampireRace = static_cast<RE::TESRace*>(objManager->GetObject(RE::DEFAULT_OBJECT::kVampireRace));
			return player->GetRace() == vampireRace;
		}
		return false;
	}

	bool IsEquipped(RE::TESForm* a_form)
	{
		// The function got inlined in AE, call the papyrus function instead
		// The first two args are not used, unless a_form is null (make sure it isn't!)
		if (a_form)
		{
			using func_t = std::uint64_t (*)(void*, std::uint32_t, RE::Actor*, RE::TESForm*);
			REL::Relocation<func_t> func(REL::ID{ 54707 });
			return func(nullptr, 0, RE::PlayerCharacter::GetSingleton(), a_form);
		}
		return false;
	}

	//Hooks the function that looks up if the button is a hotkey or not in FavoritesHandler::CanProcess, so we don't have to hook that
	std::uint8_t IsHotkey_Hook(RE::InputEvent* a_event)
	{
		RE::UI* ui = RE::UI::GetSingleton();
		if (ui->GameIsPaused())
		{
			return 0xFF;  //false
		}

		RE::MenuControls* mc = RE::MenuControls::GetSingleton();
		if (mc->beastForm && !IsPlayerVampire())
		{
			return 0xFF;  //false
		}

		RE::BSInputDeviceManager* idm = RE::BSInputDeviceManager::GetSingleton();

		if (idm->IsGamepadEnabled() && a_event->device == RE::INPUT_DEVICE::kGamepad)
		{
			if (a_event->eventType == RE::INPUT_EVENT_TYPE::kButton && a_event->HasIDCode())
			{
				RE::ButtonEvent* evn = static_cast<RE::ButtonEvent*>(a_event);

				if (EHKS::IsVanillaHotkey(evn->userEvent) && evn->IsUp())
				{
					return 1;  //true
				}
			}
			return 0xFF;  //false
		}

		if (a_event->eventType == RE::INPUT_EVENT_TYPE::kButton && a_event->HasIDCode())
		{
			RE::ButtonEvent* evn = static_cast<RE::ButtonEvent*>(a_event);

			if (evn->IsUp())
			{
				if (evn->userEvent == RE::UserEvents::GetSingleton()->favorites)
				{
					return 1;  //true
				}

				using EHKS::HotkeyManager;

				HotkeyManager* hotkeyManager = HotkeyManager::GetSingleton();
				Hotkey* hotkey = hotkeyManager->GetHotkey(evn->device.get(), evn->idCode);

				if (hotkey)
				{
					return 1;  //true
				}
			}
		}
		return 0xFF;  //false
	}

	void EquipItem(RE::TESForm* a_item, RE::ExtraDataList* a_extraData)
	{
		auto task = [a_item, a_extraData]() {
			RE::ActorEquipManager* em = RE::ActorEquipManager::GetSingleton();
			RE::BGSDefaultObjectManager* objManager = RE::BGSDefaultObjectManager::GetSingleton();
			RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();

			if (!a_item)
			{
				return;
			}

			RE::BGSEquipSlot* rightHandSlot = nullptr;
			RE::BGSEquipSlot* leftHandSlot = nullptr;

			if (objManager->IsInitialized())
			{
				rightHandSlot = static_cast<RE::BGSEquipSlot*>(objManager->GetObject(RE::DEFAULT_OBJECTS::kRightHandEquip));
				leftHandSlot = static_cast<RE::BGSEquipSlot*>(objManager->GetObject(RE::DEFAULT_OBJECTS::kLeftHandEquip));
			}

			switch (a_item->formType.get())
			{
			case RE::FormType::Armor:
				{
					RE::TESObjectARMO* item = static_cast<RE::TESObjectARMO*>(a_item);

					if (IsEquipped(item))
					{
						em->UnequipObject(player, item, a_extraData, 1, item->GetEquipSlot());
					}
					else
					{
						em->EquipObject(player, item, a_extraData, 1, item->GetEquipSlot());
					}
					break;
				}
			case RE::FormType::Weapon:
				{
					RE::TESObjectWEAP* item = static_cast<RE::TESObjectWEAP*>(a_item);
					if (item == player->GetActorRuntimeData().currentProcess->GetEquippedLeftHand() || item == player->GetActorRuntimeData().currentProcess->GetEquippedRightHand())
					{
						//Item already equipped
						em->UnequipObject(player, item, nullptr, 1, item->GetEquipSlot());
					}
					else
					{
						em->EquipObject(player, item, a_extraData, 1, item->GetEquipSlot());
					}
					break;
				}
			case RE::FormType::Light:
				{
					RE::TESObjectLIGH* item = static_cast<RE::TESObjectLIGH*>(a_item);
					if (a_item == player->GetActorRuntimeData().currentProcess->GetEquippedLeftHand() || a_item == player->GetActorRuntimeData().currentProcess->GetEquippedRightHand())
					{
						//Item already equipped
						em->UnequipObject(player, item, nullptr, 1, item->GetEquipSlot());
					}
					else
					{
						em->EquipObject(player, item, a_extraData, 1, item->GetEquipSlot());
					}
					break;
				}
			case RE::FormType::AlchemyItem:
				{
					RE::AlchemyItem* item = static_cast<RE::AlchemyItem*>(a_item);
					em->EquipObject(player, item, a_extraData, 1, item->GetEquipSlot());
					break;
				}
			case RE::FormType::Ingredient:
				{
					RE::IngredientItem* item = static_cast<RE::IngredientItem*>(a_item);
					em->EquipObject(player, item, a_extraData, 1, item->GetEquipSlot());
					break;
				}
			case RE::FormType::Spell:
				{
					RE::SpellItem* item = static_cast<RE::SpellItem*>(a_item);
					if (item->IsTwoHanded())
					{
						em->EquipSpell(player, item, item->GetEquipSlot());
					}
					else
					{
						if (player->GetActorRuntimeData().selectedSpells[RE::PlayerCharacter::SlotTypes::kLeftHand] != item)
						{
							//Equip spell to left hand
							em->EquipSpell(player, item, leftHandSlot);
						}
						else if (player->GetActorRuntimeData().selectedSpells[RE::PlayerCharacter::SlotTypes::kRightHand] != item)
						{
							//Equip spell to right hand
							em->EquipSpell(player, item, rightHandSlot);
						}
					}

					return;  //Nothing to equip
				}
			case RE::FormType::Shout:
				{
					RE::TESShout* item = static_cast<RE::TESShout*>(a_item);
					em->EquipShout(player, item);
					break;
				}
			case RE::FormType::Ammo:
				{
					RE::TESAmmo* item = static_cast<RE::TESAmmo*>(a_item);
					if (IsEquipped(item))
					{
						em->UnequipObject(player, item, nullptr, 1, nullptr);
					}
					else
					{
						em->EquipObject(player, item, a_extraData, 1, nullptr);
					}
					break;
				}
			case RE::FormType::Scroll:
				{
					RE::ScrollItem* item = static_cast<RE::ScrollItem*>(a_item);
					em->EquipObject(player, item, a_extraData, 1, nullptr);
					break;
				}
			}
			RE::PlaySound("UIFavorite");
		};

		TaskQueue::GetSingleton()->AddTask(task);
	}

	bool FavoritesHandlerEx::ProcessButton_Hook(RE::ButtonEvent* a_event)
	{
		using EHKS::HotkeyManager;

		RE::UserEvents* userEvents = RE::UserEvents::GetSingleton();
		RE::UI* ui = RE::UI::GetSingleton();

		// Note: When used with Skyrim Souls RE, this input handler is already blocked
		if (ui->GameIsPaused() || ui->IsMenuOpen(RE::CraftingMenu::MENU_NAME))
		{
			return false;
		}

		if (a_event->userEvent == userEvents->favorites)
		{
			if (!ui->IsMenuOpen(RE::FavoritesMenu::MENU_NAME))
			{
				RE::UIMessageQueue* messageQueue = RE::UIMessageQueue::GetSingleton();
				messageQueue->AddMessage(RE::FavoritesMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kShow, 0);
				return true;
			}
		}

		if (a_event->eventType == RE::INPUT_EVENT_TYPE::kButton)
		{
			HotkeyManager* hotkeyManager = HotkeyManager::GetSingleton();
			Hotkey* hotkey = nullptr;

			if (a_event->device == RE::INPUT_DEVICE::kGamepad && !EHKS::IsVanillaHotkey(a_event->userEvent))
			{
				return false;
			}

			bool isVampire = IsPlayerVampire();
			isVampire ? hotkey = hotkeyManager->GetVampireHotkey(a_event->device.get(), a_event->idCode) : hotkey = hotkeyManager->GetHotkey(a_event->device.get(), a_event->idCode);

			if (hotkey)
			{
				//Update hotkeys and check again
				hotkeyManager->UpdateHotkeys();

				if (isVampire ? hotkey = hotkeyManager->GetVampireHotkey(a_event->device.get(), a_event->idCode) : hotkey = hotkeyManager->GetHotkey(a_event->device.get(), a_event->idCode))
				{
					if (hotkey->type == Hotkey::HotkeyType::kItem)
					{
						ItemHotkey* itemHotkey = static_cast<ItemHotkey*>(hotkey);
						RE::TESForm* baseForm = itemHotkey->GetBaseForm();
						if (baseForm)
						{
							RE::ExtraDataList* extraData = itemHotkey->GetExtraData();
							EquipItem(baseForm, extraData);
							return true;
						}
					}
					else
					{
						MagicHotkey* magicHotkey = static_cast<MagicHotkey*>(hotkey);
						EquipItem(magicHotkey->form, nullptr);
						return true;
					}
				}
			}
		}
		return false;
	}

	void FavoritesHandlerEx::InstallHook()
	{
		REL::ID favoritesHandler_IsHotkey_Hook{ 52258 };
		SKSE::GetTrampoline().write_call<6>(favoritesHandler_IsHotkey_Hook.address() + 0x2F, (uintptr_t)IsHotkey_Hook);
		std::uint8_t codes[] = { 0x90, 0x90, 0x90, 0x90, 0x90 };
		REL::safe_write(favoritesHandler_IsHotkey_Hook.address() + 0x2F + 0x6, codes, sizeof(codes));

		REL::Relocation<std::uintptr_t> vTable(RE::VTABLE_FavoritesHandler[0]);
		_ProcessButton = vTable.write_vfunc(0x5, &FavoritesHandlerEx::ProcessButton_Hook);
	}
}
