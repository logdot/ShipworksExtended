#include "hook.h"

#include <windows.h>

#include <array>
#include <cstdint>
#include <cstring>

#include "mem.h"

namespace {

struct Site {
    std::uintptr_t address = 0;
    std::array<unsigned char, 5> original{};
};

std::array<Site, 4> g_sites;

// The detour is called from a page close to the game, so the call site's rel32
// can reach it. The site is patched with a `call`, so the bridge is entered
// exactly like the original callee: the return address is already on the stack.
// The bridge pads by 8 before calling the detour and drops it afterwards, which
// keeps the detour's own prologue 16-byte aligned.
//
//   48 83 EC 08              sub  rsp, 8
//   FF 15 06 00 00 00        call qword ptr [rip+6]   ; the address at +0x10
//   48 83 C4 08              add  rsp, 8
//   C3                       ret
//   90                       nop
//   <8-byte address>
constexpr std::array<unsigned char, 16> kBridge = {
    0x48, 0x83, 0xEC, 0x08, 0xFF, 0x15, 0x06, 0x00,
    0x00, 0x00, 0x48, 0x83, 0xC4, 0x08, 0xC3, 0x90,
};
constexpr std::size_t kBridgeSize = 32;  // 16 bytes of code + the pointer

void* AllocateNear(std::uintptr_t site) {
    constexpr std::uintptr_t kStep = 0x100000;      // 1 MiB
    constexpr std::uintptr_t kReach = 0x70000000;   // < 2 GiB, so rel32 fits

    for (std::uintptr_t delta = kStep; delta < kReach; delta += kStep) {
        for (const int direction : {-1, 1}) {
            const auto hint = static_cast<std::uintptr_t>(
                static_cast<std::intptr_t>(site) + direction * static_cast<std::intptr_t>(delta));
            if (hint < 0x10000) {
                continue;
            }
            void* page = VirtualAlloc(reinterpret_cast<void*>(hint), kBridgeSize,
                                      MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
            if (page != nullptr) {
                const auto pageAddress = reinterpret_cast<std::uintptr_t>(page);
                const auto distance = static_cast<std::int64_t>(pageAddress) -
                                      static_cast<std::int64_t>(site);
                if (distance > INT32_MIN && distance < INT32_MAX) {
                    return page;
                }
                VirtualFree(page, 0, MEM_RELEASE);
            }
        }
    }
    return nullptr;
}

}  // namespace

bool hook::InstallCall(std::uintptr_t site, void* detour, std::uintptr_t expectedTarget) {
    Site* slot = nullptr;
    for (Site& candidate : g_sites) {
        if (candidate.address == 0) {
            slot = &candidate;
            break;
        }
    }
    if (slot == nullptr) {
        return false;
    }

    auto* code = reinterpret_cast<unsigned char*>(site);
    if (code[0] != 0xE8) {
        return false;
    }
    std::int32_t displacement = 0;
    std::memcpy(&displacement, code + 1, sizeof displacement);
    const auto target = static_cast<std::uintptr_t>(
        static_cast<std::intptr_t>(site + 5) + displacement);
    if (target != expectedTarget) {
        return false;
    }

    auto* bridge = static_cast<unsigned char*>(AllocateNear(site));
    if (bridge == nullptr) {
        return false;
    }
    std::memcpy(bridge, kBridge.data(), kBridge.size());
    std::memcpy(bridge + kBridge.size(), &detour, sizeof detour);
    FlushInstructionCache(GetCurrentProcess(), bridge, kBridgeSize);

    const auto jump = static_cast<std::int64_t>(reinterpret_cast<std::uintptr_t>(bridge)) -
                      static_cast<std::int64_t>(site + 5);
    if (jump < INT32_MIN || jump > INT32_MAX) {
        VirtualFree(bridge, 0, MEM_RELEASE);
        return false;
    }

    std::array<unsigned char, 5> patch = {0xE8, 0, 0, 0, 0};
    const auto relative = static_cast<std::int32_t>(jump);
    std::memcpy(patch.data() + 1, &relative, sizeof relative);

    std::memcpy(slot->original.data(), code, slot->original.size());
    if (!mem::Patch(code, patch.data(), patch.size())) {
        slot->original.fill(0);
        return false;
    }
    slot->address = site;
    return true;
}

void hook::RemoveAll() {
    for (Site& site : g_sites) {
        if (site.address == 0) {
            continue;
        }
        mem::Patch(reinterpret_cast<void*>(site.address), site.original.data(),
                   site.original.size());
        site.address = 0;
    }
}
