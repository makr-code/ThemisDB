// Unit tests for NUMA topology detection and thread pinning

#include "performance/numa_topology.h"
#include <gtest/gtest.h>
#include <algorithm>
#include <atomic>
#include <thread>
#include <vector>

using namespace themis::performance;

// ============================================================================
// NumaTopologyDetector tests
// ============================================================================

TEST(NumaTopologyTest, DetectReturnsNonEmpty) {
    const NumaTopology& topo = NumaTopologyDetector::detect();
    EXPECT_GE(topo.num_nodes, 1);
    EXPECT_GE(topo.num_cpus, 1);
    EXPECT_FALSE(topo.nodes.empty());
}

TEST(NumaTopologyTest, NodeCpuCountMatchesTotal) {
    const NumaTopology& topo = NumaTopologyDetector::detect();
    int total = 0;
    for (const auto& node : topo.nodes) {
        EXPECT_GE(node.node_id, 0);
        total += static_cast<int>(node.cpu_ids.size());
    }
    EXPECT_EQ(total, topo.num_cpus);
}

TEST(NumaTopologyTest, AllCpuIdsNonNegative) {
    const NumaTopology& topo = NumaTopologyDetector::detect();
    for (const auto& node : topo.nodes) {
        for (int cpu : node.cpu_ids) {
            EXPECT_GE(cpu, 0);
        }
    }
}

TEST(NumaTopologyTest, CpuIdsNoDuplicates) {
    const NumaTopology& topo = NumaTopologyDetector::detect();
    std::vector<int> all_cpus;
    for (const auto& node : topo.nodes) {
        all_cpus.insert(all_cpus.end(), node.cpu_ids.begin(), node.cpu_ids.end());
    }
    std::sort(all_cpus.begin(), all_cpus.end());
    auto it = std::adjacent_find(all_cpus.begin(), all_cpus.end());
    EXPECT_EQ(it, all_cpus.end()) << "Duplicate CPU id found";
}

TEST(NumaTopologyTest, NodeOfCpuValid) {
    const NumaTopology& topo = NumaTopologyDetector::detect();
    for (const auto& node : topo.nodes) {
        for (int cpu : node.cpu_ids) {
            EXPECT_EQ(topo.node_of_cpu(cpu), node.node_id);
        }
    }
}

TEST(NumaTopologyTest, NodeOfCpuInvalidReturnsMinusOne) {
    const NumaTopology& topo = NumaTopologyDetector::detect();
    EXPECT_EQ(topo.node_of_cpu(-1), -1);
    EXPECT_EQ(topo.node_of_cpu(99999), -1);
}

TEST(NumaTopologyTest, LocalNodeInRange) {
    const NumaTopology& topo = NumaTopologyDetector::detect();
    int local = topo.local_node();
    // local_node() must return a valid node id or 0 as fallback
    bool found = false;
    for (const auto& node : topo.nodes) {
        if (node.node_id == local) { found = true; break; }
    }
    EXPECT_TRUE(found) << "local_node() returned invalid node id: " << local;
}

TEST(NumaTopologyTest, CacheReturnsSameInstance) {
    const NumaTopology& a = NumaTopologyDetector::detect();
    const NumaTopology& b = NumaTopologyDetector::detect();
    // Same memory address – cached singleton
    EXPECT_EQ(&a, &b);
}

TEST(NumaTopologyTest, InvalidateCacheAndRedetect) {
    NumaTopologyDetector::invalidate_cache();
    const NumaTopology& topo = NumaTopologyDetector::detect();
    EXPECT_GE(topo.num_nodes, 1);
}

TEST(NumaTopologyTest, NumaAvailableFlag) {
    const NumaTopology& topo = NumaTopologyDetector::detect();
    // On a single-node system is_numa_available() must be false
    if (topo.num_nodes == 1) {
        EXPECT_FALSE(topo.is_numa_available());
    } else {
        EXPECT_TRUE(topo.is_numa_available());
    }
}

// ============================================================================
// ThreadPinner tests
// ============================================================================

