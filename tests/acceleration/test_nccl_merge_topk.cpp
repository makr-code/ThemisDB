// Integration tests for NCCL/RCCL distributed mergeTopK().
// Test suite name: NCCLMergeTopKFocusedTests
//
// Coverage:
//   Section A — CPU-side merge algorithm simulation (no hardware required)
//     A-01: Simulated merge worldSize=2, k=10  — global top-k is correct
//     A-02: Simulated merge worldSize=2, k=100 — global top-k is correct
//     A-03: Simulated merge worldSize=2, k=256 — global top-k is correct
//     A-04: Simulated merge worldSize=4, k=10  — global top-k is correct
//     A-05: Simulated merge worldSize=4, k=100 — global top-k is correct
//     A-06: Simulated merge worldSize=4, k=256 — global top-k is correct
//     A-07: Simulated merge worldSize=8, k=10  — global top-k is correct
//     A-08: Simulated merge worldSize=8, k=100 — global top-k is correct
//     A-09: Simulated merge worldSize=8, k=256 — global top-k is correct
//     A-10: Tie-breaking: duplicated distances resolved deterministically
//     A-11: All-equal distances: all candidates are tied; top-k selects any k
//     A-12: Already-sorted per-rank input — merge still returns global top-k
//     A-13: k equals totalK — all candidates returned sorted
//
//   Section B — NCCLVectorBackend single-rank path (skipped without NCCL hardware)
//     B-01: worldSize=1, k=10  — copy path is correct
//     B-02: worldSize=1, k=100 — copy path is correct
//     B-03: worldSize=1, k=256 — copy path is correct
//     B-04: worldSize=1, k=0   — no-op returns true
//     B-05: k > localK is rejected (returns false)
//     B-06: uninitialized backend returns false
//
//   Section C — RCCLVectorBackend single-rank path (skipped without RCCL hardware)
//     C-01: worldSize=1, k=10  — copy path is correct
//     C-02: worldSize=1, k=100 — copy path is correct
//     C-03: worldSize=1, k=256 — copy path is correct
//     C-04: worldSize=1, k=0   — no-op returns true
//     C-05: k > localK is rejected (returns false)
//     C-06: uninitialized backend returns false

#include <gtest/gtest.h>
#include "acceleration/nccl_vector_backend.h"
#include "acceleration/rccl_vector_backend.h"

#include <algorithm>
#include <cfloat>
#include <cstdint>
#include <numeric>
#include <random>
#include <vector>

#ifdef THEMIS_ENABLE_NCCL
#include <cuda_runtime.h>
#endif

#ifdef THEMIS_ENABLE_RCCL
#include <hip/hip_runtime.h>
#endif

using namespace themis::acceleration;

// =============================================================================
// Section A — CPU-side merge algorithm simulation (no hardware required)
//
// These tests validate the correctness of the merge-sort algorithm used by
// NCCLVectorBackend::mergeTopK() and RCCLVectorBackend::mergeTopK().
// The algorithm:
//   1. Assume AllGather has produced a flat gathered buffer of
//      worldSize × localK candidates on every rank.
//   2. std::partial_sort over all candidates by ascending distance.
//   3. Copy the top-k (smallest distances) to the output.
//
// By running the same algorithm here on synthetic "gathered" data we can
// verify correctness for arbitrary (worldSize, k) combinations without
// requiring any GPU hardware.
// =============================================================================

