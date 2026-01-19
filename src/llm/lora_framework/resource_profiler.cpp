#include "llm/lora_framework/resource_profiler.h"
#include "llm/lora_framework/gpu_memory.h"
#include <spdlog/spdlog.h>
#include <fstream>
#include <numeric>
#include <mutex>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <sys/sysinfo.h>
#include <unistd.h>
#endif

namespace themis {
namespace llm {
namespace lora {

// ===== ResourceProfiler::Impl =====

class ResourceProfiler::Impl {
public:
    Config config;
    bool running = false;
    std::vector<ResourceSnapshot> snapshots;
    std::chrono::system_clock::time_point start_time;
    std::vector<ResourceMonitorCallback> callbacks;
    std::mutex mutex;
    
    // Throughput tracking
    int last_step = 0;
    std::chrono::system_clock::time_point last_snapshot_time;
    
    Impl(const Config& cfg) : config(cfg) {}
};

// ===== ResourceProfiler Implementation =====

ResourceProfiler::ResourceProfiler(const Config& config)
    : impl_(std::make_unique<Impl>(config)) {
    spdlog::info("ResourceProfiler initialized");
    spdlog::info("  Snapshot interval: {} steps", config.snapshot_interval_steps);
    spdlog::info("  Log to file: {}", config.log_to_file);
    if (config.log_to_file) {
        spdlog::info("  Log file: {}", config.log_file);
    }
}

ResourceProfiler::~ResourceProfiler() {
    if (impl_->running) {
        stop();
    }
}

void ResourceProfiler::start() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    if (impl_->running) {
        spdlog::warn("ResourceProfiler already running");
        return;
    }
    
    impl_->running = true;
    impl_->start_time = std::chrono::system_clock::now();
    impl_->last_snapshot_time = impl_->start_time;
    impl_->last_step = 0;
    impl_->snapshots.clear();
    
    spdlog::info("ResourceProfiler started");
}

void ResourceProfiler::stop() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    if (!impl_->running) {
        return;
    }
    
    impl_->running = false;
    
    // Compute and log final statistics
    auto stats = compute_stats();
    spdlog::info("ResourceProfiler stopped");
    spdlog::info("  Training time: {} seconds", stats.total_training_time.count());
    spdlog::info("  Peak GPU memory: {:.2f} GB", stats.peak_gpu_memory / (1024.0 * 1024.0 * 1024.0));
    spdlog::info("  Avg GPU utilization: {:.1f}%", stats.avg_gpu_utilization);
    spdlog::info("  Avg throughput: {:.1f} samples/s", stats.avg_samples_per_second);
    
