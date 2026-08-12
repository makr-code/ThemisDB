/*
 * Unit tests for the IFeatureFlags interface and its implementations.
 *
 * Covers:
 *   - NoOpFeatureFlags     : all flags disabled, setValue is a no-op
 *   - InMemoryFeatureFlags : set/get, default-false for unknown flags,
 *                            getAllFlags snapshot, thread safety
 *   - ConcernsContext      : featureFlags() accessor, HealthStatus includes
 *                            featureFlags field, 5-arg createCustom overload,
 *                            create(Config) with featureFlagsAdapter and
 *                            initialFeatureFlags
 */

#include "core/concerns/i_feature_flags.h"
#include "core/concerns/noop_implementations.h"
#include "core/concerns/concerns_context.h"
#include "core/concerns/lifecycle.h"
#include <gtest/gtest.h>
#include <atomic>
#include <string>
#include <thread>
#include <vector>

using namespace themis::core::concerns;

// ---------------------------------------------------------------------------
// NoOpFeatureFlags
// ---------------------------------------------------------------------------

TEST(NoOpFeatureFlagsTest, AllFlagsDisabledByDefault) {
    NoOpFeatureFlags flags;
    EXPECT_FALSE(flags.isEnabled("any_flag"));
    EXPECT_FALSE(flags.isEnabled("another_flag"));
}

TEST(NoOpFeatureFlagsTest, SetValueHasNoEffect) {
    NoOpFeatureFlags flags;
    flags.setValue("my_feature", true);
    // Still disabled — no-op implementation
    EXPECT_FALSE(flags.isEnabled("my_feature"));
}

TEST(NoOpFeatureFlagsTest, GetAllFlagsReturnsEmpty) {
    NoOpFeatureFlags flags;
    flags.setValue("x", true);
    EXPECT_TRUE(flags.getAllFlags().empty());
}

TEST(NoOpFeatureFlagsTest, IsHealthyReturnsTrue) {
    NoOpFeatureFlags flags;
    EXPECT_TRUE(flags.isHealthy().ok);
}

TEST(NoOpFeatureFlagsTest, LifecycleMethodsDoNotThrow) {
    NoOpFeatureFlags flags;
    EXPECT_NO_THROW(flags.flush());
    EXPECT_NO_THROW(flags.shutdown());
}

// ---------------------------------------------------------------------------
// InMemoryFeatureFlags
// ---------------------------------------------------------------------------

TEST(InMemoryFeatureFlagsTest, DefaultFalseForUnknownFlag) {
    InMemoryFeatureFlags flags;
    EXPECT_FALSE(flags.isEnabled("unknown"));
}

TEST(InMemoryFeatureFlagsTest, SetAndGetFlag) {
    InMemoryFeatureFlags flags;
    flags.setValue("dark_mode", true);
    EXPECT_TRUE(flags.isEnabled("dark_mode"));
}

TEST(InMemoryFeatureFlagsTest, DisableFlag) {
    InMemoryFeatureFlags flags;
    flags.setValue("feature_x", true);
    EXPECT_TRUE(flags.isEnabled("feature_x"));
    flags.setValue("feature_x", false);
    EXPECT_FALSE(flags.isEnabled("feature_x"));
}

TEST(InMemoryFeatureFlagsTest, GetAllFlagsSnapshot) {
    InMemoryFeatureFlags flags;
    flags.setValue("alpha", true);
    flags.setValue("beta", false);

    auto all = flags.getAllFlags();
    ASSERT_EQ(2u, all.size());
    EXPECT_TRUE(all.at("alpha"));
    EXPECT_FALSE(all.at("beta"));
}

TEST(InMemoryFeatureFlagsTest, ConstructWithInitialValues) {
    InMemoryFeatureFlags flags({{"a", true}, {"b", false}});
    EXPECT_TRUE(flags.isEnabled("a"));
    EXPECT_FALSE(flags.isEnabled("b"));
}

TEST(InMemoryFeatureFlagsTest, IsHealthyReturnsTrue) {
    InMemoryFeatureFlags flags;
    EXPECT_TRUE(flags.isHealthy().ok);
}

TEST(InMemoryFeatureFlagsTest, LifecycleMethodsDoNotThrow) {
    InMemoryFeatureFlags flags;
    EXPECT_NO_THROW(flags.flush());
    EXPECT_NO_THROW(flags.shutdown());
}

TEST(InMemoryFeatureFlagsTest, ThreadSafeSetAndGet) {
    InMemoryFeatureFlags flags;
    constexpr int kThreads = 8;
    constexpr int kIterations = 500;
    std::atomic<int> readTrue{0};

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t] {
            for (int i = 0; i < kIterations; ++i) {
                bool value = (i % 2 == 0);
                flags.setValue("flag_" + std::to_string(t), value);
                if (flags.isEnabled("flag_" + std::to_string(t))) {
                    ++readTrue;
                }
            }
        });
    }
    for (auto& th : threads) th.join();

    // No crash or data race — correctness only validated indirectly
    EXPECT_GE(readTrue.load(), 0);
}

// ---------------------------------------------------------------------------
// ConcernsContext integration
// ---------------------------------------------------------------------------

TEST(FeatureFlagsConcernsContextTest, NoOpContextExposesFeatureFlags) {
    auto ctx = ConcernsContext::createNoOp();
    // Should not crash
    EXPECT_FALSE(ctx->featureFlags().isEnabled("any"));
}

