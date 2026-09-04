/**
 * @file active_vram_allocator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 95/100
 * @note Gap Summary: total=5; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=2, Debt=0, C=0, H=13, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "llm/active_vram_allocator.h"
#include "llm/gpu_memory_manager.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <unordered_map>

// Use cudaMemcpy for device↔host transfers when CUDA is available.
#ifdef THEMIS_ENABLE_CUDA
#include <cuda_runtime.h>
#endif

namespace themis {
namespace llm {

namespace {

/// Return current time in milliseconds since epoch.
int64_t nowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(
        steady_clock::now().time_since_epoch()
    ).count();
}

/**
 * @brief Round `n` up to the nearest multiple of `alignment` (must be power-of-two).
 */
size_t alignUp(size_t n, size_t alignment) {
    return (n + alignment - 1) & ~(alignment - 1);
}

/**
 * @brief Copy `bytes` from `src` to `dst`.
 *
 * Uses `cudaMemcpy(DeviceToHost)` / `cudaMemcpy(HostToDevice)` when CUDA is
 * available and the source is a device pointer; falls back to `std::memcpy`
 * in CPU-simulation or non-CUDA builds.
 *
 * @param dst         Destination buffer (CPU or GPU).
 * @param src         Source buffer (GPU or CPU).
 * @param bytes       Number of bytes to copy.
 * @param device_to_host  True for GPU→CPU, false for CPU→GPU.
 * @param gpu_available   True when a real CUDA device is in use.
 */
void copyMemory(void* dst, const void* src, size_t bytes,
                [[maybe_unused]] bool device_to_host,
                [[maybe_unused]] bool gpu_available)
{
    if (bytes == 0 || dst == nullptr || src == nullptr) {
        return;
    }

#ifdef THEMIS_ENABLE_CUDA
    if (gpu_available) {
        cudaMemcpyKind kind = device_to_host
            ? cudaMemcpyDeviceToHost
            : cudaMemcpyHostToDevice;
        cudaError_t err = cudaMemcpy(dst, src, bytes, kind);
        if (err != cudaSuccess) {
            spdlog::warn("[ActiveVRAMAllocator] cudaMemcpy failed: {}",
                         cudaGetErrorString(err));
            // Fall back to std::memcpy as best-effort
            std::memcpy(dst, src, bytes);
        }
        return;
    }
#else
#endif
    std::memcpy(dst, src, bytes);
}

/// Generate a unique internal model key for GPUMemoryManager from owner + id.
std::string makeModelKey(const std::string& owner_id, uint64_t alloc_id) {
    return owner_id + "__avram_" + std::to_string(alloc_id);
}

} // namespace

// =============================================================================
// Impl
// =============================================================================

/** @brief Impl. */
class ActiveVRAMAllocator::Impl {
public:
    explicit Impl(const Config& cfg)
        : cfg_(cfg)
        , next_id_(1)
    {
        // Build GPUMemoryManager config
        GPUMemoryManager::Config gm_cfg;
        gm_cfg.max_vram_bytes   = cfg_.max_vram_bytes;   // 0 → GPUMemoryManager auto-detects
        gm_cfg.max_ram_bytes    = cfg_.max_cpu_spill_bytes;
        gm_cfg.min_free_vram_bytes = cfg_.min_free_vram_reserve;
        gm_cfg.enable_memory_pooling    = true;
        gm_cfg.enable_defragmentation   = cfg_.enable_defragmentation;

        gpu_mgr_ = std::make_unique<GPUMemoryManager>(gm_cfg);

        // Resolve GPU device
        resolved_device_id_ = cfg_.gpu_device_id >= 0
            ? cfg_.gpu_device_id
            : gpu_mgr_->getLeastLoadedGPU();

        gpu_available_ = gpu_mgr_->isGPUAvailable(resolved_device_id_);

        // Cache total VRAM for Stats
        stats_.total_vram_bytes = gpu_mgr_->getTotalVRAM();
        if (stats_.total_vram_bytes == 0 && cfg_.max_vram_bytes > 0) {
            stats_.total_vram_bytes = cfg_.max_vram_bytes;
        }

        spdlog::info("[ActiveVRAMAllocator] Initialized on device {} ({}). "
                     "Total VRAM: {} MB",
                     resolved_device_id_,
                     gpu_available_ ? "GPU" : "CPU-simulation",
                     stats_.total_vram_bytes / (1024 * 1024));
    }

