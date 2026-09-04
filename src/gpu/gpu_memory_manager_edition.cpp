/**
 * @file gpu_memory_manager_edition.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=24, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * GPU Memory Manager with Edition-Specific Constraints
 * ======================================================
 * Enforces GPU VRAM limits based on edition at runtime.
 * Edition constraints are set at compile-time via CMakeLists.txt.
 * 
 * Phase 3 Hardening (Memory Management):
 * - All allocation paths are exception-safe via RAII
 * - Tenant quota tracking is exception-safe (updates post-allocation)
 * - Rollback mechanism for allocation failures
 * - Diagnostic events for troubleshooting
 */

#include "themis/gpu/memory_manager.h"
#include "themis/gpu/gpu_error.h"
#include <spdlog/spdlog.h>
#include <utility>

namespace themis {
namespace gpu {

// ============================================================================
// Internal RAII guard for allocation rollback (Phase 3 Hardening)
// ============================================================================

class AllocationGuard {
public:
    AllocationGuard(GPUMemoryManager* mgr, uint64_t size, const std::string& tenant_id)
        : manager_(mgr), size_(size), tenant_id_(tenant_id), committed_(false) {}

    ~AllocationGuard() noexcept {
        if (!committed_ && manager_) {
            manager_->RollbackAllocationUnderLock(tenant_id_, size_);
        }
    }

    void commit() noexcept { committed_ = true; }

