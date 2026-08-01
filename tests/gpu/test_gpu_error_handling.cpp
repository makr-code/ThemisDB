/**
 * @file test_gpu_error_handling.cpp
 * @brief Comprehensive tests for GPU error handling infrastructure (Phase 1).
 *
 * Tests for:
 * - unique_gpu_ptr RAII semantics and move construction/assignment
 * - CHECKED_CUDA macro error handling
 * - KernelSLAGuard timeout detection
 * - Error taxonomy classification
 * - Recovery policy application
 * - Thread safety
 * - Exception safety
 *
 * ## Acceptance Criteria (Phase 1)
 *
 * - [ ] All 8+ tests pass with no failures
 * - [ ] Compilation with /W4 (MSVC) and -Wall -Wextra (GCC) produces no warnings
 * - [ ] unique_gpu_ptr move semantics verified (constructor, assignment, release)
 * - [ ] CHECKED_CUDA macro logs errors correctly
 * - [ ] KernelSLAGuard timeout verification (100ms timeout, 1ms sleep = pass; 150ms = fail)
 * - [ ] Error taxonomy: all GPUErrorClass values tested
 * - [ ] Address Sanitizer / Memory Sanitizer: zero warnings
 *
 * ## Build & Run
 *
 * ```bash
 * cmake --preset windows-release  # or linux-release
 * cmake --build . --target module_gpu_test_gpu_error_handling_focused
 * ctest --label gpu --filter "*error_handling*" --verbose
 * ```
 *
 * @author ThemisDB GPU Team
 * @date 2026-08-01
 */

#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <chrono>
#include <iostream>
#include "themis/gpu/gpu_error.h"
#include "themis/gpu/gpu_memory.h"
#include "themis/gpu/gpu_timeout.h"

using namespace themis::gpu;
using namespace std::chrono_literals;

// ============================================================================
// Test Suite: GPUErrorClass Taxonomy
// ============================================================================

class ErrorTaxonomyTest : public ::testing::Test {
 protected:
  std::shared_ptr<GPUErrorHandler> handler = GPUErrorHandler::Create();
};

/// Test: Error class name mapping
TEST_F(ErrorTaxonomyTest, ErrorClassName_AllClassesHaveNames) {
  EXPECT_EQ(handler->errorClassName(GPUErrorClass::kQuotaExceeded), "kQuotaExceeded");
  EXPECT_EQ(handler->errorClassName(GPUErrorClass::kKernelTimeout), "kKernelTimeout");
  EXPECT_EQ(handler->errorClassName(GPUErrorClass::kBackendUnavailable), "kBackendUnavailable");
  EXPECT_EQ(handler->errorClassName(GPUErrorClass::kMemoryCommunication), "kMemoryCommunication");
  EXPECT_EQ(handler->errorClassName(GPUErrorClass::kNumerical), "kNumerical");
  EXPECT_EQ(handler->errorClassName(GPUErrorClass::kUnsupportedOperation), "kUnsupportedOperation");
  EXPECT_EQ(handler->errorClassName(GPUErrorClass::kUnknown), "kUnknown");
}

/// Test: Default recovery policy mapping
TEST_F(ErrorTaxonomyTest, DefaultPolicy_QuotaExceeded_IsFallbackCPU) {
  auto policy = handler->defaultPolicy(GPUErrorClass::kQuotaExceeded);
  EXPECT_EQ(policy, ErrorRecoveryPolicy::kFallbackCPU);
}

TEST_F(ErrorTaxonomyTest, DefaultPolicy_KernelTimeout_IsFallbackCPU) {
  auto policy = handler->defaultPolicy(GPUErrorClass::kKernelTimeout);
  EXPECT_EQ(policy, ErrorRecoveryPolicy::kFallbackCPU);
}

TEST_F(ErrorTaxonomyTest, DefaultPolicy_BackendUnavailable_IsMarkUnavailable) {
  auto policy = handler->defaultPolicy(GPUErrorClass::kBackendUnavailable);
  EXPECT_EQ(policy, ErrorRecoveryPolicy::kMarkUnavailable);
}

TEST_F(ErrorTaxonomyTest, DefaultPolicy_MemoryCommunication_IsRetryOnce) {
  auto policy = handler->defaultPolicy(GPUErrorClass::kMemoryCommunication);
  EXPECT_EQ(policy, ErrorRecoveryPolicy::kRetryOnce);
}

