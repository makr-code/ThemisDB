/**
 * @file shard_resource_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=1, Debt=0, C=2, H=4, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "sharding/shard_resource_manager.h"
#include "utils/logger.h"
#include "utils/thread_join_utils.h"
#include <thread>
#include <algorithm>
#include <numeric>

#ifdef _WIN32
#include <windows.h>
#include <pdh.h>
#include <psapi.h>
#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "psapi.lib")
#else
#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <fstream>
#include <sstream>
#endif

#ifdef THEMIS_ENABLE_CUDA
#include <cuda_runtime.h>
#endif
#ifdef THEMIS_ENABLE_HIP
#include <hip/hip_runtime.h>
#endif

namespace themis::sharding {

// ============================================================================
// Utility Functions
// ============================================================================

namespace {
    // Calculate memory usage percentage
    double calculateMemoryUsagePercent(uint64_t used_bytes, uint64_t total_bytes) {
        return (total_bytes > 0) 
            ? (static_cast<double>(used_bytes) / total_bytes * 100.0)
            : 0.0;
    }
}

// ============================================================================
// ResourceSnapshot Serialization
// ============================================================================

/** @brief Serialize resource snapshot into JSON document. */
nlohmann::json ShardResourceManager::ResourceSnapshot::toJson() const {
    nlohmann::json j;
    j["cpu_usage_percent"] = cpu_usage_percent;
    j["ram_usage_bytes"] = ram_usage_bytes;
    j["ram_total_bytes"] = ram_total_bytes;
    j["vram_usage_bytes"] = vram_usage_bytes;
    j["vram_total_bytes"] = vram_total_bytes;
    j["disk_used_bytes"] = disk_used_bytes;
    j["disk_available_bytes"] = disk_available_bytes;
    j["network_in_bps"] = network_in_bps;
    j["network_out_bps"] = network_out_bps;
    j["active_queries"] = active_queries;
    j["pending_queries"] = pending_queries;
    j["active_transactions"] = active_transactions;
    j["avg_query_latency_ms"] = avg_query_latency_ms;
    j["p99_query_latency_ms"] = p99_query_latency_ms;
    j["health_score"] = health_score;
    
    auto time_since_epoch = timestamp.time_since_epoch();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch).count();
    j["timestamp_ms"] = ms;
    
    return j;
}

/** @brief Deserialize resource snapshot from JSON document. */
ShardResourceManager::ResourceSnapshot ShardResourceManager::ResourceSnapshot::fromJson(const nlohmann::json& j) {
    ResourceSnapshot snapshot;
    
    snapshot.cpu_usage_percent = j.value("cpu_usage_percent", 0.0f);
    snapshot.ram_usage_bytes = j.value("ram_usage_bytes", 0);
    snapshot.ram_total_bytes = j.value("ram_total_bytes", 0);
    snapshot.vram_usage_bytes = j.value("vram_usage_bytes", 0);
    snapshot.vram_total_bytes = j.value("vram_total_bytes", 0);
    snapshot.disk_used_bytes = j.value("disk_used_bytes", 0);
    snapshot.disk_available_bytes = j.value("disk_available_bytes", 0);
    snapshot.network_in_bps = j.value("network_in_bps", 0);
    snapshot.network_out_bps = j.value("network_out_bps", 0);
    snapshot.active_queries = j.value("active_queries", 0);
    snapshot.pending_queries = j.value("pending_queries", 0);
    snapshot.active_transactions = j.value("active_transactions", 0);
    snapshot.avg_query_latency_ms = j.value("avg_query_latency_ms", 0.0f);
    snapshot.p99_query_latency_ms = j.value("p99_query_latency_ms", 0.0f);
    snapshot.health_score = j.value("health_score", 100.0f);
    
    if (j.contains("timestamp_ms")) {
        auto ms = j["timestamp_ms"].get<int64_t>();
        snapshot.timestamp = std::chrono::system_clock::time_point(
            std::chrono::milliseconds(ms)
        );
    } else {
        snapshot.timestamp = std::chrono::system_clock::now();
    }
    
    return snapshot;
}

