/**
 * @file gpu_memory_oversubscription.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "index/gpu_memory_oversubscription.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <list>
#include <mutex>
#include <unordered_map>
#include "utils/logger.h"

namespace themis {
namespace index {

// =============================================================================
// GPUMemoryOversubscriptionManager::Impl
// =============================================================================

/** @brief GPUMemoryOversubscriptionManager::Impl. */
class GPUMemoryOversubscriptionManager::Impl {
public:
    // -----------------------------------------------------------------------
    // Internal partition record
    // -----------------------------------------------------------------------

    struct Partition {
        size_t id         = 0;
        size_t num_vectors = 0;
        size_t dimension  = 0;
        std::string tag;

        // Host-side copy (always valid while the partition exists).
        std::vector<float> host_data;

        // VRAM state.  On GPU builds vram_ptr is the cudaMallocManaged /
        // hipMallocManaged pointer; on CPU builds it aliases host_data.data().
        void* vram_ptr    = nullptr;
        bool  in_vram     = false;

        // Access tracking.
        uint64_t last_access_ns = 0;
        size_t   access_count   = 0;
    };

    // -----------------------------------------------------------------------
    // Members
    // -----------------------------------------------------------------------

    Config config;

    // All partitions keyed by their ID.
    std::unordered_map<size_t, Partition> partitions;
    size_t next_id = 0;

    // Insertion-order list so sequential prefetch can find "N+1".
    std::vector<size_t> insertion_order;

    // LRU management for VRAM-resident (hot) partitions.
    // Front of the list  = MRU (most recently used).
    // Back  of the list  = LRU (candidate for eviction).
    std::list<size_t>                                  lru_list;
    std::unordered_map<size_t, std::list<size_t>::iterator> lru_map;

    // Counters.
    size_t vram_used_bytes     = 0;
    size_t host_ram_used_bytes = 0;
    size_t evictions           = 0;
    size_t loads               = 0;
    size_t prefetch_requests   = 0;
    size_t prefetch_hits       = 0;

    mutable std::mutex mutex;

    // -----------------------------------------------------------------------
    // Constructor
    // -----------------------------------------------------------------------

    explicit Impl(const Config& cfg) : config(cfg) {}

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

    static uint64_t nowNs() {
        using namespace std::chrono;
        return static_cast<uint64_t>(
            duration_cast<nanoseconds>(steady_clock::now().time_since_epoch())
                .count());
    }

    size_t vramBudgetBytes() const {
        return config.vram_budget_mb > 0
                   ? config.vram_budget_mb * 1024ULL * 1024ULL
                   : SIZE_MAX;
    }

    static size_t partitionBytes(const Partition& p) {
        return p.num_vectors * p.dimension * sizeof(float);
    }

    // -----------------------------------------------------------------------
    // Internal: evict one partition from VRAM (caller holds mutex).
    // -----------------------------------------------------------------------

    void evictPartitionLocked(Partition& p) {
        if (!p.in_vram) return;

        const size_t bytes = partitionBytes(p);

#if defined(THEMIS_ENABLE_CUDA) || defined(THEMIS_ENABLE_HIP)
        // Free the GPU-managed allocation only when we actually allocated it
        // (i.e. it does not alias the host_data pointer).
        if (p.vram_ptr && p.vram_ptr != static_cast<void*>(p.host_data.data())) {
            themis::gpu::GPUUnifiedMemoryAllocator::GetInstance().free(p.vram_ptr);
        }
#endif
        p.vram_ptr = nullptr;
        p.in_vram  = false;

        vram_used_bytes = (vram_used_bytes >= bytes) ? vram_used_bytes - bytes : 0;
        ++evictions;
    }

    // -----------------------------------------------------------------------
    // Internal: evict the LRU hot partition (caller holds mutex).
    // -----------------------------------------------------------------------