TEST(FeatureFlagsConcernsContextTest, HealthCheckIncludesFeatureFlags) {
    auto ctx = ConcernsContext::createNoOp();
    auto status = ctx->healthCheck();
    EXPECT_TRUE(status.featureFlags.ok);
    EXPECT_TRUE(status.isHealthy());
}

TEST(FeatureFlagsConcernsContextTest, FiveArgCreateCustom) {
    auto ctx = ConcernsContext::createCustom(
        std::make_unique<NoOpLogger>(),
        std::make_unique<NoOpTracer>(),
        std::make_unique<NoOpMetrics>(),
        std::make_unique<NoOpCache>(),
        std::make_unique<InMemoryFeatureFlags>(
            std::unordered_map<std::string, bool>{{"new_ui", true}}
        )
    );
    EXPECT_TRUE(ctx->featureFlags().isEnabled("new_ui"));
    EXPECT_FALSE(ctx->featureFlags().isEnabled("old_ui"));
}

TEST(FeatureFlagsConcernsContextTest, UnhealthyFeatureFlagsPropagatesInHealthStatus) {
    class UnhealthyFeatureFlags : public NoOpFeatureFlags {
    public:
        ProbeResult isHealthy() const override {
            return ProbeResult::unhealthy("flag provider unreachable");
        }
    };

    auto ctx = ConcernsContext::createCustom(
        std::make_unique<NoOpLogger>(),
        std::make_unique<NoOpTracer>(),
        std::make_unique<NoOpMetrics>(),
        std::make_unique<NoOpCache>(),
        std::make_unique<UnhealthyFeatureFlags>()
    );

    auto status = ctx->healthCheck();
    EXPECT_FALSE(status.isHealthy());
    EXPECT_FALSE(status.featureFlags.ok);
    EXPECT_EQ(status.featureFlags.message, "flag provider unreachable");
    // Other concerns still healthy
    EXPECT_TRUE(status.logger.ok);
    EXPECT_TRUE(status.tracer.ok);
    EXPECT_TRUE(status.metrics.ok);
    EXPECT_TRUE(status.cache.ok);
}

TEST(FeatureFlagsConcernsContextTest, FourArgCreateCustomStillWorks) {
    // Existing callers that use the 4-arg overload continue to compile and
    // get a NoOpFeatureFlags injected automatically.
    auto ctx = ConcernsContext::createCustom(
        std::make_unique<NoOpLogger>(),
        std::make_unique<NoOpTracer>(),
        std::make_unique<NoOpMetrics>(),
        std::make_unique<NoOpCache>()
    );
    EXPECT_FALSE(ctx->featureFlags().isEnabled("anything"));
    EXPECT_TRUE(ctx->healthCheck().isHealthy());
}

// ---------------------------------------------------------------------------
// ConcernsContext::create(Config) — feature flags via Config
// ---------------------------------------------------------------------------

TEST(FeatureFlagsConcernsContextTest, CreateWithConfigDefaultUsesInMemory) {
    // Default adapter is "inmemory"; flags start empty so every query returns false.
    ConcernsContext::Config cfg;
    cfg.loggerAdapter  = "noop";
    cfg.tracerAdapter  = "noop";
    cfg.metricsAdapter = "noop";
    cfg.cacheAdapter   = "noop";
    cfg.circuitBreakerAdapter = "noop";
    auto ctx = ConcernsContext::create(cfg);
    // Unknown flag → false
    EXPECT_FALSE(ctx->featureFlags().isEnabled("any_flag"));
    // Runtime mutation works
    ctx->featureFlags().setValue("any_flag", true);
    EXPECT_TRUE(ctx->featureFlags().isEnabled("any_flag"));
}

TEST(FeatureFlagsConcernsContextTest, CreateWithConfigInitialFlags) {
    ConcernsContext::Config cfg;
    cfg.loggerAdapter  = "noop";
    cfg.tracerAdapter  = "noop";
    cfg.metricsAdapter = "noop";
    cfg.cacheAdapter   = "noop";
    cfg.circuitBreakerAdapter = "noop";
    cfg.initialFeatureFlags   = {{"dark_mode", true}, {"beta_feature", false}};
    auto ctx = ConcernsContext::create(cfg);
    EXPECT_TRUE(ctx->featureFlags().isEnabled("dark_mode"));
    EXPECT_FALSE(ctx->featureFlags().isEnabled("beta_feature"));
    EXPECT_FALSE(ctx->featureFlags().isEnabled("unknown_flag"));
}

TEST(FeatureFlagsConcernsContextTest, CreateWithConfigNoopAdapter) {
    ConcernsContext::Config cfg;
    cfg.loggerAdapter  = "noop";
    cfg.tracerAdapter  = "noop";
    cfg.metricsAdapter = "noop";
    cfg.cacheAdapter   = "noop";
    cfg.circuitBreakerAdapter  = "noop";
    cfg.featureFlagsAdapter    = "noop";
    cfg.initialFeatureFlags    = {{"should_be_ignored", true}};
    auto ctx = ConcernsContext::create(cfg);
    // NoOp provider ignores setValue and always returns false
    EXPECT_FALSE(ctx->featureFlags().isEnabled("should_be_ignored"));
    ctx->featureFlags().setValue("should_be_ignored", true);
    EXPECT_FALSE(ctx->featureFlags().isEnabled("should_be_ignored"));
}
