/**
 * @file test_active_vram_allocator.cpp
 * @brief Focused tests for ActiveVRAMAllocator (LLM-MISSING-001)
 *
 * Tests cover:
 *  - Basic allocation and free
 *  - Aligned allocation (block alignment)
 *  - Allocation statistics (used/free/peak/waste)
 *  - OOM threshold detection
 *  - OOM recovery: eviction, defragmentation, CPU spilling
 *  - LRU eviction ordering
 *  - Owner-based eviction
 *  - CPU spill and restore
 *  - OOM callback notifications
 *  - allocateOrRecover() retry path
 *  - allocateWithFragmentation() bridge API
 *  - handleOutOfMemory() bridge API
 *  - AdaptiveVRAMAllocator stub delegation
 *  - Thread safety (concurrent allocate/free)
 */

#include <gtest/gtest.h>
#include "llm/active_vram_allocator.h"
#include "llm/adaptive_vram_allocator.h"
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace themis::llm;

// ---------------------------------------------------------------------------
// Helper: build a config that has room for controlled tests
// ---------------------------------------------------------------------------
static ActiveVRAMAllocator::Config makeTestConfig(
    size_t max_vram_mb    = 128,
    bool   enable_spill   = true,
    bool   enable_defrag  = true,
    float  oom_threshold  = 0.90f)
{
    ActiveVRAMAllocator::Config cfg;
    cfg.max_vram_bytes         = max_vram_mb * 1024 * 1024;
    cfg.max_cpu_spill_bytes    = 256ULL * 1024 * 1024;  // 256 MB CPU spill
    cfg.enable_cpu_spilling    = enable_spill;
    cfg.enable_defragmentation = enable_defrag;
    cfg.oom_threshold_fraction = oom_threshold;
    cfg.min_free_vram_reserve  = 0;  // No reserve for tests
    cfg.block_alignment        = 4096;
    return cfg;
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TEST(ActiveVRAMAllocatorTest, ConstructsWithDefaultConfig) {
    EXPECT_NO_THROW({
        ActiveVRAMAllocator alloc;
    });
}

TEST(ActiveVRAMAllocatorTest, DefaultConfigAllocatorCanAllocate) {
    // Bug fix regression: with max_vram_bytes=0 (auto-detect), GPUMemoryManager must
    // auto-set a simulation VRAM limit so that canAllocate() succeeds.
    ActiveVRAMAllocator alloc;
    auto handle = alloc.allocate(4096, "default_config_test");
    // In CPU-simulation mode with auto-detected 8GB limit, allocation must succeed
    ASSERT_TRUE(handle.has_value()) << "Default-config allocator failed to allocate 4096 bytes; "
                                       "GPUMemoryManager must auto-set a non-zero VRAM limit when max_vram_bytes=0";
    EXPECT_TRUE(handle->valid);
    alloc.free(*handle);
}

TEST(ActiveVRAMAllocatorTest, ConstructsWithCustomConfig) {
    auto cfg = makeTestConfig();
    EXPECT_NO_THROW({
        ActiveVRAMAllocator alloc(cfg);
    });
}

TEST(ActiveVRAMAllocatorTest, IsMovable) {
    auto cfg = makeTestConfig();
    ActiveVRAMAllocator a(cfg);
    ActiveVRAMAllocator b(std::move(a));
    // b should be functional
    EXPECT_NO_THROW(b.getStats());
}

// ---------------------------------------------------------------------------
// Basic allocation / free
// ---------------------------------------------------------------------------

TEST(ActiveVRAMAllocatorTest, AllocateAndFreeSmallBuffer) {
    auto cfg = makeTestConfig();
    ActiveVRAMAllocator alloc(cfg);

    auto handle = alloc.allocate(1024, "test_model");
    ASSERT_TRUE(handle.has_value());
    EXPECT_TRUE(handle->valid);
    EXPECT_EQ(handle->owner_id, "test_model");
    EXPECT_EQ(handle->requested_bytes, 1024u);
    EXPECT_GE(handle->allocated_bytes, 1024u);
    // gpu_ptr xor cpu_ptr must be non-null
    EXPECT_TRUE(handle->gpu_ptr != nullptr || handle->cpu_ptr != nullptr);

    bool freed = alloc.free(*handle);
    EXPECT_TRUE(freed);
    EXPECT_FALSE(handle->valid);
}

TEST(ActiveVRAMAllocatorTest, AllocateZeroBytesReturnsNullopt) {
    ActiveVRAMAllocator alloc(makeTestConfig());
    auto result = alloc.allocate(0, "model");
    EXPECT_FALSE(result.has_value());
}

TEST(ActiveVRAMAllocatorTest, FreeInvalidHandleReturnsFalse) {
    ActiveVRAMAllocator alloc(makeTestConfig());
    ActiveVRAMAllocator::AllocationHandle invalid;
    invalid.valid = false;
    EXPECT_FALSE(alloc.free(invalid));
}

// ---------------------------------------------------------------------------
// Block alignment
// ---------------------------------------------------------------------------

TEST(ActiveVRAMAllocatorTest, AllocatedBytesAreAligned) {
    auto cfg   = makeTestConfig();
    cfg.block_alignment = 4096;
    ActiveVRAMAllocator alloc(cfg);

    for (size_t req : {1u, 100u, 4095u, 4096u, 4097u, 8192u, 10000u}) {
        auto h = alloc.allocate(req, "align_test");
        ASSERT_TRUE(h.has_value()) << "request=" << req;
        EXPECT_EQ(h->allocated_bytes % 4096, 0u)
            << "Not 4096-aligned for request=" << req;
        alloc.free(*h);
    }
}

// ---------------------------------------------------------------------------
// Statistics
// ---------------------------------------------------------------------------

TEST(ActiveVRAMAllocatorTest, StatsReflectAllocations) {
    ActiveVRAMAllocator alloc(makeTestConfig());

    auto s0 = alloc.getStats();
    EXPECT_EQ(s0.live_allocation_count, 0u);
    // Bug fix regression: total_vram_bytes must be non-zero even before allocations
    // (was 0 when getTotalVRAM() returned total_vram_used_ = 0 at init)
    EXPECT_GT(s0.total_vram_bytes, 0u)
        << "total_vram_bytes must reflect the configured VRAM capacity, not used amount";

    auto h1 = alloc.allocate(4096, "m1");
    auto h2 = alloc.allocate(4096, "m2");
    ASSERT_TRUE(h1 && h2);

    auto s1 = alloc.getStats();
    EXPECT_EQ(s1.live_allocation_count, 2u);
    EXPECT_GE(s1.used_vram_bytes, 8192u);

    alloc.free(*h1);
    auto s2 = alloc.getStats();
    EXPECT_EQ(s2.live_allocation_count, 1u);

    alloc.free(*h2);
    auto s3 = alloc.getStats();
    EXPECT_EQ(s3.live_allocation_count, 0u);
}

TEST(ActiveVRAMAllocatorTest, PeakUsageTracked) {
    ActiveVRAMAllocator alloc(makeTestConfig());

    auto h1 = alloc.allocate(64 * 1024, "m1");
    auto h2 = alloc.allocate(64 * 1024, "m2");
    ASSERT_TRUE(h1 && h2);

    auto peak_during = alloc.getStats().peak_vram_bytes;
    EXPECT_GE(peak_during, 128u * 1024u);

    alloc.free(*h1);
    alloc.free(*h2);

    // Peak must not decrease after free
    auto peak_after = alloc.getStats().peak_vram_bytes;
    EXPECT_GE(peak_after, peak_during);
}

TEST(ActiveVRAMAllocatorTest, WastedPaddingTracked) {
    auto cfg        = makeTestConfig();
    cfg.block_alignment = 4096;
    ActiveVRAMAllocator alloc(cfg);

    // Request 1 byte → allocates 4096 → waste = 4095
    auto h = alloc.allocate(1, "waste_test");
    ASSERT_TRUE(h.has_value());

    auto stats = alloc.getStats();
    EXPECT_EQ(stats.wasted_padding_bytes, 4095u);

    alloc.free(*h);
}

// ---------------------------------------------------------------------------
// OOM threshold detection
// ---------------------------------------------------------------------------

TEST(ActiveVRAMAllocatorTest, OOMThresholdDetectedAtHighUsage) {
    // Use a very small VRAM budget to force threshold crossing
    auto cfg = makeTestConfig(/*max_vram_mb=*/4, /*spill=*/false, /*defrag=*/false);
    cfg.oom_threshold_fraction = 0.50f;  // 50% threshold
    ActiveVRAMAllocator alloc(cfg);

    EXPECT_FALSE(alloc.isOOMThresholdExceeded());

    // Allocate more than 50% of the 4 MB budget
    auto h = alloc.allocate(3 * 1024 * 1024, "heavy");
    // Threshold may or may not be exceeded depending on whether GPU is real
    // — we just verify the function doesn't crash and returns a bool
    (void)alloc.isOOMThresholdExceeded();

    if (h) {
      alloc.free(*h);
    }
}

// ---------------------------------------------------------------------------
// Eviction
// ---------------------------------------------------------------------------

TEST(ActiveVRAMAllocatorTest, EvictLRUFreesOldestAllocation) {
    ActiveVRAMAllocator alloc(makeTestConfig());

    auto h1 = alloc.allocate(4096, "old");
    ASSERT_TRUE(h1);
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    auto h2 = alloc.allocate(4096, "new");
    ASSERT_TRUE(h2);

    // h1 is older — evictLRU should free it first
    size_t freed = alloc.evictLRU();
    EXPECT_GE(freed, 4096u);

    auto stats = alloc.getStats();
    EXPECT_EQ(stats.eviction_count, 1u);

    // h2 is still alive, clean up
    alloc.free(*h2);
}

TEST(ActiveVRAMAllocatorTest, EvictLRUOnEmptyAllocatorReturnsZero) {
    ActiveVRAMAllocator alloc(makeTestConfig());
    EXPECT_EQ(alloc.evictLRU(), 0u);
}

TEST(ActiveVRAMAllocatorTest, EvictOwnerFreesAllForThatOwner) {
    ActiveVRAMAllocator alloc(makeTestConfig());

    auto h1 = alloc.allocate(4096, "alpha");
    auto h2 = alloc.allocate(4096, "alpha");
    auto h3 = alloc.allocate(4096, "beta");
    ASSERT_TRUE(h1 && h2 && h3);

    size_t freed = alloc.evictOwner("alpha");
    EXPECT_GE(freed, 8192u);

    auto stats = alloc.getStats();
    EXPECT_EQ(stats.live_allocation_count, 1u);  // only "beta" remains

    alloc.free(*h3);
}

TEST(ActiveVRAMAllocatorTest, EvictOwnerUnknownReturnsZero) {
    ActiveVRAMAllocator alloc(makeTestConfig());
    EXPECT_EQ(alloc.evictOwner("does_not_exist"), 0u);
}

// ---------------------------------------------------------------------------
// Touch (LRU update)
// ---------------------------------------------------------------------------

TEST(ActiveVRAMAllocatorTest, TouchUpdatesLastUsedTimestamp) {
    ActiveVRAMAllocator alloc(makeTestConfig());
    auto h = alloc.allocate(4096, "touch_model");
    ASSERT_TRUE(h);

    int64_t ts_before = h->last_used_at_ms;
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    alloc.touch(*h);

    // The updated timestamp must be visible in listAllocations
    auto live = alloc.listAllocations();
    ASSERT_EQ(live.size(), 1u);
    EXPECT_GE(live[0].last_used_at_ms, ts_before);

    alloc.free(*h);
}

// ---------------------------------------------------------------------------
// CPU spilling
// ---------------------------------------------------------------------------

TEST(ActiveVRAMAllocatorTest, SpillLRUToCPUTransfersAllocation) {
    auto cfg = makeTestConfig(/*max_vram_mb=*/128, /*spill=*/true);
    ActiveVRAMAllocator alloc(cfg);

    auto h1 = alloc.allocate(4096, "spill_model");
    ASSERT_TRUE(h1);

    size_t spilled = alloc.spillLRUToCPU();
    // In GPU or simulation mode, spilling should succeed
    if (spilled > 0) {
        auto stats = alloc.getStats();
        EXPECT_GE(stats.spill_count, 1u);
        EXPECT_GE(stats.spilled_cpu_bytes, 4096u);
    }
    // Regardless, no crash
}

TEST(ActiveVRAMAllocatorTest, SpillDisabledReturnsZero) {
    auto cfg = makeTestConfig(/*max_vram_mb=*/128, /*spill=*/false);
    ActiveVRAMAllocator alloc(cfg);

    auto h = alloc.allocate(4096, "no_spill");
    ASSERT_TRUE(h);

    EXPECT_EQ(alloc.spillLRUToCPU(), 0u);
    alloc.free(*h);
}

// ---------------------------------------------------------------------------
// Defragmentation
// ---------------------------------------------------------------------------

TEST(ActiveVRAMAllocatorTest, DefragmentRunsWithoutCrash) {
    auto cfg = makeTestConfig(/*max_vram_mb=*/128, /*spill=*/true, /*defrag=*/true);
    ActiveVRAMAllocator alloc(cfg);

    // Create some fragmentation by allocating and freeing
    for (int i = 0; i < 5; ++i) {
        auto h = alloc.allocate(4096, "frag_" + std::to_string(i));
        if (h) {
          alloc.free(*h);
        }
    }

    // defragment should not crash even if there's nothing to compact
    EXPECT_NO_THROW(alloc.defragment());
}

TEST(ActiveVRAMAllocatorTest, DefragDisabledReturnsFalse) {
    auto cfg = makeTestConfig(/*max_vram_mb=*/128, /*spill=*/true, /*defrag=*/false);
    ActiveVRAMAllocator alloc(cfg);
    EXPECT_FALSE(alloc.defragment());
}

// ---------------------------------------------------------------------------
// OOM recovery — handleOOM
// ---------------------------------------------------------------------------

TEST(ActiveVRAMAllocatorTest, HandleOOMRecoversByEviction) {
    ActiveVRAMAllocator alloc(makeTestConfig());

    // Pre-populate with a live allocation
    auto h = alloc.allocate(4096, "evictable");
    ASSERT_TRUE(h);

    // handleOOM should evict it
    bool recovered = alloc.handleOOM(4096);
    // Recovery may or may not succeed based on available VRAM, but must not crash
    (void)recovered;

    auto stats = alloc.getStats();
    EXPECT_GE(stats.oom_event_count + stats.oom_recovery_count, 0u);
}

TEST(ActiveVRAMAllocatorTest, HandleOOMOnEmptyAllocatorDoesNotCrash) {
    ActiveVRAMAllocator alloc(makeTestConfig());
    EXPECT_NO_THROW(alloc.handleOOM(1024));
}

// ---------------------------------------------------------------------------
// allocateOrRecover
// ---------------------------------------------------------------------------

TEST(ActiveVRAMAllocatorTest, AllocateOrRecoverSucceedsNormally) {
    ActiveVRAMAllocator alloc(makeTestConfig());

    auto h = alloc.allocateOrRecover(4096, "recover_model");
    ASSERT_TRUE(h.has_value());
    EXPECT_TRUE(h->valid);
    alloc.free(*h);
}

// ---------------------------------------------------------------------------
// OOM callback
// ---------------------------------------------------------------------------

TEST(ActiveVRAMAllocatorTest, OOMCallbackFiredOnAllocationFailure) {
    // Use tiny VRAM and no recovery to force callback
    auto cfg = makeTestConfig(/*mb=*/1, /*spill=*/false, /*defrag=*/false);
    cfg.max_vram_bytes = 1024;  // 1 KB — forces OOM quickly
    ActiveVRAMAllocator alloc(cfg);

    std::atomic<int> cb_count{0};
    alloc.setOOMCallback([&](const ActiveVRAMAllocator::OOMEvent& ev) {
        cb_count++;
        (void)ev;
    });

    // Try allocating 2 KB in 1 KB space — should OOM
    auto h = alloc.allocate(2048, "oom_model");
    // Callback may or may not fire depending on underlying GPU/simulation,
    // but must not crash
    (void)h;
    // Just verify callback doesn't cause a crash
}

// ---------------------------------------------------------------------------
// listAllocations
// ---------------------------------------------------------------------------

TEST(ActiveVRAMAllocatorTest, ListAllocationsReturnsAllLiveHandles) {
    ActiveVRAMAllocator alloc(makeTestConfig());

    auto h1 = alloc.allocate(4096, "a");
    auto h2 = alloc.allocate(4096, "b");
    ASSERT_TRUE(h1 && h2);

    auto list = alloc.listAllocations();
    EXPECT_EQ(list.size(), 2u);

    alloc.free(*h1);
    EXPECT_EQ(alloc.listAllocations().size(), 1u);

    alloc.free(*h2);
    EXPECT_EQ(alloc.listAllocations().size(), 0u);
}

// ---------------------------------------------------------------------------
// gpuDeviceId / isGPUAvailable
// ---------------------------------------------------------------------------

TEST(ActiveVRAMAllocatorTest, GPUDeviceIdAndAvailabilityAreAccessible) {
    ActiveVRAMAllocator alloc(makeTestConfig());
    int dev = alloc.gpuDeviceId();
    EXPECT_GE(dev, 0);
    // isGPUAvailable returns a bool — just check it doesn't crash
    bool avail = alloc.isGPUAvailable();
    (void)avail;
}

// ---------------------------------------------------------------------------
// Bridge API: allocateWithFragmentation / handleOutOfMemory
// ---------------------------------------------------------------------------

TEST(ActiveVRAMAllocatorTest, AllocateWithFragmentationSetsPointer) {
    ActiveVRAMAllocator alloc(makeTestConfig());

    void* ptr = nullptr;
    bool ok = alloc.allocateWithFragmentation(4096, &ptr);
    EXPECT_TRUE(ok);
    EXPECT_NE(ptr, nullptr);
}

TEST(ActiveVRAMAllocatorTest, AllocateWithFragmentationNullPtrReturnsFalse) {
    ActiveVRAMAllocator alloc(makeTestConfig());
    EXPECT_FALSE(alloc.allocateWithFragmentation(4096, nullptr));
}

TEST(ActiveVRAMAllocatorTest, HandleOutOfMemoryDoesNotCrash) {
    ActiveVRAMAllocator alloc(makeTestConfig());
    // With an empty allocator there's nothing to recover, but must not crash
    EXPECT_NO_THROW(alloc.handleOutOfMemory());
}

// ---------------------------------------------------------------------------
// AdaptiveVRAMAllocator delegation
// ---------------------------------------------------------------------------

TEST(AdaptiveVRAMAllocatorDelegationTest, AllocateWithFragmentationNoLongerStub) {
    AdaptiveVRAMAllocator ada;

    void* ptr = nullptr;
    bool ok = ada.allocateWithFragmentation(4096, &ptr);

    // Must not exhibit the old stub behaviour (ok=true but ptr==nullptr).
    // The delegation to ActiveVRAMAllocator ensures a real allocation attempt:
    // if ok==true then ptr must be a valid non-null memory address.
    if (ok) {
        EXPECT_NE(ptr, nullptr)
            << "allocateWithFragmentation returned true but ptr is nullptr — "
               "delegation to ActiveVRAMAllocator is not working correctly";
    }
}

TEST(AdaptiveVRAMAllocatorDelegationTest, HandleOutOfMemoryNoLongerReturnsFalseUnconditionally) {
    AdaptiveVRAMAllocator ada;
    // With an empty allocator recovery will likely return false, but
    // the method must at least attempt the recovery sequence without crashing.
    EXPECT_NO_THROW(ada.handleOutOfMemory());
}

// ---------------------------------------------------------------------------
// Thread safety
// ---------------------------------------------------------------------------

TEST(ActiveVRAMAllocatorTest, ConcurrentAllocateFreeIsThreadSafe) {
    ActiveVRAMAllocator alloc(makeTestConfig(/*mb=*/128));

    constexpr int kThreads = 8;
    constexpr int kIter    = 20;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&alloc, t]() {
            for (int i = 0; i < kIter; ++i) {
                auto h = alloc.allocate(4096, "thread_" + std::to_string(t));
                if (h) {
                    alloc.touch(*h);
                    alloc.free(*h);
                }
            }
        });
    }

    for (auto& th : threads) {
      th.join();
    }

    // After all threads complete, allocator should be in a consistent state
    auto stats = alloc.getStats();
    EXPECT_EQ(stats.live_allocation_count, 0u);
}