// ============================================================================
// Constructor / Destructor
// ============================================================================

/**
 * @brief Construct resource manager with explicit runtime configuration.
 * @param local_shard_id Local shard identifier.
 * @param gossip_manager Gossip manager dependency (optional).
 * @param config Sampling/throttling/gossip configuration.
 */
ShardResourceManager::ShardResourceManager(
    const std::string& local_shard_id,
    std::shared_ptr<GossipConfigManager> gossip_manager,
    const Config& config)
    : local_shard_id_(local_shard_id)
    , gossip_manager_(gossip_manager)
    , config_(config)
#ifndef _WIN32
    , prev_cpu_total_(0)
    , prev_cpu_idle_(0)
#endif
{
    // Initialize local snapshot
    local_snapshot_.timestamp = std::chrono::system_clock::now();
    local_snapshot_.health_score = 100.0f;

    // Initialise repair I/O token-bucket rate limiter.
    // Rate  = peak_node_iops * (repair_iops_budget_percent / 100)
    // Burst = same value (1-second burst window)
    if (config_.enable_repair_iops_throttle && config_.peak_node_iops > 0) {
        double rate = static_cast<double>(config_.peak_node_iops) *
                      (config_.repair_iops_budget_percent / 100.0f);
        rate = std::max(rate, 1.0);  // floor at 1 token/s
        repair_io_limiter_ = std::make_unique<themis::utils::RateLimiter>(rate, rate);
    }
}

/**
 * @brief Construct resource manager with default configuration.
 * @param local_shard_id Local shard identifier.
 * @param gossip_manager Gossip manager dependency (optional).
 */
ShardResourceManager::ShardResourceManager(
    const std::string& local_shard_id,
    std::shared_ptr<GossipConfigManager> gossip_manager)
    : ShardResourceManager(local_shard_id, gossip_manager, Config{})
{
}

/** @brief Stop manager and release platform monitoring resources. */
ShardResourceManager::~ShardResourceManager() {
    stop();
    
#ifdef _WIN32
    // Cleanup PDH resources
    if (pdh_initialized_) {
        if (pdh_query_) {
            PdhCloseQuery(reinterpret_cast<PDH_HQUERY>(pdh_query_));
        }
    }
#endif
}

// ============================================================================
// Lifecycle
// ============================================================================

/** @brief Start background monitoring loop when not already running. */
void ShardResourceManager::start() {
    if (running_.exchange(true)) {
        return; // Already running
    }
    
    monitoring_thread_ = std::thread([this]() {
        monitoringLoop();
    });
}

/** @brief Stop background monitoring loop and join worker thread. */
void ShardResourceManager::stop() {
    if (!running_.exchange(false)) {
        return; // Not running
    }
    
    // thread_join_no_timeout (W4): bounded join via joinThreadWithin
    if (!themis::utils::joinThreadWithin(monitoring_thread_)) {
        THEMIS_WARN("[ShardResourceManager] monitoring thread did not finish within shutdown deadline; detaching.");
    }
}

// ============================================================================
// Local Resource Management
// ============================================================================

/** @brief Return latest locally cached resource snapshot. */
ShardResourceManager::ResourceSnapshot ShardResourceManager::getCurrentSnapshot() const {
    std::shared_lock lock(local_mutex_);
    return local_snapshot_;
}

/** @brief Evaluate admission for query based on local CPU/RAM headroom and thresholds. */
bool ShardResourceManager::canAcceptQuery(const QuerySpec& spec) const {
    if (!config_.enable_auto_throttling) {
        return true;
    }
    
    std::shared_lock lock(local_mutex_);
    
    // Check CPU capacity
    float cpu_available = 100.0f - local_snapshot_.cpu_usage_percent;
    if (cpu_available < spec.estimated_cpu_percent) {
        return false;
    }
    
    // Check RAM capacity
    uint64_t ram_available = local_snapshot_.ram_total_bytes - local_snapshot_.ram_usage_bytes;
    if (ram_available < spec.estimated_memory_bytes) {
        return false;
    }
    
    // Check if above throttle threshold
    float cpu_ratio = local_snapshot_.cpu_usage_percent / 100.0f;
    float ram_ratio = static_cast<float>(local_snapshot_.ram_usage_bytes) / 
                      static_cast<float>(local_snapshot_.ram_total_bytes);
    
    float max_load = std::max(cpu_ratio, ram_ratio);
    
    return max_load < config_.throttle_threshold;
}

