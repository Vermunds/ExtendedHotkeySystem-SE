#include "Hooks_FavoritesMenu.h"

#include "HotkeyManager.h"
#include "Settings.h"
#include "Util.h"

#include <ModConfigUI/Localization.h>

namespace EHKS
{
	bool IsAssignmentKeyDown()
	{
		EHKS::Settings* settings = EHKS::Settings::GetSingleton();
		RE::BSInputDeviceManager* inputDeviceManager = RE::BSInputDeviceManager::GetSingleton();

		RE::BSInputDevice* inputDevice;
		if (settings->assignmentKey.inputDevice == RE::INPUT_DEVICE::kKeyboard)
		{
			inputDevice = inputDeviceManager->GetKeyboard();
		}
		else
		{
			inputDevice = inputDeviceManager->GetMouse();
		}
		return inputDevice->IsPressed(settings->assignmentKey.id);
	}

	RE::UI_MESSAGE_RESULTS FavoritesMenuEx::ProcessMessage_Hook(RE::UIMessage& a_message)
	{
		if (a_message.type == RE::UI_MESSAGE_TYPE::kShow)
		{
			bool controllerMode = RE::BSInputDeviceManager::GetSingleton()->IsGamepadEnabled();
			this->UpdateAssignHint(controllerMode);
		}
		if (a_message.type == RE::UI_MESSAGE_TYPE::kUserEvent || a_message.type == RE::UI_MESSAGE_TYPE::kScaleformEvent)
		{
			if (IsAssignmentKeyDown())
			{
				return RE::UI_MESSAGE_RESULTS::kIgnore;
			}
		}
		return _ProcessMessage(this, a_message);
	}

	void FavoritesMenuEx::AdvanceMovie_Hook(float a_interval, std::uint32_t a_currentTime)
	{
		bool controllerMode = RE::BSInputDeviceManager::GetSingleton()->IsGamepadEnabled();
		this->UpdateHotkeyIcons(controllerMode);
		this->UpdateAssignHint(controllerMode);
		this->_AdvanceMovie(this, a_interval, a_currentTime);
	}

	bool FavoritesMenuEx::CanProcess_Hook(RE::InputEvent* a_event)
	{
		if (a_event->HasIDCode() && a_event->eventType == RE::INPUT_EVENT_TYPE::kButton && static_cast<RE::ButtonEvent*>(a_event)->IsUp())
		{
			return true;
		}
		return false;
	}