    ~Impl() = default;

    // ------------------------------------------------------------------
    // allocate
    // ------------------------------------------------------------------

    std::optional<AllocationHandle> allocate(
        size_t bytes,
        const std::string& owner_id,
        int gpu_device_id)
    {
        if (bytes == 0) {
            spdlog::warn("[ActiveVRAMAllocator] Attempted to allocate 0 bytes for '{}'",
                         owner_id);
            return std::nullopt;
        }

        std::lock_guard<std::mutex> lock(mu_);

        const size_t aligned = alignUp(bytes, cfg_.block_alignment);
        const int device = (gpu_device_id >= 0) ? gpu_device_id : resolved_device_id_;

        // Check if we would exceed the OOM threshold
        if (stats_.total_vram_bytes > 0) {
            size_t projected_used = stats_.used_vram_bytes + aligned;
            float util = static_cast<float>(projected_used) /
                         static_cast<float>(stats_.total_vram_bytes);
            if (util > cfg_.oom_threshold_fraction) {
                stats_.oom_threshold_exceeded = true;
                spdlog::warn("[ActiveVRAMAllocator] OOM threshold ({:.0f}%) exceeded: "
                             "projected usage {:.1f}%",
                             cfg_.oom_threshold_fraction * 100.0f,
                             util * 100.0f);
            }
        }

        // Attempt real GPU allocation
        void* gpu_ptr = gpu_mgr_->allocateGPU(
            makeModelKey(owner_id, next_id_), aligned, device);

        if (!gpu_ptr) {
            stats_.oom_event_count++;
            spdlog::warn("[ActiveVRAMAllocator] GPU allocation failed for '{}' ({} bytes)",
                         owner_id, aligned);
            notifyOOM({aligned, OOMRecoveryStrategy::Failed, false, 0});
            return std::nullopt;
        }

        AllocationHandle h;
        h.id              = next_id_.fetch_add(1, std::memory_order_relaxed);
        h.owner_id        = owner_id;
        h.requested_bytes = bytes;
        h.allocated_bytes = aligned;
        h.gpu_ptr         = gpu_ptr;
        h.cpu_ptr         = nullptr;
        h.is_spilled      = false;
        h.valid           = true;
        h.allocated_at_ms = nowMs();
        h.last_used_at_ms = h.allocated_at_ms;

        // Track padding waste
        stats_.wasted_padding_bytes += (aligned - bytes);
        stats_.used_vram_bytes      += aligned;
        stats_.live_allocation_count++;
        if (stats_.used_vram_bytes > stats_.peak_vram_bytes) {
            stats_.peak_vram_bytes = stats_.used_vram_bytes;
        }
        updateFreeVRAM();

        allocations_[h.id] = h;

        spdlog::debug("[ActiveVRAMAllocator] Allocated {} bytes for '{}' (id={})",
                      aligned, owner_id, h.id);
        return h;
    }

    // ------------------------------------------------------------------
    // allocateOrRecover
    // ------------------------------------------------------------------

    std::optional<AllocationHandle> allocateOrRecover(
        size_t bytes,
        const std::string& owner_id,
        int gpu_device_id)
    {
        auto result = allocate(bytes, owner_id, gpu_device_id);
        if (result) return result;

        spdlog::info("[ActiveVRAMAllocator] Triggering OOM recovery for '{}' ({} bytes)",
                     owner_id, bytes);
        // Use the public handleOOM() wrapper which acquires mu_ independently.
        // Calling the private handleOOMInternal() here would be a data race
        // because allocate() already released the lock before returning.
        bool recovered = handleOOM(bytes);
        if (!recovered) {
            spdlog::error("[ActiveVRAMAllocator] OOM recovery failed for '{}' ({} bytes)",
                          owner_id, bytes);
            return std::nullopt;
        }

        return allocate(bytes, owner_id, gpu_device_id);
    }

    // ------------------------------------------------------------------
    // free
    // ------------------------------------------------------------------

    bool free(AllocationHandle& handle) {
        if (!handle.valid) {
            spdlog::warn("[ActiveVRAMAllocator] Attempted to free invalid handle {}",
                         handle.id);
            return false;
        }

        std::lock_guard<std::mutex> lock(mu_);
        return freeHandleLocked(handle);
    }

