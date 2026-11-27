# ShipworksExtended
Is a mod made for the [Highfleet-Modloader](https://github.com/logdot/Highfleet-Modloader)
that allows you to define parts to be loaded into the shipworks.

This is Duumvir's fork of [logdot/ShipworksExtended](https://github.com/logdot/ShipworksExtended).
It keeps the part loader and adds:

- custom part menus, defined in the config instead of the game's hardcoded 14;
- a palette row that shows every menu the palette contains, not only the first 14;
- a click guard for custom menus (a click on one no longer picks it up as a part);
- validated config handling, a timestamped log and a crash report in the log;
- `highfleet-qol`-style MinGW/Proton build support, inherited from upstream.

Everything here was tested on the Steam 1.163 build of Highfleet.

## Using
To use the mod simply go into the [latest release](https://github.com/Duumvir-hf/ShipworksExtended-CustomMenusFork/releases/latest)
and drag and drop the contents of the zip file into the highfleet
folder


## Defining parts
Defining parts to be added into the shipworks is really simple. You only need
to add the part's m_oid into the mod's configuration file at `Modloader/config/Shipworks.json`.
You can edit the configuration file while the game is running and it's changes
will be aplied the next time you enter the shipworks.

The file is validated when it is read: a missing file, invalid JSON, a setting
that is not `true`/`false`, an unknown setting and a non-string part id are all
reported in `Modloader/shipworks.txt` instead of being ignored silently.

Here is an example Shipworks.json:
```json
{
	"LoadVanilla": false,
	"LoadVanillaHulls": false,
	"Hulls": [
		"HULL2x2_1",
		"HULL3x3_1"	
	],
	"LoadVanillaArmors": true,
	"Armors": [

	],
	"LoadVanillaFuels":  true,
	"Fuels": [

	],
	"LoadVanillaAmmos": true,
	"Ammos": [

	],
	"LoadVanillaLegs": false,
	"Legs": [

	],
	"LoadVanillaEngines": true,
	"Engines": [
		"MDL_ENGINE_5"
	],
	"LoadVanillaGuns": true,
	"Guns": [

	],
	"LoadVanillaSensors": true,
	"Sensors": [

	],
	"LoadVanillaExpendables": true,
	"Expendables": [

	],
	"LoadVanillaCrafts": true,
	"Crafts": [
		"Custom_Craft_00",
		"Custom_Craft_01",
		"Custom_Craft_02"
	],
	"LoadVanillaDefences": true,
	"Defences": [

	],
	"LoadVanillaGenerators": true,
	"Generators": [

	],
	"LoadVanillaQuarters": true,
	"Quarters": [

	],
	"LoadVanillaEmergency": true,
	"Emergency": [

	]
}
```

## Custom part menus
The constructor palette is a row of buttons; hovering one opens a sub-list of
parts. The game hardcodes 14 of them. This mod can serve a list of your own for
a button of your own, without touching that hardcoded table.

```json
"CustomMenus": [
	{
		"Button": "MDL_QUARTERS_01",
		"Parts": [
			"MDL_QUARTERS_01",
			"MDL_GENERATOR_01",
			"MDL_GENERATOR_02"
		]
	}
]
```

* `Button` is the `m_oid` of a **body in `Libraries/constructor.seria`** - the
  palette entry that opens the menu. It also decides the icon and the name of
  that entry, because those come from the part's stats in `OL.seria`, so use an
  oid that already exists (an `ITEM_*` item or an `MDL_*` part both work).
* `Parts` is what the flyout offers, in order.

To get a new button, add one body to `Libraries/constructor.seria`: copy any of
the `ITEM_*` entries and rename its `m_oid` to the id used as `Button`. The
palette does not keep the file order - the game sorts the cells by the stats'
`m_sort_index` - so that is the knob for the position, not the place in the file.

To *remove* a menu, delete its body from `Libraries/constructor.seria`. A part
menu is nothing but one of that file's children; there is no list to maintain
anywhere else.

How it works: the mod hooks the single `call BottomMenu_` in `mainMap`. For a
custom button it renames the hovered body's oid to `ITEM_HULL` for the duration
of one call and points the hull palette slot at the custom node, so the game
builds its own flyout - layout, count and purchase path - from your list. Both
are restored right afterwards. `ClickFunc` has its own 14-name guard that stops
a palette button from being picked up as a part, so the same rename is applied
around its call site in `HandleClick`.

`Modloader/shipworks.txt` prints `custom menu "X": N of M parts` when the list
is built and `opened custom menu "X"` the first time the button is hovered, so
a menu that is configured but never opened is easy to tell apart from one that
is not wired up.

## The palette row
The row lays out exactly `element width / cell width` cells and puts anything
past that past its own edge, which is why a 15th menu used to be invisible. The
mod measures the row (cells, cell width, extent) and widens the row element to
`cells x cellWidth`, so every menu in `constructor.seria` is shown at the game's
own spacing. The row grows symmetrically; the panel graphic behind it does not.

`shipworks.txt` reports the measurement once per screen:

```
palette row: 15 cells, grid 15x1, cell 100.000000, extent 1400.000000, spacing 0.000000
palette row: extent 1400.000000 -> 1500.000000 for 15 cells
```

## Hooks
Every hook can be switched off from the config without a rebuild, which is the
only sane way to bisect a game crash:

```json
"Hooks": {
	"PartMenu": true,
	"Click": true,
	"Layout": true
}
```

`PartMenu` is the hover flyout, `Click` the click guard, `Layout` the row
widening. Leaving a key out means "on".

## Crash log
Every line of `Modloader/shipworks.txt` carries the time since the mod started
(`[+   6.138]`). `src/crash.cpp` installs a vectored exception handler that
writes an extra block before the game's own `error.log`:

```
CRASH code=0xc0000005 operation=0x8 touched=0x1 (other) rip=0x1 (other)
CRASH stack 0x0: 0x7ffed01b183d (other)
```

The exception code, the touched address, the faulting instruction and the first
frames are printed with `game+0x…` / `mod+0x…` offsets where they can be
attributed to a module - enough to tell our code from the game's without a
debugger.

## Common bugs
Thanks to the nature of Highfleet modding being quite unstable you shouldn't
be surprised if your game crashes if you have incorrectly defined your part in
the game.

A part id in `Shipworks.json` that does not exist is reported in the log and
skipped. A part that exists but is malformed (wrong slots, broken meshes,
missing stats in `OL.seria`, ...) is still cloned by the game and can crash it.

The mod writes `Modloader/shipworks.txt` and opens a console window with the
same text. Every line is flushed immediately, so if the game crashes the last
line of the log is the part that caused it.

If you encounter any bugs or crashes not related to parts being wrongly defined or the
configuration file being wrong, please create an issue here on github or message me
directly

## Versioning
The major version will be the same as the major version of the modloader the
mod is made to work with. For example, if the version of the mod is V3.X.X
it will only work on the modloader of version V3.X.X

Minor and patch versions work as normal, being added features and bugfixes
respectively.

## Building from source
To build from source you will need cmake installed.

### Windows
1. Install Visual Studio with C++ support.
2. Clone the repository.
3. Open a terminal in the repository folder.
4. `mkdir build && cd build`
5. `cmake ..`
6. `cmake --build .`

### Linux
1. Install MinGW (e.g. `sudo pacman -S mingw-w64-gcc` on Arch).
2. Clone the repository.
3. Open a terminal in the repository folder.
4. `mkdir build && cd build`
5. `cmake .. -DCMAKE_TOOLCHAIN_FILE=../mingw-toolchain.cmake`
6. `cmake --build .`

## Linux support
This mod is compatible with Linux.
Mods using ShipworksExtended are also compatible (Flight To Hara Vera).

### Prerequisites:
1. **Proton Version:** Tested on dw-proton-10.0-14, in theory should work GE-Proton-10.
2. **Dependencies:** You must install `gdiplus` for the interface to render.
	* `protontricks 1434950 gdiplus`

### Installation:
1. Build from source or download the latest release.
2. Place the `ShipworksExtended.dll` in your `Highfleet/Modloader/mods` directory.
3. Set the following Steam Launch Options to enable Modloader and GDI+:
	```text
	WINEDLLOVERRIDES="dinput8=n,b;gdiplus=n,b;bass=n,b" %command%
	```

## Troubleshooting/Extra
### Game crashes when entering shipworks?
If you are using Linux COMPATIBLE .dll and still crash:
1. Ensure you installed `gdiplus`.
2. Check your launch options or use this abomination:
	`PROTON_LOG=1 WINEDLLOVERRIDES="dinput8=n,b;gdiplus=n,b;d3dcompiler_47=n,b;uiautomationcore=b;bass=n,b;xalia.exe=d;tabtip.exe=d" %command%`

### Possible fix: .NET
If your Proton log (written to /home/user/ by default) is still experiencing crashes and shows errors related to `.NET` or `mscoree.dll`, you may need to manually clean up the Wine prefix.
For some odd reason, uninstalling and trying to install .NET 4.8 raises an error. This might not be the case for you, but if it is, you need to edit the registry.

1. Open Proton's registry editor:
	```bash
	protontricks 1434950 regedit
	```
2. Navigate to: `HKEY_LOCAL_MACHINE\Software\Wow6432Node\Microsoft\NET Framework Setup\NDP`.
3. Look for a folder named `v4`.
4. Right-click it and Delete. Confirm.
5. After that force install .NET 4.8 again: 
	`protontricks 1434950 --force dotnet48`