/** @brief Update active/pending query counters and average latency metric. */
void ShardResourceManager::updateQueryMetrics(uint32_t active, uint32_t pending, float avg_latency_ms) {
    std::unique_lock lock(local_mutex_);
    local_snapshot_.active_queries = active;
    local_snapshot_.pending_queries = pending;
    local_snapshot_.avg_query_latency_ms = avg_latency_ms;
}

/** @brief Apply emergency health downgrade when critical utilization threshold is reached. */
void ShardResourceManager::throttleIfNeeded() {
    float cpu_ratio, ram_ratio, max_load;
    
    {
        std::shared_lock lock(local_mutex_);
        cpu_ratio = local_snapshot_.cpu_usage_percent / 100.0f;
        ram_ratio = static_cast<float>(local_snapshot_.ram_usage_bytes) / 
                    static_cast<float>(local_snapshot_.ram_total_bytes);
        max_load = std::max(cpu_ratio, ram_ratio);
    }
    
    if (max_load >= config_.critical_threshold) {
        // Critical threshold - could implement throttling here
        // For now, just update health score
        std::unique_lock lock(local_mutex_);
        local_snapshot_.health_score = std::min(local_snapshot_.health_score, 20.0f);
    }
}

/** @brief Try to consume repair I/O tokens with optional bounded wait. */
bool ShardResourceManager::acquireRepairIOToken(double io_ops,
                                                std::chrono::milliseconds wait_timeout) {
    if (!config_.enable_repair_iops_throttle || !repair_io_limiter_) {
        return true;
    }
    if (repair_io_limiter_->try_acquire(io_ops)) {
        return true;
    }

    if (wait_timeout <= std::chrono::milliseconds::zero()) {
        return false;
    }

    const auto deadline = std::chrono::steady_clock::now() + wait_timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if (repair_io_limiter_->try_acquire(io_ops)) {
            return true;
        }
    }

    return false;
}

/** @brief Return whether GPU erasure coding can be used under current build/config. */
bool ShardResourceManager::isGPUErasureCodingEnabled() const {
    if (!config_.enable_gpu_erasure_coding) {
        return false;
    }
#ifdef THEMIS_ENABLE_CUDA
    return true;
#else
    return false;
#endif
}

// ============================================================================
// Gossip Integration
// ============================================================================

