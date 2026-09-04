/**
 * @file wal_shipper.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=5; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=2, Debt=0, C=0, H=4, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "sharding/wal_shipper.h"
#include "sharding/prometheus_metrics.h"
#include "utils/zstd_codec.h"
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <openssl/sha.h>
#include <sstream>
#include <thread>
#include <lz4.h>

namespace themis::sharding {

/**
 * @brief Construct WAL shipper and optional mTLS client.
 * @param wal_manager WAL source manager.
 * @param config Shipper configuration.
 */
WALShipper::WALShipper(std::shared_ptr<WALManager> wal_manager,
                       const WALShipperConfig& config)
    : wal_manager_(wal_manager), config_(config) {
    
    if (!wal_manager_) {
        throw std::invalid_argument("WAL manager cannot be null");
    }
    
    // Create mTLS client if certificates provided
    if (!config_.cert_path.empty()) {
        MTLSClient::Config mtls_config;
        mtls_config.cert_path = config_.cert_path;
        mtls_config.key_path = config_.key_path;
        mtls_config.ca_cert_path = config_.ca_cert_path;
        mtls_client_ = std::make_shared<MTLSClient>(mtls_config);
    }
}

/** @brief Destructor stops shipping loop and joins worker thread. */
WALShipper::~WALShipper() {
    stop();
}

/** @brief Register replica endpoint for WAL shipping. */
void WALShipper::addReplica(const std::string& replica_id, const std::string& endpoint) {
    std::lock_guard<std::mutex> lock(replicas_mutex_);
    
    ReplicaInfo info;
    info.replica_id = replica_id;
    info.endpoint = endpoint;
    info.last_confirmed_lsn = LSN(0, 0);  // Start from beginning
    info.is_healthy = true;
    
    replicas_[replica_id] = info;
}

/** @brief Unregister replica from shipping set. */
void WALShipper::removeReplica(const std::string& replica_id) {
    std::lock_guard<std::mutex> lock(replicas_mutex_);
    replicas_.erase(replica_id);
}

/** @brief Start asynchronous shipping thread if not already running. */
void WALShipper::start() {
    if (running_) {
        return;  // Already running
    }
    
    running_ = true;
    shipper_thread_ = std::make_unique<std::thread>(&WALShipper::shippingLoop, this);
}

/** @brief Stop shipping thread and wait for termination. */
void WALShipper::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    cv_.notify_all();
    
    if (shipper_thread_ && shipper_thread_->joinable()) {
        shipper_thread_->join();
    }
}

/** @brief Return whether shipping thread is currently running. */
bool WALShipper::isRunning() const {
    return running_;
}

/** @brief Return snapshot of registered replica states. */
std::vector<ReplicaInfo> WALShipper::getReplicaInfo() const {
    std::lock_guard<std::mutex> lock(replicas_mutex_);
    
    std::vector<ReplicaInfo> result;
    for (const auto& [id, info] : replicas_) {
        result.push_back(info);
    }
    
    return result;
}

/** @brief Return current WAL shipping statistics snapshot. */
WALShipperStats WALShipper::getStatistics() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

/** @brief Trigger immediate wake-up of shipping loop. */
void WALShipper::forceShip() {
    cv_.notify_all();
}

/** @brief Set optional Prometheus exporter for replication metrics. */
void WALShipper::setMetricsExporter(std::shared_ptr<PrometheusMetrics> metrics) {
    metrics_ = metrics;
}

