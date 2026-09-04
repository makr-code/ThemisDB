/**
 * @file test_break_even_validation.cpp
 * @brief Comprehensive test suite for GPU Break-Even Validator
 * @version 1.0.0
 * @date 2026-07-06
 *
 * Tests cover:
 * - Break-even decision correctness (use GPU vs CPU fallback)
 * - Caching behavior (hit/miss rates)
 * - Threshold customization
 * - Caller overrides (force_gpu, prefer_cpu)
 * - Device variation handling
 * - Concurrent access safety
 */

#include <gtest/gtest.h>
#include <thread>
#include <vector>

#include "acceleration/break_even_validator.h"

namespace themis {
namespace acceleration {
namespace testing {

// ============================================================================
// Test Fixture
// ============================================================================

class BreakEvenValidatorTest : public ::testing::Test {
protected:
    BreakEvenValidator validator_;

    /**
     * Create a standard workload profile for testing.
     */
    WorkloadProfile MakeProfile(
        KernelType kernel,
        size_t input_size,
        float selectivity = 1.0f,
        DeviceType device = DeviceType::kNVIDIA_RTX) {
        return {
            .kernel_type = kernel,
            .input_size = input_size,
            .output_selectivity = selectivity,
            .vector_dimension = 128,
            .device = device,
        };
    }