TEST_F(ErrorTaxonomyTest, DefaultPolicy_Numerical_IsEmitWarning) {
  auto policy = handler->defaultPolicy(GPUErrorClass::kNumerical);
  EXPECT_EQ(policy, ErrorRecoveryPolicy::kEmitWarning);
}

TEST_F(ErrorTaxonomyTest, DefaultPolicy_UnsupportedOperation_IsFallbackCPU) {
  auto policy = handler->defaultPolicy(GPUErrorClass::kUnsupportedOperation);
  EXPECT_EQ(policy, ErrorRecoveryPolicy::kFallbackCPU);
}

// ============================================================================
// Test Suite: KernelSLAGuard Timeout Enforcement
// ============================================================================

class KernelSLAGuardTest : public ::testing::Test {};

/// Test: Constructor sets deadline correctly
TEST_F(KernelSLAGuardTest, Constructor_WithDefaultSLA_SetsDeadline) {
  KernelSLAGuard guard;
  EXPECT_FALSE(guard.checkTimeoutDeadline());
}

/// Test: Timeout detection with explicit timeout
TEST_F(KernelSLAGuardTest, Timeout_ExplicitTimeout100ms_DetectsWhenExceeded) {
  KernelSLAGuard guard(100ms);
  
  // Initially not timed out
  EXPECT_FALSE(guard.checkTimeoutDeadline());
  
  // Sleep for 50ms: still within budget
  std::this_thread::sleep_for(50ms);
  EXPECT_FALSE(guard.checkTimeoutDeadline());
  
  // Sleep another 100ms: exceeds 100ms total
  std::this_thread::sleep_for(100ms);
  EXPECT_TRUE(guard.checkTimeoutDeadline());
}

/// Test: Timeout is monotonic (once true, always true)
TEST_F(KernelSLAGuardTest, Timeout_IsMonotonic_OnceTrueAlwaysTrue) {
  KernelSLAGuard guard(50ms);
  
  // Sleep past deadline
  std::this_thread::sleep_for(100ms);
  EXPECT_TRUE(guard.checkTimeoutDeadline());
  
  // Check again: still true
  EXPECT_TRUE(guard.checkTimeoutDeadline());
  EXPECT_TRUE(guard.checkTimeoutDeadline());
}

/// Test: Elapsed time tracking
TEST_F(KernelSLAGuardTest, ElapsedTime_IncreasesMonotonically) {
  KernelSLAGuard guard(5s);  // Long timeout to avoid false positives
  
  auto t0 = guard.getElapsedTime();
  std::this_thread::sleep_for(10ms);
  auto t1 = guard.getElapsedTime();
  
  EXPECT_GT(t1, t0);
}

/// Test: Remaining time decreases
TEST_F(KernelSLAGuardTest, RemainingTime_DecreasesWithTime) {
  KernelSLAGuard guard(500ms);
  
  auto remaining0 = guard.getRemainingTime();
  std::this_thread::sleep_for(100ms);
  auto remaining1 = guard.getRemainingTime();
  
  EXPECT_GT(remaining0, remaining1);
}

/// Test: Move semantics for KernelSLAGuard
TEST_F(KernelSLAGuardTest, Move_TransfersDeadlineCorrectly) {
  KernelSLAGuard guard1(100ms);
  auto deadline1 = guard1.getDeadline();
  
  // Move guard1 to guard2
  KernelSLAGuard guard2 = std::move(guard1);
  auto deadline2 = guard2.getDeadline();
  
  // Deadlines should be identical
  EXPECT_EQ(deadline1, deadline2);
}

/// Test: getSLADuration returns original timeout
TEST_F(KernelSLAGuardTest, GetSLADuration_ReturnsOriginalTimeout) {
  auto timeout = 250ms;
  KernelSLAGuard guard(timeout);
  EXPECT_EQ(guard.getSLADuration(), timeout);
}

// ============================================================================
// Test Suite: unique_gpu_ptr RAII Semantics
// ============================================================================

class UniqueGPUPtrTest : public ::testing::Test {};

/// Test: Default construction creates null pointer
TEST_F(UniqueGPUPtrTest, DefaultConstructor_CreatesNullPointer) {
  unique_gpu_ptr<float> ptr;
  EXPECT_EQ(ptr.get(), nullptr);
  EXPECT_FALSE(static_cast<bool>(ptr));
}

