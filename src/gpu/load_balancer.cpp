/**
 * @file load_balancer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * GPU Load Balancer — multi-GPU work distribution.
 */

#include "themis/gpu/load_balancer.h"
#include "themis/gpu/gpu_backend_dispatch_contract.h"
#include "themis/gpu/gpu_backend_dispatch_diagnostics.h"
#include <spdlog/spdlog.h>
#include <chrono>

namespace themis {
namespace gpu {

// Helper: Measure selectDevice operation time against bounded runtime contract
static inline uint64_t getCurrentTimeUS() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

// ============================================================================
// Construction
// ============================================================================

GPULoadBalancer::GPULoadBalancer(Strategy strategy) : strategy_(strategy) {}

GPULoadBalancer::GPULoadBalancer(Strategy strategy, const std::vector<DeviceInfo> &devices) : strategy_(strategy) {
    updateDevices(devices);
}

// ============================================================================
// Device management
// ============================================================================

void GPULoadBalancer::updateDevices(const std::vector<DeviceInfo> &devices) {
    std::lock_guard<std::mutex> lock(mutex_);
    devices_.clear();
    devices_.reserve(devices.size());
    for (const auto &d : devices) {
        DeviceEntry e;
        e.info                = d;
        e.balancer_healthy    = d.is_healthy;
        e.tracked_alloc_bytes = 0;
        devices_.push_back(std::move(e));
    }
    round_robin_cursor_ = 0;
}

void GPULoadBalancer::markDeviceFailed(int device_index, const std::string &reason) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto &e : devices_) {
        if (e.info.index == device_index) {
            e.balancer_healthy = false;
            e.failure_reason   = reason;
            break;
        }
    }
}

void GPULoadBalancer::resetDevice(int device_index) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto &e : devices_) {
        if (e.info.index == device_index) {
            e.balancer_healthy = true;
            e.failure_reason.clear();
            break;
        }
    }
}

// ============================================================================
// setTopology
// ============================================================================

void GPULoadBalancer::setTopology(const GPUClusterTopology &topology) {
    std::lock_guard<std::mutex> lock(mutex_);
    topology_ = topology;
}

// ============================================================================
// isEligible (called under mutex_)
// ============================================================================

bool GPULoadBalancer::isEligible(const DeviceEntry &e, uint64_t required_vram) const {
    if (!e.balancer_healthy || !e.info.is_healthy) {
        return false;
    }
    if (e.info.backend == "CPU_FALLBACK") {
        return false;
    }
    if (required_vram > 0 && e.info.free_vram_bytes < required_vram) {
        return false;
    }
    return true;
}

// ============================================================================
// Selection helpers (called under mutex_)
// ============================================================================

GPULoadBalancer::DeviceEntry *GPULoadBalancer::selectRoundRobin(uint64_t required_vram) {
    const size_t n = devices_.size();
    if (n == 0) {
        return nullptr;
    }

    for (size_t tried = 0; tried < n; ++tried) {
        const size_t idx    = round_robin_cursor_ % n;
        round_robin_cursor_ = (round_robin_cursor_ + 1) % n;
        if (isEligible(devices_[idx], required_vram)) {
            return &devices_[idx];
        }
    }
    return nullptr;
}

GPULoadBalancer::DeviceEntry *GPULoadBalancer::selectLeastLoaded(uint64_t required_vram) {
    DeviceEntry *best = nullptr;
    for (auto &e : devices_) {
        if (!isEligible(e, required_vram)) {
            continue;
        }
        if (best == nullptr || e.info.free_vram_bytes > best->info.free_vram_bytes) {
            best = &e;
        }
    }
    return best;
}

GPULoadBalancer::DeviceEntry *GPULoadBalancer::selectFirstHealthy(uint64_t required_vram) {
    for (auto &e : devices_) {
        if (isEligible(e, required_vram)) {
            return &e;
        }
    }
    return nullptr;
}

GPULoadBalancer::DeviceEntry *GPULoadBalancer::selectTopologyAware(uint64_t required_vram) {
    // If no NVLink topology is available, fall back to least-loaded selection.
    if (!topology_.has_nvlink || topology_.num_gpus == 0) {
        return selectLeastLoaded(required_vram);
    }

    // For each eligible device, sum its outgoing bandwidths from the topology's
    // bandwidth_matrix to find the device that is best-connected over NVLink.
    DeviceEntry *best = nullptr;
    float best_bw     = -1.0f;

    for (auto &e : devices_) {
        if (!isEligible(e, required_vram)) {
            continue;
        }

        const int idx = e.info.index;
        if (idx < 0 || idx >= topology_.num_gpus) {
            continue;
        }

        float bw_sum = 0.0f;
        for (int j = 0; j < topology_.num_gpus; ++j) {
            if (j == idx) {
                continue;
            }
            bw_sum += topology_.bandwidthBetween(idx, j);
        }

        if (bw_sum > best_bw) {
            best_bw = bw_sum;
            best    = &e;
        }
    }

    // If no device in the topology is eligible, fall back to least-loaded.
    return best ? best : selectLeastLoaded(required_vram);
}