    // ------------------------------------------------------------------
    // touch
    // ------------------------------------------------------------------

    void touch(AllocationHandle& handle) {
        if (!handle.valid) return;
        std::lock_guard<std::mutex> lock(mu_);
        handle.last_used_at_ms = nowMs();
        auto it = allocations_.find(handle.id);
        if (it != allocations_.end()) {
            it->second.last_used_at_ms = handle.last_used_at_ms;
        }
    }

    // ------------------------------------------------------------------
    // handleOOM (public wrapper — takes lock)
    // ------------------------------------------------------------------

    bool handleOOM([[maybe_unused]] size_t need_bytes) {
        std::lock_guard<std::mutex> lock(mu_);
        return handleOOMInternal(need_bytes);
    }

    // ------------------------------------------------------------------
    // evictLRU
    // ------------------------------------------------------------------

    size_t evictLRU() {
        std::lock_guard<std::mutex> lock(mu_);
        return evictLRULocked();
    }

    // ------------------------------------------------------------------
    // evictOwner
    // ------------------------------------------------------------------

    size_t evictOwner(const std::string& owner_id) {
        std::lock_guard<std::mutex> lock(mu_);
        size_t freed = 0;

        std::vector<uint64_t> to_evict;
        for (auto& [id, h] : allocations_) {
            if (h.owner_id == owner_id) {
                to_evict.push_back(id);
            }
        }

        for (uint64_t id : to_evict) {
            auto it = allocations_.find(id);
            if (it == allocations_.end()) continue;
            AllocationHandle& h = it->second;
            freed += h.allocated_bytes;
            freeHandleLocked(h);
        }

        if (freed > 0) {
            spdlog::info("[ActiveVRAMAllocator] Evicted owner '{}': freed {} bytes",
                         owner_id, freed);
            stats_.eviction_count++;
        }
        return freed;
    }

    // ------------------------------------------------------------------
    // defragment
    // ------------------------------------------------------------------

    bool defragment() {
        if (!cfg_.enable_defragmentation) {
            spdlog::debug("[ActiveVRAMAllocator] Defragmentation is disabled");
            return false;
        }

        std::lock_guard<std::mutex> lock(mu_);
        bool result = gpu_mgr_->defragment();

        if (result) {
            stats_.defrag_count++;
            updateFreeVRAM();
            spdlog::info("[ActiveVRAMAllocator] Defragmentation completed");
        }
        return result;
    }

    // ------------------------------------------------------------------
    // spillLRUToCPU
    // ------------------------------------------------------------------

    size_t spillLRUToCPU() {
        if (!cfg_.enable_cpu_spilling) {
            spdlog::debug("[ActiveVRAMAllocator] CPU spilling is disabled");
            return 0;
        }

        std::lock_guard<std::mutex> lock(mu_);
        return spillLRUToCPULocked();
    }

    // ------------------------------------------------------------------
    // restoreFromCPU
    // ------------------------------------------------------------------

    bool restoreFromCPU(AllocationHandle& handle) {
        if (!handle.valid || !handle.is_spilled || !handle.cpu_ptr) {
            return false;
        }

        std::lock_guard<std::mutex> lock(mu_);

        size_t bytes = handle.allocated_bytes;
        void* gpu_ptr = gpu_mgr_->allocateGPU(
            makeModelKey(handle.owner_id, handle.id), bytes, resolved_device_id_);

        if (!gpu_ptr) {
            spdlog::warn("[ActiveVRAMAllocator] restoreFromCPU failed: no VRAM for "
                         "'{}' ({} bytes)", handle.owner_id, bytes);
            return false;
        }

        // Copy data from CPU back to GPU (uses cudaMemcpy when CUDA is available)
        copyMemory(gpu_ptr, handle.cpu_ptr, bytes, /*device_to_host=*/false, gpu_available_);

        // Free the CPU buffer
        static_cast<void>(gpu_mgr_->freeCPU(makeModelKey(handle.owner_id, handle.id), handle.cpu_ptr));

        // Update tracking.
        // Note: live_allocation_count is NOT decremented on spill (the handle
        // remains valid), so we must NOT increment it on restore either.
        stats_.spilled_cpu_bytes -= bytes;
        stats_.used_vram_bytes   += bytes;
        updateFreeVRAM();

        handle.gpu_ptr    = gpu_ptr;
        handle.cpu_ptr    = nullptr;
        handle.is_spilled = false;

        // Sync into our internal map
        auto it = allocations_.find(handle.id);
        if (it != allocations_.end()) {
            it->second = handle;
        } else {
            allocations_[handle.id] = handle;
        }

        spdlog::info("[ActiveVRAMAllocator] Restored '{}' (id={}) from CPU to VRAM",
                     handle.owner_id, handle.id);
        return true;
    }