    void evictLRULocked() {
        if (lru_list.empty()) return;

        const size_t lru_id = lru_list.back();
        lru_list.pop_back();
        lru_map.erase(lru_id);

        auto it = partitions.find(lru_id);
        if (it != partitions.end()) {
            evictPartitionLocked(it->second);
        }
    }

    // -----------------------------------------------------------------------
    // Internal: ensure the VRAM budget has room for `needed_bytes`.
    // Evicts LRU partitions until enough room is available or no more hot
    // partitions exist.  Returns true when room was found.
    // -----------------------------------------------------------------------

    bool ensureVRAMRoom(size_t needed_bytes) {
        const size_t budget = vramBudgetBytes();
        if (budget == SIZE_MAX) return true;  // Unlimited budget.

        while (vram_used_bytes + needed_bytes > budget && !lru_list.empty()) {
            evictLRULocked();
        }

        return (vram_used_bytes + needed_bytes <= budget);
    }

    // -----------------------------------------------------------------------
    // Internal: load a partition into VRAM (caller holds mutex).
    // -----------------------------------------------------------------------

    bool loadPartitionLocked(Partition& p) {
        if (p.in_vram) return true;

        const size_t bytes = partitionBytes(p);
        if (!ensureVRAMRoom(bytes)) {
            // Budget exhausted even after evictions.
            THEMIS_ERROR("GPUMemoryOversubscriptionManager: VRAM budget exhausted; "
                         "cannot load partition {} ({} KiB required)",
                         p.id, bytes / 1024);
            return false;
        }

#if defined(THEMIS_ENABLE_CUDA) || defined(THEMIS_ENABLE_HIP)
        if (config.use_unified_memory &&
            themis::gpu::GPUUnifiedMemoryAllocator::isSupported()) {
            // Allocate managed memory and copy from host.
            const std::string alloc_tag =
                "oversubscription_partition_" + std::to_string(p.id);
            void* ptr = themis::gpu::GPUUnifiedMemoryAllocator::GetInstance()
                            .allocate(bytes, alloc_tag);
            if (ptr) {
                std::memcpy(ptr, p.host_data.data(), bytes);
                // Hint the runtime to migrate pages to GPU 0 before the kernel.
                themis::gpu::GPUUnifiedMemoryAllocator::GetInstance()
                    .prefetch(ptr, bytes, 0);
                p.vram_ptr = ptr;
            } else {
                // Allocation failed; fall through to host pointer.
                p.vram_ptr = p.host_data.data();
            }
        } else {
            // GPU is present but unified memory is off: alias host data.
            p.vram_ptr = p.host_data.data();
        }
#else
        // CPU-only build: alias host data (no actual migration).
        p.vram_ptr = p.host_data.data();
#endif

        p.in_vram = true;
        vram_used_bytes += bytes;
        ++loads;
        return true;
    }

    // -----------------------------------------------------------------------
    // Internal: update LRU position (caller holds mutex).
    // -----------------------------------------------------------------------

    void touchLRULocked(size_t partition_id) {
        // Iterator Safety (A-2.1): Ensure safe iterator handling
        // - Cache iterator before mutation
        // - Re-fetch after container modifications
        auto it = lru_map.find(partition_id);
        if (it != lru_map.end()) {
            // Erase the existing entry; this only invalidates iterators to the erased element
            lru_list.erase(it->second);
        }
        // Push to front and update map with new iterator (safe: push_front doesn't invalidate)
        lru_list.push_front(partition_id);
        lru_map[partition_id] = lru_list.begin();  // lru_list.begin() is always valid after push_front
    }

    // -----------------------------------------------------------------------
    // Internal: index of a partition in insertion_order (-1 if not found).
    // -----------------------------------------------------------------------

    ptrdiff_t insertionIndex(size_t partition_id) const {
        for (size_t i = 0; i < insertion_order.size(); ++i) {
            if (insertion_order[i] == partition_id) {
                return static_cast<ptrdiff_t>(i);
            }
        }
        return -1;
    }

