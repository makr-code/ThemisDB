#include <gtest/gtest.h>
#include "server/adaptive_rate_limiter.h"
#include "server/cost_based_rate_limiter.h"

#include <chrono>
#include <limits>
#include <thread>

using namespace themis::server;

// ============================================================================
// AdaptiveRateLimiter – basic functionality
// ============================================================================

class AdaptiveRateLimiterTest : public ::testing::Test {
protected:
    AdaptiveRateLimiter::Config makeCfg(size_t base_cap = 100) {
        AdaptiveRateLimiter::Config cfg;
        cfg.base_capacity             = base_cap;
        cfg.high_latency_threshold_ms = 500;
        cfg.low_latency_threshold_ms  = 100;
        cfg.high_error_rate           = 0.05;
        cfg.low_error_rate            = 0.01;
        cfg.recovery_step             = 0.1;
        cfg.window_seconds            = 60;
        cfg.min_samples_to_adapt      = 10;
        return cfg;
    }
};

TEST_F(AdaptiveRateLimiterTest, InitialCapacityIsBaseCapacity) {
    AdaptiveRateLimiter limiter(makeCfg(100));
    EXPECT_EQ(limiter.getCurrentCapacity(""), 100u);
}

TEST_F(AdaptiveRateLimiterTest, AllowsRequestsUpToCapacity) {
    AdaptiveRateLimiter limiter(makeCfg(5));
    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(limiter.allowRequest(""))
            << "Request " << i << " should be allowed";
    }
    EXPECT_FALSE(limiter.allowRequest(""));
}

TEST_F(AdaptiveRateLimiterTest, MetricsTrackAllowedAndRejected) {
    AdaptiveRateLimiter limiter(makeCfg(3));
    limiter.allowRequest("");
    limiter.allowRequest("");
    limiter.allowRequest("");
    limiter.allowRequest(""); // rejected

    EXPECT_EQ(limiter.getTotalRequests(),   4u);
    EXPECT_EQ(limiter.getTotalRejections(), 1u);
}

TEST_F(AdaptiveRateLimiterTest, ResetRestoresCapacity) {
    AdaptiveRateLimiter limiter(makeCfg(3));
    for (int i = 0; i < 3; ++i) {
      limiter.allowRequest("");
    }
    EXPECT_FALSE(limiter.allowRequest(""));

    limiter.reset();
    EXPECT_EQ(limiter.getTotalRequests(),   0u);
    EXPECT_EQ(limiter.getTotalRejections(), 0u);
    EXPECT_TRUE(limiter.allowRequest(""));
}

TEST_F(AdaptiveRateLimiterTest, PerTenantIndependentBudgets) {
    AdaptiveRateLimiter limiter(makeCfg(2));

    // Exhaust tenant "a"
    EXPECT_TRUE(limiter.allowRequest("a"));
    EXPECT_TRUE(limiter.allowRequest("a"));
    EXPECT_FALSE(limiter.allowRequest("a"));

    // Tenant "b" unaffected
    EXPECT_TRUE(limiter.allowRequest("b"));
    EXPECT_TRUE(limiter.allowRequest("b"));
    EXPECT_FALSE(limiter.allowRequest("b"));
}

TEST_F(AdaptiveRateLimiterTest, HighErrorRateReducesCapacity) {
    AdaptiveRateLimiter::Config cfg = makeCfg(1000);
    cfg.min_samples_to_adapt = 10;
    cfg.high_error_rate      = 0.05; // 5 %
    AdaptiveRateLimiter limiter(cfg);

    // Inject 10 samples with 100 % error rate (well above 5 %).
    for (int i = 0; i < 10; ++i) {
        BackendHealthSample s;
        s.latency_ms = std::chrono::milliseconds{50};
        s.is_error   = true;
        limiter.recordSample("", s);
    }

    // Capacity should have been reduced to ≤ 20 % of 1000 = 200.
    EXPECT_LE(limiter.getCurrentCapacity(""), 200u);
}

