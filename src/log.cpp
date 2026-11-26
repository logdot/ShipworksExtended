#include "log.h"

#include <windows.h>

#include <cstdio>
#include <cstring>

namespace {

constexpr char kLogPath[] = "Modloader/shipworks.txt";

HANDLE g_file = INVALID_HANDLE_VALUE;
bool g_console = false;
bool g_consoleAllocated = false;
LARGE_INTEGER g_frequency = {};
LARGE_INTEGER g_start = {};

double Elapsed() {
    if (g_frequency.QuadPart == 0) {
        return 0.0;
    }
    LARGE_INTEGER now = {};
    QueryPerformanceCounter(&now);
    return static_cast<double>(now.QuadPart - g_start.QuadPart) /
           static_cast<double>(g_frequency.QuadPart);
}

void WriteFileRaw(const char* data, std::size_t size) noexcept {
    if (g_file == INVALID_HANDLE_VALUE || size == 0) {
        return;
    }
    DWORD written = 0;
    WriteFile(g_file, data, static_cast<DWORD>(size), &written, nullptr);
    FlushFileBuffers(g_file);
}

}  // namespace

void logging::Open() noexcept {
    // The game has no console of its own; a console that is already attached
    // (launched from a terminal, or another mod) is reused instead.
    if (GetConsoleWindow() == nullptr) {
        g_consoleAllocated = AllocConsole() != FALSE;
    }

    FILE* console = nullptr;
#if defined(_MSC_VER)
    if (freopen_s(&console, "CONOUT$", "w", stdout) != 0) {
        console = nullptr;
    }
#else
    // MinGW: freopen_s exists, but freopen is the portable spelling.
    console = freopen("CONOUT$", "w", stdout);
#endif
    g_console = console != nullptr;

    g_file = CreateFileA(kLogPath, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
                         nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

    QueryPerformanceFrequency(&g_frequency);
    QueryPerformanceCounter(&g_start);
}

void logging::Line(std::string_view text) noexcept {
    char prefix[32] = {};
    std::snprintf(prefix, sizeof prefix, "[+%9.3f] ", Elapsed());

    if (g_console) {
        std::fputs(prefix, stdout);
        std::fwrite(text.data(), 1, text.size(), stdout);
        std::fputc('\n', stdout);
        std::fflush(stdout);
    }

    WriteFileRaw(prefix, std::strlen(prefix));
    WriteFileRaw(text.data(), text.size());
    WriteFileRaw("\n", 1);
}

void logging::Raw(const char* text) noexcept {
    WriteFileRaw(text, std::strlen(text));
    WriteFileRaw("\n", 1);
    if (g_console) {
        std::fputs(text, stdout);
        std::fputc('\n', stdout);
        std::fflush(stdout);
    }
}

void logging::Close() noexcept {
    if (g_file != INVALID_HANDLE_VALUE) {
        CloseHandle(g_file);
        g_file = INVALID_HANDLE_VALUE;
    }
    if (g_console) {
        std::fflush(stdout);
    }
    if (g_consoleAllocated) {
        FreeConsole();
    }
    g_console = false;
    g_consoleAllocated = false;
}
