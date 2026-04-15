/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            write_concern.h                                    ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:09:35                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     98                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <cstdint>
#include <string>
#include <chrono>

namespace themis::sharding {

/**
 * Write Concern Level
 * Specifies how many replicas must acknowledge a write before it's considered successful
 */
enum class WriteConcern {
    ONE,        // Only primary must acknowledge (fastest, lowest durability)
    MAJORITY,   // Majority of replicas must acknowledge (balanced)
    ALL,        // All replicas must acknowledge (slowest, highest durability)
    QUORUM      // Configurable quorum (use write_quorum)
};

/**
 * Write Concern Configuration
 */
struct WriteConcernConfig {
    WriteConcern level = WriteConcern::ONE;
    std::chrono::milliseconds timeout{5000};  // 5 second default timeout
    bool wait_for_sync = true;                // Wait for disk sync on replicas
};

/**
 * Parse WriteConcern from string
 */
inline WriteConcern parseWriteConcern(const std::string& str) {
    if (str == "ONE" || str == "one" || str == "1") {
        return WriteConcern::ONE;
    } else if (str == "MAJORITY" || str == "majority") {
        return WriteConcern::MAJORITY;
    } else if (str == "ALL" || str == "all") {
        return WriteConcern::ALL;
    } else if (str == "QUORUM" || str == "quorum" || str == "Q") {
        return WriteConcern::QUORUM;
    }
    return WriteConcern::ONE; // default
}

/**
 * Convert WriteConcern to string
 */
inline std::string toString(WriteConcern wc) {
    switch (wc) {
        case WriteConcern::ONE: return "ONE";
        case WriteConcern::MAJORITY: return "MAJORITY";
        case WriteConcern::ALL: return "ALL";
        case WriteConcern::QUORUM: return "QUORUM";
        default: return "ONE";
    }
}

/**
 * Calculate required replica count for given concern
 */
inline size_t calculateRequiredReplicas(WriteConcern concern, size_t total_replicas) {
    switch (concern) {
        case WriteConcern::ONE:
            return 1; // Primary only
        case WriteConcern::MAJORITY:
            return (total_replicas / 2) + 1; // Quorum
        case WriteConcern::ALL:
            return total_replicas; // All replicas
        case WriteConcern::QUORUM:
            return (total_replicas / 2) + 1; // Default quorum
        default:
            return 1;
    }
}

} // namespace themis::sharding
