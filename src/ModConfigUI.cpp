#include "ModConfigUI.h"

#include "HotkeyManager.h"
#include "Settings.h"
#include "Version.h"

#include <ModConfigUI/Binding.h>
#include <ModConfigUI/Localization.h>

namespace EHKS
{
	const char* Translate(const char* a_key)
	{
		return ModConfigUI::Localization::Get(a_key);
	}

	// One drawn row, with every cell already resolved. The button is drawn as the image of its key.
	struct HotkeyRow
	{
		std::string id;  // Only item hotkeys have one, see BuildRows
		const char* device;
		ModConfigUI::ButtonBinding control;
		std::string item;

		Hotkey::EquipMode equipMode = Hotkey::EquipMode::kAuto;
		bool canChooseHand = false;  // Only then is there an equip mode to pick

		// A button can be shared between hotkeys, so the row points at its own. Only ever compared, never
		// dereferenced, as the hotkey may be gone by the time it is used.
		const Hotkey* hotkey = nullptr;
	};

	std::vector<HotkeyRow> g_itemRows;
	std::vector<HotkeyRow> g_magicRows;
	std::vector<HotkeyRow> g_vampireRows;
	bool g_characterLoaded = false;

	// Resolving the rows walks the player's whole inventory, which is far too much to do per frame, so
	// it is redone on a timer instead. Short enough that assigning a hotkey and tabbing over shows it.
	constexpr std::chrono::milliseconds REFRESH_INTERVAL{ 500 };

	// Set by a delete once it has actually run, so the table stops showing the row it removed without
	// waiting out the interval above.
	bool g_refreshPending = false;

	std::string GetFormName(RE::TESForm* a_form)
	{
		if (!a_form)
		{
			return Translate("$EHKS_Item_Missing");
		}

		const char* name = a_form->GetName();
		if (!name || name[0] == '\0')
		{
			// Nameless forms still say something useful, the form id is what the console shows too.
			return std::format("[{:08X}]", a_form->GetFormID());
		}

		return name;
	}

	// Every hotkeyed item in the player's inventory, by the extra data id the hotkey refers to.
	// ItemHotkey::GetBaseForm walks the inventory itself, which would be one full pass per row.
	void CollectHotkeyedItems(std::map<std::uint8_t, RE::TESForm*>& a_items)
	{
		RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
		if (!player)
		{
			return;
		}

		RE::TESObjectREFR::InventoryItemMap inventory = player->GetInventory();

		for (RE::TESObjectREFR::InventoryItemMap::iterator it = inventory.begin(); it != inventory.end(); ++it)
		{
			RE::InventoryEntryData* entryData = it->second.second.get();
			if (!entryData || !entryData->extraLists)
			{
				continue;
			}

			for (RE::BSSimpleList<RE::ExtraDataList*>::iterator it2 = entryData->extraLists->begin(); it2 != entryData->extraLists->end(); ++it2)
			{
				RE::ExtraDataList* extraDataEntry = *it2;
				if (!extraDataEntry || !extraDataEntry->HasType(RE::ExtraDataType::kHotkey))
				{
					continue;
				}

				RE::ExtraHotkey* extraHotkey = static_cast<RE::ExtraHotkey*>(extraDataEntry->GetByType(RE::ExtraDataType::kHotkey));
				a_items.insert_or_assign(static_cast<std::uint8_t>(extraHotkey->hotkey.get()), entryData->object);
			}
		}
	}

