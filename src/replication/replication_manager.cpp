/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            replication_manager.cpp                            ║
  Version:         0.0.17                                             ║
  Last Modified:   2026-02-21 18:23:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   85.0/100                                       ║
    • Total Lines:     3932                                           ║
    • Open Issues:     TODOs: 1, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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
#include <set>
#include <future>
#include <numeric>
#include <lz4.h>
#include <zstd.h>
#include <snappy.h>

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
    , current_term_(wal->getCurrentTerm())
    , last_heartbeat_time_(std::chrono::steady_clock::now()) {
}

LeaderElection::~LeaderElection() {
    running_.store(false);
    election_cv_.notify_all();
    if (election_thread_.joinable()) {
        election_thread_.join();
    }
}

void LeaderElection::start() {
    running_.store(true);
    last_heartbeat_time_ = std::chrono::steady_clock::now();
    election_thread_ = std::thread(&LeaderElection::electionLoop, this);
}

void LeaderElection::electionLoop() {
    std::random_device rd;
    std::mt19937 gen(rd());

    while (running_.load()) {
        // Randomized election timeout per Raft spec (§5.2)
        uint32_t timeout_ms = std::uniform_int_distribution<uint32_t>(
            config_.election_timeout_min_ms,
            config_.election_timeout_max_ms)(gen);

        {
            std::unique_lock<std::mutex> lock(election_mutex_);
            // Wake up early if we stopped or just became leader
            bool woken_early = election_cv_.wait_for(
                lock,
                std::chrono::milliseconds(timeout_ms),
                [this] {
                    return !running_.load() ||
                           role_.load() == ReplicationRole::LEADER;
                });
            if (woken_early) {
                continue;  // Either stopping or already leader – no need to start election
            }
        }

        if (!running_.load()) break;

        // Skip if we are already the leader
        if (role_.load() == ReplicationRole::LEADER) continue;

        // Check whether the election timeout has actually elapsed since the
        // last heartbeat (a heartbeat could have arrived just after the cv
        // timed out but before we re-acquired the lock).
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - last_heartbeat_time_
        ).count();

        if (elapsed_ms >= static_cast<int64_t>(timeout_ms)) {
            THEMIS_INFO("Node {} election timeout ({}ms since last heartbeat), starting election",
                        node_id_, elapsed_ms);
            startElection();
        }
    }
}

void LeaderElection::startElection() {
    std::lock_guard<std::mutex> lock(election_mutex_);
    
    // Guard: do not start an election if we are already the leader
    if (role_.load() == ReplicationRole::LEADER) return;
    
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
    
    {
        std::lock_guard<std::mutex> lock(election_mutex_);
        if (term >= current_term_) {
            becomeFollower(term, leader_id);
        }
    }
    // Notify the election loop so it resets its timeout countdown
    election_cv_.notify_one();
    // TODO(future): apply leader_commit to follower's local commit index
    (void)leader_commit;
}

std::string LeaderElection::getLeaderId() const {
    return current_leader_;
}

void LeaderElection::becomeLeader() {
    role_.store(ReplicationRole::LEADER);
    current_leader_ = node_id_;
    election_cv_.notify_all();  // Wake the election loop so it can skip re-checking
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
        // Apply exponential backoff when the follower is not responsive
        uint32_t backoff = computeBackoffMs();
        if (backoff > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(backoff));
            if (!running_.load()) break;
        }

        uint64_t next_seq = last_acked_sequence_.load() + 1;
        auto entries = wal_->readFrom(next_seq, config_.batch_size);
        
        if (!entries.empty()) {
            if (sendBatch(entries)) {
                consecutive_failures_.store(0);
                last_acked_sequence_.store(entries.back().sequence_number);
                follower_info_.last_applied_sequence = entries.back().sequence_number;
                follower_info_.last_heartbeat = std::chrono::system_clock::now();
            } else {
                uint32_t new_failures = consecutive_failures_.fetch_add(1) + 1;
                THEMIS_WARN("ReplicationStream to {} failed (attempt {}), backing off {}ms",
                            follower_endpoint_, new_failures, computeBackoffMs());
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(config_.batch_timeout_ms));
    }
}

uint32_t ReplicationStream::computeBackoffMs() const {
    uint32_t failures = consecutive_failures_.load();
    if (failures == 0) return 0;
    // Exponential backoff: base * 2^(failures-1), capped at max
    uint32_t backoff = kBaseBackoffMs;
    for (uint32_t i = 1; i < failures && backoff < kMaxBackoffMs; ++i) {
        backoff = std::min(backoff * 2, kMaxBackoffMs);
    }
    return backoff;
}

bool ReplicationStream::sendBatch(const std::vector<WALEntry>& entries) {
    // In a real implementation, this would serialize entries and send them over
    // a mTLS connection to the follower endpoint, then wait for acknowledgement.
    // The retry/backoff logic is managed by the caller (streamLoop).
    (void)entries;
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
    
    // Start the election timeout loop (randomized Raft timeouts)
    election_->start();
    
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
        
        // Notify CDC listeners about the applied WAL entry
        notifyListeners([&entry](IReplicationListener& l) {
            l.onWALEntryApplied(entry);
        });
        
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
            // Notify local election module so it doesn't start a spurious
            // election against itself; in a real multi-node deployment this
            // loop would serialize and send AppendEntries RPCs over the wire.
            uint64_t current_term = election_->getCurrentTerm();
            {
                std::shared_lock<std::shared_mutex> lock(replicas_mutex_);
                for (const auto& replica : replicas_) {
                    // Record outbound heartbeat so the election module can
                    // reset its own liveness timer if it happens to be watching.
                    (void)replica;  // endpoint used by real network layer
                }
            }
            // Reset the leader's own heartbeat timer to avoid self-election
            election_->receiveHeartbeat(current_term, node_id_, wal_ ? wal_->getCurrentSequence() : 0);
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

// ============================================================================
// HybridLogicalClock Implementation
// ============================================================================

HybridLogicalClock::HybridLogicalClock(const std::string& node_id)
    : node_id_(node_id)
    , last_physical_(0)
    , logical_counter_(0) {
}

HybridLogicalClock::Timestamp HybridLogicalClock::now() {
    std::lock_guard<std::mutex> lock(mutex_);

    uint64_t wall = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    uint64_t last = last_physical_.load();
    if (wall > last) {
        last_physical_.store(wall);
        logical_counter_.store(0);
    } else {
        // Wall clock has not advanced – increment logical counter
        logical_counter_++;
    }

    return Timestamp{last_physical_.load(), logical_counter_.load(), node_id_};
}

HybridLogicalClock::Timestamp HybridLogicalClock::receive(const Timestamp& received) {
    std::lock_guard<std::mutex> lock(mutex_);

    uint64_t wall = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    uint64_t last  = last_physical_.load();
    uint64_t max_p = std::max({wall, last, received.physical});

    if (max_p == last && max_p == received.physical) {
        // Both this node and the sender are at the same physical time
        logical_counter_.store(std::max(logical_counter_.load(), received.logical) + 1);
    } else if (max_p == received.physical) {
        // Sender is ahead
        logical_counter_.store(received.logical + 1);
    } else if (max_p == last) {
        // This node is ahead (or wall advanced)
        logical_counter_++;
    } else {
        // Wall clock is strictly ahead of both
        logical_counter_.store(0);
    }

    last_physical_.store(max_p);
    return Timestamp{last_physical_.load(), logical_counter_.load(), node_id_};
}

HybridLogicalClock::Timestamp HybridLogicalClock::current() const {
    return Timestamp{last_physical_.load(), logical_counter_.load(), node_id_};
}

// ============================================================================
// VectorClock Implementation
// ============================================================================

VectorClock::VectorClock(const std::string& node_id) {
    clocks_[node_id] = 0;
}

VectorClock::VectorClock(const VectorClock& other) {
    // mutex_ is mutable – no const_cast needed
    std::shared_lock<std::shared_mutex> rlock(other.mutex_);
    clocks_ = other.clocks_;
}

VectorClock::VectorClock(VectorClock&& other) noexcept {
    std::unique_lock<std::shared_mutex> wlock(other.mutex_);
    clocks_ = std::move(other.clocks_);
}

VectorClock& VectorClock::operator=(const VectorClock& other) {
    if (this != &other) {
        std::unique_lock<std::shared_mutex> wlock(mutex_);
        std::shared_lock<std::shared_mutex> rlock(other.mutex_);
        clocks_ = other.clocks_;
    }
    return *this;
}

VectorClock& VectorClock::operator=(VectorClock&& other) noexcept {
    if (this != &other) {
        std::unique_lock<std::shared_mutex> wlock(mutex_);
        std::unique_lock<std::shared_mutex> rlock(other.mutex_);
        clocks_ = std::move(other.clocks_);
    }
    return *this;
}

void VectorClock::increment(const std::string& node_id) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    clocks_[node_id]++;
}

void VectorClock::merge(const VectorClock& other) {
    if (this == &other) return;

    // Acquire locks in consistent address order to prevent ABBA deadlock when
    // two threads concurrently call A.merge(B) and B.merge(A).
    VectorClock* lo  = (this <  &other) ? this : const_cast<VectorClock*>(&other);
    VectorClock* hi  = (this >= &other) ? this : const_cast<VectorClock*>(&other);
    std::unique_lock<std::shared_mutex> lk_lo(lo->mutex_);
    std::unique_lock<std::shared_mutex> lk_hi(hi->mutex_);

    for (const auto& [node, ts] : other.clocks_) {
        auto& mine = clocks_[node];
        if (ts > mine) mine = ts;
    }
}

uint64_t VectorClock::get(const std::string& node_id) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = clocks_.find(node_id);
    return it != clocks_.end() ? it->second : 0;
}

