/*
 * ThemisDB | File: eviction_policy.h | Version: 0.0.15
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=1; TODO=0, Stub=0, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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
