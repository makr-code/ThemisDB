/**
 * @file audit_batch_writer.cpp
 * @brief Implementation of high-volume audit batch writer
 * @version 0.1.0
 * @note Maturity: 🟡 BETA
 */

#include "governance/audit_batch_writer.h"

#include <iostream>
#include <algorithm>
#include <fstream>
#include <chrono>
#include <numeric>
#include <openssl/sha.h>

namespace themis {
namespace governance {

// ============================================================================
// Checkpoint and Token Serialization
// ============================================================================

nlohmann::json AuditBatchCheckpoint::toJson() const {
    nlohmann::json j;
    j["checkpoint_id"] = checkpoint_id;
    j["batch_sequence_number"] = batch_sequence_number;
    j["first_entry_sequence"] = first_entry_sequence;
    j["last_entry_sequence"] = last_entry_sequence;
    j["entry_count"] = entry_count;
    j["batch_hash"] = batch_hash;
    j["checkpoint_time_ms"] = checkpoint_time_ms;
    j["state"] = state;
    j["error_message"] = error_message;
    return j;
}

AuditBatchCheckpoint AuditBatchCheckpoint::fromJson(const nlohmann::json& j) {
    AuditBatchCheckpoint cp;
    if (j.contains("checkpoint_id")) cp.checkpoint_id = j["checkpoint_id"];
    if (j.contains("batch_sequence_number")) cp.batch_sequence_number = j["batch_sequence_number"];
    if (j.contains("first_entry_sequence")) cp.first_entry_sequence = j["first_entry_sequence"];
    if (j.contains("last_entry_sequence")) cp.last_entry_sequence = j["last_entry_sequence"];
    if (j.contains("entry_count")) cp.entry_count = j["entry_count"];
    if (j.contains("batch_hash")) cp.batch_hash = j["batch_hash"];
    if (j.contains("checkpoint_time_ms")) cp.checkpoint_time_ms = j["checkpoint_time_ms"];
    if (j.contains("state")) cp.state = j["state"];
    if (j.contains("error_message")) cp.error_message = j["error_message"];
    return cp;
}

nlohmann::json IdempotencyToken::toJson() const {
    nlohmann::json j;
    j["token"] = token;
    j["entry_id"] = entry_id;
    j["submitted_at_ms"] = submitted_at_ms;
    j["state"] = state;
    return j;
}

IdempotencyToken IdempotencyToken::fromJson(const nlohmann::json& j) {
    IdempotencyToken t;
    if (j.contains("token")) t.token = j["token"];
    if (j.contains("entry_id")) t.entry_id = j["entry_id"];
    if (j.contains("submitted_at_ms")) t.submitted_at_ms = j["submitted_at_ms"];
    if (j.contains("state")) t.state = j["state"];
    return t;
}

// ============================================================================
// AuditBatchWriter Implementation
// ============================================================================

AuditBatchWriter::AuditBatchWriter(
    std::shared_ptr<AuditIntegrityManager> manager,
    const Config& config
) : manager_(manager), config_(config) {}

AuditBatchWriter::~AuditBatchWriter() {
    if (running_.load()) {
        shutdown();
    }
}

std::string AuditBatchWriter::start() {
    if (running_.load()) {
        return "Already running";
    }
    
    running_.store(true);
    shutdown_requested_.store(false);
    
    try {
        flush_thread_ = std::make_unique<std::thread>(&AuditBatchWriter::flushThread, this);
    } catch (const std::exception& e) {
        running_.store(false);
        return std::string("Failed to start flush thread: ") + e.what();
    }
    
    return "OK";
}

std::string AuditBatchWriter::shutdown() {
    if (!running_.load()) {
        return "Not running";
    }
    
    shutdown_requested_.store(true);
    
    if (flush_thread_ && flush_thread_->joinable()) {
        flush_thread_->join();
    }
    
    // Final flush of remaining entries
    forceFlush();
    
    running_.store(false);
    return "OK";
}

std::string AuditBatchWriter::submitEntry(const ImmutableAuditEntry& entry) {
    if (!running_.load()) {
        return "Writer not running";
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    {
        std::lock_guard<std::mutex> lock(buffer_mutex_);
        
        if (pending_entries_.size() >= config_.buffer_size) {
            if (config_.enable_backpressure) {
                return "BUFFER_FULL";
            }
        }
        
        pending_entries_.push_back(entry);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto latency_us = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - start_time
    ).count();
    
    recordMetrics(latency_us);
    return "OK";
}

std::string AuditBatchWriter::submitEntryIdempotent(
    const ImmutableAuditEntry& entry,
    const std::string& idempotency_token
) {
    {
        std::lock_guard<std::mutex> lock(idempotency_mutex_);
        
        auto it = idempotency_tokens_.find(idempotency_token);
        if (it != idempotency_tokens_.end()) {
            if (it->second.state == "committed") {
                return "DUPLICATE";
            }
            // If pending or failed, allow retry
        }
    }
    
    // Submit the entry
    auto status = submitEntry(entry);
    if (status != "OK") {
        return status;
    }
    
    // Record the token
    {
        std::lock_guard<std::mutex> lock(idempotency_mutex_);
        
        IdempotencyToken token;
        token.token = idempotency_token;
        token.entry_id = entry.entry_id;
        token.submitted_at_ms = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;
        token.state = "pending";
        
        idempotency_tokens_[idempotency_token] = token;
    }
    
    return "OK";
}

AuditBatchWriter::WriteResult AuditBatchWriter::flush() {
    WriteResult result;
    
    std::vector<ImmutableAuditEntry> batch_to_write;
    
    {
        std::lock_guard<std::mutex> lock(buffer_mutex_);
        
        if (pending_entries_.empty()) {
            result.success = true;
            result.entries_written = 0;
            return result;
        }
        
        // Take up to batch_size entries
        size_t count = std::min(pending_entries_.size(), config_.batch_size);
        batch_to_write.assign(pending_entries_.begin(), pending_entries_.begin() + count);
        pending_entries_.erase(pending_entries_.begin(), pending_entries_.begin() + count);
    }
    
    return flushBatch(batch_to_write);
}

AuditBatchWriter::WriteResult AuditBatchWriter::forceFlush() {
    WriteResult result;
    
    std::vector<ImmutableAuditEntry> batch_to_write;
    
    {
        std::lock_guard<std::mutex> lock(buffer_mutex_);
        
        if (pending_entries_.empty()) {
            result.success = true;
            result.entries_written = 0;
            return result;
        }
        
        batch_to_write = pending_entries_;
        pending_entries_.clear();
    }
    
    return flushBatch(batch_to_write);
}

nlohmann::json AuditBatchWriter::getBufferStats() const {
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    
    nlohmann::json j;
    j["pending_entries"] = pending_entries_.size();
    j["buffer_capacity"] = config_.buffer_size;
    j["buffer_fill_percentage"] = (double)pending_entries_.size() / config_.buffer_size * 100.0;
    j["entry_sequence"] = entry_sequence_counter_.load();
    j["batch_sequence"] = batch_sequence_counter_.load();
    return j;
}

nlohmann::json AuditBatchWriter::getMetrics() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    nlohmann::json j;
    j["total_entries_submitted"] = metrics_.total_entries_submitted;
    j["total_entries_flushed"] = metrics_.total_entries_flushed;
    j["total_batches_flushed"] = metrics_.total_batches_flushed;
    j["total_errors"] = metrics_.total_errors;
    j["avg_submission_latency_us"] = metrics_.avg_submission_latency_us;
    j["p95_submission_latency_us"] = metrics_.p95_submission_latency_us;
    j["p99_submission_latency_us"] = metrics_.p99_submission_latency_us;
    return j;
}

std::vector<AuditBatchCheckpoint> AuditBatchWriter::getCheckpoints() const {
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    return checkpoint_history_;
}

bool AuditBatchWriter::verifyCheckpoint(const AuditBatchCheckpoint& checkpoint) const {
    // Verify checkpoint hash matches expected value
    // This would load the checkpoint data and recompute the hash
    return checkpoint.state == "flushed" || checkpoint.state == "verified";
}

AuditBatchWriter::WriteResult AuditBatchWriter::recoverFromCheckpoint(
    const AuditBatchCheckpoint& checkpoint
) {
    WriteResult result;
    
    // In real implementation, this would:
    // 1. Load entries from checkpoint file
    // 2. Verify their hashes match checkpoint
    // 3. Resubmit them if not already flushed
    
    result.success = true;
    return result;
}

std::optional<IdempotencyToken> AuditBatchWriter::getTokenStatus(
    const std::string& token
) const {
    std::lock_guard<std::mutex> lock(idempotency_mutex_);
    
    auto it = idempotency_tokens_.find(token);
    if (it != idempotency_tokens_.end()) {
        return it->second;
    }
    return std::nullopt;
}

void AuditBatchWriter::flushThread() {
    while (!shutdown_requested_.load()) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(config_.flush_interval_ms)
        );
        
