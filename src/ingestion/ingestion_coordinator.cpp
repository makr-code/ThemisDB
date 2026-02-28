/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ingestion_coordinator.cpp                          ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-02-28                                         ║
  Author:          copilot-swe-agent[bot]                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file ingestion_coordinator.cpp
 * @brief Distributed ingestion coordinator implementation.
 *
 * Implements:
 *   - InProcessLeaderElection  — TTL-based in-process leader lease
 *   - ConsistentHashRing       — FNV-1a virtual-node hash ring
 *   - InProcessWorkerNode      — local worker backed by IngestionManager
 *   - IngestionCoordinator     — orchestrator: partitions, dispatches, aggregates
 *
 * @author ThemisDB Team
 * @date February 2026
 */

#include "ingestion/ingestion_coordinator.h"
#include "ingestion/ingestion_manager.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <future>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace themis {
namespace ingestion {

// ============================================================================
// Internal helpers
// ============================================================================

namespace {

/// FNV-1a 64-bit hash — deterministic, no external dependency.
inline uint64_t fnv1a64(const std::string& s) {
    uint64_t h = UINT64_C(14695981039346656037);
    for (unsigned char c : s) {
        h ^= static_cast<uint64_t>(c);
        h *= UINT64_C(1099511628211);
    }
    return h;
}

/// Build a unique virtual-node key: "<node_id>#<replica_index>".
inline std::string vnodeKey(const std::string& node_id, size_t replica) {
    return node_id + '#' + std::to_string(replica);
}

} // anonymous namespace

// ============================================================================
// InProcessLeaderElection
// ============================================================================

bool InProcessLeaderElection::tryAcquireLease(const std::string& node_id,
                                               std::chrono::milliseconds ttl) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto now = std::chrono::steady_clock::now();

    // Acquire if: no valid lease OR we already own it (renewal).
    bool can_acquire = !current_lease_.isValid()
                    || current_lease_.owner_node_id == node_id;
    if (!can_acquire) {
        return false;
    }

    // Fresh acquisition gets a new epoch; renewal keeps the same epoch.
    if (current_lease_.owner_node_id != node_id) {
        ++epoch_;
    }
    current_lease_.owner_node_id = node_id;
    current_lease_.expires_at    = now + ttl;
    current_lease_.epoch         = epoch_;
    return true;
}

LeaderLease InProcessLeaderElection::getCurrentLease() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_lease_;
}

void InProcessLeaderElection::revokeLease(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (current_lease_.owner_node_id == node_id) {
        current_lease_ = LeaderLease{};
    }
}

// ============================================================================
// ConsistentHashRing
// ============================================================================

ConsistentHashRing::ConsistentHashRing(size_t virtual_nodes_per_node)
    : virtual_nodes_per_node_(virtual_nodes_per_node == 0
                                   ? kDefaultVirtualNodes
                                   : virtual_nodes_per_node) {}

void ConsistentHashRing::addNode(const std::string& node_id) {
    // Idempotent: skip if already present.
    if (std::find(node_ids_.begin(), node_ids_.end(), node_id) != node_ids_.end()) {
        return;
    }
    node_ids_.push_back(node_id);
    for (size_t i = 0; i < virtual_nodes_per_node_; ++i) {
        uint64_t h = hashKey(vnodeKey(node_id, i));
        ring_[h]   = node_id;
    }
}

void ConsistentHashRing::removeNode(const std::string& node_id) {
    auto it = std::find(node_ids_.begin(), node_ids_.end(), node_id);
    if (it == node_ids_.end()) {
        return;  // not present
    }
    node_ids_.erase(it);
    for (size_t i = 0; i < virtual_nodes_per_node_; ++i) {
        uint64_t h = hashKey(vnodeKey(node_id, i));
        ring_.erase(h);
    }
}

std::string ConsistentHashRing::getNode(const std::string& key) const {
    if (ring_.empty()) {
        return {};
    }
    uint64_t h = hashKey(key);
    auto it    = ring_.lower_bound(h);
    if (it == ring_.end()) {
        // Wrap around to the first node on the ring.
        it = ring_.begin();
    }
    return it->second;
}

