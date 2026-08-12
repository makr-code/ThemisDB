#include <gtest/gtest.h>
#include "config/config_metrics_exporter.h"
#include "config/config_path_resolver.h"

#include <chrono>
#include <string>
#include <string_view>

namespace themis {
namespace config {
namespace test {

// ─────────────────────────────────────────────────────────────────────────────
// Helper
// ─────────────────────────────────────────────────────────────────────────────

static constexpr long long kScrapeLatencyLimitUs = 1000LL; // 1 ms expressed in µs

/// Returns true when @p text contains @p needle as a substring.
static bool contains(const std::string& text, std::string_view needle) {
    return text.find(needle) != std::string::npos;
}

// ─────────────────────────────────────────────────────────────────────────────
// Fixture
// ─────────────────────────────────────────────────────────────────────────────

class ConfigMetricsScrapeTest : public ::testing::Test {
protected:
    void SetUp() override {
        ConfigPathResolver::resetMetrics();
        ConfigPathResolver::clearCache();
    }

    void TearDown() override {
        ConfigPathResolver::resetMetrics();
        ConfigPathResolver::clearCache();
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// 1. Scrape latency – core performance requirement (< 1 ms)
// ═══════════════════════════════════════════════════════════════════════════

// Cold scrape: all counters at zero, empty cache.
TEST_F(ConfigMetricsScrapeTest, ScrapeLatencyColdBelowOneMillisecond) {
    const auto t0 = std::chrono::steady_clock::now();
    const std::string text = ConfigMetricsExporter::collect();
    const auto t1 = std::chrono::steady_clock::now();

    const long long elapsed_us =
        std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();

    EXPECT_LT(elapsed_us, kScrapeLatencyLimitUs)
        << "Cold scrape took " << elapsed_us << " µs; requirement is < 1 ms (1000 µs)";

    // Sanity: output must be non-empty
    EXPECT_FALSE(text.empty());
}

// Warm scrape: counters populated by 200 resolve attempts → hits + misses recorded.
TEST_F(ConfigMetricsScrapeTest, ScrapeLatencyWarmBelowOneMillisecond) {
    // Populate counters without touching the filesystem (tryResolve on a
    // non-existent path still records a miss and increments unmapped_requests).
    for (int i = 0; i < 200; ++i) {
        ConfigPathResolver::tryResolve("config/lora_training_config.yaml");
    }
    ConfigPathResolver::clearCache();
    ConfigPathResolver::tryResolve("config/lora_training_config.yaml");

    const auto t0 = std::chrono::steady_clock::now();
    const std::string text = ConfigMetricsExporter::collect();
    const auto t1 = std::chrono::steady_clock::now();

    const long long elapsed_us =
        std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();

    EXPECT_LT(elapsed_us, kScrapeLatencyLimitUs)
        << "Warm scrape took " << elapsed_us << " µs; requirement is < 1 ms (1000 µs)";
}

// Repeated scrapes: verify that none of 100 consecutive calls exceeds the limit.
TEST_F(ConfigMetricsScrapeTest, RepeatedScrapesAllBelowOneMillisecond) {
    constexpr int kRuns = 100;
    for (int i = 0; i < kRuns; ++i) {
        const auto t0 = std::chrono::steady_clock::now();
        const std::string text = ConfigMetricsExporter::collect();
        const auto t1 = std::chrono::steady_clock::now();

        const long long elapsed_us =
            std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();

        EXPECT_LT(elapsed_us, kScrapeLatencyLimitUs)
            << "Scrape #" << i << " took " << elapsed_us
            << " µs; requirement is < 1 ms (1000 µs)";

        (void)text; // suppress unused-variable warning
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// 2. Output format – well-formed Prometheus text exposition
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(ConfigMetricsScrapeTest, OutputContainsAllRequiredMetricNames) {
    const std::string text = ConfigMetricsExporter::collect();

    EXPECT_TRUE(contains(text, "themis_config_resolution_hits_total"))
        << "Missing metric: themis_config_resolution_hits_total";
    EXPECT_TRUE(contains(text, "themis_config_resolution_misses_total"))
        << "Missing metric: themis_config_resolution_misses_total";
    EXPECT_TRUE(contains(text, "themis_config_legacy_fallbacks_total"))
        << "Missing metric: themis_config_legacy_fallbacks_total";
    EXPECT_TRUE(contains(text, "themis_config_legacy_fallbacks_total{category=\"unknown\""))
        << "Missing category label on legacy fallback metric";
    EXPECT_TRUE(contains(text, "themis_config_unmapped_requests_total"))
        << "Missing metric: themis_config_unmapped_requests_total";
    EXPECT_TRUE(contains(text, "themis_config_cache_hit_ratio"))
        << "Missing metric: themis_config_cache_hit_ratio";
    EXPECT_TRUE(contains(text, "themis_config_cache_capacity"))
        << "Missing metric: themis_config_cache_capacity";
    EXPECT_TRUE(contains(text, "themis_config_cache_ttl_seconds"))
        << "Missing metric: themis_config_cache_ttl_seconds";
}

TEST_F(ConfigMetricsScrapeTest, OutputContainsHelpAndTypeAnnotations) {
    const std::string text = ConfigMetricsExporter::collect();

    // Every metric must have a # HELP and a # TYPE line
    EXPECT_TRUE(contains(text, "# HELP themis_config_resolution_hits_total"));
    EXPECT_TRUE(contains(text, "# TYPE themis_config_resolution_hits_total counter"));
    EXPECT_TRUE(contains(text, "# HELP themis_config_cache_hit_ratio"));
    EXPECT_TRUE(contains(text, "# TYPE themis_config_cache_hit_ratio gauge"));
    EXPECT_TRUE(contains(text, "# HELP themis_config_cache_capacity"));
    EXPECT_TRUE(contains(text, "# TYPE themis_config_cache_capacity gauge"));
}

TEST_F(ConfigMetricsScrapeTest, OutputEndsWithNewline) {
    const std::string text = ConfigMetricsExporter::collect();
    ASSERT_FALSE(text.empty());
    EXPECT_EQ(text.back(), '\n')
        << "Prometheus text-exposition format requires a trailing newline";
}

// ═══════════════════════════════════════════════════════════════════════════
// 3. Counter accuracy – values reflect actual resolver activity
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(ConfigMetricsScrapeTest, ZeroCountersOnFreshReset) {
    const std::string text = ConfigMetricsExporter::collect();

    // After reset all counters must be 0
    EXPECT_TRUE(contains(text, "themis_config_resolution_hits_total 0"));
    EXPECT_TRUE(contains(text, "themis_config_resolution_misses_total 0"));
    EXPECT_TRUE(contains(text, "themis_config_legacy_fallbacks_total{category="));
    EXPECT_TRUE(contains(text, "themis_config_legacy_fallbacks_total{category=\"unknown\"} 0"));
    EXPECT_TRUE(contains(text, "themis_config_unmapped_requests_total 0"));
}

TEST_F(ConfigMetricsScrapeTest, CacheHitRatioIsZeroWhenNoLookups) {
    const std::string text = ConfigMetricsExporter::collect();
    // With no lookups the hit ratio must be exactly 0
    EXPECT_TRUE(contains(text, "themis_config_cache_hit_ratio 0"));
}

} // namespace test
} // namespace config
} // namespace themis