    // Non-copyable, non-movable
    AllocationGuard(const AllocationGuard&) = delete;
    AllocationGuard& operator=(const AllocationGuard&) = delete;
    AllocationGuard(AllocationGuard&&) = delete;
    AllocationGuard& operator=(AllocationGuard&&) = delete;

private:
    GPUMemoryManager* manager_;
    uint64_t size_ = {};
    std::string tenant_id_;
    bool committed_;
};

// ============================================================================
// Internal helper — rollback allocation on failure (must hold mutex_)
// ============================================================================

void GPUMemoryManager::RollbackAllocationUnderLock(const std::string &tenant_id, uint64_t size_bytes) {
    auto logger = spdlog::get("gpu");

    // Decrement global counter.
    if (gpu_memory_allocated_ >= size_bytes) {
        gpu_memory_allocated_ -= size_bytes;
    } else {
        gpu_memory_allocated_ = 0;
    }

    // Rollback tenant quota if applicable.
    if (!tenant_id.empty()) {
        auto it = tenant_states_.find(tenant_id);
        if (it != tenant_states_.end()) {
            if (it->second.allocated_bytes >= size_bytes) {
                it->second.allocated_bytes -= size_bytes;
            } else {
                it->second.allocated_bytes = 0;
            }
            if (logger) {
                logger->info("Rollback: tenant={}, size={}, tenant_remaining={}", 
                           tenant_id, size_bytes, it->second.allocated_bytes);
            }
        }
    } else if (logger) {
        logger->info("Rollback: global allocation, size={}, remaining={}", size_bytes, gpu_memory_allocated_);
    }
}

// ============================================================================
// Internal helper — must be called with mutex_ already held
// Exception-safe allocation with RAII guards (Phase 3 Hardening)
// ============================================================================

bool GPUMemoryManager::TryAllocateUnderLock(uint64_t size_bytes, const std::string &tag, const std::string &tenant_id) {
    const uint64_t max_vram = GetMaxGPUVRAMBytes();
    // Hints count against the VRAM budget so that reserved headroom is
    // protected from other callers.  Use a separate budget_check variable for
    // the limit test so that gpu_memory_allocated_ only tracks committed bytes
    // and hint_reserved_bytes_ is not double-counted on subsequent allocations.
    const uint64_t budget_check = gpu_memory_allocated_ + hint_reserved_bytes_ + size_bytes;
    const uint64_t new_total    = gpu_memory_allocated_ + size_bytes;

    auto logger = spdlog::get("gpu");

    // Check global edition limit.
    if (budget_check > max_vram) {
        if (logger) {
            logger->warn("Allocation denied: global limit exceeded (current={}, requested={}, limit={})",
                        gpu_memory_allocated_, size_bytes, max_vram);
        }
        return false;
    }

    // Check per-tenant quota (if one is registered and non-zero).
    if (!tenant_id.empty()) {
        auto it = tenant_states_.find(tenant_id);
        if (it != tenant_states_.end() && it->second.quota_bytes > 0) {
            if (it->second.allocated_bytes + size_bytes > it->second.quota_bytes) {
                if (logger) {
                    logger->warn("Allocation denied: tenant quota exceeded (tenant={}, current={}, requested={}, quota={})",
                               tenant_id, it->second.allocated_bytes, size_bytes, it->second.quota_bytes);
                }
                return false;
            }
        }
    }

    // Create a guard to automatically rollback on exception.
    // The guard is not active in the committed state.
    AllocationGuard guard(this, size_bytes, tenant_id);

    // RAII-safe allocation: add to tracking first (may throw on vector alloc)
    // If this throws, the guard's destructor will rollback.
    try {
        active_allocations_.push_back({size_bytes, tag, tenant_id});
    } catch (const std::bad_alloc &ex) {
        // Vector allocation failed; guard will rollback on scope exit
        if (logger) {
            logger->warn("TryAllocateUnderLock: vector allocation failed: {}", ex.what());
        }
        return false;
    } catch (const std::exception &ex) {
        // Unexpected exception during allocation tracking; guard will rollback
        if (logger) {
            logger->error("TryAllocateUnderLock: unexpected exception: {}", ex.what());
        }
        return false;
    }

    // Commit the global counter (only reached if push succeeded).
    gpu_memory_allocated_ = new_total;
    if (gpu_memory_allocated_ > peak_bytes_) {
        peak_bytes_ = gpu_memory_allocated_;
    }
    ++allocation_count_;

    // Update tenant state (exception-safe: post-allocation update).
    // This occurs after the allocation record is safely stored.
    if (!tenant_id.empty()) {
        try {
            auto &ts = tenant_states_[tenant_id];
            ts.allocated_bytes += size_bytes;
            if (ts.allocated_bytes > ts.peak_bytes) {
                ts.peak_bytes = ts.allocated_bytes;
            }
        } catch (const std::exception &ex) {
            // If tenant state update fails, rollback the entire allocation.
            if (logger) {
                logger->error("Failed to update tenant state: {}", ex.what());
            }
            RollbackAllocationUnderLock(tenant_id, size_bytes);
            // Remove the allocation record we just added.
            if (!active_allocations_.empty() && active_allocations_.back().size_bytes == size_bytes) {
                active_allocations_.pop_back();
            }
            return false;
        }
    }

    // Mark guard as committed to prevent rollback on destruction.
    guard.commit();

    if (logger) {
        logger->debug("Allocation granted: tag={}, size={}, tenant={}, total={}", 
                     tag, size_bytes, tenant_id.empty() ? "(global)" : tenant_id, gpu_memory_allocated_);
    }

    return true;
}

// ============================================================================
// IVRAMPolicy implementation
// ============================================================================

bool GPUMemoryManager::canAllocate(uint64_t size_bytes, const std::string &tenant_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const uint64_t max_vram  = GetMaxGPUVRAMBytes();
    const uint64_t new_total = gpu_memory_allocated_ + hint_reserved_bytes_ + size_bytes;
    if (new_total > max_vram) {
        return false;
    }
    if (!tenant_id.empty()) {
        auto it = tenant_states_.find(tenant_id);
        if (it != tenant_states_.end() && it->second.quota_bytes > 0) {
            if (it->second.allocated_bytes + size_bytes > it->second.quota_bytes) {
                return false;
            }
        }
    }
    return true;
}

void GPUMemoryManager::onAllocate(uint64_t size_bytes, const std::string &tag, const std::string &tenant_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    TryAllocateUnderLock(size_bytes, tag, tenant_id);
}

void GPUMemoryManager::onDeallocate(uint64_t size_bytes, const std::string &tenant_id) {
    if (tenant_id.empty()) {
        DeallocateGPU(size_bytes);
    } else {
        DeallocateGPU(size_bytes, tenant_id);
    }
}

uint64_t GPUMemoryManager::usedBytes() const {
    return GetGPUMemoryUsed();
}

bool GPUMemoryManager::isGPUEnabled() const noexcept {
    return IsGPUAccelerationEnabled();
}

// ============================================================================
// Tenant quota management
// ============================================================================

void GPUMemoryManager::SetTenantQuota(const std::string &tenant_id, uint64_t quota_bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    tenant_states_[tenant_id].quota_bytes = quota_bytes;
}

void GPUMemoryManager::RemoveTenantQuota(const std::string &tenant_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    // Always set quota_bytes = 0 (no cap) regardless of current usage.
    // The tenant entry remains in the map so that usage tracking continues;
    // it will naturally disappear when allocations drain to zero and the
    // entry is not needed for quota enforcement.
    auto it = tenant_states_.find(tenant_id);
    if (it != tenant_states_.end()) {
        it->second.quota_bytes = 0;
    }
}

// ============================================================================
// Pre-allocation hint management
// ============================================================================

GPUMemoryManager::HintHandle GPUMemoryManager::ReserveHint(uint64_t size_bytes, const std::string &tag,
                                                           const std::string &tenant_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    const uint64_t max_vram = GetMaxGPUVRAMBytes();
    // Check that the hint fits within the remaining budget (accounting for
    // already-allocated bytes and existing hints).
    if (gpu_memory_allocated_ + hint_reserved_bytes_ + size_bytes > max_vram) {
        return {0, 0, tag, tenant_id}; // id == 0 → failure
    }
    // Check tenant quota if applicable — account for both existing allocations
    // and outstanding hints for this tenant to prevent over-reservation.
    if (!tenant_id.empty()) {
        auto it = tenant_states_.find(tenant_id);
        if (it != tenant_states_.end() && it->second.quota_bytes > 0) {
            // Sum up existing hints for this tenant.
            uint64_t tenant_hint_bytes = 0;
            for (const auto &h : active_hints_) {
                if (h.tenant_id == tenant_id) {
                    tenant_hint_bytes += h.bytes;
                }
            }
            if (it->second.allocated_bytes + tenant_hint_bytes + size_bytes > it->second.quota_bytes) {
                return {0, 0, tag, tenant_id};
            }
        }
    }
    const uint64_t id = next_hint_id_++;
    active_hints_.push_back({id, size_bytes, tag, tenant_id});
    hint_reserved_bytes_ += size_bytes;
    return {id, size_bytes, tag, tenant_id};
}

void GPUMemoryManager::CancelHint([[maybe_unused]] uint64_t hint_id) {
    if (hint_id == 0) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = active_hints_.begin(); it != active_hints_.end(); ++it) {
        if (it->id == hint_id) {
            if (hint_reserved_bytes_ >= it->bytes) {
                hint_reserved_bytes_ -= it->bytes;
            } else {
                hint_reserved_bytes_ = 0;
            }
            active_hints_.erase(it);
            return;
        }
    }
}

bool GPUMemoryManager::ConsumeHint([[maybe_unused]] uint64_t hint_id) {
    if (hint_id == 0) {
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = active_hints_.begin(); it != active_hints_.end(); ++it) {
        if (it->id == hint_id) {
            const uint64_t bytes     = it->bytes;
            const std::string tag    = it->tag;
            const std::string tenant = it->tenant_id;
            // Remove from hints.
            if (hint_reserved_bytes_ >= bytes) {
                hint_reserved_bytes_ -= bytes;
            } else {
                hint_reserved_bytes_ = 0;
            }
            active_hints_.erase(it);
            // Commit as real allocation (no limit re-check needed — the hint
            // already held this capacity).
            gpu_memory_allocated_ += bytes;
            if (gpu_memory_allocated_ > peak_bytes_) {
                peak_bytes_ = gpu_memory_allocated_;
            }
            ++allocation_count_;
            active_allocations_.push_back({bytes, tag, tenant});
            if (!tenant.empty()) {
                auto &ts = tenant_states_[tenant];
                ts.allocated_bytes += bytes;
                if (ts.allocated_bytes > ts.peak_bytes) {
                    ts.peak_bytes = ts.allocated_bytes;
                }
            }
            return true;
        }
    }
    return false;
}

uint64_t GPUMemoryManager::GetHintReservedBytes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return hint_reserved_bytes_;
}

