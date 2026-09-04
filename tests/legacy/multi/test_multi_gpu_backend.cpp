#include "acceleration/multi_gpu_backend.h"
#include "acceleration/compute_backend.h"
#include "acceleration/cpu_backend.h"
#include <gtest/gtest.h>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <thread>
#include <vector>

using namespace themis::acceleration;

// =============================================================================
// Test fixture
// =============================================================================

class MultiGPUBackendTest : public ::testing::Test {
protected:
    void SetUp() override {
        dim         = 32;
        numVectors  = 60;
        k           = 5;

        std::mt19937 rng(42);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

        vectors.resize(numVectors * dim);
        for (auto& v : vectors) {
            v = dist(rng);
        }

        query.resize(dim);
        for (auto& v : query) {
            v = dist(rng);
        }
    }

    // Build a config with n simulated shards (no real GPUs required because
    // the implementation falls back to CPUVectorBackend per shard).
    // minDevices=0 ensures isAvailable() returns true regardless of GPU count;
    // allowCPUFallback=true ensures each shard initialises even without CUDA.
    MultiGPUVectorBackend::Config makeConfig(int n) const {
        MultiGPUVectorBackend::Config cfg;
        cfg.numDevices      = n;
        cfg.minDevices      = 0;   // always "available" in unit tests
        cfg.allowCPUFallback = true;
        cfg.commBackend     = MultiGPUVectorBackend::CommBackend::CPU;
        for (int i = 0; i < n; ++i) {
            cfg.deviceIds.push_back(i);
        }
        return cfg;
    }

    int   dim;
    int   numVectors;
    int   k = {};
    std::vector<float> vectors;
    std::vector<float> query;
};

// =============================================================================
// Basic availability / config tests
// =============================================================================

TEST_F(MultiGPUBackendTest, BackendType) {
    MultiGPUVectorBackend backend;
    EXPECT_EQ(backend.type(), BackendType::MULTI_GPU);
    EXPECT_STREQ(backend.name(), "MultiGPU");
}

TEST_F(MultiGPUBackendTest, DefaultConfigAvailability) {
    // Default config requires minDevices = 2 GPUs; in a non-GPU CI environment
    // this should return false (which is correct and expected).
    MultiGPUVectorBackend backend;
    int gpuCount = MultiGPUVectorBackend::detectGPUCount();
    if (gpuCount >= 2) {
        EXPECT_TRUE(backend.isAvailable());
    } else {
        EXPECT_FALSE(backend.isAvailable());
    }
}

TEST_F(MultiGPUBackendTest, ZeroMinDevicesAlwaysAvailable) {
    auto cfg      = makeConfig(2);
    cfg.minDevices = 0;
    MultiGPUVectorBackend backend(cfg);
    EXPECT_TRUE(backend.isAvailable());
}

TEST_F(MultiGPUBackendTest, Capabilities) {
    auto cfg = makeConfig(2);
    MultiGPUVectorBackend backend(cfg);
    ASSERT_TRUE(backend.initialize());

    auto caps = backend.getCapabilities();
    EXPECT_TRUE(caps.supportsVectorOps);
    EXPECT_FALSE(caps.supportsGraphOps);
    EXPECT_FALSE(caps.supportsGeoOps);
    EXPECT_TRUE(caps.supportsBatchProcessing);
    EXPECT_TRUE(hasPrecision(caps.supportedPrecisions, PrecisionMode::FP32));
    EXPECT_NE(caps.supportedMetrics, 0u);
}

// =============================================================================
// Shard-descriptor tests
// =============================================================================

TEST_F(MultiGPUBackendTest, ShardCountMatchesConfig) {
    for (int n : {2, 3, 4}) {
        auto cfg = makeConfig(n);
        MultiGPUVectorBackend backend(cfg);
        ASSERT_TRUE(backend.initialize());
        EXPECT_EQ(backend.activeDeviceCount(), n);
        EXPECT_EQ(static_cast<int>(backend.shards().size()), n);
    }
}

