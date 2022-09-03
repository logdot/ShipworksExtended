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