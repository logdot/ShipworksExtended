#include "mem.h"

#include <windows.h>

#include <cstring>

bool mem::Patch(void* dst, const void* src, std::size_t size) noexcept {
    DWORD previous = 0;
    if (VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &previous) == FALSE) {
        return false;
    }

    std::memcpy(dst, src, size);
    FlushInstructionCache(GetCurrentProcess(), dst, size);

    DWORD ignored = 0;
    VirtualProtect(dst, size, previous, &ignored);
    return true;
}