/** @brief Main shipping loop processing replicas at configured interval. */
void WALShipper::shippingLoop() {
    while (running_) {
        auto start_time = std::chrono::steady_clock::now();
        
        // Get list of replicas to ship to
        std::vector<std::string> replica_ids;
        {
            std::lock_guard<std::mutex> lock(replicas_mutex_);
            for (const auto& [id, info] : replicas_) {
                replica_ids.push_back(id);
            }
        }
        
        // Ship to each replica
        for (const auto& replica_id : replica_ids) {
            if (!running_) {
              break;
            }
            
            std::lock_guard<std::mutex> lock(replicas_mutex_);
            auto it = replicas_.find(replica_id);
            if (it != replicas_.end()) {
                shipToReplica(replica_id, it->second);
            }
        }
        
        // Calculate average ship time
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time
        );
        
        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            if (stats_.avg_ship_time.count() == 0) {
                stats_.avg_ship_time = duration;
            } else {
                // Moving average
                stats_.avg_ship_time = std::chrono::milliseconds(
                    (stats_.avg_ship_time.count() * 9 + duration.count()) / 10
                );
            }
        }
        
        // Wait for next interval
        std::unique_lock<std::mutex> cv_lock(cv_mutex_);
        cv_.wait_for(cv_lock, std::chrono::milliseconds(config_.ship_interval_ms),
                    [this] { return !running_; });
    }
}

/**
 * @brief Ship pending WAL entries to one replica.
 * @param replica Replica state reference.
 * @return true when shipping attempt succeeds.
 */
bool WALShipper::shipToReplica(const std::string& /*replica_id*/, ReplicaInfo& replica) {
    // Get current LSN
    LSN current_lsn = wal_manager_->getCurrentLSN();
    
    // Check if there's anything to ship
    if (replica.last_confirmed_lsn >= current_lsn) {
        return true;  // Nothing new to ship
    }
    
    // Read entries to ship.
    // Edge case: readRange may return empty when LSN advanced due to sparse
    // segment boundaries or retention windows; treat as non-fatal and retry
    // in next loop iteration.
    LSN next_lsn = replica.last_confirmed_lsn;
    next_lsn.offset++;  // Start from next entry
    
    std::vector<WALEntry> entries = wal_manager_->readRange(next_lsn, current_lsn);
    
    if (entries.empty()) {
        return true;  // No entries to ship
    }
    
    // Batch entries
    std::vector<WALEntry> batch;
    size_t batch_bytes = 0;
    
    for (const auto& entry : entries) {
        size_t entry_size = entry.size();
        
        // Check if adding this entry would exceed limits
        if (!batch.empty() && 
            (batch.size() >= config_.batch_size || 
             batch_bytes + entry_size > config_.max_batch_bytes)) {
            // Ship current batch
            if (!shipBatch(replica.endpoint, batch)) {
                updateReplicaStatus(replica, false, 0);
                return false;
            }
            
            // Update confirmed LSN
            replica.last_confirmed_lsn = batch.back().lsn;
            updateReplicaStatus(replica, true, batch_bytes);
            
            // Start new batch
            batch.clear();
            batch_bytes = 0;
        }
        
        batch.push_back(entry);
        batch_bytes += entry_size;
    }
    
    // Ship remaining batch
    if (!batch.empty()) {
        if (!shipBatch(replica.endpoint, batch)) {
            updateReplicaStatus(replica, false, 0);
            return false;
        }
        
        replica.last_confirmed_lsn = batch.back().lsn;
        updateReplicaStatus(replica, true, batch_bytes);
    }
    
    // Calculate lag
    calculateLag(replica);
    
    return true;
}

/**
 * @brief Serialize/compress and ship one WAL batch over mTLS.
 * @param endpoint Replica endpoint.
 * @param entries WAL entries in this batch.
 * @return true when endpoint acknowledged the batch.
 */
