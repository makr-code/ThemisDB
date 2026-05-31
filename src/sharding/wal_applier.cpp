/*
 * ThemisDB | File: wal_applier.cpp | Version: 0.0.47 | Last Modified: 2026-05-20 17:39:29
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 93/100 | Lines: 174
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=7, H=5, M=3, L=0
 * PR History (last 5): none
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
            const bool bootstrap_replay =
                (current_lsn_.segment == 0 && current_lsn_.offset == 0 &&
                 entry.lsn.segment == 0 && entry.lsn.offset == 0 &&
                 result.entries_applied == 0);

            if (!bootstrap_replay && entry.lsn <= current_lsn_) {
                std::string error = "LSN stale or duplicate: current " + current_lsn_.toString() +
                                  " but got " + entry.lsn.toString();
                result.errors.push_back(error);

                {
                    std::lock_guard<std::mutex> stats_lock(stats_mutex_);
                    stats_.lsn_mismatches++;
                }

                return result;
            }

            LSN expected_lsn = current_lsn_;
            expected_lsn.offset++;  // Next expected

            if (!bootstrap_replay && !validateLSN(expected_lsn, entry.lsn)) {
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
    // Same segment: must be exact successor offset.
    if (expected.segment == actual.segment) {
        return actual.offset == expected.offset;
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