    // ------------------------------------------------------------------
    // getStats
    // ------------------------------------------------------------------

    Stats getStats() const {
        std::lock_guard<std::mutex> lock(mu_);
        Stats s = stats_;
        // Refresh fragmentation from underlying manager
        s.fragmentation_pct = static_cast<float>(
            gpu_mgr_->getMemoryFragmentation());
        return s;
    }

    bool isOOMThresholdExceeded() const {
        std::lock_guard<std::mutex> lock(mu_);
        return stats_.oom_threshold_exceeded;
    }

    void setOOMCallback([[maybe_unused]] OOMCallback cb) {
        std::lock_guard<std::mutex> lock(mu_);
        oom_cb_ = std::move(cb);
    }

    std::vector<AllocationHandle> listAllocations() const {
        std::lock_guard<std::mutex> lock(mu_);
        std::vector<AllocationHandle> result;
        result.reserve(allocations_.size());
        for (const auto& [id, h] : allocations_) {
            result.push_back(h);
        }
        return result;
    }

    int gpuDeviceId() const noexcept { return resolved_device_id_; }

    bool isGPUAvailable() const noexcept { return gpu_available_; }

    // ------------------------------------------------------------------
    // External-memory registration
    // ------------------------------------------------------------------

    /// Register externally-managed VRAM without allocating any memory.
    AllocationHandle registerExternal(size_t bytes, const std::string& owner_id) {
        std::lock_guard<std::mutex> lock(mu_);

        AllocationHandle h;
        h.id              = next_id_.fetch_add(1, std::memory_order_relaxed);
        h.owner_id        = owner_id;
        h.requested_bytes = bytes;
        h.allocated_bytes = bytes;   // no alignment padding for external allocs
        h.gpu_ptr         = nullptr; // owned by external runtime
        h.cpu_ptr         = nullptr;
        h.is_spilled      = false;
        h.is_external     = true;
        h.valid           = true;
        h.allocated_at_ms = nowMs();
        h.last_used_at_ms = h.allocated_at_ms;

        stats_.used_vram_bytes += bytes;
        stats_.live_allocation_count++;
        if (stats_.used_vram_bytes > stats_.peak_vram_bytes) {
            stats_.peak_vram_bytes = stats_.used_vram_bytes;
        }
        updateFreeVRAM();

        allocations_[h.id] = h;

        spdlog::info("[ActiveVRAMAllocator] Registered external allocation '{}' "
                     "({} MB, id={})",
                     owner_id, bytes / (1024 * 1024), h.id);
        return h;
    }

    // ------------------------------------------------------------------
    // Bridge API (for AdaptiveVRAMAllocator)
    // ------------------------------------------------------------------

    bool allocateWithFragmentation(size_t bytes, void** ptr) {
        if (!ptr) return false;

        auto handle = allocateOrRecover(bytes, "__bridge__", -1);
        if (!handle) {
            *ptr = nullptr;
            return false;
        }

        // Store handle so memory is not leaked; keyed by raw pointer
        std::lock_guard<std::mutex> lock(mu_);
        bridge_handles_[handle->id] = *handle;
        
        // For external allocations, we cannot provide a direct pointer
        // as the memory is managed externally
        if (handle->is_external) {
            *ptr = nullptr;
            return false;
        }
        
        *ptr = handle->is_spilled ? handle->cpu_ptr : handle->gpu_ptr;
        return *ptr != nullptr;
    }

    bool handleOutOfMemory() {
        return handleOOM(0);
    }

private:
    // ------------------------------------------------------------------
    // Internal helpers (must be called with mu_ held)
    // ------------------------------------------------------------------