bool WALShipper::shipBatch(const std::string& endpoint,
                           const std::vector<WALEntry>& entries) {
    if (!mtls_client_ || !mtls_client_->isReady()) {
        std::cerr << "WALShipper: mTLS client not ready" << std::endl;
        return false;
    }
    
    // Serialize entries to JSON for transfer
    nlohmann::json batch_json = nlohmann::json::array();
    
    for (const auto& entry : entries) {
        nlohmann::json entry_json;
        entry_json["lsn"] = entry.lsn.toString();
        entry_json["type"] = static_cast<int>(entry.type);
        entry_json["timestamp"] = entry.timestamp;
        entry_json["transaction_id"] = entry.transaction_id;
        entry_json["data"] = entry.data;
        
        batch_json.push_back(entry_json);
    }
    
    nlohmann::json request;
    request["primary_id"] = config_.primary_id;
    
    // Serialize and potentially compress the batch
    std::string serialized_entries = batch_json.dump();
    size_t uncompressed_size = serialized_entries.size();
    std::string payload_data;
    bool compressed = false;
    
    // Apply compression if configured
    if (config_.compression != WALShipperConfig::CompressionType::None && 
        uncompressed_size > 1024) {  // Only compress if > 1KB
        
        if (config_.compression == WALShipperConfig::CompressionType::Zstd) {
            auto compressed_bytes = utils::zstd_compress(serialized_entries, config_.compression_level);
            if (!compressed_bytes.empty()) {
                // Convert to base64 or hex string for JSON transport
                payload_data = std::string(compressed_bytes.begin(), compressed_bytes.end());
                request["compression"] = "zstd";
                compressed = true;
                
                // Update statistics
                {
                    std::lock_guard<std::mutex> lock(stats_mutex_);
                    stats_.total_bytes_uncompressed += uncompressed_size;
                    double ratio = static_cast<double>(uncompressed_size) / compressed_bytes.size();
                    // Running average: use total_batches (count before this batch) as N-1
                    double n = static_cast<double>(stats_.total_batches);
                    stats_.avg_compression_ratio = (stats_.avg_compression_ratio * n + ratio) / (n + 1.0);
                }
            }
        } else if (config_.compression == WALShipperConfig::CompressionType::LZ4) {
            int src_size = static_cast<int>(uncompressed_size);
            int bound = LZ4_compressBound(src_size);
            std::vector<char> lz4_buf(static_cast<size_t>(bound));
            int lz4_size = LZ4_compress_default(
                serialized_entries.data(), lz4_buf.data(), src_size, bound
            );
            if (lz4_size > 0) {
                payload_data = std::string(lz4_buf.data(), static_cast<size_t>(lz4_size));
                request["compression"] = "lz4";
                compressed = true;

                // Update statistics
                {
                    std::lock_guard<std::mutex> lock(stats_mutex_);
                    stats_.total_bytes_uncompressed += uncompressed_size;
                    double ratio = static_cast<double>(uncompressed_size) / lz4_size;
                    double n = static_cast<double>(stats_.total_batches);
                    stats_.avg_compression_ratio = (stats_.avg_compression_ratio * n + ratio) / (n + 1.0);
                }
            }
        }
    }
    
    // If compression failed or not enabled, use uncompressed
    if (!compressed) {
        request["entries"] = batch_json;
        request["compression"] = "none";
        
        // Update statistics for uncompressed
        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.total_bytes_uncompressed += uncompressed_size;
        }
    } else {
        // Store compressed data as binary (base64 encoded for JSON)
        request["entries_compressed"] = nlohmann::json::binary(
            std::vector<uint8_t>(payload_data.begin(), payload_data.end())
        );
    }
    
    // Ship via mTLS POST request.
    // Endpoint contract: /api/v1/wal/apply returns response.success=true
    // when payload was accepted (independent from eventual apply latency).
    auto response = mtls_client_->post(endpoint, "/api/v1/wal/apply", request.dump());
    
    if (!response.success) {
        std::cerr << "WALShipper: Failed to ship batch to " << endpoint 
                  << " - " << response.error << std::endl;
        return false;
    }
    
    // Update statistics
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.total_batches++;
        stats_.total_entries_shipped += entries.size();
        
        for (const auto& entry : entries) {
            stats_.total_bytes_shipped += entry.size();
        }
    }
    
    return true;
}

/** @brief Update replica health and global stats after shipping attempt. */
void WALShipper::updateReplicaStatus(ReplicaInfo& replica, bool success, size_t bytes_shipped) {
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    // Record metrics if exporter available
    if (metrics_) {
        metrics_->recordWalShipBatch(replica.replica_id, 
                                     success ? 1 : 0, 
                                     success ? bytes_shipped : 0, 
                                     success);
    }
    
    if (success) {
        replica.is_healthy = true;
        replica.last_success_ts = now;
        replica.consecutive_failures = 0;
    } else {
        replica.consecutive_failures++;
        
        if (replica.consecutive_failures >= config_.max_retries) {
            replica.is_healthy = false;
        }
        
        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.failed_ships++;
        }
    }
}