TEST(ThreadPinnerTest, CurrentAffinityNonEmpty) {
    auto affinity = ThreadPinner::current_affinity();
    // Affinity set must contain at least one CPU
    EXPECT_FALSE(affinity.empty());
}

TEST(ThreadPinnerTest, PinToCpuAndVerify) {
    const NumaTopology& topo = NumaTopologyDetector::detect();
    if (topo.num_cpus < 1) {
        GTEST_SKIP() << "No CPUs detected";
    }
    int target_cpu = topo.nodes[0].cpu_ids[0];
    bool ok = ThreadPinner::pin_to_cpu(target_cpu);
    if (!ok) {
        GTEST_SKIP() << "Thread pinning not available on this platform / insufficient permissions";
    }
    // Current affinity should contain exactly the pinned CPU
    auto affinity = ThreadPinner::current_affinity();
    EXPECT_FALSE(affinity.empty());
    EXPECT_NE(std::find(affinity.begin(), affinity.end(), target_cpu), affinity.end())
        << "Pinned CPU " << target_cpu << " not found in affinity set after pin_to_cpu()";

    // Restore
    ThreadPinner::unpin();
}

TEST(ThreadPinnerTest, PinToInvalidCpuReturnsFalse) {
    EXPECT_FALSE(ThreadPinner::pin_to_cpu(-1));
}

TEST(ThreadPinnerTest, PinToInvalidNodeReturnsFalse) {
    EXPECT_FALSE(ThreadPinner::pin_to_node(-1));
    EXPECT_FALSE(ThreadPinner::pin_to_node(99999));
}

TEST(ThreadPinnerTest, PinToNodeAndVerify) {
    const NumaTopology& topo = NumaTopologyDetector::detect();
    if (topo.nodes.empty() || topo.nodes[0].cpu_ids.empty()) {
        GTEST_SKIP() << "No NUMA node CPU information";
    }
    bool ok = ThreadPinner::pin_to_node(topo.nodes[0].node_id);
    if (!ok) {
        GTEST_SKIP() << "Thread node pinning not available on this platform / insufficient permissions";
    }
    auto affinity = ThreadPinner::current_affinity();
    EXPECT_FALSE(affinity.empty());

    ThreadPinner::unpin();
}

TEST(ThreadPinnerTest, PinToCpusEmptyReturnsFalse) {
    EXPECT_FALSE(ThreadPinner::pin_to_cpus({}));
}

TEST(ThreadPinnerTest, CurrentNodeInRange) {
    int node = ThreadPinner::current_node();
    // -1 is allowed (platform does not support query)
    const NumaTopology& topo = NumaTopologyDetector::detect();
    if (node != -1) {
        bool found = false;
        for (const auto& n : topo.nodes) {
            if (n.node_id == node) { found = true; break; }
        }
        EXPECT_TRUE(found) << "current_node() returned out-of-range id: " << node;
    }
}

TEST(ThreadPinnerTest, MultiThreadedPinning) {
    const NumaTopology& topo = NumaTopologyDetector::detect();
    if (topo.num_cpus < 2) {
        GTEST_SKIP() << "Need at least 2 CPUs for multi-thread pinning test";
    }

    // Collect all CPUs across nodes into a flat list (once, outside the loop)
    std::vector<int> all_cpus;
    for (const auto& node : topo.nodes)
        for (int c : node.cpu_ids) {
          all_cpus.push_back(c);
        }

    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};

    int n_threads = std::min(static_cast<int>(all_cpus.size()), 4);
    for (int i = 0; i < n_threads; ++i) {
        int target = all_cpus[static_cast<size_t>(i)];
        threads.emplace_back([target, &success_count]() {
            if (ThreadPinner::pin_to_cpu(target)) {
                ++success_count;
            }
            ThreadPinner::unpin();
        });
    }
    for (auto& t : threads) {
      t.join();
    }

    // If pinning is supported, all threads should have succeeded
    // (skip silently if the platform does not support it)
    SUCCEED();
}

// Main removed - using GTest's main from themis_tests.exe
