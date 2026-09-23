#include "Hooks_FavoritesHandler.h"

#include "HotkeyManager.h"
#include "Settings.h"
#include "Util.h"

namespace EHKS
{
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
		if (mc->beastForm && !HotkeyManager::GetSingleton()->IsPlayerVampire())
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

	RE::TESForm* GetHotkeyForm(Hotkey* a_hotkey)
	{
		if (a_hotkey->type == Hotkey::HotkeyType::kItem)
		{
			return static_cast<ItemHotkey*>(a_hotkey)->GetBaseForm();
		}
		return static_cast<MagicHotkey*>(a_hotkey)->form;
	}

	// The hotkey's equip mode, or kAuto for an item that doesn't fit either hand
	Hotkey::EquipMode GetEquipMode(const Hotkey* a_hotkey, RE::TESForm* a_form)
	{
		return HotkeyManager::CanChooseHand(a_form) ? a_hotkey->equipMode : Hotkey::EquipMode::kAuto;
	}

	bool IsInHand(RE::TESForm* a_item, Hotkey::EquipMode a_equipMode)
	{
		RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
		bool inLeftHand = a_item == player->currentProcess->GetEquippedLeftHand();
		bool inRightHand = a_item == player->currentProcess->GetEquippedRightHand();

		switch (a_equipMode)
		{
		case Hotkey::EquipMode::kLeft:
			return inLeftHand;
		case Hotkey::EquipMode::kRight:
			return inRightHand;
		default:
			return inLeftHand || inRightHand;
		}
	}

	void GetWornCopies(RE::TESBoundObject* a_item, std::int32_t& a_count, RE::ExtraDataList*& a_wornLeft, RE::ExtraDataList*& a_wornRight)
	{
		a_count = 0;
		a_wornLeft = nullptr;
		a_wornRight = nullptr;

		RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
		RE::TESObjectREFR::InventoryItemMap inventory = player->GetInventory([a_item](RE::TESBoundObject& a_object) { return &a_object == a_item; });

		RE::TESObjectREFR::InventoryItemMap::iterator it = inventory.find(a_item);
		if (it == inventory.end())
		{
			return;
		}

		a_count = it->second.first;

		RE::InventoryEntryData* entryData = it->second.second.get();
		if (!entryData || !entryData->extraLists)
		{
			return;
		}

		for (RE::BSSimpleList<RE::ExtraDataList*>::iterator it2 = entryData->extraLists->begin(); it2 != entryData->extraLists->end(); ++it2)
		{
			RE::ExtraDataList* extraDataEntry = *it2;
			if (!extraDataEntry)
			{
				continue;
			}

			if (extraDataEntry->HasType(RE::ExtraDataType::kWornLeft))
			{
				a_wornLeft = extraDataEntry;
			}
			if (extraDataEntry->HasType(RE::ExtraDataType::kWorn))
			{
				a_wornRight = extraDataEntry;
			}
		}
	}

	bool CanDualWield(RE::TESForm* a_item, Hotkey::EquipMode a_equipMode, bool& a_toLeftHand)
	{
		if (a_equipMode != Hotkey::EquipMode::kAuto || !Settings::GetSingleton()->dualWieldSupport)
		{
			return false;
		}

		RE::TESObjectWEAP* weapon = a_item->As<RE::TESObjectWEAP>();
		RE::BGSDefaultObjectManager* objManager = RE::BGSDefaultObjectManager::GetSingleton();
		if (!weapon || !objManager->IsInitialized())
		{
			return false;
		}

		RE::TESRace* race = RE::PlayerCharacter::GetSingleton()->GetRace();
		RE::BGSEquipSlot* slot = weapon->GetEquipSlot();
		bool fitsEitherHand = slot == objManager->GetObject(RE::DEFAULT_OBJECTS::kEitherHandEquip);
		bool namesHand = slot == objManager->GetObject(RE::DEFAULT_OBJECTS::kLeftHandEquip) || slot == objManager->GetObject(RE::DEFAULT_OBJECTS::kRightHandEquip);
		bool raceCanDualWield = race && race->data.flags.all(RE::RACE_DATA::Flag::kCanDualWield);
		if (!fitsEitherHand && !(namesHand && raceCanDualWield))
		{
			return false;
		}

		std::int32_t count = 0;
		RE::ExtraDataList* wornLeft = nullptr;
		RE::ExtraDataList* wornRight = nullptr;
		GetWornCopies(weapon, count, wornLeft, wornRight);

		if (count < 2 || (wornLeft != nullptr) == (wornRight != nullptr))
		{
			return false;
		}

		a_toLeftHand = wornRight != nullptr;
		return true;
	}

	// Items that can be taken off again: the first of these on the button decides whether the press equips or unequips
	bool IsToggleable(RE::TESForm* a_item, Hotkey::EquipMode a_equipMode, bool& a_isEquipped)
	{
		switch (a_item->formType.get())
		{
		case RE::FormType::Armor:
		case RE::FormType::Ammo:
			a_isEquipped = IsEquipped(a_item);
			return true;
		case RE::FormType::Weapon:
		case RE::FormType::Light:
			{
				bool toLeftHand = false;
				a_isEquipped = IsInHand(a_item, a_equipMode) && !CanDualWield(a_item, a_equipMode, toLeftHand);
				return true;
			}
		default:
			return false;
		}
	}

