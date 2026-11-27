#include "menus.h"

#include <array>
#include <cstdint>
#include <cstring>

#include "game.h"
#include "log.h"

namespace {

struct Entry {
    std::string button;
    void* node = nullptr;
    std::vector<void*> paletteBodies;  // resolved once, when the library is loaded
};

std::vector<Entry> g_entries;
std::vector<std::string> g_opened;

// The game matches a palette button against the 14 hardcoded keys in
// BottomMenu_ (0x14020d820). Instead of patching that chain, the button's own
// oid is renamed to the hull key for the duration of one call, and the hull
// library slot is pointed at the custom node. The game then builds its usual
// sub-list, from our parts, with its own layout and purchase path.
constexpr const char* kVanillaKey = "ITEM_HULL";
constexpr std::size_t kVanillaKeyLength = 9;

std::string_view OidOf(void* body) {
    const game::PartName name = game::NameOf(body);
    if (name.text == nullptr || name.length == nullptr) {
        return {};
    }
    return std::string_view(name.text, static_cast<std::size_t>(*name.length));
}

}  // namespace

void menus::Rebuild(const std::vector<Definition>& definitions) {
    for (Entry& entry : g_entries) {
        game::DestroyNode(entry.node);
    }
    g_entries.clear();
    g_opened.clear();

    for (const Definition& definition : definitions) {
        void* node = game::CreateLibraryNode(definition.button.c_str());
        if (node == nullptr) {
            logging::Line("error: could not create a node for custom menu \"" +
                          definition.button + "\"");
            continue;
        }

        std::size_t added = 0;
        for (const std::string& oid : definition.parts) {
            if (game::DefinePartIn(node, oid.c_str())) {
                ++added;
            } else {
                logging::Line("error: could not add \"" + oid + "\" to custom menu \"" +
                              definition.button + "\" - no such part id");
            }
        }

        g_entries.push_back({definition.button, node});
        Entry& created = g_entries.back();

        // Resolve the palette bodies that carry this oid now, while the library
        // is known to be loaded. The click guard must never walk it later: the
        // click handler is also called while the screen is being built.
        if (void* library = game::ConstructorLibraryNode()) {
            const std::size_t count = game::ChildCount(library);
            for (std::size_t index = 0; index < count; ++index) {
                void* child = game::ChildAt(library, index);
                const game::PartName name = game::NameOf(child);
                if (name.text == nullptr || name.length == nullptr) {
                    continue;
                }
                if (std::string_view(name.text, static_cast<std::size_t>(*name.length)) ==
                    created.button) {
                    created.paletteBodies.push_back(child);
                }
            }
        }

        logging::Line("custom menu \"" + definition.button + "\": " +
                      std::to_string(added) + " of " +
                      std::to_string(definition.parts.size()) + " parts, " +
                      std::to_string(created.paletteBodies.size()) + " button(s)");
        if (created.paletteBodies.empty()) {
            logging::Line("warning: no palette entry is named \"" + definition.button +
                          "\"; its clicks cannot be guarded");
        }
    }
}

bool menus::Open(void* hoveredNode, void* body, SubListFn original) {
    if (original == nullptr || body == nullptr) {
        return false;
    }

    const std::string_view oid = OidOf(body);
    if (oid.empty()) {
        return false;
    }

    const Entry* entry = nullptr;
    for (const Entry& candidate : g_entries) {
        if (candidate.button == oid && candidate.node != nullptr) {
            entry = &candidate;
            break;
        }
    }
    if (entry == nullptr) {
        return false;
    }

    const game::PartName name = game::NameOf(body);
    if (name.text == nullptr || name.length == nullptr ||
        name.capacity < kVanillaKeyLength + 1) {
        logging::Line("warning: custom menu \"" + entry->button +
                      "\" has an unusable button body; falling back to the game");
        return false;
    }

    const std::uint64_t savedLength = *name.length;
    std::array<char, 16> saved{};
    std::memcpy(saved.data(), name.text, kVanillaKeyLength + 1);

    void* const savedHull = game::PaletteNode(game::Library::Hull);
    game::SetPaletteNode(game::Library::Hull, entry->node);
    std::memcpy(name.text, kVanillaKey, kVanillaKeyLength + 1);
    *name.length = kVanillaKeyLength;

    original(hoveredNode, body);

    std::memcpy(name.text, saved.data(), kVanillaKeyLength + 1);
    *name.length = savedLength;
    game::SetPaletteNode(game::Library::Hull, savedHull);

    // Bookkeeping only; never let it break the call we just made.
    try {
        bool alreadyLogged = false;
        for (const std::string& opened : g_opened) {
            alreadyLogged = alreadyLogged || opened == entry->button;
        }
        if (!alreadyLogged) {
            g_opened.push_back(entry->button);
            logging::Line("opened custom menu \"" + entry->button + "\"");
        }
    } catch (...) {
    }
    return true;
}

// --- Click guard ----------------------------------------------------------

namespace {

constexpr std::size_t kMaxRenames = 16;

struct Renamed {
    char* text;
    std::uint64_t* length;
    std::uint64_t savedLength;
    std::array<char, 16> saved;
};

std::array<Renamed, kMaxRenames> g_renamed;
std::size_t g_renamedCount = 0;

}  // namespace

void menus::BeginClick() {
    g_renamedCount = 0;

    for (const Entry& entry : g_entries) {
        for (void* body : entry.paletteBodies) {
            if (g_renamedCount >= kMaxRenames) {
                return;
            }
            game::PartName name = game::NameOf(body);
            if (name.text == nullptr || name.length == nullptr ||
                name.capacity < kVanillaKeyLength + 1) {
                continue;
            }

            Renamed& renamed = g_renamed[g_renamedCount];
            renamed.text = name.text;
            renamed.length = name.length;
            renamed.savedLength = *name.length;
            std::memcpy(renamed.saved.data(), name.text, kVanillaKeyLength + 1);
            std::memcpy(name.text, kVanillaKey, kVanillaKeyLength + 1);
            *name.length = kVanillaKeyLength;
            ++g_renamedCount;
        }
    }
}

void menus::EndClick() {
    for (std::size_t index = 0; index < g_renamedCount; ++index) {
        const Renamed& renamed = g_renamed[index];
        std::memcpy(renamed.text, renamed.saved.data(), kVanillaKeyLength + 1);
        *renamed.length = renamed.savedLength;
    }
    g_renamedCount = 0;
}