uint64_t ConsistentHashRing::hashKey(const std::string& key) const {
    return fnv1a64(key);
}

// ============================================================================
// InProcessWorkerNode
// ============================================================================

InProcessWorkerNode::InProcessWorkerNode(const std::string& node_id,
                                         const std::string& db_connection)
    : node_id_(node_id)
    , db_connection_(db_connection) {}

bool InProcessWorkerNode::isAvailable() const {
    return !busy_.load(std::memory_order_acquire);
}

IngestionReport InProcessWorkerNode::ingest(
    const std::vector<SourceConfig>& sources,
    const std::string& target_collection,
    ProgressCallback progress_callback)
{
    // Serialise concurrent calls from the same coordinator (should not happen
    // in normal operation, but protects against misuse).
    std::lock_guard<std::mutex> lock(busy_mutex_);
    busy_.store(true, std::memory_order_release);

    struct BusyGuard {
        std::atomic<bool>& flag;
        ~BusyGuard() { flag.store(false, std::memory_order_release); }
    } guard{busy_};

    if (sources.empty()) {
        return IngestionReport{};
    }

    // Create a local IngestionManager for this node and register all assigned
    // sources.  Using a per-run manager keeps the state isolated between nodes.
    IngestionManager manager(db_connection_);
    manager.setTargetCollection(target_collection);
    manager.setParallelProcessing(true);

    for (const auto& src : sources) {
        manager.registerSource(src);
    }

    return manager.ingestAll(progress_callback);
}

// ============================================================================
// IngestionCoordinator — helpers
// ============================================================================

namespace {

/// Generate a unique coordinator node ID (hex timestamp + counter).
std::string makeCoordinatorNodeId() {
    static std::atomic<uint64_t> counter{0};
    auto ts = static_cast<uint64_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    auto seq = ++counter;
    std::ostringstream ss;
    ss << std::hex << ts << '-' << seq;
    return "coord-" + ss.str();
}

} // anonymous namespace

// ============================================================================
// IngestionCoordinator — construction / destruction
// ============================================================================

IngestionCoordinator::IngestionCoordinator(const Config& config)
    : config_(config)
    , my_node_id_(makeCoordinatorNodeId())
    , leader_election_(std::make_shared<InProcessLeaderElection>())
{
    // Default num_nodes = hardware_concurrency / 2, min 1.
    if (config_.num_nodes == 0) {
        unsigned hw = std::thread::hardware_concurrency();
        config_.num_nodes = std::max(1u, hw / 2);
    }
}

IngestionCoordinator::~IngestionCoordinator() {
    stop();
}

// ============================================================================
// IngestionCoordinator — lifecycle
// ============================================================================

void IngestionCoordinator::start() {
    if (running_.exchange(true)) {
        return;  // already running
    }

    // Create default in-process worker nodes when none have been registered.
    {
        std::lock_guard<std::mutex> lock(nodes_mutex_);
        if (nodes_.empty()) {
            for (size_t i = 0; i < config_.num_nodes; ++i) {
                std::string nid = "worker-" + std::to_string(i);
                auto node = std::make_shared<InProcessWorkerNode>(
                    nid, config_.db_connection);
                nodes_.push_back(node);
                hash_ring_.addNode(nid);
            }
        }
    }

    // Start lease renewal background thread.
    lease_renewal_running_.store(true);
    lease_renewal_thread_ = std::thread(&IngestionCoordinator::leaseRenewalLoop, this);
}

void IngestionCoordinator::stop() {
    if (!running_.exchange(false)) {
        return;  // already stopped
    }

    // Stop lease renewal thread.
    lease_renewal_running_.store(false);
    if (lease_renewal_thread_.joinable()) {
        lease_renewal_thread_.join();
    }

    // Voluntarily release the leader lease.
    leader_election_->revokeLease(my_node_id_);
}

// ============================================================================
// IngestionCoordinator — node management
// ============================================================================

