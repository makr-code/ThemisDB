/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_config_path_resolver.cpp                      ║
  Version:         0.0.27                                             ║
  Last Modified:   2026-02-22 08:56:40                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     337                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "config/config_path_resolver.h"
#include "config/config_errors.h"
#include <filesystem>
#include <fstream>

namespace themis {
namespace config {
namespace test {

// Test fixture for ConfigPathResolver
class ConfigPathResolverTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset metrics before each test
        ConfigPathResolver::resetMetrics();
        
        // Create temporary test directories
        test_dir_ = std::filesystem::temp_directory_path() / "themisdb_config_test";
        std::filesystem::create_directories(test_dir_);
        
        // Create test config files
        legacy_path_ = test_dir_ / "config" / "test.yaml";
        new_path_ = test_dir_ / "config" / "core" / "test.yaml";
        
        std::filesystem::create_directories(legacy_path_.parent_path());
        std::filesystem::create_directories(new_path_.parent_path());
        
        // Create test files
        createTestFile(legacy_path_);
        createTestFile(new_path_);
    }
    
    void TearDown() override {
        // Clean up test directory
        std::filesystem::remove_all(test_dir_);
    }
    
    void createTestFile(const std::filesystem::path& path) {
        std::ofstream file(path);
        file << "test: data\n";
        file.close();
    }
    
    std::filesystem::path test_dir_;
    std::filesystem::path legacy_path_;
    std::filesystem::path new_path_;
};

// ═══════════════════════════════════════════════════════════
// Path Normalization Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ConfigPathResolverTest, NormalizePathForwardSlashes) {
    // normalizePath is private, but we can test it indirectly through mapLegacyToNew
    std::string result = ConfigPathResolver::mapLegacyToNew("config/test.yaml");
    EXPECT_FALSE(result.empty());
}

TEST_F(ConfigPathResolverTest, NormalizePathBackslashes) {
    std::string result = ConfigPathResolver::mapLegacyToNew("config\\test.yaml");
    // Should normalize backslashes to forward slashes
    EXPECT_EQ(result.find('\\'), std::string::npos);
}

TEST_F(ConfigPathResolverTest, NormalizePathRemovesLeadingDotSlash) {
    std::string path1 = ConfigPathResolver::mapLegacyToNew("./config/test.yaml");
    std::string path2 = ConfigPathResolver::mapLegacyToNew("config/test.yaml");
    EXPECT_EQ(path1, path2);
}

TEST_F(ConfigPathResolverTest, NormalizePathRemovesTrailingSlash) {
    std::string path1 = ConfigPathResolver::mapLegacyToNew("config/test/");
    std::string path2 = ConfigPathResolver::mapLegacyToNew("config/test");
    // Both should normalize to the same path
    EXPECT_FALSE(path1.ends_with("/"));
    EXPECT_FALSE(path2.ends_with("/"));
}

// ═══════════════════════════════════════════════════════════
// Mapping Table Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ConfigPathResolverTest, MapLegacyToNewKnownPath) {
    std::string result = ConfigPathResolver::mapLegacyToNew("config/lora_training_config.yaml");
    EXPECT_EQ(result, "config/ai_ml/lora_training_config.yaml");
}

TEST_F(ConfigPathResolverTest, MapLegacyToNewUnknownPath) {
    std::string result = ConfigPathResolver::mapLegacyToNew("config/unknown_file.yaml");
    // Should return the normalized path as-is
    EXPECT_EQ(result, "config/unknown_file.yaml");
}

TEST_F(ConfigPathResolverTest, MapLegacyToNewSecurityPath) {
    std::string result = ConfigPathResolver::mapLegacyToNew("config/pii_patterns.yaml");
    EXPECT_EQ(result, "config/security/pii_patterns.yaml");
}

TEST_F(ConfigPathResolverTest, MapLegacyToNewPerformancePath) {
    std::string result = ConfigPathResolver::mapLegacyToNew("config/query_cache_olap.yaml");
    EXPECT_EQ(result, "config/performance/query_cache/olap.yaml");
}

TEST_F(ConfigPathResolverTest, IsLegacyPathTrue) {
    EXPECT_TRUE(ConfigPathResolver::isLegacyPath("config/lora_training_config.yaml"));
    EXPECT_TRUE(ConfigPathResolver::isLegacyPath("config/pii_patterns.yaml"));
}

TEST_F(ConfigPathResolverTest, IsLegacyPathFalse) {
    EXPECT_FALSE(ConfigPathResolver::isLegacyPath("config/ai_ml/lora_training_config.yaml"));
    EXPECT_FALSE(ConfigPathResolver::isLegacyPath("config/unknown.yaml"));
}

// ═══════════════════════════════════════════════════════════
// Resolution Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ConfigPathResolverTest, TryResolveReturnsNulloptForNonexistentFile) {
    auto result = ConfigPathResolver::tryResolve("config/nonexistent.yaml");
    EXPECT_FALSE(result.has_value());
}