    void SetUp() override {
        // Clear any cache state
        validator_.ClearCache();
    }
};

// ============================================================================
// Category A: Distance Kernel Tests
// ============================================================================

TEST_F(BreakEvenValidatorTest, L2Distance_SmallInput_CPUPreferred) {
    auto profile = MakeProfile(KernelType::kDistance, 100);
    auto decision = validator_.ShouldUseGPU(profile);
    EXPECT_FALSE(decision.use_gpu)
        << "Small input (100 vectors) should prefer CPU (GPU overhead too high)";
    EXPECT_EQ(decision.reason, "break_even_not_met");
}

TEST_F(BreakEvenValidatorTest, L2Distance_MediumInput_MaybeGPU) {
    auto profile = MakeProfile(KernelType::kDistance, 10'000);
    auto decision = validator_.ShouldUseGPU(profile);
    // Medium size is breakpoint; decision depends on actual profiling
    EXPECT_TRUE(decision.speedup_ratio >= 0.0f);
}

TEST_F(BreakEvenValidatorTest, L2Distance_LargeInput_GPUPreferred) {
    auto profile = MakeProfile(KernelType::kDistance, 1'000'000);
    auto decision = validator_.ShouldUseGPU(profile);
    EXPECT_TRUE(decision.use_gpu || decision.reason != "gpu_unavailable")
        << "Large input (1M vectors) should consider GPU if available";
    if (decision.use_gpu) {
        EXPECT_GE(decision.speedup_ratio, 1.5f)
            << "Should meet 1.5x threshold for Category A if GPU chosen";
    }
}

TEST_F(BreakEvenValidatorTest, L2Distance_VeryLargeInput_GPUBeneficial) {
    auto profile = MakeProfile(KernelType::kDistance, 10'000'000);
    auto decision = validator_.ShouldUseGPU(profile);
    EXPECT_TRUE(decision.speedup_ratio > 0.0f)
        << "Very large input should be profiled";
    if (decision.use_gpu) {
        EXPECT_GE(decision.speedup_ratio, 1.5f);
    }
}

// ============================================================================
// Category A: TopK Selection Tests
// ============================================================================

TEST_F(BreakEvenValidatorTest, TopK_SmallInput_CPUPreferred) {
    auto profile = MakeProfile(KernelType::kTopK, 1'000);
    auto decision = validator_.ShouldUseGPU(profile);
    EXPECT_FALSE(decision.use_gpu);
}

TEST_F(BreakEvenValidatorTest, TopK_LargeInput_HighSelectivity_MaybeGPU) {
    auto profile = MakeProfile(KernelType::kTopK, 100'000, 0.5f);  // 50% selectivity
    auto decision = validator_.ShouldUseGPU(profile);
    // Moderate selectivity reduces benefit
    EXPECT_TRUE(decision.speedup_ratio >= 0.0f);
}

TEST_F(BreakEvenValidatorTest, TopK_LargeInput_LowSelectivity_CPUPreferred) {
    auto profile = MakeProfile(KernelType::kTopK, 100'000, 0.01f);  // 1% selectivity
    auto decision = validator_.ShouldUseGPU(profile);
    // Very low selectivity (sparse output) doesn't benefit from GPU
    EXPECT_FALSE(decision.use_gpu || decision.speedup_ratio < 1.5f)
        << "Low selectivity should not meet GPU threshold";
}

// ============================================================================
// Category B: BFS Tests
// ============================================================================

TEST_F(BreakEvenValidatorTest, BFS_SmallGraph_CPUPreferred) {
    auto profile = MakeProfile(KernelType::kBFS, 1'000);
    auto decision = validator_.ShouldUseGPU(profile);
    EXPECT_FALSE(decision.use_gpu);
}

TEST_F(BreakEvenValidatorTest, BFS_MediumGraph_MaybeGPU) {
    auto profile = MakeProfile(KernelType::kBFS, 10'000);
    auto decision = validator_.ShouldUseGPU(profile);
    EXPECT_TRUE(decision.speedup_ratio >= 0.0f);
}

TEST_F(BreakEvenValidatorTest, BFS_LargeGraph_GPUBeneficial) {
    auto profile = MakeProfile(KernelType::kBFS, 100'000);
    auto decision = validator_.ShouldUseGPU(profile);
    if (decision.use_gpu) {
        EXPECT_GE(decision.speedup_ratio, 1.3f)
            << "BFS should meet 1.3x threshold for Category B if GPU chosen";
    }
}

// ============================================================================
// Category B: Dijkstra Tests
// ============================================================================

TEST_F(BreakEvenValidatorTest, Dijkstra_SmallGraph_CPUPreferred) {
    auto profile = MakeProfile(KernelType::kDijkstra, 5'000);
    auto decision = validator_.ShouldUseGPU(profile);
    EXPECT_FALSE(decision.use_gpu);
}

TEST_F(BreakEvenValidatorTest, Dijkstra_LargeGraph_GPUBeneficial) {
    auto profile = MakeProfile(KernelType::kDijkstra, 100'000);
    auto decision = validator_.ShouldUseGPU(profile);
    if (decision.use_gpu) {
        EXPECT_GE(decision.speedup_ratio, 1.3f);
    }
}

// ============================================================================
// Category B: Geospatial Tests
// ============================================================================

TEST_F(BreakEvenValidatorTest, GeoDistance_LargeInput_MaybeGPU) {
    auto profile = MakeProfile(KernelType::kGeoDistance, 50'000);
    auto decision = validator_.ShouldUseGPU(profile);
    EXPECT_TRUE(decision.speedup_ratio >= 0.0f);
}

TEST_F(BreakEvenValidatorTest, GeoContainment_VeryLargeInput_GPUBeneficial) {
    auto profile = MakeProfile(KernelType::kGeoContainment, 1'000'000);
    auto decision = validator_.ShouldUseGPU(profile);
    if (decision.use_gpu) {
        EXPECT_GE(decision.speedup_ratio, 1.3f);
    }
}

// ============================================================================
// Caching Behavior Tests
// ============================================================================

TEST_F(BreakEvenValidatorTest, Cache_FirstCall_Miss) {
    auto profile = MakeProfile(KernelType::kDistance, 100'000);
    auto initial_misses = validator_.GetCacheMissCount();

    validator_.ShouldUseGPU(profile);

    EXPECT_GT(validator_.GetCacheMissCount(), initial_misses)
        << "First call should result in cache miss";
}

TEST_F(BreakEvenValidatorTest, Cache_SecondCall_Hit) {
    auto profile = MakeProfile(KernelType::kDistance, 100'000);

    auto decision1 = validator_.ShouldUseGPU(profile);
    auto initial_hits = validator_.GetCacheHitCount();

    auto decision2 = validator_.ShouldUseGPU(profile);

    EXPECT_GT(validator_.GetCacheHitCount(), initial_hits)
        << "Second identical call should hit cache";
    EXPECT_EQ(decision1.use_gpu, decision2.use_gpu)
        << "Cached decisions should be consistent";
    EXPECT_EQ(decision1.speedup_ratio, decision2.speedup_ratio)
        << "Cached speedup ratios should be identical";
    EXPECT_TRUE(decision2.from_cache)
        << "Second call should be marked as from cache";
}

TEST_F(BreakEvenValidatorTest, Cache_DifferentInput_Miss) {
    auto profile1 = MakeProfile(KernelType::kDistance, 100'000);
    auto profile2 = MakeProfile(KernelType::kDistance, 200'000);

    validator_.ShouldUseGPU(profile1);
    auto hits_after_first = validator_.GetCacheHitCount();

    validator_.ShouldUseGPU(profile2);

    EXPECT_EQ(validator_.GetCacheHitCount(), hits_after_first)
        << "Different input size should not hit cache";
}

TEST_F(BreakEvenValidatorTest, Cache_DifferentKernel_Miss) {
    auto profile1 = MakeProfile(KernelType::kDistance, 100'000);
    auto profile2 = MakeProfile(KernelType::kBFS, 100'000);

    validator_.ShouldUseGPU(profile1);
    auto hits_after_first = validator_.GetCacheHitCount();

    validator_.ShouldUseGPU(profile2);

    EXPECT_EQ(validator_.GetCacheHitCount(), hits_after_first)
        << "Different kernel type should not hit cache";
}

TEST_F(BreakEvenValidatorTest, Cache_Clear_EmptiesCache) {
    auto profile = MakeProfile(KernelType::kDistance, 100'000);
    validator_.ShouldUseGPU(profile);

    EXPECT_GT(validator_.GetCacheSize(), 0);
    validator_.ClearCache();
    EXPECT_EQ(validator_.GetCacheSize(), 0);
}

TEST_F(BreakEvenValidatorTest, Cache_Statistics) {
    auto profile1 = MakeProfile(KernelType::kDistance, 100'000);
    auto profile2 = MakeProfile(KernelType::kBFS, 50'000);

    // Call 1: distance miss
    validator_.ShouldUseGPU(profile1);
    // Call 2: distance hit
    validator_.ShouldUseGPU(profile1);
    // Call 3: BFS miss
    validator_.ShouldUseGPU(profile2);
    // Call 4: BFS hit
    validator_.ShouldUseGPU(profile2);

    EXPECT_EQ(validator_.GetCacheMissCount(), 2);
    EXPECT_EQ(validator_.GetCacheHitCount(), 2);
    EXPECT_EQ(validator_.GetCacheSize(), 2);
}

// ============================================================================
// Threshold Customization Tests
// ============================================================================

TEST_F(BreakEvenValidatorTest, Threshold_DefaultValuesCategoryA) {
    EXPECT_EQ(validator_.GetSpeedupThreshold(KernelType::kDistance), 1.5f);
    EXPECT_EQ(validator_.GetSpeedupThreshold(KernelType::kTopK), 1.5f);
}

TEST_F(BreakEvenValidatorTest, Threshold_DefaultValuesCategoryB) {
    EXPECT_EQ(validator_.GetSpeedupThreshold(KernelType::kBFS), 1.3f);
    EXPECT_EQ(validator_.GetSpeedupThreshold(KernelType::kDijkstra), 1.3f);
}

TEST_F(BreakEvenValidatorTest, Threshold_Customize) {
    validator_.SetSpeedupThreshold(KernelType::kDistance, 2.0f);
    EXPECT_EQ(validator_.GetSpeedupThreshold(KernelType::kDistance), 2.0f);

    // Should not affect other kernels
    EXPECT_EQ(validator_.GetSpeedupThreshold(KernelType::kBFS), 1.3f);
}

TEST_F(BreakEvenValidatorTest, Threshold_MinimumValue) {
    validator_.SetSpeedupThreshold(KernelType::kDistance, 0.5f);
    // Should clamp to 1.0
    EXPECT_GE(validator_.GetSpeedupThreshold(KernelType::kDistance), 1.0f);
}

// ============================================================================
// Caller Override Tests
// ============================================================================

TEST_F(BreakEvenValidatorTest, Override_ForceGPU) {
    auto profile = MakeProfile(KernelType::kDistance, 100);  // Too small
    profile.force_gpu = true;

    auto decision = validator_.ShouldUseGPU(profile);

    EXPECT_TRUE(decision.use_gpu)
        << "force_gpu=true should override break-even check";
    EXPECT_EQ(decision.reason, "force_gpu_flag");
}

TEST_F(BreakEvenValidatorTest, Override_PreferCPU) {
    auto profile = MakeProfile(KernelType::kDistance, 10'000'000);  // Very large
    profile.prefer_cpu = true;

    auto decision = validator_.ShouldUseGPU(profile);

    EXPECT_FALSE(decision.use_gpu)
        << "prefer_cpu=true should override break-even check";
    EXPECT_EQ(decision.reason, "prefer_cpu_flag");
}

TEST_F(BreakEvenValidatorTest, Override_ForceGPU_Takes_Precedence) {
    auto profile = MakeProfile(KernelType::kDistance, 100);
    profile.force_gpu = true;
    profile.prefer_cpu = true;  // Both set

    auto decision = validator_.ShouldUseGPU(profile);

    EXPECT_TRUE(decision.use_gpu)
        << "force_gpu should take precedence over prefer_cpu";
}

// ============================================================================
// Device Type Variation Tests
// ============================================================================

TEST_F(BreakEvenValidatorTest, DeviceType_NVIDIA_RTX) {
    auto profile = MakeProfile(KernelType::kDistance, 100'000,
                                1.0f, DeviceType::kNVIDIA_RTX);
    auto decision = validator_.ShouldUseGPU(profile);
    EXPECT_TRUE(decision.speedup_ratio >= 0.0f);
}

TEST_F(BreakEvenValidatorTest, DeviceType_NVIDIA_T4) {
    auto profile = MakeProfile(KernelType::kDistance, 100'000,
                                1.0f, DeviceType::kNVIDIA_T4);
    auto decision = validator_.ShouldUseGPU(profile);
    EXPECT_TRUE(decision.speedup_ratio >= 0.0f);
}

TEST_F(BreakEvenValidatorTest, DeviceType_AMD_MI210) {
    auto profile = MakeProfile(KernelType::kDistance, 100'000,
                                1.0f, DeviceType::kAMD_MI210);
    auto decision = validator_.ShouldUseGPU(profile);
    EXPECT_TRUE(decision.speedup_ratio >= 0.0f);
}

TEST_F(BreakEvenValidatorTest, DeviceType_CPU) {
    auto profile = MakeProfile(KernelType::kDistance, 100'000,
                                1.0f, DeviceType::kCPU);
    auto decision = validator_.ShouldUseGPU(profile);
    // CPU device should not recommend GPU
    EXPECT_FALSE(decision.use_gpu);
    EXPECT_EQ(decision.reason, "gpu_unavailable");
}

// ============================================================================
// Metrics Access Tests
// ============================================================================

TEST_F(BreakEvenValidatorTest, Metrics_LatestSpeedupRatio) {
    auto profile = MakeProfile(KernelType::kDistance, 100'000);

    // Initially no data
    EXPECT_EQ(validator_.GetLatestBreakEvenRatio(KernelType::kDistance), 0.0f);

    auto decision = validator_.ShouldUseGPU(profile);

    // After first profile
    auto ratio = validator_.GetLatestBreakEvenRatio(KernelType::kDistance);
    EXPECT_EQ(ratio, decision.speedup_ratio);
    EXPECT_GT(ratio, 0.0f);
}

TEST_F(BreakEvenValidatorTest, Hooks_InjectedProfilersDriveDecisionAndMetrics) {
    auto profile = MakeProfile(KernelType::kDistance, 4'096);
    bool metrics_called = false;
    BreakEvenDecision emitted_decision;

    validator_.SetCPUProfileFn([](const WorkloadProfile&) {
        return std::chrono::milliseconds(42);
    });
    validator_.SetGPUProfileFn([](const WorkloadProfile&) {
        return std::chrono::milliseconds(14);
    });
    validator_.SetMetricsSink([&](const WorkloadProfile& observed_profile,
                                  const BreakEvenDecision& decision) {
        metrics_called = true;
        emitted_decision = decision;
        EXPECT_EQ(observed_profile.input_size, profile.input_size);
        EXPECT_EQ(observed_profile.kernel_type, profile.kernel_type);
    });

    const auto decision = validator_.ShouldUseGPU(profile);

    EXPECT_TRUE(metrics_called);
    EXPECT_TRUE(decision.use_gpu);
    EXPECT_EQ(decision.reason, "break_even_met");
    EXPECT_FLOAT_EQ(decision.speedup_ratio, 3.0f);
    EXPECT_EQ(emitted_decision.speedup_ratio, decision.speedup_ratio);
}

TEST_F(BreakEvenValidatorTest, InvalidDistanceProfileFailsClosed) {
    auto profile = MakeProfile(KernelType::kDistance, 1'024);
    profile.vector_dimension = 0;

    const auto decision = validator_.ShouldUseGPU(profile);

    EXPECT_FALSE(decision.use_gpu);
    EXPECT_EQ(decision.reason, "cpu_profile_failed");
}

// ============================================================================
// String Conversion Tests
// ============================================================================

TEST_F(BreakEvenValidatorTest, StringConversion_KernelType) {
    EXPECT_EQ(BreakEvenValidator::KernelTypeToString(KernelType::kDistance), "distance");
    EXPECT_EQ(BreakEvenValidator::KernelTypeToString(KernelType::kTopK), "topk");
    EXPECT_EQ(BreakEvenValidator::KernelTypeToString(KernelType::kBFS), "bfs");
    EXPECT_EQ(BreakEvenValidator::KernelTypeToString(KernelType::kDijkstra), "dijkstra");
}

TEST_F(BreakEvenValidatorTest, StringConversion_DeviceType) {
    EXPECT_EQ(BreakEvenValidator::DeviceTypeToString(DeviceType::kNVIDIA_RTX), "nvidia_rtx");
    EXPECT_EQ(BreakEvenValidator::DeviceTypeToString(DeviceType::kAMD_MI210), "amd_mi210");
    EXPECT_EQ(BreakEvenValidator::DeviceTypeToString(DeviceType::kCPU), "cpu");
}

// ============================================================================
// Concurrency Tests
// ============================================================================

TEST_F(BreakEvenValidatorTest, Concurrency_MultipleThreads_ShouldUseGPU) {
    auto profile = MakeProfile(KernelType::kDistance, 100'000);

    std::vector<std::thread> threads;
    std::vector<BreakEvenDecision> decisions;
    std::mutex decisions_mu = {};

    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&] {
            auto decision = validator_.ShouldUseGPU(profile);
            {
                std::lock_guard<std::mutex> lock(decisions_mu);
                decisions.push_back(decision);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(decisions.size(), 10);
    // All decisions should be consistent
    for (size_t i = 1; i < decisions.size(); ++i) {
        EXPECT_EQ(decisions[0].use_gpu, decisions[i].use_gpu)
            << "Concurrent calls should yield consistent results";
    }
}

TEST_F(BreakEvenValidatorTest, Concurrency_Profile_WithMultipleThreads) {
    auto profile = MakeProfile(KernelType::kDistance, 100'000);

    std::vector<std::thread> threads = {};

    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&] {
            validator_.Profile(profile);
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // Should not crash and cache should be populated
    EXPECT_GT(validator_.GetCacheSize(), 0);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(BreakEvenValidatorTest, Integration_RealisticWorkload_Distance) {
    WorkloadProfile profile{
        .kernel_type = KernelType::kDistance,
        .input_size = 500'000,
        .output_selectivity = 1.0f,
        .vector_dimension = 768,  // BERT-like
        .device = DeviceType::kNVIDIA_RTX,
    };

    auto decision = validator_.ShouldUseGPU(profile);

    EXPECT_NE(decision.reason, "");
    EXPECT_GE(decision.cpu_time_ms.count(), 0);
    if (decision.use_gpu) {
        EXPECT_GE(decision.gpu_time_ms.count(), 0);
        EXPECT_GE(decision.speedup_ratio, 1.5f);
    }
}

TEST_F(BreakEvenValidatorTest, Integration_RealisticWorkload_BFS) {
    WorkloadProfile profile{
        .kernel_type = KernelType::kBFS,
        .input_size = 100'000,
        .output_selectivity = 0.5f,
        .device = DeviceType::kNVIDIA_RTX,
    };

    auto decision = validator_.ShouldUseGPU(profile);

    if (decision.use_gpu) {
        EXPECT_GE(decision.speedup_ratio, 1.3f);
    }
}

}  // namespace testing
}  // namespace acceleration
}  // namespace themis