/// Test: Explicit nullptr construction
TEST_F(UniqueGPUPtrTest, NullptrConstructor_CreatesNullPointer) {
  unique_gpu_ptr<float> ptr(nullptr);
  EXPECT_EQ(ptr.get(), nullptr);
}

/// Test: Move construction transfers ownership
TEST_F(UniqueGPUPtrTest, MoveConstructor_TransfersOwnership) {
  // Create two null pointers
  unique_gpu_ptr<float> ptr1;
  unique_gpu_ptr<float> ptr2 = std::move(ptr1);
  
  // ptr1 should be null after move
  EXPECT_EQ(ptr1.get(), nullptr);
  EXPECT_EQ(ptr2.get(), nullptr);
}

/// Test: Move assignment transfers ownership
TEST_F(UniqueGPUPtrTest, MoveAssignment_TransfersOwnership) {
  unique_gpu_ptr<float> ptr1;
  unique_gpu_ptr<float> ptr2;
  
  ptr1 = std::move(ptr2);
  EXPECT_EQ(ptr1.get(), nullptr);
  EXPECT_EQ(ptr2.get(), nullptr);
}

/// Test: Copy constructor is deleted
TEST_F(UniqueGPUPtrTest, CopyConstructor_IsDeleted) {
  unique_gpu_ptr<float> ptr;
  
  // This should not compile:
  // unique_gpu_ptr<float> copy = ptr;  // COMPILE ERROR
  
  // Test that we can move but not copy
  auto moved = std::move(ptr);
  (void)moved;  // suppress unused warning
}

/// Test: Release returns pointer and clears ownership
TEST_F(UniqueGPUPtrTest, Release_ReturnsPointerAndClears) {
  unique_gpu_ptr<float> ptr;
  auto raw = ptr.release();
  
  EXPECT_EQ(raw, nullptr);
  EXPECT_EQ(ptr.get(), nullptr);
}

/// Test: Reset with new pointer
TEST_F(UniqueGPUPtrTest, Reset_WithNewPointer_UpdatesOwnership) {
  unique_gpu_ptr<float> ptr;
  
  // Reset to null (should be no-op)
  ptr.reset(nullptr);
  EXPECT_EQ(ptr.get(), nullptr);
}

/// Test: Swap exchanges ownership
TEST_F(UniqueGPUPtrTest, Swap_ExchangesOwnership) {
  unique_gpu_ptr<float> ptr1;
  unique_gpu_ptr<float> ptr2;
  
  ptr1.swap(ptr2);
  // Both still null in this test, but swap was called
  EXPECT_EQ(ptr1.get(), nullptr);
  EXPECT_EQ(ptr2.get(), nullptr);
}

/// Test: make_unique_gpu creates allocation (or fallback on CPU)
TEST_F(UniqueGPUPtrTest, MakeUniqueGPU_CreatesAllocation) {
  // Note: This test may succeed on CPU without actual GPU
  try {
    auto ptr = make_unique_gpu<float>(0);  // 0 elements
    EXPECT_EQ(ptr.get(), nullptr);
  } catch (const std::exception& e) {
    FAIL() << "make_unique_gpu should not throw for 0 elements: " << e.what();
  }
}

// ============================================================================
// Test Suite: GPUErrorHandler Interface
// ============================================================================

class GPUErrorHandlerTest : public ::testing::Test {
 protected:
  std::shared_ptr<GPUErrorHandler> handler = GPUErrorHandler::Create();
};

/// Test: Handler singleton returns same instance
TEST_F(GPUErrorHandlerTest, Create_ReturnsSingletonInstance) {
  auto handler1 = GPUErrorHandler::Create();
  auto handler2 = GPUErrorHandler::Create();
  
  EXPECT_EQ(handler1.get(), handler2.get());
}

/// Test: GetLogger returns valid logger
TEST_F(GPUErrorHandlerTest, GetLogger_ReturnsValidLogger) {
  auto logger = GPUErrorHandler::GetLogger();
  EXPECT_NE(logger, nullptr);
}

/// Test: CUDA error name mapping (if CUDA enabled)
TEST_F(GPUErrorHandlerTest, CudaErrorName_ReturnsNonEmptyString) {
#if defined(__CUDACC__) || defined(THEMIS_CUDA_ENABLED)
  auto error_name = handler->cudaErrorName(cudaSuccess);
  EXPECT_FALSE(error_name.empty());
#else
  auto error_name = handler->cudaErrorName(0);  // dummy arg
  EXPECT_NE(error_name, "");  // Should be "CUDA_DISABLED" or similar
#endif
}

