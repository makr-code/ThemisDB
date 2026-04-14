/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            gossip_config_manager.cpp                          ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 07:06:09                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     810                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ad6e8f172c  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • 971a3c49d5  2026-03-20  Build/test fixes and auth role mapping refactor ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "sharding/gossip_config_manager.h"
#include "sharding/shard_topology.h"
#include "sharding/mtls_client.h"
#include "sharding/prometheus_metrics.h"
#include "shard_rpc.pb.h"
#include <random>
#include <algorithm>
#include <sstream>
#include <iomanip>
// Note: uuid/uuid.h is Linux-specific, Windows uses different UUID APIs
// For cross-platform UUID support, consider using boost::uuid or similar
// #include <uuid/uuid.h>

namespace themis {
namespace sharding {

// ==============================================================================
// VectorClock Implementation
// ==============================================================================

void VectorClock::increment(const std::string& shard_id) {
    clocks_[shard_id]++;
}

void VectorClock::merge(const VectorClock& other) {
    for (const auto& [shard_id, clock_value] : other.clocks_) {
        clocks_[shard_id] = std::max(clocks_[shard_id], clock_value);
    }
}

VectorClock::Ordering VectorClock::compare(const VectorClock& other) const {
    bool this_greater = false;
    bool other_greater = false;
    
    // Collect all shard IDs from both clocks
    std::set<std::string> all_shards;
    for (const auto& [shard_id, _] : clocks_) {
        all_shards.insert(shard_id);
    }
    for (const auto& [shard_id, _] : other.clocks_) {
        all_shards.insert(shard_id);
    }
    
    // Compare all shard clocks
    for (const auto& shard_id : all_shards) {
        uint64_t this_val = get(shard_id);
        uint64_t other_val = other.get(shard_id);
        
        if (this_val > other_val) {
            this_greater = true;
        } else if (this_val < other_val) {
            other_greater = true;
        }
    }
    
    if (this_greater && !other_greater) {
        return Ordering::AFTER;
    } else if (!this_greater && other_greater) {
        return Ordering::BEFORE;
    } else if (this_greater && other_greater) {
        return Ordering::CONCURRENT;
    } else {
        return Ordering::EQUAL;
    }
}

uint64_t VectorClock::get(const std::string& shard_id) const {
    auto it = clocks_.find(shard_id);
    return (it != clocks_.end()) ? it->second : 0;
}

void VectorClock::set(const std::string& shard_id, uint64_t value) {
    clocks_[shard_id] = value;
}

proto::VectorClock VectorClock::toProto() const {
    proto::VectorClock proto_clock;
    for (const auto& [shard_id, value] : clocks_) {
        (*proto_clock.mutable_clocks())[shard_id] = value;
    }
    return proto_clock;
}

VectorClock VectorClock::fromProto(const proto::VectorClock& proto) {
    VectorClock clock;
    for (const auto& [shard_id, value] : proto.clocks()) {
        clock.set(shard_id, value);
    }
    return clock;
}

// ==============================================================================
// ConfigUpdate Implementation
// ==============================================================================

proto::ConfigUpdate ConfigUpdate::toProto() const {
    proto::ConfigUpdate proto_update;
    proto_update.set_update_id(update_id);
    proto_update.set_config_key(config_key);
    proto_update.set_config_value(config_value);
    proto_update.set_timestamp_ns(timestamp_ns);
    *proto_update.mutable_vector_clock() = vector_clock.toProto();
    proto_update.set_originator_shard_id(originator_shard_id);
    proto_update.set_ttl(ttl);
    return proto_update;
}

ConfigUpdate ConfigUpdate::fromProto(const proto::ConfigUpdate& proto) {
    ConfigUpdate update;
    update.update_id = proto.update_id();
    update.config_key = proto.config_key();
    update.config_value = proto.config_value();
    update.timestamp_ns = proto.timestamp_ns();
    update.vector_clock = VectorClock::fromProto(proto.vector_clock());
    update.originator_shard_id = proto.originator_shard_id();
    update.ttl = proto.ttl();
    return update;
}

// ==============================================================================
// ResourceSnapshot Implementation
// ==============================================================================

proto::ResourceSnapshot ResourceSnapshot::toProto() const {
    proto::ResourceSnapshot proto_snapshot;
    proto_snapshot.set_shard_id(shard_id);
    proto_snapshot.set_timestamp_ns(timestamp_ns);
    proto_snapshot.set_available_memory_bytes(available_memory_bytes);
    proto_snapshot.set_total_memory_bytes(total_memory_bytes);
    proto_snapshot.set_available_cpu_cores(available_cpu_cores);
    proto_snapshot.set_total_cpu_cores(total_cpu_cores);
    proto_snapshot.set_available_disk_bytes(available_disk_bytes);
    proto_snapshot.set_total_disk_bytes(total_disk_bytes);
    proto_snapshot.set_rocksdb_sst_files_count(rocksdb_sst_files_count);
    proto_snapshot.set_rocksdb_total_size_bytes(rocksdb_total_size_bytes);
    proto_snapshot.set_cpu_usage_percent(cpu_usage_percent);
    proto_snapshot.set_memory_usage_percent(memory_usage_percent);
    proto_snapshot.set_disk_usage_percent(disk_usage_percent);
    proto_snapshot.set_requests_per_second(requests_per_second);
    proto_snapshot.set_avg_latency_ms(avg_latency_ms);
    proto_snapshot.set_is_healthy(is_healthy);
    proto_snapshot.set_status(status);
    for (const auto& warning : warnings) {
        proto_snapshot.add_warnings(warning);
    }
    return proto_snapshot;
}

ResourceSnapshot ResourceSnapshot::fromProto(const proto::ResourceSnapshot& proto) {
    ResourceSnapshot snapshot;
    snapshot.shard_id = proto.shard_id();
    snapshot.timestamp_ns = proto.timestamp_ns();
    snapshot.available_memory_bytes = proto.available_memory_bytes();
    snapshot.total_memory_bytes = proto.total_memory_bytes();
    snapshot.available_cpu_cores = proto.available_cpu_cores();
    snapshot.total_cpu_cores = proto.total_cpu_cores();
    snapshot.available_disk_bytes = proto.available_disk_bytes();
    snapshot.total_disk_bytes = proto.total_disk_bytes();
    snapshot.rocksdb_sst_files_count = proto.rocksdb_sst_files_count();
    snapshot.rocksdb_total_size_bytes = proto.rocksdb_total_size_bytes();
    snapshot.cpu_usage_percent = proto.cpu_usage_percent();
    snapshot.memory_usage_percent = proto.memory_usage_percent();
    snapshot.disk_usage_percent = proto.disk_usage_percent();
    snapshot.requests_per_second = proto.requests_per_second();
    snapshot.avg_latency_ms = proto.avg_latency_ms();
    snapshot.is_healthy = proto.is_healthy();
    snapshot.status = proto.status();
    for (const auto& warning : proto.warnings()) {
        snapshot.warnings.push_back(warning);
    }
    return snapshot;
}

// ==============================================================================
// GossipConfigManager Implementation
// ==============================================================================

GossipConfigManager::GossipConfigManager(
    const GossipConfigManagerConfig& config,
    std::shared_ptr<ShardTopology> topology,
    std::shared_ptr<PrometheusMetrics> metrics
) : config_(config),
    topology_(std::move(topology)),
    metrics_(std::move(metrics)) {
    
    // Initialize mTLS client if required
    if (config_.require_mtls) {
        MTLSClient::Config client_config;
        client_config.verify_peer = true;
        client_config.connect_timeout_ms = 5000;
        client_config.request_timeout_ms = 10000;
        client_config.max_retries = 2;
        client_ = std::make_unique<MTLSClient>(client_config);
    }
    
    // Initialize local clock
    {
        std::lock_guard<std::mutex> lock(clock_mutex_);
        local_clock_.set(config_.local_shard_id, 0);
    }
}

GossipConfigManager::~GossipConfigManager() {
    stop();
}

void GossipConfigManager::start() {
    if (!config_.enabled) {
        return;  // Manager disabled
    }
    
    if (running_.load()) {
        return;  // Already running
    }
    
    running_.store(true);
    
    // Start gossip thread
    gossip_thread_ = std::thread(&GossipConfigManager::gossipLoop, this);
    
    // Start anti-entropy thread
    anti_entropy_thread_ = std::thread(&GossipConfigManager::antiEntropyLoop, this);
}

void GossipConfigManager::stop() {
    if (!running_.load()) {
        return;  // Already stopped
    }
    
    running_.store(false);
    
    if (gossip_thread_.joinable()) {
        gossip_thread_.join();
    }
    
    if (anti_entropy_thread_.joinable()) {
        anti_entropy_thread_.join();
    }
}

std::string GossipConfigManager::publishConfigUpdate(
    const std::string& config_key,
    const std::string& config_value
) {
    ConfigUpdate update;
    update.update_id = generateUpdateId();
    update.config_key = config_key;
    update.config_value = config_value;
    update.timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    update.originator_shard_id = config_.local_shard_id;
    update.ttl = config_.update_ttl;
    
    // Increment local clock
    {
        std::lock_guard<std::mutex> lock(clock_mutex_);
        local_clock_.increment(config_.local_shard_id);
        update.vector_clock = local_clock_;
    }
    
    // Store update locally
    handleConfigUpdate(update);
    
    // Immediately propagate to peers
    auto peers = selectRandomPeers(config_.fanout);
    for (const auto& peer : peers) {
        if (!topology_) continue;
        auto shard_info = topology_->getShard(peer);
        if (shard_info) {
            sendGossipMessage(shard_info->primary_endpoint, 
                            createConfigUpdateMessage(update));
        }
    }
    
    config_updates_sent_++;
    
    // Record metric
    if (metrics_) {
        metrics_->recordGossipConfigUpdate("sent");
    }
    
    return update.update_id;
}

void GossipConfigManager::publishResourceSnapshot(const ResourceSnapshot& snapshot) {
    // Store snapshot locally
    handleResourceSnapshot(snapshot);
    
    // Propagate to peers
    auto peers = selectRandomPeers(config_.fanout);
    for (const auto& peer : peers) {
        if (!topology_) continue;
        auto shard_info = topology_->getShard(peer);
        if (shard_info) {
            sendGossipMessage(shard_info->primary_endpoint,
                            createResourceSnapshotMessage(snapshot));
        }
    }
    
    resource_snapshots_sent_++;
    
    // Record metric
    if (metrics_) {
        metrics_->recordGossipResourceSnapshot("sent");
    }
}

proto::GossipMessage GossipConfigManager::handleGossipMessage(
    const proto::GossipMessage& message
) {
    messages_received_++;
    
    // Record metric
    if (metrics_) {
        metrics_->recordGossipMessagesReceived();
    }
    
    // Merge vector clock
    if (message.has_vector_clock()) {
        mergeVectorClock(VectorClock::fromProto(message.vector_clock()));
    }
    
    // Handle message based on type
    if (message.message_type() == "config_update") {
        if (message.has_config_update()) {
            auto update = ConfigUpdate::fromProto(message.config_update());
            handleConfigUpdate(update);
        }
    } else if (message.message_type() == "resource_snapshot") {
        if (message.has_resource_snapshot()) {
            auto snapshot = ResourceSnapshot::fromProto(message.resource_snapshot());
            handleResourceSnapshot(snapshot);
        }
    } else if (message.message_type() == "heartbeat") {
        // Respond with our heartbeat
        return createHeartbeatMessage();
    } else if (message.message_type() == "anti_entropy") {
        // Handle anti-entropy request
        return createAntiEntropyMessage();
    }
    
    // Return acknowledgment
    proto::GossipMessage ack;
    ack.set_sender_shard_id(config_.local_shard_id);
    ack.set_timestamp_ns(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count());
    ack.set_message_type("ack");
    
    {
        std::lock_guard<std::mutex> lock(clock_mutex_);
        *ack.mutable_vector_clock() = local_clock_.toProto();
    }
    
    return ack;
}

void GossipConfigManager::onConfigUpdate(ConfigUpdateCallback callback) {
    config_update_callback_ = std::move(callback);
}

void GossipConfigManager::onResourceSnapshot(ResourceSnapshotCallback callback) {
    resource_snapshot_callback_ = std::move(callback);
}

std::string GossipConfigManager::getConfig(const std::string& config_key) const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    auto it = current_config_.find(config_key);
    return (it != current_config_.end()) ? it->second : "";
}

std::map<std::string, std::string> GossipConfigManager::getAllConfigs() const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    return current_config_;
}

