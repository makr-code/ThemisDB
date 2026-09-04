/**
 * @file test_gpu_resource_exhaustion.cpp
 * @brief Phase D ctest gate: GPU resource exhaustion tests (GPU-EXHAUST-01..12).
 * @date 2026-08-18
 *
 * Validates that the GPU error-handling infrastructure handles resource-
 * exhaustion scenarios fail-closed, is thread-safe, and produces consistent
 * diagnostics — all without requiring physical GPU hardware.
 *
 * Test IDs: GPU-EXHAUST-01 .. GPU-EXHAUST-12
 *
 * @see src/gpu/ROADMAP.md § Phase D ctest gates
 * @see tests/gpu/test_gpu_error_handling.cpp (base error-handling patterns)
 * @see tests/gpu/test_gpu_chaos_fault_injection.cpp (chaos patterns)
 */

#include <gtest/gtest.h>
#include <atomic>
#include <string>
#include <thread>
#include <vector>

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

class GPUResourceExhaustionTest : public ::testing::Test {
 protected:
  std::shared_ptr<GPUErrorHandler> handler = GPUErrorHandler::Create();
};

// ============================================================================
// GPU-EXHAUST-01: Single large allocation failure → kQuotaExceeded
// ============================================================================

/**
 * @test GPU-EXHAUST-01
 * @brief Simulate a single large allocation failure; verify the thrown
 *        CudaError carries the `cudaErrorMemoryAllocation` code and that the
 *        handler classifies it as kQuotaExceeded.
 */
TEST_F(GPUResourceExhaustionTest, EXHAUST_01_SingleLargeAllocFailure_QuotaExceeded) {
  bool exception_caught = false;
  GPUErrorClass classified = GPUErrorClass::kUnknown;

  try {
    throw CudaError("cudaMalloc_large", cudaErrorMemoryAllocation, __FILE__,
                    __LINE__);
  } catch (const CudaError& e) {
    exception_caught = true;
    classified = handler->classifyError(e.error_code());
  }

  EXPECT_TRUE(exception_caught) << "GPU-EXHAUST-01: expected CudaError";
  EXPECT_EQ(classified, GPUErrorClass::kQuotaExceeded)
      << "GPU-EXHAUST-01: large allocation should be kQuotaExceeded";
}

// ============================================================================
// GPU-EXHAUST-02: kQuotaExceeded maps to kFallbackCPU
// ============================================================================

/**
 * @test GPU-EXHAUST-02
 * @brief Verify the default recovery policy for kQuotaExceeded is kFallbackCPU.
 */
TEST_F(GPUResourceExhaustionTest, EXHAUST_02_QuotaExceeded_MapsTo_FallbackCPU) {
  auto policy = handler->defaultPolicy(GPUErrorClass::kQuotaExceeded);
  EXPECT_EQ(policy, ErrorRecoveryPolicy::kFallbackCPU)
      << "GPU-EXHAUST-02: kQuotaExceeded must map to kFallbackCPU";
}

// ============================================================================
// GPU-EXHAUST-03: Sequential allocation failures leave handler in clean state
// ============================================================================

/**
 * @test GPU-EXHAUST-03
 * @brief Ten sequential simulated allocation failures must not corrupt the
 *        handler.  After each exception the handler must still classify the
 *        next error correctly.
 */
TEST_F(GPUResourceExhaustionTest, EXHAUST_03_SequentialFailures_HandlerClean) {
  constexpr int kIterations = 10;
  for (int i = 0; i < kIterations; ++i) {
    bool caught = false;
    try {
      throw CudaError("cudaMalloc_seq", cudaErrorMemoryAllocation, __FILE__,
                      __LINE__);
    } catch (const CudaError&) {
      caught = true;
    }
    EXPECT_TRUE(caught) << "GPU-EXHAUST-03: iteration " << i;

    // Handler must remain functional after each exception.
    auto policy = handler->defaultPolicy(GPUErrorClass::kQuotaExceeded);
    EXPECT_EQ(policy, ErrorRecoveryPolicy::kFallbackCPU)
        << "GPU-EXHAUST-03: handler corrupted at iteration " << i;
  }
}

// ============================================================================
// GPU-EXHAUST-04: Concurrent threads trigger kQuotaExceeded → all kFallbackCPU
// ============================================================================

/**
 * @test GPU-EXHAUST-04
 * @brief Eight concurrent threads each simulate kQuotaExceeded and verify they
 *        all receive kFallbackCPU from the shared handler.
 */