// ============================================================================
// selectDevice
// ============================================================================

const DeviceInfo *GPULoadBalancer::selectDevice(uint64_t required_vram_bytes) {
    uint64_t start_time = getCurrentTimeUS();
    
    std::lock_guard<std::mutex> lock(mutex_);
    DeviceEntry *entry = nullptr;
    switch (strategy_) {
        case Strategy::ROUND_ROBIN:
            entry = selectRoundRobin(required_vram_bytes);
            break;
        case Strategy::LEAST_LOADED:
            entry = selectLeastLoaded(required_vram_bytes);
            break;
        case Strategy::FIRST_HEALTHY:
            entry = selectFirstHealthy(required_vram_bytes);
            break;
        case Strategy::TOPOLOGY_AWARE:
            entry = selectTopologyAware(required_vram_bytes);
            break;
    }
    
    // Phase 2/3 Hardening: Fail-closed diagnostic emission
    if (!entry) {
        GPUBackendDispatchDiagnostics::emitDiagnostic(
            GPUDispatchErrorCode::BACKEND_NO_DEVICE_AVAILABLE,
            -1,  // No specific device
            "selectDevice: No eligible device found (strategy=" + 
            std::string(strategy_ == Strategy::ROUND_ROBIN ? "ROUND_ROBIN" :
                       strategy_ == Strategy::LEAST_LOADED ? "LEAST_LOADED" :
                       strategy_ == Strategy::FIRST_HEALTHY ? "FIRST_HEALTHY" :
                       "TOPOLOGY_AWARE") + 
            ", required_vram=" + std::to_string(required_vram_bytes) + 
            ", total_devices=" + std::to_string(devices_.size()) + ")");
    }
    
    // Verify bounded runtime contract
    uint64_t elapsed_us = getCurrentTimeUS() - start_time;
    if (elapsed_us > GPUBackendDispatchContract::MAX_SELECT_DEVICE_LATENCY_US) {
        auto logger = spdlog::get("gpu");
        if (!logger) {
            logger = spdlog::get("default");
        }
        if (logger) {
            logger->warn(
                "selectDevice exceeded SLA: elapsed={}µs threshold={}µs devices={}",
                elapsed_us,
                GPUBackendDispatchContract::MAX_SELECT_DEVICE_LATENCY_US,
                devices_.size());
        }
    }
    
    return entry ? &entry->info : nullptr;
}

// ============================================================================
// recordAllocation / recordDeallocation
// ============================================================================

void GPULoadBalancer::recordAllocation(int device_index, uint64_t bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto &e : devices_) {
        if (e.info.index == device_index) {
            e.tracked_alloc_bytes += bytes;
            // Update the free_vram estimate.
            if (e.info.free_vram_bytes >= bytes) {
                e.info.free_vram_bytes -= bytes;
            } else {
                e.info.free_vram_bytes = 0;
            }
            break;
        }
    }
}

void GPULoadBalancer::recordDeallocation(int device_index, uint64_t bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto &e : devices_) {
        if (e.info.index == device_index) {
            if (e.tracked_alloc_bytes >= bytes) {
                e.tracked_alloc_bytes -= bytes;
            } else {
                e.tracked_alloc_bytes = 0;
            }
            e.info.free_vram_bytes += bytes;
            break;
        }
    }
}

// ============================================================================
// Queries
// ============================================================================

size_t GPULoadBalancer::totalDevices() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return devices_.size();
}

size_t GPULoadBalancer::healthyDevices() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t n = 0;
    for (const auto &e : devices_) {
        if (e.balancer_healthy && e.info.is_healthy && e.info.backend != "CPU_FALLBACK") {
            ++n;
        }
    }
    return n;
}

std::vector<GPULoadBalancer::DeviceLoad> GPULoadBalancer::getDeviceLoads() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<DeviceLoad> result;
    result.reserve(devices_.size());
    for (const auto &e : devices_) {
        DeviceLoad dl;
        dl.index               = e.info.index;
        dl.name                = e.info.name;
        dl.backend             = e.info.backend;
        dl.free_vram_bytes     = e.info.free_vram_bytes;
        dl.tracked_alloc_bytes = e.tracked_alloc_bytes;
        dl.is_healthy          = e.balancer_healthy && e.info.is_healthy;
        dl.failure_reason      = e.failure_reason;
        result.push_back(std::move(dl));
    }
    return result;
}

} // namespace gpu
} // namespace themis
