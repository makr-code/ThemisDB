/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            hlc.cpp                                            ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:30:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     115                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "storage/hlc.h"
#include <chrono>
#include <sstream>
#include <algorithm>

namespace themis {

// ─────────────────────────────────────────────────────────────────────────────
// HLCTimestamp helpers
// ─────────────────────────────────────────────────────────────────────────────

std::string HLCTimestamp::toString() const {
    std::ostringstream oss;
    oss << physical() << '.' << logical();
    return oss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// HybridLogicalClock
// ─────────────────────────────────────────────────────────────────────────────

HybridLogicalClock::HybridLogicalClock()
    : last_physical_ms_(wallClockMs()), logical_(0) {}

uint64_t HybridLogicalClock::wallClockMs() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
}

HLCTimestamp HybridLogicalClock::advanceTo(uint64_t phys_ms) {
    // Called with mutex held.
    if (phys_ms > last_physical_ms_) {
        last_physical_ms_ = phys_ms;
        logical_ = 0;
    } else {
        // Wall clock did not advance – bump the logical counter.
        ++logical_;
        if (logical_ > HLCTimestamp::MAX_LOGICAL) {
            // Overflow: advance the physical component by 1 ms to make room.
            ++last_physical_ms_;
            logical_ = 0;
        }
    }
    return HLCTimestamp::from(last_physical_ms_, logical_);
}

HLCTimestamp HybridLogicalClock::now() {
    std::lock_guard<std::mutex> lock(mutex_);
    return advanceTo(wallClockMs());
}

HLCTimestamp HybridLogicalClock::update(HLCTimestamp received) {
    std::lock_guard<std::mutex> lock(mutex_);
    // Advance to the maximum of the local wall clock and the received physical ts.
    uint64_t local_phys = wallClockMs();
    uint64_t max_phys   = std::max(local_phys, received.physical());

    if (max_phys == last_physical_ms_) {
        // Both local and remote share the same physical millisecond.
        // Logical = max(local_logical, received_logical) + 1.
        logical_ = std::max(logical_, received.logical()) + 1;
        if (logical_ > HLCTimestamp::MAX_LOGICAL) {
            ++last_physical_ms_;
            logical_ = 0;
        }
    } else if (max_phys == received.physical() && max_phys > last_physical_ms_) {
        // Received message has a newer physical timestamp.
        last_physical_ms_ = max_phys;
        logical_ = received.logical() + 1;
        if (logical_ > HLCTimestamp::MAX_LOGICAL) {
            ++last_physical_ms_;
            logical_ = 0;
        }
    } else {
        // Local wall clock is strictly ahead.
        last_physical_ms_ = max_phys;
        logical_ = 0;
    }
    return HLCTimestamp::from(last_physical_ms_, logical_);
}

HLCTimestamp HybridLogicalClock::peek() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return HLCTimestamp::from(last_physical_ms_, logical_);
}

} // namespace themis
