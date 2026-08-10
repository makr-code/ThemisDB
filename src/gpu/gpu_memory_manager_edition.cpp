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
 * ThemisDB | File: gpu_memory_manager_edition.cpp | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 401
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=40, M=4, L=0
 * PR History (last 5): #3624 feat(gpu): Register all src... (2026-03-12) | #3561 docs(gpu): reality-check sr... (2026-03-12) | #240 Replace GPU Memory Manager ... (2026-03-11) | #1278 GPU module: production-read... (2026-03-11)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/*
 * GPU Memory Manager with Edition-Specific Constraints
 * ======================================================
 * Enforces GPU VRAM limits based on edition at runtime.
 * Edition constraints are set at compile-time via CMakeLists.txt.
 */

#include "themis/gpu/memory_manager.h"

namespace themis {
namespace gpu {

// ============================================================================
// Internal helper — must be called with mutex_ already held
// ============================================================================

bool GPUMemoryManager::TryAllocateUnderLock(uint64_t size_bytes, const std::string &tag, const std::string &tenant_id) {
    const uint64_t max_vram = GetMaxGPUVRAMBytes();
    // Hints count against the VRAM budget so that reserved headroom is
    // protected from other callers.
    const uint64_t new_total = gpu_memory_allocated_ + hint_reserved_bytes_ + size_bytes;

    // Check global edition limit.
    if (new_total > max_vram) {
        return false;
    }

    // Check per-tenant quota (if one is registered and non-zero).
    if (!tenant_id.empty()) {
        auto it = tenant_states_.find(tenant_id);
        if (it != tenant_states_.end() && it->second.quota_bytes > 0) {
            if (it->second.allocated_bytes + size_bytes > it->second.quota_bytes) {
                return false;
            }
        }
    }

    AllocationRecord record{size_bytes, tag, tenant_id};
    decltype(&tenant_states_.begin()->second) tenant_state = nullptr;
    if (!tenant_id.empty()) {
        auto [it, inserted] = tenant_states_.try_emplace(tenant_id);
        (void)inserted;
        tenant_state = &it->second;
    }

    active_allocations_.push_back(record);

    // Commit the allocation only after all throwing operations succeeded.
    gpu_memory_allocated_ = new_total;
    if (gpu_memory_allocated_ > peak_bytes_) {
        peak_bytes_ = gpu_memory_allocated_;
    }
    ++allocation_count_;

    if (tenant_state != nullptr) {
        tenant_state->allocated_bytes += size_bytes;
        if (tenant_state->allocated_bytes > tenant_state->peak_bytes) {
            tenant_state->peak_bytes = tenant_state->allocated_bytes;
        }
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

void GPUMemoryManager::CancelHint(uint64_t hint_id) {
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

bool GPUMemoryManager::ConsumeHint(uint64_t hint_id) {
    if (hint_id == 0) {
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = active_hints_.begin(); it != active_hints_.end(); ++it) {
        if (it->id == hint_id) {
            const uint64_t bytes     = it->bytes;
            const std::string tag    = it->tag;
            const std::string tenant = it->tenant_id;
            AllocationRecord record{bytes, tag, tenant};
            decltype(&tenant_states_.begin()->second) tenant_state = nullptr;
            if (!tenant.empty()) {
                auto [tenant_it, inserted] = tenant_states_.try_emplace(tenant);
                (void)inserted;
                tenant_state = &tenant_it->second;
            }

            active_allocations_.push_back(record);

            // Remove from hints only after the allocation record is durable.
            if (hint_reserved_bytes_ >= bytes) {
                hint_reserved_bytes_ -= bytes;
            } else {
                hint_reserved_bytes_ = 0;
            }
            active_hints_.erase(it);

            gpu_memory_allocated_ += bytes;
            if (gpu_memory_allocated_ > peak_bytes_) {
                peak_bytes_ = gpu_memory_allocated_;
            }
            ++allocation_count_;
            if (tenant_state != nullptr) {
                tenant_state->allocated_bytes += bytes;
                if (tenant_state->allocated_bytes > tenant_state->peak_bytes) {
                    tenant_state->peak_bytes = tenant_state->allocated_bytes;
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

void GPUMemoryManager::DeallocateGPU(uint64_t size_bytes) {
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

void GPUMemoryManager::ValidateAllocation(uint64_t size_bytes) {
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
    std::vector<TenantStats> result;
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
