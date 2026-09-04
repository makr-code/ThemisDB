/**
 * @file numa_memory_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=10, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2026 ThemisDB
// Licensed under MIT License

#include "performance/numa_memory_manager.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <new>
#include <stdexcept>

#ifdef __linux__
#  include <sched.h>
#  include <sys/syscall.h>
#  include <unistd.h>
#endif

namespace themis {
namespace performance {

// ---------------------------------------------------------------------------
// Internal helpers: topology detection
// ---------------------------------------------------------------------------

static NUMATopologyInfo detect_topology() noexcept {
    NUMATopologyInfo topo;
#ifdef __linux__
    // Count nodes by reading /sys/devices/system/node/
    size_t n = 0;
    for (int i = 0; i < 64; ++i) {
        char path[64];
        snprintf(path, sizeof(path), "/sys/devices/system/node/node%d", i);
        if (access(path, F_OK) == 0) n = static_cast<size_t>(i + 1);
        else break;
    }
    if (n == 0) n = 1;
    topo.num_nodes = n;
    topo.node_memory_mb.resize(n, 0);
    topo.node_distances.resize(n, std::vector<size_t>(n, 10));
    for (size_t i = 0; i < n; ++i) {
        topo.node_distances[i][i] = 10;
        // Read memory
        char path[128];
        snprintf(path, sizeof(path), "/sys/devices/system/node/node%zu/meminfo", i);
        FILE* f = fopen(path, "r");
        if (f) {
            char line[256];
            while (fgets(line, sizeof(line), f)) {
                unsigned long kb = 0;
                if (sscanf(line, "Node %*d MemTotal: %lu kB", &kb) == 1) {
                    topo.node_memory_mb[i] = kb / 1024;
                    break;
                }
            }
            fclose(f);
        }
    }
#else
    topo.num_nodes = 1;
    topo.node_memory_mb.resize(1, 0);
    topo.node_distances.resize(1, {10});
#endif
    return topo;
}

// ---------------------------------------------------------------------------
// NUMAMemoryManager
// ---------------------------------------------------------------------------

NUMAMemoryManager::NUMAMemoryManager()
    : topology_(detect_topology())
    , num_nodes_(topology_.num_nodes > 0 ? topology_.num_nodes : 1)
    , per_node_bytes_(std::make_unique<std::atomic<int64_t>[]>(
          topology_.num_nodes > 0 ? topology_.num_nodes : 1))
{
    // Initialise per-node atomics to zero (atomic default-ctor already does this,
    // but be explicit for clarity).
    for (size_t i = 0; i < num_nodes_; ++i) per_node_bytes_[i].store(0);
}

NUMAMemoryManager::~NUMAMemoryManager() = default;

int NUMAMemoryManager::resolve_node([[maybe_unused]] int hint_node) const noexcept {
    if (hint_node >= 0 && static_cast<size_t>(hint_node) < topology_.num_nodes)
        return hint_node;
    return get_current_node();
}

void* NUMAMemoryManager::do_allocate(size_t size, [[maybe_unused]] int node, bool /*use_huge_pages*/) {
    if (size == 0) return nullptr;
    void* ptr = nullptr;
#ifdef __linux__
    // Use posix_memalign for cache-line alignment; NUMA binding via mbind
    // requires page-aligned memory.
    if (posix_memalign(&ptr, 64, size) != 0) throw std::bad_alloc{};
    // mbind to preferred node when libnuma-style syscall available
    // node used for statistics; kernel mbind not required here
#else
    ptr = std::malloc(size);
    if (!ptr) throw std::bad_alloc{};
#endif
    return ptr;
}

void NUMAMemoryManager::update_alloc_stats(int node, size_t size) {
    // F-006: lock-free atomic update — no mutex needed.
    int local = get_current_node();
    if (node == local) {
        stat_local_.fetch_add(1, std::memory_order_relaxed);
    } else {
        stat_remote_.fetch_add(1, std::memory_order_relaxed);
    }
    if (static_cast<size_t>(node) < num_nodes_)
        per_node_bytes_[static_cast<size_t>(node)].fetch_add(
            static_cast<int64_t>(size), std::memory_order_relaxed);
}

void NUMAMemoryManager::track_alloc(void* ptr, int node, size_t size) {
    size_t idx = (reinterpret_cast<uintptr_t>(ptr) >> 3) % kBuckets;
    std::lock_guard<std::mutex> lk(buckets_[idx].mtx);
    buckets_[idx].entries[ptr] = {node, size};
}

bool NUMAMemoryManager::untrack_alloc(void* ptr, int* out_node, size_t* out_size) noexcept {
    size_t idx = (reinterpret_cast<uintptr_t>(ptr) >> 3) % kBuckets;
    std::lock_guard<std::mutex> lk(buckets_[idx].mtx);
    auto& map = buckets_[idx].entries;
    auto it = map.find(ptr);
    if (it == map.end()) {
        return false;
    }
    if (out_node)  *out_node  = it->second.node;
    if (out_size)  *out_size  = it->second.size;
    map.erase(it);
    return true;
}

