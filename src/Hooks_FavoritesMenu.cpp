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

	inline void UnSetHotkeyIcon(RE::GFxValue a_hotkeyIcon)
	{
		RE::GFxValue::DisplayInfo displayInfo;

		a_hotkeyIcon.GetDisplayInfo(&displayInfo);
		displayInfo.SetVisible(false);
		a_hotkeyIcon.SetDisplayInfo(displayInfo);
		a_hotkeyIcon.GotoAndStop("0");
	}

	inline void SetHotkeyIcon(RE::GFxValue a_hotkeyIcon, RE::INPUT_DEVICE a_device, UInt32 a_keyMask, bool a_controllerMode)
	{
		RE::GFxValue::DisplayInfo displayInfo;
		std::string str;

		if (a_device == RE::INPUT_DEVICE::kKeyboard)
		{
			str = std::to_string(a_keyMask);
		}
		else if (a_device == RE::INPUT_DEVICE::kMouse)
		{
			str = std::to_string(a_keyMask + 256);
		}
		else if (a_device == RE::INPUT_DEVICE::kGamepad)
		{
			str = std::to_string(MHK::GetGamepadIconIndex(a_keyMask));
		}
		else
		{
			//Hotkey is not available
			UnSetHotkeyIcon(a_hotkeyIcon);
			return;
		}

		a_hotkeyIcon.GetDisplayInfo(&displayInfo);
		displayInfo.SetVisible(true);
		a_hotkeyIcon.SetDisplayInfo(displayInfo);
		a_hotkeyIcon.GotoAndStop(str.c_str());
	}

	void UpdateHotkeyIcons(RE::FavoritesMenu* a_favoritesMenu, bool a_controllerMode)
	{
		using MHK::HotkeyManager;

		HotkeyManager* hotkeyManager = HotkeyManager::GetSingleton();

		RE::GFxValue clipMgr, entryList;
		a_favoritesMenu->view->GetVariable(&clipMgr, "_root.MenuHolder.Menu_mc.itemList._entryClipManager");
		a_favoritesMenu->view->GetVariable(&entryList, "_root.MenuHolder.Menu_mc.itemList._entryList");
		if (clipMgr.GetType() == RE::GFxValue::ValueType::kUndefined)
		{
			return;
		}
		if (entryList.GetType() == RE::GFxValue::ValueType::kUndefined)
		{
			return;
		}

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

			if (a_favoritesMenu->isVampire)
			{
				if (HotkeyManager::Hotkey* hotkey = hotkeyManager->GetVampireHotkey(a_favoritesMenu->favorites[i].item))
				{
					SetHotkeyIcon(hotkeyIcon, hotkey->device, hotkey->keyMask, a_controllerMode);
					continue;
				}
			}
			else
			{
				HotkeyManager::Hotkey::Type hotkeyType = HotkeyManager::GetHotkeyType(a_favoritesMenu->favorites[i].item);
				if (hotkeyType == HotkeyManager::Hotkey::Type::kItem)
				{
					if (HotkeyManager::Hotkey* hotkey = hotkeyManager->GetItemHotkey(a_favoritesMenu->favorites[i].entryData))
					{
						SetHotkeyIcon(hotkeyIcon, hotkey->device, hotkey->keyMask, a_controllerMode);
						continue;
					}
				}
				else
				{
					if (HotkeyManager::Hotkey* hotkey = hotkeyManager->GetMagicHotkey(a_favoritesMenu->favorites[i].item))
					{
						//Hotkey found
						SetHotkeyIcon(hotkeyIcon, hotkey->device, hotkey->keyMask, a_controllerMode);
						continue;
					}
				}
			}
			//Hotkey not found
			UnSetHotkeyIcon(hotkeyIcon);
		}
	}

	bool CanProcess_Hook(RE::MenuEventHandler* a_this, RE::InputEvent* a_event)
	{
		RE::BSInputDeviceManager* inputDeviceManager = RE::BSInputDeviceManager::GetSingleton();

		RE::UI* ui = RE::UI::GetSingleton();
		RE::InterfaceStrings* interfaceStrings = RE::InterfaceStrings::GetSingleton();
		RE::FavoritesMenu* favoritesMenu = static_cast<RE::FavoritesMenu*>(ui->GetMenu(interfaceStrings->favoritesMenu).get());

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

		using MHK::HotkeyManager;

		RE::UI* ui = RE::UI::GetSingleton();
		RE::InterfaceStrings* interfaceStrings = RE::InterfaceStrings::GetSingleton();

		RE::FavoritesMenu* favoritesMenu = static_cast<RE::FavoritesMenu*>(ui->GetMenu(interfaceStrings->favoritesMenu).get());

		if (favoritesMenu)
		{
			RE::GFxValue result;
			favoritesMenu->view->GetVariable(&result, "_root.MenuHolder.Menu_mc.itemList._selectedIndex");

			if (result.GetType() == RE::GFxValue::ValueType::kNumber)
			{
				UInt32 selectedIndex = static_cast<UInt32>(result.GetNumber());


				bool isValidGamepadButton = a_event->device == RE::INPUT_DEVICE::kGamepad && MHK::IsVanillaHotkey(a_event->userEvent);
				bool allowModifier = !settings->useWhiteList || (settings->useWhiteList && settings->allowOverride);
				bool isValid = IsModifierKeyDown() && a_event->idCode != settings->modifierKey.id;
				bool isInWhitelist = settings->useWhiteList && settings->IsInWhitelist(a_event->device, a_event->idCode);

				if (isValidGamepadButton || (allowModifier && isValid) || isInWhitelist)
				{
					if (0 <= selectedIndex && selectedIndex < favoritesMenu->favorites.size())
					{
						RE::TESForm* selectedItem = favoritesMenu->favorites[selectedIndex].item;

						HotkeyManager* hotkeyManager = HotkeyManager::GetSingleton();

						if (favoritesMenu->isVampire)
						{
							hotkeyManager->AddVampireHotkey(a_event->device, a_event->idCode, selectedItem);
						}
						else
						{
							HotkeyManager::Hotkey::Type hotkeyType = HotkeyManager::GetHotkeyType(selectedItem);
							if (hotkeyType == HotkeyManager::Hotkey::Type::kItem)
							{
								hotkeyManager->AddItemHotkey(a_event->device, a_event->idCode, favoritesMenu->favorites[selectedIndex].entryData);
							}
							else
							{
								hotkeyManager->AddMagicHotkey(a_event->device, a_event->idCode, selectedItem);
							}
						}
						void(*PlaySound)(const char*) = reinterpret_cast<void(*)(const char*)>(Offsets::PlaySound.GetUIntPtr());
						PlaySound("UIFavorite");
						return true;
					}
				}
				else if (a_event->device == RE::INPUT_DEVICE::kGamepad)
				{

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