    bool handleOOMInternal([[maybe_unused]] size_t need_bytes) {
        // Strategy 1: Eviction
        {
            size_t freed = evictLRULocked();
            if (freed > 0) {
                stats_.oom_recovery_count++;
                OOMEvent ev{need_bytes, OOMRecoveryStrategy::Eviction, true, freed};
                notifyOOM(ev);
                if (freed >= need_bytes || gpu_mgr_->getFreeVRAM() >= need_bytes) {
                    spdlog::info("[ActiveVRAMAllocator] OOM recovered via eviction "
                                 "({} bytes freed)", freed);
                    return true;
                }
            }
        }

        // Strategy 2: Defragmentation
        if (cfg_.enable_defragmentation) {
            bool ok = gpu_mgr_->defragment();
            if (ok) {
                stats_.defrag_count++;
                updateFreeVRAM();
                size_t free_after = gpu_mgr_->getFreeVRAM();
                if (free_after >= need_bytes) {
                    stats_.oom_recovery_count++;
                    OOMEvent ev{need_bytes, OOMRecoveryStrategy::Defragmentation,
                                true, free_after};
                    notifyOOM(ev);
                    spdlog::info("[ActiveVRAMAllocator] OOM recovered via defragmentation "
                                 "({} bytes free after)", free_after);
                    return true;
                }
            }
        }

        // Strategy 3: CPU Spilling
        if (cfg_.enable_cpu_spilling) {
            size_t spilled = spillLRUToCPULocked();
            if (spilled > 0) {
                size_t free_after = gpu_mgr_->getFreeVRAM();
                if (free_after >= need_bytes) {
                    stats_.oom_recovery_count++;
                    OOMEvent ev{need_bytes, OOMRecoveryStrategy::CPUSpilling,
                                true, spilled};
                    notifyOOM(ev);
                    spdlog::info("[ActiveVRAMAllocator] OOM recovered via CPU spilling "
                                 "({} bytes spilled)", spilled);
                    return true;
                }
            }
        }

        // All strategies failed
        OOMEvent ev{need_bytes, OOMRecoveryStrategy::Failed, false, 0};
        notifyOOM(ev);
        spdlog::error("[ActiveVRAMAllocator] OOM recovery failed: "
                      "need {} bytes, free {} bytes",
                      need_bytes, gpu_mgr_->getFreeVRAM());
        return false;
    }

    size_t evictLRULocked() {
        if (allocations_.empty()) return 0;

        // Find the non-spilled, non-external allocation with the smallest last_used_at_ms.
        // External allocations are owned by the inference runtime and must not be evicted.
        uint64_t lru_id = 0;
        int64_t  lru_ts = std::numeric_limits<int64_t>::max();

        for (const auto& [id, h] : allocations_) {
            if (!h.is_spilled && !h.is_external && h.last_used_at_ms < lru_ts) {
                lru_ts = h.last_used_at_ms;
                lru_id = id;
            }
        }

        if (lru_id == 0) return 0;  // nothing evictable

        auto it = allocations_.find(lru_id);
        if (it == allocations_.end()) return 0;

        AllocationHandle& h = it->second;
        size_t freed = h.allocated_bytes;

        spdlog::info("[ActiveVRAMAllocator] Evicting LRU allocation id={} owner='{}' "
                     "({} bytes, last used {}ms ago)",
                     h.id, h.owner_id, h.allocated_bytes,
                     nowMs() - h.last_used_at_ms);

        freeHandleLocked(h);
        stats_.eviction_count++;
        return freed;
    }