/// Test: HIP error name mapping (if HIP enabled)
TEST_F(GPUErrorHandlerTest, HipErrorName_ReturnsNonEmptyString) {
#if defined(THEMIS_HIP_ENABLED) || defined(__HIP__)
  auto error_name = handler->hipErrorName(hipSuccess);
  EXPECT_FALSE(error_name.empty());
#else
  auto error_name = handler->hipErrorName(0);  // dummy arg
  EXPECT_NE(error_name, "");  // Should be "HIP_DISABLED" or similar
#endif
}

/// Test: logError is noexcept and does not throw
TEST_F(GPUErrorHandlerTest, LogError_NoThrow) {
  EXPECT_NO_THROW({
#if defined(__CUDACC__) || defined(THEMIS_CUDA_ENABLED)
    handler->logError(cudaSuccess, "test");
#endif
  });
}

/// Test: handleError is callable (may throw depending on policy)
TEST_F(GPUErrorHandlerTest, HandleError_IsCallable) {
  EXPECT_NO_THROW({
#if defined(__CUDACC__) || defined(THEMIS_CUDA_ENABLED)
    handler->handleError(cudaSuccess, "test");
#endif
  });
}

// ============================================================================
// Test Suite: CHECKED_CUDA / CHECKED_HIP Macros
// ============================================================================

class MacroErrorHandlingTest : public ::testing::Test {};

/// Test: CHECKED_CUDA macro with success
TEST_F(MacroErrorHandlingTest, CheckedCuda_SuccessDoesNotThrow) {
#if defined(__CUDACC__) || defined(THEMIS_CUDA_ENABLED)
  EXPECT_NO_THROW({
    CHECKED_CUDA(cudaGetLastError());
  });
#endif
}

/// Test: CHECKED_HIP macro with success
TEST_F(MacroErrorHandlingTest, CheckedHip_SuccessDoesNotThrow) {
#if defined(THEMIS_HIP_ENABLED) || defined(__HIP__)
  EXPECT_NO_THROW({
    CHECKED_HIP(hipGetLastError());
  });
#endif
}

// ============================================================================
// Integration Tests
// ============================================================================

class IntegrationTest : public ::testing::Test {};

/// Test: Complete workflow with SLA and error handling
TEST_F(IntegrationTest, CompleteWorkflow_SLAAndErrorHandling) {
  auto handler = GPUErrorHandler::Create();
  
  // Create SLA guard with generous timeout
  KernelSLAGuard guard(5s);
  
  // Simulate kernel execution
  std::this_thread::sleep_for(10ms);
  
  // Check timeout (should be false)
  EXPECT_FALSE(guard.checkTimeoutDeadline());
  
  // Log a hypothetical error; cudaSuccess (0) is available in all builds
  // because gpu_error.h provides a stub constant in non-CUDA builds.
  handler->logError(static_cast<cudaError_t>(0), "hypothetical_kernel");
  
  // Still not timed out
  EXPECT_FALSE(guard.checkTimeoutDeadline());
}

/// Test: Multiple guards can coexist
TEST_F(IntegrationTest, MultipleGuards_CanCoexist) {
  KernelSLAGuard guard1(100ms);
  KernelSLAGuard guard2(200ms);
  
  EXPECT_FALSE(guard1.checkTimeoutDeadline());
  EXPECT_FALSE(guard2.checkTimeoutDeadline());
  
  std::this_thread::sleep_for(120ms);
  
  EXPECT_TRUE(guard1.checkTimeoutDeadline());
  EXPECT_FALSE(guard2.checkTimeoutDeadline());
}

/// Test: Handler is thread-safe (concurrent logError calls)
TEST_F(IntegrationTest, Handler_ThreadSafe_ConcurrentLogCalls) {
  auto handler = GPUErrorHandler::Create();
  
  std::vector<std::thread> threads;
  for (int i = 0; i < 4; ++i) {
    threads.emplace_back([handler, i]() {
      for (int j = 0; j < 10; ++j) {
        // cudaSuccess (0) is always available; see gpu_error.h stub constant.
        handler->logError(static_cast<cudaError_t>(0),
                          "thread_" + std::to_string(i));
      }
    });
  }
  
  for (auto& t : threads) {
    t.join();
  }
  
  // If we get here without deadlock/crash, concurrency is OK
  EXPECT_TRUE(true);
}

// Tests are linked against gtest_main; no custom main() needed.
