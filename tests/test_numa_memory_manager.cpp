// Copyright 2026 ThemisDB — NUMAMemoryManager focused tests (Issue #228)
#include "performance/numa_memory_manager.h"
#include <gtest/gtest.h>
#include <cstring>
#include <thread>
#include <vector>

using namespace themis::performance;

namespace {

class NUMAMemoryManagerTest : public ::testing::Test {
protected:
    NUMAMemoryManager mgr;
};

// --- Construction & topology --------------------------------------------------

TEST_F(NUMAMemoryManagerTest, ConstructionSucceeds) {
    EXPECT_GE(mgr.get_topology().num_nodes, 1u);
}

TEST_F(NUMAMemoryManagerTest, TopologyNodeMemoryPopulated) {
    auto topo = mgr.get_topology();
    EXPECT_EQ(topo.node_memory_mb.size(), topo.num_nodes);
}

TEST_F(NUMAMemoryManagerTest, TopologyDistanceMatrixSize) {
    auto topo = mgr.get_topology();
    EXPECT_EQ(topo.node_distances.size(), topo.num_nodes);
}

TEST_F(NUMAMemoryManagerTest, IsNumaAvailableReflectsNodeCount) {
    auto topo = mgr.get_topology();
    EXPECT_EQ(mgr.is_numa_available(), topo.num_nodes > 1);
}

// --- allocate_local -----------------------------------------------------------

TEST_F(NUMAMemoryManagerTest, AllocateLocalReturnsNonNull) {
    void* p = mgr.allocate_local(1024);
    ASSERT_NE(p, nullptr);
    mgr.deallocate(p, 1024);
}

TEST_F(NUMAMemoryManagerTest, AllocateLocalSmallSize) {
    void* p = mgr.allocate_local(1);
    ASSERT_NE(p, nullptr);
    mgr.deallocate(p, 1);
}

TEST_F(NUMAMemoryManagerTest, AllocateLocalLargeSize) {
    void* p = mgr.allocate_local(4 * 1024 * 1024);
    ASSERT_NE(p, nullptr);
    std::memset(p, 0xAB, 4 * 1024 * 1024);
    mgr.deallocate(p, 4 * 1024 * 1024);
}

// --- allocate_on_node --------------------------------------------------------

TEST_F(NUMAMemoryManagerTest, AllocateOnNodeZero) {
    void* p = mgr.allocate_on_node(512, 0);
    ASSERT_NE(p, nullptr);
    mgr.deallocate(p, 512);
}

TEST_F(NUMAMemoryManagerTest, AllocateOnNegativeNodeFallsBackToLocal) {
    void* p = mgr.allocate_on_node(512, -1);
    ASSERT_NE(p, nullptr);
    mgr.deallocate(p, 512);
}

TEST_F(NUMAMemoryManagerTest, AllocateOnOutOfRangeNodeFallsBack) {
    void* p = mgr.allocate_on_node(512, 9999);
    ASSERT_NE(p, nullptr);
    mgr.deallocate(p, 512);
}

// --- allocate with hint -------------------------------------------------------

TEST_F(NUMAMemoryManagerTest, AllocateWithDefaultHint) {
    void* p = mgr.allocate(1024);
    ASSERT_NE(p, nullptr);
    mgr.deallocate(p, 1024);
}

TEST_F(NUMAMemoryManagerTest, AllocateWithHugePageHint) {
    AllocationHint hint;
    hint.use_huge_pages = true;
    void* p = mgr.allocate(2 * 1024 * 1024, hint);
    ASSERT_NE(p, nullptr);
    mgr.deallocate(p, 2 * 1024 * 1024);
}

// --- deallocate ---------------------------------------------------------------

TEST_F(NUMAMemoryManagerTest, DeallocateNullptrSafe) {
    EXPECT_NO_THROW(mgr.deallocate(nullptr, 0));
}

TEST_F(NUMAMemoryManagerTest, DeallocateWithSizeZeroSafe) {
    void* p = mgr.allocate_local(64);
    ASSERT_NE(p, nullptr);
    EXPECT_NO_THROW(mgr.deallocate(p, 0));
}

// --- migrate_to_node ---------------------------------------------------------

TEST_F(NUMAMemoryManagerTest, MigrateToNodeNocrash) {
    void* p = mgr.allocate_local(1024);
    ASSERT_NE(p, nullptr);
    EXPECT_NO_THROW(mgr.migrate_to_node(p, 1024, 0));
    mgr.deallocate(p, 1024);
}

TEST_F(NUMAMemoryManagerTest, MigrateNullptrSafe) {
    EXPECT_NO_THROW(mgr.migrate_to_node(nullptr, 0, 0));
}

// --- get_current_node ---------------------------------------------------------

TEST_F(NUMAMemoryManagerTest, GetCurrentNodeInRange) {
    int node = mgr.get_current_node();
    EXPECT_GE(node, 0);
    EXPECT_LT(static_cast<size_t>(node), mgr.get_topology().num_nodes);
}

// --- statistics ---------------------------------------------------------------

TEST_F(NUMAMemoryManagerTest, StatsLocalAccessCountIncremented) {
    mgr.reset_stats();
    void* p = mgr.allocate_local(64);
    auto stats = mgr.get_stats();
    // local + remote >= 1
    EXPECT_GE(stats.local_accesses + stats.remote_accesses, 1u);
    mgr.deallocate(p, 64);
}

TEST_F(NUMAMemoryManagerTest, StatsPerNodeAllocationsSize) {
    auto stats = mgr.get_stats();
    EXPECT_EQ(stats.per_node_allocations.size(), mgr.get_topology().num_nodes);
}

TEST_F(NUMAMemoryManagerTest, StatsLocalityRatioInRange) {
    mgr.reset_stats();
    void* p = mgr.allocate_local(128);
    auto stats = mgr.get_stats();
    EXPECT_GE(stats.locality_ratio, 0.0);
    EXPECT_LE(stats.locality_ratio, 1.0);
    mgr.deallocate(p, 128);
}

TEST_F(NUMAMemoryManagerTest, ResetStatsZeroesCounters) {
    mgr.allocate_local(64);
    mgr.reset_stats();
    auto stats = mgr.get_stats();
    EXPECT_EQ(stats.local_accesses, 0u);
    EXPECT_EQ(stats.remote_accesses, 0u);
}

// --- thread safety -----------------------------------------------------------

TEST_F(NUMAMemoryManagerTest, ConcurrentAllocationsThreadSafe) {
    constexpr int kThreads = 4, kAllocs = 32;
    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([this]() {
            for (int i = 0; i < kAllocs; ++i) {
                void* p = mgr.allocate_local(256);
                ASSERT_NE(p, nullptr);
                mgr.deallocate(p, 256);
            }
        });
    }
    for (auto& th : threads) th.join();
}

} // namespace