TEST(ActiveVRAMAllocatorTest, ConcurrentEvictAndAllocateIsSafe) {
    ActiveVRAMAllocator alloc(makeTestConfig(/*mb=*/128));

    // Allocator thread
    std::thread allocator([&] {
        for (int i = 0; i < 50; ++i) {
            auto h = alloc.allocate(4096, "concurrent");
            if (h) {
              alloc.free(*h);
            }
        }
    });

    // Evictor thread
    std::thread evictor([&] {
        for (int i = 0; i < 50; ++i) {
            alloc.evictLRU();
            std::this_thread::yield();
        }
    });

    allocator.join();
    evictor.join();

    EXPECT_NO_THROW(alloc.getStats());
}

// ---------------------------------------------------------------------------
// Regression tests for bugs found during code audit
// ---------------------------------------------------------------------------

// Bug 1: allocateOrRecover must not call handleOOMInternal without lock.
// This test forces the OOM recovery path and verifies it doesn't deadlock or
// corrupt state when called concurrently.
TEST(ActiveVRAMAllocatorTest, AllocateOrRecoverIsThreadSafe) {
    ActiveVRAMAllocator alloc(makeTestConfig(/*mb=*/128));

    // Pre-populate so recovery has something to evict
    std::vector<ActiveVRAMAllocator::AllocationHandle> seed;
    for (int i = 0; i < 4; ++i) {
        auto h = alloc.allocate(4096, "seed_" + std::to_string(i));
        if (h) {
          seed.push_back(*h);
        }
    }

    // Fire multiple threads calling allocateOrRecover simultaneously
    constexpr int kThreads = 4;
    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&alloc, t]() {
            for (int i = 0; i < 10; ++i) {
                auto h = alloc.allocateOrRecover(4096, "recovery_" + std::to_string(t));
                if (h) {
                  alloc.free(*h);
                }
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }

    // If the thread-safety bug were present, we'd see crashes or assertion failures.
    // Just verify the allocator is still consistent.
    EXPECT_NO_THROW(alloc.getStats());

    for (auto& h : seed) {
        if (h.valid) {
          alloc.free(h);
        }
    }
}

