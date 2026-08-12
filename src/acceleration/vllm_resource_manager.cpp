/**
 * @file vllm_resource_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=2, Sim=0, Debt=0, C=2, H=3, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "acceleration/vllm_resource_manager.h"
#include "acceleration/cpu_backend.h"

#include <algorithm>
#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <thread>

#include "utils/logger.h"

#ifdef __linux__
#include <fstream>
#include <inttypes.h>
#include <string>
#endif

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#ifdef THEMIS_ENABLE_CUDA
#include "acceleration/cuda_backend.h"
#include <cuda_runtime.h>
#ifdef __linux__
#include <nvml.h>
#endif
#endif

namespace themis {
namespace acceleration {

// CPU snapshot cache TTL: refresh interval between blocking two-snapshot reads.
// Calls within this window reuse the last snapshot as the base, avoiding sleep.
static constexpr std::chrono::milliseconds kCpuCacheTTL{200};

namespace {

VLLMResourceManager::SimilarityDispatchResult runSimilarityDispatch(const ANNKernelDispatch &dispatch, bool mark_gpu,
                                                                    const float *queries, size_t num_queries, size_t dim,
                                                                    const float *vectors, size_t num_vectors, size_t top_k,
                                                                    DistanceMetric metric) {
    VLLMResourceManager::SimilarityDispatchResult out;
    out.used_gpu = mark_gpu;

    if (queries == nullptr || vectors == nullptr) {
        out.error = "queries and vectors pointers must be non-null";
        return out;
    }
    if (num_queries == 0 || num_vectors == 0 || dim == 0 || top_k == 0) {
        out.error = "num_queries, num_vectors, dim, and top_k must all be > 0";
        return out;
    }

    ANNDistanceFn distance_launcher = dispatch.distanceLauncherFor(metric);
    if (distance_launcher == nullptr || dispatch.launchTopK == nullptr) {
        out.error = "ANN dispatch table missing required launcher(s)";
        return out;
    }

    const size_t effective_k = std::min(top_k, num_vectors);
    std::vector<float> distance_matrix(num_queries * num_vectors);
    out.topk_indices.resize(num_queries * effective_k);
    out.topk_distances.resize(num_queries * effective_k);

    const int distance_rc = distance_launcher(queries, vectors, distance_matrix.data(), static_cast<int>(num_queries),
                                              static_cast<int>(num_vectors), static_cast<int>(dim), nullptr);
    if (distance_rc != 0) {
        out.error = "distance kernel failed with code " + std::to_string(distance_rc);
        return out;
    }

#ifdef THEMIS_ENABLE_CUDA
    if (mark_gpu) {
        const cudaError_t distance_cuda_error = cudaGetLastError();
        if (distance_cuda_error != cudaSuccess) {
            out.error = std::string("distance kernel cudaGetLastError: ") + cudaGetErrorString(distance_cuda_error);
            return out;
        }
    }
#endif

    const int topk_rc = dispatch.launchTopK(distance_matrix.data(), out.topk_indices.data(), out.topk_distances.data(),
                                            static_cast<int>(num_queries), static_cast<int>(num_vectors),
                                            static_cast<int>(effective_k), nullptr);
    if (topk_rc != 0) {
        out.error = "top-k kernel failed with code " + std::to_string(topk_rc);
        return out;
    }

#ifdef THEMIS_ENABLE_CUDA
    if (mark_gpu) {
        const cudaError_t topk_cuda_error = cudaGetLastError();
        if (topk_cuda_error != cudaSuccess) {
            out.error = std::string("top-k kernel cudaGetLastError: ") + cudaGetErrorString(topk_cuda_error);
            return out;
        }
    }
#endif

    out.success = true;
    return out;
}

} // namespace

VLLMResourceManager::VLLMResourceManager(const Config &config) : config_(config) {}

VLLMResourceManager::~VLLMResourceManager() {
    shutdown();
}

bool VLLMResourceManager::initialize() {
    if (initialized_) {
        THEMIS_WARN("VLLMResourceManager already initialized");
        return true;
    }

    // Detect system resources
    config_.total_cpu_cores = std::thread::hardware_concurrency();

    // Calculate ThemisDB allocation (remaining after vLLM)
    config_.themis_cpu_cores = config_.total_cpu_cores - config_.vllm_cpu_cores;
    config_.themis_ram_gb    = config_.total_ram_gb - config_.vllm_ram_gb;

    THEMIS_INFO("VLLMResourceManager initialized:");
    THEMIS_INFO("  System: {} CPU cores, {} GB RAM", config_.total_cpu_cores, config_.total_ram_gb);
    THEMIS_INFO("  vLLM reservation: {} cores, {} GB RAM", config_.vllm_cpu_cores, config_.vllm_ram_gb);
    THEMIS_INFO("  ThemisDB allocation: {} cores, {} GB RAM", config_.themis_cpu_cores, config_.themis_ram_gb);

    // Initialize NVML for GPU monitoring
#ifdef THEMIS_ENABLE_CUDA
    if (!initializeNVML()) {
        THEMIS_WARN("NVML initialization failed - GPU monitoring disabled");
    }
#else
    THEMIS_INFO("CUDA not enabled - GPU monitoring disabled");
#endif

    initialized_ = true;
    return true;
}

void VLLMResourceManager::shutdown() {
    if (!initialized_) {
        return;
    }

#ifdef THEMIS_ENABLE_CUDA
    shutdownNVML();
#endif

    initialized_ = false;
    THEMIS_INFO("VLLMResourceManager shutdown");
}

bool VLLMResourceManager::canUseGPU() {
#ifndef THEMIS_ENABLE_CUDA
    // Test override: allows CI tests to verify GPU-busy logic without real CUDA.
    if (gpu_util_provider_for_testing_) {
        auto util = gpu_util_provider_for_testing_();
        if (!util.has_value()) {
            return false;
        }

        return util.value() < 80.0;
    }
    return false; // CUDA not enabled
#else
    // Test override: bypasses NVML for CI/mock environments.
    if (gpu_util_provider_for_testing_) {
        auto util = gpu_util_provider_for_testing_();
        if (!util.has_value())
            return false;
        return util.value() < 80.0;
    }

    // Wrap the NVML query in a background future with a 500 ms deadline.
    // If the NVML driver is wedged the query can hang indefinitely; returning
    // false (safe CPU fallback) is preferable to blocking the caller.
    //
    // Safety: the future captures a copy of nvml_devices_ (vector of void*)
    // rather than `this`, so the background task cannot dereference a destroyed
    // VLLMResourceManager if the timeout fires.  The shared ownership means no
    // use-after-free is possible.
    std::vector<void *> device_handles = nvml_devices_;
    if (device_handles.empty()) {
        return false; // NVML not initialized
    }

    auto shared_future = std::make_shared<std::future<std::optional<double>>>(
        std::async(std::launch::async, [device_handles]() -> std::optional<double> {
#if defined(THEMIS_ENABLE_CUDA) && defined(__linux__)
            // Return max utilization across all monitored devices so
            // that a single busy GPU blocks new ThemisDB work.
            double max_util = 0.0;
            bool got_any    = false;
            for (void *handle : device_handles) {
                nvmlUtilization_t util;
                nvmlDevice_t dev = static_cast<nvmlDevice_t>(handle);
                if (nvmlDeviceGetUtilizationRates(dev, &util) == NVML_SUCCESS) {
                    double u = static_cast<double>(util.gpu);
                    max_util = std::max(max_util, u);
                    got_any  = true;
                }
            }
            return got_any ? std::optional<double>{max_util} : std::nullopt;
#endif
            return std::nullopt;
        }));

    std::optional<double> gpu_util;
    if (shared_future->wait_for(std::chrono::milliseconds(500)) == std::future_status::ready) {
        gpu_util = shared_future->get();
    } else {
        // NVML query timed out — assume GPU busy, fall back to CPU.
        // The background task keeps running in the shared_ptr-owned future;
        // it will complete on its own without accessing this object.
        THEMIS_WARN("GPU utilization query timed out (>500 ms) — using CPU fallback");
        return false;
    }

    if (!gpu_util.has_value()) {
        // Can't query GPU - assume busy (safe fallback to CPU)
        return false;
    }

    // Only use GPU if vLLM is not heavily utilizing it (< 80%)
    bool can_use = gpu_util.value() < 80.0;

    if (!can_use) {
        THEMIS_DEBUG("GPU busy ({}% utilization) - using CPU fallback", gpu_util.value());
    }

    return can_use;
#endif
}

VLLMResourceManager::SimilarityDispatchResult VLLMResourceManager::dispatchVectorSimilarity(
    const float *queries, size_t num_queries, size_t dim, const float *vectors, size_t num_vectors, size_t top_k,
    DistanceMetric metric) {
    if (!initialized_) {
        SimilarityDispatchResult out;
        out.error = "VLLMResourceManager not initialized";
        return out;
    }

#ifdef THEMIS_ENABLE_CUDA
    if (canUseGPU()) {
        CUDAVectorBackend cuda_backend;
        if (cuda_backend.initialize()) {
            SimilarityDispatchResult gpu_result = runSimilarityDispatch(cuda_backend.populateANNDispatch(), true, queries,
                                                                        num_queries, dim, vectors, num_vectors, top_k, metric);
            if (gpu_result.success) {
                return gpu_result;
            }
            THEMIS_WARN("VLLMResourceManager: CUDA vector similarity failed: {} — using CPU fallback", gpu_result.error);
        } else {
            THEMIS_WARN("VLLMResourceManager: CUDA backend init failed — using CPU fallback");
        }
    } else {
        THEMIS_DEBUG("VLLMResourceManager: canUseGPU() blocked GPU dispatch — using CPU fallback");
    }
#endif

    CPUVectorBackend cpu_backend;
    (void)cpu_backend.initialize();
    SimilarityDispatchResult cpu_result
        = runSimilarityDispatch(cpu_backend.populateANNDispatch(), false, queries, num_queries, dim, vectors, num_vectors,
                                top_k, metric);
    if (!cpu_result.success) {
        THEMIS_WARN("VLLMResourceManager: CPU fallback vector similarity failed: {}", cpu_result.error);
    }
    return cpu_result;
}

size_t VLLMResourceManager::getRecommendedThreadCount(const std::string &operation_type) const {
    if (!initialized_) {
        return std::thread::hardware_concurrency();
    }

    if (operation_type == "rocksdb") {
        return static_cast<size_t>(config_.themis_cpu_cores * config_.rocksdb_thread_ratio);
    } else if (operation_type == "tbb") {
        return static_cast<size_t>(config_.themis_cpu_cores * config_.tbb_thread_ratio);
    } else {
        // General purpose - use TBB allocation
        return static_cast<size_t>(config_.themis_cpu_cores * config_.tbb_thread_ratio);
    }
}

VLLMResourceManager::Stats VLLMResourceManager::getStats() const {
    Stats stats;

    if (!initialized_) {
        return stats;
    }

    // CPU stats
    stats.active_threads = config_.themis_cpu_cores;

#if defined(__linux__)
    // Linux CPU utilization: two /proc/stat snapshots.
    // If a fresh cached snapshot exists (< 200 ms old) it is used as the base
    // (t0) to avoid a blocking 100 ms sleep on rapid successive calls.
    // Format of line 1: "cpu  user nice system idle iowait irq softirq steal ..."
    auto readCpuTimes = [](uint64_t &total, uint64_t &idle) -> bool {
        std::ifstream f("/proc/stat");
        if (!f.is_open())
            return false;
        std::string tag;
        uint64_t user, nice, system, idle_val, iowait, irq, softirq, steal;
        f >> tag >> user >> nice >> system >> idle_val >> iowait >> irq >> softirq >> steal;
        if (tag != "cpu")
            return false;
        idle  = idle_val + iowait;
        total = user + nice + system + idle_val + iowait + irq + softirq + steal;
        return true;
    };

    const auto now = std::chrono::steady_clock::now();
    uint64_t t0 = 0, i0 = 0;
    bool have_t0 = false;
    {
        std::lock_guard<std::mutex> lock(cpu_cache_mutex_);
        if (cpu_snapshot_cache_.valid && (now - cpu_snapshot_cache_.ts) < kCpuCacheTTL) {
            t0      = cpu_snapshot_cache_.v0;
            i0      = cpu_snapshot_cache_.v1;
            have_t0 = true;
        }
    }
    if (!have_t0) {
        have_t0 = readCpuTimes(t0, i0);
        if (have_t0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    if (have_t0) {
        uint64_t t1 = 0, i1 = 0;
        if (readCpuTimes(t1, i1)) {
            uint64_t dtotal = (t1 > t0) ? (t1 - t0) : 0;
            if (dtotal == 0) {
                // Same jiffy — counters haven't advanced yet.
                // Reuse the last computed utilization to avoid returning 0.0.
                // Still refresh the baseline (v0/v1/ts) so the next call uses
                // the current read as its starting point and avoids stale deltas.
                std::lock_guard<std::mutex> lock(cpu_cache_mutex_);
                if (cpu_snapshot_cache_.valid) {
                    stats.cpu_utilization = cpu_snapshot_cache_.last_cpu_util;
                }
                cpu_snapshot_cache_.v0 = t1;
                cpu_snapshot_cache_.v1 = i1;
                cpu_snapshot_cache_.ts = std::chrono::steady_clock::now();
                // last_cpu_util and valid are unchanged
            } else {
                uint64_t didle        = (i1 > i0) ? (i1 - i0) : 0;
                stats.cpu_utilization = 100.0 * (1.0 - static_cast<double>(didle) / static_cast<double>(dtotal));
                std::lock_guard<std::mutex> lock(cpu_cache_mutex_);
                cpu_snapshot_cache_.v0            = t1;
                cpu_snapshot_cache_.v1            = i1;
                cpu_snapshot_cache_.last_cpu_util = stats.cpu_utilization;
                cpu_snapshot_cache_.ts            = std::chrono::steady_clock::now();
                cpu_snapshot_cache_.valid         = true;
            }
        }
    }

    // Linux RAM: /proc/meminfo  (MemTotal and MemAvailable in kB)
    {
        std::ifstream mf("/proc/meminfo");
        uint64_t mem_total_kb = 0;
        uint64_t mem_avail_kb = 0;
        std::string line;
        while (std::getline(mf, line) && (mem_total_kb == 0 || mem_avail_kb == 0)) {
            if (line.rfind("MemTotal:", 0) == 0) {
                sscanf(line.c_str(), "MemTotal: %" SCNu64 " kB", &mem_total_kb);
            } else if (line.rfind("MemAvailable:", 0) == 0) {
                sscanf(line.c_str(), "MemAvailable: %" SCNu64 " kB", &mem_avail_kb);
            }
        }
        if (mem_total_kb > 0) {
            uint64_t used_kb      = (mem_total_kb > mem_avail_kb) ? (mem_total_kb - mem_avail_kb) : 0;
            stats.ram_used_mb     = used_kb / 1024u;
            stats.ram_utilization = 100.0 * static_cast<double>(used_kb) / static_cast<double>(mem_total_kb);
        }
    }

#elif defined(_WIN32)
    // Windows CPU utilization: delta of GetSystemTimes().
    // If a fresh cached snapshot exists (< 200 ms old) it is used as the base
    // to avoid a blocking 100 ms sleep on rapid successive calls.
    {
        const auto now_win = std::chrono::steady_clock::now();

        auto ft2u64 = [](const FILETIME &ft) -> uint64_t {
            return (static_cast<uint64_t>(ft.dwHighDateTime) << 32) | static_cast<uint64_t>(ft.dwLowDateTime);
        };

        uint64_t base_idle = 0, base_kernel = 0, base_user = 0;
        bool have_base = false;
        {
            std::lock_guard<std::mutex> lock(cpu_cache_mutex_);
            if (cpu_snapshot_cache_.valid && (now_win - cpu_snapshot_cache_.ts) < kCpuCacheTTL) {
                base_idle   = cpu_snapshot_cache_.v0;
                base_kernel = cpu_snapshot_cache_.v1;
                base_user   = cpu_snapshot_cache_.v2;
                have_base   = true;
            }
        }
        if (!have_base) {
            FILETIME idle0, kernel0, user0;
            if (GetSystemTimes(&idle0, &kernel0, &user0)) {
                base_idle   = ft2u64(idle0);
                base_kernel = ft2u64(kernel0);
                base_user   = ft2u64(user0);
                have_base   = true;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        if (have_base) {
            FILETIME idle1, kernel1, user1;
            if (GetSystemTimes(&idle1, &kernel1, &user1)) {
                uint64_t idle1v   = ft2u64(idle1);
                uint64_t kernel1v = ft2u64(kernel1);
                uint64_t user1v   = ft2u64(user1);
                uint64_t idle     = (idle1v > base_idle) ? (idle1v - base_idle) : 0;
                uint64_t kernel   = (kernel1v > base_kernel) ? (kernel1v - base_kernel) : 0;
                uint64_t user     = (user1v > base_user) ? (user1v - base_user) : 0;
                uint64_t total    = kernel + user; // kernel already includes idle
                if (total == 0) {
                    // Same resolution tick — reuse last computed utilization.
                    // Still refresh the baseline (v0/v1/v2/ts) to avoid stale deltas
                    // on the next call.
                    std::lock_guard<std::mutex> lock(cpu_cache_mutex_);
                    if (cpu_snapshot_cache_.valid) {
                        stats.cpu_utilization = cpu_snapshot_cache_.last_cpu_util;
                    }
                    cpu_snapshot_cache_.v0 = idle1v;
                    cpu_snapshot_cache_.v1 = kernel1v;
                    cpu_snapshot_cache_.v2 = user1v;
                    cpu_snapshot_cache_.ts = std::chrono::steady_clock::now();
                    // last_cpu_util and valid are unchanged
                } else {
                    stats.cpu_utilization = 100.0 * (1.0 - static_cast<double>(idle) / static_cast<double>(total));
                    std::lock_guard<std::mutex> lock(cpu_cache_mutex_);
                    cpu_snapshot_cache_.v0            = idle1v;
                    cpu_snapshot_cache_.v1            = kernel1v;
                    cpu_snapshot_cache_.v2            = user1v;
                    cpu_snapshot_cache_.last_cpu_util = stats.cpu_utilization;
                    cpu_snapshot_cache_.ts            = std::chrono::steady_clock::now();
                    cpu_snapshot_cache_.valid         = true;
                }
            }
        }
    }

    // Windows RAM: GlobalMemoryStatusEx
    {
        MEMORYSTATUSEX ms{};
        ms.dwLength = sizeof(ms);
        if (GlobalMemoryStatusEx(&ms)) {
            stats.ram_utilization = static_cast<double>(ms.dwMemoryLoad);
            uint64_t used         = ms.ullTotalPhys - ms.ullAvailPhys;
            stats.ram_used_mb     = static_cast<size_t>(used / (1024u * 1024u));
        }
    }
#endif
    // macOS / unknown: cpu_utilization and ram_used_mb remain 0.0 / 0.

    // GPU stats via NVML (or test provider).
    // queryGPUUtilization() returns nullopt when neither the test provider nor
    // a real NVML device is available, so this is safe to call unconditionally.
    auto gpu_util = const_cast<VLLMResourceManager *>(this)->queryGPUUtilization();
    if (gpu_util.has_value()) {
        stats.gpu_available   = true;
        stats.gpu_utilization = gpu_util.value();

        // Estimate vLLM usage (anything > 20% is likely vLLM)
        if (stats.gpu_utilization > 20.0) {
            stats.vllm_detected  = true;
            stats.vllm_gpu_usage = stats.gpu_utilization;
        }
    }

    return stats;
}

void VLLMResourceManager::setConfig(const Config &config) {
    if (initialized_) {
        THEMIS_WARN("Cannot change config while initialized - call shutdown() first");
        return;
    }
    config_ = config;
}

void VLLMResourceManager::setGpuUtilizationProviderForTesting(std::function<std::optional<double>()> provider) {
    gpu_util_provider_for_testing_ = std::move(provider);
}

bool VLLMResourceManager::initializeNVML() {
#if defined(THEMIS_ENABLE_CUDA) && defined(__linux__)
    nvmlReturn_t result = nvmlInit();
    if (result != NVML_SUCCESS) {
        THEMIS_ERROR("NVML initialization failed: {}", nvmlErrorString(result));
        return false;
    }

    // Build the list of device indices to monitor.
    // gpu_device_indices (explicit multi-device) takes priority over gpu_device_index.
    const std::vector<uint32_t> indices = !config_.gpu_device_indices.empty()
                                              ? config_.gpu_device_indices
                                              : std::vector<uint32_t>{config_.gpu_device_index};

    nvml_devices_.clear();
    for (uint32_t idx : indices) {
        nvmlDevice_t dev;
        result = nvmlDeviceGetHandleByIndex(idx, &dev);
        if (result != NVML_SUCCESS) {
            THEMIS_ERROR("Failed to get NVML device handle for device {}: {}", idx, nvmlErrorString(result));
            nvml_devices_.clear();
            nvml_device_ = nullptr;
            nvmlShutdown();
            return false;
        }
        nvml_devices_.push_back(static_cast<void *>(dev));
    }

    // nvml_device_ is a convenience alias to the first monitored device; it is
    // only used by canUseGPU() which monitors the primary device for the timeout
    // check.  queryGPUUtilization() always iterates nvml_devices_ for max across
    // all devices.
    nvml_device_ = nvml_devices_.empty() ? nullptr : nvml_devices_.front();

    THEMIS_INFO("NVML initialized, monitoring {} GPU device(s)", nvml_devices_.size());
    return true;
#else
    return false; // NVML not available
#endif
}

void VLLMResourceManager::shutdownNVML() {
#if defined(THEMIS_ENABLE_CUDA) && defined(__linux__)
    if (!nvml_devices_.empty()) {
        // Release all device handle references before calling nvmlShutdown().
        nvml_devices_.clear();
        nvml_device_ = nullptr;
        nvmlShutdown();
        THEMIS_INFO("NVML shutdown");
    }
#endif
}

std::optional<double> VLLMResourceManager::queryGPUUtilization() {
    // Test override: allows CI tests to verify utilization logic without real CUDA.
    if (gpu_util_provider_for_testing_) {
        return gpu_util_provider_for_testing_();
    }

#if defined(THEMIS_ENABLE_CUDA) && defined(__linux__)
    if (nvml_devices_.empty()) {
        return std::nullopt;
    }

    // Return the maximum utilization across all monitored devices so that a
    // single busy GPU blocks ThemisDB from scheduling new work on any device.
    double max_utilization = 0.0;
    bool got_any           = false;
    for (void *handle : nvml_devices_) {
        nvmlDevice_t device = static_cast<nvmlDevice_t>(handle);
        nvmlUtilization_t utilization;
        nvmlReturn_t result = nvmlDeviceGetUtilizationRates(device, &utilization);
        if (result != NVML_SUCCESS) {
            THEMIS_WARN("Failed to query GPU utilization: {}", nvmlErrorString(result));
            continue;
        }
        double util     = static_cast<double>(utilization.gpu);
        max_utilization = std::max(max_utilization, util);
        got_any         = true;
    }
    return got_any ? std::optional<double>{max_utilization} : std::nullopt;
#else
    return std::nullopt; // NVML not available
#endif
}

} // namespace acceleration
} // namespace themis
