/**
 * @file test_gpu_stream_cuda_bridge.cpp
 * @brief Unit tests for GPUStreamManager CudaStreamBackendFn bridge (STUB #77).
 *
 * Verifies that the CudaStreamBackendFn injection works correctly in
 * non-CUDA builds (THEMIS_ENABLE_CUDA not defined):
 *   GSM-CB-01  No fn set              → ROCm/CPU fallback used; stream created.
 *   GSM-CB-02  Fn returns BackendFn   → stream created with injected backend.
 *   GSM-CB-03  Fn throws exception    → fall back to ROCm/CPU (fail-safe).
 *
 * Tests run in builds WITHOUT THEMIS_ENABLE_CUDA.  In CUDA builds the injected
 * fn is never reached and the tests are skipped.
 */

#include <gtest/gtest.h>
#include "themis/gpu/stream_manager.h"

#include <stdexcept>
#include <atomic>

using namespace themis::gpu;

class GpuStreamCudaBridgeTest : public ::testing::Test {
protected:
    void TearDown() override {
        GPUStreamManager::setCudaStreamBackendFn({});  // restore clean state
    }
};

// ── GSM-CB-01 ────────────────────────────────────────────────────────────────
// With no fn registered, createCudaStream() falls back to the ROCm/CPU backend
// and still returns true (stream successfully created).
TEST_F(GpuStreamCudaBridgeTest, NoFnUsesRocmFallback) {
#ifdef THEMIS_ENABLE_CUDA
    GTEST_SKIP() << "capability:stub_path_active=false;reason=real_cuda_path_active";
#endif
    GPUStreamManager::setCudaStreamBackendFn({});  // ensure clean state

    GPUStreamManager mgr;
    GPUStreamManager::StreamConfig cfg;
    cfg.name = "test_stream_no_fn";

    bool ok = mgr.createCudaStream(cfg, 0);
    EXPECT_TRUE(ok) << "createCudaStream() must succeed even without injected fn";
    EXPECT_TRUE(mgr.hasStream("test_stream_no_fn"));
}

// ── GSM-CB-02 ────────────────────────────────────────────────────────────────
// An injected CudaStreamBackendFn is called and its returned BackendFn is used
// to create the stream.
TEST_F(GpuStreamCudaBridgeTest, InjectedFnIsCalledAndStreamIsCreated) {
#ifdef THEMIS_ENABLE_CUDA
    GTEST_SKIP() << "capability:stub_path_active=false;reason=real_cuda_path_active";
#endif

    std::atomic<int> backend_fn_call_count{0};
    std::atomic<int> factory_call_count{0};
    const int expected_device = 2;
    int received_device = -1;

    GPUStreamManager::setCudaStreamBackendFn(
        [&](int device_index) -> GPULauncher::BackendFn {
            ++factory_call_count;
            received_device = device_index;
            // Return a simple CPU-dispatch backend fn.
            return [&](const GPULauncher::WorkItem& item) -> bool {
                ++backend_fn_call_count;
                (void)item;
                return true;
            };
        });

    GPUStreamManager mgr;
    GPUStreamManager::StreamConfig cfg;
    cfg.name = "test_stream_injected";

    bool ok = mgr.createCudaStream(cfg, expected_device);
    EXPECT_TRUE(ok) << "createCudaStream() must succeed with injected fn";
    EXPECT_EQ(factory_call_count.load(), 1) << "factory fn must be called once";
    EXPECT_EQ(received_device, expected_device)
        << "factory fn must receive the correct device index";
    EXPECT_TRUE(mgr.hasStream("test_stream_injected"));
}

// ── GSM-CB-03 ────────────────────────────────────────────────────────────────
// If the injected fn throws, createCudaStream() must fall back to the ROCm/CPU
// backend and still return true (fail-safe, not fail-closed).
TEST_F(GpuStreamCudaBridgeTest, ThrowingFnFallsBackToRocm) {
#ifdef THEMIS_ENABLE_CUDA
    GTEST_SKIP() << "capability:stub_path_active=false;reason=real_cuda_path_active";
#endif

    GPUStreamManager::setCudaStreamBackendFn(
        [](int /*device_index*/) -> GPULauncher::BackendFn {
            throw std::runtime_error("simulated CUDA init error");
        });

    GPUStreamManager mgr;
    GPUStreamManager::StreamConfig cfg;
    cfg.name = "test_stream_throw";

    bool ok = false;
    EXPECT_NO_THROW(ok = mgr.createCudaStream(cfg, 0))
        << "Throwing CudaStreamBackendFn must not propagate exception";
    EXPECT_TRUE(ok) << "createCudaStream() must succeed via ROCm fallback";
    EXPECT_TRUE(mgr.hasStream("test_stream_throw"));
}
