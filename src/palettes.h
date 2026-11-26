#pragma once

#include <cstddef>

#include "game.h"

// The 14 shipworks palettes, in the order the game's own DefineEditorParts
// builds them. That order is also the order the parts appear in the popup menu,
// so do not sort or reorder this table.
namespace palettes {

constexpr std::size_t kCount = 14;

struct PartIds {
    const char* const* ids;
    std::size_t count;
};

struct Palette {
    game::Library library;
    const char* nodeName;     // name the game gives the node
    const char* jsonKey;      // Shipworks.json array with extra part ids
    const char* vanillaFlag;  // "LoadVanilla<X>" switch for the vanilla list
    PartIds vanilla;          // what the game itself puts in this palette
};

extern const Palette kAll[kCount];

// Recreates all 14 palette nodes, empty and unlinking the previous ones.
void CreateLibraries() noexcept;

// Adds the vanilla list of one palette.
void DefineVanilla(const Palette& palette) noexcept;

}  // namespace palettes