ResourceSnapshot GossipConfigManager::getResourceSnapshot(const std::string& shard_id) const {
    std::lock_guard<std::mutex> lock(resource_mutex_);
    auto it = resource_snapshots_.find(shard_id);
    return (it != resource_snapshots_.end()) ? it->second : ResourceSnapshot{};
}

std::map<std::string, ResourceSnapshot> GossipConfigManager::getAllResourceSnapshots() const {
    std::lock_guard<std::mutex> lock(resource_mutex_);
    return resource_snapshots_;
}

VectorClock GossipConfigManager::getVectorClock() const {
    std::lock_guard<std::mutex> lock(clock_mutex_);
    return local_clock_;
}

GossipConfigManager::Statistics GossipConfigManager::getStatistics() const {
    Statistics stats;
    stats.gossip_rounds = gossip_rounds_.load();
    stats.messages_sent = messages_sent_.load();
    stats.messages_received = messages_received_.load();
    stats.config_updates_sent = config_updates_sent_.load();
    stats.config_updates_received = config_updates_received_.load();
    stats.resource_snapshots_sent = resource_snapshots_sent_.load();
    stats.resource_snapshots_received = resource_snapshots_received_.load();
    stats.conflicts_resolved = conflicts_resolved_.load();
    stats.anti_entropy_syncs = anti_entropy_syncs_.load();
    
    // Calculate average propagation latency
    {
        std::lock_guard<std::mutex> lock(latency_mutex_);
        if (!propagation_latencies_ms_.empty()) {
            double sum = 0.0;
            for (const auto& latency : propagation_latencies_ms_) {
                sum += latency;
            }
            stats.avg_propagation_latency_ms = sum / propagation_latencies_ms_.size();
        } else {
            stats.avg_propagation_latency_ms = 0.0;
        }
    }
    
    return stats;
}