TEST_F(AdaptiveRateLimiterTest, HighLatencyReducesCapacity) {
    AdaptiveRateLimiter::Config cfg = makeCfg(1000);
    cfg.min_samples_to_adapt        = 10;
    cfg.high_latency_threshold_ms   = 500;
    AdaptiveRateLimiter limiter(cfg);

    // Inject 10 samples all with p99 latency = 600 ms (above 500 ms threshold).
    for (int i = 0; i < 10; ++i) {
        BackendHealthSample s;
        s.latency_ms = std::chrono::milliseconds{600};
        s.is_error   = false;
        limiter.recordSample("", s);
    }

    // Capacity should be ≤ 50 % of 1000 = 500.
    EXPECT_LE(limiter.getCurrentCapacity(""), 500u);
}

TEST_F(AdaptiveRateLimiterTest, LowLatencyIncreasesCapacity) {
    AdaptiveRateLimiter::Config cfg = makeCfg(1000);
    cfg.min_samples_to_adapt      = 10;
    // With a sliding sample window, recovery is evaluated on mixed history.
    // Use thresholds that still degrade on all-error input but can recover
    // once healthy samples dominate the current window.
    cfg.high_error_rate           = 0.90;
    cfg.high_latency_threshold_ms = 500;
    cfg.low_latency_threshold_ms  = 100;
    cfg.low_error_rate            = 0.60;
    cfg.recovery_step             = 0.1;
    AdaptiveRateLimiter limiter(cfg);

    // First, push capacity down via high error rate.
    for (int i = 0; i < 10; ++i) {
        BackendHealthSample s;
        s.latency_ms = std::chrono::milliseconds{50};
        s.is_error   = true;
        limiter.recordSample("tenant", s);
    }
    const size_t degraded_cap = limiter.getCurrentCapacity("tenant");
    EXPECT_LT(degraded_cap, 1000u);

    // Now inject 10 healthy samples (low latency, no errors) to trigger recovery.
    for (int i = 0; i < 10; ++i) {
        BackendHealthSample s;
        s.latency_ms = std::chrono::milliseconds{20};
        s.is_error   = false;
        limiter.recordSample("tenant", s);
    }

    // Capacity should have recovered at least one step above degraded level.
    EXPECT_GT(limiter.getCurrentCapacity("tenant"), degraded_cap);
}

TEST_F(AdaptiveRateLimiterTest, RecoveryCapCappedAtBaseCapacity) {
    AdaptiveRateLimiter::Config cfg = makeCfg(100);
    cfg.min_samples_to_adapt  = 5;
    cfg.low_latency_threshold_ms = 100;
    cfg.low_error_rate        = 0.01;
    cfg.recovery_step         = 1.0; // 100 % per window — would overshoot without cap
    AdaptiveRateLimiter limiter(cfg);

    // Inject healthy samples to drive recovery.
    for (int i = 0; i < 5; ++i) {
        BackendHealthSample s;
        s.latency_ms = std::chrono::milliseconds{10};
        s.is_error   = false;
        limiter.recordSample("", s);
    }

    // Should not exceed base_capacity.
    EXPECT_LE(limiter.getCurrentCapacity(""), 100u);
}

