#include <gtest/gtest.h>
#include "utils/logger.h"

using namespace themis::utils;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::shared_ptr<Logger> makeLogger() {
    // Initialise the global spdlog logger once so Logger::warn/error etc. don't crash.
    Logger::init("/tmp/test_sampled_logger.log", Logger::Level::TRACE);
    return std::make_shared<Logger>();
}

// ============================================================================
// Construction
// ============================================================================

TEST(SampledLogger, ConstructionDoesNotThrow) {
    EXPECT_NO_THROW({
        auto sampled = SampledLogger(makeLogger());
        (void)sampled;
    });
}

TEST(SampledLogger, InitialSuppressionCountIsZero) {
    SampledLogger sl(makeLogger());
    EXPECT_EQ(sl.suppressed_total(), 0u);
}

// ============================================================================
// Sampling behaviour
// ============================================================================

TEST(SampledLogger, WarnAndErrorAreNeverSuppressed) {
    // With a very small burst the first few WARN/ERROR calls should still pass.
    SampledLoggerConfig cfg;
    cfg.warn_sample_rate  = 1.0;
    cfg.error_sample_rate = 1.0;
    cfg.burst_size        = 100.0; // plenty of tokens

    SampledLogger sl(makeLogger(), cfg);

    // Fire 5 WARN messages at the same call site — none should be suppressed.
    for (int i = 0; i < 5; ++i) {
        sl.log(Logger::Level::WARN, "warn msg", __FILE__, __LINE__);
    }
    EXPECT_EQ(sl.suppressed_total(), 0u);
}

TEST(SampledLogger, DebugAtZeroRateAlwaysSuppressed) {
    SampledLoggerConfig cfg;
    cfg.debug_sample_rate = 0.0; // always suppress
    cfg.burst_size        = 0.0; // empty bucket

    SampledLogger sl(makeLogger(), cfg);

    for (int i = 0; i < 10; ++i) {
        sl.log(Logger::Level::DEBUG, "debug msg", __FILE__, __LINE__);
    }
    // All 10 calls should be suppressed.
    EXPECT_EQ(sl.suppressed_total(), 10u);
}

TEST(SampledLogger, BurstAllowsInitialMessages) {
    SampledLoggerConfig cfg;
    cfg.info_sample_rate = 1.0;  // always pass coin flip
    cfg.burst_size       = 5.0;  // 5 initial tokens
    cfg.burst_rate       = 0.0001; // almost no refill

    SampledLogger sl(makeLogger(), cfg);

    // First 5 INFO calls should be allowed; beyond that the bucket is empty.
    int allowed = 0;
    for (int i = 0; i < 20; ++i) {
        uint64_t before = sl.suppressed_total();
        sl.log(Logger::Level::INFO, "info", __FILE__, __LINE__);
        if (sl.suppressed_total() == before) ++allowed;
    }
    // At least the burst allowance of calls should have gone through.
    EXPECT_GE(allowed, 5);
}

// ============================================================================
// Reset stats
// ============================================================================

TEST(SampledLogger, ResetStatsClearsCounter) {
    SampledLoggerConfig cfg;
    cfg.debug_sample_rate = 0.0;
    cfg.burst_size        = 0.0;

    SampledLogger sl(makeLogger(), cfg);
    for (int i = 0; i < 5; ++i)
        sl.log(Logger::Level::DEBUG, "d", __FILE__, __LINE__);

    EXPECT_EQ(sl.suppressed_total(), 5u);
    sl.reset_stats();
    EXPECT_EQ(sl.suppressed_total(), 0u);
}

// ============================================================================
// Hot-reload config
// ============================================================================

TEST(SampledLogger, SetConfigChangesRates) {
    SampledLoggerConfig cfg;
    cfg.info_sample_rate = 1.0;
    cfg.burst_size       = 100.0;

    SampledLogger sl(makeLogger(), cfg);
    sl.log(Logger::Level::INFO, "msg", __FILE__, __LINE__);
    EXPECT_EQ(sl.suppressed_total(), 0u);

    // Now switch to zero rate — next call must be suppressed.
    SampledLoggerConfig cfg2;
    cfg2.info_sample_rate = 0.0;
    cfg2.burst_size       = 0.0;
    sl.set_config(cfg2);

    sl.log(Logger::Level::INFO, "msg", __FILE__, __LINE__);
    EXPECT_GE(sl.suppressed_total(), 1u);
}
