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
    ALL         // All replicas must acknowledge (slowest, highest durability)
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
 * Write Result with replication status
 */
struct WriteResult {
    bool success = false;
    size_t replicas_acknowledged = 0;
    size_t replicas_required = 0;
    std::string error;
    std::chrono::milliseconds latency{0};
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
        default:
            return 1;
    }
}

} // namespace themis::sharding
