#include "game.h"

#include <cstring>

namespace {

// --- Game addresses -------------------------------------------------------
constexpr std::uintptr_t kNew = 0x140334094;            // void* New(size_t)
constexpr std::uintptr_t kNodeConstructor = 0x14027f200;  // Node* Node::Node(void*)
constexpr std::uintptr_t kSetLink = 0x140194de0;        // SetLink(Link*, Node*)
constexpr std::uintptr_t kCopyString = 0x1400056e0;     // CopyString(EscadraName*, const char*, size_t)
constexpr std::uintptr_t kDefinePart = 0x14021b190;     // definePart(ALL_PARTS, oid, library, count)

// --- Node layout ----------------------------------------------------------
constexpr std::size_t kNodeSize = 0x128;
constexpr std::size_t kNameOffset = 0xC8;    // EscadraName inside Node
constexpr std::size_t kCountOffset = 0x7a8;  // per-part stock count
constexpr std::size_t kLinkBeforeNode = 8;   // the Link sits right before the Node*
constexpr std::size_t kDestroySlot = 0x48;   // vtable slot of the node destructor
constexpr int kUnlimited = 999;

// --- Body layout ----------------------------------------------------------
constexpr std::size_t kOidOffset = 0x190;
constexpr std::size_t kOidLengthOffset = 0x1a0;
constexpr std::size_t kOidCapacityOffset = 0x1a8;
constexpr std::uint64_t kInlineCapacity = 16;

using NewFn = void* (*)(std::size_t);
using NodeConstructorFn = void* (*)(void*);
using SetLinkFn = void (*)(void* link, void* node);
using CopyStringFn = void (*)(void* dest, const char* src, std::size_t length);
using DefinePartFn = void* (*)(void* parts, const char* oid, void* library, int count);
using DestroyNodeFn = void (*)(void* node);

void* SlotAddress(game::Library library) noexcept {
    return reinterpret_cast<void*>(static_cast<std::uintptr_t>(library));
}

void* AllParts() noexcept {
    return *reinterpret_cast<void**>(game::kAllParts);
}

}  // namespace

void* game::CreateLibraryNode(const char* name) noexcept {
    auto* node = reinterpret_cast<NewFn>(kNew)(kNodeSize);
    if (node != nullptr) {
        node = reinterpret_cast<NodeConstructorFn>(kNodeConstructor)(node);
    }
    if (node == nullptr) {
        return nullptr;
    }
    reinterpret_cast<CopyStringFn>(kCopyString)(
        static_cast<char*>(node) + kNameOffset, name, std::strlen(name));
    return node;
}

void game::DestroyNode(void* node) noexcept {
    if (node == nullptr) {
        return;
    }
    auto** vtable = *static_cast<void***>(node);
    reinterpret_cast<DestroyNodeFn>(vtable[kDestroySlot / sizeof(void*)])(node);
}

void game::CreateLibrary(Library library, const char* name) noexcept {
    // Same as the game's own function: the previous node is destroyed through
    // its vtable slot +0x48, with the node itself in RCX. The old MASM version
    // skipped this call because it passed the address of the global instead of
    // the node, which made the destructor work on garbage.
    DestroyNode(PaletteNode(library));

    void* node = CreateLibraryNode(name);
    if (node == nullptr) {
        return;
    }
    reinterpret_cast<SetLinkFn>(kSetLink)(
        static_cast<char*>(SlotAddress(library)) - kLinkBeforeNode, node);
}

bool game::DefinePartIn(void* library, const char* oid) noexcept {
    void* allParts = AllParts();
    if (library == nullptr || allParts == nullptr) {
        return false;
    }

    void* clone = reinterpret_cast<DefinePartFn>(kDefinePart)(allParts, oid, library, 1);
    if (clone == nullptr) {
        return false;
    }
    *reinterpret_cast<int*>(static_cast<char*>(clone) + kCountOffset) = kUnlimited;
    return true;
}

bool game::DefinePart(Library library, const char* oid) noexcept {
    return DefinePartIn(PaletteNode(library), oid);
}

void* game::PaletteNode(Library library) noexcept {
    return *static_cast<void**>(SlotAddress(library));
}

void game::SetPaletteNode(Library library, void* node) noexcept {
    *static_cast<void**>(SlotAddress(library)) = node;
}

game::PartName game::NameOf(void* body) noexcept {
    auto* base = static_cast<char*>(body);
    if (base == nullptr) {
        return {nullptr, nullptr, 0};
    }
    const auto capacity =
        *reinterpret_cast<std::uint64_t*>(base + kOidCapacityOffset);
    auto* text = capacity < kInlineCapacity
                     ? base + kOidOffset
                     : *reinterpret_cast<char**>(base + kOidOffset);
    return {text,
            reinterpret_cast<std::uint64_t*>(base + kOidLengthOffset),
            capacity};
}

// --- Constructor library walking ------------------------------------------

namespace {

constexpr std::size_t kChildListOffset = 0x100;
constexpr std::size_t kChildPayloadOffset = 0x10;

void* NextChild(void* entry) noexcept {
    return *static_cast<void**>(entry);
}

void* PayloadOf(void* entry) noexcept {
    return *reinterpret_cast<void**>(static_cast<char*>(entry) + kChildPayloadOffset);
}

void** SentinelOf(void* node) noexcept {
    return node == nullptr
               ? nullptr
               : *reinterpret_cast<void***>(static_cast<char*>(node) + kChildListOffset);
}

}  // namespace

void* game::ConstructorLibraryNode() noexcept {
    return *reinterpret_cast<void**>(kConstructorLibrary);
}

std::size_t game::ChildCount(void* node) noexcept {
    void** sentinel = SentinelOf(node);
    if (sentinel == nullptr) {
        return 0;
    }
    std::size_t count = 0;
    for (void* entry = *sentinel; entry != sentinel; entry = NextChild(entry)) {
        ++count;
    }
    return count;
}

void* game::ChildAt(void* node, std::size_t index) noexcept {
    void** sentinel = SentinelOf(node);
    if (sentinel == nullptr) {
        return nullptr;
    }
    std::size_t current = 0;
    for (void* entry = *sentinel; entry != sentinel; entry = NextChild(entry)) {
        if (current == index) {
            return PayloadOf(entry);
        }
        ++current;
    }
    return nullptr;
}