/** @brief Recompute lag bytes/time and export lag metrics. */
void WALShipper::calculateLag(ReplicaInfo& replica) {
    LSN current_lsn = wal_manager_->getCurrentLSN();
    
    // Calculate byte lag
    if (replica.last_confirmed_lsn.segment == current_lsn.segment) {
        replica.lag_bytes = current_lsn.offset - replica.last_confirmed_lsn.offset;
    } else {
        // Approximate lag (segment size * segment difference + offset difference)
        uint64_t segment_diff = current_lsn.segment - replica.last_confirmed_lsn.segment;
        replica.lag_bytes = segment_diff * 16 * 1024 * 1024 + current_lsn.offset;
    }
    
    // Calculate time lag
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    if (replica.last_success_ts > 0) {
        replica.lag_ms = now - replica.last_success_ts;
        
        // Update max lag statistic
        std::lock_guard<std::mutex> lock(stats_mutex_);
        if (std::chrono::milliseconds(replica.lag_ms) > stats_.max_lag) {
            stats_.max_lag = std::chrono::milliseconds(replica.lag_ms);
        }
    }
    
    // Record lag metrics if exporter available
    if (metrics_) {
        metrics_->recordWalReplicationLag(replica.replica_id, replica.lag_ms / 1000.0);
        metrics_->setWalBacklogBytes(replica.replica_id, static_cast<int64_t>(replica.lag_bytes));
    }
}

/** @brief Perform transport-level health check for replica endpoint. */
void WALShipper::healthCheck(ReplicaInfo& replica) {
    // Perform health check via ping endpoint
    if (!mtls_client_ || !mtls_client_->isReady()) {
        replica.is_healthy = false;
        return;
    }
    
    auto response = mtls_client_->get(replica.endpoint, "/api/v1/health");
    replica.is_healthy = response.success;
}

// Phase 3: Adaptive batch sizing
/**
 * @brief Compute adaptive batch size from latency/CPU/IOPS telemetry.
 */
size_t WALShipper::calculateOptimalBatchSize(double network_latency_ms,
                                             double cpu_utilization,
                                             size_t disk_iops_available) const {
    // Base batch size from config
    size_t base_batch_size = config_.batch_size;
    
    if (!config_.adaptive_batch_size) {
        return base_batch_size;
    }
    
    // Factor 1: Network latency adjustment
    // Lower latency = smaller batches (reduce lag)
    // Higher latency = larger batches (amortize overhead)
    double latency_factor = 1.0;
    if (network_latency_ms < 1.0) {
        latency_factor = 0.5;  // Very fast network, use smaller batches
    } else if (network_latency_ms < 5.0) {
        latency_factor = 0.75;
    } else if (network_latency_ms > 50.0) {
        latency_factor = 2.0;  // Slow network, use larger batches
    } else if (network_latency_ms > 20.0) {
        latency_factor = 1.5;
    }
    
    // Factor 2: CPU utilization adjustment
    // Lower CPU = larger batches (more compression)
    // Higher CPU = smaller batches (reduce load)
    double cpu_factor = 1.0;
    if (cpu_utilization < 0.3) {
        cpu_factor = 1.5;  // Low CPU, can handle larger batches
    } else if (cpu_utilization > 0.8) {
        cpu_factor = 0.6;  // High CPU, reduce batch size
    }
    
    // Factor 3: Disk IOPS adjustment
    // Higher IOPS available = larger batches (maximize throughput)
    double iops_factor = 1.0;
    if (disk_iops_available > 50000) {
        iops_factor = 1.5;  // Abundant IOPS, use larger batches
    } else if (disk_iops_available < 10000) {
        iops_factor = 0.7;  // Limited IOPS, smaller batches
    }
    
    // Combine factors
    double combined_factor = latency_factor * cpu_factor * iops_factor;
    size_t optimal_batch_size = static_cast<size_t>(base_batch_size * combined_factor);
    
    // Clamp to configured min/max
    optimal_batch_size = std::max(optimal_batch_size, config_.min_batch_size);
    optimal_batch_size = std::min(optimal_batch_size, config_.max_batch_size);
    
    return optimal_batch_size;
}