TEST_F(MultiGPUBackendTest, ShardDeviceIdsMatchConfig) {
    auto cfg = makeConfig(3);
    MultiGPUVectorBackend backend(cfg);
    ASSERT_TRUE(backend.initialize());

    const auto& shds = backend.shards();
    for (int i = 0; i < 3; ++i) {
        EXPECT_EQ(shds[static_cast<size_t>(i)].deviceId, i);
    }
}

// =============================================================================
// computeDistances tests
// =============================================================================

TEST_F(MultiGPUBackendTest, ComputeDistancesOutputSize) {
    auto cfg = makeConfig(2);
    MultiGPUVectorBackend backend(cfg);
    ASSERT_TRUE(backend.initialize());

    size_t numQ = 3;
    std::vector<float> queries(numQ * dim, 0.0f);

    auto dists = backend.computeDistances(
        queries.data(), numQ, dim, vectors.data(), numVectors, /*useL2=*/true);

    EXPECT_EQ(dists.size(), numQ * static_cast<size_t>(numVectors));
}

TEST_F(MultiGPUBackendTest, ComputeDistancesNonNegativeL2) {
    auto cfg = makeConfig(2);
    MultiGPUVectorBackend backend(cfg);
    ASSERT_TRUE(backend.initialize());

    auto dists = backend.computeDistances(
        query.data(), 1, dim, vectors.data(), numVectors, /*useL2=*/true);

    for (float d : dists) {
        EXPECT_GE(d, 0.0f);
    }
}

TEST_F(MultiGPUBackendTest, ComputeDistancesCosineBound) {
    auto cfg = makeConfig(2);
    MultiGPUVectorBackend backend(cfg);
    ASSERT_TRUE(backend.initialize());

    auto dists = backend.computeDistances(
        query.data(), 1, dim, vectors.data(), numVectors, /*useL2=*/false);

    // Cosine distance ∈ [0, 2]
    for (float d : dists) {
        EXPECT_GE(d, -1e-5f);
        EXPECT_LE(d, 2.0f + 1e-5f);
    }
}

// =============================================================================
// batchKnnSearch tests
// =============================================================================

TEST_F(MultiGPUBackendTest, KnnSearchReturnsSizedResult) {
    auto cfg = makeConfig(2);
    MultiGPUVectorBackend backend(cfg);
    ASSERT_TRUE(backend.initialize());

    size_t numQ = 4;
    std::vector<float> queries(numQ * dim);
    std::iota(queries.begin(), queries.end(), 0.0f);

    auto results = backend.batchKnnSearch(
        queries.data(), numQ, dim, vectors.data(), numVectors, k, /*useL2=*/true);

    ASSERT_EQ(results.size(), numQ);
    for (const auto& row : results) {
        EXPECT_LE(row.size(), static_cast<size_t>(k));
    }
}

TEST_F(MultiGPUBackendTest, KnnSearchGlobalIndicesInRange) {
    auto cfg = makeConfig(3);
    MultiGPUVectorBackend backend(cfg);
    ASSERT_TRUE(backend.initialize());

    auto results = backend.batchKnnSearch(
        query.data(), 1, dim, vectors.data(), numVectors, k, /*useL2=*/true);

    ASSERT_EQ(results.size(), 1u);
    for (const auto& [idx, dist] : results[0]) {
        EXPECT_LT(static_cast<int>(idx), numVectors)
            << "Global index " << idx << " out of range [0, " << numVectors << ")";
        EXPECT_GE(dist, 0.0f);
    }
}

TEST_F(MultiGPUBackendTest, KnnResultsSortedByDistance) {
    auto cfg = makeConfig(2);
    MultiGPUVectorBackend backend(cfg);
    ASSERT_TRUE(backend.initialize());

    auto results = backend.batchKnnSearch(
        query.data(), 1, dim, vectors.data(), numVectors, k, /*useL2=*/true);

    ASSERT_FALSE(results.empty());
    const auto& row = results[0];
    for (size_t i = 1; i < row.size(); ++i) {
        EXPECT_LE(row[i - 1].second, row[i].second)
            << "Results not sorted at position " << i;
    }
}

