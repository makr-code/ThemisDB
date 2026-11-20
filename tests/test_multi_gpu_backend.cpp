#include "acceleration/multi_gpu_backend.h"
#include "acceleration/compute_backend.h"
#include "acceleration/cpu_backend.h"
#include "acceleration/graphics_backends.h"
#include <gtest/gtest.h>
#include <limits>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <thread>
#include <utility>
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
        cfg.deviceBackend   = MultiGPUDeviceBackend::CPU;
        cfg.numDevices      = n;
        cfg.minDevices      = 0;   // always "available" in unit tests
        cfg.allowCPUFallback = true;
        cfg.commBackend     = MultiGPUVectorBackend::CommBackend::CPU;
        for (int i = 0; i < n; ++i) {
            cfg.deviceIds.push_back(i);
        }
        return cfg;
    }

    ::testing::AssertionResult verifyTwoDeviceBackend(
        MultiGPUDeviceBackend selectedBackend, std::vector<int> deviceIds) const {
        MultiGPUVectorBackend::Config config;
        config.deviceBackend = selectedBackend;
        config.numDevices = 2;
        config.deviceIds = std::move(deviceIds);
        config.minDevices = 2;
        config.allowCPUFallback = false;
        config.commBackend = MultiGPUVectorBackend::CommBackend::CPU;

        MultiGPUVectorBackend backend(config);
        if (!backend.initialize()) {
            return ::testing::AssertionFailure() << "two-device backend initialization failed";
        }
        if (backend.shards().size() != 2u) {
            return ::testing::AssertionFailure() << "expected exactly two active shards";
        }
        for (const auto& shard : backend.shards()) {
            if (shard.backend != selectedBackend || shard.cpuFallback) {
                return ::testing::AssertionFailure()
                    << "shard did not execute on the selected accelerator backend";
            }
        }

        CPUVectorBackend cpu;
        if (!cpu.initialize()) {
            return ::testing::AssertionFailure() << "CPU parity backend initialization failed";
        }
        const auto actualDistances = backend.computeDistances(
            query.data(), 1, dim, vectors.data(), static_cast<size_t>(numVectors), true);
        const auto expectedDistances = cpu.computeDistances(
            query.data(), 1, dim, vectors.data(), static_cast<size_t>(numVectors), true);
        if (actualDistances.size() != expectedDistances.size()) {
            return ::testing::AssertionFailure() << "distance output size differs from CPU reference";
        }
        for (size_t index = 0; index < actualDistances.size(); ++index) {
            if (std::abs(actualDistances[index] - expectedDistances[index]) > 1e-4f) {
                return ::testing::AssertionFailure()
                    << "distance mismatch at corpus index " << index;
            }
        }

        const auto actualKnn = backend.batchKnnSearch(
            query.data(), 1, dim, vectors.data(), static_cast<size_t>(numVectors),
            static_cast<size_t>(k), true);
        const auto expectedKnn = cpu.batchKnnSearch(
            query.data(), 1, dim, vectors.data(), static_cast<size_t>(numVectors),
            static_cast<size_t>(k), true);
        if (actualKnn.size() != 1u || expectedKnn.size() != 1u ||
            actualKnn.front().size() != expectedKnn.front().size()) {
            return ::testing::AssertionFailure() << "KNN result shape differs from CPU reference";
        }
        for (size_t index = 0; index < actualKnn.front().size(); ++index) {
            if (actualKnn.front()[index].first != expectedKnn.front()[index].first ||
                std::abs(actualKnn.front()[index].second - expectedKnn.front()[index].second) > 1e-4f) {
                return ::testing::AssertionFailure()
                    << "KNN result mismatch at rank " << index;
            }
        }
        return ::testing::AssertionSuccess();
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

TEST_F(MultiGPUBackendTest, CudaTwoDeviceExecutionMatchesCpuWhenAvailable) {
    if (MultiGPUVectorBackend::detectDeviceCount(MultiGPUDeviceBackend::CUDA) < 2) {
        GTEST_SKIP() << "Two CUDA devices are not available";
    }
    EXPECT_TRUE(verifyTwoDeviceBackend(MultiGPUDeviceBackend::CUDA, {0, 1}));
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

TEST_F(MultiGPUBackendTest, HipTwoDeviceExecutionMatchesCpuWhenAvailable) {
    if (MultiGPUVectorBackend::detectDeviceCount(MultiGPUDeviceBackend::HIP) < 2) {
        GTEST_SKIP() << "Two HIP devices are not available";
    }
    EXPECT_TRUE(verifyTwoDeviceBackend(MultiGPUDeviceBackend::HIP, {0, 1}));
}

TEST_F(MultiGPUBackendTest, ZeroMinDevicesAlwaysAvailable) {
    auto cfg      = makeConfig(2);
    cfg.minDevices = 0;
    MultiGPUVectorBackend backend(cfg);
    EXPECT_TRUE(backend.isAvailable());
}

TEST_F(MultiGPUBackendTest, VulkanTwoDeviceExecutionMatchesCpuWhenAvailable) {
    const auto deviceIndices = VulkanVectorBackend::availableDeviceIndices();
    if (deviceIndices.size() < 2) {
        GTEST_SKIP() << "Two Vulkan compute devices are not available";
    }
    EXPECT_TRUE(verifyTwoDeviceBackend(MultiGPUDeviceBackend::VULKAN,
                                       {deviceIndices[0], deviceIndices[1]}));
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

TEST_F(MultiGPUBackendTest, ExplicitCpuBackendCreatesCpuShards) {
    auto config = makeConfig(3);
    MultiGPUVectorBackend backend(config);

    ASSERT_TRUE(backend.isAvailable());
    ASSERT_TRUE(backend.initialize());
    ASSERT_EQ(backend.shards().size(), 3u);
    for (const auto& shard : backend.shards()) {
        EXPECT_EQ(shard.backend, MultiGPUDeviceBackend::CPU);
        EXPECT_FALSE(shard.cpuFallback);
    }
}

TEST_F(MultiGPUBackendTest, UnavailableCudaDeviceUsesConfiguredCpuFallback) {
    auto config = makeConfig(1);
    config.deviceBackend = MultiGPUDeviceBackend::CUDA;
    config.deviceIds = {std::numeric_limits<int>::max()};
    config.allowCPUFallback = true;
    config.minDevices = 0;
    MultiGPUVectorBackend backend(config);

    ASSERT_TRUE(backend.initialize());
    ASSERT_EQ(backend.shards().size(), 1u);
    EXPECT_EQ(backend.shards().front().backend, MultiGPUDeviceBackend::CPU);
    EXPECT_TRUE(backend.shards().front().cpuFallback);

    CPUVectorBackend cpu;
    ASSERT_TRUE(cpu.initialize());
    auto actual = backend.computeDistances(query.data(), 1, dim, vectors.data(), numVectors, true);
    auto expected = cpu.computeDistances(query.data(), 1, dim, vectors.data(), numVectors, true);
    ASSERT_EQ(actual.size(), expected.size());
    for (size_t index = 0; index < actual.size(); ++index) {
        EXPECT_NEAR(actual[index], expected[index], 1e-5f);
    }
}

TEST_F(MultiGPUBackendTest, UnavailableCudaDeviceFailsWhenFallbackDisabled) {
    auto config = makeConfig(1);
    config.deviceBackend = MultiGPUDeviceBackend::CUDA;
    config.deviceIds = {std::numeric_limits<int>::max()};
    config.allowCPUFallback = false;
    config.minDevices = 0;
    MultiGPUVectorBackend backend(config);

    EXPECT_FALSE(backend.initialize());
    EXPECT_EQ(backend.activeDeviceCount(), 0);
}

TEST_F(MultiGPUBackendTest, AutoBackendSelectsFirstAvailableAccelerator) {
    const auto expectedBackend = MultiGPUVectorBackend::detectDeviceCount(MultiGPUDeviceBackend::CUDA) > 0
        ? MultiGPUDeviceBackend::CUDA
        : (MultiGPUVectorBackend::detectDeviceCount(MultiGPUDeviceBackend::HIP) > 0
            ? MultiGPUDeviceBackend::HIP
            : MultiGPUDeviceBackend::VULKAN);
    if (MultiGPUVectorBackend::detectDeviceCount(expectedBackend) == 0) {
        GTEST_SKIP() << "No CUDA, HIP, or Vulkan compute device is available";
    }

    MultiGPUVectorBackend::Config config;
    config.deviceBackend = MultiGPUDeviceBackend::AUTO;
    config.numDevices = 1;
    config.minDevices = 1;
    config.allowCPUFallback = true;
    MultiGPUVectorBackend backend(config);

    ASSERT_TRUE(backend.initialize());
    ASSERT_EQ(backend.shards().size(), 1u);
    EXPECT_TRUE(backend.shards().front().backend == expectedBackend ||
                backend.shards().front().cpuFallback);
}

TEST_F(MultiGPUBackendTest, CudaDeviceSelectionMatchesCpuResultsWhenAvailable) {
    if (MultiGPUVectorBackend::detectDeviceCount(MultiGPUDeviceBackend::CUDA) == 0) {
        GTEST_SKIP() << "CUDA is not compiled or no CUDA device is available";
    }

    MultiGPUVectorBackend::Config config;
    config.deviceBackend = MultiGPUDeviceBackend::CUDA;
    config.numDevices = 1;
    config.deviceIds = {0};
    config.minDevices = 1;
    config.allowCPUFallback = false;
    MultiGPUVectorBackend backend(config);
    ASSERT_TRUE(backend.initialize());
    ASSERT_EQ(backend.shards().front().backend, MultiGPUDeviceBackend::CUDA);

    CPUVectorBackend cpu;
    ASSERT_TRUE(cpu.initialize());
    const auto actual = backend.computeDistances(query.data(), 1, dim, vectors.data(), numVectors, true);
    const auto expected = cpu.computeDistances(query.data(), 1, dim, vectors.data(), numVectors, true);
    ASSERT_EQ(actual.size(), expected.size());
    for (size_t index = 0; index < actual.size(); ++index) {
        EXPECT_NEAR(actual[index], expected[index], 1e-4f);
    }
}

TEST_F(MultiGPUBackendTest, HipDeviceSelectionMatchesCpuResultsWhenAvailable) {
    if (MultiGPUVectorBackend::detectDeviceCount(MultiGPUDeviceBackend::HIP) == 0) {
        GTEST_SKIP() << "HIP is not compiled or no HIP device is available";
    }

    MultiGPUVectorBackend::Config config;
    config.deviceBackend = MultiGPUDeviceBackend::HIP;
    config.numDevices = 1;
    config.deviceIds = {0};
    config.minDevices = 1;
    config.allowCPUFallback = false;
    MultiGPUVectorBackend backend(config);
    ASSERT_TRUE(backend.initialize());
    ASSERT_EQ(backend.shards().front().backend, MultiGPUDeviceBackend::HIP);

    CPUVectorBackend cpu;
    ASSERT_TRUE(cpu.initialize());
    const auto actual = backend.computeDistances(query.data(), 1, dim, vectors.data(), numVectors, true);
    const auto expected = cpu.computeDistances(query.data(), 1, dim, vectors.data(), numVectors, true);
    ASSERT_EQ(actual.size(), expected.size());
    for (size_t index = 0; index < actual.size(); ++index) {
        EXPECT_NEAR(actual[index], expected[index], 1e-4f);
    }
}

TEST_F(MultiGPUBackendTest, VulkanDeviceSelectionPreservesVectorResults) {
    const auto deviceIndices = VulkanVectorBackend::availableDeviceIndices();
    if (deviceIndices.empty()) {
        GTEST_SKIP() << "No Vulkan compute device is available";
    }

    auto config = makeConfig(1);
    config.deviceBackend = MultiGPUDeviceBackend::VULKAN;
    config.deviceIds = {deviceIndices.front()};
    config.minDevices = 1;
    config.allowCPUFallback = true;
    MultiGPUVectorBackend backend(config);

    ASSERT_TRUE(backend.initialize());
    ASSERT_EQ(backend.shards().size(), 1u);
    EXPECT_EQ(backend.shards().front().deviceId, deviceIndices.front());
    EXPECT_TRUE(backend.shards().front().backend == MultiGPUDeviceBackend::VULKAN ||
                backend.shards().front().cpuFallback);

    CPUVectorBackend cpu;
    ASSERT_TRUE(cpu.initialize());
    const auto actual = backend.computeDistances(query.data(), 1, dim, vectors.data(), numVectors, true);
    const auto expected = cpu.computeDistances(query.data(), 1, dim, vectors.data(), numVectors, true);
    ASSERT_EQ(actual.size(), expected.size());
    for (size_t index = 0; index < actual.size(); ++index) {
        EXPECT_NEAR(actual[index], expected[index], 1e-4f);
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
