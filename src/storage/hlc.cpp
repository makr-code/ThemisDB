/*
 * ThemisDB | File: hlc.cpp | Version: 0.0.47 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 128
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=36 | delta=33 | status=divergent
 * External Severity (v3): C=0, H=29, M=7
 * PR: #1320 Integrate MVCC and HLC timestamping for versioned data and consiste... (2026-03-11T21:28:06Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
