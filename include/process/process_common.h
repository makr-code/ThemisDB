/*
 * ThemisDB | File: process_common.h | Version: 0.0.1
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/*
 * Shared utilities for process module implementations
 */
#pragma once

#include <chrono>
#include <cstdint>

namespace themis::process {

/**
 * @brief Get current wall-clock time in milliseconds since Unix epoch.
 *
 * @return Current time in milliseconds
 */
inline int64_t nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

} // namespace themis::process