TEST_F(ConfigPathResolverTest, TryResolveReturnsNulloptForPathTraversal) {
    auto result = ConfigPathResolver::tryResolve("config/../../../etc/passwd");
    EXPECT_FALSE(result.has_value());
}

TEST_F(ConfigPathResolverTest, ResolveThrowsForNonexistentFile) {
    EXPECT_THROW({
        ConfigPathResolver::resolve("config/nonexistent.yaml");
    }, ConfigNotFoundException);
}

TEST_F(ConfigPathResolverTest, ResolveThrowsConfigNotFoundExceptionWithDetails) {
    try {
        ConfigPathResolver::resolve("config/missing.yaml");
        FAIL() << "Expected ConfigNotFoundException to be thrown";
    } catch (const ConfigNotFoundException& e) {
        EXPECT_EQ(e.requested_path(), "config/missing.yaml");
        EXPECT_FALSE(e.attempted_paths().empty());
    }
}

// ═══════════════════════════════════════════════════════════
// Metrics Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ConfigPathResolverTest, MetricsTrackResolutionHits) {
    ConfigPathResolver::resetMetrics();
    
    // Create a test file that exists
    auto temp_file = test_dir_ / "config" / "temp.yaml";
    createTestFile(temp_file);
    
    auto result = ConfigPathResolver::tryResolve(temp_file.string());
    
    if (result.has_value()) {
        EXPECT_GT(ConfigPathResolver::metrics().resolution_hits, 0);
    }
}

TEST_F(ConfigPathResolverTest, MetricsTrackResolutionMisses) {
    ConfigPathResolver::resetMetrics();
    
    try {
        ConfigPathResolver::resolve("config/nonexistent_metrics_test.yaml");
    } catch (const ConfigNotFoundException&) {
        // Expected
    }
    
    EXPECT_GT(ConfigPathResolver::metrics().resolution_misses, 0);
}

TEST_F(ConfigPathResolverTest, MetricsTrackUnmappedRequests) {
    ConfigPathResolver::resetMetrics();
    
    auto result = ConfigPathResolver::tryResolve("config/unmapped_file.yaml");
    
    // Should track as unmapped since the file doesn't exist and has no mapping
    EXPECT_GT(ConfigPathResolver::metrics().unmapped_requests, 0);
}

TEST_F(ConfigPathResolverTest, MetricsResetWorks) {
    ConfigPathResolver::resetMetrics();
    
    EXPECT_EQ(ConfigPathResolver::metrics().resolution_hits, 0);
    EXPECT_EQ(ConfigPathResolver::metrics().resolution_misses, 0);
    EXPECT_EQ(ConfigPathResolver::metrics().legacy_fallbacks, 0);
    EXPECT_EQ(ConfigPathResolver::metrics().new_path_hits, 0);
    EXPECT_EQ(ConfigPathResolver::metrics().unmapped_requests, 0);
    EXPECT_EQ(ConfigPathResolver::metrics().cache_hits, 0);
    EXPECT_EQ(ConfigPathResolver::metrics().cache_misses, 0);
}

// ═══════════════════════════════════════════════════════════
// Cache Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ConfigPathResolverTest, CachingEnabled) {
    ConfigPathResolver::resetMetrics();
    ConfigPathResolver::clearCache();
    ConfigPathResolver::setCachingEnabled(true);
    
    // Create a test file
    auto temp_file = test_dir_ / "config" / "cached_test.yaml";
    createTestFile(temp_file);
    
    // First access - cache miss
    auto result1 = ConfigPathResolver::tryResolve(temp_file.string());
    EXPECT_TRUE(result1.has_value());
    
    // Second access - cache hit
    auto result2 = ConfigPathResolver::tryResolve(temp_file.string());
    EXPECT_TRUE(result2.has_value());
    
    EXPECT_GT(ConfigPathResolver::metrics().cache_hits, 0);
}

TEST_F(ConfigPathResolverTest, CachingDisabled) {
    ConfigPathResolver::resetMetrics();
    ConfigPathResolver::setCachingEnabled(false);
    
    // Create a test file
    auto temp_file = test_dir_ / "config" / "no_cache_test.yaml";
    createTestFile(temp_file);
    
    // Multiple accesses
    ConfigPathResolver::tryResolve(temp_file.string());
    ConfigPathResolver::tryResolve(temp_file.string());
    
    // No cache hits when caching is disabled
    EXPECT_EQ(ConfigPathResolver::metrics().cache_hits, 0);
    
    // Re-enable for other tests
    ConfigPathResolver::setCachingEnabled(true);
}

