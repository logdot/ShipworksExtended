#pragma once

#include <cstddef>

namespace mem {

// Overwrites `size` bytes at `dst` with `src`, making the page writable for the
// copy and restoring its previous protection afterwards. Returns false when the
// page could not be unprotected; nothing is written in that case.
bool Patch(void* dst, const void* src, std::size_t size) noexcept;

}  // namespace mem