    // Export to file if configured
    if (impl_->config.log_to_file) {
        export_to_json(impl_->config.log_file);
    }
}

void ResourceProfiler::snapshot(int epoch, int step, float loss, float lr) {
    if (!impl_->running || !impl_->config.enabled) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    // Check if we should take a snapshot
    if (step % impl_->config.snapshot_interval_steps != 0) {
        return;
    }
    
    ResourceSnapshot snapshot;
    snapshot.timestamp = std::chrono::system_clock::now();
    snapshot.current_epoch = epoch;
    snapshot.current_step = step;
    snapshot.current_loss = loss;
    snapshot.learning_rate = lr;
    
    // Query resource usage
    query_gpu_memory(snapshot);
    query_cpu_memory(snapshot);
    query_gpu_utilization(snapshot);
    
    // Compute throughput
    auto time_delta = std::chrono::duration_cast<std::chrono::milliseconds>(
        snapshot.timestamp - impl_->last_snapshot_time).count();
    
    if (time_delta > 0 && step > impl_->last_step) {
        int step_delta = step - impl_->last_step;
        snapshot.samples_per_second = (step_delta * 1000.0f) / time_delta;
        // Assume average of 512 tokens per sample
        snapshot.tokens_per_second = snapshot.samples_per_second * 512.0f;
    }
    
    impl_->last_step = step;
    impl_->last_snapshot_time = snapshot.timestamp;
    
    // Store snapshot
    impl_->snapshots.push_back(snapshot);
    
    // Check alerts
    if (impl_->config.enable_alerts) {
        check_alerts(snapshot);
    }
    
    // Log to file
    if (impl_->config.log_to_file) {
        log_snapshot(snapshot);
    }
    
    // Call callbacks
    for (const auto& callback : impl_->callbacks) {
        try {
            callback(snapshot);
        } catch (const std::exception& e) {
            spdlog::error("Callback error: {}", e.what());
        }
    }
    
    // Verbose logging
    if (impl_->config.verbose_logging) {
        spdlog::info("Resource snapshot [step {}]:", step);
        spdlog::info("  GPU memory: {:.2f}/{:.2f} GB ({:.1f}%)",
            snapshot.gpu_memory_allocated / (1024.0 * 1024.0 * 1024.0),
            snapshot.gpu_memory_total / (1024.0 * 1024.0 * 1024.0),
            snapshot.gpu_memory_utilization);
        spdlog::info("  GPU utilization: {:.1f}%", snapshot.gpu_utilization);
        spdlog::info("  Throughput: {:.1f} samples/s", snapshot.samples_per_second);
    }
}

ResourceSnapshot ResourceProfiler::get_current_snapshot() const {
    ResourceSnapshot snapshot;
    snapshot.timestamp = std::chrono::system_clock::now();
    
    query_gpu_memory(snapshot);
    query_cpu_memory(snapshot);
    query_gpu_utilization(snapshot);
    
    return snapshot;
}

std::vector<ResourceSnapshot> ResourceProfiler::get_snapshots() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->snapshots;
}

ResourceStats ResourceProfiler::compute_stats() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    ResourceStats stats;
    stats.num_snapshots = impl_->snapshots.size();
    
    if (impl_->snapshots.empty()) {
        return stats;
    }
    
    // Compute peaks
    for (const auto& snapshot : impl_->snapshots) {
        stats.peak_gpu_memory = std::max(stats.peak_gpu_memory, snapshot.gpu_memory_allocated);
        stats.peak_cpu_memory = std::max(stats.peak_cpu_memory, snapshot.cpu_memory_used);
    }
    
    // Compute averages
    float sum_gpu_util = 0.0f;
    float sum_gpu_mem_util = 0.0f;
    float sum_samples_per_sec = 0.0f;
    float sum_tokens_per_sec = 0.0f;
    
    for (const auto& snapshot : impl_->snapshots) {
        sum_gpu_util += snapshot.gpu_utilization;
        sum_gpu_mem_util += snapshot.gpu_memory_utilization;
        sum_samples_per_sec += snapshot.samples_per_second;
        sum_tokens_per_sec += snapshot.tokens_per_second;
    }
    
    size_t n = impl_->snapshots.size();
    stats.avg_gpu_utilization = sum_gpu_util / n;
    stats.avg_gpu_memory_utilization = sum_gpu_mem_util / n;
    stats.avg_samples_per_second = sum_samples_per_sec / n;
    stats.avg_tokens_per_second = sum_tokens_per_sec / n;
    
    // Compute training time
    if (impl_->running) {
        auto now = std::chrono::system_clock::now();
        stats.total_training_time = std::chrono::duration_cast<std::chrono::seconds>(
            now - impl_->start_time);
    } else if (!impl_->snapshots.empty()) {
        stats.total_training_time = std::chrono::duration_cast<std::chrono::seconds>(
            impl_->snapshots.back().timestamp - impl_->start_time);
    }
    
    return stats;
}

void ResourceProfiler::register_callback(ResourceMonitorCallback callback) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->callbacks.push_back(callback);
}

void ResourceProfiler::clear() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->snapshots.clear();
}

bool ResourceProfiler::is_running() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->running;
}

ResourceProfiler::Config ResourceProfiler::get_config() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->config;
}

