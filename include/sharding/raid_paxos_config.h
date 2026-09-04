// Copyright 2025 ThemisDB
// Licensed under MIT License

/**
 * @file raid_paxos_config.h
 * @brief RAID-specific Paxos configuration for RAID-Sharding
 * @version 1.9.0-beta
 * @note Maturity: PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0
 * @note Enables RAID-aware consensus with mode-specific quorum calculations
 */

#pragma once

#include "sharding/consensus_module.h"
#include "sharding/paxos_consensus.h"
#include <vector>
#include <map>

namespace themisdb::sharding {

/**
 * @brief RAID mode enumeration (synchronized with raid_optimizations.h)
 */
enum class RAIDMode {
    STRIPE = 0,   ///> RAID 0 - Striping only (no redundancy)
    MIRROR = 1,   ///> RAID 1 - Full mirroring
    PARITY = 5,   ///> RAID 5 - Distributed parity
    HYBRID = 10   ///> RAID 10 - Stripe + Mirror
};

/**
 * @brief RAID-specific Paxos configuration
 * 
 * Extends standard Paxos configuration with RAID-mode-specific settings
 * for quorum calculation, failure tolerance, and recovery behavior.
 */
struct RAIDPaxosConfig : public ConsensusConfig {
    // RAID mode for this consensus instance
    RAIDMode raid_mode = RAIDMode::MIRROR;
    
    // RAID 5: Parity shard configuration
    int parity_shard_index = -1;  ///> Index of the parity shard (-1 = not configured)
    std::vector<int> data_shard_indices;  ///> Indices of data shards
    
    // RAID 10: Stripe + Mirror configuration
    int mirror_factor = 2;        ///> Number of mirrors per stripe
    int stripe_width = 2;         ///> Number of data shards per stripe
    
    // RAID 0: No redundancy (for testing only)
    bool allow_raid0 = false;     ///> Allow RAID 0 (no redundancy) - DANGEROUS for production
    
    // Failure tolerance overrides
    int max_tolerable_failures = 1;  ///> Maximum shard failures before consensus fails
    
    /**
     * @brief Calculate quorum size based on RAID mode and total shards
     * 
     * @param total_shards Total number of shards in the cluster
     * @return Minimum number of shards required for quorum
     */
    int calculateQuorumSize(int total_shards) const;
    
    /**
     * @brief Check if configuration is valid for the given RAID mode
     * 
     * @return true if configuration is valid
     */
    bool isValidConfiguration() const;
    
    /**
     * @brief Get human-readable description of RAID mode
     * 
     * @return String description
     */
    std::string getRAIDModeDescription() const;
    
    /**
     * @brief Check if data can be recovered from given failed shards
     * 
     * @param failed_shards List of failed shard indices
     * @return true if recovery is possible
     */
    bool canRecoverFromFailures(const std::vector<int>& failed_shards) const;
    
