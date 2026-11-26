#pragma once

#include <string>
#include <string_view>
#include <vector>

// Custom part menus: the construct.seria button whose m_oid is `button` opens a
// sub-list built from `parts` instead of anything the game would show.
namespace menus {

struct Definition {
    std::string button;
    std::vector<std::string> parts;
};

using SubListFn = void (*)(void* hoveredNode, void* hoveredBody);

// Rebuilds every custom menu. Must run after the game's ALL_PARTS exists and
// before the screen is drawn, which is what the palette hook already guarantees.
void Rebuild(const std::vector<Definition>& definitions);

// If `body` is one of the configured buttons, lets the game build the usual
// sub-list but sourced from the custom node, then puts everything back.
// Returns false for every other body, so the caller can call the game directly.
bool Open(void* hoveredNode, void* body, SubListFn original);

// The click handler has its own early-out for the 14 palette entries: a click on
// one of them must not pick the body up as a part. It keys on the oid, so the
// button bodies are renamed to a known key for the duration of one click.
void BeginClick();
void EndClick();

}  // namespace menus
