#include "palettes.h"

namespace {

// --- Vanilla palettes -----------------------------------------------------
// Copied verbatim from the game's DefineEditorParts, including its order.

const char* const kHulls[] = {
    "MDL_DECK_01",
    "MDL_HARD2X2_04",
    "MDL_HARD2X2_03",
    "MDL_HARD2X2_02",
    "MDL_HARD2X2_01",
    "MDL_HARD2X1_01",
    "MDL_HARD1X1_04",
    "MDL_HARD1X1_01",
    "MDL_FERMA2X2_04",
    "MDL_FERMA2X2_03",
    "MDL_FERMA2X2_02",
    "MDL_FERMA2X2_01",
    "MDL_FERMA2X1_01",
    "MDL_FERMA1X1_04",
    "MDL_FERMA1X1_01",
    "MDL_FERMA4X4_01",
};

const char* const kArmors[] = {
    "MDL_ARMOR2X1_01",
    "MDL_ARMOR1X1_04",
    "MDL_ARMOR1X1_03",
    "MDL_ARMOR1X1_02",
    "MDL_ARMOR1X1_01",
};

const char* const kFuels[] = {
    "MDL_FUEL_02",
    "MDL_FUEL_01",
};

const char* const kAmmos[] = {
    "MDL_AMMO_02",
    "MDL_AMMO",
};

const char* const kLegs[] = {
    "MDL_LEG_04",
    "MDL_LEG_03",
    "MDL_LEG_02",
    "MDL_LEG_01",
};

const char* const kEngines[] = {
    "MDL_ENGINE_03",
    "MDL_ENGINE_04",
    "MDL_ENGINE_05",
    "MDL_ENGINE_01",
    "MDL_ENGINE_02",
};

const char* const kGuns[] = {
    "MDL_CANNON_30_6",
    "MDL_CANNON_57_2",
    "MDL_CANNON_100_2",
    "MDL_CANNON_130",
    "MDL_CANNON_180",
    "MDL_CANNON_180_2",
    "MDL_CANNON_305_2",
    "MDL_RSZO_220",
};

const char* const kSensors[] = {
    "MDL_RADAR_01",
    "MDL_RADAR_02",
    "MDL_SPO_01",
    "MDL_SPO_02",
    "MDL_FCR_01",
    "MDL_FCR_02",
    "MDL_IRST_01",
    "MDL_JAMMER_01",
    "MDL_ANTENNA_01",
};

const char* const kExpendables[] = {
    "MDL_MISSILE_01",
    "MDL_MISSILE_03",
    "MDL_BOMB_01",
    "MDL_NUKE_01_CONV",
    "MDL_NUKE_02_CONV",
    "MDL_NUKE_03_CONV",
    "MDL_NUKE_04_CONV",
    "MDL_NUKE_01",
    "MDL_NUKE_02",
    "MDL_NUKE_03",
    "MDL_NUKE_04",
};

const char* const kCrafts[] = {
    "CRAFT_LA29",
    "CRAFT_T7",
};

const char* const kDefences[] = {
    "MDL_FLARES",
    "MDL_KAZ",
};

const char* const kGenerators[] = {
    "MDL_GENERATOR_01",
    "MDL_GENERATOR_02",
};

const char* const kQuarters[] = {
    "MDL_QUARTERS_01",
    "MDL_QUARTERS_02",
};

const char* const kEmergency[] = {
    "MDL_FSS_02",
    "MDL_EVAC",
};

template <std::size_t N>
constexpr palettes::PartIds List(const char* const (&ids)[N]) noexcept {
    return {ids, N};
}

}  // namespace

const palettes::Palette palettes::kAll[palettes::kCount] = {
    {game::Library::Hull,        "EDITOR_LIBRARY_HULL",        "Hulls",       "LoadVanillaHulls",       List(kHulls)},
    {game::Library::Armor,       "EDITOR_LIBRARY_ARMOR",       "Armors",      "LoadVanillaArmors",      List(kArmors)},
    {game::Library::Fuel,        "EDITOR_LIBRARY_FUEL",        "Fuels",       "LoadVanillaFuels",       List(kFuels)},
    {game::Library::Ammo,        "EDITOR_LIBRARY_AMMO",        "Ammos",       "LoadVanillaAmmos",       List(kAmmos)},
    {game::Library::Leg,         "EDITOR_LIBRARY_LEG",         "Legs",        "LoadVanillaLegs",        List(kLegs)},
    {game::Library::Engine,      "EDITOR_LIBRARY_ENGINES",     "Engines",     "LoadVanillaEngines",     List(kEngines)},
    {game::Library::Gun,         "EDITOR_LIBRARY_GUNS",        "Guns",        "LoadVanillaGuns",        List(kGuns)},
    {game::Library::Sensor,      "EDITOR_LIBRARY_SENSORS",     "Sensors",     "LoadVanillaSensors",     List(kSensors)},
    {game::Library::Expendables, "EDITOR_LIBRARY_EXPENDABLES", "Expendables", "LoadVanillaExpendables", List(kExpendables)},
    {game::Library::Craft,       "EDITOR_LIBRARY_CRAFTS",      "Crafts",      "LoadVanillaCrafts",      List(kCrafts)},
    {game::Library::Defences,    "EDITOR_LIBRARY_DEFENCE",     "Defences",    "LoadVanillaDefences",    List(kDefences)},
    {game::Library::Generator,   "EDITOR_LIBRARY_GENERATORS",  "Generators",  "LoadVanillaGenerators",  List(kGenerators)},
    // The game names the quarters node EDITOR_LIBRARY_GENERATORS as well; the
    // name is only a label, so it is kept identical to vanilla.
    {game::Library::Quarters,    "EDITOR_LIBRARY_GENERATORS",  "Quarters",    "LoadVanillaQuarters",    List(kQuarters)},
    {game::Library::Emergency,   "EDITOR_LIBRARY_EMERGENCY",   "Emergency",   "LoadVanillaEmergency",   List(kEmergency)},
};

void palettes::CreateLibraries() noexcept {
    for (const Palette& palette : kAll) {
        game::CreateLibrary(palette.library, palette.nodeName);
    }
}

void palettes::DefineVanilla(const Palette& palette) noexcept {
    for (std::size_t i = 0; i < palette.vanilla.count; ++i) {
        game::DefinePart(palette.library, palette.vanilla.ids[i]);
    }
}