void ResourceProfiler::set_config(const Config& config) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->config = config;
}

void ResourceProfiler::export_to_json(const std::string& filename) const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    std::ofstream file(filename);
    if (!file.is_open()) {
        spdlog::error("Failed to open file for writing: {}", filename);
        return;
    }
    
    // Write snapshots as JSONL (one JSON per line)
    for (const auto& snapshot : impl_->snapshots) {
        file << snapshot.toJSON().dump() << "\n";
    }
    
    spdlog::info("Exported {} snapshots to {}", impl_->snapshots.size(), filename);
}

size_t ResourceProfiler::get_peak_gpu_memory() const {
    auto stats = compute_stats();
    return stats.peak_gpu_memory;
}

size_t ResourceProfiler::get_peak_cpu_memory() const {
    auto stats = compute_stats();
    return stats.peak_cpu_memory;
}

void ResourceProfiler::query_gpu_memory(ResourceSnapshot& snapshot) const {
    try {
        // Try to get GPU memory from GPUMemoryManager
        auto backends = GPUMemoryManager::detect_backends();
        if (!backends.empty()) {
            auto stats = GPUMemoryManager::get_memory_stats();
            snapshot.gpu_memory_allocated = stats.allocated_bytes;
            snapshot.gpu_memory_reserved = stats.reserved_bytes;
            snapshot.gpu_memory_total = stats.total_bytes;
            snapshot.gpu_memory_free = stats.total_bytes - stats.allocated_bytes;
            
            if (stats.total_bytes > 0) {
                snapshot.gpu_memory_utilization = 
                    100.0f * stats.allocated_bytes / stats.total_bytes;
            }
        }
    } catch (const std::exception& e) {
        // GPU queries may fail if no GPU available
        spdlog::debug("GPU memory query failed: {}", e.what());
    }
}

void ResourceProfiler::query_cpu_memory(ResourceSnapshot& snapshot) const {
#ifdef _WIN32
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        snapshot.cpu_memory_available = memInfo.ullAvailPhys;
        snapshot.cpu_memory_used = memInfo.ullTotalPhys - memInfo.ullAvailPhys;
    }
#else
    struct sysinfo memInfo;
    if (sysinfo(&memInfo) == 0) {
        snapshot.cpu_memory_available = memInfo.freeram * memInfo.mem_unit;
        snapshot.cpu_memory_used = (memInfo.totalram - memInfo.freeram) * memInfo.mem_unit;
    }
#endif
}

void ResourceProfiler::query_gpu_utilization(ResourceSnapshot& snapshot) const {
    // TODO: Implement GPU utilization query
    // This would require NVML (NVIDIA), ROCm SMI (AMD), or other vendor APIs
    // For now, leave at 0
    snapshot.gpu_utilization = 0.0f;
}

void ResourceProfiler::check_alerts(const ResourceSnapshot& snapshot) {
    // Check GPU memory threshold
    if (snapshot.gpu_memory_utilization > impl_->config.gpu_memory_alert_threshold * 100) {
        spdlog::warn("GPU memory usage high: {:.1f}% (threshold: {:.1f}%)",
            snapshot.gpu_memory_utilization,
            impl_->config.gpu_memory_alert_threshold * 100);
    }
    
    // Check GPU utilization threshold
    if (snapshot.gpu_utilization > impl_->config.gpu_utilization_alert_threshold * 100) {
        spdlog::warn("GPU utilization high: {:.1f}% (threshold: {:.1f}%)",
            snapshot.gpu_utilization,
            impl_->config.gpu_utilization_alert_threshold * 100);
    }
}

void ResourceProfiler::log_snapshot(const ResourceSnapshot& snapshot) {
    // Append to file in JSONL format
    std::ofstream file(impl_->config.log_file, std::ios::app);
    if (file.is_open()) {
        file << snapshot.toJSON().dump() << "\n";
    }
}

} // namespace lora
} // namespace llm
} // namespace themis