    // -----------------------------------------------------------------------
    // Internal: apply prefetch strategy after accessing `accessed_id`.
    // Caller holds mutex.
    // -----------------------------------------------------------------------

    void applyPrefetchLocked(size_t accessed_id) {
        if (config.prefetch_strategy == PrefetchStrategy::NONE) return;

        size_t target_id = SIZE_MAX;

        switch (config.prefetch_strategy) {
        case PrefetchStrategy::SEQUENTIAL: {
            // Prefetch the partition inserted immediately after accessed_id.
            ptrdiff_t idx = insertionIndex(accessed_id);
            if (idx >= 0 &&
                static_cast<size_t>(idx + 1) < insertion_order.size()) {
                target_id = insertion_order[static_cast<size_t>(idx + 1)];
            }
            break;
        }
        case PrefetchStrategy::LRU: {
            // Prefetch the cold partition that was evicted most recently
            // (back of the LRU list that is still present but cold, or the
            // first cold partition not yet in the LRU list).
            // Walk from back of lru_list → find a cold entry.
            for (auto it = lru_list.rbegin(); it != lru_list.rend(); ++it) {
                if (*it == accessed_id) continue;
                auto pit = partitions.find(*it);
                if (pit != partitions.end() && !pit->second.in_vram) {
                    target_id = *it;
                    break;
                }
            }
            // Fall back: find any cold partition not currently in LRU.
            if (target_id == SIZE_MAX) {
                for (const size_t pid : insertion_order) {
                    if (pid == accessed_id) continue;
                    auto pit = partitions.find(pid);
                    if (pit != partitions.end() && !pit->second.in_vram &&
                        lru_map.find(pid) == lru_map.end()) {
                        target_id = pid;
                        break;
                    }
                }
            }
            break;
        }
        case PrefetchStrategy::MRU: {
            // Prefetch the cold partition that was most recently accessed
            // (front of the LRU list that is cold).
            for (auto it = lru_list.begin(); it != lru_list.end(); ++it) {
                if (*it == accessed_id) continue;
                auto pit = partitions.find(*it);
                if (pit != partitions.end() && !pit->second.in_vram) {
                    target_id = *it;
                    break;
                }
            }
            break;
        }
        default:
            break;
        }

        if (target_id == SIZE_MAX) return;

        auto pit = partitions.find(target_id);
        if (pit == partitions.end()) return;

        ++prefetch_requests;

        if (pit->second.in_vram) {
            ++prefetch_hits;
            return;
        }

        // Only prefetch if we have budget slack (don't evict for background prefetch).
        const size_t bytes = partitionBytes(pit->second);
        const size_t budget = vramBudgetBytes();
        if (budget != SIZE_MAX && vram_used_bytes + bytes > budget) {
            return;  // No room without evicting — skip best-effort prefetch.
        }

        (void)loadPartitionLocked(pit->second);
    }
};

// =============================================================================
// GPUMemoryOversubscriptionManager public API
// =============================================================================

GPUMemoryOversubscriptionManager::GPUMemoryOversubscriptionManager()
    : GPUMemoryOversubscriptionManager(Config{}) {}

GPUMemoryOversubscriptionManager::GPUMemoryOversubscriptionManager(
    const Config& config)
    : pImpl_(std::make_unique<Impl>(config)) {}

GPUMemoryOversubscriptionManager::~GPUMemoryOversubscriptionManager() {
    // Evict all hot partitions to release unified-memory allocations.
    if (!pImpl_) return;
    std::lock_guard<std::mutex> lk(pImpl_->mutex);
    for (auto& [id, p] : pImpl_->partitions) {
        pImpl_->evictPartitionLocked(p);
    }
}

// ---------------------------------------------------------------------------
// addPartition
// ---------------------------------------------------------------------------