/** @brief Publish local resource snapshot to gossip subsystem. */
void ShardResourceManager::broadcastResourceUpdate() {
    if (!config_.enable_gossip_broadcast || !gossip_manager_) {
        return;
    }
    
    auto snapshot = getCurrentSnapshot();
    
    // Create a resource snapshot for gossip using GossipConfigManager's ResourceSnapshot
    themis::sharding::ResourceSnapshot gossip_snapshot;
    gossip_snapshot.shard_id = local_shard_id_;
    gossip_snapshot.timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        snapshot.timestamp.time_since_epoch()
    ).count();
    gossip_snapshot.cpu_usage_percent = snapshot.cpu_usage_percent;
    gossip_snapshot.memory_usage_percent = calculateMemoryUsagePercent(
        snapshot.ram_usage_bytes, snapshot.ram_total_bytes
    );
    gossip_snapshot.available_memory_bytes = snapshot.ram_total_bytes - snapshot.ram_usage_bytes;
    gossip_snapshot.total_memory_bytes = snapshot.ram_total_bytes;
    // std::thread::hardware_concurrency() is the correct cross-platform approach
    // (maps to sysconf(_SC_NPROCESSORS_ONLN) on Linux, GetSystemInfo on Windows).
    gossip_snapshot.total_cpu_cores = std::thread::hardware_concurrency();
    gossip_snapshot.available_cpu_cores = std::max(0, 
        gossip_snapshot.total_cpu_cores - static_cast<uint32_t>(snapshot.cpu_usage_percent / 100.0f * gossip_snapshot.total_cpu_cores)
    );
    gossip_snapshot.available_disk_bytes = snapshot.disk_available_bytes;
    gossip_snapshot.total_disk_bytes = snapshot.disk_used_bytes + snapshot.disk_available_bytes;
    gossip_snapshot.disk_usage_percent = 
        (gossip_snapshot.total_disk_bytes > 0)
            ? (static_cast<double>(snapshot.disk_used_bytes) / gossip_snapshot.total_disk_bytes * 100.0)
            : 0.0;
    gossip_snapshot.rocksdb_sst_files_count = 0;
    gossip_snapshot.rocksdb_total_size_bytes = 0;
    gossip_snapshot.requests_per_second = snapshot.active_queries; // Simplified
    gossip_snapshot.avg_latency_ms = snapshot.avg_query_latency_ms;
    gossip_snapshot.is_healthy = snapshot.health_score > 50.0;
    gossip_snapshot.status = gossip_snapshot.is_healthy ? "healthy" : "degraded";
    
    gossip_manager_->publishResourceSnapshot(gossip_snapshot);
}

/** @brief Store received peer resource snapshot into cache. */
void ShardResourceManager::receiveResourceUpdate(const std::string& shard_id, 
                                                   const ResourceSnapshot& snapshot) {
    std::unique_lock lock(peer_mutex_);
    peer_resources_[shard_id] = snapshot;
}

// ============================================================================
// Peer Awareness
// ============================================================================

std::map<std::string, ShardResourceManager::ResourceSnapshot> 
ShardResourceManager::getPeerResources() const {
    std::shared_lock lock(peer_mutex_);
    return peer_resources_;
}

std::optional<ShardResourceManager::ResourceSnapshot> 
ShardResourceManager::getPeerResource(const std::string& shard_id) const {
    std::shared_lock lock(peer_mutex_);
    auto it = peer_resources_.find(shard_id);
    if (it != peer_resources_.end()) {
        return it->second;
    }
    return std::nullopt;
}

/** @brief Return peer ids with health score above healthy threshold. */
std::vector<std::string> ShardResourceManager::getHealthyPeers() const {
    std::shared_lock lock(peer_mutex_);
    std::vector<std::string> healthy_peers;
    
    for (const auto& [shard_id, snapshot] : peer_resources_) {
        if (snapshot.health_score > 50.0f) {
            healthy_peers.push_back(shard_id);
        }
    }
    
    return healthy_peers;
}

/** @brief Return peer ids whose max(cpu,ram) load exceeds threshold. */
std::vector<std::string> ShardResourceManager::getOverloadedPeers(float threshold) const {
    std::shared_lock lock(peer_mutex_);
    std::vector<std::string> overloaded_peers;
    
    for (const auto& [shard_id, snapshot] : peer_resources_) {
        float cpu_ratio = snapshot.cpu_usage_percent / 100.0f;
        float ram_ratio = (snapshot.ram_total_bytes > 0)
            ? static_cast<float>(snapshot.ram_usage_bytes) / snapshot.ram_total_bytes
            : 0.0f;
        
        float max_load = std::max(cpu_ratio, ram_ratio);
        if (max_load >= threshold) {
            overloaded_peers.push_back(shard_id);
        }
    }
    
    return overloaded_peers;
}

// ============================================================================
// Health Scoring
// ============================================================================

/** @brief Compute current local health score from cached snapshot signals. */
float ShardResourceManager::calculateHealthScore() const {
    std::shared_lock lock(local_mutex_);
    return calculateHealthScoreInternal(local_snapshot_);
}