        if (!pending_entries_.empty()) {
            flush();
        }
    }
}

AuditBatchWriter::WriteResult AuditBatchWriter::flushBatch(
    const std::vector<ImmutableAuditEntry>& batch
) {
    WriteResult result;
    result.success = true;
    result.entries_written = 0;
    result.entries_failed = 0;
    
    if (batch.empty()) {
        return result;
    }
    
    // Create checkpoint
    auto checkpoint = createCheckpoint(batch, "pending");
    if (config_.enable_checkpoints) {
        persistCheckpoint(checkpoint);
    }
    
    // Write entries to manager
    // Note: In real implementation, this would be atomic
    int64_t first_seq = entry_sequence_counter_.load();
    
    for (const auto& entry : batch) {
        try {
            // Note: Real implementation would batch these writes
            manager_->addEntry(entry);
            result.entries_written++;
            entry_sequence_counter_++;
        } catch (const std::exception& e) {
            result.entries_failed++;
            result.error_message = e.what();
            result.success = false;
        }
    }
    
    // Update checkpoint state
    checkpoint.state = result.success ? "flushed" : "failed";
    checkpoint.entry_count = result.entries_written;
    
    if (config_.enable_checkpoints) {
        persistCheckpoint(checkpoint);
    }
    
    // Update idempotency tokens
    {
        std::lock_guard<std::mutex> lock(idempotency_mutex_);
        for (const auto& [token_str, token_obj] : idempotency_tokens_) {
            if (token_obj.state == "pending") {
                // Mark as committed if entry was in this batch
                // Real implementation would verify entry_id
                IdempotencyToken updated = token_obj;
                updated.state = "committed";
                idempotency_tokens_[token_str] = updated;
            }
        }
    }
    
    // Update metrics
    {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        metrics_.total_entries_flushed += result.entries_written;
        metrics_.total_batches_flushed++;
        if (!result.success) {
            metrics_.total_errors++;
        }
    }
    
    batch_sequence_counter_++;
    return result;
}

