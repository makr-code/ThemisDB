/**
 * @file cycle_metrics.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "performance/cycle_metrics.h"
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <cstring>
#include <mutex>

#ifdef __linux__
#include <unistd.h>
#endif

namespace themis {
namespace performance {

uint64_t HardwareCycleCounter::cpu_frequency_hz() noexcept {
    static uint64_t cached_frequency = 0;
    
    if (cached_frequency != 0) {
        return cached_frequency;
    }

#ifdef __linux__
    // Try to read from /proc/cpuinfo
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (cpuinfo.is_open()) {
        std::string line;
        while (std::getline(cpuinfo, line)) {
            if (line.find("cpu MHz") != std::string::npos) {
                size_t colon_pos = line.find(':');
                if (colon_pos != std::string::npos) {
                    double mhz = std::stod(line.substr(colon_pos + 1));
                    cached_frequency = static_cast<uint64_t>(mhz * 1'000'000);
                    return cached_frequency;
                }
            }
        }
    }
    
    // Fallback: Try /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq
    std::ifstream freq_file("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
    if (freq_file.is_open()) {
        uint64_t khz;
        freq_file >> khz;
        cached_frequency = khz * 1000;
        return cached_frequency;
    }
#endif

    // Default fallback: assume 3 GHz
    cached_frequency = 3'000'000'000;
    return cached_frequency;
}

std::string HardwareCycleCounter::cpu_model() noexcept {
    static std::string cached_model;
    
    if (!cached_model.empty()) {
        return cached_model;
    }

#ifdef __linux__
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (cpuinfo.is_open()) {
        std::string line;
        while (std::getline(cpuinfo, line)) {
            if (line.find("model name") != std::string::npos) {
                size_t colon_pos = line.find(':');
                if (colon_pos != std::string::npos) {
                    cached_model = line.substr(colon_pos + 1);
                    // Trim leading whitespace
                    size_t first = cached_model.find_first_not_of(" \t");
                    if (first != std::string::npos) {
                        cached_model = cached_model.substr(first);
                    }
                    return cached_model;
                }
            }
        }
    }
#endif

#if defined(__x86_64__) || defined(_M_X64)
    cached_model = "x86_64 CPU";
#elif defined(__aarch64__) || defined(_M_ARM64)
    cached_model = "ARM64 CPU";
#else
    cached_model = "Unknown CPU";
#endif

    return cached_model;
}

#ifdef THEMIS_ENABLE_GPU_CYCLE_METRICS
namespace {
std::mutex& gpuStartFnMutex() {
    static std::mutex m;
    return m;
}
HardwareCycleCounter::GpuCyclesStartFn& gpuStartFnStorage() {
    static HardwareCycleCounter::GpuCyclesStartFn fn;
    return fn;
}
std::mutex& gpuEndFnMutex() {
    static std::mutex m;
    return m;
}
HardwareCycleCounter::GpuCyclesEndFn& gpuEndFnStorage() {
    static HardwareCycleCounter::GpuCyclesEndFn fn;
    return fn;
}
} // namespace

void HardwareCycleCounter::setGpuCyclesStartFn(GpuCyclesStartFn fn) {
    std::lock_guard<std::mutex> lk(gpuStartFnMutex());
    gpuStartFnStorage() = std::move(fn);
}

void HardwareCycleCounter::setGpuCyclesEndFn(GpuCyclesEndFn fn) {
    std::lock_guard<std::mutex> lk(gpuEndFnMutex());
    gpuEndFnStorage() = std::move(fn);
}

#ifdef __CUDACC__
#include <cuda_runtime.h>

void* HardwareCycleCounter::gpu_cycles_start() noexcept {
    cudaEvent_t event;
    cudaEventCreate(&event);
    cudaEventRecord(event);
    return reinterpret_cast<void*>(event);
}

uint64_t HardwareCycleCounter::gpu_cycles_end(void* event) noexcept {
    if (!event) {
      return 0;
    }
    
    cudaEvent_t start_event = reinterpret_cast<cudaEvent_t>(event);
    cudaEvent_t end_event;
    cudaEventCreate(&end_event);
    cudaEventRecord(end_event);
    cudaEventSynchronize(end_event);
    
    float milliseconds = 0;
    cudaEventElapsedTime(&milliseconds, start_event, end_event);
    
    cudaEventDestroy(start_event);
    cudaEventDestroy(end_event);
    
    // Convert milliseconds to approximate GPU cycles
    // Assuming ~1.5 GHz GPU clock
    return static_cast<uint64_t>(milliseconds * 1'500'000);
}
#else
// STUB/SIMULATION NOTE:
// Purpose:          No-op stubs for GPU cycle measurement when CUDA is not available.
//                   gpu_cycles_start() returns nullptr; gpu_cycles_end() returns 0.
//                   Allows the cycle_metrics interface to compile and link on non-CUDA hosts.
// Activation:       Active when THEMIS_ENABLE_GPU_CYCLE_METRICS is defined but __CUDACC__
//                   is not — i.e. non-CUDA compilation units that include the header.
// Production Delta: No GPU timing data is collected; callers receive zero elapsed cycles.
// Removal Plan:     Compile this translation unit with nvcc (or clang CUDA) and the CUDA
//                   runtime linked to enable real GPU cycle measurement.
//                   Tracked in src/performance/FUTURE_ENHANCEMENTS.md § GPU Cycle Metrics.
void* HardwareCycleCounter::gpu_cycles_start() noexcept {
    GpuCyclesStartFn fn;
    {
        std::lock_guard<std::mutex> lk(gpuStartFnMutex());
        fn = gpuStartFnStorage();
    }
    if (fn) {
        try {
            return fn();
        } catch (...) {
            return nullptr;
        }
    }
    return nullptr;
}

uint64_t HardwareCycleCounter::gpu_cycles_end(void* event) noexcept {
    GpuCyclesEndFn fn;
    {
        std::lock_guard<std::mutex> lk(gpuEndFnMutex());
        fn = gpuEndFnStorage();
    }
    if (fn) {
        try {
            return fn(event);
        } catch (...) {
            return 0;
        }
    }
    return 0;
}
#endif
#endif

} // namespace performance
} // namespace themis