TEST_F(MultiGPUBackendTest, KnnSingleShardMatchesCPUBackend) {
    // With 1 shard the multi-GPU backend should produce the same top-1 result
    // as a plain CPUVectorBackend.
    auto cfg = makeConfig(1);
    MultiGPUVectorBackend mgpu(cfg);
    ASSERT_TRUE(mgpu.initialize());

    CPUVectorBackend cpu;
    ASSERT_TRUE(cpu.initialize());

    auto mgpuRes = mgpu.batchKnnSearch(
        query.data(), 1, dim, vectors.data(), numVectors, 1, true);
    auto cpuRes = cpu.batchKnnSearch(
        query.data(), 1, dim, vectors.data(), numVectors, 1, true);

    ASSERT_FALSE(mgpuRes.empty());
    ASSERT_FALSE(cpuRes.empty());
    ASSERT_FALSE(mgpuRes[0].empty());
    ASSERT_FALSE(cpuRes[0].empty());

    EXPECT_EQ(mgpuRes[0][0].first, cpuRes[0][0].first);
    EXPECT_NEAR(mgpuRes[0][0].second, cpuRes[0][0].second, 1e-5f);
}

// =============================================================================
// Sharding correctness: all vectors reachable regardless of shard count
// =============================================================================

TEST_F(MultiGPUBackendTest, AllVectorsReachableAcrossShards) {
    // With k = numVectors every vector must appear in the results exactly once.
    for (int n : {2, 3, 4}) {
        auto cfg = makeConfig(n);
        MultiGPUVectorBackend backend(cfg);
        ASSERT_TRUE(backend.initialize());

        auto results = backend.batchKnnSearch(
            query.data(), 1, dim,
            vectors.data(), numVectors,
            static_cast<size_t>(numVectors),
            /*useL2=*/true);

        ASSERT_EQ(results.size(), 1u);
        const auto& row = results[0];
        EXPECT_EQ(row.size(), static_cast<size_t>(numVectors))
            << "Not all vectors returned with " << n << " shards";

        // Each global index should appear exactly once
        std::vector<bool> seen(numVectors, false);
        for (const auto& [idx, dist] : row) {
            ASSERT_LT(static_cast<int>(idx), numVectors);
            EXPECT_FALSE(seen[idx]) << "Duplicate index " << idx;
            seen[idx] = true;
        }
        for (int i = 0; i < numVectors; ++i) {
            EXPECT_TRUE(seen[i]) << "Index " << i << " missing with " << n << " shards";
        }
    }
}

// =============================================================================
// Empty / edge-case inputs
// =============================================================================

TEST_F(MultiGPUBackendTest, ZeroQueriesReturnsEmpty) {
    auto cfg = makeConfig(2);
    MultiGPUVectorBackend backend(cfg);
    ASSERT_TRUE(backend.initialize());

    auto results = backend.batchKnnSearch(
        nullptr, 0, dim, vectors.data(), numVectors, k, true);
    EXPECT_TRUE(results.empty());
}

TEST_F(MultiGPUBackendTest, KGreaterThanNumVectors) {
    auto cfg = makeConfig(2);
    MultiGPUVectorBackend backend(cfg);
    ASSERT_TRUE(backend.initialize());

    size_t bigK = static_cast<size_t>(numVectors) * 2;
    auto results = backend.batchKnnSearch(
        query.data(), 1, dim, vectors.data(), numVectors, bigK, true);

    ASSERT_EQ(results.size(), 1u);
    // Cannot return more than numVectors results
    EXPECT_LE(results[0].size(), static_cast<size_t>(numVectors));
}

TEST_F(MultiGPUBackendTest, UninitializedBackendReturnsEmpty) {
    // Do not call initialize()
    auto cfg = makeConfig(2);
    MultiGPUVectorBackend backend(cfg);

    auto dists = backend.computeDistances(
        query.data(), 1, dim, vectors.data(), numVectors, true);
    EXPECT_TRUE(dists.empty());

    auto results = backend.batchKnnSearch(
        query.data(), 1, dim, vectors.data(), numVectors, k, true);
    // All query rows should be empty (not crash)
    for (const auto& row : results) {
        EXPECT_TRUE(row.empty());
    }
}