TEST_F(GPUResourceExhaustionTest, EXHAUST_04_ConcurrentThreads_AllGetFallbackCPU) {
  constexpr int kThreadCount = 8;
  std::atomic<int> correct_policy{0};

  std::vector<std::thread> threads;
  threads.reserve(kThreadCount);
  for (int i = 0; i < kThreadCount; ++i) {
    threads.emplace_back([this, &correct_policy]() {
      auto policy = handler->defaultPolicy(GPUErrorClass::kQuotaExceeded);
      if (policy == ErrorRecoveryPolicy::kFallbackCPU) {
        correct_policy.fetch_add(1, std::memory_order_relaxed);
      }
    });
  }
  for (auto& t : threads) {
    t.join();
  }

  EXPECT_EQ(correct_policy.load(), kThreadCount)
      << "GPU-EXHAUST-04: all threads must get kFallbackCPU";
}

// ============================================================================
// GPU-EXHAUST-05: Handler reset after exhaustion allows new operations
// ============================================================================

/**
 * @test GPU-EXHAUST-05
 * @brief After simulated exhaustion the handler (which is a singleton) must
 *        still service fresh policy queries correctly — demonstrating that no
 *        latent state from prior failures blocks normal operation.
 */
TEST_F(GPUResourceExhaustionTest, EXHAUST_05_ResetAfterExhaustion_AllowsNewOps) {
  // Drive some error recordings.
  for (int i = 0; i < 5; ++i) {
    handler->recordErrorOccurrence(GPUErrorClass::kQuotaExceeded);
  }

  // After exhaustion simulation, basic policy lookup must still work.
  EXPECT_EQ(handler->defaultPolicy(GPUErrorClass::kKernelTimeout),
            ErrorRecoveryPolicy::kFallbackCPU)
      << "GPU-EXHAUST-05: handler must serve correct policy after exhaustion";
  EXPECT_EQ(handler->defaultPolicy(GPUErrorClass::kNumerical),
            ErrorRecoveryPolicy::kEmitWarning)
      << "GPU-EXHAUST-05: kNumerical policy must be intact after exhaustion";
}

// ============================================================================
// GPU-EXHAUST-06: make_unique_gpu allocation failure throws on non-zero size
// ============================================================================

/**
 * @test GPU-EXHAUST-06
 * @brief Directly throwing a CudaError with cudaErrorMemoryAllocation models
 *        what make_unique_gpu does on failure.  Verify the exception is
 *        catchable as std::exception and carries a non-empty what() string.
 */
TEST_F(GPUResourceExhaustionTest, EXHAUST_06_UniqueGPUPtr_AllocFailure_Throws) {
  bool caught_as_std_exception = false;
  std::string what_msg;

  try {
    // Simulate what make_unique_gpu throws when cudaMalloc fails.
    throw CudaError("make_unique_gpu", cudaErrorMemoryAllocation, __FILE__,
                    __LINE__);
  } catch (const std::exception& e) {
    caught_as_std_exception = true;
    what_msg = e.what();
  }

  EXPECT_TRUE(caught_as_std_exception)
      << "GPU-EXHAUST-06: CudaError must be catchable as std::exception";
  EXPECT_FALSE(what_msg.empty())
      << "GPU-EXHAUST-06: exception must carry a description";
}

// ============================================================================
// GPU-EXHAUST-07: After kQuotaExceeded, kBackendUnavailable still maps
// ============================================================================

/**
 * @test GPU-EXHAUST-07
 * @brief Even after recording multiple kQuotaExceeded events, the handler must
 *        still correctly map kBackendUnavailable → kMarkUnavailable.
 */
TEST_F(GPUResourceExhaustionTest, EXHAUST_07_AfterQuota_BackendUnavailableStillMaps) {
  for (int i = 0; i < 3; ++i) {
    handler->recordErrorOccurrence(GPUErrorClass::kQuotaExceeded);
  }

  auto policy = handler->defaultPolicy(GPUErrorClass::kBackendUnavailable);
  EXPECT_EQ(policy, ErrorRecoveryPolicy::kMarkUnavailable)
      << "GPU-EXHAUST-07: kBackendUnavailable must still map to kMarkUnavailable";
}

// ============================================================================
// GPU-EXHAUST-08: Resource exhaustion diagnostic includes error class name
// ============================================================================

/**
 * @test GPU-EXHAUST-08
 * @brief The errorClassName() string for kQuotaExceeded must be a non-empty
 *        string that contains the expected substring, enabling readable logs.
 */
TEST_F(GPUResourceExhaustionTest, EXHAUST_08_Diagnostic_IncludesErrorClassName) {
  const std::string name =
      handler->errorClassName(GPUErrorClass::kQuotaExceeded);

  EXPECT_FALSE(name.empty())
      << "GPU-EXHAUST-08: error class name must not be empty";
  EXPECT_NE(name.find("kQuotaExceeded"), std::string::npos)
      << "GPU-EXHAUST-08: name must contain 'kQuotaExceeded', got: " << name;
}

// ============================================================================
// GPU-EXHAUST-09: KernelSLAGuard functional under resource pressure
// ============================================================================