	void UpdateShield(RE::PlayerCharacter* a_player, RE::TESForm* a_item)
	{
		if (!a_player->currentProcess)
		{
			return;
		}

		a_player->currentProcess->Update3DModel(a_player);

		if (a_item->formType.get() == RE::FormType::Armor)
		{
			REL::Relocation<RE::BIPED_OBJECT (*)(RE::Actor*)> GetShieldObjectSlot(REL::ID{ 19630 });
			REL::Relocation<bool (*)(RE::BGSBipedObjectForm*, RE::BIPED_OBJECT)> HasPartOf(REL::ID{ 14119 });
			REL::Relocation<void (*)(RE::Actor*)> DoUpdateShield(REL::ID{ 40418 });

			if (HasPartOf(static_cast<RE::TESObjectARMO*>(a_item), GetShieldObjectSlot(a_player)))
			{
				DoUpdateShield(a_player);
			}
		}
	}

	void EquipItem(RE::TESForm* a_item, RE::ExtraDataList* a_extraData, bool a_equip, Hotkey::EquipMode a_equipMode)
	{
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

		// The hand the equip mode picks, null for kAuto
		RE::BGSEquipSlot* chosenHandSlot = nullptr;
		if (a_equipMode == Hotkey::EquipMode::kLeft)
		{
			chosenHandSlot = leftHandSlot;
		}
		else if (a_equipMode == Hotkey::EquipMode::kRight)
		{
			chosenHandSlot = rightHandSlot;
		}

		switch (a_item->formType.get())
		{
		case RE::FormType::Armor:
			{
				RE::TESObjectARMO* item = static_cast<RE::TESObjectARMO*>(a_item);

				if (!a_equip)
				{
					em->UnequipObject(player, item, a_extraData, 1, item->GetEquipSlot());
				}
				else if (!IsEquipped(item))
				{
					em->EquipObject(player, item, a_extraData, 1, item->GetEquipSlot());
				}
				break;
			}
		case RE::FormType::Weapon:
			{
				RE::TESObjectWEAP* item = static_cast<RE::TESObjectWEAP*>(a_item);
				RE::BGSEquipSlot* slot = chosenHandSlot ? chosenHandSlot : item->GetEquipSlot();
				bool toLeftHand = false;
				if (!a_equip)
				{
					std::int32_t count = 0;
					RE::ExtraDataList* wornLeft = nullptr;
					RE::ExtraDataList* wornRight = nullptr;
					if (!chosenHandSlot && Settings::GetSingleton()->dualWieldSupport)
					{
						GetWornCopies(item, count, wornLeft, wornRight);
					}

					if (wornLeft && wornRight)
					{
						em->UnequipObject(player, item, wornLeft, 1, leftHandSlot);
						em->UnequipObject(player, item, wornRight, 1, rightHandSlot);
					}
					else
					{
						em->UnequipObject(player, item, nullptr, 1, slot);
					}
				}
				else if (CanDualWield(item, a_equipMode, toLeftHand))
				{
					em->EquipObject(player, item, nullptr, 1, toLeftHand ? leftHandSlot : rightHandSlot);
				}
				else if (!IsInHand(item, a_equipMode))
				{
					em->EquipObject(player, item, a_extraData, 1, slot);
				}
				break;
			}
		case RE::FormType::Light:
			{
				RE::TESObjectLIGH* item = static_cast<RE::TESObjectLIGH*>(a_item);
				if (!a_equip)
				{
					em->UnequipObject(player, item, nullptr, 1, item->GetEquipSlot());
				}
				else if (!IsInHand(item, a_equipMode))
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
				else if (chosenHandSlot)
				{
					em->EquipSpell(player, item, chosenHandSlot);
				}
				else
				{
					if (player->selectedSpells[RE::PlayerCharacter::SlotTypes::kLeftHand] != item)
					{
						//Equip spell to left hand
						em->EquipSpell(player, item, leftHandSlot);
					}
					else if (player->selectedSpells[RE::PlayerCharacter::SlotTypes::kRightHand] != item)
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
				if (!a_equip)
				{
					em->UnequipObject(player, item, nullptr, 1, nullptr);
				}
				else if (!IsEquipped(item))
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

		UpdateShield(player, a_item);

		RE::PlaySound("UIFavorite");
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

		if (a_event->userEvent == userEvents->favorites && a_event->IsDown())
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

			if (a_event->device == RE::INPUT_DEVICE::kGamepad && !EHKS::IsVanillaHotkey(a_event->userEvent))
			{
				return false;
			}

			std::vector<Hotkey*> hotkeys = hotkeyManager->GetActiveHotkeys(a_event->device.get(), a_event->idCode);

			//Every item on the button goes the same way, decided by the first one that can be taken off
			bool equip = true;
			for (std::vector<Hotkey*>::iterator it = hotkeys.begin(); it != hotkeys.end(); ++it)
			{
				RE::TESForm* form = GetHotkeyForm(*it);
				bool isEquipped = false;
				if (form && IsToggleable(form, GetEquipMode(*it, form), isEquipped))
				{
					equip = !isEquipped;
					break;
				}
			}

			bool handled = false;
			for (std::vector<Hotkey*>::iterator it = hotkeys.begin(); it != hotkeys.end(); ++it)
			{
				Hotkey* hotkey = *it;
				RE::TESForm* form = GetHotkeyForm(hotkey);
				if (form)
				{
					RE::ExtraDataList* extraData = hotkey->type == Hotkey::HotkeyType::kItem ? static_cast<ItemHotkey*>(hotkey)->GetExtraData() : nullptr;
					EquipItem(form, extraData, equip, GetEquipMode(hotkey, form));
					handled = true;
				}
			}
			return handled;
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