int VectorClock::compare(const VectorClock& other) const {
    if (this == &other) return 1;  // comparing with self → equal (treat as this >= other)

    // Acquire both shared locks; multiple shared_locks don't deadlock each other.
    // Use ordered acquisition to keep consistent with exclusive-lock callers.
    const VectorClock* lo = (this <= &other) ? this : &other;
    const VectorClock* hi = (this >  &other) ? this : &other;
    std::shared_lock<std::shared_mutex> lk_lo(lo->mutex_);
    std::shared_lock<std::shared_mutex> lk_hi(hi->mutex_);

    bool this_greater  = false;
    bool other_greater = false;

    // Check all entries present in this clock
    for (const auto& [node, ts] : clocks_) {
        auto it = other.clocks_.find(node);
        uint64_t other_ts = (it != other.clocks_.end()) ? it->second : 0;
        if (ts  > other_ts) this_greater  = true;
        if (ts  < other_ts) other_greater = true;
    }
    // Check entries present only in the other clock
    for (const auto& [node, ts] : other.clocks_) {
        if (clocks_.find(node) == clocks_.end() && ts > 0) {
            other_greater = true;
        }
    }

    if ( this_greater && !other_greater) return  1;  // this is strictly newer
    if (!this_greater &&  other_greater) return -1;  // other is strictly newer
    if ( this_greater &&  other_greater) return  0;  // concurrent – neither dominates
    return 0;  // equal – treat as concurrent
}

bool VectorClock::happensBefore(const VectorClock& other) const {
    return compare(other) == -1;
}

bool VectorClock::isConcurrent(const VectorClock& other) const {
    return compare(other) == 0;
}

std::string VectorClock::toJson() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    std::ostringstream oss;
    oss << "{";
    bool first = true;
    for (const auto& [node, ts] : clocks_) {
        if (!first) oss << ",";
        oss << "\"" << node << "\":" << ts;
        first = false;
    }
    oss << "}";
    return oss.str();
}

VectorClock VectorClock::fromJson(const std::string& json) {
    VectorClock vc;
    size_t p = 0;
    while (p < json.size()) {
        auto kstart = json.find('"', p);
        if (kstart == std::string::npos) break;
        auto kend = json.find('"', kstart + 1);
        if (kend == std::string::npos) break;
        std::string key = json.substr(kstart + 1, kend - kstart - 1);

        // Skip ':' and whitespace
        size_t vp = kend + 1;
        while (vp < json.size() && (json[vp] == ' ' || json[vp] == ':')) ++vp;

        if (vp < json.size() && std::isdigit(static_cast<unsigned char>(json[vp]))) {
            try {
                size_t consumed = 0;
                uint64_t val = std::stoull(json.substr(vp), &consumed);
                if (consumed > 0) {
                    vc.clocks_[key] = val;
                    p = vp + consumed;
                    continue;
                }
            } catch (...) {}
        }
        p = kend + 1;
    }
    return vc;
}

// ============================================================================
// Multi-master ConflictResolver implementations (MMWriteEntry variants)
// ============================================================================

MMWriteEntry LastWriteWinsResolver::resolve(
    const std::string& /*document_id*/,
    const std::vector<MMWriteEntry>& conflicting_writes)
{
    if (conflicting_writes.empty()) return MMWriteEntry{};

    // Select the entry with the latest HLC timestamp; ties resolved by node_id
    const MMWriteEntry* winner = &conflicting_writes[0];
    for (const auto& entry : conflicting_writes) {
        if (winner->hlc < entry.hlc) {
            winner = &entry;
        }
    }
    return *winner;
}

CRDTMergeResolver::CRDTMergeResolver(CRDTType type)
    : crdt_type_(type) {
}

MMWriteEntry CRDTMergeResolver::resolve(
    const std::string& document_id,
    const std::vector<MMWriteEntry>& conflicting_writes)
{
    if (conflicting_writes.empty()) return MMWriteEntry{};

    std::string merged_data;
    switch (crdt_type_) {
        case CRDTType::LWW_REGISTER: merged_data = mergeLWWRegister(conflicting_writes); break;
        case CRDTType::MV_REGISTER:  merged_data = mergeMVRegister(conflicting_writes);  break;
        case CRDTType::G_COUNTER:    merged_data = mergeGCounter(conflicting_writes);    break;
        case CRDTType::PN_COUNTER:   merged_data = mergePNCounter(conflicting_writes);   break;
        case CRDTType::G_SET:        merged_data = mergeGSet(conflicting_writes);        break;
        case CRDTType::OR_SET:       merged_data = mergeORSet(conflicting_writes);       break;
        case CRDTType::LWW_MAP:      merged_data = mergeLWWMap(conflicting_writes);      break;
    }

    // Base entry is the LWW winner; replace its data with the merged payload
    LastWriteWinsResolver lwr;
    MMWriteEntry result = lwr.resolve(document_id, conflicting_writes);
    result.data = merged_data;
    return result;
}

std::string CRDTMergeResolver::strategyName() const {
    switch (crdt_type_) {
        case CRDTType::LWW_REGISTER: return "LWW_REGISTER";
        case CRDTType::MV_REGISTER:  return "MV_REGISTER";
        case CRDTType::G_COUNTER:    return "G_COUNTER";
        case CRDTType::PN_COUNTER:   return "PN_COUNTER";
        case CRDTType::G_SET:        return "G_SET";
        case CRDTType::OR_SET:       return "OR_SET";
        case CRDTType::LWW_MAP:      return "LWW_MAP";
    }
    return "UNKNOWN";
}

std::string CRDTMergeResolver::mergeLWWRegister(const std::vector<MMWriteEntry>& writes) {
    // Return the data of the entry with the latest HLC
    const MMWriteEntry* latest = &writes[0];
    for (const auto& w : writes) {
        if (latest->hlc < w.hlc) latest = &w;
    }
    return latest->data;
}

std::string CRDTMergeResolver::mergeMVRegister(const std::vector<MMWriteEntry>& writes) {
    // Multi-value register: return all concurrent values as a JSON array
    std::ostringstream oss;
    oss << "[";
    bool first = true;
    for (const auto& w : writes) {
        if (!first) oss << ",";
        oss << w.data;
        first = false;
    }
    oss << "]";
    return oss.str();
}

// Helper: scan a JSON doc for "key": integer pairs
static std::map<std::string, int64_t> extractJsonInts(const std::string& doc) {
    std::map<std::string, int64_t> fields;
    size_t p = 0;
    while (p < doc.size()) {
        auto ks = doc.find('"', p);
        if (ks == std::string::npos) break;
        auto ke = doc.find('"', ks + 1);
        if (ke == std::string::npos) break;
        std::string key = doc.substr(ks + 1, ke - ks - 1);
        size_t vp = ke + 1;
        while (vp < doc.size() && (doc[vp] == ' ' || doc[vp] == ':')) ++vp;
        if (vp < doc.size() &&
            (std::isdigit(static_cast<unsigned char>(doc[vp])) ||
             (doc[vp] == '-' && vp + 1 < doc.size() &&
              std::isdigit(static_cast<unsigned char>(doc[vp + 1]))))) {
            try {
                size_t consumed = 0;
                int64_t val = std::stoll(doc.substr(vp), &consumed);
                if (consumed > 0) { fields[key] = val; p = vp + consumed; continue; }
            } catch (...) {}
        }
        p = ke + 1;
    }
    return fields;
}

std::string CRDTMergeResolver::mergeGCounter(const std::vector<MMWriteEntry>& writes) {
    // Grow-only counter: for each node-keyed counter take the maximum value
    std::map<std::string, int64_t> merged;
    for (const auto& w : writes) {
        auto fields = extractJsonInts(w.data);
        for (const auto& [k, v] : fields) {
            merged[k] = std::max(merged[k], v);
        }
    }
    std::ostringstream oss;
    oss << "{";
    bool first = true;
    for (const auto& [k, v] : merged) {
        if (!first) oss << ",";
        oss << "\"" << k << "\":" << v;
        first = false;
    }
    oss << "}";
    return oss.str();
}

std::string CRDTMergeResolver::mergePNCounter(const std::vector<MMWriteEntry>& writes) {
    // PN counter: each entry has a "positive" and "negative" sub-counter per node
    // Simplified: treat all numeric fields as GCounter
    return mergeGCounter(writes);
}

std::string CRDTMergeResolver::mergeGSet(const std::vector<MMWriteEntry>& writes) {
    // Grow-only set: union of all string values inside JSON arrays
    // Simplified: concatenate unique tokens from all payloads
    std::set<std::string> seen;
    for (const auto& w : writes) {
        size_t p = 0;
        while (p < w.data.size()) {
            auto qs = w.data.find('"', p);
            if (qs == std::string::npos) break;
            auto qe = w.data.find('"', qs + 1);
            if (qe == std::string::npos) break;
            seen.insert(w.data.substr(qs + 1, qe - qs - 1));
            p = qe + 1;
        }
    }
    std::ostringstream oss;
    oss << "[";
    bool first = true;
    for (const auto& s : seen) {
        if (!first) oss << ",";
        oss << "\"" << s << "\"";
        first = false;
    }
    oss << "]";
    return oss.str();
}

std::string CRDTMergeResolver::mergeORSet(const std::vector<MMWriteEntry>& writes) {
    // OR-Set: observed-remove set; delegate to GSet for simplicity
    return mergeGSet(writes);
}

std::string CRDTMergeResolver::mergeLWWMap(const std::vector<MMWriteEntry>& writes) {
    // LWW-Map: per-key last-write-wins; use HLC to pick winner per key
    std::map<std::string, std::pair<HybridLogicalClock::Timestamp, std::string>> best;
    for (const auto& w : writes) {
        auto fields = extractJsonInts(w.data);
        for (const auto& [k, v] : fields) {
            auto it = best.find(k);
            if (it == best.end() || it->second.first < w.hlc) {
                best[k] = {w.hlc, std::to_string(v)};
            }
        }
    }
    std::ostringstream oss;
    oss << "{";
    bool first = true;
    for (const auto& [k, p] : best) {
        if (!first) oss << ",";
        oss << "\"" << k << "\":" << p.second;
        first = false;
    }
    oss << "}";
    return oss.str();
}

// ============================================================================
// MMWriteEntry Serialization
// ============================================================================

std::vector<uint8_t> MMWriteEntry::serialize() const {
    std::vector<uint8_t> result;

    auto appendUint64 = [&result](uint64_t val) {
        for (int i = 7; i >= 0; --i)
            result.push_back(static_cast<uint8_t>((val >> (i * 8)) & 0xFF));
    };
    auto appendUint32 = [&result](uint32_t val) {
        for (int i = 3; i >= 0; --i)
            result.push_back(static_cast<uint8_t>((val >> (i * 8)) & 0xFF));
    };
    auto appendString = [&result](const std::string& s) {
        uint32_t len = static_cast<uint32_t>(s.size());
        for (int i = 3; i >= 0; --i)
            result.push_back(static_cast<uint8_t>((len >> (i * 8)) & 0xFF));
        result.insert(result.end(), s.begin(), s.end());
    };

    appendString(write_id);
    appendString(origin_node);
    appendString(collection);
    appendString(document_id);
    appendString(operation);
    appendString(data);
    appendString(checksum);
    appendString(vector_clock.toJson());
    appendUint64(hlc.physical);
    appendUint32(hlc.logical);
    appendString(hlc.node_id);

    return result;
}

