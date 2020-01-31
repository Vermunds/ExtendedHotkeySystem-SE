#include "skse64_common/SafeWrite.h"
#include "skse64/gamethreads.h" //TaskDelegate

#include "RE/Skyrim.h"

#include "HotkeyManager.h"
#include "Util.h"
#include "Settings.h"
#include "Offsets.h"

#include "SKSE\API.h"

namespace Hooks_FavoritesMenu
{
	uintptr_t ProcessButton_Original_ptr;
	uintptr_t CanProcess_Original_ptr;
	uintptr_t ProcessMessage_Original_ptr;
	uintptr_t AdvanceMovie_Original_ptr;

	RE::ExtraHotkey::Hotkey GetVanillaHotkey(RE::TESForm* a_form)
	{
		RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
		RE::BSSimpleList<RE::InventoryEntryData*>* entries = player->GetInventoryChanges()->entryList;

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
						RE::ExtraHotkey* extraHotkey = static_cast<RE::ExtraHotkey*>(extraData->GetByType(RE::ExtraDataType::kHotkey));
						return extraHotkey->hotkey;
					}
				}
			}
		}
		return RE::ExtraHotkey::Hotkey::kUnbound;
	}

	bool IsModifierKeyDown()
	{
		MHK::Settings* settings = MHK::Settings::GetSingleton();
		RE::BSInputDeviceManager* inputDeviceManager = RE::BSInputDeviceManager::GetSingleton();

		RE::BSInputDevice* inputDevice;
		if (settings->modifierKey.inputDevice == RE::INPUT_DEVICE::kKeyboard)
		{
			inputDevice = inputDeviceManager->GetKeyboard();
		}
		else
		{
			inputDevice = inputDeviceManager->GetMouse();
		}
		return inputDevice->IsPressed(settings->modifierKey.id);
	}

	void UpdateHotkeyIcons(RE::FavoritesMenu* a_favoritesMenu, bool a_controllerMode)
	{
		using MHK::HotkeyManager;

		HotkeyManager* hotkeyManager = HotkeyManager::GetSingleton();

		RE::GFxValue clipMgr, entryList;
		a_favoritesMenu->view->GetVariable(&clipMgr, "_root.MenuHolder.Menu_mc.itemList._entryClipManager");
		a_favoritesMenu->view->GetVariable(&entryList, "_root.MenuHolder.Menu_mc.itemList._entryList");
		_ASSERT(clipMgr.GetType() != RE::GFxValue::ValueType::kUndefined);
		_ASSERT(entryList.GetType() != RE::GFxValue::ValueType::kUndefined);

		for (UInt32 i = 0; i < a_favoritesMenu->favorites.size(); ++i)
		{
			RE::GFxValue entry, clipIndex, clip, hotkeyIcon;

			entryList.GetElement(i, &entry);
			if (entry.GetType() == RE::GFxValue::ValueType::kUndefined)
			{
				continue;
			}
			entry.GetMember("clipIndex", &clipIndex);
			if (clipIndex.GetType() == RE::GFxValue::ValueType::kUndefined)
			{
				continue;
			}
			clipMgr.Invoke("getClip", &clip, &clipIndex, 1);
			if (clip.GetType() == RE::GFxValue::ValueType::kUndefined)
			{
				continue;
			}
			clip.GetMember("hotkeyIcon", &hotkeyIcon);
			if (hotkeyIcon.GetType() == RE::GFxValue::ValueType::kUndefined)
			{
				continue;
			}

			RE::GFxValue::DisplayInfo displayInfo;

			if (a_controllerMode)
			{
				using VanillaHotkey = RE::ExtraHotkey::Hotkey;

				RE::ControlMap* controlMap = RE::ControlMap::GetSingleton();
				RE::UserEvents* userEvents = RE::UserEvents::GetSingleton();

				VanillaHotkey vanillaHotkey = GetVanillaHotkey(a_favoritesMenu->favorites[i].item);
				UInt32 mappedKey = 0;

				switch (vanillaHotkey)
				{
				case VanillaHotkey::kSlot1:
				{
					mappedKey = controlMap->GetMappedKey(userEvents->hotkey1, RE::INPUT_DEVICE::kGamepad, RE::UserEvents::INPUT_CONTEXT_ID::kGameplay);
					break;
				}
				case VanillaHotkey::kSlot2:
				{
					mappedKey = controlMap->GetMappedKey(userEvents->hotkey2, RE::INPUT_DEVICE::kGamepad, RE::UserEvents::INPUT_CONTEXT_ID::kGameplay);
					break;
				}
				case VanillaHotkey::kSlot3:
				{
					mappedKey = controlMap->GetMappedKey(userEvents->hotkey3, RE::INPUT_DEVICE::kGamepad, RE::UserEvents::INPUT_CONTEXT_ID::kGameplay);
					break;
				}
				case VanillaHotkey::kSlot4:
				{
					mappedKey = controlMap->GetMappedKey(userEvents->hotkey4, RE::INPUT_DEVICE::kGamepad, RE::UserEvents::INPUT_CONTEXT_ID::kGameplay);
					break;
				}
				case VanillaHotkey::kSlot5:
				{
					mappedKey = controlMap->GetMappedKey(userEvents->hotkey5, RE::INPUT_DEVICE::kGamepad, RE::UserEvents::INPUT_CONTEXT_ID::kGameplay);
					break;
				}
				case VanillaHotkey::kSlot6:
				{
					mappedKey = controlMap->GetMappedKey(userEvents->hotkey6, RE::INPUT_DEVICE::kGamepad, RE::UserEvents::INPUT_CONTEXT_ID::kGameplay);
					break;
				}
				case VanillaHotkey::kSlot7:
				{
					mappedKey = controlMap->GetMappedKey(userEvents->hotkey7, RE::INPUT_DEVICE::kGamepad, RE::UserEvents::INPUT_CONTEXT_ID::kGameplay);
					break;
				}
				case VanillaHotkey::kSlot8:
				{
					mappedKey = controlMap->GetMappedKey(userEvents->hotkey8, RE::INPUT_DEVICE::kGamepad, RE::UserEvents::INPUT_CONTEXT_ID::kGameplay);
					break;
				}
				}

				if (mappedKey)
				{
					//Item has a hotkey
					hotkeyIcon.GetDisplayInfo(&displayInfo);
					displayInfo.SetVisible(true);
					hotkeyIcon.SetDisplayInfo(displayInfo);
					hotkeyIcon.GotoAndStop(std::to_string(MHK::GetGamepadIconIndex(mappedKey)).c_str());
				}
				else
				{
					//Item has no hotkey
					hotkeyIcon.GetDisplayInfo(&displayInfo);
					displayInfo.SetVisible(false);
					hotkeyIcon.SetDisplayInfo(displayInfo);
					hotkeyIcon.GotoAndStop("0");
				}

			}
			else
			{
				if (HotkeyManager::Hotkey* hotkey = hotkeyManager->GetHotkey(a_favoritesMenu->favorites[i].item, a_favoritesMenu->isVampire))
				{
					//Hotkey found
					std::string str;
					switch (hotkey->device)
					{
					case RE::INPUT_DEVICE::kKeyboard:
						str = std::to_string(hotkey->keyMask);
						break;
					case RE::INPUT_DEVICE::kMouse:
						str = std::to_string(hotkey->keyMask + 256);
						break;
					}
					hotkeyIcon.GetDisplayInfo(&displayInfo);
					displayInfo.SetVisible(true);
					hotkeyIcon.SetDisplayInfo(displayInfo);
					hotkeyIcon.GotoAndStop(str.c_str());
				}
				else
				{
					//Hotkey not found
					hotkeyIcon.GetDisplayInfo(&displayInfo);
					displayInfo.SetVisible(false);
					hotkeyIcon.SetDisplayInfo(displayInfo);
					hotkeyIcon.GotoAndStop("0");
				}
			}
		}
	}

	bool CanProcess_Hook(RE::MenuEventHandler* a_this, RE::InputEvent* a_event)
	{
		RE::BSInputDeviceManager* inputDeviceManager = RE::BSInputDeviceManager::GetSingleton();

		RE::UI* ui = RE::UI::GetSingleton();
		RE::InterfaceStrings* interfaceStrings = RE::InterfaceStrings::GetSingleton();
		RE::FavoritesMenu* favoritesMenu = static_cast<RE::FavoritesMenu*>(ui->GetMenu(interfaceStrings->favoritesMenu).get());

		if (inputDeviceManager->IsGamepadEnabled())
		{
			bool(*CanProcess_Original)(RE::FavoritesMenu*, RE::InputEvent*);
			CanProcess_Original = reinterpret_cast<bool(*)(RE::FavoritesMenu*, RE::InputEvent*)>(CanProcess_Original_ptr);;
			return CanProcess_Original(favoritesMenu, a_event);
		}

		if (a_event->HasIDCode() && a_event->eventType == RE::INPUT_EVENT_TYPE::kButton && static_cast<RE::ButtonEvent*>(a_event)->IsUp())
		{
			return true;
		}
		return false;
	}

	bool ProcessButton_Hook(RE::MenuEventHandler* a_this, RE::ButtonEvent* a_event)
	{
		RE::BSInputDeviceManager* inputDeviceManager = RE::BSInputDeviceManager::GetSingleton();
		MHK::Settings* settings = MHK::Settings::GetSingleton();

		if (inputDeviceManager->IsGamepadEnabled())
		{
			bool(*ProcessButton_Original)(RE::MenuEventHandler*, RE::InputEvent*);
			ProcessButton_Original = reinterpret_cast<bool(*)(RE::MenuEventHandler*, RE::InputEvent*)>(ProcessButton_Original_ptr);;
			return ProcessButton_Original(a_this, a_event);
		}

		using MHK::HotkeyManager;

		RE::UI* ui = RE::UI::GetSingleton();
		RE::InterfaceStrings* interfaceStrings = RE::InterfaceStrings::GetSingleton();

		RE::FavoritesMenu* favoritesMenu = static_cast<RE::FavoritesMenu*>(ui->GetMenu(interfaceStrings->favoritesMenu).get());

		if (favoritesMenu)
		{
			RE::GFxValue result;
			favoritesMenu->view->Invoke("_root.GetSelectedIndex", &result, nullptr, 0);

			if (result.GetType() == RE::GFxValue::ValueType::kNumber)
			{
				UInt32 selectedIndex = static_cast<UInt32>(result.GetNumber());

				bool allowModifier = !settings->useWhiteList || (settings->useWhiteList && settings->allowOverride);
				bool isValid = IsModifierKeyDown() && a_event->idCode != settings->modifierKey.id;
				bool isInWhitelist = settings->useWhiteList && settings->IsInWhitelist(a_event->device, a_event->idCode);

				if ((allowModifier && isValid) || isInWhitelist)
				{
					if (0 <= selectedIndex && selectedIndex < favoritesMenu->favorites.size())
					{
						RE::TESForm* selectedItem = favoritesMenu->favorites[selectedIndex].item;

						HotkeyManager* hotkeyManager = HotkeyManager::GetSingleton();
						HotkeyManager::Hotkey* hotkey = hotkeyManager->GetHotkey(favoritesMenu->favorites[selectedIndex].item, favoritesMenu->isVampire);

						if (hotkey && hotkey->keyMask == a_event->idCode)
						{
							//Removing hotkey
							hotkeyManager->RemoveHotkey(hotkey, favoritesMenu->isVampire);
						}
						else
						{
							//Creating hotkey
							HotkeyManager::Hotkey* newHotkey = new HotkeyManager::Hotkey();
							newHotkey->device = a_event->device;
							newHotkey->keyMask = a_event->idCode;
							newHotkey->item = selectedItem;
							hotkeyManager->AddHotkey(newHotkey, favoritesMenu->isVampire);

							//_MESSAGE("Created new keyboard hotkey: deviceType: %d, keyMask: %d, formID: %.8x", a_event->device, a_event->idCode, selectedItem->formID);
						}
						void(*PlaySound)(const char*) = reinterpret_cast<void(*)(const char*)>(Offsets::PlaySound.GetUIntPtr());
						PlaySound("UIFavorite");
						return true;
					}
				}
			}
		}

		return false;
	}

	RE::UI_MESSAGE_RESULTS ProcessMessage_Hook(RE::FavoritesMenu* a_this, RE::UIMessage& a_message)
	{
		if (a_message.type == RE::UI_MESSAGE_TYPE::kUserEvent || a_message.type == RE::UI_MESSAGE_TYPE::kScaleformEvent)
		{
			if (IsModifierKeyDown())
			{
				return RE::UI_MESSAGE_RESULTS::kIgnore;
			}
		}
		RE::UI_MESSAGE_RESULTS(*ProcessMessage_Original)(RE::FavoritesMenu*, RE::UIMessage&);
		ProcessMessage_Original = reinterpret_cast<RE::UI_MESSAGE_RESULTS(*)(RE::FavoritesMenu*, RE::UIMessage&)>(ProcessMessage_Original_ptr);;
		return ProcessMessage_Original(a_this,a_message);
	}

	void AdvanceMovie_Hook(RE::FavoritesMenu* a_this, float a_interval, UInt32 a_currentTime)
	{
		UpdateHotkeyIcons(a_this, RE::BSInputDeviceManager::GetSingleton()->IsGamepadEnabled());

		void(*AdvanceMovie_Original)(RE::IMenu*, float, UInt32);
		AdvanceMovie_Original = reinterpret_cast<void(*)(RE::IMenu*, float, UInt32)>(AdvanceMovie_Original_ptr);;
		return AdvanceMovie_Original(a_this, a_interval, a_currentTime);
	}

	void InstallHooks()
	{
		RelocAddr<uintptr_t*> CanProcessPtr = RE::Offset::FavoritesMenu::Vtbl + (0x8 * 0x8) + 0x10 + (0x1 * 0x8);
		RelocAddr<uintptr_t*> ProcessButtonPtr = RE::Offset::FavoritesMenu::Vtbl + (0x8 * 0x8) + 0x10 + (0x5 * 0x8);
		RelocAddr<uintptr_t*> processMessagePtr = RE::Offset::FavoritesMenu::Vtbl + (0x4 * 0x8);
		RelocAddr<uintptr_t*> advanceMoviePtr = RE::Offset::FavoritesMenu::Vtbl + (0x5 * 0x8);

		CanProcess_Original_ptr = *(reinterpret_cast<uintptr_t*>(CanProcessPtr.GetUIntPtr()));
		ProcessButton_Original_ptr = *(reinterpret_cast<uintptr_t*>(ProcessButtonPtr.GetUIntPtr()));
		ProcessMessage_Original_ptr = *(reinterpret_cast<uintptr_t*>(processMessagePtr.GetUIntPtr()));
		AdvanceMovie_Original_ptr = *(reinterpret_cast<uintptr_t*>(advanceMoviePtr.GetUIntPtr()));

		SafeWrite64(CanProcessPtr.GetUIntPtr(), (uintptr_t)CanProcess_Hook);
		SafeWrite64(ProcessButtonPtr.GetUIntPtr(), (uintptr_t)ProcessButton_Hook);
		SafeWrite64(processMessagePtr.GetUIntPtr(), (uintptr_t)ProcessMessage_Hook);
		SafeWrite64(advanceMoviePtr.GetUIntPtr(), (uintptr_t)AdvanceMovie_Hook);
	}
}