	// Adds the hotkeys of a_type only, item and magic hotkeys are shown in tables of their own.
	void BuildRows(const std::list<Hotkey*>& a_hotkeys, Hotkey::HotkeyType a_type, const std::map<std::uint8_t, RE::TESForm*>& a_items, std::vector<HotkeyRow>& a_rows)
	{
		for (const Hotkey* hotkey : a_hotkeys)
		{
			if (!hotkey || hotkey->type != a_type)
			{
				continue;
			}

			RE::TESForm* form = nullptr;

			// Only item hotkeys carry an id of their own, the extra data slot the hotkey is written to.
			// A magic hotkey is identified by its form, there is nothing to show.
			std::string id;

			if (hotkey->type == Hotkey::HotkeyType::kItem)
			{
				const ItemHotkey* itemHotkey = static_cast<const ItemHotkey*>(hotkey);

				std::map<std::uint8_t, RE::TESForm*>::const_iterator it = a_items.find(itemHotkey->extraDataId);
				if (it != a_items.end())
				{
					form = it->second;
				}

				id = std::format("{}", itemHotkey->extraDataId);
			}
			else
			{
				form = static_cast<const MagicHotkey*>(hotkey)->form;
			}

			a_rows.push_back(HotkeyRow{
				.id = id,
				.device = ModConfigUI::GetDeviceName(hotkey->device),
				.control = ModConfigUI::ButtonBinding::FromEvent(hotkey->device, hotkey->keyMask),
				.item = GetFormName(form),
				.equipMode = hotkey->equipMode,
				.canChooseHand = HotkeyManager::CanChooseHand(form),
				.hotkey = hotkey });
		}

		// The hotkeys are held in the order they were assigned, which reads as no order at all.
		std::sort(a_rows.begin(), a_rows.end(), [](const HotkeyRow& a_lhs, const HotkeyRow& a_rhs) {
			return a_lhs.device != a_rhs.device ? a_lhs.device < a_rhs.device : a_lhs.control.key < a_rhs.control.key;
		});
	}

	void RefreshRows()
	{
		g_itemRows.clear();
		g_magicRows.clear();
		g_vampireRows.clear();

		// Nothing is assigned to anyone until a save is loaded, and the inventory can't be read either.
		RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
		g_characterLoaded = player && player->Is3DLoaded();

		if (!g_characterLoaded)
		{
			return;
		}

		std::map<std::uint8_t, RE::TESForm*> items;
		CollectHotkeyedItems(items);

		HotkeyManager* hotkeyManager = HotkeyManager::GetSingleton();

		BuildRows(hotkeyManager->GetHotkeys(), Hotkey::HotkeyType::kItem, items, g_itemRows);
		BuildRows(hotkeyManager->GetHotkeys(), Hotkey::HotkeyType::kMagic, items, g_magicRows);

		// Vampire Lord hotkeys are always spells
		BuildRows(hotkeyManager->GetVampireHotkeys(), Hotkey::HotkeyType::kMagic, items, g_vampireRows);
	}

	void RefreshRowsIfStale()
	{
		static std::chrono::steady_clock::time_point lastRefresh{};

		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		if (!g_refreshPending && now - lastRefresh < REFRESH_INTERVAL)
		{
			return;
		}
		g_refreshPending = false;
		lastRefresh = now;

		RefreshRows();
	}

	// Removing a hotkey rewrites the manager's list and, for an item, the extra data on the inventory entry behind it.
	void RemoveHotkey(const Hotkey* a_hotkey, bool a_isVampire)
	{
		HotkeyManager* hotkeyManager = HotkeyManager::GetSingleton();

		bool removed = a_isVampire ? hotkeyManager->RemoveVampireHotkey(a_hotkey) : hotkeyManager->RemoveHotkey(a_hotkey);
		if (!removed)
		{
			logger::warn("Failed to remove the hotkey, it no longer exists.");
		}

		// Only now that the list has actually changed is there a new snapshot worth taking.
		g_refreshPending = true;
	}

	void SetEquipMode(const Hotkey* a_hotkey, Hotkey::EquipMode a_equipMode)
	{
		if (!HotkeyManager::GetSingleton()->SetEquipMode(a_hotkey, a_equipMode))
		{
			logger::warn("Failed to set the equip mode, the hotkey no longer exists.");
		}
		g_refreshPending = true;
	}

	// Auto, Left, Right, then round again
	Hotkey::EquipMode GetNextEquipMode(Hotkey::EquipMode a_equipMode)
	{
		switch (a_equipMode)
		{
		case Hotkey::EquipMode::kAuto:
			return Hotkey::EquipMode::kLeft;
		case Hotkey::EquipMode::kLeft:
			return Hotkey::EquipMode::kRight;
		default:
			return Hotkey::EquipMode::kAuto;
		}
	}