TEST_F(AdaptiveRateLimiterTest, TokensReplenishAfterWindow) {
    AdaptiveRateLimiter::Config cfg;
    cfg.base_capacity        = 3;
    cfg.window_seconds       = 1; // short window for testing
    // Set well above any number of samples this test will inject so
    // capacity-adaptation logic stays dormant.
    cfg.min_samples_to_adapt = std::numeric_limits<size_t>::max();
    AdaptiveRateLimiter limiter(cfg);

    // Exhaust the tokens.
    EXPECT_TRUE(limiter.allowRequest(""));
    EXPECT_TRUE(limiter.allowRequest(""));
    EXPECT_TRUE(limiter.allowRequest(""));
    EXPECT_FALSE(limiter.allowRequest(""));

    // Poll until the window resets (up to 3 s).
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    bool reset_observed = false;
    while (std::chrono::steady_clock::now() < deadline) {
        if (limiter.allowRequest("")) {
            reset_observed = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    EXPECT_TRUE(reset_observed) << "Tokens did not replenish within 3 seconds";
}

// ============================================================================
// CostBasedRateLimiter – default operation costs
// ============================================================================

TEST(DefaultCostForTest, CorrectWeights) {
    EXPECT_EQ(defaultCostFor(OperationType::SIMPLE_GET),     1u);
    EXPECT_EQ(defaultCostFor(OperationType::COMPLEX_QUERY),  10u);
    EXPECT_EQ(defaultCostFor(OperationType::VECTOR_SEARCH),  20u);
    EXPECT_EQ(defaultCostFor(OperationType::LLM_COMPLETION), 100u);
    EXPECT_EQ(defaultCostFor(OperationType::CUSTOM),         1u);
}

// ============================================================================
// CostBasedRateLimiter – functionality
// ============================================================================

class CostBasedRateLimiterTest : public ::testing::Test {
protected:
    CostBasedRateLimiter::Config makeCfg(size_t budget = 100,
                                         uint32_t window_s = 60) {
        CostBasedRateLimiter::Config cfg;
        cfg.budget_per_window = budget;
        cfg.window_seconds    = window_s;
        cfg.max_clients       = 1000;
        return cfg;
    }
};

TEST_F(CostBasedRateLimiterTest, SimpleGetCostsOneUnit) {
    CostBasedRateLimiter limiter(makeCfg(5));
    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(limiter.allowRequest("c", OperationType::SIMPLE_GET))
            << "Request " << i << " should be allowed";
    }
    EXPECT_FALSE(limiter.allowRequest("c", OperationType::SIMPLE_GET));
    EXPECT_EQ(limiter.getRemainingBudget("c"), 0u);
}

TEST_F(CostBasedRateLimiterTest, ComplexQueryCostsTenUnits) {
    CostBasedRateLimiter limiter(makeCfg(100));
    // 10 complex queries = 100 units = full budget.
    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(limiter.allowRequest("c", OperationType::COMPLEX_QUERY));
    }
    EXPECT_FALSE(limiter.allowRequest("c", OperationType::COMPLEX_QUERY));
    EXPECT_EQ(limiter.getRemainingBudget("c"), 0u);
}

TEST_F(CostBasedRateLimiterTest, VectorSearchCostsTwentyUnits) {
    CostBasedRateLimiter limiter(makeCfg(100));
    // 5 vector searches = 100 units.
    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(limiter.allowRequest("c", OperationType::VECTOR_SEARCH));
    }
    EXPECT_FALSE(limiter.allowRequest("c", OperationType::VECTOR_SEARCH));
}

TEST_F(CostBasedRateLimiterTest, LlmCompletionCostsOneHundredUnits) {
    CostBasedRateLimiter limiter(makeCfg(100));
    EXPECT_TRUE(limiter.allowRequest("c", OperationType::LLM_COMPLETION));
    EXPECT_FALSE(limiter.allowRequest("c", OperationType::LLM_COMPLETION));
    EXPECT_EQ(limiter.getRemainingBudget("c"), 0u);
}

TEST_F(CostBasedRateLimiterTest, CustomCostConsumedCorrectly) {
    CostBasedRateLimiter limiter(makeCfg(50));
    EXPECT_TRUE(limiter.allowRequest("c", 30u));
    EXPECT_EQ(limiter.getRemainingBudget("c"), 20u);
    EXPECT_FALSE(limiter.allowRequest("c", 30u)); // only 20 left
    EXPECT_TRUE(limiter.allowRequest("c", 20u));   // exactly 20
    EXPECT_EQ(limiter.getRemainingBudget("c"), 0u);
}

TEST_F(CostBasedRateLimiterTest, IndependentBudgetsPerClient) {
    CostBasedRateLimiter limiter(makeCfg(10));
    limiter.allowRequest("a", OperationType::SIMPLE_GET); // uses 1 unit
    EXPECT_EQ(limiter.getRemainingBudget("a"), 9u);
    // Client "b" still has full budget.
    EXPECT_EQ(limiter.getRemainingBudget("b"), 10u);
}

TEST_F(CostBasedRateLimiterTest, MetricsAccumulate) {
    CostBasedRateLimiter limiter(makeCfg(10));
    limiter.allowRequest("c", 5u);
    limiter.allowRequest("c", 5u);
    limiter.allowRequest("c", 1u); // rejected

    EXPECT_EQ(limiter.getTotalRequests(),   3u);
    EXPECT_EQ(limiter.getTotalRejections(), 1u);
    EXPECT_EQ(limiter.getTotalCostConsumed(), 10u);
}