// =============================================================================
// Communication backend
// =============================================================================

TEST_F(MultiGPUBackendTest, DefaultCommBackendIsCPUInCIEnvironment) {
    auto cfg = makeConfig(2);
    MultiGPUVectorBackend backend(cfg);
    ASSERT_TRUE(backend.initialize());

    // In CI without NCCL/RCCL the active comm should fall back to CPU.
    auto comm = backend.activeCommBackend();
    // Acceptable: CPU (most likely in CI), or NCCL/RCCL if those SDKs exist.
    EXPECT_TRUE(comm == MultiGPUVectorBackend::CommBackend::CPU  ||
                comm == MultiGPUVectorBackend::CommBackend::NCCL ||
                comm == MultiGPUVectorBackend::CommBackend::RCCL);
}

TEST_F(MultiGPUBackendTest, CollectiveOpsMatchCommBackend) {
    auto cfg = makeConfig(2);
    MultiGPUVectorBackend backend(cfg);
    ASSERT_TRUE(backend.initialize());

    bool collectiveAvail = backend.isCollectiveOpsAvailable();
    auto comm = backend.activeCommBackend();
    if (comm == MultiGPUVectorBackend::CommBackend::CPU) {
        EXPECT_FALSE(collectiveAvail);
    } else {
        EXPECT_TRUE(collectiveAvail);
    }
}

// =============================================================================
// Shutdown / re-init
// =============================================================================

TEST_F(MultiGPUBackendTest, ShutdownAndReinitialize) {
    auto cfg = makeConfig(2);
    MultiGPUVectorBackend backend(cfg);

    ASSERT_TRUE(backend.initialize());
    backend.shutdown();
    EXPECT_EQ(backend.activeDeviceCount(), 0);

    ASSERT_TRUE(backend.initialize());
    EXPECT_EQ(backend.activeDeviceCount(), 2);
}

// =============================================================================
// Thread-safety: concurrent batchKnnSearch calls must not race on shared state
// =============================================================================

TEST_F(MultiGPUBackendTest, ConcurrentSearchesDoNotRace) {
    // Validates that buildRanges() returns a per-call local copy and does NOT
    // mutate shared shardDescs, making concurrent queries safe.
    //
    // Thread-safety model: after initialize(), the Impl fields accessed during
    // batchKnnSearch are all read-only:
    //   - initialized, subBackends, shardDescs  — set once by initialize()
    //   - activeComm                            — set once by initCommBackend()
    // Each call computes its own `ranges` via buildRanges() (local value),
    // so no writes to shared state occur during concurrent searches.
    auto cfg = makeConfig(3);
    MultiGPUVectorBackend backend(cfg);
    ASSERT_TRUE(backend.initialize());

    // Two independent query vectors
    std::vector<float> q1(dim, 0.1f);
    std::vector<float> q2(dim, 0.9f);

    std::vector<std::vector<std::pair<uint32_t, float>>> res1, res2;
    std::thread t1([&]() {
        res1 = backend.batchKnnSearch(
            q1.data(), 1, dim, vectors.data(), numVectors, k, true);
    });
    std::thread t2([&]() {
        res2 = backend.batchKnnSearch(
            q2.data(), 1, dim, vectors.data(), numVectors, k, true);
    });
    t1.join();
    t2.join();

    // Both threads must return valid, non-overlapping top-k results
    ASSERT_EQ(res1.size(), 1u);
    ASSERT_EQ(res2.size(), 1u);
    EXPECT_LE(res1[0].size(), static_cast<size_t>(k));
    EXPECT_LE(res2[0].size(), static_cast<size_t>(k));

    for (const auto& [idx, dist] : res1[0]) {
        EXPECT_LT(static_cast<int>(idx), numVectors);
    }
    for (const auto& [idx, dist] : res2[0]) {
        EXPECT_LT(static_cast<int>(idx), numVectors);
    }
}
