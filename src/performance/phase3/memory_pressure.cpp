/**
 * @file memory_pressure.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "performance/phase3/memory_pressure.h"

#include <algorithm>

#if defined(__linux__)
#include <fstream>
#include <string>
#elif defined(__APPLE__)
#include <sys/sysctl.h>
#include <sys/types.h>
#include <mach/mach.h>
#elif defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace themis {
namespace performance {
namespace phase3 {

// ============================================================================
// Construction / destruction
// ============================================================================

SystemMemoryPressureMonitor::SystemMemoryPressureMonitor()
    : config_(Config{}) {}

SystemMemoryPressureMonitor::SystemMemoryPressureMonitor(Config config)
    : config_(config) {}

SystemMemoryPressureMonitor::~SystemMemoryPressureMonitor() {
    stop();
}

// ============================================================================
// Lifecycle
// ============================================================================

void SystemMemoryPressureMonitor::start() {
    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true)) {
        return; // already running
    }
    poll_thread_ = std::thread(&SystemMemoryPressureMonitor::poll_loop, this);
}

void SystemMemoryPressureMonitor::stop() {
    running_.store(false, std::memory_order_relaxed);
    if (poll_thread_.joinable()) {
        poll_thread_.join();
    }
}

bool SystemMemoryPressureMonitor::is_running() const noexcept {
    return running_.load(std::memory_order_relaxed);
}

// ============================================================================
// Callback registration
// ============================================================================

size_t SystemMemoryPressureMonitor::register_eviction_callback(
    PressureLevel trigger_level, EvictionCallback callback) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    size_t handle = next_handle_++;
    callbacks_.push_back({handle, trigger_level, std::move(callback)});
    return handle;
}

void SystemMemoryPressureMonitor::unregister_eviction_callback(size_t handle) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    callbacks_.erase(
        std::remove_if(callbacks_.begin(), callbacks_.end(),
                       [handle](const CallbackEntry& e) { return e.handle == handle; }),
        callbacks_.end());
}

// ============================================================================
// Observation
// ============================================================================

SystemMemoryPressureMonitor::MemorySnapshot
SystemMemoryPressureMonitor::sample() const {
    return read_os_memory();
}

SystemMemoryPressureMonitor::MemorySnapshot
SystemMemoryPressureMonitor::last_snapshot() const {
    std::lock_guard<std::mutex> lock(snapshot_mutex_);
    return last_snapshot_;
}

uint64_t SystemMemoryPressureMonitor::eviction_trigger_count() const noexcept {
    return eviction_trigger_count_.load(std::memory_order_relaxed);
}

std::string SystemMemoryPressureMonitor::level_name(PressureLevel level) {
    switch (level) {
        case PressureLevel::NORMAL:   return "NORMAL";
        case PressureLevel::MODERATE: return "MODERATE";
        case PressureLevel::HIGH:     return "HIGH";
        case PressureLevel::CRITICAL: return "CRITICAL";
    }
    return "UNKNOWN";
}

// ============================================================================
// Internal helpers
// ============================================================================

SystemMemoryPressureMonitor::MemorySnapshot
SystemMemoryPressureMonitor::read_os_memory() const {
    MemorySnapshot snap;
    snap.sampled_at = std::chrono::steady_clock::now();

#if defined(__linux__)
    // Parse /proc/meminfo for MemTotal and MemAvailable
    std::ifstream meminfo("/proc/meminfo");
    size_t mem_total_kb = 0;
    size_t mem_available_kb = 0;
    std::string line = {};
    while (std::getline(meminfo, line)) {
        if (line.rfind("MemTotal:", 0) == 0) {
            mem_total_kb = std::stoull(line.substr(9));
        } else if (line.rfind("MemAvailable:", 0) == 0) {
            mem_available_kb = std::stoull(line.substr(13));
        }
        if (mem_total_kb && mem_available_kb) {
          break;
        }
    }
    snap.total_bytes     = mem_total_kb * 1024;
    snap.available_bytes = mem_available_kb * 1024;

#elif defined(__APPLE__)
    // Use host_statistics64 for accurate available memory
    mach_port_t host = mach_host_self();
    vm_statistics64_data_t vm_stat;
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    if (host_statistics64(host, HOST_VM_INFO64,
                          reinterpret_cast<host_info64_t>(&vm_stat),
                          &count) == KERN_SUCCESS) {
        uint64_t page_size = 0;
        host_page_size(host, reinterpret_cast<vm_size_t*>(&page_size));
        snap.available_bytes = (static_cast<uint64_t>(vm_stat.free_count) +
                                static_cast<uint64_t>(vm_stat.inactive_count)) *
                               page_size;
    }
    // Total physical memory via sysctl
    int mib[2] = {CTL_HW, HW_MEMSIZE};
    uint64_t hw_mem = 0;
    size_t len = sizeof(hw_mem);
    if (sysctl(mib, 2, &hw_mem, &len, nullptr, 0) == 0) {
        snap.total_bytes = hw_mem;
    }

#elif defined(_WIN32)
    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    if (GlobalMemoryStatusEx(&ms)) {
        snap.total_bytes     = static_cast<size_t>(ms.ullTotalPhys);
        snap.available_bytes = static_cast<size_t>(ms.ullAvailPhys);
    }

#else
    // Fallback: use configured total if provided, assume 50% available
    if (config_.total_memory_override_bytes) {
        snap.total_bytes     = config_.total_memory_override_bytes;
        snap.available_bytes = config_.total_memory_override_bytes / 2;
    }
#endif

    // Allow test/override of total
    if (config_.total_memory_override_bytes) {
        snap.total_bytes = config_.total_memory_override_bytes;
    }

    if (snap.total_bytes > 0) {
        snap.used_bytes = snap.total_bytes > snap.available_bytes
                          ? snap.total_bytes - snap.available_bytes
                          : 0;
        snap.usage_percent = 100.0 * static_cast<double>(snap.used_bytes) /
                             static_cast<double>(snap.total_bytes);
    }

    snap.level = classify(snap.usage_percent);
    return snap;
}

SystemMemoryPressureMonitor::PressureLevel
SystemMemoryPressureMonitor::classify(double usage_percent) const noexcept {
    const auto& t = config_.thresholds;
    if (usage_percent >= t.critical_threshold) {
      return PressureLevel::CRITICAL;
    }
    if (usage_percent >= t.high_threshold) {
      return PressureLevel::HIGH;
    }
    if (usage_percent >= t.moderate_threshold) {
      return PressureLevel::MODERATE;
    }
    return PressureLevel::NORMAL;
}

void SystemMemoryPressureMonitor::poll_loop() {
    while (running_.load(std::memory_order_relaxed)) {
        MemorySnapshot snap = read_os_memory();

        {
            std::lock_guard<std::mutex> lock(snapshot_mutex_);
            last_snapshot_ = snap;
        }

        if (snap.level != PressureLevel::NORMAL) {
            trigger_callbacks(snap.level);
        }

        // Sleep in small increments so stop() can interrupt quickly.
        const auto deadline =
            std::chrono::steady_clock::now() + config_.poll_interval;
        constexpr auto kSlice = std::chrono::milliseconds(50);
        while (running_.load(std::memory_order_relaxed) &&
               std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(kSlice);
        }
    }
}

void SystemMemoryPressureMonitor::trigger_callbacks(PressureLevel current_level) {
    std::vector<EvictionCallback> to_call;
    {
        std::lock_guard<std::mutex> lock(callbacks_mutex_);
        for (const auto& entry : callbacks_) {
            if (static_cast<int>(current_level) >=
                static_cast<int>(entry.trigger_level)) {
                to_call.push_back(entry.callback);
            }
        }
    }
    if (!to_call.empty()) {
        eviction_trigger_count_.fetch_add(to_call.size(),
                                          std::memory_order_relaxed);
        for (auto& cb : to_call) {
            cb();
        }
    }
}

} // namespace phase3
} // namespace performance
} // namespace themis