/** @brief Compute health score from supplied snapshot without additional locking. */
float ShardResourceManager::calculateHealthScoreInternal(const ResourceSnapshot& snapshot) const {
    float score = 100.0f;
    
    // CPU usage component (30% weight)
    float cpu_penalty = (snapshot.cpu_usage_percent / 100.0f) * 30.0f;
    score -= cpu_penalty;
    
    // RAM usage component (25% weight)
    if (snapshot.ram_total_bytes > 0) {
        float ram_ratio = static_cast<float>(snapshot.ram_usage_bytes) / 
                          snapshot.ram_total_bytes;
        float ram_penalty = ram_ratio * 25.0f;
        score -= ram_penalty;
    }
    
    // Disk usage component (20% weight)
    uint64_t disk_total = snapshot.disk_used_bytes + snapshot.disk_available_bytes;
    if (disk_total > 0) {
        float disk_ratio = static_cast<float>(snapshot.disk_used_bytes) / disk_total;
        float disk_penalty = disk_ratio * 20.0f;
        score -= disk_penalty;
    }
    
    // Query latency component (15% weight)
    // Assume good latency is < 10ms, bad is > 100ms
    if (snapshot.p99_query_latency_ms > 10.0f) {
        float latency_penalty = std::min(
            (snapshot.p99_query_latency_ms - 10.0f) / 90.0f * 15.0f,
            15.0f
        );
        score -= latency_penalty;
    }
    
    // Pending queries component (10% weight)
    if (snapshot.pending_queries > 0) {
        float pending_penalty = std::min(
            static_cast<float>(snapshot.pending_queries) / 10.0f * 10.0f,
            10.0f
        );
        score -= pending_penalty;
    }
    
    return std::max(0.0f, std::min(100.0f, score));
}

// ============================================================================
// Monitoring Loop
// ============================================================================

/** @brief Periodic monitoring worker loop for sampling and peer upkeep. */
void ShardResourceManager::monitoringLoop() {
    while (running_.load()) {
        collectSystemMetrics();
        cleanupStaleSnapshots();
        
        if (config_.enable_gossip_broadcast) {
            broadcastResourceUpdate();
        }
        
        std::this_thread::sleep_for(
            std::chrono::milliseconds(config_.snapshot_interval_ms)
        );
    }
}

/** @brief Collect current host metrics and refresh local snapshot fields. */
void ShardResourceManager::collectSystemMetrics() {
    std::unique_lock lock(local_mutex_);
    
    local_snapshot_.timestamp = std::chrono::system_clock::now();
    
    // Collect platform-specific metrics
    local_snapshot_.cpu_usage_percent = getCpuUsage();
    
    auto [ram_used, ram_total] = getRamUsage();
    local_snapshot_.ram_usage_bytes = ram_used;
    local_snapshot_.ram_total_bytes = ram_total;
    
    auto [vram_used, vram_total] = getVramUsage();
    local_snapshot_.vram_usage_bytes = vram_used;
    local_snapshot_.vram_total_bytes = vram_total;
    
    auto [disk_used, disk_available] = getDiskUsage();
    local_snapshot_.disk_used_bytes = disk_used;
    local_snapshot_.disk_available_bytes = disk_available;
    
    auto [net_in, net_out] = getNetworkUsage();
    local_snapshot_.network_in_bps = net_in;
    local_snapshot_.network_out_bps = net_out;
    
    // Calculate health score while holding the lock using internal method
    float health_score = calculateHealthScoreInternal(local_snapshot_);
    local_snapshot_.health_score = health_score;
}

/** @brief Drop peer snapshots older than configured cache TTL. */
void ShardResourceManager::cleanupStaleSnapshots() {
    std::unique_lock lock(peer_mutex_);
    
    auto now = std::chrono::system_clock::now();
    auto ttl = std::chrono::milliseconds(config_.peer_cache_ttl_ms);
    
    auto it = peer_resources_.begin();
    while (it != peer_resources_.end()) {
        auto age = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - it->second.timestamp
        );
        
        if (age > ttl) {
            it = peer_resources_.erase(it);
        } else {
            ++it;
        }
    }
}

