/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            hlc.cpp                                            ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:38:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     142                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 71d99c4f28  2026-04-14  fix(concurrency): eliminate deadlocks, blocking I/O under... ║
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
    : state_(wallClockMs() << HLCTimestamp::LOGICAL_BITS) {}

uint64_t HybridLogicalClock::wallClockMs() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
}

// advanceTo is a helper for the CAS loop in now() – not called externally.
// Computes the next HLCTimestamp value given current packed state and wall clock.
HLCTimestamp HybridLogicalClock::advanceTo(uint64_t phys_ms) {
    // CAS loop: atomically advance the packed (physical||logical) state.
    uint64_t cur = state_.load(std::memory_order_relaxed);
    for (;;) {
        uint64_t cur_phys = cur >> HLCTimestamp::LOGICAL_BITS;
        uint32_t cur_log  = static_cast<uint32_t>(cur & HLCTimestamp::LOGICAL_MASK);

        uint64_t new_phys;
        uint32_t new_log;
        if (phys_ms > cur_phys) {
            new_phys = phys_ms;
            new_log  = 0;
        } else {
            new_phys = cur_phys;
            new_log  = cur_log + 1;
            if (new_log > HLCTimestamp::MAX_LOGICAL) {
                ++new_phys;
                new_log = 0;
            }
        }

        uint64_t desired = (new_phys << HLCTimestamp::LOGICAL_BITS) | new_log;
        if (state_.compare_exchange_weak(cur, desired,
                                         std::memory_order_acq_rel,
                                         std::memory_order_relaxed)) {
            return HLCTimestamp{desired};
        }
        // cur has been updated by compare_exchange_weak on failure – retry.
    }
}

HLCTimestamp HybridLogicalClock::now() {
    return advanceTo(wallClockMs());
}

HLCTimestamp HybridLogicalClock::update(HLCTimestamp received) {
    uint64_t cur = state_.load(std::memory_order_relaxed);
    for (;;) {
        uint64_t local_phys = wallClockMs();
        uint64_t max_phys   = std::max(local_phys, received.physical());

        uint64_t cur_phys = cur >> HLCTimestamp::LOGICAL_BITS;
        uint32_t cur_log  = static_cast<uint32_t>(cur & HLCTimestamp::LOGICAL_MASK);

        uint64_t new_phys;
        uint32_t new_log;
        if (max_phys == cur_phys) {
            // Both local and remote share the same physical millisecond.
            new_phys = cur_phys;
            new_log  = std::max(cur_log, received.logical()) + 1;
            if (new_log > HLCTimestamp::MAX_LOGICAL) {
                ++new_phys;
                new_log = 0;
            }
        } else if (max_phys == received.physical() && max_phys > cur_phys) {
            // Received message has a newer physical timestamp.
            new_phys = max_phys;
            new_log  = received.logical() + 1;
            if (new_log > HLCTimestamp::MAX_LOGICAL) {
                ++new_phys;
                new_log = 0;
            }
        } else {
            // Local wall clock is strictly ahead.
            new_phys = max_phys;
            new_log  = 0;
        }

        uint64_t desired = (new_phys << HLCTimestamp::LOGICAL_BITS) | new_log;
        if (state_.compare_exchange_weak(cur, desired,
                                         std::memory_order_acq_rel,
                                         std::memory_order_relaxed)) {
            return HLCTimestamp{desired};
        }
        // cur was refreshed by CAS failure – retry with new wall clock sample.
    }
}

HLCTimestamp HybridLogicalClock::peek() const {
    return HLCTimestamp{state_.load(std::memory_order_acquire)};
}

} // namespace themis