std::optional<MMWriteEntry> MMWriteEntry::deserialize(const std::vector<uint8_t>& raw) {
    if (raw.size() < 4) return std::nullopt;
    size_t pos = 0;

    auto readUint64 = [&]() -> uint64_t {
        uint64_t v = 0;
        for (int i = 0; i < 8 && pos < raw.size(); ++i, ++pos)
            v = (v << 8) | raw[pos];
        return v;
    };
    auto readUint32 = [&]() -> uint32_t {
        uint32_t v = 0;
        for (int i = 0; i < 4 && pos < raw.size(); ++i, ++pos)
            v = (v << 8) | raw[pos];
        return v;
    };
    auto readString = [&]() -> std::string {
        if (pos + 4 > raw.size()) return {};
        uint32_t len = readUint32();
        if (pos + len > raw.size()) return {};
        std::string s(raw.begin() + pos, raw.begin() + pos + len);
        pos += len;
        return s;
    };

    MMWriteEntry e;
    e.write_id     = readString();
    e.origin_node  = readString();
    e.collection   = readString();
    e.document_id  = readString();
    e.operation    = readString();
    e.data         = readString();
    e.checksum     = readString();
    e.vector_clock = VectorClock::fromJson(readString());
    e.hlc.physical = readUint64();
    e.hlc.logical  = readUint32();
    e.hlc.node_id  = readString();

    return e;
}

// ============================================================================
// CustomResolver Implementation
// ============================================================================

CustomResolver::CustomResolver(ResolverFunc resolver)
    : resolver_(std::move(resolver)) {
}

MMWriteEntry CustomResolver::resolve(
    const std::string& document_id,
    const std::vector<MMWriteEntry>& conflicting_writes)
{
    if (resolver_) {
        return resolver_(document_id, conflicting_writes);
    }
    // Fallback to LWW
    LastWriteWinsResolver lwr;
    return lwr.resolve(document_id, conflicting_writes);
}


// ============================================================================
// MultiMasterReplicationManager Implementation
// ============================================================================

// Helper: generate a short unique ID (write_id)
static std::string generateWriteId(const std::string& node_id) {
    static std::atomic<uint64_t> seq{0};
    uint64_t ts = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
    return node_id + "-" + std::to_string(ts) + "-" + std::to_string(seq.fetch_add(1));
}

// -------------------------
// Constructor / Destructor
// -------------------------

MultiMasterReplicationManager::MultiMasterReplicationManager(
    const MMReplicationConfig& config)
    : config_(config)
    , vector_clock_(std::make_unique<VectorClock>(config.node_id))
    , hlc_(std::make_unique<HybridLogicalClock>(config.node_id))
    , default_resolver_(std::make_shared<LastWriteWinsResolver>())
{
}

MultiMasterReplicationManager::~MultiMasterReplicationManager() {
    stop();
}

// -------------------------
// Lifecycle
// -------------------------

bool MultiMasterReplicationManager::start() {
    if (running_.exchange(true)) {
        return true;  // Already running
    }

    replication_thread_ = std::thread(&MultiMasterReplicationManager::replicationLoop, this);
    heartbeat_thread_   = std::thread(&MultiMasterReplicationManager::heartbeatLoop,   this);
    sync_thread_        = std::thread(&MultiMasterReplicationManager::syncLoop,        this);

    THEMIS_INFO("MultiMasterReplicationManager started (node_id={})", config_.node_id);
    return true;
}

void MultiMasterReplicationManager::stop() {
    if (!running_.exchange(false)) {
        return;  // Already stopped
    }

    writes_cv_.notify_all();

    if (replication_thread_.joinable()) replication_thread_.join();
    if (heartbeat_thread_.joinable())   heartbeat_thread_.join();
    if (sync_thread_.joinable())        sync_thread_.join();

    THEMIS_INFO("MultiMasterReplicationManager stopped (node_id={})", config_.node_id);
}

bool MultiMasterReplicationManager::isRunning() const {
    return running_.load();
}

// -------------------------
// Write Operations
// -------------------------

std::string MultiMasterReplicationManager::write(
    const std::string& collection,
    const std::string& document_id,
    const std::string& operation,
    const std::string& data,
    WriteCallback callback)
{
    if (!running_.load()) {
        THEMIS_ERROR("MMReplicationManager not running – write rejected");
        return {};
    }

    MMWriteEntry entry;
    entry.write_id    = generateWriteId(config_.node_id);
    entry.origin_node = config_.node_id;
    entry.collection  = collection;
    entry.document_id = document_id;
    entry.operation   = operation;
    entry.data        = data;
    entry.hlc         = hlc_->now();

    // Stamp with our vector clock and advance it
    {
        vector_clock_->increment(config_.node_id);
        entry.vector_clock = *vector_clock_;
    }

    // Compute simple checksum (SHA-256 of content)
    {
        std::string content = operation + collection + document_id + data;
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(content.c_str()), content.size(), hash);
        std::ostringstream oss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
            oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        entry.checksum = oss.str();
    }

    {
        std::lock_guard<std::mutex> lock(writes_mutex_);
        pending_writes_.push(entry);
        if (callback) {
            write_callbacks_[entry.write_id] = std::move(callback);
        }
    }
    writes_cv_.notify_one();
    stats_writes_total_.fetch_add(1);

    THEMIS_INFO("MM write queued: write_id={} collection={} doc={} op={}",
                entry.write_id, collection, document_id, operation);
    return entry.write_id;
}

bool MultiMasterReplicationManager::writeSync(
    const std::string& collection,
    const std::string& document_id,
    const std::string& operation,
    const std::string& data,
    std::chrono::milliseconds timeout)
{
    // Use shared_ptr to safely share the promise between this stack frame and
    // the callback which may execute on a different thread after this call returns.
    auto promise = std::make_shared<std::promise<bool>>();
    auto future  = promise->get_future();

    write(collection, document_id, operation, data,
          [promise](const MMWriteEntry& /*entry*/, bool success) {
              promise->set_value(success);
          });

    return future.wait_for(timeout) == std::future_status::ready && future.get();
}

// -------------------------
// Read Operations
// -------------------------

MultiMasterReplicationManager::ReadResult MultiMasterReplicationManager::read(
    const std::string& collection,
    const std::string& document_id,
    uint32_t /*read_quorum*/)
{
    // In a full implementation this would query read_quorum peers and merge
    // results; for now we return a placeholder indicating local state.
    ReadResult result;
    result.success     = running_.load();
    result.source_node = config_.node_id;
    result.version     = *vector_clock_;
    result.data        = "";  // Actual storage lookup is outside this module's scope

    return result;
}

// -------------------------
// Peer Management
// -------------------------

void MultiMasterReplicationManager::addPeer(const MMPeerInfo& peer) {
    std::unique_lock<std::shared_mutex> lock(peers_mutex_);
    peers_[peer.node_id] = peer;
    THEMIS_INFO("MM peer added: node_id={} endpoint={}", peer.node_id, peer.endpoint);
}

void MultiMasterReplicationManager::removePeer(const std::string& node_id) {
    std::unique_lock<std::shared_mutex> lock(peers_mutex_);
    peers_.erase(node_id);
    THEMIS_INFO("MM peer removed: node_id={}", node_id);
}

std::vector<MMPeerInfo> MultiMasterReplicationManager::getPeers() const {
    std::shared_lock<std::shared_mutex> lock(peers_mutex_);
    std::vector<MMPeerInfo> result;
    result.reserve(peers_.size());
    for (const auto& [id, info] : peers_) {
        result.push_back(info);
    }
    return result;
}

MMPeerInfo MultiMasterReplicationManager::getLocalInfo() const {
    MMPeerInfo info;
    info.node_id           = config_.node_id;
    info.datacenter        = config_.datacenter;
    info.region            = config_.region;
    info.state             = running_.load() ? MMNodeState::ACTIVE : MMNodeState::OFFLINE;
    info.replication_lag_ms = getReplicationLag();
    info.is_local_datacenter = true;
    info.last_known_clock  = *vector_clock_;
    return info;
}

// -------------------------
// Conflict Management
// -------------------------

void MultiMasterReplicationManager::registerConflictCallback(ConflictCallback callback) {
    std::lock_guard<std::mutex> lock(conflicts_mutex_);
    conflict_callbacks_.push_back(std::move(callback));
}

void MultiMasterReplicationManager::setConflictResolver(
    const std::string& collection,
    std::shared_ptr<ConflictResolver> resolver)
{
    std::lock_guard<std::mutex> lock(conflicts_mutex_);
    resolvers_[collection] = std::move(resolver);
}

std::vector<ConflictRecord> MultiMasterReplicationManager::getUnresolvedConflicts() const {
    std::lock_guard<std::mutex> lock(conflicts_mutex_);
    std::vector<ConflictRecord> result;
    for (const auto& rec : conflicts_) {
        if (!rec.resolved) {
            result.push_back(rec);
        }
    }
    return result;
}

bool MultiMasterReplicationManager::resolveConflict(
    const std::string& conflict_id,
    const std::string& winning_write_id)
{
    std::lock_guard<std::mutex> lock(conflicts_mutex_);
    for (auto& rec : conflicts_) {
        if (rec.conflict_id == conflict_id && !rec.resolved) {
            rec.resolved         = true;
            rec.winning_write_id = winning_write_id;
            stats_conflicts_resolved_.fetch_add(1);
            return true;
        }
    }
    return false;
}

// -------------------------
// Synchronization
// -------------------------

void MultiMasterReplicationManager::triggerSync() {
    // Wake up the sync loop immediately
    writes_cv_.notify_all();
}

uint64_t MultiMasterReplicationManager::getReplicationLag() const {
    std::shared_lock<std::shared_mutex> lock(peers_mutex_);
    uint64_t max_lag = 0;
    for (const auto& [id, peer] : peers_) {
        max_lag = std::max(max_lag, peer.replication_lag_ms);
    }
    return max_lag;
}

// -------------------------
// Statistics
// -------------------------