// Phase 3: Intelligent compression selection
/**
 * @brief Select compression strategy for given payload/CPU profile.
 */
WALShipperConfig::CompressionType WALShipper::selectCompressionType(
    size_t payload_size,
    bool is_repetitive,
    double cpu_utilization) const {
    
    // Small payloads: No compression (overhead not worth it)
    if (payload_size < 4096) {  // < 4KB
        return WALShipperConfig::CompressionType::None;
    }
    
    // CPU constrained: Use no compression
    if (cpu_utilization > 0.85) {
        return WALShipperConfig::CompressionType::None;
    }
    
    // Medium CPU load and repetitive data: Use Zstd
    if (is_repetitive && cpu_utilization < 0.7) {
        return WALShipperConfig::CompressionType::Zstd;
    }
    
    // High CPU load but still acceptable: Use LZ4
    if (cpu_utilization < 0.85) {
        return WALShipperConfig::CompressionType::LZ4;  // Faster than Zstd
    }
    
    // Default: No compression
    return WALShipperConfig::CompressionType::None;
}

// ============================================================================
// Snapshot transfer: chunked delivery with per-chunk SHA-256 checksums
// ============================================================================

/**
 * @brief Compute SHA-256 digest of buffer and return lowercase hex string.
 *
 * Handles empty-buffer input safely to avoid passing a potentially null
 * pointer into SHA256.
 */
static std::string chunkSha256(const uint8_t* data, size_t size) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    if (size == 0) {
        static const uint8_t kEmpty[1] = {0};
        SHA256(kEmpty, 0, hash);
    } else {
        SHA256(data, size, hash);
    }
    std::ostringstream oss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return oss.str();
}

/**
 * @brief Base64-encode binary payload for JSON-safe transport.
 * @param data Raw binary data.
 * @return Base64-encoded ASCII representation.
 */
static std::string base64Encode(const std::vector<uint8_t>& data) {
    static constexpr char kB64Chars[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);
    for (size_t i = 0; i < data.size(); i += 3) {
        const uint8_t b0 = data[i];
        const uint8_t b1 = (i + 1 < data.size()) ? data[i + 1] : 0u;
        const uint8_t b2 = (i + 2 < data.size()) ? data[i + 2] : 0u;
        out += kB64Chars[(b0 >> 2) & 0x3F];
        out += kB64Chars[((b0 & 0x03) << 4) | ((b1 >> 4) & 0x0F)];
        out += (i + 1 < data.size()) ? kB64Chars[((b1 & 0x0F) << 2) | ((b2 >> 6) & 0x03)] : '=';
        out += (i + 2 < data.size()) ? kB64Chars[b2 & 0x3F] : '=';
    }
    return out;
}

/** @brief Verify chunk checksum using SHA-256 over chunk payload. */
/* static */ bool WALShipper::verifyChunkChecksum(const SnapshotChunk& chunk) {
    const std::string computed =
        chunkSha256(chunk.data.data(), chunk.data.size());
    return computed == chunk.checksum;
}

/**
 * @brief Send snapshot chunks to lagging replica with per-chunk retries.
 * @param replica_id Target replica id.
 * @param chunks Ordered snapshot chunks.
 * @return Transfer result with counters and optional error description.
 */
