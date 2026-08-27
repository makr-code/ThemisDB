/**
 * @file test_gpu_stream_raii.cpp
 * @brief STREAM-RAII-01..10 — CudaStreamGuard adoption + GPUStreamManager RAII tests.
 *
 * Wave A Phase C prerequisite: raw `uintptr_t` CUDA stream handles replaced by
 * `CudaStreamGuard` with `adopt()` factory.  These tests verify the non-CUDA
 * (CPU-stub) build path so they run in CI without CUDA hardware.
 *
 * Tests cover:
 *   STREAM-RAII-01  — default-constructed guard is empty (not valid)
 *   STREAM-RAII-02  — adopt(nullptr) produces an empty guard
 *   STREAM-RAII-03  — move-constructed guard transfers ownership
 *   STREAM-RAII-04  — move-assigned guard transfers ownership
 *   STREAM-RAII-05  — adopted guard destructs without crash on non-CUDA build
 *   STREAM-RAII-06  — GPUStreamManager createStream / destroyStream round-trip
 *   STREAM-RAII-07  — destroyStream returns false for unknown stream
 *   STREAM-RAII-08  — createStream rejects duplicate names
 *   STREAM-RAII-09  — destructor cleans up all streams without crashing
 *   STREAM-RAII-10  — concurrent createStream + destroyStream is race-free
 */

#include <atomic>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "gpu/cuda_raii.h"
#include "themis/gpu/stream_manager.h"

// ---------------------------------------------------------------------------
// CudaStreamGuard unit tests (CPU-stub path)
// ---------------------------------------------------------------------------

TEST(StreamRaii, STREAM_RAII_01_DefaultIsEmpty) {
    themis::gpu::CudaStreamGuard g;
    EXPECT_FALSE(g.isValid());
}

TEST(StreamRaii, STREAM_RAII_02_AdoptNullptrIsEmpty) {
    auto g = themis::gpu::CudaStreamGuard::adopt(nullptr);
    EXPECT_FALSE(g.isValid());
}

TEST(StreamRaii, STREAM_RAII_03_MoveConstructTransfersOwnership) {
    // On non-CUDA builds, cudaStream_t is `void*`; use a sentinel pointer.
    auto* sentinel = reinterpret_cast<decltype(themis::gpu::CudaStreamGuard{}.stream)>(0xDEADBEEF);
    auto g1 = themis::gpu::CudaStreamGuard::adopt(sentinel);
    EXPECT_TRUE(g1.isValid());

    themis::gpu::CudaStreamGuard g2(std::move(g1));
    EXPECT_TRUE(g2.isValid());
    EXPECT_FALSE(g1.isValid()); // NOLINT(bugprone-use-after-move) — intentional post-move check
    // Prevent actual cudaStreamDestroy being called on the sentinel in CUDA builds:
    // release ownership manually before the guard destructs.
    g2.stream = nullptr;
}

TEST(StreamRaii, STREAM_RAII_04_MoveAssignTransfersOwnership) {
    auto* sentinel = reinterpret_cast<decltype(themis::gpu::CudaStreamGuard{}.stream)>(0xCAFEBABE);
    auto g1 = themis::gpu::CudaStreamGuard::adopt(sentinel);
    themis::gpu::CudaStreamGuard g2;

    g2 = std::move(g1);
    EXPECT_TRUE(g2.isValid());
    EXPECT_FALSE(g1.isValid()); // NOLINT(bugprone-use-after-move)
    g2.stream = nullptr;
}

TEST(StreamRaii, STREAM_RAII_05_NullAdoptedGuardDestructsCleanly) {
    // On non-CUDA builds the destructor is a no-op; on CUDA builds
    // cudaStreamDestroy(nullptr) is a safe no-op per CUDA docs.
    {
        auto g = themis::gpu::CudaStreamGuard::adopt(nullptr);
        // Let g destruct normally.
    }
    SUCCEED();
}

// ---------------------------------------------------------------------------
// GPUStreamManager RAII integration tests
// ---------------------------------------------------------------------------

namespace {
static themis::gpu::GPUStreamManager::StreamConfig makeCfg(const std::string& name) {
    themis::gpu::GPUStreamManager::StreamConfig cfg;
    cfg.name = name;
    return cfg;
}
} // namespace

TEST(StreamRaii, STREAM_RAII_06_CreateDestroyRoundTrip) {
    themis::gpu::GPUStreamManager mgr;
    ASSERT_TRUE(mgr.createStream(makeCfg("s1")));
    EXPECT_EQ(mgr.streamCount(), 1u);
    EXPECT_TRUE(mgr.destroyStream("s1"));
    EXPECT_EQ(mgr.streamCount(), 0u);
}

TEST(StreamRaii, STREAM_RAII_07_DestroyUnknownReturnsFalse) {
    themis::gpu::GPUStreamManager mgr;
    EXPECT_FALSE(mgr.destroyStream("nonexistent"));
}

TEST(StreamRaii, STREAM_RAII_08_CreateDuplicateReturnsFalse) {
    themis::gpu::GPUStreamManager mgr;
    ASSERT_TRUE(mgr.createStream(makeCfg("dup")));
    EXPECT_FALSE(mgr.createStream(makeCfg("dup")));
    EXPECT_EQ(mgr.streamCount(), 1u);
}

TEST(StreamRaii, STREAM_RAII_09_DestructorCleansUpAllStreams) {
    {
        themis::gpu::GPUStreamManager mgr;
        for (int i = 0; i < 8; ++i) {
            mgr.createStream(makeCfg("s" + std::to_string(i)));
        }
        EXPECT_EQ(mgr.streamCount(), 8u);
        // mgr destructs here — no crash, no leak (RAII).
    }
    SUCCEED();
}

TEST(StreamRaii, STREAM_RAII_10_ConcurrentCreateDestroyIsRaceFree) {
    themis::gpu::GPUStreamManager mgr;

    constexpr int kThreads = 4;
    constexpr int kIter    = 20;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    std::atomic<int> errors{0};

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&mgr, &errors, t]() {
            for (int i = 0; i < kIter; ++i) {
                const std::string name =
                    "t" + std::to_string(t) + "_s" + std::to_string(i);
                try {
                    mgr.createStream(makeCfg(name));
                    mgr.destroyStream(name);
                } catch (...) {
                    ++errors;
                }
            }
        });
    }
    for (auto& th : threads) th.join();

    EXPECT_EQ(errors.load(), 0);
}