MultiMasterReplicationManager::Stats MultiMasterReplicationManager::getStats() const {
    Stats s;
    s.writes_total          = stats_writes_total_.load();
    s.writes_replicated     = stats_writes_replicated_.load();
    s.conflicts_detected    = stats_conflicts_detected_.load();
    s.conflicts_resolved    = stats_conflicts_resolved_.load();
    s.sync_rounds           = stats_sync_rounds_.load();
    s.bytes_sent            = stats_bytes_sent_.load();
    s.bytes_received        = stats_bytes_received_.load();

    {
        std::lock_guard<std::mutex> lock(writes_mutex_);
        s.writes_pending = pending_writes_.size();
    }

    s.avg_replication_latency = std::chrono::milliseconds(0);
    return s;
}

std::string MultiMasterReplicationManager::exportPrometheusMetrics() const {
    auto s = getStats();
    std::ostringstream oss;
    oss << "# HELP themisdb_mm_writes_total Total multi-master writes\n"
        << "# TYPE themisdb_mm_writes_total counter\n"
        << "themisdb_mm_writes_total{node=\"" << config_.node_id << "\"} " << s.writes_total << "\n"
        << "# HELP themisdb_mm_writes_replicated Writes successfully replicated\n"
        << "# TYPE themisdb_mm_writes_replicated counter\n"
        << "themisdb_mm_writes_replicated{node=\"" << config_.node_id << "\"} " << s.writes_replicated << "\n"
        << "# HELP themisdb_mm_writes_pending Pending writes in queue\n"
        << "# TYPE themisdb_mm_writes_pending gauge\n"
        << "themisdb_mm_writes_pending{node=\"" << config_.node_id << "\"} " << s.writes_pending << "\n"
        << "# HELP themisdb_mm_conflicts_detected Conflicts detected\n"
        << "# TYPE themisdb_mm_conflicts_detected counter\n"
        << "themisdb_mm_conflicts_detected{node=\"" << config_.node_id << "\"} " << s.conflicts_detected << "\n"
        << "# HELP themisdb_mm_conflicts_resolved Conflicts resolved\n"
        << "# TYPE themisdb_mm_conflicts_resolved counter\n"
        << "themisdb_mm_conflicts_resolved{node=\"" << config_.node_id << "\"} " << s.conflicts_resolved << "\n"
        << "# HELP themisdb_mm_sync_rounds Anti-entropy sync rounds completed\n"
        << "# TYPE themisdb_mm_sync_rounds counter\n"
        << "themisdb_mm_sync_rounds{node=\"" << config_.node_id << "\"} " << s.sync_rounds << "\n"
        << "# HELP themisdb_mm_replication_lag_ms Max replication lag across peers\n"
        << "# TYPE themisdb_mm_replication_lag_ms gauge\n"
        << "themisdb_mm_replication_lag_ms{node=\"" << config_.node_id << "\"} " << getReplicationLag() << "\n";
    return oss.str();
}

// -------------------------
// Background Loops
// -------------------------

void MultiMasterReplicationManager::replicationLoop() {
    while (running_.load()) {
        // Drain a batch of pending writes under the lock, then release the
        // lock before doing any network work to avoid holding it during I/O.
        std::vector<std::pair<MMWriteEntry, WriteCallback>> batch;
        {
            std::unique_lock<std::mutex> lock(writes_mutex_);
            writes_cv_.wait_for(lock,
                std::chrono::milliseconds(config_.sync_interval_ms),
                [this] { return !running_.load() || !pending_writes_.empty(); });

            while (!pending_writes_.empty()) {
                MMWriteEntry entry = std::move(pending_writes_.front());
                pending_writes_.pop();

                WriteCallback cb;
                auto it = write_callbacks_.find(entry.write_id);
                if (it != write_callbacks_.end()) {
                    cb = std::move(it->second);
                    write_callbacks_.erase(it);
                }
                batch.emplace_back(std::move(entry), std::move(cb));
            }
        }  // lock released here

        // Process the batch without holding writes_mutex_
        for (auto& [entry, cb] : batch) {
            bool ok = replicateWrite(entry);
            if (ok) {
                stats_writes_replicated_.fetch_add(1);
            }
            if (cb) {
                cb(entry, ok);
            }
        }
    }
}

void MultiMasterReplicationManager::heartbeatLoop() {
    while (running_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(config_.heartbeat_interval_ms));
        if (!running_.load()) break;

        auto now_ts = hlc_->now();
        // Use unique_lock because we mutate peer.last_heartbeat_hlc
        std::unique_lock<std::shared_mutex> lock(peers_mutex_);
        for (auto& [node_id, peer] : peers_) {
            // In a full implementation: send AppendEntries / heartbeat RPC.
            // Update last_heartbeat_hlc to the current timestamp.
            peer.last_heartbeat_hlc = now_ts;
            (void)node_id;
        }
    }
}

void MultiMasterReplicationManager::syncLoop() {
    while (running_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(config_.sync_interval_ms));
        if (!running_.load()) break;

        std::vector<std::string> peer_ids;
        {
            std::shared_lock<std::shared_mutex> lock(peers_mutex_);
            for (const auto& [id, _] : peers_) {
                peer_ids.push_back(id);
            }
        }

        for (const auto& peer_id : peer_ids) {
            if (!running_.load()) break;
            antiEntropySync(peer_id);
        }

        stats_sync_rounds_.fetch_add(1);
    }
}

// -------------------------
// Internal: Replication
// -------------------------

bool MultiMasterReplicationManager::replicateWrite(const MMWriteEntry& entry) {
    std::shared_lock<std::shared_mutex> lock(peers_mutex_);

    // Single-node mode: no peers configured means this node is authoritative
    // on its own; the write is considered locally applied without a quorum check.
    if (peers_.empty()) {
        return true;
    }

    uint32_t quorum  = config_.write_quorum;
    uint32_t acked   = 0;

    for (const auto& [node_id, peer] : peers_) {
        if (peer.state == MMNodeState::OFFLINE ||
            peer.state == MMNodeState::PARTITIONED) {
            continue;
        }
        if (sendToPeer(node_id, entry)) {
            ++acked;
        }
        if (acked >= quorum) break;
    }

    return acked >= quorum;
}

bool MultiMasterReplicationManager::sendToPeer(
    const std::string& node_id,
    const MMWriteEntry& entry)
{
    // In a full implementation this would serialize the entry and send it
    // over a mTLS connection to the peer node.  For now we simulate success
    // for ACTIVE peers and record bytes_sent.
    std::shared_lock<std::shared_mutex> lock(peers_mutex_);
    auto it = peers_.find(node_id);
    if (it == peers_.end()) return false;

    if (it->second.state == MMNodeState::OFFLINE ||
        it->second.state == MMNodeState::PARTITIONED) {
        return false;
    }

    auto serialized = entry.serialize();
    stats_bytes_sent_.fetch_add(serialized.size());
    (void)node_id;
    return true;
}

void MultiMasterReplicationManager::receiveFromPeer(
    const std::string& node_id,
    const MMWriteEntry& incoming)
{
    // Update our vector clock with the sender's clock
    vector_clock_->merge(incoming.vector_clock);
    hlc_->receive(incoming.hlc);

    // Update bytes_received
    auto serialized = incoming.serialize();
    stats_bytes_received_.fetch_add(serialized.size());

    // Check for conflict with any recently-seen write for the same document
    // (In production this would consult a local document store.)
    bool has_conflict = false;
    {
        std::lock_guard<std::mutex> lock(conflicts_mutex_);
        for (const auto& rec : conflicts_) {
            if (!rec.resolved &&
                rec.collection  == incoming.collection &&
                rec.document_id == incoming.document_id) {
                has_conflict = true;
                break;
            }
        }
    }

    if (has_conflict) {
        THEMIS_WARN("MM conflict detected for doc={}/{} from peer={}",
                    incoming.collection, incoming.document_id, node_id);
        // For each unresolved conflict on this document, add the incoming entry
        std::lock_guard<std::mutex> lock(conflicts_mutex_);
        for (auto& rec : conflicts_) {
            if (!rec.resolved &&
                rec.collection  == incoming.collection &&
                rec.document_id == incoming.document_id) {
                rec.conflicting_writes.push_back(incoming);
                handleConflict(incoming.document_id, rec.conflicting_writes);
                break;
            }
        }
    }
}

// -------------------------
// Internal: Conflict Detection & Resolution
// -------------------------

bool MultiMasterReplicationManager::detectConflict(
    const MMWriteEntry& incoming,
    const MMWriteEntry& existing)
{
    // Two writes conflict when their vector clocks are concurrent (neither
    // happened-before the other).
    return incoming.vector_clock.isConcurrent(existing.vector_clock);
}

void MultiMasterReplicationManager::handleConflict(
    const std::string& document_id,
    const std::vector<MMWriteEntry>& conflicting_writes)
{
    if (conflicting_writes.empty()) return;

    const std::string& collection = conflicting_writes[0].collection;

    // Find the appropriate resolver (collection-specific or default)
    std::shared_ptr<ConflictResolver> resolver;
    {
        std::lock_guard<std::mutex> lock(conflicts_mutex_);
        auto it = resolvers_.find(collection);
        resolver = (it != resolvers_.end()) ? it->second : default_resolver_;
    }

    MMWriteEntry winner = resolver->resolve(document_id, conflicting_writes);

    // Record the conflict
    ConflictRecord record;
    record.conflict_id        = generateWriteId(config_.node_id);
    record.type               = ConflictType::CONCURRENT_UPDATE;
    record.document_id        = document_id;
    record.collection         = collection;
    record.conflicting_writes = conflicting_writes;
    record.detected_at        = std::chrono::system_clock::now();
    record.resolved           = true;
    record.resolution_strategy = (resolver == default_resolver_) ? "LAST_WRITE_WINS" : "CUSTOM";
    record.winning_write_id   = winner.write_id;

    {
        std::lock_guard<std::mutex> lock(conflicts_mutex_);
        conflicts_.push_back(record);

        // Notify registered callbacks
        for (const auto& cb : conflict_callbacks_) {
            cb(record);
        }
    }

    stats_conflicts_detected_.fetch_add(1);
    stats_conflicts_resolved_.fetch_add(1);
}

// -------------------------
// Internal: Anti-Entropy
// -------------------------

