/**
 * @file test_zluda_backend.cpp
 * @brief GTest suite for the ZludaKernelFn injection bridge in ZLUDAVectorBackend.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Status: Production Ready
 *
 * Exercises the setZludaKernelFn() bridge API without requiring real ZLUDA
 * hardware.  Because the ZludaKernelFn bridge is checked before the
 * initialized_ guard inside computeDistances() and batchKnnSearch(), these
 * tests run correctly on any host without an AMD GPU.
 */


#ifdef THEMIS_ENABLE_ZLUDA

#include <gtest/gtest.h>
#include "acceleration/zluda_backend.h"

#include <atomic>
#include <cstdint>
#include <thread>
#include <vector>

using namespace themis::acceleration;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

/// Build a trivial flat distance vector: distance[q][v] = (float)(q * 10 + v).
std::vector<float> makeFlatDistances(size_t numQueries, size_t numVectors) {
    std::vector<float> d(numQueries * numVectors);
    for (size_t q = 0; q < numQueries; ++q)
        for (size_t v = 0; v < numVectors; ++v)
            d[q * numVectors + v] = static_cast<float>(q * 10 + v);
    return d;
}

/// RAII guard that clears the ZludaKernelFn bridge after each test so that
/// static state does not leak between test cases.
struct ZludaKernelFnGuard {
    ~ZludaKernelFnGuard() { setZludaKernelFn(ZludaKernelFn{}); }
};

} // namespace

// ---------------------------------------------------------------------------
// Test 1: Default behaviour — no fn set → CPU fallback, no crash
// ---------------------------------------------------------------------------

/**
 * @brief Verifies that computeDistances() and batchKnnSearch() return the CPU
 *        fallback (empty vector) without crashing when no ZludaKernelFn is
 *        injected and the backend is not initialized.
 */
TEST(ZludaBackendBridgeTest, DefaultNoFn_ReturnsCpuFallback_NoCrash) {
    ZludaKernelFnGuard guard;

    auto backend = createZLUDABackend();
    ASSERT_NE(backend, nullptr);

    // Not initialized — bridge not set — should return {} without crashing.
    const float q[2] = {1.0f, 0.0f};
    const float v[4] = {1.0f, 0.0f, 0.0f, 1.0f};

    auto distances = backend->computeDistances(q, 1, 2, v, 2);
    EXPECT_TRUE(distances.empty())
        << "Expected CPU fallback (empty result) when no bridge is injected";

    auto knn = backend->batchKnnSearch(q, 1, 2, v, 2, 1);
    EXPECT_TRUE(knn.empty())
        << "Expected CPU fallback (empty result) when no bridge is injected";
}

// ---------------------------------------------------------------------------
// Test 2: computeDistances delegates to injected ZludaKernelFn
// ---------------------------------------------------------------------------

/**
 * @brief Verifies that computeDistances() calls the injected ZludaKernelFn
 *        and returns its result verbatim when the function returns a non-empty
 *        vector.
 */
TEST(ZludaBackendBridgeTest, ComputeDistances_WithKernelFn_CallsFn) {
    ZludaKernelFnGuard guard;

    std::atomic<int> callCount{0};
    const size_t numQueries  = 2;
    const size_t numVectors  = 3;
    const size_t dim         = 4;

    // Inject a kernel fn that returns a known distance matrix.
    setZludaKernelFn([&](const std::vector<float>& in) -> std::vector<float> {
        ++callCount;
        // Validate input size: queries + vectors packed flat.
        EXPECT_EQ(in.size(), numQueries * dim + numVectors * dim);
        return makeFlatDistances(numQueries, numVectors);
    });

    auto backend = createZLUDABackend();

    std::vector<float> queries(numQueries * dim, 0.5f);
    std::vector<float> vectors(numVectors * dim, 0.1f);

    auto result = backend->computeDistances(
        queries.data(), numQueries, dim,
        vectors.data(), numVectors);

    EXPECT_EQ(callCount.load(), 1) << "ZludaKernelFn should have been called exactly once";
    ASSERT_EQ(result.size(), numQueries * numVectors);
    // Check a couple of known values from makeFlatDistances.
    EXPECT_FLOAT_EQ(result[0], 0.0f);  // q=0, v=0 → 0*10+0 = 0
    EXPECT_FLOAT_EQ(result[1], 1.0f);  // q=0, v=1 → 0*10+1 = 1
    EXPECT_FLOAT_EQ(result[3], 10.0f); // q=1, v=0 → 1*10+0 = 10
}

