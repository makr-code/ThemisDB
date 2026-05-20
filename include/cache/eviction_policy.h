/*
 * ThemisDB | File: eviction_policy.h | Version: 0.0.15 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 54
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=n/a | delta=n/a | status=no_external_data
 * External Severity (v3): C=n/a, H=n/a, M=n/a
 * PR: #2794 feat(cache): Configurable eviction policies beyond LRU (LFU, ARC) f... (2026-03-12T06:01:32Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include "core/concerns/eviction_strategies.h"
#include <memory>

namespace themis {
namespace cache {

/**
 * @brief Eviction policy selector for AdaptiveQueryCache tiers.
 *
 * Selects the victim-selection algorithm used by L1 and L2 cache tiers.
 * Defaults to LRU for backward compatibility.
 */
enum class EvictionPolicy {
    LRU,  ///< Least Recently Used
    LFU,  ///< Least Frequently Used
    ARC   ///< Adaptive Replacement Cache (scan-resistant, self-tuning)
};

/**
 * @brief Factory: create an IEvictionStrategy for the given policy.
 *
 * @param policy       The desired eviction policy.
 * @param arc_capacity Capacity hint passed to ARCEvictionStrategy; ignored for
 *                     LRU and LFU policies.
 * @return Owning pointer to a new strategy instance.
 */
inline std::unique_ptr<core::concerns::IEvictionStrategy>
makeEvictionStrategy(EvictionPolicy policy, size_t arc_capacity = 128) {
    switch (policy) {
        case EvictionPolicy::LFU:
            return std::make_unique<core::concerns::LFUEvictionStrategy>();
        case EvictionPolicy::ARC:
            return std::make_unique<core::concerns::ARCEvictionStrategy>(arc_capacity);
        case EvictionPolicy::LRU:
        default:
            return std::make_unique<core::concerns::LRUEvictionStrategy>();
    }
}

} // namespace cache
} // namespace themis