void IngestionCoordinator::registerNode(std::shared_ptr<IIngestionWorkerNode> node) {
    if (!node) return;
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    // Avoid duplicate registration.
    for (const auto& existing : nodes_) {
        if (existing->nodeId() == node->nodeId()) {
            return;
        }
    }
    nodes_.push_back(node);
    hash_ring_.addNode(node->nodeId());
}

std::vector<NodeInfo> IngestionCoordinator::getNodes() const {
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    std::string leader_id = leader_election_->getCurrentLease().owner_node_id;

    std::vector<NodeInfo> result;
    result.reserve(nodes_.size());
    for (const auto& n : nodes_) {
        NodeInfo info;
        info.node_id  = n->nodeId();
        info.is_local = true;
        info.is_leader = (n->nodeId() == leader_id);
        result.push_back(std::move(info));
    }
    return result;
}

std::string IngestionCoordinator::getLeaderNodeId() const {
    auto lease = leader_election_->getCurrentLease();
    return lease.isValid() ? lease.owner_node_id : std::string{};
}

// ============================================================================
// IngestionCoordinator — source partitioning
// ============================================================================

std::unordered_map<std::string, std::vector<SourceConfig>>
IngestionCoordinator::partitionSources(
    const std::vector<SourceConfig>& sources) const
{
    std::lock_guard<std::mutex> lock(nodes_mutex_);

    std::unordered_map<std::string, std::vector<SourceConfig>> partitions;

    if (hash_ring_.empty()) {
        // No nodes — return everything under an empty-string key so callers
        // can detect the degenerate case.
        partitions[""] = sources;
        return partitions;
    }

    for (const auto& src : sources) {
        std::string node_id = hash_ring_.getNode(src.source_id);
        partitions[node_id].push_back(src);
    }
    return partitions;
}

// ============================================================================
// IngestionCoordinator — distributed ingestion
// ============================================================================

IngestionReport IngestionCoordinator::ingestAll(
    const std::vector<SourceConfig>& sources,
    ProgressCallback progress_callback)
{
    auto run_start = std::chrono::steady_clock::now();

    // Step 1 — Acquire leader lease.
    bool is_leader = leader_election_->tryAcquireLease(my_node_id_,
                                                        config_.lease_ttl);
    if (!is_leader) {
        // Return empty report with an error indicating this node is not leader.
        IngestionReport report;
        IngestionStats dummy;
        dummy.addError(IngestionErrorCode::SOURCE_UNAVAILABLE,
                       IngestionErrorSeverity::ERROR,
                       "IngestionCoordinator: failed to acquire leader lease — "
                       "another node may be leading");
        report.source_stats["__coordinator__"] = dummy;
        return report;
    }

    // Update election counter metric.
    {
        std::lock_guard<std::mutex> ml(metrics_mutex_);
        ++metrics_.leader_elections;
    }

    // Step 2 — Partition sources.
    auto partitions = partitionSources(sources);

    // Grab a snapshot of available nodes (under the nodes lock).
    std::vector<std::shared_ptr<IIngestionWorkerNode>> active_nodes;
    {
        std::lock_guard<std::mutex> lock(nodes_mutex_);
        for (const auto& n : nodes_) {
            active_nodes.push_back(n);
        }
    }

    // Step 3 — Dispatch each partition to its owning node concurrently.
    using FutureResult = std::future<IngestionReport>;
    std::vector<FutureResult> futures;
    futures.reserve(active_nodes.size());

    size_t tasks_submitted = 0;
    for (const auto& node : active_nodes) {
        auto it = partitions.find(node->nodeId());
        if (it == partitions.end() || it->second.empty()) {
            // No work for this node — submit a no-op future.
            futures.push_back(
                std::async(std::launch::deferred, []() -> IngestionReport {
                    return IngestionReport{};
                }));
            continue;
        }

        const auto& node_sources = it->second;
        ++tasks_submitted;

        // Capture by value so the lambda owns the data.
        futures.push_back(std::async(
            std::launch::async,
            [node, node_sources, this, progress_callback]() -> IngestionReport {
                return node->ingest(node_sources,
                                    config_.target_collection,
                                    progress_callback);
            }));
    }

    // Step 4 — Collect results with per-worker timeout.
    std::vector<IngestionReport> partial_reports;
    partial_reports.reserve(futures.size());

    for (auto& fut : futures) {
        if (config_.worker_timeout.count() > 0) {
            auto status = fut.wait_for(config_.worker_timeout);
            if (status == std::future_status::timeout) {
                // Worker timed out — record an error report and move on.
                IngestionReport err_report;
                IngestionStats err_stats;
                err_stats.addError(
                    IngestionErrorCode::HTTP_TIMEOUT,
                    IngestionErrorSeverity::ERROR,
                    "IngestionCoordinator: worker node timed out");
                err_report.source_stats["__timeout__"] = err_stats;
                partial_reports.push_back(err_report);
                continue;
            }
        }
        try {
            partial_reports.push_back(fut.get());
        } catch (const std::exception& ex) {
            IngestionReport err_report;
            IngestionStats err_stats;
            err_stats.addError(
                IngestionErrorCode::INTERNAL_ERROR,
                IngestionErrorSeverity::ERROR,
                std::string("IngestionCoordinator: worker threw: ") + ex.what());
            err_report.source_stats["__exception__"] = err_stats;
            partial_reports.push_back(err_report);
        }
    }

    // Step 5 — Aggregate.
    IngestionReport final_report = aggregateReports(partial_reports);

    double elapsed = std::chrono::duration<double>(
                         std::chrono::steady_clock::now() - run_start)
                         .count();

    // Update metrics.
    {
        std::lock_guard<std::mutex> ml(metrics_mutex_);
        metrics_.nodes_active    = active_nodes.size();
        metrics_.tasks_submitted = tasks_submitted;
        metrics_.tasks_completed = tasks_submitted;
        metrics_.last_run_seconds = elapsed;
    }

    return final_report;
}

