/*
 * ThemisDB | File: wal_applier.cpp | Version: 0.0.47 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 165
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=45 | delta=42 | status=divergent
 * External Severity (v3): C=16, H=21, M=8
 * PR: none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "sharding/wal_applier.h"
#include <iostream>
#include <thread>

namespace themis::sharding {

WALApplier::WALApplier(const WALApplierConfig& config)
    : config_(config), current_lsn_(0, 0) {
}

WALApplier::~WALApplier() {
}

void WALApplier::setApplyHandler(ApplyHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    apply_handler_ = handler;
}

ApplyResult WALApplier::applyBatch(const std::vector<WALEntry>& entries) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    ApplyResult result;
    
    if (!apply_handler_) {
        result.errors.push_back("No apply handler set");
        return result;
    }
    
    for (const auto& entry : entries) {
        // Validate LSN sequence if strict mode
        if (config_.strict_mode) {
            LSN expected_lsn = current_lsn_;
            expected_lsn.offset++;  // Next expected
            
            if (!validateLSN(expected_lsn, entry.lsn)) {
                std::string error = "LSN mismatch: expected " + expected_lsn.toString() +
                                  " but got " + entry.lsn.toString();
                result.errors.push_back(error);
                
                {
                    std::lock_guard<std::mutex> stats_lock(stats_mutex_);
                    stats_.lsn_mismatches++;
                }
                
                // In strict mode, stop on LSN mismatch
                return result;
            }
        }
        
        // Apply entry
        if (applyEntry(entry)) {
            current_lsn_ = entry.lsn;
            result.entries_applied++;
            result.last_applied_lsn = entry.lsn;
            
            {
                std::lock_guard<std::mutex> stats_lock(stats_mutex_);
                stats_.total_entries_applied++;
                stats_.total_bytes_applied += entry.size();
                stats_.current_replica_lsn = current_lsn_;
            }
        } else {
            std::string error = "Failed to apply entry at LSN " + entry.lsn.toString();
            result.errors.push_back(error);
            
            {
                std::lock_guard<std::mutex> stats_lock(stats_mutex_);
                stats_.apply_failures++;
            }
            
            // Don't continue on apply failure
            return result;
        }
    }
    
    result.success = true;
    return result;
}

LSN WALApplier::getCurrentLSN() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_lsn_;
}

void WALApplier::setCurrentLSN(const LSN& lsn) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_lsn_ = lsn;
}

WALApplierStats WALApplier::getStatistics() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

void WALApplier::resetStatistics() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_ = WALApplierStats();
    stats_.current_replica_lsn = current_lsn_;
}

bool WALApplier::applyEntry(const WALEntry& entry) {
    // Exponential backoff for transient apply failures.
    themis::utils::RetryConfig backoff_cfg;
    backoff_cfg.max_attempts       = static_cast<uint32_t>(config_.max_apply_retries);
    backoff_cfg.initial_backoff_ms = config_.retry_initial_delay_ms;
    backoff_cfg.max_backoff_ms     = 30'000u;
    backoff_cfg.multiplier         = 2.0;
    backoff_cfg.jitter_fraction    = 0.0;
    themis::utils::ExponentialBackoff backoff(backoff_cfg);

    for (size_t attempt = 0; attempt < config_.max_apply_retries; ++attempt) {
        try {
            // Call apply handler
            if (apply_handler_(entry)) {
                return true;
            }
        } catch (const std::exception& e) {
            std::cerr << "WALApplier: Exception applying entry at LSN "
                      << entry.lsn.toString() << ": " << e.what() << std::endl;
        }

        // Wait before the next attempt (no-op on the last iteration).
        if (attempt < config_.max_apply_retries - 1) {
            backoff.wait();
        }
    }

    return false;
}

bool WALApplier::validateLSN(const LSN& expected, const LSN& actual) {
    // Allow same segment with consecutive offset
    if (expected.segment == actual.segment) {
        // Consecutive offsets are OK
        // Also allow same offset (idempotent replay)
        return actual.offset >= expected.offset - 1 && actual.offset <= expected.offset;
    }
    
    // Allow next segment if offset is 0
    if (actual.segment == expected.segment + 1 && actual.offset == 0) {
        return true;
    }
    
    return false;
}

bool WALApplier::handleConflict(const WALEntry& entry) {
    if (!config_.enable_conflict_detection) {
        return true;  // Conflicts ignored
    }
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.conflicts_detected++;
    }
    
    // Conflict resolution strategy (can be extended)
    // For now, log and continue
    std::cerr << "WALApplier: Conflict detected for entry at LSN " 
              << entry.lsn.toString() << std::endl;
    
    return true;  // Continue applying
}

} // namespace themis::sharding
