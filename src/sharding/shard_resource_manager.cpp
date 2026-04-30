/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            shard_resource_manager.cpp                         ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:50:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     625                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 6897bb74a5  2026-04-13  docs(aql): Close all remaining ROADMAP items — Doxygen, L... ║
    • e8953e1175  2026-04-13  docs(aql): Close all remaining ROADMAP items — Doxygen, L... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "sharding/shard_resource_manager.h"
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

ShardResourceManager::ResourceSnapshot ShardResourceManager::ResourceSnapshot::fromJson(const nlohmann::json& j) {
    ResourceSnapshot snapshot;
    
    snapshot.cpu_usage_percent = j.value("cpu_usage_percent", 0.0f);
    snapshot.ram_usage_bytes = j.value("ram_usage_bytes", 0ULL);
    snapshot.ram_total_bytes = j.value("ram_total_bytes", 0ULL);
    snapshot.vram_usage_bytes = j.value("vram_usage_bytes", 0ULL);
    snapshot.vram_total_bytes = j.value("vram_total_bytes", 0ULL);
    snapshot.disk_used_bytes = j.value("disk_used_bytes", 0ULL);
    snapshot.disk_available_bytes = j.value("disk_available_bytes", 0ULL);
    snapshot.network_in_bps = j.value("network_in_bps", 0ULL);
    snapshot.network_out_bps = j.value("network_out_bps", 0ULL);
    snapshot.active_queries = j.value("active_queries", 0U);
    snapshot.pending_queries = j.value("pending_queries", 0U);
    snapshot.active_transactions = j.value("active_transactions", 0U);
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

ShardResourceManager::ShardResourceManager(
    const std::string& local_shard_id,
    std::shared_ptr<GossipConfigManager> gossip_manager)
    : ShardResourceManager(local_shard_id, gossip_manager, Config{})
{
}

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

void ShardResourceManager::start() {
    if (running_.exchange(true)) {
        return; // Already running
    }
    
    monitoring_thread_ = std::thread([this]() {
        monitoringLoop();
    });
}

void ShardResourceManager::stop() {
    if (!running_.exchange(false)) {
        return; // Not running
    }
    
    if (monitoring_thread_.joinable()) {
        monitoring_thread_.join();
    }
}

// ============================================================================
// Local Resource Management
// ============================================================================

ShardResourceManager::ResourceSnapshot ShardResourceManager::getCurrentSnapshot() const {
    std::shared_lock lock(local_mutex_);
    return local_snapshot_;
}

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

void ShardResourceManager::updateQueryMetrics(uint32_t active, uint32_t pending, float avg_latency_ms) {
    std::unique_lock lock(local_mutex_);
    local_snapshot_.active_queries = active;
    local_snapshot_.pending_queries = pending;
    local_snapshot_.avg_query_latency_ms = avg_latency_ms;
}

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

bool ShardResourceManager::acquireRepairIOToken(double io_ops) {
    if (!config_.enable_repair_iops_throttle || !repair_io_limiter_) {
        return true;
    }
    return repair_io_limiter_->try_acquire(io_ops);
}

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
    gossip_snapshot.available_cpu_cores = std::max(0U, 
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

float ShardResourceManager::calculateHealthScore() const {
    std::shared_lock lock(local_mutex_);
    return calculateHealthScoreInternal(local_snapshot_);
}

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
    
    std::string line;
    std::getline(stat_file, line);
    
    std::istringstream ss(line);
    std::string cpu_label;
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

std::pair<uint64_t, uint64_t> ShardResourceManager::getVramUsage() const {
    // STUB/SIMULATION NOTE:
    // Purpose: Satisfies the VRAM usage query API on platforms without a GPU
    //          runtime (no CUDA / HIP / Vulkan available at link time).
    // Activation: Always — no GPU API is queried.
    // Production Delta: Returns (0, 0); resource-aware shard scheduling decisions
    //                   that rely on VRAM headroom will not account for actual GPU
    //                   memory consumption.
    // Removal Plan: Add CUDA (nvmlDeviceGetMemoryInfo) / HIP (hipMemGetInfo) /
    //               Vulkan (VK_EXT_memory_budget) backends, guarded by
    //               THEMIS_ENABLE_CUDA / THEMIS_ENABLE_HIP / THEMIS_ENABLE_VULKAN.
    //               See src/sharding/FUTURE_ENHANCEMENTS.md §VRAM Usage Monitoring.
    return {0, 0};
}

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

std::pair<uint64_t, uint64_t> ShardResourceManager::getNetworkUsage() const {
    // STUB/SIMULATION NOTE:
    // Purpose: Satisfies the network I/O usage API while platform-specific
    //          counters are not yet integrated.
    // Activation: Always — no platform API is queried.
    // Production Delta: Returns (0, 0); network-bandwidth-aware shard routing
    //                   decisions will not account for actual NIC utilisation.
    // Removal Plan: On Linux parse /proc/net/dev (rx/tx bytes); on Windows use
    //               `GetIfTable2()` / Performance Counters.  Guard per-platform
    //               behind compile-time detection or runtime feature flags.
    //               See src/sharding/FUTURE_ENHANCEMENTS.md §Network Usage Monitoring.
    return {0, 0};
}

} // namespace themis::sharding
