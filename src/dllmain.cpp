// ShipworksExtended - loads extra parts into the shipworks palettes.
//
// The mod replaces the game's DefineEditorParts (0x1400d0ac0) with its own
// palette builder: the 14 editor libraries are recreated on every entry into
// the shipworks and filled from Shipworks.json, so edits to the file apply the
// next time the screen is opened.

#include <windows.h>

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <exception>
#include <fstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "crash.h"
#include "game.h"
#include "hook.h"
#include "json.hpp"
#include "log.h"
#include "mem.h"
#include "menus.h"
#include "palettes.h"

namespace {

using json = nlohmann::json;

constexpr char kConfigPath[] = "Modloader/config/Shipworks.json";

std::uintptr_t DetourLayout(void* element);

// --- The patch ------------------------------------------------------------
// DefineEditorParts begins with
//
//   push rbx / sub rsp,0x20
//   mov  rcx,[EDITOR_LIBRARY_HULL]
//   cmp  byte [LOAD_CONSTRUCTOR_OR_EDITOR],0
//   jz   0x1400d1c05          ; constructor flag clear -> game's other path
//   test rcx,rcx
//   0x1400d0add: jz 0x1400d0ae5   <-- the mod replaces this instruction
//
// so the mod only replaces the palette build used by the constructor screen;
// the menu-editor/shop path is jumped to before the patch site and is never
// affected. The 22 bytes from 0x1400d0add are replaced with:
//
//   call qword ptr [rip+2]   ; -> ReloadPalettes
//   jmp  +8
//   <absolute address of ReloadPalettes>
//   add rsp, 0x20 / pop rbx / ret
//
// i.e. the vanilla body never runs, and the game's own epilogue still does.
//
// The 22 overwritten bytes are saved before patching and written back on eject,
// so the restore cannot drift from the image: the shipped version of this mod
// restored a hardcoded array whose `call New` displacement did not match the
// 1.163 image at all. The site is checked before anything is written: the two
// opcode groups must match and the call must still resolve to the allocator.
constexpr std::uintptr_t kPatchRva = 0x1400d0add - 0x140000000;
constexpr std::uintptr_t kNewRva = 0x140334094 - 0x140000000;
constexpr std::size_t kOriginalSize = 22;

// Part menus (the flyout a palette button opens) are built by BottomMenu_
// 0x14020d820, which mainMap calls from exactly one site. The call is redirected
// so a custom button can be served before the game's own oid switch runs.
constexpr std::uintptr_t kBottomMenuRva = 0x14020d820 - 0x140000000;
constexpr std::uintptr_t kBottomMenuCallRva = 0x14014074a - 0x140000000;

// The click handler has its own early-out for the 14 palette entries, so that a
// click on a palette button does not pick the body up as a part. HandleClick
// calls it once, from 0x14012d168.
constexpr std::uintptr_t kClickRva = 0x14012f120 - 0x140000000;
constexpr std::uintptr_t kClickCallRva = 0x14012d168 - 0x140000000;

// ObjectList vtable +0x348 (ObjectList_SetupGridLayout) and its current target.
constexpr std::uintptr_t kLayoutSlotRva = 0x140362050 - 0x140000000;
constexpr std::uintptr_t kLayoutOriginalRva = 0x14012e1e0 - 0x140000000;
const std::uintptr_t kDetourLayoutAddress =
    reinterpret_cast<std::uintptr_t>(&DetourLayout);

// jz +6 / mov rax,[rcx] / call [rax+0x48] / mov ecx,0x128
constexpr std::array<unsigned char, 13> kOriginalHead = {
    0x74, 0x06, 0x48, 0x8b, 0x01, 0xff, 0x50, 0x48, 0xb9, 0x28, 0x01, 0x00, 0x00,
};
// call rel32
constexpr std::size_t kCallOpcode = 13;
constexpr std::size_t kCallOperand = 14;
constexpr std::size_t kAfterCall = 18;
// mov [rsp+0x30],rax
constexpr std::array<unsigned char, 4> kOriginalTail = {0x48, 0x89, 0x44, 0x24};

constexpr std::array<unsigned char, 8> kCallPatch = {
    0xff, 0x15, 0x02, 0x00, 0x00, 0x00, 0xeb, 0x08,
};
constexpr std::array<unsigned char, 6> kEpiloguePatch = {
    0x48, 0x83, 0xc4, 0x20, 0x5b, 0xc3,
};

struct Patch {
    bool installed = false;
    unsigned char* site = nullptr;
    std::array<unsigned char, kOriginalSize> original{};
};

std::string Hex(std::uintptr_t value) {
    char text[24] = {};
    std::snprintf(text, sizeof text, "0x%llx", static_cast<unsigned long long>(value));
    return text;
}

std::string Bytes(const unsigned char* data, std::size_t size) {
    std::string text;
    char byte[4] = {};
    for (std::size_t i = 0; i < size; ++i) {
        std::snprintf(byte, sizeof byte, "%02x ", data[i]);
        text += byte;
    }
    return text;
}

// --- Custom menus ---------------------------------------------------------

#if defined(_MSC_VER)
#define SHIPWORKS_RETURN_ADDRESS() _ReturnAddress()
#else
#define SHIPWORKS_RETURN_ADDRESS() __builtin_return_address(0)
#endif

using BottomMenuFn = void (*)(void* hoveredNode, void* hoveredBody);

const auto kBottomMenu = reinterpret_cast<BottomMenuFn>(0x14020d820);

std::vector<menus::Definition> ReadCustomMenus(const json& config) {
    std::vector<menus::Definition> definitions;

    const auto entry = config.find("CustomMenus");
    if (entry == config.end()) {
        return definitions;
    }
    if (!entry->is_array()) {
        logging::Line("warning: \"CustomMenus\" must be an array; ignored");
        return definitions;
    }

    for (const json& element : *entry) {
        if (!element.is_object()) {
            logging::Line("warning: \"CustomMenus\" contains something that is not an object; skipped");
            continue;
        }

        const auto button = element.find("Button");
        if (button == element.end() || !button->is_string() ||
            button->get<std::string>().empty()) {
            logging::Line("warning: a custom menu has no \"Button\" part id; skipped");
            continue;
        }

        menus::Definition definition;
        definition.button = button->get<std::string>();

        const auto parts = element.find("Parts");
        if (parts != element.end()) {
            if (!parts->is_array()) {
                logging::Line("warning: the \"Parts\" of custom menu \"" + definition.button +
                              "\" must be an array; skipped");
                continue;
            }
            for (const json& part : *parts) {
                if (part.is_string()) {
                    definition.parts.push_back(part.get<std::string>());
                } else {
                    logging::Line("warning: the \"Parts\" of custom menu \"" + definition.button +
                                  "\" contain something that is not a part id; skipped");
                }
            }
        }

        definitions.push_back(std::move(definition));
    }

    return definitions;
}

void DetourBottomMenu(void* hoveredNode, void* hoveredBody) {
    static bool reportedFirstHover = false;
    if (!reportedFirstHover) {
        reportedFirstHover = true;
        try {
            logging::Line("detour: part menu entered from " +
                          Hex(reinterpret_cast<std::uintptr_t>(SHIPWORKS_RETURN_ADDRESS())));
            const game::PartName name = game::NameOf(hoveredBody);
            if (name.text != nullptr && name.length != nullptr) {
                logging::Line("part menu: first hover on \"" +
                              std::string(name.text, static_cast<std::size_t>(*name.length)) +
                              "\" (" + std::to_string(*name.length) + " chars, capacity " +
                              std::to_string(name.capacity) + ")");
            }
        } catch (...) {
        }
    }

    if (menus::Open(hoveredNode, hoveredBody, kBottomMenu)) {
        return;
    }
    kBottomMenu(hoveredNode, hoveredBody);
}

// --- Hook switches --------------------------------------------------------
// Read once at startup from Shipworks.json, so a hook can be taken out without a
// rebuild while one of them is misbehaving.
struct HookSwitches {
    bool partMenu = true;
    bool click = true;
    bool layout = true;
};

HookSwitches ReadHookSwitches() {
    HookSwitches switches;

    std::ifstream stream(kConfigPath);
    if (!stream.is_open()) {
        return switches;
    }
    const json config = json::parse(stream, nullptr, /*allow_exceptions=*/false);
    if (config.is_discarded() || !config.is_object()) {
        return switches;
    }
    const auto hooks = config.find("Hooks");
    if (hooks == config.end() || !hooks->is_object()) {
        return switches;
    }

    auto read = [&](const char* key, bool& value) {
        const auto entry = hooks->find(key);
        if (entry == hooks->end() || !entry->is_boolean()) {
            return;
        }
        value = entry->get<bool>();
    };
    read("PartMenu", switches.partMenu);
    read("Click", switches.click);
    read("Layout", switches.layout);
    return switches;
}

// --- Palette row geometry -------------------------------------------------
// ObjectList_SetupGridLayout (0x14012e1e0, ObjectList vtable +0x348) places
// exactly `columns * rows` cells and skips every cell past that, which is why a
// 15th palette entry is loaded but never shown. The grid is inline element state
// (columns: dword +0xee0, rows: dword +0xee4), so the row is widened by writing
// those fields just before the game lays the row out. The vtable slot holds a
// plain data pointer, so this hook needs no bridge.
//
// Deliberately not FindGuiElement: it walks a list of elements while the screen
// is still building them and faults on a half-initialised entry (address 1).

using LayoutFn = std::uintptr_t (*)(void*);

LayoutFn const kOriginalLayout = reinterpret_cast<LayoutFn>(0x14012e1e0);
bool g_rowFitted = false;
std::size_t g_paletteEntries = 0;

std::string_view ElementName(void* element, std::size_t offset) {
    auto* base = static_cast<char*>(element);
    const auto capacity = *reinterpret_cast<std::uint64_t*>(base + offset + 0x18);
    const auto length = *reinterpret_cast<std::uint64_t*>(base + offset + 0x10);
    const char* text = capacity < 16 ? base + offset : *reinterpret_cast<char**>(base + offset);
    return std::string_view(text, static_cast<std::size_t>(length));
}

// The palette row is the ObjectList whose owner link (+0xe70, node pointer at
// +0xe78) is the constructor library - DrawShipworksScreen does
// `+0x360(PARTS_CONSTRUCTOR_SERIA)` right after creating it. That is a stronger
// identity than the name: the constructor only sets the class name "ObjectList"
// at +8, and these elements carry no instance name.
void* OwnerOf(void* element) {
    return *reinterpret_cast<void**>(static_cast<char*>(element) + 0xe78);
}

bool IsPaletteRow(void* element) {
    void* owner = OwnerOf(element);
    return owner != nullptr && owner == game::ConstructorLibraryNode();
}

std::uintptr_t DetourLayout(void* element) {
    static bool reportedFirstLayout = false;
    if (!reportedFirstLayout) {
        reportedFirstLayout = true;
        logging::Line("detour: layout entered from " +
                      Hex(reinterpret_cast<std::uintptr_t>(SHIPWORKS_RETURN_ADDRESS())));
    }

    // A few owner samples, so a mismatch is diagnosable: the palette row is the
    // one whose owner is the constructor library.
    static int ownerSamples = 0;
    if (ownerSamples < 6) {
        void* owner = OwnerOf(element);
        if (owner != nullptr) {
            ++ownerSamples;
            try {
                logging::Line("layout owner: " + Hex(reinterpret_cast<std::uintptr_t>(owner)) +
                              " library " +
                              Hex(reinterpret_cast<std::uintptr_t>(
                                  game::ConstructorLibraryNode())));
            } catch (...) {
            }
        }
    }

    if (!g_rowFitted && IsPaletteRow(element)) {
        try {
            auto* base = static_cast<char*>(element);
            const int columns = *reinterpret_cast<int*>(base + 0xee0);
            const int rows = *reinterpret_cast<int*>(base + 0xee4);
            const float spacing = *reinterpret_cast<float*>(base + 0xee8);
            const float extentX = *reinterpret_cast<float*>(base + 0x218);

            auto* cellsBegin = *reinterpret_cast<char**>(base + 0xeb8);
            auto* cellsEnd = *reinterpret_cast<char**>(base + 0xec0);
            const std::size_t cells =
                (cellsBegin != nullptr && cellsEnd != nullptr && cellsEnd > cellsBegin)
                    ? static_cast<std::size_t>(cellsEnd - cellsBegin) / 0x18
                    : 0;

            float cellSize = 0.0f;
            if (cells > 0) {
                auto* cellGui = *reinterpret_cast<char**>(cellsBegin + 8);
                if (cellGui != nullptr) {
                    cellSize = *reinterpret_cast<float*>(cellGui + 0x218);
                }
            }

            logging::Line("palette row: " + std::to_string(cells) + " cells, grid " +
                          std::to_string(columns) + "x" + std::to_string(rows) +
                          ", cell " + std::to_string(cellSize) + ", extent " +
                          std::to_string(extentX) + ", spacing " + std::to_string(spacing));

            // The row starts at cellSize/2 - extentX/2 and steps right by
            // cellSize + spacing, so exactly extentX/cellSize cells fit inside the
            // element; the rest are placed past its edge and never seen. Widen the
            // element to hold them at the game's own spacing - compressing the
            // step instead would close the gaps between the cells.
            if (cells > 1 && cellSize > 20.0f && cellSize < 400.0f) {
                const float needed = static_cast<float>(cells) * cellSize;
                if (needed > extentX + 0.5f) {
                    *reinterpret_cast<float*>(base + 0x218) = needed;
                    logging::Line("palette row: extent " + std::to_string(extentX) + " -> " +
                                  std::to_string(needed) + " for " + std::to_string(cells) +
                                  " cells");
                }
                g_rowFitted = true;
            }
        } catch (...) {
        }
    }
    return kOriginalLayout(element);
}

using ClickFn = void (*)(void* a, void* b, int c, float d);

const auto kClickFunc = reinterpret_cast<ClickFn>(0x14012f120);

void DetourClick(void* a, void* b, int c, float d) {
    static bool reportedFirstClick = false;
    if (!reportedFirstClick) {
        reportedFirstClick = true;
        logging::Line("detour: click entered from " +
                      Hex(reinterpret_cast<std::uintptr_t>(SHIPWORKS_RETURN_ADDRESS())));
    }
    menus::BeginClick();
    kClickFunc(a, b, c, d);
    menus::EndClick();
}

void ReportConstructorLibrary() {
    void* node = game::ConstructorLibraryNode();
    if (node == nullptr) {
        logging::Line("palette: no constructor library loaded");
        return;
    }

    const std::size_t count = game::ChildCount(node);
    g_paletteEntries = count;
    logging::Line("palette: " + std::to_string(count) + " entries");
    for (std::size_t index = 0; index < count && index < 40; ++index) {
        const game::PartName name = game::NameOf(game::ChildAt(node, index));
        if (name.text == nullptr || name.length == nullptr) {
            logging::Line("  [" + std::to_string(index) + "] <no name>");
            continue;
        }
        logging::Line("  [" + std::to_string(index) + "] " +
                      std::string(name.text, static_cast<std::size_t>(*name.length)));
    }
}

// --- Config ---------------------------------------------------------------

bool ReadFlag(const json& config, std::string_view key) {
    const auto entry = config.find(key);
    if (entry == config.end()) {
        return false;
    }
    if (!entry->is_boolean()) {
        logging::Line("warning: \"" + std::string(key) + "\" must be true or false; ignored");
        return false;
    }
    return entry->get<bool>();
}

void AddConfiguredParts(const json& config, const palettes::Palette& palette) {
    const auto entry = config.find(palette.jsonKey);
    if (entry == config.end()) {
        return;
    }
    if (!entry->is_array()) {
        logging::Line("warning: \"" + std::string(palette.jsonKey) +
                      "\" must be an array of part ids; ignored");
        return;
    }

    for (const json& element : *entry) {
        if (!element.is_string()) {
            logging::Line("warning: \"" + std::string(palette.jsonKey) +
                          "\" contains something that is not a part id; skipped");
            continue;
        }
        const std::string oid = element.get<std::string>();
        if (game::DefinePart(palette.library, oid.c_str())) {
            logging::Line("added " + oid + " to " + palette.nodeName);
        } else {
            logging::Line("error: could not add \"" + oid + "\" to " + palette.nodeName +
                          " - no such part id");
        }
    }
}

void WarnAboutUnknownKeys(const json& config) {
    for (const auto& item : config.items()) {
        const std::string key = item.key();
        bool known = key == "LoadVanilla" || key == "CustomMenus" || key == "Hooks";
        for (const palettes::Palette& palette : palettes::kAll) {
            known = known || key == palette.jsonKey || key == palette.vanillaFlag;
        }
        if (!known) {
            logging::Line("warning: unknown setting \"" + key + "\"");
        }
    }
}

void ReportConfigPath() {
    char absolute[2 * MAX_PATH] = {};
    const DWORD length = GetFullPathNameA(kConfigPath, static_cast<DWORD>(sizeof absolute),
                                         absolute, nullptr);
    if (length == 0 || length >= sizeof absolute) {
        logging::Line(std::string("config: ") + kConfigPath);
        return;
    }
    logging::Line(std::string("config: ") + absolute);
}

// Reads Shipworks.json every time the shipworks is entered. A missing or broken
// file is reported and leaves the palettes empty instead of crashing the game.
void LoadConfig() {
    std::vector<menus::Definition> customMenus;

    std::ifstream stream(kConfigPath);
    if (!stream.is_open()) {
        logging::Line("error: " + std::string(kConfigPath) +
                      " is missing; the palettes stay empty");
        menus::Rebuild(customMenus);
        return;
    }

    const json config = json::parse(stream, nullptr, /*allow_exceptions=*/false);
    if (config.is_discarded() || !config.is_object()) {
        logging::Line("error: " + std::string(kConfigPath) +
                      " is not valid JSON; the palettes stay empty");
        menus::Rebuild(customMenus);
        return;
    }

    WarnAboutUnknownKeys(config);

    const bool loadAllVanilla = ReadFlag(config, "LoadVanilla");
    for (const palettes::Palette& palette : palettes::kAll) {
        if (loadAllVanilla || ReadFlag(config, palette.vanillaFlag)) {
            palettes::DefineVanilla(palette);
        }
        AddConfiguredParts(config, palette);
    }

    customMenus = ReadCustomMenus(config);
    menus::Rebuild(customMenus);
}

// The injected function. It runs in place of the constructor-screen palette
// build, on the game thread, with the vanilla prologue's stack frame in place.
#if defined(_MSC_VER)
#define SHIPWORKS_NOINLINE __declspec(noinline)
#else
#define SHIPWORKS_NOINLINE __attribute__((noinline))
#endif

void SHIPWORKS_NOINLINE ReloadPalettes() {
    logging::Line("reloading palettes");
    try {
        palettes::CreateLibraries();
        LoadConfig();
        ReportConstructorLibrary();
        g_rowFitted = false;
    } catch (const std::exception& error) {
        // Allocation-free on purpose: this runs on the game thread.
        logging::Line(error.what());
    } catch (...) {
        logging::Line("error: unknown failure while building the shipworks palettes");
    }
}

// --- Thread ---------------------------------------------------------------

void PrintBanner() {
    logging::Line("----------------------------------------------------");
    logging::Line("/\\/\\/     Welcome to ShipworksExtended     \\/\\/\\");
    logging::Line("----------------------------------------------------");
    logging::Line("");
    logging::Line("Extra parts are read from " + std::string(kConfigPath) + ",");
    logging::Line("every time the shipworks is opened.");
    logging::Line("");
    logging::Line("If the game crashes while entering the shipworks, check this log.");
    logging::Line("A wrong part id is reported here; a malformed part can still crash");
    logging::Line("the game when it clones it.");
    logging::Line("");
    logging::Line("Issues: https://github.com/logdot/ShipworksExtended");
    logging::Line("");
}

Patch Install(std::uintptr_t moduleBase) {
    Patch patch;
    patch.site = reinterpret_cast<unsigned char*>(moduleBase + kPatchRva);
    auto* site = patch.site;

    if (std::memcmp(site, kOriginalHead.data(), kOriginalHead.size()) != 0 ||
        std::memcmp(site + kAfterCall, kOriginalTail.data(), kOriginalTail.size()) != 0 ||
        site[kCallOpcode] != 0xe8) {
        logging::Line("error: the code at " + Hex(moduleBase + kPatchRva) +
                      " is not the expected one; nothing was changed.");
        logging::Line("error: found " + Bytes(site, kOriginalSize));
        logging::Line("error: this build of Highfleet is not supported.");
        return patch;
    }

    // The displacement is checked instead of hardcoded: it depends on the build.
    std::int32_t displacement = 0;
    std::memcpy(&displacement, site + kCallOperand, sizeof displacement);
    const auto called = reinterpret_cast<std::uintptr_t>(site + kAfterCall) +
                        static_cast<std::uintptr_t>(static_cast<std::intptr_t>(displacement));
    if (called != moduleBase + kNewRva) {
        logging::Line("error: the call at " + Hex(moduleBase + kPatchRva) +
                      " does not go to the allocator; nothing was changed.");
        logging::Line("error: this build of Highfleet is not supported.");
        return patch;
    }

    std::memcpy(patch.original.data(), site, kOriginalSize);

    const std::uintptr_t target = reinterpret_cast<std::uintptr_t>(&ReloadPalettes);
    const bool written =
        mem::Patch(site, kCallPatch.data(), kCallPatch.size()) &&
        mem::Patch(site + kCallPatch.size(), &target, sizeof target) &&
        mem::Patch(site + kCallPatch.size() + sizeof target,
                   kEpiloguePatch.data(), kEpiloguePatch.size());
    if (!written) {
        mem::Patch(site, patch.original.data(), patch.original.size());
        logging::Line("error: could not write the patch; nothing was changed.");
        return patch;
    }

    patch.installed = true;
    logging::Line("patched DefineEditorParts at " + Hex(moduleBase + kPatchRva));
    return patch;
}

void Remove(const Patch& patch) {
    if (patch.installed) {
        mem::Patch(patch.site, patch.original.data(), patch.original.size());
    }
}

void Run() {
    PrintBanner();
    ReportConfigPath();

    const auto moduleBase = reinterpret_cast<std::uintptr_t>(GetModuleHandle(nullptr));
    crash::Install(moduleBase, reinterpret_cast<std::uintptr_t>(&PrintBanner));
    const Patch patch = Install(moduleBase);
    if (!patch.installed) {
        return;
    }

    const HookSwitches switches = ReadHookSwitches();
    logging::Line(std::string("hook switches: part menu ") +
                  (switches.partMenu ? "on" : "off") + ", click " +
                  (switches.click ? "on" : "off") + ", layout " +
                  (switches.layout ? "on" : "off"));

    bool hooked = false;
    if (switches.partMenu) {
        hooked = hook::InstallCall(moduleBase + kBottomMenuCallRva,
                                   reinterpret_cast<void*>(&DetourBottomMenu),
                                   moduleBase + kBottomMenuRva);
        if (hooked) {
            logging::Line("hooked the part menu at " + Hex(moduleBase + kBottomMenuCallRva));
        } else {
            logging::Line("warning: the part menu could not be hooked");
        }
    }

    bool clickHooked = false;
    if (switches.click) {
        clickHooked = hook::InstallCall(moduleBase + kClickCallRva,
                                        reinterpret_cast<void*>(&DetourClick),
                                        moduleBase + kClickRva);
        if (clickHooked) {
            logging::Line("hooked the palette click at " + Hex(moduleBase + kClickCallRva));
        } else {
            logging::Line("warning: the palette click could not be hooked; a custom button can "
                          "be picked up as a part");
        }
    }

    // ObjectList's layout virtual, patched in the vtable itself (data pointer, no
    // bridge needed). Used to widen the palette row before it is laid out.
    const std::uintptr_t layoutSlot = moduleBase + kLayoutSlotRva;
    bool layoutInstalled = false;
    if (switches.layout) {
        const auto before = *reinterpret_cast<std::uintptr_t*>(layoutSlot);
        logging::Line("layout slot holds " + Hex(before) + ", detour is " +
                      Hex(kDetourLayoutAddress));
        layoutInstalled = before == moduleBase + kLayoutOriginalRva &&
                          mem::Patch(reinterpret_cast<void*>(layoutSlot),
                                     &kDetourLayoutAddress, sizeof kDetourLayoutAddress);
        if (layoutInstalled) {
            logging::Line("hooked the palette layout at " + Hex(layoutSlot) + ", now " +
                          Hex(*reinterpret_cast<std::uintptr_t*>(layoutSlot)));
        } else {
            logging::Line("warning: the palette layout could not be hooked; the row shows the "
                          "palette as the game builds it");
        }
    }

    logging::Line("");
    logging::Line("Press END to eject the mod.");
    while ((GetAsyncKeyState(VK_END) & 1) == 0) {
        Sleep(10);
    }

    if (layoutInstalled) {
        const std::uintptr_t original = moduleBase + kLayoutOriginalRva;
        mem::Patch(reinterpret_cast<void*>(layoutSlot), &original, sizeof original);
    }
    hook::RemoveAll();
    Remove(patch);
    logging::Line("Ejected.");
}

DWORD WINAPI MainThread(LPVOID module) {
    const auto self = static_cast<HMODULE>(module);

    logging::Open();
    try {
        Run();
    } catch (const std::exception& error) {
        logging::Line(error.what());
    } catch (...) {
        logging::Line("error: unexpected failure in the ShipworksExtended thread");
    }
    logging::Close();

    FreeLibraryAndExitThread(self, 0);
}

}  // namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        const HANDLE thread = CreateThread(nullptr, 0, MainThread, module, 0, nullptr);
        if (thread != nullptr) {
            CloseHandle(thread);
        }
    }
    return TRUE;
}