	const char* GetEquipModeLabel(Hotkey::EquipMode a_equipMode)
	{
		switch (a_equipMode)
		{
		case Hotkey::EquipMode::kLeft:
			return Translate("$EHKS_EquipMode_Left");
		case Hotkey::EquipMode::kRight:
			return Translate("$EHKS_EquipMode_Right");
		default:
			return Translate("$EHKS_EquipMode_Auto");
		}
	}

	// a_showId adds the ID column in front, only item hotkeys have one. a_showEquipMode adds the Equip Mode
	// column, which the Vampire Lord's hotkeys don't have.
	void DrawHotkeyTable(ModConfigUI::Renderer& a_renderer, const char* a_id, const std::vector<HotkeyRow>& a_rows, bool a_isVampire, bool a_showId, bool a_showEquipMode)
	{
		if (a_rows.empty())
		{
			a_renderer.TextDisabled(Translate("$EHKS_NoHotkeys"));
			return;
		}

		// The widths are shares of the table, so a column left out gives its room to the others.
		// The item name is the longest of them by far, so it gets the room the others don't need.
		std::vector<const char*> headers;
		std::vector<float> widths;

		if (a_showId)
		{
			headers.push_back(Translate("$EHKS_Column_Id"));
			widths.push_back(0.08f);
		}

		headers.push_back(Translate("$EHKS_Column_Device"));
		widths.push_back(0.15f);
		headers.push_back(Translate("$EHKS_Column_Control"));
		widths.push_back(0.22f);
		headers.push_back(Translate("$EHKS_Column_Item"));
		widths.push_back(0.38f);

		if (a_showEquipMode)
		{
			headers.push_back(Translate("$EHKS_Column_EquipMode"));
			widths.push_back(0.15f);
		}

		headers.push_back(Translate("$EHKS_Column_Actions"));
		widths.push_back(0.17f);

		if (!a_renderer.BeginTable(a_id, headers.data(), static_cast<std::int32_t>(headers.size()), widths.data()))
		{
			return;
		}

		const char* deleteLabel = Translate("$EHKS_Delete");

		for (std::int32_t i = 0; i < static_cast<std::int32_t>(a_rows.size()); ++i)
		{
			const HotkeyRow& row = a_rows[i];

			// Every delete button carries the same label, the row number is what tells them apart.
			a_renderer.PushID(i);

			a_renderer.TableNextRow();

			if (a_showId)
			{
				a_renderer.TableNextColumn();
				a_renderer.Text(row.id.c_str());
			}

			a_renderer.TableNextColumn();
			a_renderer.Text(row.device);

			a_renderer.TableNextColumn();
			a_renderer.ButtonImage(row.control);

			a_renderer.TableNextColumn();
			a_renderer.Text(row.item.c_str());

			if (a_showEquipMode)
			{
				// Left empty for what only fits one slot anyway, a two-handed weapon or a potion
				a_renderer.TableNextColumn();
				if (row.canChooseHand && a_renderer.Button(GetEquipModeLabel(row.equipMode)))
				{
					SetEquipMode(row.hotkey, GetNextEquipMode(row.equipMode));
				}
			}

			a_renderer.TableNextColumn();
			if (a_renderer.Button(deleteLabel))
			{
				RemoveHotkey(row.hotkey, a_isVampire);
			}

			a_renderer.PopID();
		}

		a_renderer.EndTable();
	}

	// Pages
	void DrawHotkeyOverviewPage(ModConfigUI::Renderer& a_renderer)
	{
		RefreshRowsIfStale();

		a_renderer.TextWrappedMuted(Translate("$EHKS_Hotkeys_Description"));

		if (!g_characterLoaded)
		{
			a_renderer.Spacing();
			a_renderer.TextDisabled(Translate("$EHKS_NoCharacter"));
			return;
		}

		a_renderer.SeparatorText(Translate("$EHKS_Section_ItemHotkeys"));
		DrawHotkeyTable(a_renderer, "EHKS_ItemHotkeys", g_itemRows, false, true, true);

		a_renderer.SeparatorText(Translate("$EHKS_Section_MagicHotkeys"));
		DrawHotkeyTable(a_renderer, "EHKS_MagicHotkeys", g_magicRows, false, false, true);

		a_renderer.SeparatorText(Translate("$EHKS_Section_VampireHotkeys"));
		DrawHotkeyTable(a_renderer, "EHKS_VampireHotkeys", g_vampireRows, true, false, false);
	}