// ==============================================================================
// Private Methods
// ==============================================================================

void GossipConfigManager::gossipLoop() {
    while (running_.load()) {
        performGossipRound();
        
        // Sleep for gossip interval
        auto start = std::chrono::steady_clock::now();
        while (running_.load()) {
            auto elapsed = std::chrono::steady_clock::now() - start;
            if (elapsed >= std::chrono::milliseconds(config_.gossip_interval_ms)) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void GossipConfigManager::antiEntropyLoop() {
    while (running_.load()) {
        performAntiEntropyScan();
        
        // Sleep for anti-entropy interval
        auto start = std::chrono::steady_clock::now();
        while (running_.load()) {
            auto elapsed = std::chrono::steady_clock::now() - start;
            if (elapsed >= std::chrono::milliseconds(config_.anti_entropy_interval_ms)) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void GossipConfigManager::performGossipRound() {
    gossip_rounds_++;
    
    // Record gossip round metric
    if (metrics_) {
        metrics_->recordGossipConfigRound();
    }
    
    // Select random peers
    auto peers = selectRandomPeers(config_.fanout);
    
    // Send heartbeat to each peer
    for (const auto& peer : peers) {
        if (!topology_) continue;
        
        auto shard_info = topology_->getShard(peer);
        if (shard_info && shard_info->is_healthy) {
            sendGossipMessage(shard_info->primary_endpoint, createHeartbeatMessage());
        }
    }
}

void GossipConfigManager::performAntiEntropyScan() {
    anti_entropy_syncs_++;
    
    // Record anti-entropy metric
    if (metrics_) {
        metrics_->recordGossipConfigAntiEntropy();
    }
    
    // Select random peers for anti-entropy
    auto peers = selectRandomPeers(config_.fanout);
    
    for (const auto& peer : peers) {
        if (!topology_) continue;
        
        auto shard_info = topology_->getShard(peer);
        if (shard_info && shard_info->is_healthy) {
            sendGossipMessage(shard_info->primary_endpoint, createAntiEntropyMessage());
        }
    }
}

std::vector<std::string> GossipConfigManager::selectRandomPeers(size_t count) {
    std::vector<std::string> selected;
    
    if (!topology_) {
        return selected;
    }
    
    auto all_shards = topology_->getAllShards();
    
    // Filter out ourselves and unhealthy shards
    std::vector<std::string> candidates;
    for (const auto& shard : all_shards) {
        if (shard.shard_id != config_.local_shard_id && shard.is_healthy) {
            candidates.push_back(shard.shard_id);
        }
    }
    
    if (candidates.empty()) {
        return selected;
    }
    
    // Random selection
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    std::shuffle(candidates.begin(), candidates.end(), gen);
    
    size_t select_count = std::min(count, candidates.size());
    selected.insert(selected.end(), candidates.begin(), candidates.begin() + select_count);
    
    return selected;
}

void GossipConfigManager::sendGossipMessage(
    const std::string& peer_endpoint [[maybe_unused]],
    const proto::GossipMessage& message [[maybe_unused]]
) {
    if (!client_) return;
    
    try {
        // Serialize message to JSON for HTTP POST
        // In a real implementation, this would use protobuf serialization
        // For now, we'll skip the actual network call
        messages_sent_++;
        
        // Track propagation latency
        auto now = std::chrono::steady_clock::now();
        // In a real implementation, we'd measure round-trip time
        
    } catch ([[maybe_unused]] const std::exception& e) {
        // silence unused warning in stub
        // Log error (in production, use proper logging)
    }
}

void GossipConfigManager::handleConfigUpdate(const ConfigUpdate& update) {
    // Check if we should accept this update
    if (!shouldAcceptUpdate(update)) {
        return;
    }
    
    // Store the update
    {
        std::lock_guard<std::mutex> lock(config_mutex_);
        
        // Check for conflicts using vector clock
        auto it = config_updates_.find(update.config_key);
        if (it != config_updates_.end()) {
            auto ordering = update.vector_clock.compare(it->second.vector_clock);
            
            if (ordering == VectorClock::Ordering::BEFORE) {
                // Old update, ignore
                return;
            } else if (ordering == VectorClock::Ordering::CONCURRENT) {
                // Conflict! Resolve by timestamp (last-write-wins)
                if (update.timestamp_ns <= it->second.timestamp_ns) {
                    return;  // Keep existing update
                }
                conflicts_resolved_++;
                
                // Record conflict metric
                if (metrics_) {
                    metrics_->recordGossipConfigConflict("last_write_wins");
                }
            }
        }
        
        // Accept the update
        config_updates_[update.config_key] = update;
        current_config_[update.config_key] = update.config_value;
    }
    
    config_updates_received_++;
    
    // Record metric
    if (metrics_) {
        metrics_->recordGossipConfigUpdate("received");
    }
    
    // Notify callback
    if (config_update_callback_) {
        config_update_callback_(update);
    }
    
    // Calculate and track propagation latency
    auto now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    double latency_ms = (now_ns - update.timestamp_ns) / 1e6;
    {
        std::lock_guard<std::mutex> lock(latency_mutex_);
        propagation_latencies_ms_.push_back(latency_ms);
        
        // Keep only last N latencies
        static constexpr size_t MAX_LATENCY_SAMPLES = 1000;
        if (propagation_latencies_ms_.size() > MAX_LATENCY_SAMPLES) {
            propagation_latencies_ms_.erase(propagation_latencies_ms_.begin());
        }
    }
    
    // Record propagation latency metric
    if (metrics_) {
        metrics_->observeGossipPropagationLatency(latency_ms);
    }
}

void GossipConfigManager::handleResourceSnapshot(const ResourceSnapshot& snapshot) {
    {
        std::lock_guard<std::mutex> lock(resource_mutex_);
        
        // Check if we have a newer snapshot
        auto it = resource_snapshots_.find(snapshot.shard_id);
        if (it != resource_snapshots_.end() && 
            it->second.timestamp_ns > snapshot.timestamp_ns) {
            return;  // Keep existing snapshot
        }
        
        resource_snapshots_[snapshot.shard_id] = snapshot;
    }
    
    resource_snapshots_received_++;
    
    // Record metric
    if (metrics_) {
        metrics_->recordGossipResourceSnapshot("received");
    }
    
    // Notify callback
    if (resource_snapshot_callback_) {
        resource_snapshot_callback_(snapshot);
    }
}

bool GossipConfigManager::shouldAcceptUpdate(const ConfigUpdate& update) {
    // Check TTL
    if (update.ttl == 0) {
        return false;
    }
    
    // Check update age
    auto now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    // Reject updates older than 1 hour
    static constexpr uint64_t MAX_UPDATE_AGE_NS = 3600ULL * 1000000000ULL;
    if (now_ns - update.timestamp_ns > MAX_UPDATE_AGE_NS) {
        return false;
    }
    
    // Check max updates limit
    {
        std::lock_guard<std::mutex> lock(config_mutex_);
        if (config_updates_.size() >= config_.max_updates) {
            // Remove oldest update
            auto oldest_it = config_updates_.begin();
            for (auto it = config_updates_.begin(); it != config_updates_.end(); ++it) {
                if (it->second.timestamp_ns < oldest_it->second.timestamp_ns) {
                    oldest_it = it;
                }
            }
            config_updates_.erase(oldest_it);
        }
    }
    
    return true;
}

void GossipConfigManager::mergeVectorClock(const VectorClock& other) {
    std::lock_guard<std::mutex> lock(clock_mutex_);
    local_clock_.merge(other);
}

std::string GossipConfigManager::generateUpdateId() const {
    // Generate a simple unique ID using timestamp and random number
    // For production, consider using Windows UUID APIs (CoCreateGuid) or boost::uuid
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    uint64_t random_part = dis(gen);
    
    std::stringstream ss;
    ss << std::hex << timestamp << "-" << random_part;
    return ss.str();
}

proto::GossipMessage GossipConfigManager::createHeartbeatMessage() {
    proto::GossipMessage message;
    message.set_sender_shard_id(config_.local_shard_id);
    message.set_timestamp_ns(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count());
    message.set_message_type("heartbeat");
    
    {
        std::lock_guard<std::mutex> lock(clock_mutex_);
        *message.mutable_vector_clock() = local_clock_.toProto();
    }
    
    return message;
}

proto::GossipMessage GossipConfigManager::createConfigUpdateMessage(
    const ConfigUpdate& update
) {
    proto::GossipMessage message;
    message.set_sender_shard_id(config_.local_shard_id);
    message.set_timestamp_ns(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count());
    message.set_message_type("config_update");
    
    {
        std::lock_guard<std::mutex> lock(clock_mutex_);
        *message.mutable_vector_clock() = local_clock_.toProto();
    }
    
    *message.mutable_config_update() = update.toProto();
    
    return message;
}

proto::GossipMessage GossipConfigManager::createResourceSnapshotMessage(
    const ResourceSnapshot& snapshot
) {
    proto::GossipMessage message;
    message.set_sender_shard_id(config_.local_shard_id);
    message.set_timestamp_ns(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count());
    message.set_message_type("resource_snapshot");
    
    {
        std::lock_guard<std::mutex> lock(clock_mutex_);
        *message.mutable_vector_clock() = local_clock_.toProto();
    }
    
    *message.mutable_resource_snapshot() = snapshot.toProto();
    
    return message;
}

proto::GossipMessage GossipConfigManager::createAntiEntropyMessage() {
    proto::GossipMessage message;
    message.set_sender_shard_id(config_.local_shard_id);
    message.set_timestamp_ns(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count());
    message.set_message_type("anti_entropy");
    
    {
        std::lock_guard<std::mutex> lock(clock_mutex_);
        *message.mutable_vector_clock() = local_clock_.toProto();
    }
    
    // In a full implementation, this would include checksums or
    // Merkle tree digests of the config state
    
    return message;
}

} // namespace sharding
} // namespace themis
