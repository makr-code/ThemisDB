/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            wal_shipper.cpp                                    ║
  Version:         0.0.14                                             ║
  Last Modified:   2026-02-21 16:53:04                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     496                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "sharding/wal_shipper.h"
#include "sharding/prometheus_metrics.h"
#include "utils/zstd_codec.h"
#include <algorithm>
#include <iostream>

namespace themis::sharding {

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

WALShipper::~WALShipper() {
    stop();
}

void WALShipper::addReplica(const std::string& replica_id, const std::string& endpoint) {
    std::lock_guard<std::mutex> lock(replicas_mutex_);
    
    ReplicaInfo info;
    info.replica_id = replica_id;
    info.endpoint = endpoint;
    info.last_confirmed_lsn = LSN(0, 0);  // Start from beginning
    info.is_healthy = true;
    
    replicas_[replica_id] = info;
}

void WALShipper::removeReplica(const std::string& replica_id) {
    std::lock_guard<std::mutex> lock(replicas_mutex_);
    replicas_.erase(replica_id);
}

void WALShipper::start() {
    if (running_) {
        return;  // Already running
    }
    
    running_ = true;
    shipper_thread_ = std::make_unique<std::thread>(&WALShipper::shippingLoop, this);
}

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

bool WALShipper::isRunning() const {
    return running_;
}

std::vector<ReplicaInfo> WALShipper::getReplicaInfo() const {
    std::lock_guard<std::mutex> lock(replicas_mutex_);
    
    std::vector<ReplicaInfo> result;
    for (const auto& [id, info] : replicas_) {
        result.push_back(info);
    }
    
    return result;
}

WALShipperStats WALShipper::getStatistics() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

void WALShipper::forceShip() {
    cv_.notify_all();
}

void WALShipper::setMetricsExporter(std::shared_ptr<PrometheusMetrics> metrics) {
    metrics_ = metrics;
}

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
            if (!running_) break;
            
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

bool WALShipper::shipToReplica(const std::string& /*replica_id*/, ReplicaInfo& replica) {
    // Get current LSN
    LSN current_lsn = wal_manager_->getCurrentLSN();
    
    // Check if there's anything to ship
    if (replica.last_confirmed_lsn >= current_lsn) {
        return true;  // Nothing new to ship
    }
    
    // Read entries to ship
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
                    stats_.avg_compression_ratio = 
                        (stats_.avg_compression_ratio * (stats_.total_batches - 1) + ratio) / 
                        stats_.total_batches;
                }
            }
        }
        // LZ4 support can be added here in the future
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
    
    // Ship via mTLS POST request
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
WALShipperConfig::CompressionType WALShipper::selectCompressionType(
    size_t payload_size,
    bool is_repetitive,
    double cpu_utilization) const {
    
    // Small payloads: No compression (overhead not worth it)
    if (payload_size < 4096) {  // < 4KB
        return WALShipperConfig::CompressionType::None;
    }
    
    // CPU constrained: Use faster or no compression
    if (cpu_utilization > 0.85) {
        return WALShipperConfig::CompressionType::None;  // or LZ4 if available
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

} // namespace themis::sharding
