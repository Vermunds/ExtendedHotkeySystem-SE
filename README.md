# Extended Hotkey System
[![Nexus Mods](https://img.shields.io/badge/NexusMods-Download-orange)](https://www.nexusmods.com/skyrimspecialedition/mods/32225)
[![GitHub release](https://img.shields.io/github/v/release/Vermunds/ExtendedHotkeySystem-SE)](https://github.com/Vermunds/ExtendedHotkeySystem-SE/releases)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](./LICENSE.md)

A mod for The Elder Scrolls V: Skyrim - Special Edition.

This mod replaces Skyrim's hotkey system with a custom one, allowing you to use as many hotkeys as you want. The hotkeys are created directly from the Favorites menu. Requires SkyUI.

## Features
- Just as responsive as vanilla hotkeys
- Assign hotkeys directly from the favorites menu
- Allows you to use any keyboard or mouse button for hotkeys
- Support for vampire lord abilities
- Allows you to map hotkeys on the keyboard even if you use a controller.
- Supports whitelisting (see below, disabled by default)

## Usage:
Same as vanilla, but there is one extra step: you have to hold down the Left control when you assign the hotkey. This is necessary because the Favorites Menu can't tell the difference when you want to assign a hotkey and when you don't. So for example, if you want to set the F1 button as a hotkey, you press Ctrl + F1 when assigning it.

If you don't mind some configuring, you can also create a whitelist. Whitelisted buttons don't need Ctrl to be pressed, and it will be 100% like the vanilla system. See the included .ini file for details.

Warning: The mod does not check for conflicts with existing controls. It will allow you to map literally anything, so it's you responsibility to ensure that it doesn't conflict with other controls the game or other mods may use.

## INI Configuration
The configuration file is located at:

`<Skyrim install folder>/Data/SKSE/Plugins/ExtendedHotkeySystem.ini`

The configuration file is fully commented for clarity. Lines starting with # are comments and have no effect in-game.
Delete the .ini file and launch the game once to regenerate the defaults in case you lost them.
Note that some mod managers may place this file in different locations than the one above. In this case, refer to the documentation of your mod manager of choice.

## Information for mod creators
This mod requires the Favorites Menu to provide the necessary icons for keyboard/controller buttons. A custom implementation of the SkyUI Favorites Menu is attached to the mod, but any mod that makes any changes to `favoritesmenu.swf` is not going to be compatible, unless a compatibility patch is available, or the mod was designed with compatibility in mind from the start.

## Download
Available on [Nexusmods](https://www.nexusmods.com/skyrimspecialedition/mods/32225).

## Build
To build this mod refer to my wrapper project [here](https://github.com/Vermunds/SkyrimSE-Mods).

## License
This software is available under the MIT License. See LICENSE.md for details.