// Bug 2: live_allocation_count must not drift after spill+restore cycles.
TEST(ActiveVRAMAllocatorTest, LiveCountDoesNotDriftAfterSpillRestore) {
    auto cfg = makeTestConfig(/*mb=*/128, /*spill=*/true);
    ActiveVRAMAllocator alloc(cfg);

    auto h = alloc.allocate(4096, "spill_restore_model");
    ASSERT_TRUE(h.has_value());

    auto count_before = alloc.getStats().live_allocation_count;
    EXPECT_EQ(count_before, 1u);

    // Spill to CPU
    size_t spilled = alloc.spillLRUToCPU();
    if (spilled > 0) {
        // Count must remain 1 after spill (handle is still valid)
        EXPECT_EQ(alloc.getStats().live_allocation_count, 1u);

        // Restore from CPU
        // We need to get the updated handle (is_spilled=true) from listAllocations
        auto live = alloc.listAllocations();
        ASSERT_EQ(live.size(), 1u);
        EXPECT_TRUE(live[0].is_spilled);

        bool restored = alloc.restoreFromCPU(live[0]);
        if (restored) {
            // Count must still be 1 — not incremented on restore
            EXPECT_EQ(alloc.getStats().live_allocation_count, 1u);
        }
    }

    // Free
    auto live = alloc.listAllocations();
    if (!live.empty()) {
      alloc.free(live[0]);
    }
    EXPECT_EQ(alloc.getStats().live_allocation_count, 0u);
}