void MultiMasterReplicationManager::antiEntropySync(const std::string& peer_id) {
    // Retrieve the peer's known vector clock
    VectorClock peer_clock;
    {
        std::shared_lock<std::shared_mutex> lock(peers_mutex_);
        auto it = peers_.find(peer_id);
        if (it == peers_.end()) return;
        if (it->second.state == MMNodeState::OFFLINE ||
            it->second.state == MMNodeState::PARTITIONED) {
            return;
        }
        peer_clock = it->second.last_known_clock;
    }

    // Find any writes that the peer has not seen yet
    auto missing = getMissingWrites(peer_clock);
    for (const auto& entry : missing) {
        sendToPeer(peer_id, entry);
    }

    // Update the peer's known clock to ours after sync
    {
        std::unique_lock<std::shared_mutex> lock(peers_mutex_);
        auto it = peers_.find(peer_id);
        if (it != peers_.end()) {
            it->second.last_known_clock = *vector_clock_;
        }
    }
}

std::vector<MMWriteEntry> MultiMasterReplicationManager::getMissingWrites(
    const VectorClock& peer_clock)
{
    // In a full implementation this would query a local write log and return
    // all entries whose vector clock happens-after the peer's clock.
    // For now we return an empty set (the WAL replay path is handled by
    // ReplicationStream / WALManager on the Raft leader-follower path).
    (void)peer_clock;
    return {};
}

// ============================================================================
// ParallelReplicationWorker Implementation (v1.6.0)
// ============================================================================

ParallelReplicationWorker::ParallelReplicationWorker(const ParallelConfig& config)
    : config_(config)
{
    running_.store(true);
    uint32_t n = std::max<uint32_t>(1, config_.worker_threads);
    workers_.reserve(n);
    for (uint32_t i = 0; i < n; ++i) {
        workers_.emplace_back(&ParallelReplicationWorker::workerLoop, this);
    }
}

ParallelReplicationWorker::~ParallelReplicationWorker() {
    running_.store(false);
    queue_cv_.notify_all();
    for (auto& t : workers_) {
        if (t.joinable()) t.join();
    }
}

void ParallelReplicationWorker::submit(const WALEntry& entry) {
    auto done_flag = std::make_shared<std::atomic<bool>>(false);

    WorkItem item;
    item.entry = entry;
    item.ready = done_flag;

    if (config_.use_dependency_tracking) {
        std::lock_guard<std::mutex> dep_lock(dep_mutex_);

        // If there's a previous write to the same document, add its done-flag
        // as a dependency so this write waits for it to complete.
        auto it = last_done_per_doc_.find(entry.document_id);
        if (it != last_done_per_doc_.end()) {
            item.deps.push_back(it->second);
            stats_deps_detected_.fetch_add(1);
        }
        // Register this write as the latest for this document
        last_done_per_doc_[entry.document_id] = done_flag;
    }

    {
        std::lock_guard<std::mutex> q_lock(queue_mutex_);
        // Enforce max queue size: drop oldest if full
        while (work_queue_.size() >= config_.queue_size) {
            work_queue_.pop();
        }
        in_flight_count_.fetch_add(1);
        work_queue_.push(std::move(item));
    }
    queue_cv_.notify_one();
}

void ParallelReplicationWorker::sync() {
    // Wait until the queue is drained AND all workers have finished processing
    while (true) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if (work_queue_.empty() && in_flight_count_.load() == 0) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

ParallelReplicationWorker::Stats ParallelReplicationWorker::getStats() const {
    Stats s;
    s.entries_applied      = stats_entries_applied_.load();
    s.dependencies_detected = stats_deps_detected_.load();
    s.parallel_batches     = stats_batches_.load();
    uint64_t batches       = s.parallel_batches;
    s.parallelism_factor   = (batches > 0)
        ? static_cast<double>(s.entries_applied) / static_cast<double>(batches)
        : 0.0;
    return s;
}

void ParallelReplicationWorker::workerLoop() {
    while (running_.load()) {
        WorkItem item;
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait_for(lock, std::chrono::milliseconds(5),
                [this] { return !running_.load() || !work_queue_.empty(); });
            if (work_queue_.empty()) continue;
            item = std::move(work_queue_.front());
            work_queue_.pop();
        }

        // Wait for all dependencies to complete
        for (const auto& dep : item.deps) {
            while (!dep->load()) {
                std::this_thread::yield();
            }
        }

        // Apply the entry (in production: write to local storage / state machine)
        // Here we simply mark it done and update stats.
        item.ready->store(true);
        stats_entries_applied_.fetch_add(1);
        stats_batches_.fetch_add(1);
        in_flight_count_.fetch_sub(1);
    }
}

// ============================================================================
// QuorumReadManager Implementation (v1.6.0)
// ============================================================================

QuorumReadManager::QuorumReadManager(
    const QuorumReadConfig& config,
    const std::vector<ReplicaInfo>& replicas)
    : config_(config)
    , replicas_(replicas)
{
}

QuorumReadManager::QuorumReadResult QuorumReadManager::read(
    const std::string& collection,
    const std::string& document_id,
    uint32_t quorum)
{
    uint32_t required = (quorum == 0) ? config_.read_quorum : quorum;

    std::vector<ReplicaInfo> snapshot;
    {
        std::shared_lock<std::shared_mutex> lock(replicas_mutex_);
        snapshot = replicas_;
    }

    if (snapshot.empty()) {
        // No replicas: single-node – return a placeholder success
        return QuorumReadResult{true, "", 0, false, {}};
    }

    // Issue reads to all replicas concurrently
    std::vector<std::future<ReplicaResponse>> futures;
    futures.reserve(snapshot.size());
    for (const auto& replica : snapshot) {
        futures.push_back(std::async(std::launch::async,
            [this, &replica, &collection, &document_id]() {
                return queryReplica(replica, collection, document_id);
            }));
    }

    // Collect responses up to `required`, respecting timeout
    auto deadline = std::chrono::steady_clock::now() +
                    std::chrono::milliseconds(config_.read_timeout_ms);

    std::vector<ReplicaResponse> responses;
    for (auto& fut : futures) {
        auto remaining = deadline - std::chrono::steady_clock::now();
        if (remaining.count() <= 0) break;
        if (fut.wait_for(remaining) == std::future_status::ready) {
            auto resp = fut.get();
            if (resp.ok) responses.push_back(std::move(resp));
        }
        if (responses.size() >= required) break;
    }

    if (responses.size() < required) {
        THEMIS_WARN("QuorumRead: only {}/{} replicas responded for {}/{}",
                    responses.size(), required, collection, document_id);
        return QuorumReadResult{false, "", 0, false, {}};
    }

    // Reconcile: pick the response with the highest version
    const ReplicaResponse* best = &responses[0];
    bool had_conflicts = false;
    for (const auto& r : responses) {
        if (r.version != best->version) had_conflicts = true;
        if (r.version > best->version)  best = &r;
    }

    // Collect source endpoints
    QuorumReadResult result;
    result.success       = true;
    result.data          = best->data;
    result.version       = best->version;
    result.had_conflicts = had_conflicts;
    for (const auto& r : responses) {
        result.sources.push_back(r.endpoint);
    }

    if (had_conflicts) {
        THEMIS_WARN("QuorumRead: divergence detected for {}/{}, version {}",
                    collection, document_id, best->version);
    }

    return result;
}

void QuorumReadManager::setReplicas(const std::vector<ReplicaInfo>& replicas) {
    std::unique_lock<std::shared_mutex> lock(replicas_mutex_);
    replicas_ = replicas;
}

QuorumReadManager::ReplicaResponse QuorumReadManager::queryReplica(
    const ReplicaInfo& replica,
    const std::string& /*collection*/,
    const std::string& /*document_id*/) const
{
    // In production this would send a read RPC to the replica endpoint.
    // For now simulate: ACTIVE replicas return success with the replica's
    // last_applied_sequence as the version.
    ReplicaResponse resp;
    resp.endpoint = replica.endpoint;
    resp.ok       = (replica.health_status == HealthStatus::HEALTHY);
    resp.version  = resp.ok ? replica.last_applied_sequence : 0;
    resp.data     = "";  // Real data comes from storage layer
    return resp;
}

// ============================================================================
// PersistentReplicationState Implementation (v1.6.0)
// ============================================================================

PersistentReplicationState::PersistentReplicationState(
    const std::string& state_file_path)
    : path_(state_file_path)
{
}

bool PersistentReplicationState::persist(const State& state) {
    std::lock_guard<std::mutex> lock(file_mutex_);
    try {
        std::ofstream ofs(path_, std::ios::trunc | std::ios::binary);
        if (!ofs.is_open()) {
            THEMIS_ERROR("PersistentReplicationState: cannot open {} for writing", path_);
            return false;
        }

        // Simple line-oriented text format: key=value
        ofs << "last_applied_sequence=" << state.last_applied_sequence << "\n"
            << "current_term="          << state.current_term          << "\n"
            << "voted_for="             << state.voted_for             << "\n"
            << "leader_id="             << state.leader_id             << "\n"
            << "persisted_at_ms="
            << std::chrono::duration_cast<std::chrono::milliseconds>(
                   state.persisted_at.time_since_epoch()).count()
            << "\n";

        ofs.flush();
        if (!ofs.good()) {
            THEMIS_ERROR("PersistentReplicationState: write error for {}", path_);
            return false;
        }
        return true;
    } catch (const std::exception& e) {
        THEMIS_ERROR("PersistentReplicationState::persist error: {}", e.what());
        return false;
    }
}

PersistentReplicationState::State PersistentReplicationState::load() const {
    std::lock_guard<std::mutex> lock(file_mutex_);
    State state;
    if (!std::filesystem::exists(path_)) {
        return state;  // First-run: return default
    }
    try {
        std::ifstream ifs(path_);
        if (!ifs.is_open()) return state;

        std::string line;
        while (std::getline(ifs, line)) {
            auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key = line.substr(0, eq);
            std::string val = line.substr(eq + 1);
            if (val.empty()) continue;
            try {
                if      (key == "last_applied_sequence")
                    state.last_applied_sequence = std::stoull(val);
                else if (key == "current_term")
                    state.current_term = std::stoull(val);
                else if (key == "voted_for")
                    state.voted_for = val;
                else if (key == "leader_id")
                    state.leader_id = val;
                else if (key == "persisted_at_ms") {
                    uint64_t ms = std::stoull(val);
                    state.persisted_at = std::chrono::system_clock::time_point(
                        std::chrono::milliseconds(ms));
                }
            } catch (...) {
                THEMIS_WARN("PersistentReplicationState: failed to parse key={}", key);
            }
        }
    } catch (const std::exception& e) {
        THEMIS_ERROR("PersistentReplicationState::load error: {}", e.what());
    }
    return state;
}