    size_t spillLRUToCPULocked() {
        if (allocations_.empty()) return 0;

        // Check CPU spill budget
        if (stats_.spilled_cpu_bytes >= cfg_.max_cpu_spill_bytes) {
            spdlog::warn("[ActiveVRAMAllocator] CPU spill budget exhausted ({} / {} bytes)",
                         stats_.spilled_cpu_bytes, cfg_.max_cpu_spill_bytes);
            return 0;
        }

        // Find the LRU non-spilled, non-external allocation.
        // External allocations are owned by the inference runtime and cannot be spilled.
        uint64_t lru_id = 0;
        int64_t  lru_ts = std::numeric_limits<int64_t>::max();

        for (const auto& [id, h] : allocations_) {
            if (!h.is_spilled && !h.is_external && h.last_used_at_ms < lru_ts) {
                lru_ts = h.last_used_at_ms;
                lru_id = id;
            }
        }

        if (lru_id == 0) return 0;

        auto it = allocations_.find(lru_id);
        if (it == allocations_.end()) return 0;

        AllocationHandle& h = it->second;
        size_t bytes = h.allocated_bytes;

        // Allocate CPU buffer
        void* cpu_ptr = gpu_mgr_->allocateCPU(
            makeModelKey(h.owner_id, h.id), bytes, /*pinned=*/true);

        if (!cpu_ptr) {
            spdlog::warn("[ActiveVRAMAllocator] CPU spill: could not allocate {} bytes "
                         "of CPU memory for '{}'", bytes, h.owner_id);
            return 0;
        }

        // Copy GPU→CPU (uses cudaMemcpy when CUDA is available)
        if (h.gpu_ptr) {
            copyMemory(cpu_ptr, h.gpu_ptr, bytes, /*device_to_host=*/true, gpu_available_);
        }

        // Free GPU memory
        static_cast<void>(gpu_mgr_->freeGPU(makeModelKey(h.owner_id, h.id), h.gpu_ptr));

        // Update stats
        stats_.used_vram_bytes -= bytes;
        stats_.spilled_cpu_bytes += bytes;
        stats_.spill_count++;
        // live_allocation_count stays the same — handle is still valid, just spilled
        updateFreeVRAM();

        // Update handle
        h.cpu_ptr    = cpu_ptr;
        h.gpu_ptr    = nullptr;
        h.is_spilled = true;

        spdlog::info("[ActiveVRAMAllocator] Spilled '{}' (id={}) to CPU ({} bytes)",
                     h.owner_id, h.id, bytes);
        return bytes;
    }

    bool freeHandleLocked(AllocationHandle& handle) {
        if (!handle.valid) return false;

        const std::string key = makeModelKey(handle.owner_id, handle.id);

        if (handle.is_external) {
            // External allocation: only update accounting — do NOT touch GPU/CPU memory,
            // as the external owner (e.g., llama.cpp) manages the actual allocation.
            stats_.used_vram_bytes -= std::min(
                handle.allocated_bytes, stats_.used_vram_bytes);
        } else if (handle.is_spilled && handle.cpu_ptr) {
            static_cast<void>(gpu_mgr_->freeCPU(key, handle.cpu_ptr));
            stats_.spilled_cpu_bytes -= std::min(
                handle.allocated_bytes, stats_.spilled_cpu_bytes);
        } else if (handle.gpu_ptr) {
            static_cast<void>(gpu_mgr_->freeGPU(key, handle.gpu_ptr));
            stats_.used_vram_bytes -= std::min(
                handle.allocated_bytes, stats_.used_vram_bytes);
        }

        stats_.wasted_padding_bytes -= std::min(
            handle.allocated_bytes - handle.requested_bytes,
            stats_.wasted_padding_bytes);
        stats_.live_allocation_count -= std::min(
            static_cast<size_t>(1), stats_.live_allocation_count);

        updateFreeVRAM();

        handle.valid   = false;
        handle.gpu_ptr = nullptr;
        handle.cpu_ptr = nullptr;

        allocations_.erase(handle.id);
        // Remove from bridge map too (if it was a bridge allocation).
        bridge_handles_.erase(handle.id);
        return true;
    }

    void updateFreeVRAM() {
        size_t free_vram = gpu_mgr_->getFreeVRAM();
        stats_.free_vram_bytes = free_vram;
        if (stats_.total_vram_bytes > 0) {
            // Use the higher of the two utilization metrics so that externally-registered
            // allocations (which don't consume GPU memory through this allocator) also
            // contribute to OOM pressure detection.
            const float util_actual =
                1.0f - static_cast<float>(free_vram) /
                       static_cast<float>(stats_.total_vram_bytes);
            const float util_accounting =
                static_cast<float>(stats_.used_vram_bytes) /
                static_cast<float>(stats_.total_vram_bytes);
            const float util = std::max(util_actual, util_accounting);
            stats_.oom_threshold_exceeded = (util >= cfg_.oom_threshold_fraction);
        }
    }

    void notifyOOM([[maybe_unused]] const OOMEvent& ev) {
        if (oom_cb_) {
            try { oom_cb_(ev); } catch (...) {}
        }
    }