	bool FavoritesMenuEx::ProcessButton_Hook(RE::ButtonEvent* a_event)
	{
		EHKS::Settings* settings = EHKS::Settings::GetSingleton();

		using EHKS::HotkeyManager;

		RE::UI* ui = RE::UI::GetSingleton();

		RE::FavoritesMenu* favoritesMenu = static_cast<RE::FavoritesMenu*>(ui->GetMenu(RE::FavoritesMenu::MENU_NAME).get());

		if (favoritesMenu)
		{
			RE::GFxValue result;
			favoritesMenu->uiMovie->GetVariable(&result, "_root.MenuHolder.Menu_mc.itemList._selectedIndex");

			if (result.GetType() == RE::GFxValue::ValueType::kNumber)
			{
				std::uint32_t selectedIndex = static_cast<std::uint32_t>(result.GetNumber());

				bool isValidGamepadButton = a_event->device == RE::INPUT_DEVICE::kGamepad && EHKS::IsVanillaHotkey(a_event->userEvent);
				bool allowAssignmentKey = !settings->useWhitelist || !settings->enforceWhitelist;
				bool isValid = IsAssignmentKeyDown() && a_event->idCode != settings->assignmentKey.id;
				bool isInWhitelist = settings->useWhitelist && settings->IsInWhitelist(a_event->device.get(), a_event->idCode);

				if (isValidGamepadButton || (allowAssignmentKey && isValid) || isInWhitelist)
				{
					if (0 <= selectedIndex && selectedIndex < favoritesMenu->favorites.size())
					{
						RE::TESForm* selectedItem = favoritesMenu->favorites[selectedIndex].item;

						HotkeyManager* hotkeyManager = HotkeyManager::GetSingleton();

						if (favoritesMenu->isVampire)
						{
							hotkeyManager->AddVampireHotkey(a_event->device.get(), a_event->idCode, selectedItem);
						}
						else
						{
							Hotkey::HotkeyType hotkeyType = HotkeyManager::GetHotkeyType(selectedItem);
							if (hotkeyType == Hotkey::HotkeyType::kItem)
							{
								hotkeyManager->AddItemHotkey(a_event->device.get(), a_event->idCode, favoritesMenu->favorites[selectedIndex].entryData);
							}
							else
							{
								hotkeyManager->AddMagicHotkey(a_event->device.get(), a_event->idCode, selectedItem);
							}
						}
						RE::PlaySound("UIFavorite");
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

	void UnSetHotkeyIcon(RE::GFxValue a_hotkeyIcon)
	{
		RE::GFxValue::DisplayInfo displayInfo;

		a_hotkeyIcon.GetDisplayInfo(&displayInfo);
		displayInfo.SetVisible(false);
		a_hotkeyIcon.SetDisplayInfo(displayInfo);
		a_hotkeyIcon.GotoAndStop("0");
	}

	void SetHotkeyIcon(RE::GFxValue a_hotkeyIcon, RE::INPUT_DEVICE a_device, std::uint32_t a_keyMask, bool a_controllerMode)
	{
		RE::GFxValue::DisplayInfo displayInfo;
		std::string str;

		if (a_device == RE::INPUT_DEVICE::kKeyboard)
		{
			str = std::to_string(a_keyMask);
		}
		else if (a_device == RE::INPUT_DEVICE::kMouse)
		{
			str = std::to_string(a_keyMask + SKSE::InputMap::kMacro_MouseButtonOffset);
		}
		else if (a_device == RE::INPUT_DEVICE::kGamepad)
		{
			str = std::to_string(GetGamepadIconIndex(a_keyMask));
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

	void FavoritesMenuEx::UpdateHotkeyIcons(bool a_controllerMode)
	{
		using EHKS::HotkeyManager;

		HotkeyManager* hotkeyManager = HotkeyManager::GetSingleton();

		RE::GFxValue clipMgr, entryList;
		this->uiMovie->GetVariable(&clipMgr, "_root.MenuHolder.Menu_mc.itemList._entryClipManager");
		this->uiMovie->GetVariable(&entryList, "_root.MenuHolder.Menu_mc.itemList._entryList");
		if (clipMgr.GetType() == RE::GFxValue::ValueType::kUndefined)
		{
			return;
		}
		if (entryList.GetType() == RE::GFxValue::ValueType::kUndefined)
		{
			return;
		}

		for (std::uint32_t i = 0; i < this->favorites.size(); ++i)
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

			if (this->isVampire)
			{
				if (Hotkey* hotkey = hotkeyManager->GetVampireHotkey(this->favorites[i].item))
				{
					SetHotkeyIcon(hotkeyIcon, hotkey->device, hotkey->keyMask, a_controllerMode);
					continue;
				}
			}
			else
			{
				Hotkey::HotkeyType hotkeyType = HotkeyManager::GetHotkeyType(this->favorites[i].item);
				if (hotkeyType == Hotkey::HotkeyType::kItem)
				{
					if (Hotkey* hotkey = hotkeyManager->GetItemHotkey(this->favorites[i].entryData))
					{
						SetHotkeyIcon(hotkeyIcon, hotkey->device, hotkey->keyMask, a_controllerMode);
						continue;
					}
				}
				else
				{
					if (Hotkey* hotkey = hotkeyManager->GetMagicHotkey(this->favorites[i].item))
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

	bool GetAssignHintKey(bool a_controllerMode, std::uint32_t& a_key)
	{
		EHKS::Settings* settings = EHKS::Settings::GetSingleton();

		if (a_controllerMode)
		{
			// Gamepad hotkeys are assigned with the vanilla buttons, the assignment key does not apply
			return false;
		}
		if (settings->useWhitelist && settings->enforceWhitelist)
		{
			// The assignment key is disabled, only whitelisted keys can be assigned
			return false;
		}

		if (settings->assignmentKey.inputDevice == RE::INPUT_DEVICE::kKeyboard)
		{
			a_key = settings->assignmentKey.id;
			return true;
		}
		if (settings->assignmentKey.inputDevice == RE::INPUT_DEVICE::kMouse)
		{
			a_key = settings->assignmentKey.id + SKSE::InputMap::kMacro_MouseButtonOffset;
			return true;
		}

		return false;
	}

	void FavoritesMenuEx::UpdateAssignHint(bool a_controllerMode)
	{
		RE::GFxValue showFunction;
		if (!this->uiMovie->GetVariable(&showFunction, "_root.MenuHolder.Menu_mc.showEHKSHint") || showFunction.IsUndefined())
		{
			return;
		}

		RE::GFxValue shownKey;
		this->uiMovie->GetVariable(&shownKey, "_root.MenuHolder.Menu_mc._ehksHintKey");

		std::uint32_t key = 0;
		if (!GetAssignHintKey(a_controllerMode, key))
		{
			if (shownKey.IsNumber())
			{
				this->uiMovie->Invoke("_root.MenuHolder.Menu_mc.hideEHKSHint", nullptr, nullptr, 0);
			}
			return;
		}

		if (shownKey.IsNumber() && static_cast<std::uint32_t>(shownKey.GetNumber()) == key)
		{
			return;
		}

		RE::GFxValue args[2];
		args[0].SetNumber(static_cast<double>(key));
		args[1].SetString(ModConfigUI::Localization::Get("$EHKS_Hint_AssignHotkey"));
		this->uiMovie->Invoke("_root.MenuHolder.Menu_mc.showEHKSHint", nullptr, args, 2);
	}

	void FavoritesMenuEx::InstallHook()
	{
		REL::Relocation<std::uintptr_t> vTable(RE::VTABLE_FavoritesMenu[0]);

		_ProcessMessage = vTable.write_vfunc(0x4, &FavoritesMenuEx::ProcessMessage_Hook);
		_AdvanceMovie = vTable.write_vfunc(0x5, &FavoritesMenuEx::AdvanceMovie_Hook);

		REL::Relocation<std::uintptr_t> vTableMenuEventHandler(RE::VTABLE_FavoritesMenu[1]);

		_CanProcess = vTableMenuEventHandler.write_vfunc(0x1, &FavoritesMenuEx::CanProcess_Hook);
		_ProcessButton = vTableMenuEventHandler.write_vfunc(0x7, &FavoritesMenuEx::ProcessButton_Hook);
	}
}