	// Whitelist editing
	//
	// The presets below are written as unified keys, the space ModConfigUI and the INI file both use.
	// A keyboard scan code is its own number there, so a top row key is simply its scan code.

	// The eight keys Skyrim itself hotkeys to, 1 through 8.
	constexpr std::uint32_t VANILLA_HOTKEYS[] = { 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 };

	// Those eight and the rest of the top row up to backspace: 9, 0, - and =.
	constexpr std::uint32_t TOP_ROW[] = { 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D };

	// The numpad digits, which are not in scan code order.
	constexpr std::uint32_t NUMPAD_DIGITS[] = { 0x52, 0x4F, 0x50, 0x51, 0x4B, 0x4C, 0x4D, 0x47, 0x48, 0x49 };

	constexpr const char* ADD_KEY_POPUP = "$EHKS_Whitelist_AddSingle_Title";

	// The button the dialog is waiting on. Unbound until the user presses something.
	ModConfigUI::ButtonBinding g_pendingKey;

	bool IsWhitelisted(const std::vector<Settings::Button>& a_whitelist, ModConfigUI::ButtonBinding a_binding)
	{
		for (std::vector<Settings::Button>::const_iterator it = a_whitelist.begin(); it != a_whitelist.end(); ++it)
		{
			if (ModConfigUI::ButtonBinding::FromEvent(it->inputDevice, it->id) == a_binding)
			{
				return true;
			}
		}
		return false;
	}

	// Adds every button that isn't on the list already and saves if anything actually changed. The
	// presets overlap on purpose, so adding one after another must not duplicate what is there.
	void AddToWhitelist(std::span<const std::uint32_t> a_keys)
	{
		Settings* settings = Settings::GetSingleton();

		std::vector<Settings::Button> whitelist = settings->GetWhitelist();
		bool changed = false;

		for (std::uint32_t key : a_keys)
		{
			ModConfigUI::ButtonBinding binding{ key };
			if (!binding.IsSet() || IsWhitelisted(whitelist, binding))
			{
				continue;
			}

			whitelist.push_back(Settings::Button{ binding.GetDevice(), binding.GetDeviceId() });
			changed = true;
		}

		if (!changed)
		{
			return;
		}

		// Held in the order they were added, which reads as no order at all once a couple of presets
		// have been mixed. Sorted by the key itself, so a row lands where the keyboard would put it.
		std::sort(whitelist.begin(), whitelist.end(), [](const Settings::Button& a_lhs, const Settings::Button& a_rhs) {
			return ModConfigUI::ButtonBinding::FromEvent(a_lhs.inputDevice, a_lhs.id).key < ModConfigUI::ButtonBinding::FromEvent(a_rhs.inputDevice, a_rhs.id).key;
		});

		settings->SetWhitelist(whitelist);
		SaveSettings();
	}

	void RemoveFromWhitelist(ModConfigUI::ButtonBinding a_binding)
	{
		Settings* settings = Settings::GetSingleton();

		std::vector<Settings::Button> whitelist = settings->GetWhitelist();

		std::erase_if(whitelist, [a_binding](const Settings::Button& a_button) {
			return ModConfigUI::ButtonBinding::FromEvent(a_button.inputDevice, a_button.id) == a_binding;
		});

		settings->SetWhitelist(whitelist);
		SaveSettings();
	}

	void DrawAddKeyPopup(ModConfigUI::Renderer& a_renderer)
	{
		if (!a_renderer.BeginPopupModal(Translate(ADD_KEY_POPUP)))
		{
			return;
		}

		a_renderer.TextWrappedMuted(Translate("$EHKS_Whitelist_AddSingle_Description"));
		a_renderer.Spacing();

		// The same binder the assignment key uses. A controller button can't be whitelisted: the check
		// this feeds is only reached for a keyboard or mouse button.
		constexpr ModConfigUI::BindFilter FILTER = ModConfigUI::BindFilter::kKeyboard | ModConfigUI::BindFilter::kMouse;
		a_renderer.BindButton(Translate("$EHKS_Whitelist_AddSingle_Button"), &g_pendingKey, FILTER);

		a_renderer.Spacing();

		bool hasKey = g_pendingKey.IsSet();
		if (hasKey && a_renderer.Button(Translate("$EHKS_Whitelist_AddSingle_Confirm")))
		{
			AddToWhitelist(std::span<const std::uint32_t>{ &g_pendingKey.key, 1 });
			g_pendingKey = ModConfigUI::ButtonBinding{};
			a_renderer.CloseCurrentPopup();
		}

		if (hasKey)
		{
			a_renderer.SameLine();
		}

		if (a_renderer.Button(Translate("$EHKS_Whitelist_AddSingle_Cancel")))
		{
			g_pendingKey = ModConfigUI::ButtonBinding{};
			a_renderer.CloseCurrentPopup();
		}

		a_renderer.EndPopup();
	}

