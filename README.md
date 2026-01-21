# ShipworksExtended
Is a mod made for the [Highfleet-Modloader](https://github.com/logdot/Highfleet-Modloader)
that allows you to define parts to be loaded into the shipworks.

## Using
To use the mod simply go into the [latest release](https://github.com/logdot/ShipworksExtended/releases/latest)
and drag and drop the contents of the Artifacts.zip file into the highfleet
folder

## Defining parts
Defining parts to be added into the shipworks is really simple. You only need
to add the part's m_oid into the mod's configuration file at `Modloader/config/Shipworks.json`.
You can edit the configuration file while the game is running and it's changes
will be aplied the next time you enter the shipworks.

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

## Common bugs
Thanks to the nature of Highfleet modding being quite unstable you shouldn't
be surprised if your game crashes if you have wrongly defined the shipworks.json
file or incorrectly defined your part in the game.

Trying to load a part that doesn't exist or is malconfigured will normally
result in a crash.

If you encounter any bugs or crashes not related to parts being wrongly defined or the
configuration file being wrong, please create an issue here on github or message me
directly

## Versioning
The major version will be the same as the major version of the modloader the
mod is made to work with. For example, if the version of the mod is V3.X.X
it will only work on the modloader of version V3.X.X

Minor and patch versions work as normal, being added features and bugfixes
respectively.

## Linux support

This mod is compatible with Linux.
Mods using ShipworksExtended are also compatible (Flight To Hara Vera).


### Prerequisites:
1. **Proton Version:** Tested on dw-proton-10.0-14, in theory should work GE-Proton-10.
2. **Dependencies:** You must install `gdiplus` for the interface to render.
	* `protontricks 1434950 gdiplus`

### Installation:
1. Download the latest release (or build from source using `./build_linux.sh`).
2. Place the `ShipworksExtended.dll` in your `Highfleet/Modloader/mods` directory.
3. Set the following Steam Launch Options to enable Modloader and GDI+:
	```text
	WINEDLLOVERRIDES="dinput8=n,b;gdiplus=n,b;bass=n,b" %command%
	```
### Building from source

You can compile the mod natively on Linux using MinGW:
1. Install MinGW (e.g. `sudo pacman -S mingw-w64-gcc` on Arch).
2. Run the build script:
	```bash
	./build_linux.sh
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


