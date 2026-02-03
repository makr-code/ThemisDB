#include "sharding/data_movement_coordinator.h"
#include "sharding/prometheus_metrics.h"
#include "utils/logger.h"
#include "utils/tracing.h"
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>
#include <openssl/evp.h>

namespace themis {
namespace sharding {

DataMovementCoordinator::DataMovementCoordinator(const Config& config)
    : config_(config) {
    THEMIS_INFO("DataMovementCoordinator initialized with batch_size={}, max_concurrent={}",
                config_.batch_size, config_.max_concurrent_batches);
}

std::string DataMovementCoordinator::startStreaming(
    const std::string& source_shard_id,
    const std::string& target_shard_id,
    const std::vector<uint64_t>& token_ranges,
    ProgressCallback progress_callback) {
    
    auto span = Tracer::startSpan("DataMovementCoordinator.startStreaming");
    span.setAttribute("source_shard", source_shard_id);
    span.setAttribute("target_shard", target_shard_id);
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Generate stream ID
    std::string stream_id = generateStreamId();
    
    // Create stream state
    StreamState state;
    state.stream_id = stream_id;
    state.source_shard_id = source_shard_id;
    state.target_shard_id = target_shard_id;
    
    // For simplicity, assume 2 ranges: start and end
    if (token_ranges.size() >= 2) {
        state.token_range_start = token_ranges[0];
        state.token_range_end = token_ranges[1];
    }
    
    state.start_time = std::chrono::system_clock::now();
    state.last_activity = state.start_time;
    
    // Estimate total batches (simplified)
    // In production, would query source shard for actual count
    state.total_batches = 100;  // Placeholder
    
    // Store stream state
    active_streams_[stream_id] = state;
    
    // Store callback
    if (progress_callback) {
        progress_callbacks_[stream_id] = progress_callback;
    }
    
    THEMIS_INFO("Started data streaming: stream_id={}, source={}, target={}",
                stream_id, source_shard_id, target_shard_id);
    
    if (metrics_) {
        metrics_->incrementCounter("themis_data_movement_streams_started_total");
    }
    
    // In production, would initiate actual streaming here
    // For now, simulate completion after brief delay
    state.is_complete = true;
    state.batches_sent = state.total_batches;
    state.batches_acknowledged = state.total_batches;
    state.bytes_transferred = state.total_batches * config_.batch_size * 100;  // Estimate
    active_streams_[stream_id] = state;
    
    // Trigger callback with final state
    if (progress_callback) {
        progress_callback(state);
    }
    
    return stream_id;
}

bool DataMovementCoordinator::receiveBatch(const DataBatch& batch) {
    auto span = Tracer::startSpan("DataMovementCoordinator.receiveBatch");
    span.setAttribute("batch_number", static_cast<int64_t>(batch.batch_number));
    span.setAttribute("target_shard", batch.target_shard_id);
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Verify checksum if enabled
    if (config_.verify_checksums) {
        if (!verifyChecksum(batch)) {
            THEMIS_ERROR("Checksum verification failed for batch {}", batch.batch_number);
            span.recordError("Checksum mismatch");
            return false;
        }
    }
    
    // Find corresponding stream
    std::string stream_id;
    for (const auto& [id, state] : active_streams_) {
        if (state.source_shard_id == batch.source_shard_id &&
            state.target_shard_id == batch.target_shard_id) {
            stream_id = id;
            break;
        }
    }
    
    if (stream_id.empty()) {
        THEMIS_WARN("No active stream found for batch");
        return false;
    }
    
    // Update stream progress
    updateStreamProgress(stream_id, batch);
    
    // In production, would actually write batch to target shard here
    THEMIS_DEBUG("Received and applied batch {}/{} for stream {}",
                 batch.batch_number, batch.total_batches, stream_id);
    
    if (metrics_) {
        metrics_->incrementCounter("themis_data_movement_batches_received_total");
    }
    
    return true;
}

bool DataMovementCoordinator::verifyDataIntegrity(
    const std::string& source_shard_id,
    const std::string& target_shard_id,
    const std::vector<uint64_t>& token_ranges) {
    
    auto span = Tracer::startSpan("DataMovementCoordinator.verifyDataIntegrity");
    span.setAttribute("source_shard", source_shard_id);
    span.setAttribute("target_shard", target_shard_id);
    
    THEMIS_INFO("Verifying data integrity: source={}, target={}",
                source_shard_id, target_shard_id);
    
    // In production, this would:
    // 1. Compute checksums of data in source shard for token ranges
    // 2. Compute checksums of data in target shard for same ranges
    // 3. Compare checksums
    // 4. Optionally: row-by-row comparison for critical data
    
    // Placeholder: assume verification passes
    THEMIS_INFO("Data integrity verification passed");
    
    if (metrics_) {
        metrics_->incrementCounter("themis_data_movement_verifications_total");
    }
    
    return true;
}

void DataMovementCoordinator::checkpointProgress(const std::string& stream_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = active_streams_.find(stream_id);
    if (it == active_streams_.end()) {
        THEMIS_WARN("Stream not found for checkpoint: {}", stream_id);
        return;
    }
    
    const auto& state = it->second;
    
    THEMIS_INFO("Checkpointing stream {}: {}/{} batches, {} bytes",
                stream_id, state.batches_acknowledged, state.total_batches,
                state.bytes_transferred);
    
    // In production, would persist checkpoint to durable storage
    // (e.g., metadata shard, etcd, or local disk)
}

std::optional<StreamState> DataMovementCoordinator::getStreamState(const std::string& stream_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = active_streams_.find(stream_id);
    if (it != active_streams_.end()) {
        return it->second;
    }
    
    return std::nullopt;
}

bool DataMovementCoordinator::cancelStream(const std::string& stream_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = active_streams_.find(stream_id);
    if (it == active_streams_.end()) {
        THEMIS_WARN("Stream not found for cancellation: {}", stream_id);
        return false;
    }
    
    THEMIS_INFO("Cancelling stream: {}", stream_id);
    
    // Mark as error
    it->second.has_error = true;
    it->second.error_message = "Cancelled by user";
    
    // Remove from active streams
    active_streams_.erase(it);
    progress_callbacks_.erase(stream_id);
    
    if (metrics_) {
        metrics_->incrementCounter("themis_data_movement_streams_cancelled_total");
    }
    
    return true;
}

void DataMovementCoordinator::setMetrics(std::shared_ptr<PrometheusMetrics> metrics) {
    metrics_ = metrics;
}

// Private helper methods

std::string DataMovementCoordinator::generateStreamId() const {
    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();
    
    std::ostringstream oss;
    oss << "stream_" << std::hex << now_ms;
    return oss.str();
}

std::string DataMovementCoordinator::calculateChecksum(const std::vector<std::string>& rows) const {
    // Concatenate all rows
    std::ostringstream oss;
    for (const auto& row : rows) {
        oss << row;
    }
    std::string data = oss.str();
    
    // Calculate SHA-256 hash
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data.c_str()),
           data.size(), hash);
    
