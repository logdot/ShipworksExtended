#pragma once

#include <cstddef>
#include <cstdint>

// Thin wrappers over the handful of game functions this mod calls.
// All addresses are from the Steam 1.163 build of Highfleet.exe (image base
// 0x140000000); the mod refuses to patch anything else.
namespace game {

// ALL_PARTS: the campaign copy of parts.seria. The global holds a Node*.
constexpr std::uintptr_t kAllParts = 0x143a139f0;

// PARTS_CONSTRUCTOR_SERIA: the library the constructor screen builds its cells
// from (Libraries/constructor.seria, or parts.seria in the other mode).
constexpr std::uintptr_t kConstructorLibrary = 0x1403ef4f8;

// The shipworks palette is 14 globals, each a smart-ref Link immediately
// followed by the Node* it points at. The game's own DefineEditorParts
// (0x1400d0ac0) builds them in this order.
enum class Library : std::uintptr_t {
    Hull        = 0x1403bd400,
    Armor       = 0x1403bd418,
    Fuel        = 0x1403bd430,
    Ammo        = 0x1403bd448,
    Leg         = 0x1403bd460,
    Engine      = 0x1403bd478,
    Gun         = 0x1403bd490,
    Sensor      = 0x1403bd4a8,
    Expendables = 0x1403bd4c0,
    Craft       = 0x1403bd4d8,
    Defences    = 0x1403bd4f0,
    Generator   = 0x1403bd508,
    Quarters    = 0x1403bd520,
    Emergency   = 0x1403bd538,
};

// Replaces a palette slot with a fresh, empty node named `name`, destroying the
// previous node the same way the game does.
void CreateLibrary(Library library, const char* name) noexcept;

// A node on its own (not published through one of the 14 slots).
void* CreateLibraryNode(const char* name) noexcept;

// Destroys a node the way the game does (vtable slot +0x48).
void DestroyNode(void* node) noexcept;

// Clones a part out of ALL_PARTS into a palette slot with unlimited stock.
// Returns false when the slot is empty or the game does not know `oid`; that is
// the difference between a skipped entry and a crash.
bool DefinePart(Library library, const char* oid) noexcept;

// Same, into a node returned by CreateLibraryNode.
bool DefinePartIn(void* library, const char* oid) noexcept;

// The Node* behind one of the 14 slots.
void* PaletteNode(Library library) noexcept;
void SetPaletteNode(Library library, void* node) noexcept;

// A part's m_oid: an EscadraName at Body+0x190 with a 16-byte inline buffer, the
// length at +0x1a0 and the capacity at +0x1a8 (the buffer holds a pointer when
// the capacity is 16 or more).
struct PartName {
    char* text;
    std::uint64_t* length;
    std::uint64_t capacity;
};

PartName NameOf(void* body) noexcept;

// Diagnostic walk of the constructor library: the same child list BottomMenu_
// reads at Node+0x100 (entry: next at +0, payload at +0x10).
void* ConstructorLibraryNode() noexcept;
std::size_t ChildCount(void* node) noexcept;
void* ChildAt(void* node, std::size_t index) noexcept;

}  // namespace game