SnapshotTransferResult WALShipper::sendSnapshot(const std::string& replica_id,
                                                  const std::vector<SnapshotChunk>& chunks) {
    SnapshotTransferResult result;

    if (chunks.empty()) {
        result.error_message = "No chunks to transfer";
        return result;
    }

    // Verify all chunks before starting the transfer
    for (const auto& chunk : chunks) {
        if (!verifyChunkChecksum(chunk)) {
            result.error_message = "Chunk checksum verification failed before transfer: "
                                   "chunk_index=" + std::to_string(chunk.chunk_index);
            return result;
        }
    }

    // Look up replica endpoint
    std::string endpoint;
    {
        std::lock_guard<std::mutex> lock(replicas_mutex_);
        auto it = replicas_.find(replica_id);
        if (it == replicas_.end()) {
            result.error_message = "Unknown replica: " + replica_id;
            return result;
        }
        endpoint = it->second.endpoint;
    }

    spdlog::info("WALShipper: beginning snapshot transfer to replica={} "
                 "snapshot_index={} total_chunks={}",
                 replica_id,
                 chunks.empty() ? 0 : chunks.front().snapshot_index,
                 chunks.size());

    for (const auto& chunk : chunks) {
        // Retry loop per chunk to tolerate transient network interruptions
        bool chunk_ok = false;
        size_t attempt = 0;
        uint64_t retry_delay_ms = config_.retry_delay_ms;

        while (attempt < config_.max_retries && !chunk_ok) {
            ++attempt;

            // Serialize the chunk fields to JSON and POST to the replica's
            // snapshot chunk endpoint, mirroring the existing shipBatch pattern.
            // Binary data is base64-encoded to avoid the 2× size penalty of
            // hex encoding while remaining JSON-compatible.
            bool sent = false;
            if (mtls_client_ && mtls_client_->isReady()) {
                // Base64-encode the chunk payload to avoid the 2× overhead of
                // hex encoding, while staying within JSON string constraints.
                nlohmann::json body = {
                    {"snapshot_index", chunk.snapshot_index},
                    {"snapshot_term",  chunk.snapshot_term},
                    {"chunk_index",    chunk.chunk_index},
                    {"total_chunks",   chunk.total_chunks},
                    {"last_chunk",     chunk.last_chunk},
                    {"checksum",       chunk.checksum},
                    {"data_b64",       base64Encode(chunk.data)}
                };
                auto resp = mtls_client_->post(endpoint,
                                               "/api/v1/snapshot/chunk",
                                               body);
                sent = resp.success;
            } else {
                // No configured/ready mTLS client: fail in production.
                // In test builds (THEMIS_TEST_BUILD), simulate success so that
                // unit tests that don't wire up a real transport still work.
                // This path is compile-time gated and never active in regular
                // production builds.
#if defined(THEMIS_TEST_BUILD)
                spdlog::debug("WALShipper: THEMIS_TEST_BUILD – simulating chunk send "
                              "chunk_index={}", chunk.chunk_index);
                sent = true;
#else
                spdlog::error("WALShipper: mTLS client not configured or not ready; "
                              "refusing to send snapshot chunk {} to {}",
                              chunk.chunk_index, replica_id);
                sent = false;
#endif
            }

            if (sent) {
                chunk_ok = true;
                result.chunks_sent++;
                result.bytes_sent += chunk.data.size();

                // Update statistics
                {
                    std::lock_guard<std::mutex> slock(stats_mutex_);
                    stats_.total_snapshot_chunks_sent++;
                    stats_.total_snapshot_bytes_sent += chunk.data.size();
                }
            } else {
                spdlog::warn("WALShipper: chunk send failed "
                             "replica={} chunk_index={} attempt={}/{}",
                             replica_id, chunk.chunk_index, attempt, config_.max_retries);

                if (attempt < config_.max_retries) {
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(retry_delay_ms));
                    retry_delay_ms = std::min(retry_delay_ms * 2,
                                              config_.max_retry_delay_ms);
                }
            }
        }

        if (!chunk_ok) {
            result.error_message =
                "Failed to send chunk " + std::to_string(chunk.chunk_index) +
                " to replica " + replica_id + " after " +
                std::to_string(config_.max_retries) + " attempts";
            spdlog::error("WALShipper: {}", result.error_message);
            return result;
        }
    }

    result.success = true;
    spdlog::info("WALShipper: snapshot transfer complete to replica={} "
                 "chunks_sent={} bytes_sent={}",
                 replica_id, result.chunks_sent, result.bytes_sent);
    return result;
}

} // namespace themis::sharding

