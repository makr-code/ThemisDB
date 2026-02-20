/**
 * ThemisDB Replication Manager Implementation
 * 
 * Leader-Follower Replication with Raft-like Consensus
 * 
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "replication/replication_manager.h"
#include "replication/multi_master_replication.h"
#include "utils/logger.h"
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <fstream>
#include <filesystem>
#include <random>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace themisdb {
namespace replication {

// ============================================================================
// WALEntry Implementation
// ============================================================================

std::vector<uint8_t> WALEntry::serialize() const {
    std::vector<uint8_t> result;
    
    // Header: sequence_number (8) + term (8) + timestamp (8) + lengths
    auto appendUint64 = [&result](uint64_t val) {
        for (int i = 7; i >= 0; --i) {
            result.push_back(static_cast<uint8_t>((val >> (i * 8)) & 0xFF));
        }
    };
    
    auto appendString = [&result](const std::string& s) {
        uint32_t len = static_cast<uint32_t>(s.size());
        result.push_back(static_cast<uint8_t>((len >> 24) & 0xFF));
        result.push_back(static_cast<uint8_t>((len >> 16) & 0xFF));
        result.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));
        result.push_back(static_cast<uint8_t>(len & 0xFF));
        result.insert(result.end(), s.begin(), s.end());
    };
    
    appendUint64(sequence_number);
    appendUint64(term);
    
    auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
        timestamp.time_since_epoch()
    ).count();
    appendUint64(static_cast<uint64_t>(ts));
    
    appendString(operation);
    appendString(collection);
    appendString(document_id);
    appendString(data);
    appendString(checksum);
    
    return result;
}

std::optional<WALEntry> WALEntry::deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 24) {  // Minimum header size
        return std::nullopt;
    }
    
    size_t pos = 0;
    
    auto readUint64 = [&data, &pos]() -> uint64_t {
        uint64_t val = 0;
        for (size_t i = 0; i < 8 && pos < data.size(); ++i, ++pos) {
            val = (val << 8) | data[pos];
        }
        return val;
    };
    
    auto readString = [&data, &pos]() -> std::string {
        if (pos + 4 > data.size()) return "";
        uint32_t len = (static_cast<uint32_t>(data[pos]) << 24) |
                       (static_cast<uint32_t>(data[pos+1]) << 16) |
                       (static_cast<uint32_t>(data[pos+2]) << 8) |
                       static_cast<uint32_t>(data[pos+3]);
        pos += 4;
        if (pos + len > data.size()) return "";
        std::string s(data.begin() + pos, data.begin() + pos + len);
        pos += len;
        return s;
    };
    
    WALEntry entry;
    entry.sequence_number = readUint64();
    entry.term = readUint64();
    
    uint64_t ts_ms = readUint64();
    entry.timestamp = std::chrono::system_clock::time_point(
        std::chrono::milliseconds(ts_ms)
    );
    
    entry.operation = readString();
    entry.collection = readString();
    entry.document_id = readString();
    entry.data = readString();
    entry.checksum = readString();
    
    return entry;
}

// ============================================================================
// ReplicaInfo Implementation
// ============================================================================

int64_t ReplicaInfo::replicationLagMs() const {
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        now - last_heartbeat
    ).count();
}

void ReplicaInfo::updateHealthStatus(uint32_t heartbeat_timeout_ms, uint32_t degraded_lag_threshold_ms) {
    auto now = std::chrono::system_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - last_heartbeat
    ).count();
    
    HealthStatus old_status = health_status;
    
    // Check if replica has timed out
    if (elapsed_ms > heartbeat_timeout_ms) {
        consecutive_failures++;
        last_failure_time = now;
        health_status = HealthStatus::FAILED;
    }
    // Check if replica is lagging
    else if (elapsed_ms > degraded_lag_threshold_ms) {
        health_status = HealthStatus::DEGRADED;
        consecutive_failures = 0;  // Reset failure count if responsive
    }
    // Replica is healthy
    else {
        health_status = HealthStatus::HEALTHY;
        consecutive_failures = 0;
    }
}

// ============================================================================
// ReplicationStats Implementation
// ============================================================================

std::string ReplicationStats::toPrometheusFormat() const {
    std::ostringstream oss;
    
    oss << "# HELP themisdb_replication_entries_total Total WAL entries replicated\n"
        << "# TYPE themisdb_replication_entries_total counter\n"
        << "themisdb_replication_entries_total " << entries_replicated.load() << "\n\n";
    
    oss << "# HELP themisdb_replication_bytes_total Total bytes replicated\n"
        << "# TYPE themisdb_replication_bytes_total counter\n"
        << "themisdb_replication_bytes_total " << bytes_replicated.load() << "\n\n";
    
    oss << "# HELP themisdb_replication_errors_total Total replication errors\n"
        << "# TYPE themisdb_replication_errors_total counter\n"
        << "themisdb_replication_errors_total " << replication_errors.load() << "\n\n";
    
    oss << "# HELP themisdb_leader_elections_total Total leader elections\n"
        << "# TYPE themisdb_leader_elections_total counter\n"
        << "themisdb_leader_elections_total " << leader_elections.load() << "\n\n";
    
    oss << "# HELP themisdb_conflicts_resolved_total Total conflicts resolved\n"
        << "# TYPE themisdb_conflicts_resolved_total counter\n"
        << "themisdb_conflicts_resolved_total " << conflicts_resolved.load() << "\n\n";
    
    oss << "# HELP themisdb_replication_lag_ms Current replication lag in milliseconds\n"
        << "# TYPE themisdb_replication_lag_ms gauge\n"
        << "themisdb_replication_lag_max_ms " << max_replication_lag_ms.load() << "\n"
        << "themisdb_replication_lag_avg_ms " << avg_replication_lag_ms.load() << "\n\n";
    
    oss << "# HELP themisdb_automatic_failovers_total Total automatic failovers executed\n"
        << "# TYPE themisdb_automatic_failovers_total counter\n"
        << "themisdb_automatic_failovers_total " << automatic_failovers.load() << "\n\n";
    
    oss << "# HELP themisdb_manual_failovers_total Total manual failovers executed\n"
        << "# TYPE themisdb_manual_failovers_total counter\n"
        << "themisdb_manual_failovers_total " << manual_failovers.load() << "\n\n";
    
    oss << "# HELP themisdb_replica_failures_detected_total Total replica failures detected\n"
        << "# TYPE themisdb_replica_failures_detected_total counter\n"
        << "themisdb_replica_failures_detected_total " << replica_failures_detected.load() << "\n\n";
    
    oss << "# HELP themisdb_network_partitions_detected_total Total network partitions detected\n"
        << "# TYPE themisdb_network_partitions_detected_total counter\n"
        << "themisdb_network_partitions_detected_total " << network_partitions_detected.load() << "\n";
    
    return oss.str();
}

// ============================================================================
// WALManager Implementation
// ============================================================================

WALManager::WALManager(const ReplicationConfig& config)
    : config_(config) {
    // Ensure WAL directory exists
    std::filesystem::create_directories(config_.wal_directory);
    loadFromDisk();
}

WALManager::~WALManager() {
    sync();
}

uint64_t WALManager::append(const WALEntry& entry) {
    std::lock_guard<std::mutex> lock(wal_mutex_);
    
    uint64_t seq = ++current_sequence_;
    
    // Create a copy with the assigned sequence number
    WALEntry stored_entry = entry;
    stored_entry.sequence_number = seq;
    stored_entry.term = current_term_.load();
    
    // Compute checksum if not provided
    if (stored_entry.checksum.empty()) {
        std::string content = stored_entry.operation + stored_entry.collection + 
                             stored_entry.document_id + stored_entry.data;
        
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(content.c_str()),
               content.size(), hash);
        
        std::ostringstream oss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
            oss << std::hex << std::setw(2) << std::setfill('0') 
                << static_cast<int>(hash[i]);
        }
        stored_entry.checksum = oss.str();
    }
    
    // Serialize and write to current segment
    auto serialized = stored_entry.serialize();
    
    std::string segment_path = config_.wal_directory + "/wal_" + 
                               std::to_string(seq / 10000) + ".log";
    
    std::ofstream ofs(segment_path, std::ios::binary | std::ios::app);
    if (ofs) {
        // Write length prefix
        uint32_t len = static_cast<uint32_t>(serialized.size());
        ofs.write(reinterpret_cast<char*>(&len), sizeof(len));
        ofs.write(reinterpret_cast<char*>(serialized.data()), serialized.size());
        
        if (config_.wal_sync_on_commit) {
            ofs.flush();
        }
    }
    
    return seq;
}

std::vector<WALEntry> WALManager::readFrom(uint64_t start_sequence, uint32_t limit) {
    std::lock_guard<std::mutex> lock(wal_mutex_);
    std::vector<WALEntry> entries;
    
    // Find starting segment
    uint64_t segment_id = start_sequence / 10000;
    
    for (uint64_t seg = segment_id; entries.size() < limit; ++seg) {
        std::string segment_path = config_.wal_directory + "/wal_" + 
                                   std::to_string(seg) + ".log";
        
        if (!std::filesystem::exists(segment_path)) {
            break;
        }
        
        std::ifstream ifs(segment_path, std::ios::binary);
        if (!ifs) continue;
        
        while (entries.size() < limit) {
            uint32_t len = 0;
            ifs.read(reinterpret_cast<char*>(&len), sizeof(len));
            if (ifs.eof() || len == 0) break;
            
            // Guard against oversized or corrupt length fields
            if (len > 64 * 1024 * 1024) {
                THEMIS_ERROR("WAL segment {}: corrupt record length {}, stopping read", seg, len);
                break;
            }
            
            std::vector<uint8_t> data(len);
            ifs.read(reinterpret_cast<char*>(data.data()), len);
            if (static_cast<uint32_t>(ifs.gcount()) != len) {
                THEMIS_ERROR("WAL segment {}: incomplete read (expected {} bytes, got {})",
                             seg, len, ifs.gcount());
                break;
            }
            
            auto entry = WALEntry::deserialize(data);
            if (!entry) {
                THEMIS_ERROR("WAL segment {}: failed to deserialize entry", seg);
                continue;
            }
            
            if (entry->sequence_number >= start_sequence) {
                // Verify checksum to detect silent data corruption
                if (!entry->checksum.empty()) {
                    std::string content = entry->operation + entry->collection +
                                         entry->document_id + entry->data;
                    unsigned char hash[SHA256_DIGEST_LENGTH];
                    SHA256(reinterpret_cast<const unsigned char*>(content.c_str()),
                           content.size(), hash);
                    std::ostringstream oss;
                    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
                        oss << std::hex << std::setw(2) << std::setfill('0')
                            << static_cast<int>(hash[i]);
                    }
                    if (oss.str() != entry->checksum) {
                        THEMIS_ERROR("WAL entry seq={} checksum mismatch – possible data corruption, skipping",
                                     entry->sequence_number);
                        continue;
                    }
                }
                entries.push_back(*entry);
            }
        }
    }
    
    return entries;
}

uint64_t WALManager::incrementTerm() {
    return ++current_term_;
}

void WALManager::truncateBefore(uint64_t sequence) {
    std::lock_guard<std::mutex> lock(wal_mutex_);
    
    uint64_t segment_id = sequence / 10000;
    
    // Remove old segments
    for (uint64_t seg = 0; seg < segment_id; ++seg) {
        std::string segment_path = config_.wal_directory + "/wal_" + 
                                   std::to_string(seg) + ".log";
        std::filesystem::remove(segment_path);
    }
}

void WALManager::sync() {
    // Force sync of all open file handles
    // This is a simplified implementation
}

uint64_t WALManager::getSize() const {
    uint64_t total = 0;
    for (const auto& entry : std::filesystem::directory_iterator(config_.wal_directory)) {
        if (entry.path().extension() == ".log") {
            total += std::filesystem::file_size(entry.path());
        }
    }
    return total;
}

void WALManager::loadFromDisk() {
    // Find highest sequence number from existing WAL files
    uint64_t max_seq = 0;
    uint64_t max_term = 0;
    
    for (const auto& entry : std::filesystem::directory_iterator(config_.wal_directory)) {
        if (entry.path().extension() == ".log") {
            std::ifstream ifs(entry.path(), std::ios::binary);
            
            while (true) {
                uint32_t len = 0;
                ifs.read(reinterpret_cast<char*>(&len), sizeof(len));
                if (ifs.eof() || len == 0) break;
                
                std::vector<uint8_t> data(len);
                ifs.read(reinterpret_cast<char*>(data.data()), len);
                if (ifs.eof()) break;
                
                auto wal_entry = WALEntry::deserialize(data);
                if (wal_entry) {
                    max_seq = std::max(max_seq, wal_entry->sequence_number);
                    max_term = std::max(max_term, wal_entry->term);
                }
            }
        }
    }
    
    current_sequence_.store(max_seq);
    current_term_.store(max_term);
}

// ============================================================================
// LeaderElection Implementation
// ============================================================================

LeaderElection::LeaderElection(
    const std::string& node_id,
    const ReplicationConfig& config,
    std::shared_ptr<WALManager> wal)
    : node_id_(node_id)
    , config_(config)
    , wal_(wal)
    , current_term_(wal->getCurrentTerm()) {
}

LeaderElection::~LeaderElection() {
    running_.store(false);
    election_cv_.notify_all();
    if (election_thread_.joinable()) {
        election_thread_.join();
    }
}

void LeaderElection::startElection() {
    std::lock_guard<std::mutex> lock(election_mutex_);
    
    // Increment term and become candidate
    current_term_ = wal_->incrementTerm();
    role_.store(ReplicationRole::CANDIDATE);
    voted_for_ = node_id_;  // Vote for self
    
    // Start with 1 vote (self)
    votes_received_.store(1);
    last_heartbeat_time_ = std::chrono::steady_clock::now();
    
    uint32_t cluster = cluster_size_.load();
    uint32_t quorum = (cluster / 2) + 1;
    
    // In a single-node cluster we immediately win; in a multi-node cluster
    // the ReplicationManager will inject votes via grantVote() as peers respond
    // to RequestVote RPCs.  We only promote ourselves when we have a quorum.
    if (votes_received_.load() >= quorum) {
        THEMIS_INFO("Node {} won election for term {} (cluster_size={}, quorum={})",
                    node_id_, current_term_.load(), cluster, quorum);
        becomeLeader();
    }
}

bool LeaderElection::requestVote(
    uint64_t term,
    const std::string& candidate_id,
    uint64_t last_log_sequence,
    uint64_t last_log_term) {
    
    std::lock_guard<std::mutex> lock(election_mutex_);
    
    // If term is stale, reject
    if (term < current_term_) {
        return false;
    }
    
    // If term is newer, update and become follower
    if (term > current_term_) {
        current_term_ = term;
        role_.store(ReplicationRole::FOLLOWER);
        voted_for_.clear();
    }
    
    // Grant vote if haven't voted yet and candidate's log is up-to-date
    if (voted_for_.empty() || voted_for_ == candidate_id) {
        uint64_t my_last_seq = wal_->getCurrentSequence();
        uint64_t my_last_term = wal_->getCurrentTerm();
        
        // Candidate's log must be at least as up-to-date
        if (last_log_term > my_last_term ||
            (last_log_term == my_last_term && last_log_sequence >= my_last_seq)) {
            voted_for_ = candidate_id;
            return true;
        }
    }
    
    return false;
}

void LeaderElection::receiveHeartbeat(
    uint64_t term,
    const std::string& leader_id,
    uint64_t leader_commit) {
    
    std::lock_guard<std::mutex> lock(election_mutex_);
    
    if (term >= current_term_) {
        becomeFollower(term, leader_id);
    }
}

std::string LeaderElection::getLeaderId() const {
    return current_leader_;
}

void LeaderElection::becomeLeader() {
    role_.store(ReplicationRole::LEADER);
    current_leader_ = node_id_;
}

void LeaderElection::becomeFollower(uint64_t term, const std::string& leader_id) {
    current_term_ = term;
    role_.store(ReplicationRole::FOLLOWER);
    current_leader_ = leader_id;
    voted_for_.clear();
    last_heartbeat_time_ = std::chrono::steady_clock::now();
}

void LeaderElection::grantVote(uint64_t term) {
    std::lock_guard<std::mutex> lock(election_mutex_);
    
    // Only count votes for the current term while we are still a candidate
    if (term != current_term_.load() ||
        role_.load() != ReplicationRole::CANDIDATE) {
        return;
    }
    
    uint32_t new_count = ++votes_received_;
    uint32_t cluster = cluster_size_.load();
    uint32_t quorum = (cluster / 2) + 1;
    
    if (new_count >= quorum) {
        THEMIS_INFO("Node {} won election for term {} ({}/{} votes, quorum={})",
                    node_id_, term, new_count, cluster, quorum);
        becomeLeader();
    }
}

// ============================================================================
// ReplicationStream Implementation
// ============================================================================

ReplicationStream::ReplicationStream(
    const std::string& follower_endpoint,
    std::shared_ptr<WALManager> wal,
    const ReplicationConfig& config)
    : follower_endpoint_(follower_endpoint)
    , wal_(wal)
    , config_(config) {
    
    follower_info_.endpoint = follower_endpoint;
    follower_info_.role = ReplicationRole::FOLLOWER;
    follower_info_.last_applied_sequence = 0;
    follower_info_.last_heartbeat = std::chrono::system_clock::now();
}

ReplicationStream::~ReplicationStream() {
    stop();
}

void ReplicationStream::start() {
    running_.store(true);
    stream_thread_ = std::thread(&ReplicationStream::streamLoop, this);
}

void ReplicationStream::stop() {
    running_.store(false);
    if (stream_thread_.joinable()) {
        stream_thread_.join();
    }
}

bool ReplicationStream::isHealthy() const {
    return running_.load() && follower_info_.isHealthy();
}

void ReplicationStream::streamLoop() {
    while (running_.load()) {
        uint64_t next_seq = last_acked_sequence_.load() + 1;
        auto entries = wal_->readFrom(next_seq, config_.batch_size);
        
        if (!entries.empty()) {
            if (sendBatch(entries)) {
                last_acked_sequence_.store(entries.back().sequence_number);
                follower_info_.last_applied_sequence = entries.back().sequence_number;
                follower_info_.last_heartbeat = std::chrono::system_clock::now();
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(config_.batch_timeout_ms));
    }
}

bool ReplicationStream::sendBatch(const std::vector<WALEntry>& entries) {
    // In a real implementation, this would send entries over mTLS to the follower
    // For now, simulate successful replication
    return true;
}

// ============================================================================
// ReplicationManager Implementation
// ============================================================================

ReplicationManager::ReplicationManager(const ReplicationConfig& config)
    : config_(config) {
    // Generate unique node ID if not provided
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    node_id_ = "node-" + std::to_string(dis(gen));
}

ReplicationManager::~ReplicationManager() {
    shutdown();
}

bool ReplicationManager::initialize() {
    if (initialized_.load()) {
        return true;
    }
    
    std::lock_guard<std::mutex> lock(manager_mutex_);
    
    // Validate configuration before proceeding
    if (!validateConfig()) {
        return false;
    }
    
    // Initialize WAL Manager
    wal_ = std::make_shared<WALManager>(config_);
    
    // Initialize Leader Election
    election_ = std::make_unique<LeaderElection>(node_id_, config_, wal_);
    
    // Connect to seed nodes
    for (const auto& seed : config_.seed_nodes) {
        ReplicaInfo replica;
        replica.endpoint = seed;
        replica.role = ReplicationRole::FOLLOWER;
        replica.last_heartbeat = std::chrono::system_clock::now();
        replicas_.push_back(replica);
    }
    
    // Inform election module of current cluster size (self + replicas)
    election_->setClusterSize(static_cast<uint32_t>(replicas_.size()) + 1);
    
    initialized_.store(true);
    running_.store(true);
    
    // Start background threads
    heartbeat_thread_ = std::thread(&ReplicationManager::heartbeatLoop, this);
    compaction_thread_ = std::thread(&ReplicationManager::compactionLoop, this);
    health_monitor_thread_ = std::thread(&ReplicationManager::healthMonitorLoop, this);
    
    return true;
}

void ReplicationManager::shutdown() {
    if (!initialized_.load()) {
        return;
    }
    
    running_.store(false);
    
    if (heartbeat_thread_.joinable()) {
        heartbeat_thread_.join();
    }
    if (compaction_thread_.joinable()) {
        compaction_thread_.join();
    }
    if (health_monitor_thread_.joinable()) {
        health_monitor_thread_.join();
    }
    
    for (auto& stream : streams_) {
        stream->stop();
    }
    streams_.clear();
    
    initialized_.store(false);
}

bool ReplicationManager::replicate(const WALEntry& entry) {
    if (!initialized_.load()) {
        THEMIS_ERROR("Replication not initialized");
        return false;
    }
    
    // Only leader can accept writes
    if (!election_->isLeader()) {
        return false;
    }
    
    try {
        // Append to WAL
        uint64_t seq = wal_->append(entry);
        if (seq == 0) {
            THEMIS_ERROR("WAL append failed for entry operation={} collection={}",
                         entry.operation, entry.collection);
            stats_.replication_errors++;
            return false;
        }
        
        stats_.entries_replicated++;
        stats_.bytes_replicated += entry.data.size();
        
        // For sync/semi-sync mode, wait for replication
        if (config_.mode != ReplicationMode::ASYNC) {
            return waitForReplication(seq, config_.replication_timeout_ms);
        }
        
        return true;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Replication error: {}", e.what());
        stats_.replication_errors++;
        return false;
    }
}

bool ReplicationManager::waitForReplication(uint64_t sequence, uint32_t timeout_ms) {
    auto deadline = std::chrono::steady_clock::now() + 
                   std::chrono::milliseconds(timeout_ms > 0 ? timeout_ms : config_.replication_timeout_ms);
    
    uint32_t acked_count = 0;
    uint32_t required;
    {
        std::shared_lock<std::shared_mutex> lock(replicas_mutex_);
        required = (config_.mode == ReplicationMode::SYNC)
                   ? static_cast<uint32_t>(streams_.size())
                   : config_.min_sync_replicas;
    }
    
    while (std::chrono::steady_clock::now() < deadline) {
        acked_count = 0;
        {
            std::shared_lock<std::shared_mutex> lock(replicas_mutex_);
            for (const auto& stream : streams_) {
                if (stream->getLastAckedSequence() >= sequence) {
                    acked_count++;
                }
            }
        }
        
        if (acked_count >= required) {
            return true;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    stats_.replication_errors++;
    return false;
}

ReplicationRole ReplicationManager::getRole() const {
    return election_ ? election_->getRole() : ReplicationRole::FOLLOWER;
}

std::string ReplicationManager::getLeaderEndpoint() const {
    if (election_ && !election_->isLeader()) {
        return election_->getLeaderId();
    }
    return "";
}

std::vector<ReplicaInfo> ReplicationManager::getReplicas() const {
    std::shared_lock<std::shared_mutex> lock(replicas_mutex_);
    return replicas_;
}

void ReplicationManager::addReplica(const ReplicaInfo& replica) {
    {
        std::unique_lock<std::shared_mutex> lock(replicas_mutex_);
        replicas_.push_back(replica);
        
        if (election_ && election_->isLeader()) {
            auto stream = std::make_unique<ReplicationStream>(
                replica.endpoint, wal_, config_
            );
            stream->start();
            streams_.push_back(std::move(stream));
        }
        
        // Update cluster size in election module
        if (election_) {
            election_->setClusterSize(static_cast<uint32_t>(replicas_.size()) + 1);
        }
    }
    
    notifyListeners([&replica](IReplicationListener& l) {
        l.onReplicaAdded(replica);
    });
}

void ReplicationManager::removeReplica(const std::string& node_id) {
    {
        std::unique_lock<std::shared_mutex> lock(replicas_mutex_);
        replicas_.erase(
            std::remove_if(replicas_.begin(), replicas_.end(),
                [&node_id](const ReplicaInfo& r) { return r.node_id == node_id; }),
            replicas_.end()
        );
        
        // Update cluster size in election module
        if (election_) {
            election_->setClusterSize(static_cast<uint32_t>(replicas_.size()) + 1);
        }
    }
    
    notifyListeners([&node_id](IReplicationListener& l) {
        l.onReplicaRemoved(node_id);
    });
}

void ReplicationManager::setConflictResolver(std::shared_ptr<IConflictResolver> resolver) {
    conflict_resolver_ = resolver;
}

void ReplicationManager::addListener(std::shared_ptr<IReplicationListener> listener) {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    listeners_.push_back(listener);
}

bool ReplicationManager::triggerFailover(const std::string& target_node_id) {
    // Manual failover
    stats_.manual_failovers++;
    
    // In a real implementation, this would send a message to target node to start election
    // For now, if this node is the target, start election
    if (target_node_id == node_id_ && election_) {
        election_->startElection();
        
        if (election_->isLeader()) {
            notifyListeners([this](IReplicationListener& l) {
                l.onFailoverCompleted(node_id_, true);
            });
            return true;
        }
    }
    
    return false;
}

bool ReplicationManager::promoteToLeader() {
    if (election_) {
        election_->startElection();
        return election_->isLeader();
    }
    return false;
}

bool ReplicationManager::demoteToFollower() {
    // Implementation would step down from leader role
    return true;
}

bool ReplicationManager::enableMultiRegion(const std::string& region_id,
                                          const std::vector<std::string>& peer_regions) {
    std::unique_lock<std::shared_mutex> lock(replicas_mutex_);
    
    THEMIS_INFO("Enabling multi-region replication for region: {}", region_id);
    
    // Add peer regions as replicas
    for (const auto& peer : peer_regions) {
        ReplicaInfo replica;
        replica.endpoint = peer;
        replica.datacenter = peer;  // Use endpoint as datacenter identifier
        replica.role = ReplicationRole::FOLLOWER;
        replica.last_heartbeat = std::chrono::system_clock::now();
        replica.is_voting_member = true;
        replica.priority = 5;  // Lower priority for remote regions
        
        replicas_.push_back(replica);
        
        // Create replication stream if we're the leader
        if (election_ && election_->isLeader()) {
            auto stream = std::make_unique<ReplicationStream>(
                replica.endpoint, wal_, config_
            );
            stream->start();
            streams_.push_back(std::move(stream));
        }
    }
    
    if (election_) {
        election_->setClusterSize(static_cast<uint32_t>(replicas_.size()) + 1);
    }
    
    THEMIS_INFO("Multi-region replication enabled with {} peer regions", peer_regions.size());
    return true;
}

bool ReplicationManager::promoteReplica(const std::string& replica_id) {
    std::unique_lock<std::shared_mutex> lock(replicas_mutex_);
    
    THEMIS_INFO("Promoting replica {} to primary", replica_id);
    
    // Find the replica
    auto it = std::find_if(replicas_.begin(), replicas_.end(),
        [&replica_id](const ReplicaInfo& r) { return r.node_id == replica_id; });
    
    if (it == replicas_.end()) {
        THEMIS_ERROR("Replica {} not found", replica_id);
        return false;
    }
    
    // Step 1: Verify replica has caught up to leader's commit index
    uint64_t current_sequence = wal_->getCurrentSequence();
    if (it->last_applied_sequence < current_sequence) {
        THEMIS_WARN("Replica {} is behind (seq: {} < {}), waiting for catch-up",
                   replica_id, it->last_applied_sequence, current_sequence);
        
        // Wait for replica to catch up (with timeout)
        auto wait_start = std::chrono::steady_clock::now();
        auto wait_timeout = std::chrono::milliseconds(config_.replication_timeout_ms);
        
        while (it->last_applied_sequence < current_sequence) {
            if (std::chrono::steady_clock::now() - wait_start > wait_timeout) {
                THEMIS_ERROR("Replica {} failed to catch up within timeout", replica_id);
                return false;
            }
            // Check every 100ms
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    // Step 2: Verify replica has all committed transactions
    if (it->last_applied_sequence != current_sequence) {
        THEMIS_ERROR("Replica {} sequence mismatch after catch-up", replica_id);
        return false;
    }
    
    // Step 3: Store old role for notification
    ReplicationRole old_role = it->role;
    
    // Step 4: Promote replica to primary role
    it->role = ReplicationRole::LEADER;
    
    // Update current term
    wal_->incrementTerm();
    
    THEMIS_INFO("Replica {} promoted to LEADER (sequence: {}, term: {})",
               replica_id, it->last_applied_sequence, wal_->getCurrentTerm());
    
    // Step 5: Update cluster routing - notify election system
    if (election_) {
        election_->receiveHeartbeat(
            wal_->getCurrentTerm(),
            replica_id,
            current_sequence
        );
    }
    
    // Step 6: Notify all other replicas of new leader via listeners
    notifyListeners([&replica_id](IReplicationListener& l) {
        l.onLeaderElected(replica_id);
    });
    
    // Mark failover in statistics
    stats_.manual_failovers++;
    
    THEMIS_INFO("Replica {} promoted successfully to primary", replica_id);
    return true;
}

bool ReplicationManager::setupCascadingReplication(const std::string& source_replica,
                                                   const std::vector<std::string>& target_replicas) {
    THEMIS_INFO("Setting up cascading replication: {} -> {} targets",
               source_replica, target_replicas.size());
    
    // In production, configure source replica to replicate to targets
    // This reduces load on primary by having intermediate replicas
    
    return true;
}

int64_t ReplicationManager::getReplicationLag(const std::string& replica_id) const {
    std::shared_lock<std::shared_mutex> lock(replicas_mutex_);
    
    // Find replica
    auto it = std::find_if(replicas_.begin(), replicas_.end(),
        [&replica_id](const ReplicaInfo& r) { return r.node_id == replica_id; });
    
    if (it != replicas_.end()) {
        return it->replicationLagMs();
    }
    
    return -1;  // Not found
}

std::map<std::string, bool> ReplicationManager::getClusterHealth() const {
    std::shared_lock<std::shared_mutex> lock(replicas_mutex_);
    std::map<std::string, bool> health;
    
    // Add self
    health[node_id_] = initialized_.load() && running_.load();
    
    // Add all replicas
    for (const auto& replica : replicas_) {
        health[replica.node_id] = replica.isHealthy();
    }
    
    return health;
}

std::string ReplicationManager::exportPrometheusMetrics() const {
    std::ostringstream oss;
    
    // Export basic stats
    oss << stats_.toPrometheusFormat();
    
    // Add cluster health metrics
    auto health = getClusterHealth();
    oss << "\n# HELP themisdb_cluster_nodes_healthy Healthy nodes in cluster\n"
        << "# TYPE themisdb_cluster_nodes_healthy gauge\n";
    
    uint32_t healthy_count = 0;
    for (const auto& [node_id, is_healthy] : health) {
        if (is_healthy) healthy_count++;
    }
    
    oss << "themisdb_cluster_nodes_healthy " << healthy_count << "\n";
    oss << "themisdb_cluster_nodes_total " << health.size() << "\n";
    
    // Add replication lag metrics per replica
    oss << "\n# HELP themisdb_replication_lag_per_replica Replication lag per replica\n"
        << "# TYPE themisdb_replication_lag_per_replica gauge\n";
    
    std::shared_lock<std::shared_mutex> lock(replicas_mutex_);
    for (const auto& replica : replicas_) {
        oss << "themisdb_replication_lag_per_replica{node_id=\"" << replica.node_id
            << "\",datacenter=\"" << replica.datacenter << "\"} "
            << replica.replicationLagMs() << "\n";
    }
    
    return oss.str();
}

void ReplicationManager::heartbeatLoop() {
    while (running_.load()) {
        if (election_ && election_->isLeader()) {
            // Send heartbeats to all followers
            std::shared_lock<std::shared_mutex> lock(replicas_mutex_);
            for (const auto& replica : replicas_) {
                (void)replica;  // In real implementation, send AppendEntries RPC
            }
        }
        
        std::this_thread::sleep_for(
            std::chrono::milliseconds(config_.heartbeat_interval_ms)
        );
    }
}

void ReplicationManager::compactionLoop() {
    while (running_.load()) {
        // Find minimum acked sequence across all replicas
        uint64_t min_seq = wal_->getCurrentSequence();
        {
            std::shared_lock<std::shared_mutex> lock(replicas_mutex_);
            for (const auto& stream : streams_) {
                min_seq = std::min(min_seq, stream->getLastAckedSequence());
            }
        }
        
        // Truncate WAL up to min_seq (keep some buffer)
        if (min_seq > 10000) {
            wal_->truncateBefore(min_seq - 10000);
        }
        
        // Run every 5 minutes
        for (int i = 0; i < 300 && running_.load(); ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

void ReplicationManager::notifyListeners(
    std::function<void(IReplicationListener&)> callback) {
    for (auto& listener : listeners_) {
        if (listener) {
            callback(*listener);
        }
    }
}

std::vector<std::pair<std::string, HealthStatus>> ReplicationManager::getReplicaHealthStatus() const {
    std::shared_lock<std::shared_mutex> lock(replicas_mutex_);
    std::vector<std::pair<std::string, HealthStatus>> result;
    
    for (const auto& replica : replicas_) {
        result.emplace_back(replica.node_id, replica.health_status);
    }
    
    return result;
}

bool ReplicationManager::hasQuorum() const {
    std::shared_lock<std::shared_mutex> lock(replicas_mutex_);
    
    size_t healthy_voting_members = 0;
    size_t total_voting_members = 0;
    
    for (const auto& replica : replicas_) {
        if (replica.is_voting_member) {
            total_voting_members++;
            if (replica.health_status == HealthStatus::HEALTHY || 
                replica.health_status == HealthStatus::DEGRADED) {
                healthy_voting_members++;
            }
        }
    }
    
    // Add self if leader
    if (election_ && election_->isLeader()) {
        total_voting_members++;
        healthy_voting_members++;
    }
    
    // Quorum is majority of voting members
    return healthy_voting_members > (total_voting_members / 2);
}

void ReplicationManager::performHealthCheck() {
    // Collect all health status changes under the write lock, then
    // notify listeners outside the lock to avoid holding it during callbacks.
    struct HealthChange {
        std::string node_id;
        HealthStatus old_status;
        HealthStatus new_status;
    };
    std::vector<HealthChange> changes;
    
    {
        std::unique_lock<std::shared_mutex> lock(replicas_mutex_);
        for (auto& replica : replicas_) {
            HealthStatus old_status = replica.health_status;
            updateReplicaHealth(replica);
            if (old_status != replica.health_status) {
                changes.push_back({replica.node_id, old_status, replica.health_status});
            }
        }
    }
    
    for (const auto& change : changes) {
        notifyListeners([&change](IReplicationListener& l) {
            l.onReplicaHealthChanged(change.node_id, change.old_status, change.new_status);
        });
    }
}

bool ReplicationManager::detectNetworkPartition() const {
    std::shared_lock<std::shared_mutex> lock(replicas_mutex_);
    
    // Count failed replicas
    size_t failed_count = 0;
    for (const auto& replica : replicas_) {
        if (replica.health_status == HealthStatus::FAILED) {
            failed_count++;
        }
    }
    
    // Network partition detected if more than half of replicas are unreachable
    return failed_count > (replicas_.size() / 2);
}

void ReplicationManager::setReadPreference(ReadPreference preference) {
    config_.default_read_preference = preference;
}

void ReplicationManager::healthMonitorLoop() {
    while (running_.load()) {
        performHealthCheck();
        
        // Check for leader failure and trigger automatic failover if enabled
        if (config_.enable_auto_failover && election_ && !election_->isLeader()) {
            std::string current_leader_id = election_->getLeaderId();
            std::string failed_leader_id;
            
            {
                std::shared_lock<std::shared_mutex> lock(replicas_mutex_);
                for (const auto& replica : replicas_) {
                    if (replica.node_id == current_leader_id && 
                        replica.health_status == HealthStatus::FAILED) {
                        failed_leader_id = replica.node_id;
                        break;
                    }
                }
            }
            
            if (!failed_leader_id.empty()) {
                stats_.replica_failures_detected++;
                attemptAutomaticFailover(failed_leader_id);
            }
        }
        
        // Check for network partition
        if (detectNetworkPartition()) {
            stats_.network_partitions_detected++;
            
            std::vector<std::string> unreachable_nodes;
            {
                std::shared_lock<std::shared_mutex> lock(replicas_mutex_);
                for (const auto& replica : replicas_) {
                    if (replica.health_status == HealthStatus::FAILED) {
                        unreachable_nodes.push_back(replica.node_id);
                    }
                }
            }
            
            notifyListeners([&unreachable_nodes](IReplicationListener& l) {
                l.onNetworkPartitionDetected(unreachable_nodes);
            });
        }
        
        // Update replication lag metrics
        {
            std::shared_lock<std::shared_mutex> lock(replicas_mutex_);
            int64_t max_lag = 0;
            int64_t total_lag = 0;
            size_t replica_count = 0;
            
            for (const auto& replica : replicas_) {
                int64_t lag = replica.replicationLagMs();
                max_lag = std::max(max_lag, lag);
                total_lag += lag;
                replica_count++;
                
                // Notify listeners of excessive lag
                if (lag > static_cast<int64_t>(config_.max_replication_lag_ms)) {
                    notifyListeners([lag](IReplicationListener& l) {
                        l.onReplicationLagWarning(lag);
                    });
                }
            }
            
            stats_.max_replication_lag_ms.store(max_lag);
            if (replica_count > 0) {
                stats_.avg_replication_lag_ms.store(total_lag / replica_count);
            }
        }
        
        std::this_thread::sleep_for(
            std::chrono::milliseconds(config_.heartbeat_interval_ms)
        );
    }
}

void ReplicationManager::attemptAutomaticFailover(const std::string& failed_node_id) {
    // Check if we have quorum to proceed with failover
    if (!hasQuorum()) {
        return;
    }
    
    // Elect new leader
    bool success = electNewLeader();
    
    if (success) {
        stats_.automatic_failovers++;
        
        std::string new_leader_id = election_ ? election_->getLeaderId() : "";
        
        notifyListeners([&failed_node_id, &new_leader_id](IReplicationListener& l) {
            l.onFailoverStarted(failed_node_id, new_leader_id);
        });
        
        notifyListeners([&new_leader_id](IReplicationListener& l) {
            l.onFailoverCompleted(new_leader_id, true);
        });
    } else {
        notifyListeners([](IReplicationListener& l) {
            l.onFailoverCompleted("", false);
        });
    }
}

bool ReplicationManager::electNewLeader() {
    if (!election_) {
        return false;
    }
    
    // Find the replica with highest priority and most up-to-date log
    std::shared_lock<std::shared_mutex> lock(replicas_mutex_);
    
    ReplicaInfo* best_candidate = nullptr;
    for (auto& replica : replicas_) {
        if (!replica.is_voting_member || 
            replica.health_status == HealthStatus::FAILED) {
            continue;
        }
        
        if (!best_candidate || 
            replica.priority > best_candidate->priority ||
            (replica.priority == best_candidate->priority && 
             replica.last_applied_sequence > best_candidate->last_applied_sequence)) {
            best_candidate = &replica;
        }
    }
    
    // If this node is the best candidate, start election
    if (best_candidate && best_candidate->node_id == node_id_) {
        election_->startElection();
        return election_->isLeader();
    }
    
    return false;
}

void ReplicationManager::updateReplicaHealth(ReplicaInfo& replica) {
    replica.updateHealthStatus(
        config_.failure_detection_timeout_ms,
        config_.degraded_lag_threshold_ms
    );
}

bool ReplicationManager::validateConfig() {
    if (config_.batch_size == 0 || config_.batch_size > 1000000) {
        THEMIS_ERROR("batch_size must be 1-1000000, got {}", config_.batch_size);
        return false;
    }
    
    if (config_.heartbeat_interval_ms == 0 || config_.heartbeat_interval_ms > 60000) {
        THEMIS_ERROR("heartbeat_interval_ms must be 1-60000, got {}", config_.heartbeat_interval_ms);
        return false;
    }
    
    if (config_.election_timeout_min_ms >= config_.election_timeout_max_ms) {
        THEMIS_ERROR("election_timeout_min_ms ({}) must be less than election_timeout_max_ms ({})",
                     config_.election_timeout_min_ms, config_.election_timeout_max_ms);
        return false;
    }
    
    if (config_.failure_detection_timeout_ms == 0) {
        THEMIS_ERROR("failure_detection_timeout_ms must be > 0");
        return false;
    }
    
    if (config_.wal_directory.empty()) {
        THEMIS_ERROR("wal_directory must not be empty");
        return false;
    }
    
    if (config_.seed_nodes.empty()) {
        THEMIS_WARN("No seed nodes configured – clustering disabled (single-node mode)");
    }
    
    if (config_.mode != ReplicationMode::ASYNC && config_.min_sync_replicas == 0) {
        THEMIS_ERROR("min_sync_replicas must be > 0 for SYNC/SEMI_SYNC modes");
        return false;
    }
    
    return true;
}

// ============================================================================
// LWWConflictResolver Implementation
// ============================================================================

// Extract the "updated_at" field from a minimal JSON payload.
// We intentionally avoid a full JSON parser dependency; we just scan for the
// first occurrence of "updated_at":<number> pattern.
int64_t LWWConflictResolver::extractTimestamp(const std::string& json_doc) {
    const std::string key = "\"updated_at\"";
    auto pos = json_doc.find(key);
    if (pos == std::string::npos) {
        return -1;
    }
    // Skip past key, colon, and optional whitespace
    pos += key.size();
    while (pos < json_doc.size() && (json_doc[pos] == ' ' || json_doc[pos] == ':')) {
        ++pos;
    }
    if (pos >= json_doc.size()) {
        return -1;
    }
    // Parse the integer value
    try {
        size_t consumed = 0;
        int64_t ts = std::stoll(json_doc.substr(pos), &consumed);
        return (consumed > 0) ? ts : -1;
    } catch (...) {
        return -1;
    }
}

std::string LWWConflictResolver::resolve(
    const std::string& local,
    const std::string& remote,
    const std::string& /*collection*/,
    const std::string& /*document_id*/)
{
    int64_t local_ts  = extractTimestamp(local);
    int64_t remote_ts = extractTimestamp(remote);
    
    if (local_ts < 0 && remote_ts < 0) {
        // Neither has a timestamp – keep the remote version (conservative)
        return remote;
    }
    if (local_ts < 0)  { return remote; }
    if (remote_ts < 0) { return local;  }
    
    // Select the document with the strictly higher timestamp; ties go to remote
    return (local_ts > remote_ts) ? local : remote;
}

