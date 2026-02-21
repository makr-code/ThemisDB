/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            load_balancer.cpp                                  ║
  Version:         0.0.20                                             ║
  Last Modified:   2026-02-21 19:14:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     238                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/*
 * GPU Load Balancer — multi-GPU work distribution.
 */

#include "themis/gpu/load_balancer.h"

namespace themis {
namespace gpu {

// ============================================================================
// Construction
// ============================================================================

GPULoadBalancer::GPULoadBalancer(Strategy strategy)
    : strategy_(strategy) {}

GPULoadBalancer::GPULoadBalancer(Strategy strategy,
                                   const std::vector<DeviceInfo>& devices)
    : strategy_(strategy) {
    updateDevices(devices);
}

// ============================================================================
// Device management
// ============================================================================

void GPULoadBalancer::updateDevices(const std::vector<DeviceInfo>& devices) {
    std::lock_guard<std::mutex> lock(mutex_);
    devices_.clear();
    devices_.reserve(devices.size());
    for (const auto& d : devices) {
        DeviceEntry e;
        e.info             = d;
        e.balancer_healthy = d.is_healthy;
        e.tracked_alloc_bytes = 0;
        devices_.push_back(std::move(e));
    }
    round_robin_cursor_ = 0;
}

void GPULoadBalancer::markDeviceFailed(int device_index,
                                         const std::string& reason) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& e : devices_) {
        if (e.info.index == device_index) {
            e.balancer_healthy = false;
            e.failure_reason   = reason;
            break;
        }
    }
}

void GPULoadBalancer::resetDevice(int device_index) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& e : devices_) {
        if (e.info.index == device_index) {
            e.balancer_healthy = true;
            e.failure_reason.clear();
            break;
        }
    }
}

// ============================================================================
// isEligible (called under mutex_)
// ============================================================================

bool GPULoadBalancer::isEligible(const DeviceEntry& e,
                                   uint64_t required_vram) const {
    if (!e.balancer_healthy || !e.info.is_healthy) return false;
    if (e.info.backend == "CPU_FALLBACK") return false;
    if (required_vram > 0 && e.info.free_vram_bytes < required_vram) {
        return false;
    }
    return true;
}

// ============================================================================
// Selection helpers (called under mutex_)
// ============================================================================

GPULoadBalancer::DeviceEntry*
GPULoadBalancer::selectRoundRobin(uint64_t required_vram) {
    const size_t n = devices_.size();
    if (n == 0) return nullptr;

    for (size_t tried = 0; tried < n; ++tried) {
        const size_t idx = round_robin_cursor_ % n;
        round_robin_cursor_ = (round_robin_cursor_ + 1) % n;
        if (isEligible(devices_[idx], required_vram)) {
            return &devices_[idx];
        }
    }
    return nullptr;
}

GPULoadBalancer::DeviceEntry*
GPULoadBalancer::selectLeastLoaded(uint64_t required_vram) {
    DeviceEntry* best = nullptr;
    for (auto& e : devices_) {
        if (!isEligible(e, required_vram)) continue;
        if (best == nullptr ||
            e.info.free_vram_bytes > best->info.free_vram_bytes) {
            best = &e;
        }
    }
    return best;
}

GPULoadBalancer::DeviceEntry*
GPULoadBalancer::selectFirstHealthy(uint64_t required_vram) {
    for (auto& e : devices_) {
        if (isEligible(e, required_vram)) return &e;
    }
    return nullptr;
}

// ============================================================================
// selectDevice
// ============================================================================

const DeviceInfo* GPULoadBalancer::selectDevice(uint64_t required_vram_bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    DeviceEntry* entry = nullptr;
    switch (strategy_) {
        case Strategy::ROUND_ROBIN:
            entry = selectRoundRobin(required_vram_bytes);   break;
        case Strategy::LEAST_LOADED:
            entry = selectLeastLoaded(required_vram_bytes);  break;
        case Strategy::FIRST_HEALTHY:
            entry = selectFirstHealthy(required_vram_bytes); break;
    }
    return entry ? &entry->info : nullptr;
}

// ============================================================================
// recordAllocation / recordDeallocation
// ============================================================================

void GPULoadBalancer::recordAllocation(int device_index, uint64_t bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& e : devices_) {
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
    for (auto& e : devices_) {
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
    for (const auto& e : devices_) {
        if (e.balancer_healthy && e.info.is_healthy &&
            e.info.backend != "CPU_FALLBACK") {
            ++n;
        }
    }
    return n;
}

std::vector<GPULoadBalancer::DeviceLoad>
GPULoadBalancer::getDeviceLoads() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<DeviceLoad> result;
    result.reserve(devices_.size());
    for (const auto& e : devices_) {
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