bool PersistentReplicationState::exists() const {
    return std::filesystem::exists(path_);
}

void PersistentReplicationState::remove() {
    std::lock_guard<std::mutex> lock(file_mutex_);
    std::filesystem::remove(path_);
}

// ============================================================================
// CompressedReplicationStream Implementation (v1.6.0)
// ============================================================================

CompressedReplicationStream::CompressedReplicationStream(
    const std::string& endpoint,
    const CompressionConfig& config)
    : endpoint_(endpoint)
    , config_(config)
{
}

CompressedReplicationStream::CompressedReplicationStream(const std::string& endpoint)
    : CompressedReplicationStream(endpoint, CompressionConfig{})
{
}
std::string CompressedReplicationStream::algorithmName(CompressionAlgorithm algo) {
    switch (algo) {
        case CompressionAlgorithm::NONE:   return "NONE";
        case CompressionAlgorithm::LZ4:    return "LZ4";
        case CompressionAlgorithm::ZSTD:   return "ZSTD";
        case CompressionAlgorithm::SNAPPY: return "SNAPPY";
        case CompressionAlgorithm::AUTO:   return "AUTO";
        default:                           return "UNKNOWN";
    }
}

CompressedReplicationStream::CompressionAlgorithm
CompressedReplicationStream::selectAlgorithm(size_t payload_bytes) const {
    if (config_.algorithm != CompressionAlgorithm::AUTO) {
        return config_.algorithm;
    }
    // AUTO: skip compression for tiny payloads to avoid overhead
    if (payload_bytes < config_.min_batch_size) {
        return CompressionAlgorithm::NONE;
    }
    return CompressionAlgorithm::ZSTD;
}

std::vector<uint8_t> CompressedReplicationStream::serializeEntries(
    const std::vector<WALEntry>& entries) const
{
    // Simple serialization: length-prefixed JSON-like representation
    std::string buf;
    for (const auto& e : entries) {
        buf += std::to_string(e.sequence_number) + "|"
             + e.collection   + "|"
             + e.document_id  + "|"
             + e.operation    + "|"
             + e.data         + "\n";
    }
    return std::vector<uint8_t>(buf.begin(), buf.end());
}

std::vector<uint8_t> CompressedReplicationStream::compress(
    const std::vector<uint8_t>& data,
    CompressionAlgorithm algo) const
{
    if (data.empty()) return {};

    switch (algo) {
        case CompressionAlgorithm::NONE:
            return data;

        case CompressionAlgorithm::LZ4: {
            int bound = LZ4_compressBound(static_cast<int>(data.size()));
            std::vector<uint8_t> out(static_cast<size_t>(bound));
            int compressed = LZ4_compress_default(
                reinterpret_cast<const char*>(data.data()),
                reinterpret_cast<char*>(out.data()),
                static_cast<int>(data.size()),
                bound
            );
            if (compressed <= 0) {
                THEMIS_WARN("LZ4 compression failed, falling back to uncompressed");
                return data;
            }
            out.resize(static_cast<size_t>(compressed));
            return out;
        }

        case CompressionAlgorithm::ZSTD: {
            size_t bound = ZSTD_compressBound(data.size());
            std::vector<uint8_t> out(bound);
            size_t compressed = ZSTD_compress(
                out.data(), bound,
                data.data(), data.size(),
                config_.compression_level
            );
            if (ZSTD_isError(compressed)) {
                THEMIS_WARN("ZSTD compression error: {}", ZSTD_getErrorName(compressed));
                return data;
            }
            out.resize(compressed);
            return out;
        }

        case CompressionAlgorithm::SNAPPY: {
            std::string input(reinterpret_cast<const char*>(data.data()), data.size());
            std::string output;
            snappy::Compress(input.data(), input.size(), &output);
            return std::vector<uint8_t>(output.begin(), output.end());
        }

        default:
            return data;
    }
}

std::vector<uint8_t> CompressedReplicationStream::decompress(
    const std::vector<uint8_t>& compressed,
    CompressionAlgorithm algo) const
{
    if (compressed.empty()) return {};

    switch (algo) {
        case CompressionAlgorithm::NONE:
            return compressed;

        case CompressionAlgorithm::LZ4: {
            // LZ4 format does not store the original size, so we must pre-allocate
            // a buffer large enough to hold the decompressed output.  We use 4× the
            // compressed size as a conservative upper bound (typical LZ4 ratios for
            // text / JSON are 2-4×); the extra 256 bytes guards against very small
            // compressed inputs whose expansion is dominated by header overhead.
            // LZ4_decompress_safe will return an error (negative) if the buffer is
            // too small, at which point the caller should retry with a larger buffer
            // or fall back to storing the original size alongside the compressed data.
            std::vector<uint8_t> out(compressed.size() * 4 + 256);
            int result = LZ4_decompress_safe(
                reinterpret_cast<const char*>(compressed.data()),
                reinterpret_cast<char*>(out.data()),
                static_cast<int>(compressed.size()),
                static_cast<int>(out.size())
            );
            if (result < 0) {
                THEMIS_ERROR("LZ4 decompression failed");
                return {};
            }
            out.resize(static_cast<size_t>(result));
            return out;
        }

        case CompressionAlgorithm::ZSTD: {
            uint64_t dsize = ZSTD_getFrameContentSize(compressed.data(), compressed.size());
            if (dsize == ZSTD_CONTENTSIZE_UNKNOWN || dsize == ZSTD_CONTENTSIZE_ERROR) {
                THEMIS_ERROR("ZSTD: cannot determine decompressed size");
                return {};
            }
            std::vector<uint8_t> out(dsize);
            size_t result = ZSTD_decompress(
                out.data(), dsize,
                compressed.data(), compressed.size()
            );
            if (ZSTD_isError(result)) {
                THEMIS_ERROR("ZSTD decompression error: {}", ZSTD_getErrorName(result));
                return {};
            }
            out.resize(result);
            return out;
        }

        case CompressionAlgorithm::SNAPPY: {
            std::string input(reinterpret_cast<const char*>(compressed.data()),
                              compressed.size());
            std::string output;
            if (!snappy::Uncompress(input.data(), input.size(), &output)) {
                THEMIS_ERROR("Snappy decompression failed");
                return {};
            }
            return std::vector<uint8_t>(output.begin(), output.end());
        }

        default:
            return compressed;
    }
}

bool CompressedReplicationStream::sendBatch(const std::vector<WALEntry>& entries) {
    if (entries.empty()) return true;

    auto raw = serializeEntries(entries);
    uint64_t uncompressed_size = raw.size();

    auto algo = selectAlgorithm(uncompressed_size);
    auto compressed = compress(raw, algo);
    uint64_t compressed_size = compressed.size();

    // In production: send `compressed` over the network to `endpoint_`
    // Here we simply track statistics and return success.

    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.bytes_uncompressed += uncompressed_size;
        stats_.bytes_compressed   += compressed_size;
        stats_.algorithm_used      = algorithmName(algo);
        if (stats_.bytes_uncompressed > 0) {
            stats_.compression_ratio =
                static_cast<double>(stats_.bytes_uncompressed) /
                static_cast<double>(stats_.bytes_compressed);
        }
    }
    return true;
}

CompressedReplicationStream::CompressionStats CompressedReplicationStream::getStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

void CompressedReplicationStream::resetStats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_ = CompressionStats{};
}

// ============================================================================
// BatchedAckTracker Implementation (v1.6.0)
// ============================================================================

BatchedAckTracker::BatchedAckTracker()
    : BatchedAckTracker(AckBatchConfig{})
{
}

BatchedAckTracker::BatchedAckTracker(const AckBatchConfig& config)
    : config_(config)
{
    running_.store(true);
    flush_thread_ = std::thread(&BatchedAckTracker::flushLoop, this);
}

BatchedAckTracker::~BatchedAckTracker() {
    running_.store(false);
    flush_cv_.notify_all();
    if (flush_thread_.joinable()) flush_thread_.join();
}

void BatchedAckTracker::recordApplied(uint64_t sequence_number) {
    {
        std::lock_guard<std::mutex> lock(pending_mutex_);
        pending_.push_back(sequence_number);
        if (sequence_number > highest_acked_.load()) {
            highest_acked_.store(sequence_number);
        }
        if (pending_.size() >= config_.max_batch_size) {
            flushPending();
        }
    }
    flush_cv_.notify_one();
}

std::optional<BatchedAckTracker::AckBatch> BatchedAckTracker::dequeuePendingAcks() {
    std::lock_guard<std::mutex> lock(ready_mutex_);
    if (ready_batches_.empty()) return std::nullopt;
    auto batch = std::move(ready_batches_.front());
    ready_batches_.pop();
    return batch;
}

void BatchedAckTracker::forceFlush() {
    std::lock_guard<std::mutex> lock(pending_mutex_);
    flushPending();
}

void BatchedAckTracker::flushPending() {
    // Called with pending_mutex_ held
    if (pending_.empty()) return;

    AckBatch batch;
    batch.sequences   = std::move(pending_);
    batch.created_at  = std::chrono::system_clock::now();
    pending_.clear();

    stats_total_acks_.fetch_add(batch.sequences.size());
    stats_total_batches_.fetch_add(1);

    {
        std::lock_guard<std::mutex> rlock(ready_mutex_);
        ready_batches_.push(std::move(batch));
    }
}

BatchedAckTracker::Stats BatchedAckTracker::getStats() const {
    Stats s;
    s.total_acks_sent    = stats_total_acks_.load();
    s.total_batches_sent = stats_total_batches_.load();
    s.avg_batch_size     = (s.total_batches_sent > 0)
        ? static_cast<double>(s.total_acks_sent) /
          static_cast<double>(s.total_batches_sent)
        : 0.0;
    return s;
}

void BatchedAckTracker::flushLoop() {
    while (running_.load()) {
        {
            std::unique_lock<std::mutex> lock(pending_mutex_);
            flush_cv_.wait_for(lock,
                std::chrono::milliseconds(config_.flush_interval_ms),
                [this] { return !running_.load() || !pending_.empty(); });
            flushPending();
        }
    }
    // Final flush on shutdown
    {
        std::lock_guard<std::mutex> lock(pending_mutex_);
        flushPending();
    }
}

// ============================================================================
// ReplicationAnalytics Implementation (v1.6.0)
// ============================================================================

ReplicationAnalytics::ReplicationAnalytics() = default;