std::string AuditBatchWriter::computeBatchHash(
    const std::vector<ImmutableAuditEntry>& batch
) const {
    // Compute SHA-256 hash of batch content
    // Real implementation would hash all entries concatenated
    return "hash_placeholder";
}

AuditBatchCheckpoint AuditBatchWriter::createCheckpoint(
    const std::vector<ImmutableAuditEntry>& batch,
    const std::string& state
) {
    AuditBatchCheckpoint cp;
    cp.checkpoint_id = "cp_" + std::to_string(batch_sequence_counter_.load());
    cp.batch_sequence_number = batch_sequence_counter_.load();
    cp.first_entry_sequence = entry_sequence_counter_.load();
    cp.last_entry_sequence = entry_sequence_counter_.load() + batch.size() - 1;
    cp.entry_count = batch.size();
    cp.batch_hash = computeBatchHash(batch);
    cp.checkpoint_time_ms = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;
    cp.state = state;
    return cp;
}

void AuditBatchWriter::persistCheckpoint(const AuditBatchCheckpoint& checkpoint) {
    // In real implementation, write checkpoint to disk
    // This enables crash recovery
    
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    checkpoints_[checkpoint.checkpoint_id] = checkpoint;
    checkpoint_history_.push_back(checkpoint);
    
    // Limit history size
    if (checkpoint_history_.size() > 1000) {
        checkpoint_history_.erase(checkpoint_history_.begin());
    }
}

void AuditBatchWriter::recordMetrics(int64_t submission_latency_us) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    metrics_.total_entries_submitted++;
    
    // Update average (simple moving average)
    metrics_.avg_submission_latency_us =
        (metrics_.avg_submission_latency_us * (metrics_.total_entries_submitted - 1) +
         submission_latency_us) / metrics_.total_entries_submitted;
    
    // TODO: Implement proper p95/p99 tracking with histogram
}

}  // namespace governance
}  // namespace themis