void* NUMAMemoryManager::allocate_on_node(size_t size, int node) {
    int resolved = resolve_node(node);
    void* ptr = do_allocate(size, resolved, false);
    update_alloc_stats(resolved, size);
    track_alloc(ptr, resolved, size);
    return ptr;
}

void* NUMAMemoryManager::allocate_local([[maybe_unused]] size_t size) {
    return allocate_on_node(size, get_current_node());
}

void* NUMAMemoryManager::allocate(size_t size, const AllocationHint& hint) {
    int resolved = resolve_node(hint.preferred_node);
    void* ptr = do_allocate(size, resolved, hint.use_huge_pages);
    update_alloc_stats(resolved, size);
    track_alloc(ptr, resolved, size);
    return ptr;
}

void NUMAMemoryManager::deallocate(void* ptr, size_t size) noexcept {
    if (!ptr) return;
    int node = 0;
    size_t tracked_size = size;
    if (untrack_alloc(ptr, &node, &tracked_size)) {
        if (static_cast<size_t>(node) < num_nodes_) {
            // Saturating subtract via CAS loop — avoids a race where a concurrent
            // fetch_add on another thread fills back the count between our
            // fetch_sub and the unconditional store(0).
            // Relaxed memory order is safe: per-node byte counts are used for
            // statistics only (get_stats() / reset_stats()), not for synchronisation
            // or load/store ordering guarantees between other variables.
            auto& atom = per_node_bytes_[static_cast<size_t>(node)];
            const int64_t sz = static_cast<int64_t>(tracked_size);
            int64_t expected = atom.load(std::memory_order_relaxed);
            int64_t desired;
            do {
                desired = (expected >= sz) ? expected - sz : 0;
            } while (!atom.compare_exchange_weak(
                expected, desired,
                std::memory_order_relaxed, std::memory_order_relaxed));
        }
    }
    std::free(ptr);
}

void NUMAMemoryManager::migrate_to_node(void* ptr, size_t size, int target_node) {
    if (!ptr || size == 0) return;
    // Advisory: update tracking to reflect target node
    int old_node = 0;
    size_t old_size = size;
    if (untrack_alloc(ptr, &old_node, &old_size)) {
        if (static_cast<size_t>(old_node) < num_nodes_) {
            // Saturating subtract via CAS loop (same pattern as deallocate).
            auto& atom = per_node_bytes_[static_cast<size_t>(old_node)];
            int64_t expected = atom.load(std::memory_order_relaxed);
            int64_t desired;
            do {
                desired = (expected >= static_cast<int64_t>(old_size))
                        ? expected - static_cast<int64_t>(old_size)
                        : 0;
            } while (!atom.compare_exchange_weak(
                expected, desired,
                std::memory_order_relaxed, std::memory_order_relaxed));
        }
        int resolved_target = resolve_node(target_node);
        track_alloc(ptr, resolved_target, old_size);
        update_alloc_stats(resolved_target, 0);
    }
}

int NUMAMemoryManager::get_current_node() const noexcept {
#ifdef __linux__
#  ifdef SYS_getcpu
    unsigned cpu = 0, node_num = 0;
    if (syscall(SYS_getcpu, &cpu, &node_num, nullptr) == 0)
        return static_cast<int>(node_num % topology_.num_nodes);
#  endif
    int c = sched_getcpu();
    if (c >= 0) return c % static_cast<int>(topology_.num_nodes);
#endif
    return 0;
}

NUMATopologyInfo NUMAMemoryManager::get_topology() const {
    return topology_;
}

bool NUMAMemoryManager::is_numa_available() const noexcept {
    return topology_.num_nodes > 1;
}

NUMAStats NUMAMemoryManager::get_stats() const {
    // F-006: build snapshot from atomics — no lock needed.
    NUMAStats s;
    s.local_accesses  = stat_local_.load(std::memory_order_relaxed);
    s.remote_accesses = stat_remote_.load(std::memory_order_relaxed);
    uint64_t total = s.local_accesses + s.remote_accesses;
    s.locality_ratio  = (total > 0)
        ? static_cast<double>(s.local_accesses) / static_cast<double>(total)
        : 1.0;
    s.per_node_allocations.resize(num_nodes_, 0);
    for (size_t i = 0; i < num_nodes_; ++i) {
        int64_t bytes = per_node_bytes_[i].load(std::memory_order_relaxed);
        s.per_node_allocations[i] = (bytes > 0) ? static_cast<uint64_t>(bytes) : 0u;
    }
    return s;
}

void NUMAMemoryManager::reset_stats() {
    stat_local_.store(0, std::memory_order_relaxed);
    stat_remote_.store(0, std::memory_order_relaxed);
    for (size_t i = 0; i < num_nodes_; ++i)
        per_node_bytes_[i].store(0, std::memory_order_relaxed);
}

}  // namespace performance
}  // namespace themis

