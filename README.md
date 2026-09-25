# Extended Hotkey System
[![Nexus Mods](https://img.shields.io/badge/NexusMods-Download-orange)](https://www.nexusmods.com/skyrimspecialedition/mods/32225)
[![GitHub release](https://img.shields.io/github/v/release/Vermunds/ExtendedHotkeySystem-SE)](https://github.com/Vermunds/ExtendedHotkeySystem-SE/releases)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](./LICENSE)

A mod for The Elder Scrolls V: Skyrim - Special Edition.

This mod replaces Skyrim's hotkey system with a custom one, allowing you to use as many hotkeys as you want. The hotkeys are created directly from the Favorites menu. Requires SkyUI.

## Features
- Just as responsive as vanilla hotkeys
- Assign hotkeys directly from the favorites menu
- Allows you to use any keyboard or mouse button for hotkeys
- Support for vampire lord abilities
- Allows you to map hotkeys on the keyboard even if you use a controller.
- Supports whitelisting (by default the numeric buttons of 0-9 are whitelisted)
- Dual-wield support: a weapon hotkey equips a second copy of the weapon to the other hand, like the vanilla hotkeys

## Usage:
Same as vanilla, but there is one extra step: you have to hold down the assignment key (Left Ctrl by default) when you assign the hotkey. This is necessary because the Favorites Menu can't tell the difference when you want to assign a hotkey and when you don't. So for example, if you want to set the F1 button as a hotkey, you press Ctrl + F1 when assigning it.

You can also create a whitelist. Whitelisted buttons don't need the assignment key to be pressed, and it will be 100% like the vanilla system.

Warning: The mod does not check for conflicts with existing controls. It will allow you to map literally anything, so it's your responsibility to ensure that it doesn't conflict with other controls the game or other mods may use.

## Configuration
The settings can be changed in-game, in the settings menu of either SKSE Menu Framework or Fuzz's Legally Intelligible Core Kit, or in `Data/SKSE/Plugins/ExtendedHotkeySystem.ini`. Changes made in the menu are saved to the same file.

## Information for mod creators
This mod requires the Favorites Menu to provide the necessary icons for keyboard/controller buttons. A custom implementation of the SkyUI Favorites Menu is attached to the mod, but any mod that makes any changes to `favoritesmenu.swf` is not going to be compatible, unless a compatibility patch is available, or the mod was designed with compatibility in mind from the start.

## Download
Available on [Nexusmods](https://www.nexusmods.com/skyrimspecialedition/mods/32225).

## Build
To build this mod refer to my wrapper project [here](https://github.com/Vermunds/SkyrimSE-Mods).

## License
This software is available under the GNU General Public License v3.0 or later, with a modding exception. See [LICENSE](./LICENSE) and [EXCEPTIONS.md](./EXCEPTIONS.md) for details.