	void DrawWhitelistTable(ModConfigUI::Renderer& a_renderer, const std::vector<Settings::Button>& a_whitelist)
	{
		if (a_whitelist.empty())
		{
			a_renderer.TextDisabled(Translate("$EHKS_Whitelist_Empty"));
			return;
		}

		constexpr std::int32_t COLUMN_COUNT = 3;

		const char* headers[COLUMN_COUNT] = {
			Translate("$EHKS_Column_Device"),
			Translate("$EHKS_Column_Control"),
			Translate("$EHKS_Column_Actions")
		};

		constexpr float WIDTHS[COLUMN_COUNT] = { 0.25f, 0.50f, 0.25f };

		if (!a_renderer.BeginTable("EHKS_Whitelist", headers, COLUMN_COUNT, WIDTHS))
		{
			return;
		}

		const char* removeLabel = Translate("$EHKS_Remove");

		// What a remove button clicked this frame should take off the list. Acted on after the table is
		// closed, so the rows being drawn are not rebuilt underneath the loop.
		ModConfigUI::ButtonBinding pendingRemove;

		for (std::int32_t i = 0; i < static_cast<std::int32_t>(a_whitelist.size()); ++i)
		{
			const Settings::Button& button = a_whitelist[i];
			ModConfigUI::ButtonBinding binding = ModConfigUI::ButtonBinding::FromEvent(button.inputDevice, button.id);

			// Every remove button carries the same label, the row number is what tells them apart.
			a_renderer.PushID(i);

			a_renderer.TableNextRow();

			a_renderer.TableNextColumn();
			a_renderer.Text(ModConfigUI::GetDeviceName(button.inputDevice));

			a_renderer.TableNextColumn();
			a_renderer.ButtonImage(binding);

			a_renderer.TableNextColumn();
			if (a_renderer.Button(removeLabel))
			{
				pendingRemove = binding;
			}

			a_renderer.PopID();
		}

		a_renderer.EndTable();

		if (pendingRemove.IsSet())
		{
			RemoveFromWhitelist(pendingRemove);
		}
	}

	void DrawWhitelistSection(ModConfigUI::Renderer& a_renderer)
	{
		Settings* settings = Settings::GetSingleton();

		a_renderer.SeparatorText(Translate("$EHKS_Section_Whitelist"));

		if (a_renderer.Checkbox(Translate("$EHKS_Setting_EnableWhitelist"), &settings->useWhitelist, USE_WHITELIST_DEFAULT_VALUE, Translate("$EHKS_Setting_EnableWhitelist_Tooltip")))
		{
			SaveSettings();
		}

		if (a_renderer.Checkbox(Translate("$EHKS_Setting_EnforceWhitelist"), &settings->enforceWhitelist, ENFORCE_WHITELIST_DEFAULT_VALUE, Translate("$EHKS_Setting_EnforceWhitelist_Tooltip")))
		{
			SaveSettings();
		}

		a_renderer.Spacing();

		if (a_renderer.Button(Translate("$EHKS_Whitelist_AddVanilla")))
		{
			AddToWhitelist(VANILLA_HOTKEYS);
		}
		a_renderer.ItemTooltip(Translate("$EHKS_Whitelist_AddVanilla_Tooltip"));

		a_renderer.SameLine();
		if (a_renderer.Button(Translate("$EHKS_Whitelist_AddTopRow")))
		{
			AddToWhitelist(TOP_ROW);
		}
		a_renderer.ItemTooltip(Translate("$EHKS_Whitelist_AddTopRow_Tooltip"));

		a_renderer.SameLine();
		if (a_renderer.Button(Translate("$EHKS_Whitelist_AddNumpad")))
		{
			AddToWhitelist(NUMPAD_DIGITS);
		}
		a_renderer.ItemTooltip(Translate("$EHKS_Whitelist_AddNumpad_Tooltip"));

		a_renderer.SameLine();
		if (a_renderer.Button(Translate("$EHKS_Whitelist_AddSingle")))
		{
			g_pendingKey = ModConfigUI::ButtonBinding{};
			a_renderer.OpenPopup(Translate(ADD_KEY_POPUP));
		}
		a_renderer.ItemTooltip(Translate("$EHKS_Whitelist_AddSingle_Tooltip"));

		a_renderer.SameLine();
		if (a_renderer.Button(Translate("$EHKS_Whitelist_Clear")))
		{
			settings->SetWhitelist({});
			SaveSettings();
		}
		a_renderer.ItemTooltip(Translate("$EHKS_Whitelist_Clear_Tooltip"));

		// Drawn while the popup is open, the dialog is a window of its own on top of this one.
		DrawAddKeyPopup(a_renderer);

		a_renderer.Spacing();

		DrawWhitelistTable(a_renderer, settings->GetWhitelist());
	}