// ============================================================================
// GPUMemoryManager — allocation methods
// ============================================================================

bool GPUMemoryManager::TryAllocateGPU(uint64_t size_bytes, const std::string &tag) {
    std::lock_guard<std::mutex> lock(mutex_);
    return TryAllocateUnderLock(size_bytes, tag, "");
}

bool GPUMemoryManager::TryAllocateGPU(uint64_t size_bytes, const std::string &tag, const std::string &tenant_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    return TryAllocateUnderLock(size_bytes, tag, tenant_id);
}

void GPUMemoryManager::DeallocateGPU([[maybe_unused]] uint64_t size_bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (gpu_memory_allocated_ >= size_bytes) {
        gpu_memory_allocated_ -= size_bytes;
    } else {
        gpu_memory_allocated_ = 0; // guard against mis-matched sizes
    }
    ++deallocation_count_;

    // Remove the first active record whose size matches (FIFO / best-effort).
    for (auto it = active_allocations_.begin(); it != active_allocations_.end(); ++it) {
        if (it->size_bytes == size_bytes) {
            const std::string tid = it->tenant_id;
            active_allocations_.erase(it);
            // Decrement tenant counter if applicable.
            if (!tid.empty()) {
                auto tit = tenant_states_.find(tid);
                if (tit != tenant_states_.end()) {
                    if (tit->second.allocated_bytes >= size_bytes) {
                        tit->second.allocated_bytes -= size_bytes;
                    } else {
                        tit->second.allocated_bytes = 0;
                    }
                }
            }
            break;
        }
    }
}