// ============================================================================
// Platform-Specific Resource Collection
// ============================================================================

/** @brief Sample current CPU utilization percentage via platform-specific APIs. */
float ShardResourceManager::getCpuUsage() const {
#ifdef _WIN32
    // Windows: Use PDH (Performance Data Helper)
    std::lock_guard<std::mutex> lock(pdh_mutex_);
    
    if (!pdh_initialized_) {
        PdhOpenQuery(nullptr, 0, reinterpret_cast<PDH_HQUERY*>(&const_cast<void*&>(pdh_query_)));
        PdhAddCounter(reinterpret_cast<PDH_HQUERY>(pdh_query_), 
                     TEXT("\\Processor(_Total)\\% Processor Time"), 
                     0, 
                     reinterpret_cast<PDH_HCOUNTER*>(&const_cast<void*&>(pdh_counter_)));
        PdhCollectQueryData(reinterpret_cast<PDH_HQUERY>(pdh_query_));
        const_cast<bool&>(pdh_initialized_) = true;
    }
    
    PDH_FMT_COUNTERVALUE value;
    PdhCollectQueryData(reinterpret_cast<PDH_HQUERY>(pdh_query_));
    PdhGetFormattedCounterValue(reinterpret_cast<PDH_HCOUNTER>(pdh_counter_), 
                                PDH_FMT_DOUBLE, nullptr, &value);
    
    return static_cast<float>(value.doubleValue);
#else
    // Linux: Read /proc/stat
    std::lock_guard<std::mutex> lock(cpu_mutex_);
    
    std::ifstream stat_file("/proc/stat");
    if (!stat_file.is_open()) {
        return 0.0f;
    }
    
    std::string line = {};
    std::getline(stat_file, line);
    
    std::istringstream ss(line);
    std::string cpu_label = {};
    uint64_t user, nice, system, idle, iowait, irq, softirq, steal;
    
    ss >> cpu_label >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
    
    // Validate that all fields were successfully parsed
    if (ss.fail()) {
        return 0.0f;
    }
    
    uint64_t total = user + nice + system + idle + iowait + irq + softirq + steal;
    uint64_t total_idle = idle + iowait;
    
    uint64_t diff_total = total - prev_cpu_total_;
    uint64_t diff_idle = total_idle - prev_cpu_idle_;
    
    float usage = 0.0f;
    if (diff_total > 0) {
        usage = 100.0f * (1.0f - static_cast<float>(diff_idle) / diff_total);
    }
    
    const_cast<uint64_t&>(prev_cpu_total_) = total;
    const_cast<uint64_t&>(prev_cpu_idle_) = total_idle;
    
    return usage;
#endif
}

/** @brief Sample RAM usage as pair {used,total} bytes. */
std::pair<uint64_t, uint64_t> ShardResourceManager::getRamUsage() const {
#ifdef _WIN32
    MEMORYSTATUSEX mem_info;
    mem_info.dwLength = sizeof(mem_info);
    GlobalMemoryStatusEx(&mem_info);
    
    uint64_t used = mem_info.ullTotalPhys - mem_info.ullAvailPhys;
    uint64_t total = mem_info.ullTotalPhys;
    
    return {used, total};
#else
    struct sysinfo info;
    if (sysinfo(&info) != 0) {
        return {0, 0};
    }
    
    uint64_t total = info.totalram * info.mem_unit;
    uint64_t free = info.freeram * info.mem_unit;
    uint64_t used = total - free;
    
    return {used, total};
#endif
}