namespace {

struct Candidate {
    uint32_t index = 0;
    float    distance;
};

// Sentinel value representing an "invalid" / unfilled candidate slot.
constexpr Candidate kInvalidCandidate{static_cast<uint32_t>(-1), FLT_MAX};

// Helper: compute the global index for rank r, slot i with localK items per rank.
inline uint32_t globalIndex(int r, size_t localK, size_t i) {
    return static_cast<uint32_t>(static_cast<size_t>(r) * localK + i);
}

// Simulate the host-side merge used by mergeTopK():
// 1. Build a flat gathered vector of worldSize × localK candidates.
// 2. std::partial_sort the first k elements by ascending distance.
// 3. Return the top-k candidates.
std::vector<Candidate> simulateMerge(const std::vector<std::vector<Candidate>>& perRankData,
                                     size_t k) {
    // Flatten (simulates AllGather result)
    std::vector<uint32_t> h_indices;
    std::vector<float>    h_distances = {};

    for (const auto& rank_data : perRankData) {
        for (const auto& c : rank_data) {
            h_indices.push_back(c.index);
            h_distances.push_back(c.distance);
        }
    }

    const size_t totalK  = h_indices.size();
    const size_t selectK = (k < totalK) ? k : totalK;

    std::vector<size_t> order(totalK);
    std::iota(order.begin(), order.end(), size_t{0});
    std::partial_sort(order.begin(), order.begin() + static_cast<ptrdiff_t>(selectK), order.end(),
                      [&](size_t a, size_t b) {
                          return h_distances[a] < h_distances[b];
                      });

    std::vector<Candidate> result(k, kInvalidCandidate);
    for (size_t i = 0; i < selectK; ++i) {
        result[i] = {h_indices[order[i]], h_distances[order[i]]};
    }
    return result;
}

// Brute-force reference: merge all candidates and sort fully, then take top-k.
std::vector<Candidate> referenceMerge(const std::vector<std::vector<Candidate>>& perRankData,
                                      size_t k) {
    std::vector<Candidate> all = {};

    for (const auto& rank_data : perRankData)
        for (const auto& c : rank_data)
            all.push_back(c);

    std::sort(all.begin(), all.end(),
              [](const Candidate& a, const Candidate& b) {
                  return a.distance < b.distance;
              });

    const size_t selectK = (k < all.size()) ? k : all.size();
    return std::vector<Candidate>(all.begin(), all.begin() + static_cast<ptrdiff_t>(selectK));
}

// Generate synthetic per-rank data: each rank r contributes localK candidates
// with globally unique indices (r*localK + i) and random distances.
std::vector<std::vector<Candidate>> generatePerRankData(int worldSize, size_t localK,
                                                         uint32_t seed = 42) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist(0.0f, 1000.0f);

    std::vector<std::vector<Candidate>> perRank(worldSize);
    for (int r = 0; r < worldSize; ++r) {
        perRank[r].resize(localK);
        for (size_t i = 0; i < localK; ++i) {
            perRank[r][i] = {globalIndex(r, localK, i), dist(rng)};
        }
    }
    return perRank;
}

void assertMergeCorrectness(int worldSize, size_t k) {
    // Use localK = k so each rank contributes exactly k candidates.
    const size_t localK = k;
    auto perRank = generatePerRankData(worldSize, localK);

    auto result    = simulateMerge(perRank, k);
    auto reference = referenceMerge(perRank, k);

    ASSERT_EQ(result.size(),    k);
    ASSERT_EQ(reference.size(), k);

    // The result must be sorted by ascending distance.
    for (size_t i = 0; i + 1 < result.size(); ++i) {
        EXPECT_LE(result[i].distance, result[i + 1].distance)
            << "Result not sorted at position " << i
            << " (worldSize=" << worldSize << ", k=" << k << ")";
    }

    // Every result distance must be ≤ the k-th reference distance.
    // (i.e. simulateMerge must not return a candidate with a larger distance
    //  than what the reference merge would have included.)
    for (size_t i = 0; i < k; ++i) {
        EXPECT_FLOAT_EQ(result[i].distance, reference[i].distance)
            << "Distance mismatch at position " << i
            << " (worldSize=" << worldSize << ", k=" << k << ")";
    }
}

} // anonymous namespace

// =============================================================================
// Section A test cases
// =============================================================================

class NCCLMergeTopKSimulationTest : public ::testing::Test {};

// A-01 through A-09: worldSize × k matrix
TEST_F(NCCLMergeTopKSimulationTest, A01_WorldSize2_K10)  { assertMergeCorrectness(2, 10);  }
TEST_F(NCCLMergeTopKSimulationTest, A02_WorldSize2_K100) { assertMergeCorrectness(2, 100); }
TEST_F(NCCLMergeTopKSimulationTest, A03_WorldSize2_K256) { assertMergeCorrectness(2, 256); }
TEST_F(NCCLMergeTopKSimulationTest, A04_WorldSize4_K10)  { assertMergeCorrectness(4, 10);  }
TEST_F(NCCLMergeTopKSimulationTest, A05_WorldSize4_K100) { assertMergeCorrectness(4, 100); }
TEST_F(NCCLMergeTopKSimulationTest, A06_WorldSize4_K256) { assertMergeCorrectness(4, 256); }
TEST_F(NCCLMergeTopKSimulationTest, A07_WorldSize8_K10)  { assertMergeCorrectness(8, 10);  }
TEST_F(NCCLMergeTopKSimulationTest, A08_WorldSize8_K100) { assertMergeCorrectness(8, 100); }
TEST_F(NCCLMergeTopKSimulationTest, A09_WorldSize8_K256) { assertMergeCorrectness(8, 256); }