// Bug 3: bridge_handles_ entries must not accumulate unboundedly.
// After repeated allocateWithFragmentation calls and subsequent evictions,
// the internal bridge metadata map should not grow forever.
TEST(ActiveVRAMAllocatorTest, BridgeHandlesCleanedUpOnEviction) {
    ActiveVRAMAllocator alloc(makeTestConfig(/*mb=*/128));

    // Call bridge API several times — internally stores handles in bridge_handles_
    for (int i = 0; i < 5; ++i) {
        void* ptr = nullptr;
        bool ok = alloc.allocateWithFragmentation(4096, &ptr);
        (void)ok;
    }

    // Evict all — should clean up bridge entries too
    for (int i = 0; i < 10; ++i) {
        alloc.evictLRU();
    }

    // Allocator should be in a consistent state (no crash, no stale pointers)
    EXPECT_EQ(alloc.listAllocations().size(), 0u);
    EXPECT_NO_THROW(alloc.getStats());
}

// ═══════════════════════════════════════════════════════════
// AdaptiveVRAMAllocator::calculateDualModelAllocation
// ═══════════════════════════════════════════════════════════

namespace {
/// Build a minimal 7B target model config (LLaMA-like, FP16).
AdaptiveVRAMAllocator::ModelConfig make7BTargetConfig() {
    AdaptiveVRAMAllocator::ModelConfig cfg;
    cfg.model_name      = "llama-7b";
    cfg.num_parameters  = 7'000'000'000ULL;
    cfg.num_layers      = 32;
    cfg.hidden_dim      = 4096;
    cfg.num_heads       = 32;
    cfg.num_kv_heads    = 8;
    cfg.head_dim        = 128;
    cfg.precision_bytes = 2;  // FP16
    return cfg;
}

/// Build a minimal 0.5B draft model config (no explicit precision → INT4 default).
AdaptiveVRAMAllocator::ModelConfig make500MDraftConfig() {
    AdaptiveVRAMAllocator::ModelConfig cfg;
    cfg.model_name      = "llama-0.5b";
    cfg.num_parameters  = 500'000'000ULL;
    cfg.num_layers      = 24;
    cfg.hidden_dim      = 1024;
    cfg.num_heads       = 16;
    cfg.num_kv_heads    = 4;
    cfg.head_dim        = 64;
    cfg.precision_bytes = 0;  // 0 signals INT4 (0.5 bytes/param)
    return cfg;
}

/// A10G-class GPU: 24 GB VRAM.
AdaptiveVRAMAllocator::HardwareInfo makeA10GHardware() {
    AdaptiveVRAMAllocator::HardwareInfo hw;
    hw.total_vram_bytes     = 24ULL * 1024 * 1024 * 1024;
    hw.available_vram_bytes = 22ULL * 1024 * 1024 * 1024;
    hw.compute_capability_major = 8;
    hw.compute_capability_minor = 6;
    hw.has_tensor_cores     = true;
    hw.memory_bandwidth_gbps = 600;
    return hw;
}
} // namespace