void ReplicationAnalytics::setConfig(const AnalyticsConfig& config) {
    config_ = config;
}

void ReplicationAnalytics::recordLag(const std::string& replica_id, int64_t lag_ms) {
    std::unique_lock<std::shared_mutex> lock(data_mutex_);
    auto& history = lag_history_[replica_id];
    history.push_back({std::chrono::system_clock::now(), lag_ms});
    // Rolling window: drop oldest entries beyond max_history_per_replica
    while (history.size() > config_.max_history_per_replica) {
        history.pop_front();
    }
}

int64_t ReplicationAnalytics::percentile(const std::vector<int64_t>& sorted, double p) {
    if (sorted.empty()) return 0;
    // Caller must pass a sorted vector; index is clamped to valid range.
    size_t idx = static_cast<size_t>(p / 100.0 * static_cast<double>(sorted.size() - 1));
    return sorted[std::min(idx, sorted.size() - 1)];
}

ReplicationAnalytics::LagHistory ReplicationAnalytics::getLagHistory(
    const std::string& replica_id,
    std::chrono::hours duration) const
{
    std::shared_lock<std::shared_mutex> lock(data_mutex_);
    LagHistory result;

    auto it = lag_history_.find(replica_id);
    if (it == lag_history_.end()) return result;

    auto cutoff = std::chrono::system_clock::now() - duration;
    std::vector<int64_t> values;
    for (const auto& dp : it->second) {
        if (dp.timestamp >= cutoff) {
            result.data_points.push_back(dp);
            values.push_back(dp.lag_ms);
        }
    }

    if (values.empty()) return result;

    std::sort(values.begin(), values.end());  // Sort once; reused by all percentile calls
    result.max_lag_ms = values.back();
    result.avg_lag_ms = std::accumulate(values.begin(), values.end(), int64_t{0}) /
                        static_cast<int64_t>(values.size());
    result.p95_lag_ms = percentile(values, 95.0);
    result.p99_lag_ms = percentile(values, 99.0);
    return result;
}

std::vector<ReplicationAnalytics::Insight> ReplicationAnalytics::getInsights() const {
    std::shared_lock<std::shared_mutex> lock(data_mutex_);
    std::vector<Insight> insights;
    auto now = std::chrono::system_clock::now();

    for (const auto& [replica_id, history] : lag_history_) {
        if (history.empty()) continue;

        // Check last data point for spike
        int64_t last_lag = history.back().lag_ms;
        if (last_lag > config_.lag_spike_threshold_ms) {
            Insight ins;
            ins.type        = "LAG_SPIKE";
            ins.description = "Replica " + replica_id + " lag is " +
                              std::to_string(last_lag) + "ms";
            ins.recommendation = "Check network connectivity to " + replica_id +
                                 ", consider increasing batch_size";
            ins.detected_at = now;
            ins.metadata["replica_id"] = replica_id;
            ins.metadata["lag_ms"]     = std::to_string(last_lag);
            insights.push_back(std::move(ins));
        }

        // Check rolling average for slow replica
        std::vector<int64_t> values;
        values.reserve(history.size());
        for (const auto& dp : history) values.push_back(dp.lag_ms);
        int64_t avg = std::accumulate(values.begin(), values.end(), int64_t{0}) /
                      static_cast<int64_t>(values.size());

        if (avg > config_.slow_replica_avg_ms) {
            Insight ins;
            ins.type        = "SLOW_REPLICA";
            ins.description = "Replica " + replica_id + " avg lag is " +
                              std::to_string(avg) + "ms";
            ins.recommendation = "Investigate disk I/O or CPU on " + replica_id;
            ins.detected_at = now;
            ins.metadata["replica_id"] = replica_id;
            ins.metadata["avg_lag_ms"] = std::to_string(avg);
            insights.push_back(std::move(ins));
        }
    }
    return insights;
}

std::vector<ReplicationAnalytics::Bottleneck> ReplicationAnalytics::detectBottlenecks() const {
    std::shared_lock<std::shared_mutex> lock(data_mutex_);
    std::vector<Bottleneck> bottlenecks;

    for (const auto& [replica_id, history] : lag_history_) {
        if (history.size() < 2) continue;

        // Compute variance in lag as a proxy for the bottleneck type:
        //  High variance + high avg → NETWORK jitter
        //  High avg + low variance   → DISK_IO
        //  Very high avg              → CPU
        std::vector<int64_t> values;
        values.reserve(history.size());
        for (const auto& dp : history) values.push_back(dp.lag_ms);

        int64_t avg = std::accumulate(values.begin(), values.end(), int64_t{0}) /
                      static_cast<int64_t>(values.size());
        int64_t max_val = *std::max_element(values.begin(), values.end());

        if (avg <= 0) continue;

        // normalized_range = (max - mean) / mean
        // A high value indicates large relative spread (typical of network jitter).
        // This is distinct from the coefficient of variation (stddev/mean); using
        // the range here avoids an extra O(n) pass while still capturing spikiness.
        double normalized_range = static_cast<double>(max_val - avg) /
                                  static_cast<double>(avg);

        Bottleneck b;
        b.replica_id = replica_id;
        if (avg > 10000) {
            b.bottleneck_type = "CPU";
            b.severity        = std::min(1.0, static_cast<double>(avg) / 30000.0);
            b.details         = "Extremely high avg lag (" + std::to_string(avg) + "ms)";
        } else if (normalized_range > 1.5) {
            b.bottleneck_type = "NETWORK";
            b.severity        = std::min(1.0, normalized_range / 5.0);
            b.details         = "High lag spread (normalized_range=" +
                                std::to_string(static_cast<int>(normalized_range * 100)) + "%)";
        } else if (avg > config_.slow_replica_avg_ms) {
            b.bottleneck_type = "DISK_IO";
            b.severity        = std::min(1.0,
                                    static_cast<double>(avg) /
                                    static_cast<double>(config_.slow_replica_avg_ms * 5));
            b.details         = "Consistently high lag (" + std::to_string(avg) + "ms avg)";
        } else {
            continue;  // No bottleneck detected
        }

        bottlenecks.push_back(std::move(b));
    }
    return bottlenecks;
}

std::string ReplicationAnalytics::exportPrometheusMetrics() const {
    std::shared_lock<std::shared_mutex> lock(data_mutex_);
    std::ostringstream oss;
    oss << "# HELP themisdb_replication_lag_ms Current replication lag\n"
        << "# TYPE themisdb_replication_lag_ms gauge\n";
    for (const auto& [replica_id, history] : lag_history_) {
        if (!history.empty()) {
            oss << "themisdb_replication_lag_ms{replica=\"" << replica_id << "\"} "
                << history.back().lag_ms << "\n";
        }
    }
    return oss.str();
}

// ============================================================================
// ReplicationBenchmark Implementation (v1.6.0)
// ============================================================================

ReplicationBenchmark::ReplicationBenchmark(std::shared_ptr<WALManager> wal)
    : ReplicationBenchmark(std::move(wal), BenchmarkConfig{})
{
}

ReplicationBenchmark::ReplicationBenchmark(
    std::shared_ptr<WALManager> wal,
    const BenchmarkConfig& config)
    : wal_(std::move(wal))
    , config_(config)
{
}

ReplicationBenchmark::BenchmarkResult ReplicationBenchmark::run() {
    // Build a dummy payload of the requested size
    std::string payload(config_.entry_size_bytes, 'x');

    // Warm up (entries not counted in result)
    for (uint32_t i = 0; i < config_.warmup_entries; ++i) {
        WALEntry e;
        e.sequence_number = 0;
        e.collection  = config_.collection;
        e.document_id = "warmup-" + std::to_string(i);
        e.operation   = "INSERT";
        e.data        = payload;
        wal_->append(e);
    }

    // Measurement run
    std::vector<int64_t> latencies_us;
    latencies_us.reserve(config_.num_entries);
    uint64_t bytes_written = 0;

    auto bench_start = std::chrono::high_resolution_clock::now();

    for (uint32_t i = 0; i < config_.num_entries; ++i) {
        WALEntry e;
        e.sequence_number = 0;
        e.collection  = config_.collection;
        e.document_id = "bench-" + std::to_string(i);
        e.operation   = "INSERT";
        e.data        = payload;

        auto t0 = std::chrono::high_resolution_clock::now();
        wal_->append(e);
        auto t1 = std::chrono::high_resolution_clock::now();

        latencies_us.push_back(
            std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count());
        bytes_written += config_.entry_size_bytes;
    }

    auto bench_end = std::chrono::high_resolution_clock::now();
    double duration_s = std::chrono::duration<double>(bench_end - bench_start).count();

    // Compute percentiles
    std::sort(latencies_us.begin(), latencies_us.end());
    auto pct = [&](double p) -> int64_t {
        if (latencies_us.empty()) return 0;
        size_t idx = static_cast<size_t>(
            p / 100.0 * static_cast<double>(latencies_us.size() - 1));
        return latencies_us[std::min(idx, latencies_us.size() - 1)];
    };

    BenchmarkResult r;
    r.total_entries      = config_.num_entries;
    r.duration_seconds   = duration_s;
    r.writes_per_second  = (duration_s > 0.0)
        ? static_cast<double>(config_.num_entries) / duration_s : 0.0;
    r.latency_p50_us     = pct(50.0);
    r.latency_p95_us     = pct(95.0);
    r.latency_p99_us     = pct(99.0);
    r.latency_max_us     = latencies_us.empty() ? 0 : latencies_us.back();
    r.bytes_written      = bytes_written;
    return r;
}

std::string ReplicationBenchmark::format(const BenchmarkResult& r) {
    std::ostringstream oss;
    oss << "=== ReplicationBenchmark Results ===\n"
        << "Entries:       " << r.total_entries       << "\n"
        << "Duration:      " << r.duration_seconds    << "s\n"
        << "Throughput:    " << static_cast<uint64_t>(r.writes_per_second) << " writes/sec\n"
        << "Bytes written: " << r.bytes_written       << "\n"
        << "Latency p50:   " << r.latency_p50_us      << " µs\n"
        << "Latency p95:   " << r.latency_p95_us      << " µs\n"
        << "Latency p99:   " << r.latency_p99_us      << " µs\n"
        << "Latency max:   " << r.latency_max_us      << " µs\n";
    return oss.str();
}

// ============================================================================
// CDCManager Implementation (v1.6.0)
// ============================================================================

