/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            eviction_policy.h                                  ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-04-15 05:33:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     69                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 5f3f466a9e  2026-02-24  feat(cache): add configurable eviction policies (LFU, ARC... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