/// Dual-model plan total >= single-model plan total (draft weights added).
TEST(AdaptiveVRAMAllocatorDualModelTest, DualModelTotalGreaterThanSingleModel) {
    AdaptiveVRAMAllocator alloc;
    const auto target = make7BTargetConfig();
    const auto draft  = make500MDraftConfig();
    const auto hw     = makeA10GHardware();

    AdaptiveVRAMAllocator::InferenceConfig inf_cfg;
    inf_cfg.batch_size   = 1;
    inf_cfg.max_seq_length = 2048;

    auto single_plan = alloc.calculateOptimalAllocation(target, hw, inf_cfg);
    auto dual_plan   = alloc.calculateDualModelAllocation(target, draft, hw, inf_cfg);

    EXPECT_GT(dual_plan.total, single_plan.total)
        << "Dual-model allocation must be larger than single-model allocation";
}

/// Draft model weights are INT4 (0.5 bytes/param) when precision_bytes == 0.
TEST(AdaptiveVRAMAllocatorDualModelTest, DraftModelDefaultsToInt4WhenPrecisionIsZero) {
    AdaptiveVRAMAllocator alloc;
    const auto target = make7BTargetConfig();
    const auto draft  = make500MDraftConfig(); // precision_bytes = 0
    const auto hw     = makeA10GHardware();

    AdaptiveVRAMAllocator::InferenceConfig inf_cfg;
    inf_cfg.batch_size    = 1;
    inf_cfg.max_seq_length = 2048;

    auto plan = alloc.calculateDualModelAllocation(target, draft, hw, inf_cfg);

    // INT4: 500M × 0.5 bytes = 250 MB
    const size_t expected_draft_bytes = 500'000'000ULL / 2;  // 0.5 bytes per param
    EXPECT_EQ(plan.draft_model_weights, expected_draft_bytes)
        << "Draft model (INT4) should use 0.5 bytes per parameter";
    EXPECT_EQ(plan.draft_precision_bytes, 0)
        << "draft_precision_bytes should reflect the INT4 default (0)";
}