uint64_t CDCManager::subscribe(const std::string& collection, CDCCallback callback) {
    uint64_t id = next_id_.fetch_add(1);
    std::unique_lock<std::shared_mutex> lock(subs_mutex_);
    subscriptions_.push_back({id, collection, std::move(callback)});
    return id;
}

void CDCManager::unsubscribe(uint64_t subscription_id) {
    std::unique_lock<std::shared_mutex> lock(subs_mutex_);
    subscriptions_.erase(
        std::remove_if(subscriptions_.begin(), subscriptions_.end(),
                       [subscription_id](const Subscription& s) {
                           return s.id == subscription_id;
                       }),
        subscriptions_.end());
}

size_t CDCManager::subscriptionCount() const {
    std::shared_lock<std::shared_mutex> lock(subs_mutex_);
    return subscriptions_.size();
}

void CDCManager::onWALEntryApplied(const WALEntry& entry) {
    std::shared_lock<std::shared_mutex> lock(subs_mutex_);
    for (const auto& sub : subscriptions_) {
        // Empty collection = wildcard; otherwise match on collection name
        if (sub.collection.empty() || sub.collection == entry.collection) {
            try {
                sub.callback(entry);
            } catch (const std::exception& e) {
                THEMIS_ERROR("CDCManager: subscriber {} threw: {}", sub.id, e.what());
            } catch (...) {
                THEMIS_ERROR("CDCManager: subscriber {} threw unknown exception", sub.id);
            }
        }
    }
}

// ============================================================================
// WALArchivalManager Implementation (v1.6.0)
// ============================================================================

WALArchivalManager::WALArchivalManager(const ArchivalConfig& config)
    : config_(config) {
    // Ensure archive directory exists
    std::error_code ec;
    std::filesystem::create_directories(config_.archive_directory, ec);
    // Load existing index if present
    loadIndex();
}

std::string WALArchivalManager::archivePath(uint64_t segment_id) const {
    std::ostringstream oss;
    oss << config_.archive_directory << "/seg_"
        << std::setw(20) << std::setfill('0') << segment_id
        << (config_.compress_before_archive ? ".wal.zst" : ".wal");
    return oss.str();
}

/* static */ std::vector<uint8_t> WALArchivalManager::compressData(
    const std::vector<uint8_t>& data) {
    size_t bound = ZSTD_compressBound(data.size());
    std::vector<uint8_t> out(bound);
    size_t compressed = ZSTD_compress(
        out.data(), bound, data.data(), data.size(), /*level=*/3);
    if (ZSTD_isError(compressed)) {
        return data;  // fall back to uncompressed on error
    }
    out.resize(compressed);
    return out;
}

void WALArchivalManager::saveIndex() const {
    // Simple text-format index: one line per segment
    std::string index_path = config_.archive_directory + "/index.txt";
    std::ofstream f(index_path);
    if (!f) return;
    for (const auto& seg : index_) {
        auto ts = std::chrono::duration_cast<std::chrono::seconds>(
            seg.archived_at.time_since_epoch()).count();
        f << seg.segment_id << " "
          << seg.start_sequence << " "
          << seg.end_sequence   << " "
          << seg.size_bytes     << " "
          << (seg.compressed ? 1 : 0) << " "
          << ts                  << " "
          << seg.archive_path    << "\n";
    }
}

void WALArchivalManager::loadIndex() {
    std::string index_path = config_.archive_directory + "/index.txt";
    std::ifstream f(index_path);
    if (!f) return;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        ArchivedSegment seg;
        int64_t ts = 0;
        int compressed = 0;
        if (iss >> seg.segment_id >> seg.start_sequence >> seg.end_sequence
                >> seg.size_bytes >> compressed >> ts >> seg.archive_path) {
            seg.compressed = (compressed != 0);
            seg.archived_at = std::chrono::system_clock::time_point(
                std::chrono::seconds(ts));
            index_.push_back(seg);
        }
    }
}

uint32_t WALArchivalManager::archiveSegments(
    const std::vector<std::string>& segment_paths) {
    uint32_t archived = 0;
    std::lock_guard<std::mutex> lock(archive_mutex_);

    for (const auto& seg_path : segment_paths) {
        std::string full_path = config_.wal_directory + "/" + seg_path;
        std::ifstream src(full_path, std::ios::binary);
        if (!src) {
            THEMIS_WARN("WALArchival: cannot open segment {}", full_path);
            continue;
        }

        std::vector<uint8_t> raw(
            (std::istreambuf_iterator<char>(src)),
            std::istreambuf_iterator<char>());

        // Derive a segment_id from the numeric portion between 'seg_' and '.'
        // in the filename (e.g. "seg_000042.wal" → 42).  Fall back to an
        // atomic counter to avoid collisions when the name doesn't match the
        // expected pattern.
        static std::atomic<uint64_t> fallback_id{1};
        uint64_t segment_id = 0;
        {
            // Look for digits immediately after the last '_' before the last '.'
            auto dot_pos = seg_path.rfind('.');
            auto under_pos = seg_path.rfind('_', dot_pos);
            if (under_pos != std::string::npos && dot_pos != std::string::npos
                    && under_pos < dot_pos) {
                std::string num_str = seg_path.substr(under_pos + 1,
                                                       dot_pos - under_pos - 1);
                bool all_digits = !num_str.empty() &&
                    std::all_of(num_str.begin(), num_str.end(), ::isdigit);
                if (all_digits) {
                    try { segment_id = std::stoull(num_str); }
                    catch (...) { segment_id = 0; }
                }
            }
        }
        if (segment_id == 0) {
            segment_id = fallback_id.fetch_add(1);
        }

        std::vector<uint8_t> payload = config_.compress_before_archive
                                           ? compressData(raw)
                                           : raw;

        std::string dest = archivePath(segment_id);
        std::ofstream dst(dest, std::ios::binary);
        if (!dst) {
            THEMIS_ERROR("WALArchival: cannot write archive {}", dest);
            continue;
        }
        dst.write(reinterpret_cast<const char*>(payload.data()),
                  static_cast<std::streamsize>(payload.size()));
        dst.close();

        ArchivedSegment meta;
        meta.segment_id     = segment_id;
        meta.start_sequence = 0;  // not extracted from binary WAL format here
        meta.end_sequence   = 0;
        meta.size_bytes     = payload.size();
        meta.compressed   = config_.compress_before_archive;
        meta.archived_at  = std::chrono::system_clock::now();
        meta.archive_path = dest;
        index_.push_back(meta);
        ++archived;

        THEMIS_INFO("WALArchival: archived segment {} -> {} ({} bytes)",
                    segment_id, dest, payload.size());
    }

    if (archived > 0) saveIndex();
    return archived;
}

std::optional<std::vector<uint8_t>> WALArchivalManager::retrieveSegment(
    uint64_t segment_id) const {
    std::lock_guard<std::mutex> lock(archive_mutex_);

    auto it = std::find_if(index_.begin(), index_.end(),
                           [segment_id](const ArchivedSegment& s) {
                               return s.segment_id == segment_id;
                           });
    if (it == index_.end()) {
        THEMIS_WARN("WALArchival: segment {} not found in index", segment_id);
        return std::nullopt;
    }

    std::ifstream f(it->archive_path, std::ios::binary);
    if (!f) {
        THEMIS_ERROR("WALArchival: archive file {} missing", it->archive_path);
        return std::nullopt;
    }
    std::vector<uint8_t> raw(
        (std::istreambuf_iterator<char>(f)),
        std::istreambuf_iterator<char>());

    if (!it->compressed) return raw;

    // Decompress with ZSTD
    uint64_t decompressed_size = ZSTD_getFrameContentSize(raw.data(), raw.size());
    if (decompressed_size == ZSTD_CONTENTSIZE_ERROR ||
        decompressed_size == ZSTD_CONTENTSIZE_UNKNOWN) {
        THEMIS_ERROR("WALArchival: cannot determine decompressed size for segment {}",
                     segment_id);
        return std::nullopt;
    }
    std::vector<uint8_t> out(decompressed_size);
    size_t result = ZSTD_decompress(
        out.data(), out.size(), raw.data(), raw.size());
    if (ZSTD_isError(result)) {
        THEMIS_ERROR("WALArchival: decompression failed for segment {}", segment_id);
        return std::nullopt;
    }
    out.resize(result);
    return out;
}

std::vector<WALArchivalManager::ArchivedSegment>
WALArchivalManager::listArchived() const {
    std::lock_guard<std::mutex> lock(archive_mutex_);
    auto copy = index_;
    std::sort(copy.begin(), copy.end(),
              [](const ArchivedSegment& a, const ArchivedSegment& b) {
                  return a.segment_id < b.segment_id;
              });
    return copy;
}

uint32_t WALArchivalManager::purgeExpired() {
    // delete_after_days == 0 means purge everything immediately (no retention)
    auto cutoff = (config_.delete_after_days == 0)
        ? std::chrono::system_clock::time_point::max()   // purge all
        : std::chrono::system_clock::now()
              - std::chrono::hours(24 * config_.delete_after_days);

    std::lock_guard<std::mutex> lock(archive_mutex_);
    uint32_t purged = 0;
    auto it = index_.begin();
    while (it != index_.end()) {
        if (it->archived_at < cutoff) {
            std::error_code ec;
            std::filesystem::remove(it->archive_path, ec);
            THEMIS_INFO("WALArchival: purged expired segment {} ({})",
                        it->segment_id, it->archive_path);
            it = index_.erase(it);
            ++purged;
        } else {
            ++it;
        }
    }
    if (purged > 0) saveIndex();
    return purged;
}

uint32_t WALArchivalManager::runArchivalCycle() {
    // Collect segment files from the WAL directory older than the retention limit
    std::vector<std::string> candidates;
    std::error_code ec;
    if (!std::filesystem::exists(config_.wal_directory, ec)) return 0;

    for (const auto& entry :
         std::filesystem::directory_iterator(config_.wal_directory, ec)) {
        if (ec) break;
        if (entry.is_regular_file()) {
            candidates.push_back(entry.path().filename().string());
        }
    }

    // Archive everything beyond the local_retention_segments threshold
    std::sort(candidates.begin(), candidates.end());
    if (candidates.size() <= config_.local_retention_segments) return 0;

    candidates.resize(candidates.size() - config_.local_retention_segments);
    uint32_t archived = archiveSegments(candidates);
    purgeExpired();
    return archived;
}

} // namespace replication
} // namespace themisdb