// A-10: Tie-breaking — if two candidates have the same distance, the merged
// output must still be of length k and sorted.
TEST_F(NCCLMergeTopKSimulationTest, A10_TieBreaking_DuplicatedDistances) {
    const int    worldSize = 4;
    const size_t k         = 10;
    const size_t localK    = k;

    // All candidates share the same distance value to create ties.
    std::vector<std::vector<Candidate>> perRank(worldSize);
    for (int r = 0; r < worldSize; ++r) {
        perRank[r].resize(localK);
        for (size_t i = 0; i < localK; ++i) {
            perRank[r][i] = {globalIndex(r, localK, i), 1.0f};
        }
    }

    auto result = simulateMerge(perRank, k);
    ASSERT_EQ(result.size(), k);
    for (const auto& c : result) {
        EXPECT_FLOAT_EQ(c.distance, 1.0f);
    }
}

// A-11: All-equal distances — top-k result must still have k elements.
TEST_F(NCCLMergeTopKSimulationTest, A11_AllEqualDistances) {
    const int    worldSize = 2;
    const size_t k         = 5;
    const size_t localK    = 5;

    std::vector<std::vector<Candidate>> perRank(worldSize);
    for (int r = 0; r < worldSize; ++r) {
        perRank[r].resize(localK, {0u, 0.5f});
        for (size_t i = 0; i < localK; ++i) {
            perRank[r][i].index = globalIndex(r, localK, i);
        }
    }

    auto result = simulateMerge(perRank, k);
    ASSERT_EQ(result.size(), k);
    for (const auto& c : result) {
        EXPECT_FLOAT_EQ(c.distance, 0.5f);
    }
}

// A-12: Pre-sorted per-rank input — each rank provides already-sorted
// candidates; the merged result must be the global top-k.
TEST_F(NCCLMergeTopKSimulationTest, A12_PresortedPerRankInput) {
    const int    worldSize = 4;
    const size_t k         = 20;
    const size_t localK    = 20;

    // Rank r contributes distances [r*0.1, r*0.1+0.001, ..., r*0.1+(localK-1)*0.001]
    std::vector<std::vector<Candidate>> perRank(worldSize);
    for (int r = 0; r < worldSize; ++r) {
        perRank[r].resize(localK);
        for (size_t i = 0; i < localK; ++i) {
            perRank[r][i] = {
                globalIndex(r, localK, i),
                static_cast<float>(r) * 0.1f + static_cast<float>(i) * 0.001f
            };
        }
    }

    auto result    = simulateMerge(perRank, k);
    auto reference = referenceMerge(perRank, k);

    ASSERT_EQ(result.size(), k);
    for (size_t i = 0; i < k; ++i) {
        EXPECT_FLOAT_EQ(result[i].distance, reference[i].distance)
            << "Distance mismatch at " << i;
    }
}

// A-13: k equals totalK — all candidates returned, output is fully sorted.
TEST_F(NCCLMergeTopKSimulationTest, A13_K_EqualsTotal) {
    const int    worldSize = 2;
    const size_t localK    = 5;
    const size_t k         = static_cast<size_t>(worldSize) * localK;

    auto perRank = generatePerRankData(worldSize, localK, /*seed=*/99);
    auto result  = simulateMerge(perRank, k);

    ASSERT_EQ(result.size(), k);
    for (size_t i = 0; i + 1 < result.size(); ++i) {
        EXPECT_LE(result[i].distance, result[i + 1].distance)
            << "Result not sorted at position " << i;
    }
}

// =============================================================================
// Section B — NCCLVectorBackend single-rank path
// (Skipped when NCCL hardware is not available)
// =============================================================================

#ifdef THEMIS_ENABLE_NCCL

class NCCLMergeTopKSingleRankTest : public ::testing::Test {
protected:
    NCCLVectorBackend backend_;
    bool              initialized_ = false;

    void SetUp() override {
        if (!NCCLVectorBackend::isNCCLAvailable()) {
            GTEST_SKIP() << "NCCL hardware not available";
        }
        NCCLVectorBackend::Config cfg;
        cfg.worldSize = 1;
        cfg.rank      = 0;
        cfg.deviceIds = {0};
        initialized_ = backend_.initialize(cfg);
        if (!initialized_) {
            GTEST_SKIP() << "NCCLVectorBackend initialization failed";
        }
    }