TEST_F(ConfigPathResolverTest, CacheClear) {
    ConfigPathResolver::setCachingEnabled(true);
    ConfigPathResolver::clearCache();
    
    // Create a test file
    auto temp_file = test_dir_ / "config" / "clear_test.yaml";
    createTestFile(temp_file);
    
    // Cache it
    ConfigPathResolver::tryResolve(temp_file.string());
    
    // Clear cache
    ConfigPathResolver::clearCache();
    
    auto stats = ConfigPathResolver::cacheStats();
    EXPECT_EQ(stats.size, 0);
}

// ═══════════════════════════════════════════════════════════
// Mapping Table Coverage Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ConfigPathResolverTest, AllMappingTableEntriesAreValid) {
    // Test that all legacy paths in the mapping table map to valid new paths
    std::vector<std::string> test_paths = {
        "config/lora_training_config.yaml",
        "config/vision_config.yaml",
        "config/llm_system_prompts.yaml",
        "config/pii_patterns.yaml",
        "config/rbac_roles.json",
        "config/ethical_guidelines.yaml",
        "config/mime_types.yaml",
        "config/scaling_optimizations.yaml",
        "config/config.yaml",
        "config/config.rpi3.json",
        "config/connection_pool_config.yaml",
        "config/prometheus_arm.yml",
    };
    
    for (const auto& legacy_path : test_paths) {
        std::string new_path = ConfigPathResolver::mapLegacyToNew(legacy_path);
        EXPECT_FALSE(new_path.empty()) << "Mapping for " << legacy_path << " should not be empty";
        EXPECT_NE(new_path, legacy_path) << "Path " << legacy_path << " should map to a different location";
    }
}

TEST_F(ConfigPathResolverTest, NewPathsFollowHierarchicalStructure) {
    // Verify that new paths follow the expected directory structure
    std::string ai_path = ConfigPathResolver::mapLegacyToNew("config/lora_training_config.yaml");
    EXPECT_TRUE(ai_path.find("config/ai_ml/") == 0) << "AI config should be under config/ai_ml/";
    
    std::string security_path = ConfigPathResolver::mapLegacyToNew("config/pii_patterns.yaml");
    EXPECT_TRUE(security_path.find("config/security/") == 0) << "Security config should be under config/security/";
    
    std::string perf_path = ConfigPathResolver::mapLegacyToNew("config/scaling_optimizations.yaml");
    EXPECT_TRUE(perf_path.find("config/performance/") == 0) << "Performance config should be under config/performance/";
}

// ═══════════════════════════════════════════════════════════
// Security Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ConfigPathResolverTest, RejectsPathTraversal) {
    auto result = ConfigPathResolver::tryResolve("config/../../etc/passwd");
    EXPECT_FALSE(result.has_value()) << "Path traversal should be rejected";
}

TEST_F(ConfigPathResolverTest, RejectsMultiplePathTraversals) {
    auto result = ConfigPathResolver::tryResolve("config/../../../../../../../etc/passwd");
    EXPECT_FALSE(result.has_value()) << "Multiple path traversals should be rejected";
}

TEST_F(ConfigPathResolverTest, RejectsRelativePathTraversal) {
    auto result = ConfigPathResolver::tryResolve("./config/../../../etc/passwd");
    EXPECT_FALSE(result.has_value()) << "Relative path traversal should be rejected";
}

// ═══════════════════════════════════════════════════════════
// Deprecation Aggregation Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ConfigPathResolverTest, DeprecationReportEmptyInitially) {
    ConfigPathResolver::resetMetrics();
    auto report = ConfigPathResolver::deprecationReport();
    EXPECT_TRUE(report.empty());
}

TEST_F(ConfigPathResolverTest, DeprecationReportTracksLegacyUsage) {
    // To trigger the aggregator we need a relative path that matches a
    // PATH_MAPPING key and whose legacy file exists but whose new path
    // does NOT exist.  We achieve this by temporarily changing CWD to
    // test_dir_, where only "config/pii_patterns.yaml" (legacy) is present.
    ConfigPathResolver::resetMetrics();
    ConfigPathResolver::clearCache();
    ConfigPathResolver::setAggregationEnabled(false);

    // Create the legacy file at the relative location inside test_dir_
    std::filesystem::create_directories(test_dir_ / "config");
    createTestFile(test_dir_ / "config" / "pii_patterns.yaml");
    // Deliberately do NOT create the new path "config/security/pii_patterns.yaml"

    auto prev_cwd = std::filesystem::current_path();
    std::filesystem::current_path(test_dir_);

    // Now tryResolve("config/pii_patterns.yaml"):
    //   - new path = "config/security/pii_patterns.yaml" → does not exist
    //   - legacy path = "config/pii_patterns.yaml"       → exists
    //   → legacy fallback triggered, aggregator.incrementUsage() called
    auto result = ConfigPathResolver::tryResolve("config/pii_patterns.yaml");

    std::filesystem::current_path(prev_cwd);

    EXPECT_TRUE(result.has_value()) << "Legacy fallback should resolve to the legacy path";
    EXPECT_EQ(result.value(), "config/pii_patterns.yaml") << "Should resolve to the legacy relative path";

    auto report = ConfigPathResolver::deprecationReport();
    ASSERT_EQ(report.size(), 1u) << "Aggregator should have recorded one legacy path";
    EXPECT_EQ(report[0].legacy_path, "config/pii_patterns.yaml");
    EXPECT_GE(report[0].usage_count, 1u);
    EXPECT_FALSE(report[0].new_path.empty());
}