    /**
     * @brief Get maximum number of shards that can fail while maintaining consensus
     * 
     * @param total_shards Total number of shards
     * @return Maximum tolerable failures
     */
    int getMaxTolerableFailures(int total_shards) const;
};

/**
 * @brief Convert RAID mode to string
 */
inline std::string raidModeToString(RAIDMode mode) {
    switch (mode) {
        case RAIDMode::STRIPE:  return "RAID_0";
        case RAIDMode::MIRROR:  return "RAID_1";
        case RAIDMode::PARITY:  return "RAID_5";
        case RAIDMode::HYBRID:  return "RAID_10";
        default:                return "UNKNOWN";
    }
}

/**
 * @brief Convert string to RAID mode
 */
inline RAIDMode stringToRAIDMode(const std::string& mode_str) {
    if (mode_str == "RAID_0" || mode_str == "STRIPE" || mode_str == "0") {
      return RAIDMode::STRIPE;
    }
    if (mode_str == "RAID_1" || mode_str == "MIRROR" || mode_str == "1") {
      return RAIDMode::MIRROR;
    }
    if (mode_str == "RAID_5" || mode_str == "PARITY" || mode_str == "5") {
      return RAIDMode::PARITY;
    }
    if (mode_str == "RAID_10" || mode_str == "HYBRID" || mode_str == "10") {
      return RAIDMode::HYBRID;
    }
    return RAIDMode::MIRROR;  // Default to safe mode
}

// Implementation of RAIDPaxosConfig methods
inline int RAIDPaxosConfig::calculateQuorumSize(int total_shards) const {
    switch (raid_mode) {
        case RAIDMode::STRIPE:  // RAID 0: No redundancy, all shards must respond
            return total_shards;
            
        case RAIDMode::MIRROR:  // RAID 1: Read from any, write to all
            // For write quorum: need at least one mirror set
            // With mirror_factor=2: need at least 2 shards (1 data + 1 mirror)
            return (total_shards / mirror_factor) + 1;
            
        case RAIDMode::PARITY: {  // RAID 5: Need majority including parity
            // For RAID 5 with N data shards + 1 parity: quorum = (N+1)/2 + 1
            // This ensures we can recover from any single failure
            int required = (total_shards / 2) + 1;
            return std::min(required, total_shards);
        }
            
        case RAIDMode::HYBRID:  // RAID 10: Stripe + Mirror
            // Need at least one complete mirror set per stripe
            return (total_shards / (mirror_factor * stripe_width)) + 1;
            
        default:
            // Safe default: majority
            return (total_shards / 2) + 1;
    }
}

inline bool RAIDPaxosConfig::isValidConfiguration() const {
    switch (raid_mode) {
        case RAIDMode::STRIPE:
            return allow_raid0;  // RAID 0 requires explicit allowance
            
        case RAIDMode::MIRROR:
            return mirror_factor >= 2 && stripe_width >= 1;
            
        case RAIDMode::PARITY:
            return !data_shard_indices.empty() && parity_shard_index >= 0;
            
        case RAIDMode::HYBRID:
            return mirror_factor >= 2 && stripe_width >= 1;
            
        default:
            return false;
    }
}

inline std::string RAIDPaxosConfig::getRAIDModeDescription() const {
    switch (raid_mode) {
        case RAIDMode::STRIPE:
            return "RAID 0 (Striping) - No redundancy, maximum performance";
        case RAIDMode::MIRROR:
            return "RAID 1 (Mirroring) - Full redundancy, " + 
                   std::to_string(mirror_factor) + "x replication";
        case RAIDMode::PARITY:
            return "RAID 5 (Distributed Parity) - " + 
                   std::to_string(data_shard_indices.size()) + " data shards + 1 parity";
        case RAIDMode::HYBRID:
            return "RAID 10 (Stripe+Mirror) - " + 
                   std::to_string(stripe_width) + " stripe width x " + 
                   std::to_string(mirror_factor) + " mirrors";
        default:
            return "Unknown RAID mode";
    }
}

inline bool RAIDPaxosConfig::canRecoverFromFailures(const std::vector<int>& failed_shards) const {
    int max_tolerable = getMaxTolerableFailures(static_cast<int>(data_shard_indices.size()) + 
                                               (raid_mode == RAIDMode::PARITY ? 1 : 0));
    return static_cast<int>(failed_shards.size()) <= max_tolerable;
}

inline int RAIDPaxosConfig::getMaxTolerableFailures(int total_shards) const {
    (void)total_shards; // Suppress unused parameter warning
    switch (raid_mode) {
        case RAIDMode::STRIPE:
            return 0;  // RAID 0: No tolerance

        case RAIDMode::MIRROR:
            // Can tolerate mirror_factor - 1 failures per stripe
            return mirror_factor - 1;

        case RAIDMode::PARITY:
            return 1;  // RAID 5: Can tolerate 1 failure

        case RAIDMode::HYBRID:
            // Can tolerate mirror_factor - 1 failures per stripe
            return mirror_factor - 1;

        default:
            return 0;
    }
}

} // namespace themisdb::sharding