// ---------------------------------------------------------------------------
// Test 3: batchKnnSearch delegates to ZludaKernelFn and builds top-k result
// ---------------------------------------------------------------------------

/**
 * @brief Verifies that batchKnnSearch() calls the injected ZludaKernelFn,
 *        interprets its output as a flat numQueries×numVectors distance
 *        matrix, and performs top-k selection correctly.
 */
TEST(ZludaBackendBridgeTest, BatchKnnSearch_WithKernelFn_BuildsTopK) {
    ZludaKernelFnGuard guard;

    const size_t numQueries = 1;
    const size_t numVectors = 5;
    const size_t dim        = 2;
    const size_t k          = 3;

    // Distances for query 0: [4, 1, 3, 0, 2]
    // Top-3 (ascending): index=3 (d=0), index=1 (d=1), index=4 (d=2)
    setZludaKernelFn([&](const std::vector<float>& in) -> std::vector<float> {
        EXPECT_EQ(in.size(), numQueries * dim + numVectors * dim);
        return {4.0f, 1.0f, 3.0f, 0.0f, 2.0f};
    });

    auto backend = createZLUDABackend();

    std::vector<float> queries(numQueries * dim, 0.0f);
    std::vector<float> vectors(numVectors * dim, 0.0f);

    auto result = backend->batchKnnSearch(
        queries.data(), numQueries, dim,
        vectors.data(), numVectors, k);

    ASSERT_EQ(result.size(), numQueries);
    const auto& hits = result[0];
    ASSERT_EQ(hits.size(), k);

    // First hit must be the nearest (distance 0.0).
    EXPECT_EQ(hits[0].first,  static_cast<uint32_t>(3));
    EXPECT_FLOAT_EQ(hits[0].second, 0.0f);

    // Second hit: distance 1.0, index 1.
    EXPECT_EQ(hits[1].first,  static_cast<uint32_t>(1));
    EXPECT_FLOAT_EQ(hits[1].second, 1.0f);

    // Third hit: distance 2.0, index 4.
    EXPECT_EQ(hits[2].first,  static_cast<uint32_t>(4));
    EXPECT_FLOAT_EQ(hits[2].second, 2.0f);
}

// ---------------------------------------------------------------------------
// Test 4: Thread-safety — set fn from one thread, call from another
// ---------------------------------------------------------------------------

/**
 * @brief Smoke test for the mutex-guarded static storage.
 *
 * Sets the ZludaKernelFn from a background thread and then calls
 * computeDistances() from the main thread; verifies no data race and that
 * the injected fn is observed correctly.
 */
TEST(ZludaBackendBridgeTest, ThreadSafety_SetFromOneThread_CallFromAnother) {
    ZludaKernelFnGuard guard;

    const size_t numQueries = 1;
    const size_t numVectors = 2;
    const size_t dim        = 2;

    std::atomic<bool> fnSet{false};
    std::atomic<int>  callCount{0};

    // Setter thread: inject the fn and signal readiness.
    std::thread setter([&] {
        setZludaKernelFn([&](const std::vector<float>&) -> std::vector<float> {
            ++callCount;
            return {0.5f, 1.5f}; // numQueries * numVectors distances
        });
        fnSet.store(true, std::memory_order_release);
    });
    setter.join();

    // Caller thread: wait until setter has finished, then call backend.
    ASSERT_TRUE(fnSet.load(std::memory_order_acquire));

    auto backend = createZLUDABackend();

    std::vector<float> queries(numQueries * dim, 0.0f);
    std::vector<float> vectors(numVectors * dim, 0.0f);

    auto result = backend->computeDistances(
        queries.data(), numQueries, dim,
        vectors.data(), numVectors);

    EXPECT_EQ(callCount.load(), 1);
    ASSERT_EQ(result.size(), numQueries * numVectors);
    EXPECT_FLOAT_EQ(result[0], 0.5f);
    EXPECT_FLOAT_EQ(result[1], 1.5f);
}

#endif // THEMIS_ENABLE_ZLUDA