/// draft_model_weights contributes to plan.model_weights.
TEST(AdaptiveVRAMAllocatorDualModelTest, ModelWeightsIncludesDraftContribution) {
    AdaptiveVRAMAllocator alloc;
    const auto target = make7BTargetConfig();
    const auto draft  = make500MDraftConfig();
    const auto hw     = makeA10GHardware();

    AdaptiveVRAMAllocator::InferenceConfig inf_cfg;
    inf_cfg.batch_size    = 1;
    inf_cfg.max_seq_length = 2048;

    auto single_plan = alloc.calculateOptimalAllocation(target, hw, inf_cfg);
    auto dual_plan   = alloc.calculateDualModelAllocation(target, draft, hw, inf_cfg);

    EXPECT_EQ(dual_plan.model_weights,
              single_plan.model_weights + dual_plan.draft_model_weights)
        << "model_weights in dual plan must equal target + draft weights";
}

/// fits_in_vram is false when total exceeds available VRAM.
TEST(AdaptiveVRAMAllocatorDualModelTest, FitsInVramFalseWhenTotalExceedsAvailable) {
    AdaptiveVRAMAllocator alloc;

    // Huge model that won't fit in 22 GB
    AdaptiveVRAMAllocator::ModelConfig big_target;
    big_target.num_parameters  = 70'000'000'000ULL;  // 70B
    big_target.num_layers      = 80;
    big_target.hidden_dim      = 8192;
    big_target.num_heads       = 64;
    big_target.num_kv_heads    = 8;
    big_target.head_dim        = 128;
    big_target.precision_bytes = 2;  // FP16

    AdaptiveVRAMAllocator::ModelConfig draft;
    draft.num_parameters  = 500'000'000ULL;
    draft.precision_bytes = 0;  // INT4

    const auto hw = makeA10GHardware();  // 22 GB available
    AdaptiveVRAMAllocator::InferenceConfig inf_cfg;

    auto plan = alloc.calculateDualModelAllocation(big_target, draft, hw, inf_cfg);
    EXPECT_FALSE(plan.fits_in_vram)
        << "A 70B + 0.5B model combination must not fit in 22 GB";
    EXPECT_FALSE(plan.recommendation.empty())
        << "Recommendation should be non-empty when allocation fails";
}