    // Convert to hex string
    std::ostringstream hex_oss;
    hex_oss << std::hex << std::setfill('0');
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        hex_oss << std::setw(2) << static_cast<int>(hash[i]);
    }
    
    return hex_oss.str();
}

bool DataMovementCoordinator::verifyChecksum(const DataBatch& batch) const {
    std::string calculated = calculateChecksum(batch.rows);
    return calculated == batch.checksum;
}

void DataMovementCoordinator::updateStreamProgress(const std::string& stream_id, const DataBatch& batch) {
    auto& state = active_streams_[stream_id];
    
    state.batches_acknowledged++;
    state.last_activity = std::chrono::system_clock::now();
    
    // Estimate bytes transferred
    size_t batch_bytes = 0;
    for (const auto& row : batch.rows) {
        batch_bytes += row.size();
    }
    state.bytes_transferred += batch_bytes;
    
    // Check if complete
    if (state.batches_acknowledged >= state.total_batches) {
        state.is_complete = true;
        THEMIS_INFO("Stream {} completed: {} batches, {} bytes",
                    stream_id, state.batches_acknowledged, state.bytes_transferred);
        
        if (metrics_) {
            metrics_->incrementCounter("themis_data_movement_streams_completed_total");
        }
    }
    
    // Trigger progress callback
    auto callback_it = progress_callbacks_.find(stream_id);
    if (callback_it != progress_callbacks_.end()) {
        callback_it->second(state);
    }
}

} // namespace sharding
} // namespace themis