// ============================================================================
// CRDTConflictResolver Implementation
// ============================================================================

std::string CRDTConflictResolver::resolve(
    const std::string& local,
    const std::string& remote,
    const std::string& collection,
    const std::string& document_id)
{
    // For simple numeric fields that follow the pattern "\"<field>\":<number>"
    // we take the maximum value (grow-only counter / LWW-Max register).
    // For all other content we delegate to LWWConflictResolver.
    
    // If either document is empty, return the other
    if (local.empty())  { return remote; }
    if (remote.empty()) { return local;  }
    
    // Build merged document: walk the remote document and for numeric fields
    // keep max(local, remote); everything else comes from LWW winner.
    LWWConflictResolver lwr;
    std::string base = lwr.resolve(local, remote, collection, document_id);
    
    // Scan both documents for numeric fields and merge with max semantics.
    // We iterate over keys that appear in remote and check if they are numeric.
    std::string merged = base;
    
    // Simple heuristic: find all "key": number patterns in both documents and
    // replace with max value.
    auto extractNumericFields = [](const std::string& doc)
        -> std::map<std::string, int64_t>
    {
        std::map<std::string, int64_t> fields;
        size_t p = 0;
        while (p < doc.size()) {
            // Find next key (starts with '"')
            auto kstart = doc.find('"', p);
            if (kstart == std::string::npos) break;
            auto kend = doc.find('"', kstart + 1);
            if (kend == std::string::npos) break;
            std::string key = doc.substr(kstart + 1, kend - kstart - 1);
            
            // Skip past colon and whitespace
            size_t vp = kend + 1;
            while (vp < doc.size() && (doc[vp] == ' ' || doc[vp] == ':')) ++vp;
            
            // Check if value is numeric (starts with digit, or '-' followed by a digit)
            if (vp < doc.size() &&
                (std::isdigit(static_cast<unsigned char>(doc[vp])) ||
                 (doc[vp] == '-' && vp + 1 < doc.size() &&
                  std::isdigit(static_cast<unsigned char>(doc[vp + 1]))))) {
                try {
                    size_t consumed = 0;
                    int64_t val = std::stoll(doc.substr(vp), &consumed);
                    if (consumed > 0) {
                        fields[key] = val;
                    }
                } catch (...) {}
            }
            
            p = kend + 1;
        }
        return fields;
    };
    
    auto local_fields  = extractNumericFields(local);
    auto remote_fields = extractNumericFields(remote);
    
    // For each field present in both documents, patch the merged document with max value
    for (const auto& [key, remote_val] : remote_fields) {
        auto it = local_fields.find(key);
        if (it == local_fields.end()) continue;
        
        int64_t max_val = std::max(it->second, remote_val);
        int64_t cur_val = (base == local) ? it->second : remote_val;
        
        if (max_val != cur_val) {
            // Replace "key": cur_val with "key": max_val in merged
            std::string search = "\"" + key + "\"";
            auto pos = merged.find(search);
            if (pos != std::string::npos) {
                // Skip to value
                size_t vp = pos + search.size();
                while (vp < merged.size() && (merged[vp] == ' ' || merged[vp] == ':')) ++vp;
                size_t vend = vp;
                // Accept an optional leading '-', then only digits
                if (vend < merged.size() && merged[vend] == '-') ++vend;
                while (vend < merged.size() &&
                       std::isdigit(static_cast<unsigned char>(merged[vend]))) {
                    ++vend;
                }
                merged = merged.substr(0, vp) + std::to_string(max_val) + merged.substr(vend);
            }
        }
    }
    
    return merged;
}

// ============================================================================
// HybridLogicalClock::Timestamp Implementation
// ============================================================================

bool HybridLogicalClock::Timestamp::operator<(const Timestamp& other) const {
    if (physical != other.physical) {
        return physical < other.physical;
    }
    if (logical != other.logical) {
        return logical < other.logical;
    }
    return node_id < other.node_id;
}

bool HybridLogicalClock::Timestamp::operator==(const Timestamp& other) const {
    return physical == other.physical && 
           logical == other.logical && 
           node_id == other.node_id;
}

std::string HybridLogicalClock::Timestamp::toString() const {
    std::ostringstream oss;
    oss << "HLC(" << physical << "," << logical << "," << node_id << ")";
    return oss.str();
}

} // namespace replication
} // namespace themisdb