/** @brief Sample VRAM usage as pair {used,total} bytes, or zeros when unavailable. */
std::pair<uint64_t, uint64_t> ShardResourceManager::getVramUsage() const {
#if defined(THEMIS_ENABLE_CUDA)
    size_t free_bytes  = 0;
    size_t total_bytes = 0;
    if (cudaMemGetInfo(&free_bytes, &total_bytes) == cudaSuccess) {
        uint64_t used = static_cast<uint64_t>(total_bytes - free_bytes);
        return {used, static_cast<uint64_t>(total_bytes)};
    }
    return {0, 0};
#elif defined(THEMIS_ENABLE_HIP)
    size_t free_bytes  = 0;
    size_t total_bytes = 0;
    if (hipMemGetInfo(&free_bytes, &total_bytes) == hipSuccess) {
        uint64_t used = static_cast<uint64_t>(total_bytes - free_bytes);
        return {used, static_cast<uint64_t>(total_bytes)};
    }
    return {0, 0};
#else
    // NON-PRODUCTION PATH (Graceful Degradation)
    // Purpose: Satisfies the VRAM usage query API on platforms without a GPU
    //          runtime (no CUDA / HIP available at link time).  This is acceptable
    //          fallback behavior, not a stub that breaks functionality.
    // Activation: Neither THEMIS_ENABLE_CUDA nor THEMIS_ENABLE_HIP defined.
    // Production Delta: Returns (0, 0); resource-aware shard scheduling decisions
    //                   that rely on VRAM headroom will not account for actual GPU
    //                   memory consumption.  This is expected behavior on CPU-only systems.
    // Removal Plan: Enable CUDA or HIP via cmake; the appropriate block above will
    //               activate.  Vulkan (VK_EXT_memory_budget) path deferred.
    //               See src/sharding/FUTURE_ENHANCEMENTS.md §VRAM Usage Monitoring.
    return {0, 0};
#endif
}

/** @brief Sample disk usage as pair {used,available} bytes. */
std::pair<uint64_t, uint64_t> ShardResourceManager::getDiskUsage() const {
#ifdef _WIN32
    ULARGE_INTEGER free_bytes, total_bytes, total_free_bytes;
    if (GetDiskFreeSpaceEx(TEXT("C:\\"), &free_bytes, &total_bytes, &total_free_bytes)) {
        uint64_t used = total_bytes.QuadPart - free_bytes.QuadPart;
        return {used, free_bytes.QuadPart};
    }
    return {0, 0};
#else
    struct statvfs stat;
    if (statvfs("/", &stat) != 0) {
        return {0, 0};
    }
    
    uint64_t total = stat.f_blocks * stat.f_frsize;
    uint64_t available = stat.f_bavail * stat.f_frsize;
    uint64_t used = total - available;
    
    return {used, available};
#endif
}

/** @brief Sample network counters/rates as pair {in,out}. */
std::pair<uint64_t, uint64_t> ShardResourceManager::getNetworkUsage() const {
#ifdef _WIN32
    // Windows: GetIfTable2 / Performance Counter integration deferred.
    // Returns (0, 0) on Windows until iphlpapi-based counters are wired in.
    return {0, 0};
#else
    // Linux: aggregate rx_bytes and tx_bytes across all interfaces from
    // /proc/net/dev.  Format (after two header lines):
    //   <iface>: rx_bytes rx_pkts rx_errs rx_drop … tx_bytes tx_pkts …
    std::ifstream net_dev("/proc/net/dev");
    if (!net_dev.is_open()) {
        return {0, 0};
    }
    uint64_t total_rx = 0, total_tx = 0;
    std::string line = {};
    int line_num = 0;
    while (std::getline(net_dev, line)) {
        if (++line_num <= 2) continue; // skip the two header lines
        const auto colon = line.find(':');
        if (colon == std::string::npos) {
          continue;
        }
        std::istringstream iss(line.substr(colon + 1));
        uint64_t rx = 0, pkts = 0, errs = 0, drop = 0,
                 fifo = 0, frame = 0, comp = 0, mcast = 0, tx = 0;
        if (iss >> rx >> pkts >> errs >> drop >> fifo >> frame >> comp >> mcast >> tx) {
            total_rx += rx;
            total_tx += tx;
        }
    }
    return {total_rx, total_tx};
#endif
}

} // namespace themis::sharding
