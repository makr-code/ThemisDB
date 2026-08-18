/**
 * @file test_gpu_fallback_all_paths.cpp
 * @brief Phase D ctest gate: GPU fallback all-paths tests (GPU-FALLBACK-01..12).
 * @date 2026-08-18
 *
 * Exhaustively exercises every GPUErrorClass → ErrorRecoveryPolicy mapping
 * and the KernelSLAGuard timeout path.  All tests run without GPU hardware.
 *
 * Test IDs: GPU-FALLBACK-01 .. GPU-FALLBACK-12
 *
 * @see src/gpu/ROADMAP.md § Phase D ctest gates
 * @see tests/gpu/test_gpu_error_handling.cpp (base error-handling patterns)
 * @see tests/gpu/test_gpu_resource_exhaustion.cpp (exhaustion companion suite)
 */

#include <gtest/gtest.h>
#include <chrono>
#include <memory>
#include <thread>

#include "themis/gpu/gpu_error.h"
#include "themis/gpu/gpu_memory.h"
#include "themis/gpu/gpu_timeout.h"

using namespace themis::gpu;
using namespace std::chrono_literals;

namespace {

/// Determinism seed required by Wave A chaos conventions.
constexpr uint32_t kChaosTestSeed = 42;

// ============================================================================
// Test fixture
// ============================================================================

class GPUFallbackAllPathsTest : public ::testing::Test {
 protected:
  std::shared_ptr<GPUErrorHandler> handler = GPUErrorHandler::Create();
};

// ============================================================================
// GPU-FALLBACK-01: kQuotaExceeded → kFallbackCPU
// ============================================================================

/**
 * @test GPU-FALLBACK-01
 * @brief kQuotaExceeded must map to kFallbackCPU.
 */
TEST_F(GPUFallbackAllPathsTest, FALLBACK_01_QuotaExceeded_IsFallbackCPU) {
  EXPECT_EQ(handler->defaultPolicy(GPUErrorClass::kQuotaExceeded),
            ErrorRecoveryPolicy::kFallbackCPU)
      << "GPU-FALLBACK-01: kQuotaExceeded must map to kFallbackCPU";
}

// ============================================================================
// GPU-FALLBACK-02: kKernelTimeout → kFallbackCPU
// ============================================================================

/**
 * @test GPU-FALLBACK-02
 * @brief kKernelTimeout must map to kFallbackCPU.
 */
TEST_F(GPUFallbackAllPathsTest, FALLBACK_02_KernelTimeout_IsFallbackCPU) {
  EXPECT_EQ(handler->defaultPolicy(GPUErrorClass::kKernelTimeout),
            ErrorRecoveryPolicy::kFallbackCPU)
      << "GPU-FALLBACK-02: kKernelTimeout must map to kFallbackCPU";
}

// ============================================================================
// GPU-FALLBACK-03: kBackendUnavailable → kMarkUnavailable
// ============================================================================

/**
 * @test GPU-FALLBACK-03
 * @brief kBackendUnavailable must map to kMarkUnavailable.
 */
TEST_F(GPUFallbackAllPathsTest, FALLBACK_03_BackendUnavailable_IsMarkUnavailable) {
  EXPECT_EQ(handler->defaultPolicy(GPUErrorClass::kBackendUnavailable),
            ErrorRecoveryPolicy::kMarkUnavailable)
      << "GPU-FALLBACK-03: kBackendUnavailable must map to kMarkUnavailable";
}

// ============================================================================
// GPU-FALLBACK-04: kMemoryCommunication → kRetryOnce
// ============================================================================

/**
 * @test GPU-FALLBACK-04
 * @brief kMemoryCommunication must map to kRetryOnce.
 */
TEST_F(GPUFallbackAllPathsTest, FALLBACK_04_MemoryCommunication_IsRetryOnce) {
  EXPECT_EQ(handler->defaultPolicy(GPUErrorClass::kMemoryCommunication),
            ErrorRecoveryPolicy::kRetryOnce)
      << "GPU-FALLBACK-04: kMemoryCommunication must map to kRetryOnce";
}

// ============================================================================
// GPU-FALLBACK-05: kNumerical → kEmitWarning
// ============================================================================

/**
 * @test GPU-FALLBACK-05
 * @brief kNumerical must map to kEmitWarning.
 */
TEST_F(GPUFallbackAllPathsTest, FALLBACK_05_Numerical_IsEmitWarning) {
  EXPECT_EQ(handler->defaultPolicy(GPUErrorClass::kNumerical),
            ErrorRecoveryPolicy::kEmitWarning)
      << "GPU-FALLBACK-05: kNumerical must map to kEmitWarning";
}

// ============================================================================
// GPU-FALLBACK-06: kUnsupportedOperation → kFallbackCPU
// ============================================================================

/**
 * @test GPU-FALLBACK-06
 * @brief kUnsupportedOperation must map to kFallbackCPU.
 */
TEST_F(GPUFallbackAllPathsTest, FALLBACK_06_UnsupportedOperation_IsFallbackCPU) {
  EXPECT_EQ(handler->defaultPolicy(GPUErrorClass::kUnsupportedOperation),
            ErrorRecoveryPolicy::kFallbackCPU)
      << "GPU-FALLBACK-06: kUnsupportedOperation must map to kFallbackCPU";
}

// ============================================================================
// GPU-FALLBACK-07: kUnknown → deterministic policy (not undefined behaviour)
// ============================================================================

/**
 * @test GPU-FALLBACK-07
 * @brief defaultPolicy(kUnknown) must return *some* valid ErrorRecoveryPolicy
 *        value rather than invoking undefined behaviour or throwing unexpectedly.
 */
TEST_F(GPUFallbackAllPathsTest, FALLBACK_07_Unknown_IsDeterministicPolicy) {
  ErrorRecoveryPolicy policy = ErrorRecoveryPolicy::kFallbackCPU;
  EXPECT_NO_THROW({ policy = handler->defaultPolicy(GPUErrorClass::kUnknown); })
      << "GPU-FALLBACK-07: defaultPolicy(kUnknown) must not throw";

  // Policy must be one of the defined values (not garbage / uninitialized).
  const bool is_valid =
      (policy == ErrorRecoveryPolicy::kFallbackCPU) ||
      (policy == ErrorRecoveryPolicy::kRetryOnce) ||
      (policy == ErrorRecoveryPolicy::kMarkUnavailable) ||
      (policy == ErrorRecoveryPolicy::kEmitWarning);
  EXPECT_TRUE(is_valid)
      << "GPU-FALLBACK-07: kUnknown must map to a defined ErrorRecoveryPolicy";
}

// ============================================================================
// GPU-FALLBACK-08: KernelSLAGuard 5s timeout not tripped at t=0
// ============================================================================

/**
 * @test GPU-FALLBACK-08
 * @brief A freshly constructed KernelSLAGuard with the default 5-second SLA
 *        must not report timeout immediately.
 */
TEST_F(GPUFallbackAllPathsTest, FALLBACK_08_SLAGuard_Default5s_NotTrippedAtT0) {
  KernelSLAGuard guard;  // default SLA = 5s
  EXPECT_FALSE(guard.checkTimeoutDeadline())
      << "GPU-FALLBACK-08: 5s SLA guard must not timeout at construction";
  EXPECT_EQ(guard.getSLADuration(), 5s)
      << "GPU-FALLBACK-08: default SLA duration must be 5s";
}

// ============================================================================
// GPU-FALLBACK-09: KernelSLAGuard tripped after explicit sleep
// ============================================================================

/**
 * @test GPU-FALLBACK-09
 * @brief A KernelSLAGuard with a 100 ms SLA must report timeout after a 150 ms
 *        sleep, and must NOT report timeout before the deadline.
 */
TEST_F(GPUFallbackAllPathsTest, FALLBACK_09_SLAGuard_TrippedAfterSleep) {
  KernelSLAGuard guard(100ms);

  // Well within budget — must not trip.
  EXPECT_FALSE(guard.checkTimeoutDeadline())
      << "GPU-FALLBACK-09: guard must not timeout before sleep";

  std::this_thread::sleep_for(150ms);

  EXPECT_TRUE(guard.checkTimeoutDeadline())
      << "GPU-FALLBACK-09: guard must report timeout after 150ms sleep";
}

// ============================================================================
// GPU-FALLBACK-10: All kFallbackCPU paths set recovery_policy == kFallbackCPU
// ============================================================================

/**
 * @test GPU-FALLBACK-10
 * @brief Every error class that should map to kFallbackCPU does so.  The test
 *        collects all such classes and asserts the policy in one place.
 */
TEST_F(GPUFallbackAllPathsTest, FALLBACK_10_AllFallbackCPU_Classes_Confirmed) {
  const std::vector<GPUErrorClass> fallback_cpu_classes{
      GPUErrorClass::kQuotaExceeded,
      GPUErrorClass::kKernelTimeout,
      GPUErrorClass::kUnsupportedOperation,
  };

  for (auto cls : fallback_cpu_classes) {
    const auto policy = handler->defaultPolicy(cls);
    EXPECT_EQ(policy, ErrorRecoveryPolicy::kFallbackCPU)
        << "GPU-FALLBACK-10: class "
        << handler->errorClassName(cls)
        << " must map to kFallbackCPU";
  }
}

// ============================================================================
// GPU-FALLBACK-11: All kMarkUnavailable paths set correctly
// ============================================================================

/**
 * @test GPU-FALLBACK-11
 * @brief Every error class that should map to kMarkUnavailable does so.
 */
TEST_F(GPUFallbackAllPathsTest, FALLBACK_11_AllMarkUnavailable_Classes_Confirmed) {
  const std::vector<GPUErrorClass> mark_unavailable_classes{
      GPUErrorClass::kBackendUnavailable,
  };

  for (auto cls : mark_unavailable_classes) {
    const auto policy = handler->defaultPolicy(cls);
    EXPECT_EQ(policy, ErrorRecoveryPolicy::kMarkUnavailable)
        << "GPU-FALLBACK-11: class "
        << handler->errorClassName(cls)
        << " must map to kMarkUnavailable";
  }
}

// ============================================================================
// GPU-FALLBACK-12: unique_gpu_ptr null after failed init → no double-free
// ============================================================================

/**
 * @test GPU-FALLBACK-12
 * @brief Constructing a null unique_gpu_ptr, then moving and resetting it
 *        multiple times must not crash or cause a double-free.  Verifies RAII
 *        invariants hold when the allocation path is never taken.
 */
TEST_F(GPUFallbackAllPathsTest, FALLBACK_12_UniqueGPUPtr_NullAfterFailedInit_NoDoubleFree) {
  // Simulate failed init by constructing a null pointer.
  unique_gpu_ptr<float> ptr;
  EXPECT_EQ(ptr.get(), nullptr)
      << "GPU-FALLBACK-12: default-constructed ptr must be null";

  // Move it — source must stay null.
  unique_gpu_ptr<float> moved = std::move(ptr);
  EXPECT_EQ(ptr.get(), nullptr)
      << "GPU-FALLBACK-12: moved-from ptr must be null";
  EXPECT_EQ(moved.get(), nullptr)
      << "GPU-FALLBACK-12: moved-to ptr of a null source must also be null";

  // Reset to null — must not crash.
  EXPECT_NO_THROW(moved.reset(nullptr))
      << "GPU-FALLBACK-12: reset(nullptr) on null ptr must not throw";

  // Release — must return null without crashing.
  auto raw = moved.release();
  EXPECT_EQ(raw, nullptr)
      << "GPU-FALLBACK-12: release() on null ptr must return nullptr";

  // Destructor runs here — must not double-free.
}

}  // namespace

// Tests linked against gtest_main; no custom main() needed.