// ============================================================================
// IngestionCoordinator — metrics
// ============================================================================

IngestionCoordinator::CoordinatorMetrics IngestionCoordinator::getMetrics() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    return metrics_;
}

// ============================================================================
// IngestionCoordinator — testing hook
// ============================================================================

void IngestionCoordinator::setLeaderElectionForTesting(
    std::shared_ptr<ILeaderElection> election)
{
    leader_election_ = std::move(election);
}

// ============================================================================
// IngestionCoordinator — private helpers
// ============================================================================

void IngestionCoordinator::leaseRenewalLoop() {
    // Renew the leader lease every lease_ttl / 2 to avoid expiry under load.
    auto interval = config_.lease_ttl / 2;
    if (interval < std::chrono::milliseconds(100)) {
        interval = std::chrono::milliseconds(100);
    }

    while (lease_renewal_running_.load()) {
        // Only renew if we currently hold the lease.
        auto lease = leader_election_->getCurrentLease();
        if (lease.isValid() && lease.owner_node_id == my_node_id_) {
            leader_election_->tryAcquireLease(my_node_id_, config_.lease_ttl);
        }
        std::this_thread::sleep_for(interval);
    }
}

IngestionReport IngestionCoordinator::aggregateReports(
    const std::vector<IngestionReport>& partial) const
{
    IngestionReport result;

    for (const auto& pr : partial) {
        result.total_documents    += pr.total_documents;
        result.total_failures     += pr.total_failures;
        result.total_time_seconds += pr.total_time_seconds;
        result.quarantine_retry_successes += pr.quarantine_retry_successes;

        if (pr.dry_run) {
            result.dry_run = true;
        }

        // Merge source_stats (key = source_id).
        for (const auto& kv : pr.source_stats) {
            result.source_stats[kv.first] = kv.second;
        }

        // Append quarantined items.
        for (const auto& q : pr.quarantine) {
            result.quarantine.push_back(q);
        }
    }

    return result;
}

} // namespace ingestion
} // namespace themis