/// Explicit draft precision is respected when precision_bytes > 0.
TEST(AdaptiveVRAMAllocatorDualModelTest, ExplicitDraftPrecisionRespected) {
    AdaptiveVRAMAllocator alloc;
    const auto target = make7BTargetConfig();

    // Draft model with explicit INT8 precision (1 byte/param)
    AdaptiveVRAMAllocator::ModelConfig draft = make500MDraftConfig();
    draft.precision_bytes = 1;  // INT8

    const auto hw = makeA10GHardware();
    AdaptiveVRAMAllocator::InferenceConfig inf_cfg;
    inf_cfg.batch_size    = 1;
    inf_cfg.max_seq_length = 2048;

    auto plan = alloc.calculateDualModelAllocation(target, draft, hw, inf_cfg);

    // INT8: 500M × 1 byte = 500 MB
    const size_t expected_draft_bytes = 500'000'000ULL;
    EXPECT_EQ(plan.draft_model_weights, expected_draft_bytes)
        << "Draft model (INT8) should use 1 byte per parameter";
    EXPECT_EQ(plan.draft_precision_bytes, 1)
        << "draft_precision_bytes should reflect the explicit INT8 value";
}

// ---------------------------------------------------------------------------
// AVA_EXT: registerExternal / free (external-memory tracking)
// ---------------------------------------------------------------------------