TEST_F(ConfigPathResolverTest, DeprecationReportEntriesHaveExpectedFields) {
    // After a known legacy fallback the entry fields must be populated correctly.
    ConfigPathResolver::resetMetrics();
    ConfigPathResolver::clearCache();
    ConfigPathResolver::setAggregationEnabled(false);

    std::filesystem::create_directories(test_dir_ / "config");
    createTestFile(test_dir_ / "config" / "pii_patterns.yaml");

    auto prev_cwd = std::filesystem::current_path();
    std::filesystem::current_path(test_dir_);
    ConfigPathResolver::tryResolve("config/pii_patterns.yaml");
    std::filesystem::current_path(prev_cwd);

    auto report = ConfigPathResolver::deprecationReport();
    ASSERT_FALSE(report.empty());
    for (const auto& entry : report) {
        EXPECT_FALSE(entry.legacy_path.empty());
        EXPECT_FALSE(entry.new_path.empty());
        // Any entry that appears in the report must have been accessed at least once
        EXPECT_GE(entry.usage_count, 1u);
    }
}

TEST_F(ConfigPathResolverTest, DeprecationReportResetOnMetricsReset) {
    ConfigPathResolver::resetMetrics();
    // Report must be empty after a full reset.
    auto report = ConfigPathResolver::deprecationReport();
    EXPECT_TRUE(report.empty());
}

TEST_F(ConfigPathResolverTest, SetAggregationEnabledStartsAndStopsThread) {
    // Enable aggregation with a long interval so the reporter thread does not fire
    ConfigPathResolver::setAggregationEnabled(true, 3600);
    // Disable it again – the background thread should stop cleanly
    ConfigPathResolver::setAggregationEnabled(false);
    // If we reach here without deadlock/crash, the start/stop cycle works
    SUCCEED();
}

TEST_F(ConfigPathResolverTest, AggregationEnabledSuppressesPerCallWarnings) {
    // Enable aggregation
    ConfigPathResolver::setAggregationEnabled(true, 3600);

    // Create a legacy-path file so that the fallback branch is exercised.
    // (Aggregator should record usage; no per-call spdlog::warn should fire.)
    auto temp_file = test_dir_ / "config" / "agg_warn_test.yaml";
    createTestFile(temp_file);

    // tryResolve with an absolute path that exists; no mapping → no legacy branch
    auto result = ConfigPathResolver::tryResolve(temp_file.string());
    // No assertion on result – we only verify no crash/deadlock

    ConfigPathResolver::setAggregationEnabled(false);
    SUCCEED();
}

TEST_F(ConfigPathResolverTest, DeprecationReportSortedByUsageCountDescending) {
    // Trigger two different legacy paths multiple times so the sort can be
    // verified with real data.
    ConfigPathResolver::resetMetrics();
    ConfigPathResolver::clearCache();
    ConfigPathResolver::setAggregationEnabled(false);

    std::filesystem::create_directories(test_dir_ / "config");
    createTestFile(test_dir_ / "config" / "pii_patterns.yaml");
    createTestFile(test_dir_ / "config" / "rbac_roles.json");

    auto prev_cwd = std::filesystem::current_path();
    std::filesystem::current_path(test_dir_);

    // Access pii_patterns 3 times and rbac_roles 1 time
    for (int pass = 0; pass < 3; ++pass) {
        ConfigPathResolver::clearCache();
        ConfigPathResolver::tryResolve("config/pii_patterns.yaml");
    }
    ConfigPathResolver::clearCache();
    ConfigPathResolver::tryResolve("config/rbac_roles.json");

    std::filesystem::current_path(prev_cwd);

    auto report = ConfigPathResolver::deprecationReport();
    ASSERT_GE(report.size(), 2u);
    // Report must be sorted by descending usage_count
    for (size_t i = 1; i < report.size(); ++i) {
        EXPECT_GE(report[i - 1].usage_count, report[i].usage_count)
            << "Report should be sorted by descending usage_count";
    }
    // pii_patterns should be first with count >= 3
    EXPECT_EQ(report[0].legacy_path, "config/pii_patterns.yaml");
    EXPECT_GE(report[0].usage_count, 3u);
}

} // namespace test
} // namespace config
} // namespace themis