size_t GPUMemoryOversubscriptionManager::addPartition(
    const std::vector<float>& flat_data,
    size_t num_vectors,
    size_t dimension,
    const std::string& tag) {

    std::lock_guard<std::mutex> lk(pImpl_->mutex);

    const size_t id = pImpl_->next_id++;

    Impl::Partition p;
    p.id          = id;
    p.num_vectors = num_vectors;
    p.dimension   = dimension;
    p.tag         = tag;
    p.host_data   = flat_data;  // Copy to host RAM.

    const size_t bytes = num_vectors * dimension * sizeof(float);
    pImpl_->host_ram_used_bytes += bytes;
    pImpl_->insertion_order.push_back(id);
    pImpl_->partitions.emplace(id, std::move(p));
    return id;
}

// ---------------------------------------------------------------------------
// removePartition
// ---------------------------------------------------------------------------

bool GPUMemoryOversubscriptionManager::removePartition(size_t partition_id) {
    std::lock_guard<std::mutex> lk(pImpl_->mutex);

    auto it = pImpl_->partitions.find(partition_id);
    if (it == pImpl_->partitions.end()) return false;

    Impl::Partition& p = it->second;

    if (p.in_vram) {
        pImpl_->evictPartitionLocked(p);
    }

    const size_t bytes = Impl::partitionBytes(p);
    pImpl_->host_ram_used_bytes =
        (pImpl_->host_ram_used_bytes >= bytes)
            ? pImpl_->host_ram_used_bytes - bytes
            : 0;

    // Remove from LRU structures.
    auto lm_it = pImpl_->lru_map.find(partition_id);
    if (lm_it != pImpl_->lru_map.end()) {
        pImpl_->lru_list.erase(lm_it->second);
        pImpl_->lru_map.erase(lm_it);
    }

    // Remove from insertion order.
    auto& io = pImpl_->insertion_order;
    io.erase(std::remove(io.begin(), io.end(), partition_id), io.end());

    pImpl_->partitions.erase(it);
    return true;
}

// ---------------------------------------------------------------------------
// accessPartition
// ---------------------------------------------------------------------------

bool GPUMemoryOversubscriptionManager::accessPartition(size_t partition_id) {
    std::lock_guard<std::mutex> lk(pImpl_->mutex);

    auto it = pImpl_->partitions.find(partition_id);
    if (it == pImpl_->partitions.end()) return false;

    Impl::Partition& p = it->second;

    // Load if not already hot.
    if (!p.in_vram) {
        if (!pImpl_->loadPartitionLocked(p)) return false;
    }

    // Update LRU position and access metadata.
    pImpl_->touchLRULocked(partition_id);
    p.last_access_ns = Impl::nowNs();
    ++p.access_count;

    // Apply prefetch strategy.
    pImpl_->applyPrefetchLocked(partition_id);

    return true;
}

// ---------------------------------------------------------------------------
// evictPartition
// ---------------------------------------------------------------------------

bool GPUMemoryOversubscriptionManager::evictPartition(size_t partition_id) {
    std::lock_guard<std::mutex> lk(pImpl_->mutex);

    auto it = pImpl_->partitions.find(partition_id);
    if (it == pImpl_->partitions.end()) return false;

    Impl::Partition& p = it->second;
    if (!p.in_vram) return false;

    // Remove from LRU.
    auto lm_it = pImpl_->lru_map.find(partition_id);
    if (lm_it != pImpl_->lru_map.end()) {
        pImpl_->lru_list.erase(lm_it->second);
        pImpl_->lru_map.erase(lm_it);
    }

    pImpl_->evictPartitionLocked(p);
    return true;
}

// ---------------------------------------------------------------------------
// getPartitionData
// ---------------------------------------------------------------------------

const std::vector<float>* GPUMemoryOversubscriptionManager::getPartitionData(
    size_t partition_id) const {
    std::lock_guard<std::mutex> lk(pImpl_->mutex);

    auto it = pImpl_->partitions.find(partition_id);
    if (it == pImpl_->partitions.end()) return nullptr;
    return &it->second.host_data;
}

// ---------------------------------------------------------------------------
// getPartitionVectorCount
// ---------------------------------------------------------------------------