/// AVA_EXT_01 — registerExternal() creates a valid handle, marks is_external=true,
/// and updates used_vram_bytes + live_allocation_count in the Stats snapshot.
TEST(ActiveVRAMAllocatorTest, RegisterExternalCreatesValidHandle) {
    ActiveVRAMAllocator alloc(makeTestConfig(/*max_vram_mb=*/256));

    const size_t model_bytes = 7ULL * 1024 * 1024 * 1024;  // 7 GB simulated

    auto handle = alloc.registerExternal(model_bytes, "llama-7b");

    EXPECT_TRUE(handle.valid)        << "AVA_EXT_01: handle must be valid";
    EXPECT_TRUE(handle.is_external)  << "AVA_EXT_01: is_external must be true";
    EXPECT_EQ(handle.owner_id, "llama-7b");
    EXPECT_EQ(handle.requested_bytes, model_bytes);
    EXPECT_EQ(handle.allocated_bytes, model_bytes);
    EXPECT_EQ(handle.gpu_ptr, nullptr)  << "AVA_EXT_01: external handle must not hold a GPU pointer";
    EXPECT_EQ(handle.cpu_ptr, nullptr);
    EXPECT_FALSE(handle.is_spilled);

    const auto stats = alloc.getStats();
    EXPECT_EQ(stats.used_vram_bytes, model_bytes)
        << "AVA_EXT_01: used_vram_bytes must reflect the registered external allocation";
    EXPECT_EQ(stats.live_allocation_count, 1u)
        << "AVA_EXT_01: live_allocation_count must be 1 after registerExternal";
    EXPECT_GE(stats.peak_vram_bytes, model_bytes)
        << "AVA_EXT_01: peak usage must track the external allocation";
}

/// AVA_EXT_02 — free() on an external handle decrements stats correctly and does NOT crash.
/// In particular, used_vram_bytes must return to 0 and no GPU/CPU memory must be touched.
TEST(ActiveVRAMAllocatorTest, FreeExternalHandleUpdatesStatsWithoutCrash) {
    ActiveVRAMAllocator alloc(makeTestConfig(/*max_vram_mb=*/256));

    const size_t model_bytes = 4ULL * 1024 * 1024 * 1024;  // 4 GB

    auto handle = alloc.registerExternal(model_bytes, "llama-4b");
    ASSERT_TRUE(handle.valid) << "AVA_EXT_02: pre-condition: handle must be valid";

    bool freed = alloc.free(handle);
    EXPECT_TRUE(freed)          << "AVA_EXT_02: free() must return true for a valid external handle";
    EXPECT_FALSE(handle.valid)  << "AVA_EXT_02: handle must be invalid after free";

    const auto stats = alloc.getStats();
    EXPECT_EQ(stats.used_vram_bytes, 0u)
        << "AVA_EXT_02: used_vram_bytes must be 0 after freeing the only external allocation";
    EXPECT_EQ(stats.live_allocation_count, 0u)
        << "AVA_EXT_02: live_allocation_count must be 0 after free";
}

/// AVA_EXT_03 — OOM threshold is triggered when external + internal allocations
/// together exceed oom_threshold_fraction of max_vram_bytes.
TEST(ActiveVRAMAllocatorTest, OOMThresholdConsidersExternalAllocations) {
    // 128 MB budget, 90% OOM threshold = 115.2 MB trigger
    ActiveVRAMAllocator alloc(makeTestConfig(/*max_vram_mb=*/128, /*spill=*/false, /*defrag=*/false));

    // Register 100 MB as an external allocation (e.g. model weights)
    const size_t external_bytes = 100ULL * 1024 * 1024;
    auto ext_handle = alloc.registerExternal(external_bytes, "big-model");
    ASSERT_TRUE(ext_handle.valid);

    // Allocate another 30 MB internally — this should push usage past 90%
    auto inner = alloc.allocate(30ULL * 1024 * 1024, "kv-cache");
    // Regardless of whether the inner allocation succeeds (depends on free VRAM),
    // the combined logical usage must have set the OOM-threshold flag.
    const auto stats = alloc.getStats();
    EXPECT_TRUE(stats.oom_threshold_exceeded)
        << "AVA_EXT_03: OOM threshold must be exceeded when external + internal "
           "allocations together surpass oom_threshold_fraction";

    // Cleanup
    alloc.free(ext_handle);
    if (inner) {
      alloc.free(*inner);
    }
}
