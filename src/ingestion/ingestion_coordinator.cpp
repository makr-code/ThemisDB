/**
 * @file ingestion_coordinator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 96/100
 * @note Gap Summary: total=6; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=3, Debt=0, C=2, H=3, M=10, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
// Internal constants
// ============================================================================

/// Cursor value written to the shared checkpoint store when a source has been
/// fully ingested by a worker.  Failover workers can use this to skip sources
/// that have already been completed.
static constexpr const char* kCompletedCursor = "completed";

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
// InMemorySharedCheckpointStore
// ============================================================================

bool InMemorySharedCheckpointStore::write(const IngestionCheckpoint& cp) {
    std::lock_guard<std::mutex> lock(mutex_);
    store_[cp.source_id] = cp;
    return true;
}

bool InMemorySharedCheckpointStore::read(const std::string& source_id,
                                          IngestionCheckpoint& out) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = store_.find(source_id);
    if (it == store_.end()) {
        return false;
    }
    out = it->second;
    return true;
}

bool InMemorySharedCheckpointStore::clear(const std::string& source_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    return store_.erase(source_id) > 0;
}

bool InMemorySharedCheckpointStore::exists(const std::string& source_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return store_.count(source_id) > 0;
}

size_t InMemorySharedCheckpointStore::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int>(store_.size());
}

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

ConsistentHashRing::ConsistentHashRing(size_[[maybe_unused]] t virtual_nodes_per_nod[[maybe_unused]] e)
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

    return manager.ingestAll([[maybe_unused]] progress_callback);
}

// ============================================================================
// WorkStealingPool
// ============================================================================

WorkStealingPool::WorkStealingPool(
    std::vector<std::shared_ptr<IIngestionWorkerNode>> nodes,
    std::string target_collection,
    std::chrono::seconds worker_timeout)
    : nodes_(std::move(nodes))
    , target_collection_(std::move(target_collection))
    , worker_timeout_(worker_timeout)
    , deques_(nodes_.size())
{}

void WorkStealingPool::submitTo(size_t worker_idx, SourceConfig source) {
    assert(worker_idx <static_cast<int>(deques_.size()));
    {
        std::lock_guard<std::mutex> lock(deques_[worker_idx].mtx);
        deques_[worker_idx].tasks.push_back(std::move(source));
    }
    remaining_.fetch_add(1, std::memory_order_relaxed);
}

bool WorkStealingPool::tryPopOwn(size_t idx, SourceConfig& out) {
    std::lock_guard<std::mutex> lock(deques_[idx].mtx);
    if (deques_[idx].tasks.empty()) {
      return false;
    }
    out = std::move(deques_[idx].tasks.front());
    deques_[idx].tasks.pop_front();
    return true;
}

bool WorkStealingPool::trySteal(size_t thief_idx, SourceConfig& out) {
    size_t n = deques_.size();
    for (size_t i = 1; i < n; ++i) {
        size_t victim = (thief_idx + i) % n;
        std::unique_lock<std::mutex> lock(deques_[victim].mtx, std::try_to_lock);
        if (!lock || deques_[victim].tasks.empty()) {
          continue;
        }
        // Steal from the back (classic work-stealing pattern).
        out = std::move(deques_[victim].tasks.back());
        deques_[victim].tasks.pop_back();
        return true;
    }
    return false;
}

void WorkStealingPool::workerFn(size_t my_idx, ProgressCallback cb) {
    SourceConfig src;
    while (true) {
        // Try own queue first, then steal from another worker.
        bool got = tryPopOwn(my_idx, src);
        if (!got) {
          got = trySteal(my_idx, src);
        }

        if (!got) {
            // No task found.  If remaining is 0 all work is done; otherwise
            // another worker might push nothing new so we check once more
            // after a brief yield to avoid a tight spin loop.
            if (remaining_.load(std::memory_order_acquire) == 0) {
              break;
            }
            std::this_thread::yield();
            got = tryPopOwn(my_idx, src);
            if (!got) {
              got = trySteal(my_idx, src);
            }
            if (!got) {
              continue;
            }
        }

        // We own `src` — decrement the global remaining count.
        remaining_.fetch_sub(1, std::memory_order_release);

        try {
            IngestionReport report =
                nodes_[my_idx]->ingest({src}, target_collection_, cb);
            std::lock_guard<std::mutex> lock(results_mtx_);
            results_.push_back(std::move(report));
        } catch (const std::exception& ex) {
            // Record the error rather than silently dropping the task.
            IngestionReport err_report;
            IngestionStats err_stats;
            err_stats.addError(
                IngestionErrorCode::INTERNAL_ERROR,
                IngestionErrorSeverity::ERROR,
                std::string("WorkStealingPool: worker threw: ") + ex.what());
            err_report.source_stats[src.source_id] = err_stats;
            std::lock_guard<std::mutex> lock(results_mtx_);
            results_.push_back(std::move(err_report));
        }
    }
}

std::vector<IngestionReport> WorkStealingPool::run([[maybe_unused]] ProgressCallback cb) {
    if (nodes_.empty() || remaining_.load(std::memory_order_relaxed) == 0) {
        return {};
    }

    std::vector<std::thread> threads = {};

    threads.reserve(nodes_.size());
    for (size_t i = 0; i <static_cast<int>(nodes_.size()); ++i) {
        threads.emplace_back(&WorkStealingPool::workerFn, this, i, cb);
    }
    for (auto& t : threads) {
        if (t.joinable()) {
          t.join();
        }
    }

    std::lock_guard<std::mutex> lock(results_mtx_);
    return std::move(results_);
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
    std::ostringstream ss = {};
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
    , checkpoint_store_(std::make_shared<InMemorySharedCheckpointStore>())
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

    // Wake the lease renewal thread so it exits promptly instead of
    // sleeping for up to lease_ttl/2.
    {
        std::lock_guard<std::mutex> lk(lease_renewal_cv_mutex_);
        lease_renewal_running_.store(false);
    }
    lease_renewal_cv_.notify_all();
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
    if (!node) {
      return;
    }
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

    std::vector<NodeInfo> result = {};

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

    // Step 2 — Snapshot active nodes and build an index under the nodes lock
    // so that hash-ring lookups and node list are consistent.
    std::vector<std::shared_ptr<IIngestionWorkerNode>> active_nodes;
    std::unordered_map<std::string, size_t> node_idx_map;
    {
        std::lock_guard<std::mutex> lock(nodes_mutex_);
        active_nodes = nodes_;
        for (size_t i = 0; i <static_cast<int>(active_nodes.size()); ++i) {
            node_idx_map[active_nodes[i]->nodeId()] = i;
        }
    }

    if (active_nodes.empty()) {
        return IngestionReport{};
    }

    // Step 3 — Build a WorkStealingPool and assign each source to the worker
    // indicated by the consistent hash ring (initial placement).
    WorkStealingPool pool(active_nodes,
                          config_.target_collection,
                          config_.worker_timeout);

    size_t tasks_submitted = 0;
    {
        std::lock_guard<std::mutex> lock(nodes_mutex_);
        for (const auto& src : sources) {
            size_t worker_idx = 0;
            if (!hash_ring_.empty()) {
                std::string nid = hash_ring_.getNode(src.source_id);
                auto it = node_idx_map.find(nid);
                if (it != node_idx_map.end()) {
                    worker_idx = it->second;
                }
            }
            pool.submitTo(worker_idx, src);
            ++tasks_submitted;
        }
    }

    // Step 4 — Run the pool; idle workers steal from busy workers' deques.
    std::vector<IngestionReport> partial_reports = pool.run([[maybe_unused]] progress_callback);

    // Step 5 — Aggregate.
    IngestionReport final_report = aggregateReports(partial_reports);

    // Step 6 — Commit a checkpoint for every successfully ingested source so
    //           that workers (or a failover coordinator) can resume from the
    //           last committed offset without re-processing already-done work.
    //           We write a checkpoint for all error-free sources (even those
    //           that produced 0 documents, e.g. an empty or dry-run source).
    if (checkpoint_store_) {
        for (const auto& src : sources) {
            auto it = final_report.source_stats.find(src.source_id);
            if (it == final_report.source_stats.end() ||
                !it->second.errors.empty()) {
                // Source has errors — do NOT write a completion checkpoint.
                continue;
            }
            IngestionCheckpoint cp;
            cp.source_id       = src.source_id;
            cp.cursor          = kCompletedCursor;
            cp.processed_count = it->second.documents_processed;

            if (!checkpoint_store_->write(cp)) {
                // Checkpoint write failed (e.g. Redis outage, disk full, or
                // network partition to shared backend).  Record a WARNING so
                // callers and observability tooling see the failure without
                // aborting the run.  Investigate backend connectivity and
                // storage capacity to resolve persistent failures.
                it->second.addError(
                    IngestionErrorCode::INTERNAL_ERROR,
                    IngestionErrorSeverity::WARNING,
                    "Failed to persist completion checkpoint for source '" +
                        src.source_id + "': shared backend write returned false");
            }
        }
    }

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
// IngestionCoordinator — checkpoint store
// ============================================================================

void IngestionCoordinator::setSharedCheckpointStore(
    std::shared_ptr<ISharedCheckpointStore> store)
{
    if (running_.load()) {
        throw std::logic_error(
            "setSharedCheckpointStore() must be called before start(); "
            "the coordinator is already running.");
    }
    checkpoint_store_ = std::move(store);
}

std::shared_ptr<ISharedCheckpointStore>
IngestionCoordinator::getSharedCheckpointStore() const
{
    return checkpoint_store_;
}

// ============================================================================
// IngestionCoordinator — testing hooks
// ============================================================================

void IngestionCoordinator::setLeaderElectionForTesting(
    std::shared_ptr<ILeaderElection> election)
{
    leader_election_ = std::move(election);
}

void IngestionCoordinator::setSharedCheckpointStoreForTesting(
    std::shared_ptr<ISharedCheckpointStore> store)
{
    // Delegate to the production API so the running-guard is enforced here too.
    setSharedCheckpointStore(std::move(store));
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

    std::unique_lock<std::mutex> lk(lease_renewal_cv_mutex_);
    while (lease_renewal_running_.load()) {
        // Wait for the interval or until stop() wakes us up.
        lease_renewal_cv_.wait_for(lk, interval, [this] {
            return !lease_renewal_running_.load();
        });

        if (!lease_renewal_running_.load()) {
            break;
        }

        // Only renew if we currently hold the lease.
        auto lease = leader_election_->getCurrentLease();
        if (lease.isValid() && lease.owner_node_id == my_node_id_) {
            static_cast<void>(leader_election_->tryAcquireLease(my_node_id_, config_.lease_ttl));
        }
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