    // Helper: allocate device buffers, run mergeTopK with worldSize=1, verify
    // that globalIndices/globalDistances match the local inputs exactly.
    void runSingleRankMerge(size_t k) {
        // Generate sorted (ascending distance) local top-k
        std::vector<uint32_t> h_local_idx(k);
        std::vector<float>    h_local_dist(k);
        for (size_t i = 0; i < k; ++i) {
            h_local_idx[i]  = static_cast<uint32_t>(i);
            h_local_dist[i] = static_cast<float>(i) * 0.01f;
        }

        uint32_t *d_li, *d_gi;
        float    *d_ld, *d_gd;
        ASSERT_EQ(cudaMalloc(&d_li, k * sizeof(uint32_t)), cudaSuccess);
        ASSERT_EQ(cudaMalloc(&d_ld, k * sizeof(float)),    cudaSuccess);
        ASSERT_EQ(cudaMalloc(&d_gi, k * sizeof(uint32_t)), cudaSuccess);
        ASSERT_EQ(cudaMalloc(&d_gd, k * sizeof(float)),    cudaSuccess);

        ASSERT_EQ(cudaMemcpy(d_li, h_local_idx.data(),  k * sizeof(uint32_t), cudaMemcpyHostToDevice), cudaSuccess);
        ASSERT_EQ(cudaMemcpy(d_ld, h_local_dist.data(), k * sizeof(float),    cudaMemcpyHostToDevice), cudaSuccess);

        EXPECT_TRUE(backend_.mergeTopK(d_li, d_ld, k, d_gi, d_gd, k, 0, nullptr));

        std::vector<uint32_t> h_global_idx(k);
        std::vector<float>    h_global_dist(k);
        ASSERT_EQ(cudaMemcpy(h_global_idx.data(),  d_gi, k * sizeof(uint32_t), cudaMemcpyDeviceToHost), cudaSuccess);
        ASSERT_EQ(cudaMemcpy(h_global_dist.data(), d_gd, k * sizeof(float),    cudaMemcpyDeviceToHost), cudaSuccess);

        for (size_t i = 0; i < k; ++i) {
            EXPECT_EQ(h_global_idx[i],  h_local_idx[i])  << "index mismatch at " << i;
            EXPECT_FLOAT_EQ(h_global_dist[i], h_local_dist[i]) << "distance mismatch at " << i;
        }

        cudaFree(d_li); cudaFree(d_ld); cudaFree(d_gi); cudaFree(d_gd);
    }
};

// B-01 through B-03: single-rank copy path for each required k value
TEST_F(NCCLMergeTopKSingleRankTest, B01_WorldSize1_K10)  { runSingleRankMerge(10);  }
TEST_F(NCCLMergeTopKSingleRankTest, B02_WorldSize1_K100) { runSingleRankMerge(100); }
TEST_F(NCCLMergeTopKSingleRankTest, B03_WorldSize1_K256) { runSingleRankMerge(256); }

// B-04: k=0 is a valid no-op
TEST_F(NCCLMergeTopKSingleRankTest, B04_WorldSize1_K0_NoOp) {
    EXPECT_TRUE(backend_.mergeTopK(nullptr, nullptr, 0u, nullptr, nullptr, 0u, 0, nullptr));
}

// B-05: k > localK must be rejected
TEST_F(NCCLMergeTopKSingleRankTest, B05_RejectsKGreaterThanLocalK) {
    EXPECT_FALSE(backend_.mergeTopK(nullptr, nullptr, 5u, nullptr, nullptr, 10u, 0, nullptr))
        << "Expected mergeTopK to reject k > localK (k=10, localK=5)";
}

#endif // THEMIS_ENABLE_NCCL

// B-06: uninitialized backend must return false (no hardware guard needed)
TEST(NCCLMergeTopKTest, B06_UninitializedBackend_ReturnsFalse) {
    NCCLVectorBackend backend;
    EXPECT_FALSE(backend.mergeTopK(nullptr, nullptr, 10u, nullptr, nullptr, 5u, 0, nullptr));
}

// =============================================================================
// Section C — RCCLVectorBackend single-rank path
// (Skipped when RCCL hardware is not available)
// =============================================================================

#ifdef THEMIS_ENABLE_RCCL

