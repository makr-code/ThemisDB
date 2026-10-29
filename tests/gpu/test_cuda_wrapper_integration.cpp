#include <gtest/gtest.h>

#include <chrono>
#include <thread>
#include <utility>

#include "gpu/cuda_raii.h"
#include "themis/gpu/gpu_timeout.h"

namespace themis::gpu::test {

TEST(CudaWrapperIntegration, StreamGuardMovementAndLifecycle) {
#if THEMIS_CUDA_RAII_HAS_CUDA
   CudaStreamGuard guard;
   if (!guard.isValid()) {
       GTEST_SKIP() << "CUDA runtime is not available in this environment";
   }

   CudaStreamGuard moved(std::move(guard));
   EXPECT_TRUE(moved.isValid());
   EXPECT_FALSE(guard.isValid());
#else
   CudaStreamGuard guard;
   EXPECT_FALSE(guard.isValid());
   auto adopted = CudaStreamGuard::adopt(nullptr);
   EXPECT_FALSE(adopted.isValid());
#endif
}

TEST(CudaWrapperIntegration, EventGuardCreationIsDeterministic) {
#if THEMIS_CUDA_RAII_HAS_CUDA
   CudaEventGuard guard;
   if (!guard.isValid()) {
       GTEST_SKIP() << "CUDA runtime is not available in this environment";
   }

   CudaEventGuard moved(std::move(guard));
   EXPECT_TRUE(moved.isValid());
   EXPECT_FALSE(guard.isValid());
#else
   CudaEventGuard guard;
   EXPECT_FALSE(guard.isValid());
#endif
}

TEST(CudaWrapperIntegration, DeviceMemoryGuardAllocatesAndReleases) {
#if THEMIS_CUDA_RAII_HAS_CUDA
   CudaDeviceMemoryGuard guard(4096);
   if (!guard.isValid()) {
       GTEST_SKIP() << "CUDA memory allocation is unavailable in this environment";
   }

   EXPECT_EQ(guard.bytes, 4096u);
   EXPECT_NE(guard.ptr, nullptr);

   void* released = guard.release();
   EXPECT_NE(released, nullptr);
   EXPECT_EQ(guard.ptr, nullptr);
   EXPECT_EQ(guard.bytes, 0u);
#else
   CudaDeviceMemoryGuard guard(4096);
   EXPECT_FALSE(guard.isValid());
   EXPECT_EQ(guard.bytes, 4096u);
#endif
}

TEST(CudaWrapperIntegration, KernelTimeoutGuardFailsClosedOnExceededBudget) {
   KernelSLAGuard guard(std::chrono::milliseconds(1));
   std::this_thread::sleep_for(std::chrono::milliseconds(10));
   EXPECT_TRUE(guard.checkTimeoutDeadline());
   EXPECT_GE(guard.getElapsedTime(), std::chrono::milliseconds(1));
}

}  // namespace themis::gpu::test
// ==============================================================================
// All tests registered as `release_critical` for Wave A Gate compliance
// Evidence collected in: src/gpu/WAVE_A_CLOSURE_EVIDENCE_BUNDLE_2026-09-15.md
// Audit tool reference: tools/ci/gpu_cuda_call_audit.py
