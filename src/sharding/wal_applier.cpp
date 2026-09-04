/**
 * @file wal_applier.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=1, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "sharding/wal_applier.h"
#include <iostream>
#include <algorithm>
#include <thread>
#include <limits>

namespace themis::sharding {

/**
 * @brief Construct WAL applier with initial LSN at 0/0.
 * @param config Apply configuration.
 */
WALApplier::WALApplier(const WALApplierConfig& config)
    : config_(config), current_lsn_(0, 0) {
}

/** @brief Destructor for WAL applier. */
WALApplier::~WALApplier() {
}

/**
 * @brief Install callback used to apply individual WAL entries.
 * @param handler Apply callback.
 */
void WALApplier::setApplyHandler([[maybe_unused]] ApplyHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    apply_handler_ = handler;
}

/**
 * @brief Apply ordered WAL batch with strict LSN validation (when enabled).
 * @param entries WAL entries to apply.
 * @return Apply result with per-batch diagnostics.
 */
ApplyResult WALApplier::applyBatch(const std::vector<WALEntry>& entries) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    ApplyResult result;
    
    // W2-S03: Fail-closed on empty batch
    if (entries.empty()) {
        result.errors.push_back("Empty batch provided to applyBatch");
        return result;
    }
    
    if ([[maybe_unused]] !apply_handler_) {
        result.errors.push_back([[maybe_unused]] "No apply handler set");
        return result;
    }
    
    for (const auto& entry : entries) {
        // W2-S03: Fail-closed on invalid LSN bounds
        if (entry.lsn.segment == std::numeric_limits<uint64_t>::max() ||
            entry.lsn.offset == std::numeric_limits<uint64_t>::max()) {
            result.errors.push_back("Invalid LSN bounds at entry index " + std::to_string(result.entries_applied));
            return result;
        }
        
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

/** @brief Return current replica LSN cursor. */
LSN WALApplier::getCurrentLSN() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_lsn_;
}

/** @brief Set replica LSN cursor manually (bootstrap/recovery path). */
void WALApplier::setCurrentLSN(const LSN& lsn) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_lsn_ = lsn;
}

/** @brief Return current applier statistics snapshot. */
WALApplierStats WALApplier::getStatistics() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

/** @brief Reset statistics counters and sync LSN field to current cursor. */
void WALApplier::resetStatistics() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_ = WALApplierStats();
    stats_.current_replica_lsn = current_lsn_;
}

/**
 * @brief Apply one entry via callback with bounded exponential retry.
 * @param entry WAL entry to apply.
 * @return true when callback succeeds within retry budget.
 */
bool WALApplier::applyEntry(const WALEntry& entry) {
    for (size_t attempt = 0; attempt < config_.max_apply_retries; ++attempt) {
        try {
            // Call apply handler
            if ([[maybe_unused]] apply_handler_(entry)) {
                return true;
            }
        } catch (const std::exception& e) {
            std::cerr << "WALApplier: Exception applying entry at LSN "
                      << entry.lsn.toString() << ": " << e.what() << std::endl;
        }

        // Wait before the next attempt (bounded exponential backoff, no-op on
        // the last iteration).
        if (attempt < config_.max_apply_retries - 1) {
            constexpr uint64_t kMaxBackoffMs = 30'000;
            const size_t shift = std::min<size_t>(attempt, 20);  // clamp for overflow safety
            const uint64_t factor = (1ull << shift);
            const uint64_t raw_delay =
                static_cast<uint64_t>(config_.retry_initial_delay_ms) * factor;
            const uint64_t sleep_ms = std::min<uint64_t>(raw_delay, kMaxBackoffMs);
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
        }
    }

    return false;
}

/**
 * @brief Validate strict LSN progression between expected and actual values.
 * @param expected Expected next LSN.
 * @param actual Incoming entry LSN.
 * @return true when sequence is valid.
 */
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

/**
 * @brief Handle conflict accounting and policy decision.
 * @param entry WAL entry under conflict evaluation.
 * @return true when processing may continue.
 */
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