void GPUMemoryManager::DeallocateGPU(uint64_t size_bytes, const std::string &tenant_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (gpu_memory_allocated_ >= size_bytes) {
        gpu_memory_allocated_ -= size_bytes;
    } else {
        gpu_memory_allocated_ = 0;
    }
    ++deallocation_count_;

    // Remove the first active record matching size AND tenant.
    for (auto it = active_allocations_.begin(); it != active_allocations_.end(); ++it) {
        if (it->size_bytes == size_bytes && it->tenant_id == tenant_id) {
            active_allocations_.erase(it);
            break;
        }
    }

    // Decrement tenant counter.
    if (!tenant_id.empty()) {
        auto tit = tenant_states_.find(tenant_id);
        if (tit != tenant_states_.end()) {
            if (tit->second.allocated_bytes >= size_bytes) {
                tit->second.allocated_bytes -= size_bytes;
            } else {
                tit->second.allocated_bytes = 0;
            }
        }
    }
}

// ============================================================================
// ValidateAllocation
// ============================================================================

void GPUMemoryManager::ValidateAllocation([[maybe_unused]] uint64_t size_bytes) {
    const uint64_t max_vram = GetMaxGPUVRAMBytes();

    if (size_bytes > max_vram) {
        std::string error = "GPU allocation request (";
        error += std::to_string(size_bytes / (1024ULL * 1024ULL * 1024ULL));
        error += "GB) exceeds edition limit (";
        error += std::to_string(GetMaxGPUVRAMGB());
        error += "GB). Edition: ";
        error += std::string(edition::EDITION_STRING);
        throw std::runtime_error(error);
    }

    std::lock_guard<std::mutex> lock(mutex_);
    const uint64_t new_total = gpu_memory_allocated_ + size_bytes;

    if (new_total > max_vram) {
        std::string error = "GPU memory exhausted. Current usage: ";
        error += std::to_string(gpu_memory_allocated_ / (1024ULL * 1024ULL * 1024ULL));
        error += "GB, requested: ";
        error += std::to_string(size_bytes / (1024ULL * 1024ULL * 1024ULL));
        error += "GB, limit: ";
        error += std::to_string(GetMaxGPUVRAMGB());
        error += "GB";
        throw std::runtime_error(error);
    }
}