	void DrawSettingsPage(ModConfigUI::Renderer& a_renderer)
	{
		Settings* settings = Settings::GetSingleton();

		a_renderer.SeparatorText(Translate("$EHKS_Section_General"));

		// Both hooks read the assignment key off the singleton every time they run, so the new button is in
		// effect the moment it is written. Only the INI has to be told about it.
		ModConfigUI::ButtonBinding assignmentKey = ModConfigUI::ButtonBinding::FromEvent(settings->assignmentKey.inputDevice, settings->assignmentKey.id);

		// A controller has no assignment key to hold: its hotkeys go through the vanilla buttons instead.
		constexpr ModConfigUI::BindFilter FILTER = ModConfigUI::BindFilter::kKeyboard | ModConfigUI::BindFilter::kMouse;

		if (a_renderer.BindButton(Translate("$EHKS_Setting_AssignmentKey"), &assignmentKey, ModConfigUI::ButtonBinding{ ASSIGNMENT_KEY_DEFAULT_VALUE }, Translate("$EHKS_Setting_AssignmentKey_Tooltip"), FILTER))
		{
			settings->assignmentKey = Settings::Button{ assignmentKey.GetDevice(), assignmentKey.GetDeviceId() };
			SaveSettings();
		}

		if (a_renderer.Checkbox(Translate("$EHKS_Setting_AllowDuplicates"), &settings->allowDuplicates, ALLOW_DUPLICATES_DEFAULT_VALUE, Translate("$EHKS_Setting_AllowDuplicates_Tooltip")))
		{
			SaveSettings();
		}

		if (a_renderer.Checkbox(Translate("$EHKS_Setting_DualWieldSupport"), &settings->dualWieldSupport, DUAL_WIELD_SUPPORT_DEFAULT_VALUE, Translate("$EHKS_Setting_DualWieldSupport_Tooltip")))
		{
			SaveSettings();
		}

		a_renderer.Spacing();

		DrawWhitelistSection(a_renderer);
	}

	void InstallModConfigUI()
	{
		static constexpr ModConfigUI::ModInfo MOD_INFO{
			.pluginName = Version::NAME.data(),
			.displayName = Version::FORMATTED_NAME.data(),
			.version = Version::STRING.data(),
			.author = Version::AUTHOR.data(),
			.description = "$EHKS_Description",
			.nexusUrl = "https://www.nexusmods.com/skyrimspecialedition/mods/32225",
			.sourceUrl = "https://github.com/Vermunds/ExtendedHotkeySystem-SE"
		};

		static constexpr ModConfigUI::Page PAGES[] = {
			{ "$EHKS_Page_HotkeyOverview", &DrawHotkeyOverviewPage },
			{ "$EHKS_Page_Settings", &DrawSettingsPage }
		};

		ModConfigUI::Install(MOD_INFO, PAGES, &RestoreDefaults);
	}
}