    // ------------------------------------------------------------------
    // Data members
    // ------------------------------------------------------------------

    Config cfg_;
    std::unique_ptr<GPUMemoryManager> gpu_mgr_;

    int  resolved_device_id_ = 0;
    bool gpu_available_      = false;

    mutable std::mutex mu_;

    std::atomic<uint64_t> next_id_;
    std::unordered_map<uint64_t, AllocationHandle> allocations_;
    std::unordered_map<uint64_t, AllocationHandle> bridge_handles_;

    Stats      stats_{};
    OOMCallback oom_cb_;
};

// =============================================================================
// ActiveVRAMAllocator — public API delegating to Impl
// =============================================================================

ActiveVRAMAllocator::ActiveVRAMAllocator(const Config& cfg)
    : impl_(std::make_unique<Impl>(cfg))
{}

ActiveVRAMAllocator::ActiveVRAMAllocator()
    : impl_(std::make_unique<Impl>(Config{}))
{}

ActiveVRAMAllocator::~ActiveVRAMAllocator() = default;

ActiveVRAMAllocator::ActiveVRAMAllocator(ActiveVRAMAllocator&&) noexcept = default;
ActiveVRAMAllocator& ActiveVRAMAllocator::operator=(ActiveVRAMAllocator&&) noexcept = default;

std::optional<ActiveVRAMAllocator::AllocationHandle>
ActiveVRAMAllocator::allocate(size_t bytes, const std::string& owner_id, int gpu_device_id)
{
    return impl_->allocate(bytes, owner_id, gpu_device_id);
}

std::optional<ActiveVRAMAllocator::AllocationHandle>
ActiveVRAMAllocator::allocateOrRecover(size_t bytes, const std::string& owner_id, int gpu_device_id)
{
    return impl_->allocateOrRecover(bytes, owner_id, gpu_device_id);
}

bool ActiveVRAMAllocator::free(AllocationHandle& handle)
{
    return impl_->free(handle);
}

void ActiveVRAMAllocator::touch(AllocationHandle& handle)
{
    impl_->touch(handle);
}

bool ActiveVRAMAllocator::handleOOM([[maybe_unused]] size_t need_bytes)
{
    return impl_->handleOOM(need_bytes);
}

size_t ActiveVRAMAllocator::evictLRU()
{
    return impl_->evictLRU();
}

size_t ActiveVRAMAllocator::evictOwner(const std::string& owner_id)
{
    return impl_->evictOwner(owner_id);
}

bool ActiveVRAMAllocator::defragment()
{
    return impl_->defragment();
}

size_t ActiveVRAMAllocator::spillLRUToCPU()
{
    return impl_->spillLRUToCPU();
}

bool ActiveVRAMAllocator::restoreFromCPU(AllocationHandle& handle)
{
    return impl_->restoreFromCPU(handle);
}

ActiveVRAMAllocator::Stats ActiveVRAMAllocator::getStats() const
{
    return impl_->getStats();
}

bool ActiveVRAMAllocator::isOOMThresholdExceeded() const
{
    return impl_->isOOMThresholdExceeded();
}

void ActiveVRAMAllocator::setOOMCallback(OOMCallback cb)
{
    impl_->setOOMCallback([[maybe_unused]] std::move(cb));
}

std::vector<ActiveVRAMAllocator::AllocationHandle>
ActiveVRAMAllocator::listAllocations() const
{
    return impl_->listAllocations();
}

int ActiveVRAMAllocator::gpuDeviceId() const noexcept
{
    return impl_->gpuDeviceId();
}

bool ActiveVRAMAllocator::isGPUAvailable() const noexcept
{
    return impl_->isGPUAvailable();
}

ActiveVRAMAllocator::AllocationHandle
ActiveVRAMAllocator::registerExternal(size_t bytes, const std::string& owner_id)
{
    return impl_->registerExternal(bytes, owner_id);
}

bool ActiveVRAMAllocator::allocateWithFragmentation(size_t bytes, void** ptr)
{
    return impl_->allocateWithFragmentation(bytes, ptr);
}

bool ActiveVRAMAllocator::handleOutOfMemory()
{
    return impl_->handleOutOfMemory();
}

} // namespace llm
} // namespace themis