TEST_F(CostBasedRateLimiterTest, WindowResetRestoresBudget) {
    CostBasedRateLimiter::Config cfg;
    cfg.budget_per_window = 10;
    cfg.window_seconds    = 1; // 1-second window for fast test
    cfg.max_clients       = 10;
    CostBasedRateLimiter limiter(cfg);

    EXPECT_TRUE(limiter.allowRequest("c", 10u));
    EXPECT_FALSE(limiter.allowRequest("c", 1u));

    // Poll until the window resets (up to 3 s), avoiding hard timing dependency.
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    bool reset_observed = false;
    while (std::chrono::steady_clock::now() < deadline) {
        if (limiter.allowRequest("c", 10u)) {
            reset_observed = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    EXPECT_TRUE(reset_observed) << "Budget did not replenish within 3 seconds";
}

TEST_F(CostBasedRateLimiterTest, MaxClientsEnforced) {
    CostBasedRateLimiter::Config cfg = makeCfg(100);
    cfg.max_clients = 2;
    CostBasedRateLimiter limiter(cfg);

    EXPECT_TRUE(limiter.allowRequest("x", 1u));
    EXPECT_TRUE(limiter.allowRequest("y", 1u));
    EXPECT_EQ(limiter.getActiveClients(), 2u);
    // Third distinct client rejected.
    EXPECT_FALSE(limiter.allowRequest("z", 1u));
}

TEST_F(CostBasedRateLimiterTest, ResetClearsState) {
    CostBasedRateLimiter limiter(makeCfg(5));
    limiter.allowRequest("c", 5u);
    EXPECT_FALSE(limiter.allowRequest("c", 1u));

    limiter.reset();

    EXPECT_EQ(limiter.getTotalRequests(),    0u);
    EXPECT_EQ(limiter.getTotalRejections(),  0u);
    EXPECT_EQ(limiter.getTotalCostConsumed(), 0u);
    EXPECT_TRUE(limiter.allowRequest("c", 5u));
}

TEST_F(CostBasedRateLimiterTest, ActiveClientsCount) {
    CostBasedRateLimiter limiter(makeCfg(100));
    limiter.allowRequest("a", 1u);
    limiter.allowRequest("b", 1u);
    limiter.allowRequest("b", 1u);
    EXPECT_EQ(limiter.getActiveClients(), 2u);
}

TEST_F(CostBasedRateLimiterTest, RemainingBudgetUnknownClientIsFullBudget) {
    CostBasedRateLimiter limiter(makeCfg(100));
    EXPECT_EQ(limiter.getRemainingBudget("nobody"), 100u);
}

// ============================================================================
// Mixed operations – fairer resource allocation
// ============================================================================

TEST_F(CostBasedRateLimiterTest, MixedOperationsConsumeCorrectBudget) {
    // Budget = 200; verify that mixing ops deducts correctly.
    CostBasedRateLimiter limiter(makeCfg(200));

    EXPECT_TRUE(limiter.allowRequest("tenant", OperationType::LLM_COMPLETION)); // 100
    EXPECT_EQ(limiter.getRemainingBudget("tenant"), 100u);

    EXPECT_TRUE(limiter.allowRequest("tenant", OperationType::VECTOR_SEARCH));  // 20
    EXPECT_EQ(limiter.getRemainingBudget("tenant"), 80u);

    EXPECT_TRUE(limiter.allowRequest("tenant", OperationType::COMPLEX_QUERY));  // 10
    EXPECT_EQ(limiter.getRemainingBudget("tenant"), 70u);

    // 70 simple GETs should be allowed (1 unit each).
    for (int i = 0; i < 70; ++i) {
        EXPECT_TRUE(limiter.allowRequest("tenant", OperationType::SIMPLE_GET));
    }
    EXPECT_EQ(limiter.getRemainingBudget("tenant"), 0u);
    EXPECT_FALSE(limiter.allowRequest("tenant", OperationType::SIMPLE_GET));
}
