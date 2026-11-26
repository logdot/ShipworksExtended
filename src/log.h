#pragma once

#include <string_view>

// Console plus Modloader/shipworks.txt, one flushed line at a time, with the
// elapsed time in front of every line (crash reports are useless without it).
// `Raw` exists for the crash handler: no allocation, no locks, no std::string.
namespace logging {

void Open() noexcept;
void Line(std::string_view text) noexcept;
void Raw(const char* text) noexcept;
void Close() noexcept;

}  // namespace logging
