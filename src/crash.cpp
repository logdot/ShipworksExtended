#include "crash.h"

#include <windows.h>

#include <cstdint>
#include <cstdio>

#include "log.h"

namespace {

std::uintptr_t g_gameBase = 0;
std::uintptr_t g_modBase = 0;

// A backtrace, so a crash report says where it came from and not only which
// address was touched. kernel32 exports this on Windows 7 and later.
constexpr unsigned long kFrames = 20;

void Append(char* buffer, std::size_t size, std::size_t& used, const char* text) {
    while (*text != '\0' && used + 1 < size) {
        buffer[used++] = *text++;
    }
    buffer[used] = '\0';
}

void AppendHex(char* buffer, std::size_t size, std::size_t& used, std::uintptr_t value) {
    char text[32] = {};
    std::snprintf(text, sizeof text, "0x%llx", static_cast<unsigned long long>(value));
    Append(buffer, size, used, text);
}

const char* ModuleOf(std::uintptr_t address) {
    if (address >= g_gameBase && address < g_gameBase + 0x4000000) {
        return "game";
    }
    if (address >= g_modBase && address < g_modBase + 0x1000000) {
        return "mod";
    }
    return "other";
}

void AppendRelative(char* buffer, std::size_t size, std::size_t& used, std::uintptr_t address) {
    Append(buffer, size, used, " (");
    Append(buffer, size, used, ModuleOf(address));
    if (address >= g_gameBase && address < g_gameBase + 0x4000000) {
        Append(buffer, size, used, "+");
        AppendHex(buffer, size, used, address - g_gameBase);
    } else if (address >= g_modBase && address < g_modBase + 0x1000000) {
        Append(buffer, size, used, "+");
        AppendHex(buffer, size, used, address - g_modBase);
    }
    Append(buffer, size, used, ")");
}

LONG WINAPI OnException(EXCEPTION_POINTERS* info) {
    const auto* record = info->ExceptionRecord;
    if (record == nullptr || info->ContextRecord == nullptr) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    switch (record->ExceptionCode) {
        case EXCEPTION_ACCESS_VIOLATION:
        case EXCEPTION_IN_PAGE_ERROR:
        case EXCEPTION_ILLEGAL_INSTRUCTION:
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
        case EXCEPTION_STACK_OVERFLOW:
            break;
        default:
            return EXCEPTION_CONTINUE_SEARCH;
    }

    char text[768] = {};
    std::size_t used = 0;

    Append(text, sizeof text, used, "CRASH code=");
    AppendHex(text, sizeof text, used, record->ExceptionCode);

    if ((record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION ||
         record->ExceptionCode == EXCEPTION_IN_PAGE_ERROR) &&
        record->NumberParameters >= 2) {
        Append(text, sizeof text, used, " operation=");
        AppendHex(text, sizeof text, used, record->ExceptionInformation[0]);
        Append(text, sizeof text, used, " touched=");
        AppendHex(text, sizeof text, used, record->ExceptionInformation[1]);
        AppendRelative(text, sizeof text, used, record->ExceptionInformation[1]);
    }

    const auto rip = static_cast<std::uintptr_t>(info->ContextRecord->Rip);
    Append(text, sizeof text, used, " rip=");
    AppendHex(text, sizeof text, used, rip);
    AppendRelative(text, sizeof text, used, rip);
    logging::Raw(text);

    void* frames[kFrames] = {};
    const USHORT count = CaptureStackBackTrace(0, kFrames, frames, nullptr);
    for (USHORT index = 0; index < count; ++index) {
        char line[192] = {};
        std::size_t lineUsed = 0;
        Append(line, sizeof line, lineUsed, "CRASH stack ");
        AppendHex(line, sizeof line, lineUsed, index);
        Append(line, sizeof line, lineUsed, ": ");
        const auto address = reinterpret_cast<std::uintptr_t>(frames[index]);
        AppendHex(line, sizeof line, lineUsed, address);
        AppendRelative(line, sizeof line, lineUsed, address);
        logging::Raw(line);
    }

    // Let the game's own handler write error.log as usual.
    return EXCEPTION_CONTINUE_SEARCH;
}

}  // namespace

void crash::Install(std::uintptr_t gameBase, std::uintptr_t modBase) {
    g_gameBase = gameBase;
    g_modBase = modBase;
    AddVectoredExceptionHandler(1, OnException);
}