class RCCLMergeTopKSingleRankTest : public ::testing::Test {
protected:
    RCCLVectorBackend backend_;
    bool              initialized_ = false;

    void SetUp() override {
        if (!RCCLVectorBackend::isRCCLAvailable()) {
            GTEST_SKIP() << "RCCL hardware not available";
        }
        RCCLVectorBackend::Config cfg;
        cfg.worldSize = 1;
        cfg.rank      = 0;
        cfg.deviceIds = {0};
        initialized_ = backend_.initialize(cfg);
        if (!initialized_) {
            GTEST_SKIP() << "RCCLVectorBackend initialization failed";
        }
    }

    void runSingleRankMerge(size_t k) {
        std::vector<uint32_t> h_local_idx(k);
        std::vector<float>    h_local_dist(k);
        for (size_t i = 0; i < k; ++i) {
            h_local_idx[i]  = static_cast<uint32_t>(i);
            h_local_dist[i] = static_cast<float>(i) * 0.01f;
        }

        uint32_t *d_li, *d_gi;
        float    *d_ld, *d_gd;
        ASSERT_EQ(hipMalloc(&d_li, k * sizeof(uint32_t)), hipSuccess);
        ASSERT_EQ(hipMalloc(&d_ld, k * sizeof(float)),    hipSuccess);
        ASSERT_EQ(hipMalloc(&d_gi, k * sizeof(uint32_t)), hipSuccess);
        ASSERT_EQ(hipMalloc(&d_gd, k * sizeof(float)),    hipSuccess);

        ASSERT_EQ(hipMemcpy(d_li, h_local_idx.data(),  k * sizeof(uint32_t), hipMemcpyHostToDevice), hipSuccess);
        ASSERT_EQ(hipMemcpy(d_ld, h_local_dist.data(), k * sizeof(float),    hipMemcpyHostToDevice), hipSuccess);

        EXPECT_TRUE(backend_.mergeTopK(d_li, d_ld, k, d_gi, d_gd, k, 0, nullptr));

        std::vector<uint32_t> h_global_idx(k);
        std::vector<float>    h_global_dist(k);
        ASSERT_EQ(hipMemcpy(h_global_idx.data(),  d_gi, k * sizeof(uint32_t), hipMemcpyDeviceToHost), hipSuccess);
        ASSERT_EQ(hipMemcpy(h_global_dist.data(), d_gd, k * sizeof(float),    hipMemcpyDeviceToHost), hipSuccess);

        for (size_t i = 0; i < k; ++i) {
            EXPECT_EQ(h_global_idx[i],  h_local_idx[i])  << "index mismatch at " << i;
            EXPECT_FLOAT_EQ(h_global_dist[i], h_local_dist[i]) << "distance mismatch at " << i;
        }

        hipFree(d_li); hipFree(d_ld); hipFree(d_gi); hipFree(d_gd);
    }
};

// C-01 through C-03: single-rank copy path for each required k value
TEST_F(RCCLMergeTopKSingleRankTest, C01_WorldSize1_K10)  { runSingleRankMerge(10);  }
TEST_F(RCCLMergeTopKSingleRankTest, C02_WorldSize1_K100) { runSingleRankMerge(100); }
TEST_F(RCCLMergeTopKSingleRankTest, C03_WorldSize1_K256) { runSingleRankMerge(256); }

// C-04: k=0 is a valid no-op
TEST_F(RCCLMergeTopKSingleRankTest, C04_WorldSize1_K0_NoOp) {
    EXPECT_TRUE(backend_.mergeTopK(nullptr, nullptr, 0u, nullptr, nullptr, 0u, 0, nullptr));
}

// C-05: k > localK must be rejected
TEST_F(RCCLMergeTopKSingleRankTest, C05_RejectsKGreaterThanLocalK) {
    EXPECT_FALSE(backend_.mergeTopK(nullptr, nullptr, 5u, nullptr, nullptr, 10u, 0, nullptr))
        << "Expected mergeTopK to reject k > localK (k=10, localK=5)";
}

#endif // THEMIS_ENABLE_RCCL

// C-06: uninitialized backend must return false (no hardware guard needed)
TEST(RCCLMergeTopKTest, C06_UninitializedBackend_ReturnsFalse) {
    RCCLVectorBackend backend;
    EXPECT_FALSE(backend.mergeTopK(nullptr, nullptr, 10u, nullptr, nullptr, 5u, 0, nullptr));
}