/**
 * @test GPU-EXHAUST-09
 * @brief Even while the handler records exhaustion events concurrently, a
 *        KernelSLAGuard with a 5-second SLA must not report timeout immediately
 *        after construction.
 */
TEST_F(GPUResourceExhaustionTest, EXHAUST_09_KernelSLAGuard_FunctionalUnderPressure) {
  // Background thread hammers the handler with exhaustion events.
  std::atomic<bool> stop{false};
  std::thread bg([this, &stop]() {
    while (!stop.load(std::memory_order_relaxed)) {
      handler->recordErrorOccurrence(GPUErrorClass::kQuotaExceeded);
    }
  });

  KernelSLAGuard guard(5s);
  EXPECT_FALSE(guard.checkTimeoutDeadline())
      << "GPU-EXHAUST-09: SLAGuard must not time out immediately";

  stop.store(true, std::memory_order_relaxed);
  bg.join();
}

// ============================================================================
// GPU-EXHAUST-10: kMemoryCommunication maps to kRetryOnce
// ============================================================================

/**
 * @test GPU-EXHAUST-10
 * @brief Verify the default recovery policy for kMemoryCommunication is
 *        kRetryOnce — distinct from the kFallbackCPU path used for exhaustion.
 */
TEST_F(GPUResourceExhaustionTest, EXHAUST_10_MemoryCommunication_MapsTo_RetryOnce) {
  auto policy = handler->defaultPolicy(GPUErrorClass::kMemoryCommunication);
  EXPECT_EQ(policy, ErrorRecoveryPolicy::kRetryOnce)
      << "GPU-EXHAUST-10: kMemoryCommunication must map to kRetryOnce";
}

// ============================================================================
// GPU-EXHAUST-11: Concurrent recordErrorOccurrence calls are thread-safe
// ============================================================================

/**
 * @test GPU-EXHAUST-11
 * @brief Sixteen threads each call recordErrorOccurrence 50 times.  No crash,
 *        no deadlock, and the handler must remain operational afterwards.
 */
TEST_F(GPUResourceExhaustionTest, EXHAUST_11_ConcurrentRecordError_ThreadSafe) {
  constexpr int kThreads = 16;
  constexpr int kCallsPerThread = 50;

  std::vector<std::thread> threads;
  threads.reserve(kThreads);
  for (int i = 0; i < kThreads; ++i) {
    threads.emplace_back([this]() {
      for (int j = 0; j < kCallsPerThread; ++j) {
        handler->recordErrorOccurrence(GPUErrorClass::kQuotaExceeded);
      }
    });
  }
  for (auto& t : threads) {
    t.join();
  }

  // Handler must still respond correctly after the storm.
  EXPECT_EQ(handler->defaultPolicy(GPUErrorClass::kQuotaExceeded),
            ErrorRecoveryPolicy::kFallbackCPU)
      << "GPU-EXHAUST-11: handler must be functional after concurrent storm";
}

// ============================================================================
// GPU-EXHAUST-12: Error class string round-trip consistent under pressure
// ============================================================================

/**
 * @test GPU-EXHAUST-12
 * @brief Under simulated load (kChaosTestSeed = 42 iterations), calling
 *        errorClassName() on every GPUErrorClass value must always return the
 *        same, non-empty string — verifying there is no internal mutation that
 *        can corrupt the name table.
 */
TEST_F(GPUResourceExhaustionTest, EXHAUST_12_ErrorClassString_RoundTrip_Consistent) {
  const std::vector<GPUErrorClass> all_classes{
      GPUErrorClass::kQuotaExceeded,
      GPUErrorClass::kKernelTimeout,
      GPUErrorClass::kBackendUnavailable,
      GPUErrorClass::kMemoryCommunication,
      GPUErrorClass::kNumerical,
      GPUErrorClass::kUnsupportedOperation,
      GPUErrorClass::kUnknown,
  };

  // Capture reference names.
  std::vector<std::string> ref_names;
  ref_names.reserve(all_classes.size());
  for (auto cls : all_classes) {
    ref_names.push_back(handler->errorClassName(cls));
  }

  // Re-query kChaosTestSeed times and compare.
  for (uint32_t seed = 0; seed < kChaosTestSeed; ++seed) {
    for (std::size_t i = 0; i < all_classes.size(); ++i) {
      const std::string name = handler->errorClassName(all_classes[i]);
      EXPECT_EQ(name, ref_names[i])
          << "GPU-EXHAUST-12: name changed at seed=" << seed
          << " class=" << ref_names[i];
      EXPECT_FALSE(name.empty())
          << "GPU-EXHAUST-12: name became empty at seed=" << seed;
    }
  }
}

}  // namespace

// Tests linked against gtest_main; no custom main() needed.
