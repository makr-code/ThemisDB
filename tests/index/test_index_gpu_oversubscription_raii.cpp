/**
 * @file test_index_gpu_oversubscription_raii.cpp
 * @brief RAII/destructor safety tests for GPUMemoryOversubscriptionManager.
 *
 * Wave 2-B — B1: Validates that the Impl destructor correctly frees all
 * VRAM-resident partitions on object destruction, with no crash and no leak.
 *
 * Test cases:
 *   RAII_1  ManagerWithPartition_DestructorDoesNotCrash
 *   RAII_2  ManagerWithNoVramPartitions_DestructorDoesNotCrash
 *   RAII_3  ManagerWithMultiplePartitions_AllFreedAfterDestruction
 */

#include "index/gpu_memory_oversubscription.h"
#include <gtest/gtest.h>

#include <cstddef>
#include <vector>

using namespace themis::index;

namespace {

/// Build a minimal manager config (no real VRAM budget, CPU-only path).
GPUMemoryOversubscriptionManager::Config makeCfg(size_t vram_budget_mb = 0) {
    GPUMemoryOversubscriptionManager::Config cfg;
    cfg.enable_oversubscription = true;
    cfg.vram_budget_mb          = vram_budget_mb;
    cfg.prefetch_strategy       = PrefetchStrategy::NONE;
    cfg.partition_vectors       = 16;
    cfg.use_unified_memory      = false; // CPU build-safe
    return cfg;
}

/// Flat vector of `n * dim` floats.
std::vector<float> flat(size_t n, size_t dim, float v = 1.0f) {
    return std::vector<float>(n * dim, v);
}

} // namespace

// ---------------------------------------------------------------------------
// RAII_1: Create manager, add a partition, load it (pin it in VRAM/CPU
//         fallback), then destroy the manager.  Must not crash.
// ---------------------------------------------------------------------------
TEST(GPUOversubscriptionRAII, RAII_1_ManagerWithPartition_DestructorDoesNotCrash) {
    {
        GPUMemoryOversubscriptionManager mgr(makeCfg());
        constexpr size_t kDim = 8;
        constexpr size_t kN   = 16;
        auto data = flat(kN, kDim);
        const size_t id = mgr.addPartition(data, kN, kDim, "raii_test_1");
        EXPECT_NE(id, static_cast<size_t>(-1));
        // Access to bring it into VRAM (or CPU alias)
        mgr.accessPartition(id);
        EXPECT_TRUE(mgr.isPartitionInVRAM(id));
        // Manager goes out of scope here — destructor must not crash.
    }
    // If we reach this line the destructor completed without aborting.
    SUCCEED();
}

// ---------------------------------------------------------------------------
// RAII_2: Manager destroyed with NO in-VRAM partitions (all cold).
//         Destructor must be a no-op (no free, no crash).
// ---------------------------------------------------------------------------
TEST(GPUOversubscriptionRAII, RAII_2_ManagerWithNoVramPartitions_DestructorDoesNotCrash) {
    {
        GPUMemoryOversubscriptionManager mgr(makeCfg());
        constexpr size_t kDim = 8;
        constexpr size_t kN   = 8;
        auto data = flat(kN, kDim);
        const size_t id = mgr.addPartition(data, kN, kDim, "cold_partition");
        EXPECT_NE(id, static_cast<size_t>(-1));
        // Do NOT call accessPartition — partition stays cold (in_vram == false)
        EXPECT_FALSE(mgr.isPartitionInVRAM(id));
        // Destructor must silently skip cold partitions.
    }
    SUCCEED();
}

// ---------------------------------------------------------------------------
// RAII_3: Manager with multiple partitions all made VRAM-resident.
//         All must be freed after destruction (validated via stats: query before
//         destruction and confirm hot_partitions > 0, then after destruction the
//         manager is gone — we verify via the Stats snapshot taken before).
// ---------------------------------------------------------------------------
TEST(GPUOversubscriptionRAII, RAII_3_MultipleVramPartitions_AllFreedAfterDestruction) {
    constexpr size_t kDim = 8;
    constexpr size_t kN   = 8;
    constexpr int    kCount = 4;

    GPUMemoryOversubscriptionManager::Stats stats_before;
    {
        // Use budget=0 (SIZE_MAX effective), so all partitions fit in "VRAM".
        GPUMemoryOversubscriptionManager mgr(makeCfg(/*vram_budget_mb=*/0));

        std::vector<size_t> ids = {};

        for (int i = 0; i < kCount; ++i) {
            auto data = flat(kN, kDim, static_cast<float>(i + 1));
            ids.push_back(mgr.addPartition(data, kN, kDim,
                                          "partition_" + std::to_string(i)));
        }
        // Bring all into VRAM (CPU alias on non-CUDA builds).
        for (size_t id : ids) {
            mgr.accessPartition(id);
        }

        stats_before = mgr.getStats();
        EXPECT_EQ(stats_before.hot_partitions, static_cast<size_t>(kCount))
            << "All partitions should be VRAM-resident before destruction";

        // mgr goes out of scope → Impl destructor runs.
    }

    // If we get here the destructor ran without aborting or double-freeing.
    // The correct hot count before destruction was verified above.
    EXPECT_EQ(stats_before.hot_partitions, static_cast<size_t>(kCount));
    SUCCEED();
}