// ============================================================================
// Queries
// ============================================================================

uint64_t GPUMemoryManager::GetGPUMemoryUsed() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return gpu_memory_allocated_;
}

float GPUMemoryManager::GetGPUMemoryUsagePercent() const {
    const uint64_t max_vram = GetMaxGPUVRAMBytes();
    std::lock_guard<std::mutex> lock(mutex_);
    return max_vram == 0 ? 0.0f : (static_cast<float>(gpu_memory_allocated_) / static_cast<float>(max_vram)) * 100.0f;
}

bool GPUMemoryManager::IsGPUAccelerationEnabled() const noexcept {
    return GetMaxGPUVRAMGB() > 0;
}

GPUMemoryManager::Stats GPUMemoryManager::GetStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    Stats s;
    s.allocated_bytes    = gpu_memory_allocated_;
    s.peak_bytes         = peak_bytes_;
    s.allocation_count   = allocation_count_;
    s.deallocation_count = deallocation_count_;
    return s;
}

std::vector<GPUMemoryManager::AllocationRecord> GPUMemoryManager::GetActiveAllocations() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return active_allocations_;
}

GPUMemoryManager::TenantStats GPUMemoryManager::GetTenantStats(const std::string &tenant_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    TenantStats ts;
    ts.tenant_id = tenant_id;
    auto it      = tenant_states_.find(tenant_id);
    if (it != tenant_states_.end()) {
        ts.quota_bytes     = it->second.quota_bytes;
        ts.allocated_bytes = it->second.allocated_bytes;
        ts.peak_bytes      = it->second.peak_bytes;
    }
    return ts;
}

std::vector<GPUMemoryManager::TenantStats> GPUMemoryManager::GetAllTenantStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<TenantStats> result = {};

    result.reserve(tenant_states_.size());
    for (const auto &kv : tenant_states_) {
        TenantStats ts;
        ts.tenant_id       = kv.first;
        ts.quota_bytes     = kv.second.quota_bytes;
        ts.allocated_bytes = kv.second.allocated_bytes;
        ts.peak_bytes      = kv.second.peak_bytes;
        result.push_back(std::move(ts));
    }
    return result;
}

uint64_t GPUMemoryManager::GetTenantHeadroom(const std::string &tenant_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const uint64_t global_max  = GetMaxGPUVRAMBytes();
    const uint64_t global_used = gpu_memory_allocated_;
    const uint64_t global_left = (global_used < global_max) ? global_max - global_used : 0;

    auto it = tenant_states_.find(tenant_id);
    if (it == tenant_states_.end() || it->second.quota_bytes == 0) {
        return global_left;
    }

    const uint64_t tenant_max  = it->second.quota_bytes;
    const uint64_t tenant_used = it->second.allocated_bytes;
    const uint64_t tenant_left = (tenant_used < tenant_max) ? tenant_max - tenant_used : 0;

    return (global_left < tenant_left) ? global_left : tenant_left;
}

std::string GPUMemoryManager::GetEditionInfo() const {
    const auto info    = edition::EditionInfo::Get();
    std::string result = "Edition: ";
    result += std::string(info.name);
    result += " | GPU VRAM: ";
    result += std::to_string(info.gpu_max_vram_gb);
    result += "GB | Max Nodes: ";
    result += std::to_string(info.sharding_max_nodes);
    return result;
}

// ============================================================================
// GetGPUFallbackStrategy — free function implementation
// ============================================================================

std::string GetGPUFallbackStrategy() {
    const auto info      = edition::EditionInfo::Get();
    std::string strategy = "GPU memory limit exceeded (";
    strategy += std::to_string(info.gpu_max_vram_gb);
    strategy += "GB for ";
    strategy += std::string(info.name);
    strategy += " edition). Falling back to CPU vector search.";

    if (info.type == edition::EditionType::COMMUNITY) {
        strategy += " Consider Community GPU guidelines or upgrade to Enterprise for higher limits.";
    }
    return strategy;
}

} // namespace gpu
} // namespace themis
