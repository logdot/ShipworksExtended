#pragma once

#include <cstdint>

// Crash reporting in the spirit of hf_ally's diag: log the faulting address and
// instruction while the process is still alive, so the game's own error.log
// ("ACCESS_VIOLATION: address 1") is not the only evidence.
namespace crash {

// `gameBase` and `modBase` are only used to print module-relative addresses.
void Install(std::uintptr_t gameBase, std::uintptr_t modBase);

}  // namespace crash
