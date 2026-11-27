#pragma once

#include <cstdint>

// Five-byte `call`s are redirected through a bridge that is allocated inside the
// +-2 GB window of the call site, because the mod's own DLL is mapped far away
// and cannot be reached with a rel32.
namespace hook {

// Rewrites the `call` at `site` so that it lands in `detour` instead. The
// original target is verified first; `site` must still hold `call expectedTarget`.
bool InstallCall(std::uintptr_t site, void* detour, std::uintptr_t expectedTarget);

// Writes every original instruction back. The bridge pages are deliberately
// left allocated: another thread may still be inside one.
void RemoveAll();

}  // namespace hook