size_t GPUMemoryOversubscriptionManager::getPartitionVectorCount(
    size_t partition_id) const {
    std::lock_guard<std::mutex> lk(pImpl_->mutex);

    auto it = pImpl_->partitions.find(partition_id);
    if (it == pImpl_->partitions.end()) return 0;
    return it->second.num_vectors;
}

// ---------------------------------------------------------------------------
// isPartitionInVRAM
// ---------------------------------------------------------------------------

bool GPUMemoryOversubscriptionManager::isPartitionInVRAM(
    size_t partition_id) const {
    std::lock_guard<std::mutex> lk(pImpl_->mutex);

    auto it = pImpl_->partitions.find(partition_id);
    if (it == pImpl_->partitions.end()) return false;
    return it->second.in_vram;
}

// ---------------------------------------------------------------------------
// getHotPartitions
// ---------------------------------------------------------------------------

std::vector<size_t> GPUMemoryOversubscriptionManager::getHotPartitions() const {
    std::lock_guard<std::mutex> lk(pImpl_->mutex);

    std::vector<size_t> result;
    result.reserve(pImpl_->lru_list.size());
    for (const size_t pid : pImpl_->lru_list) {
        auto it = pImpl_->partitions.find(pid);
        if (it != pImpl_->partitions.end() && it->second.in_vram) {
            result.push_back(pid);
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// getColdPartitions
// ---------------------------------------------------------------------------

std::vector<size_t> GPUMemoryOversubscriptionManager::getColdPartitions() const {
    std::lock_guard<std::mutex> lk(pImpl_->mutex);

    std::vector<size_t> result;
    for (const size_t pid : pImpl_->insertion_order) {
        auto it = pImpl_->partitions.find(pid);
        if (it != pImpl_->partitions.end() && !it->second.in_vram) {
            result.push_back(pid);
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// getAllPartitionIds
// ---------------------------------------------------------------------------

std::vector<size_t> GPUMemoryOversubscriptionManager::getAllPartitionIds() const {
    std::lock_guard<std::mutex> lk(pImpl_->mutex);
    return pImpl_->insertion_order;
}

// ---------------------------------------------------------------------------
// prefetchPartition
// ---------------------------------------------------------------------------

void GPUMemoryOversubscriptionManager::prefetchPartition(size_t partition_id) {
    std::lock_guard<std::mutex> lk(pImpl_->mutex);

    auto it = pImpl_->partitions.find(partition_id);
    if (it == pImpl_->partitions.end()) return;

    ++pImpl_->prefetch_requests;

    if (it->second.in_vram) {
        ++pImpl_->prefetch_hits;
        return;
    }

    // Only prefetch if VRAM budget allows without eviction.
    const size_t bytes  = Impl::partitionBytes(it->second);
    const size_t budget = pImpl_->vramBudgetBytes();
    if (budget != SIZE_MAX && pImpl_->vram_used_bytes + bytes > budget) {
        return;
    }

    pImpl_->loadPartitionLocked(it->second);
}

// ---------------------------------------------------------------------------
// setPrefetchStrategy
// ---------------------------------------------------------------------------

void GPUMemoryOversubscriptionManager::setPrefetchStrategy(
    PrefetchStrategy strategy) {
    std::lock_guard<std::mutex> lk(pImpl_->mutex);
    pImpl_->config.prefetch_strategy = strategy;
}

// ---------------------------------------------------------------------------
// getPrefetchStrategy
// ---------------------------------------------------------------------------

PrefetchStrategy GPUMemoryOversubscriptionManager::getPrefetchStrategy() const {
    std::lock_guard<std::mutex> lk(pImpl_->mutex);
    return pImpl_->config.prefetch_strategy;
}

// ---------------------------------------------------------------------------
// setVRAMBudgetMB
// ---------------------------------------------------------------------------

void GPUMemoryOversubscriptionManager::setVRAMBudgetMB(size_t mb) {
    std::lock_guard<std::mutex> lk(pImpl_->mutex);
    pImpl_->config.vram_budget_mb = mb;

    // Enforce the new (potentially smaller) budget.
    if (mb > 0) {
        const size_t budget = mb * 1024ULL * 1024ULL;
        while (pImpl_->vram_used_bytes > budget && !pImpl_->lru_list.empty()) {
            pImpl_->evictLRULocked();
        }
    }
}

// ---------------------------------------------------------------------------
// getVRAMBudgetBytes
// ---------------------------------------------------------------------------

size_t GPUMemoryOversubscriptionManager::getVRAMBudgetBytes() const {
    std::lock_guard<std::mutex> lk(pImpl_->mutex);
    return pImpl_->config.vram_budget_mb > 0
               ? pImpl_->config.vram_budget_mb * 1024ULL * 1024ULL
               : 0;  // 0 signals "unlimited" to callers.
}

// ---------------------------------------------------------------------------
// getVRAMUsedBytes
// ---------------------------------------------------------------------------

size_t GPUMemoryOversubscriptionManager::getVRAMUsedBytes() const {
    std::lock_guard<std::mutex> lk(pImpl_->mutex);
    return pImpl_->vram_used_bytes;
}

// ---------------------------------------------------------------------------
// getStats
// ---------------------------------------------------------------------------

GPUMemoryOversubscriptionManager::Stats
GPUMemoryOversubscriptionManager::getStats() const {
    std::lock_guard<std::mutex> lk(pImpl_->mutex);

    Stats s;
    s.total_partitions    = pImpl_->partitions.size();
    s.hot_partitions      = 0;
    s.cold_partitions     = 0;
    for (const auto& [id, p] : pImpl_->partitions) {
        if (p.in_vram) ++s.hot_partitions;
        else           ++s.cold_partitions;
    }
    s.vram_used_bytes     = pImpl_->vram_used_bytes;
    s.host_ram_used_bytes = pImpl_->host_ram_used_bytes;
    s.vram_budget_bytes   = pImpl_->config.vram_budget_mb > 0
                                ? pImpl_->config.vram_budget_mb * 1024ULL * 1024ULL
                                : 0;
    s.evictions           = pImpl_->evictions;
    s.loads               = pImpl_->loads;
    s.prefetch_requests   = pImpl_->prefetch_requests;
    s.prefetch_hits       = pImpl_->prefetch_hits;
    s.prefetch_hit_rate   = (pImpl_->prefetch_requests > 0)
                                ? static_cast<double>(pImpl_->prefetch_hits) /
                                      static_cast<double>(pImpl_->prefetch_requests)
                                : 0.0;
    return s;
}

// ---------------------------------------------------------------------------
// getPartitionInfo
// ---------------------------------------------------------------------------

GPUMemoryOversubscriptionManager::PartitionInfo
GPUMemoryOversubscriptionManager::getPartitionInfo(size_t partition_id) const {
    std::lock_guard<std::mutex> lk(pImpl_->mutex);

    auto it = pImpl_->partitions.find(partition_id);
    if (it == pImpl_->partitions.end()) {
        PartitionInfo pi;
        pi.partition_id = SIZE_MAX;
        return pi;
    }

    const Impl::Partition& p = it->second;
    PartitionInfo pi;
    pi.partition_id   = p.id;
    pi.num_vectors    = p.num_vectors;
    pi.dimension      = p.dimension;
    pi.in_vram        = p.in_vram;
    pi.last_access_ns = p.last_access_ns;
    pi.access_count   = p.access_count;
    pi.tag            = p.tag;
    return pi;
}

// ---------------------------------------------------------------------------
// partitionCount
// ---------------------------------------------------------------------------

size_t GPUMemoryOversubscriptionManager::partitionCount() const {
    std::lock_guard<std::mutex> lk(pImpl_->mutex);
    return pImpl_->partitions.size();
}

} // namespace index
} // namespace themis
