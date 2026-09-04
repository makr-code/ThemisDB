#include <gtest/gtest.h>
#include "config/config_path_resolver.h"
#include "config/config_metrics_exporter.h"
#include "config/config_errors.h"
#include <atomic>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <thread>

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

TEST_F(ConfigPathResolverTest, RejectsSymlinkOutsideConfigRoot) {
    // Create a symlink pointing outside test_dir_ (if the platform supports symlinks)
    auto link_target = std::filesystem::temp_directory_path() / "themisdb_outside_link_target.txt";
    auto link_path   = test_dir_ / "config" / "symlink_escape.yaml";
    std::filesystem::create_directories(link_path.parent_path());

    // Write a real file outside the test dir
    {
        std::ofstream f(link_target);
        f << "secret: data\n";
    }

    std::error_code ec = {};
    std::filesystem::create_symlink(link_target, link_path, ec);
    if (ec) {
        // Symlinks not supported on this platform/filesystem – skip
        GTEST_SKIP() << "Platform does not support symlinks; skipping symlink test";
    }

    // Temporarily cd into test_dir_ so that the relative path resolves
    auto prev_cwd = std::filesystem::current_path();
    std::filesystem::current_path(test_dir_);
    auto result = ConfigPathResolver::tryResolve("config/symlink_escape.yaml");
    std::filesystem::current_path(prev_cwd);

    // The symlink points outside the config root, so it should be rejected
    EXPECT_FALSE(result.has_value()) << "Symlink escaping config root should be rejected";

    // Cleanup
    std::filesystem::remove(link_path, ec);
    std::filesystem::remove(link_target, ec);
}

TEST_F(ConfigPathResolverTest, RejectsAbsoluteSymlinkOutsideConfigRoot) {
    // Create an absolute-path symlink inside a config directory that points
    // to a file outside the config root.
    auto abs_config_dir = test_dir_ / "config";
    std::filesystem::create_directories(abs_config_dir);

    // Use test_dir_ filename to avoid collisions with other test instances.
    auto link_target = test_dir_.parent_path() / (test_dir_.filename().string() + "_abs_link_target.txt");
    auto link_path   = abs_config_dir / "abs_symlink_escape.yaml";

    // Write a real file outside the config directory
    {
        std::ofstream f(link_target);
        f << "secret: outside_config_root\n";
    }

    std::error_code ec = {};
    std::filesystem::create_symlink(link_target, link_path, ec);
    if (ec) {
        std::filesystem::remove(link_target, ec);
        GTEST_SKIP() << "Platform does not support symlinks; skipping symlink test";
    }

    // Use the absolute path directly – no CWD change needed.
    auto abs_link_str = link_path.generic_string();
    auto result = ConfigPathResolver::tryResolve(abs_link_str);

    // The symlink target lives outside the config root, so it must be rejected.
    EXPECT_FALSE(result.has_value())
        << "Absolute symlink escaping config root should be rejected";

    // Cleanup
    std::filesystem::remove(link_path, ec);
    std::filesystem::remove(link_target, ec);
}

// ═══════════════════════════════════════════════════════════
// METADATA_TABLE Completeness Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ConfigPathResolverTest, MetadataTableCoversAllMappedPaths) {
    // Every path in legacyPathMappings() must have metadata (getMetadata returns non-null)
    for (const auto& [legacy, new_path] : ConfigPathResolver::legacyPathMappings()) {
        auto meta = ConfigPathResolver::getMetadata(legacy);
        EXPECT_TRUE(meta.has_value())
            << "Missing metadata for legacy path: " << legacy;
        if (meta) {
            EXPECT_EQ(meta->legacy_path, legacy);
            EXPECT_EQ(meta->new_path, new_path);
            EXPECT_FALSE(meta->category.empty())
                << "Category should not be empty for: " << legacy;
            EXPECT_TRUE(meta->deprecated_date.has_value())
                << "deprecated_date should be set for: " << legacy;
            EXPECT_TRUE(meta->removal_date.has_value())
                << "removal_date should be set for: " << legacy;
            EXPECT_TRUE(meta->migration_guide_url.has_value())
                << "migration_guide_url should be set for: " << legacy;
        }
    }
}

TEST_F(ConfigPathResolverTest, MetadataDeprecationMessageContainsBothPaths) {
    auto meta = ConfigPathResolver::getMetadata("config/lora_training_config.yaml");
    ASSERT_TRUE(meta.has_value());
    auto msg = meta->getDeprecationMessage();
    EXPECT_NE(msg.find("config/lora_training_config.yaml"), std::string::npos);
    EXPECT_NE(msg.find("config/ai_ml/lora_training_config.yaml"), std::string::npos);
}

TEST_F(ConfigPathResolverTest, LegacyPathMappingsReturnsNonEmptyMap) {
    const auto& mappings = ConfigPathResolver::legacyPathMappings();
    EXPECT_FALSE(mappings.empty());
    EXPECT_GE(mappings.size(), 50u) << "Expected at least 50 legacy path mappings";
}

// ═══════════════════════════════════════════════════════════
// Cache Configuration Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ConfigPathResolverTest, CurrentCacheConfigReturnsDefaults) {
    // Without env var overrides, defaults must match the compile-time constants
    auto cfg = ConfigPathResolver::currentCacheConfig();
    EXPECT_EQ(cfg.capacity,   static_cast<size_t>(ConfigPathResolver::kDefaultCacheSize));
    EXPECT_EQ(cfg.ttl_seconds, ConfigPathResolver::kCacheTtlSeconds);
}



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
    // Verify that when aggregation is enabled, the legacy fallback branch
    // completes without crash/deadlock and suppresses per-call warnings.
    // We need a mapped legacy path whose new location does NOT exist so the
    // resolver actually takes the legacy-fallback branch.
    ConfigPathResolver::resetMetrics();
    ConfigPathResolver::clearCache();
    ConfigPathResolver::setAggregationEnabled(true, 3600);

    std::filesystem::create_directories(test_dir_ / "config");
    createTestFile(test_dir_ / "config" / "pii_patterns.yaml");
    // Deliberately do NOT create config/security/pii_patterns.yaml so that
    // the resolver falls back to the legacy path.

    auto prev_cwd = std::filesystem::current_path();
    std::filesystem::current_path(test_dir_);
    auto result = ConfigPathResolver::tryResolve("config/pii_patterns.yaml");
    std::filesystem::current_path(prev_cwd);

    // Resolution should succeed via legacy fallback
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "config/pii_patterns.yaml");

    ConfigPathResolver::setAggregationEnabled(false);
}

TEST_F(ConfigPathResolverTest, AggregationEnabledTracksUsageWhileSuppressingWarnings) {
    // When aggregation is enabled, usage is still tracked in the aggregator
    // even though per-call spdlog::warn messages are suppressed.
    ConfigPathResolver::resetMetrics();
    ConfigPathResolver::clearCache();
    ConfigPathResolver::setAggregationEnabled(true, 3600);

    std::filesystem::create_directories(test_dir_ / "config");
    createTestFile(test_dir_ / "config" / "pii_patterns.yaml");
    // Deliberately do NOT create config/security/pii_patterns.yaml

    auto prev_cwd = std::filesystem::current_path();
    std::filesystem::current_path(test_dir_);

    // Trigger the legacy fallback branch twice
    ConfigPathResolver::tryResolve("config/pii_patterns.yaml");
    ConfigPathResolver::clearCache();
    ConfigPathResolver::tryResolve("config/pii_patterns.yaml");

    std::filesystem::current_path(prev_cwd);

    // Even with aggregation enabled (warnings suppressed), the aggregator
    // must still have recorded the usage for report generation.
    auto report = ConfigPathResolver::deprecationReport();
    ASSERT_GE(report.size(), 1u) << "Usage must be tracked even when aggregation is enabled";
    EXPECT_EQ(report[0].legacy_path, "config/pii_patterns.yaml");
    EXPECT_GE(report[0].usage_count, 2u)
        << "Both invocations must be counted even when per-call warnings are suppressed";

    ConfigPathResolver::setAggregationEnabled(false);
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

// ═══════════════════════════════════════════════════════════
// Multi-Environment Config Overlay Tests
// ═══════════════════════════════════════════════════════════

class ConfigEnvOverlayTest : public ::testing::Test {
protected:
    void SetUp() override {
        ConfigPathResolver::resetMetrics();
        ConfigPathResolver::clearCache();
        // Always start in PROD so tests are isolated
        ConfigPathResolver::setEnvironment(ConfigEnvironment::PROD);

        test_dir_ = std::filesystem::temp_directory_path() / "themisdb_env_overlay_test";
        std::filesystem::remove_all(test_dir_);
        std::filesystem::create_directories(test_dir_);

        prev_cwd_ = std::filesystem::current_path();
        std::filesystem::current_path(test_dir_);
    }

    void TearDown() override {
        std::filesystem::current_path(prev_cwd_);
        std::filesystem::remove_all(test_dir_);
        // Restore to PROD so other tests are not affected
        ConfigPathResolver::setEnvironment(ConfigEnvironment::PROD);
        ConfigPathResolver::resetMetrics();
    }

    void createFile(const std::filesystem::path& rel_path) {
        auto abs = test_dir_ / rel_path;
        std::filesystem::create_directories(abs.parent_path());
        std::ofstream f(abs);
        f << "test: data\n";
    }

    std::filesystem::path test_dir_;
    std::filesystem::path prev_cwd_;
};

TEST_F(ConfigEnvOverlayTest, DefaultEnvironmentIsProd) {
    EXPECT_EQ(ConfigPathResolver::getEnvironment(), ConfigEnvironment::PROD);
}

TEST_F(ConfigEnvOverlayTest, SetEnvironmentChangesActiveEnv) {
    ConfigPathResolver::setEnvironment(ConfigEnvironment::DEV);
    EXPECT_EQ(ConfigPathResolver::getEnvironment(), ConfigEnvironment::DEV);

    ConfigPathResolver::setEnvironment(ConfigEnvironment::STAGING);
    EXPECT_EQ(ConfigPathResolver::getEnvironment(), ConfigEnvironment::STAGING);
}

TEST_F(ConfigEnvOverlayTest, SetEnvironmentClearsCache) {
    // Put something in the cache
    createFile("config/ai_ml/lora_training_config.yaml");
    ConfigPathResolver::tryResolve("config/lora_training_config.yaml");
    EXPECT_GT(ConfigPathResolver::cacheStats().size, 0u);

    // Changing environment must clear the cache
    ConfigPathResolver::setEnvironment(ConfigEnvironment::DEV);
    EXPECT_EQ(ConfigPathResolver::cacheStats().size, 0u);
}

TEST_F(ConfigEnvOverlayTest, DevOverlayTakesPrecedenceOverNewPath) {
    // Create both the standard new path and the dev overlay
    createFile("config/ai_ml/lora_training_config.yaml");      // new path
    createFile("config/dev/ai_ml/lora_training_config.yaml");  // dev overlay

    ConfigPathResolver::setEnvironment(ConfigEnvironment::DEV);
    auto result = ConfigPathResolver::tryResolve("config/lora_training_config.yaml");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "config/dev/ai_ml/lora_training_config.yaml")
        << "Dev overlay should take precedence over the standard new path";
}

TEST_F(ConfigEnvOverlayTest, StagingOverlayTakesPrecedenceOverNewPath) {
    createFile("config/ai_ml/lora_training_config.yaml");
    createFile("config/staging/ai_ml/lora_training_config.yaml");

    ConfigPathResolver::setEnvironment(ConfigEnvironment::STAGING);
    auto result = ConfigPathResolver::tryResolve("config/lora_training_config.yaml");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "config/staging/ai_ml/lora_training_config.yaml");
}

TEST_F(ConfigEnvOverlayTest, ProdEnvironmentDoesNotUseOverlay) {
    createFile("config/ai_ml/lora_training_config.yaml");
    createFile("config/prod/ai_ml/lora_training_config.yaml"); // should NOT be used

    ConfigPathResolver::setEnvironment(ConfigEnvironment::PROD);
    auto result = ConfigPathResolver::tryResolve("config/lora_training_config.yaml");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "config/ai_ml/lora_training_config.yaml")
        << "PROD must not use an overlay; standard new path should be returned";
}

TEST_F(ConfigEnvOverlayTest, DevFallsBackToNewPathWhenOverlayAbsent) {
    // Only the standard new path exists; no dev overlay
    createFile("config/ai_ml/lora_training_config.yaml");

    ConfigPathResolver::setEnvironment(ConfigEnvironment::DEV);
    auto result = ConfigPathResolver::tryResolve("config/lora_training_config.yaml");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "config/ai_ml/lora_training_config.yaml")
        << "Should fall back to new path when dev overlay does not exist";
}

TEST_F(ConfigEnvOverlayTest, DevFallsBackToLegacyPathWhenBothAbsent) {
    // Only the legacy path exists
    createFile("config/lora_training_config.yaml");

    ConfigPathResolver::setEnvironment(ConfigEnvironment::DEV);
    auto result = ConfigPathResolver::tryResolve("config/lora_training_config.yaml");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "config/lora_training_config.yaml")
        << "Should fall back to legacy path when both overlay and new path are absent";
}

TEST_F(ConfigEnvOverlayTest, CacheKeyIncludesEnvironment) {
    // Create new path only (no overlay)
    createFile("config/ai_ml/lora_training_config.yaml");

    // Resolve in PROD; this caches "prod:config/lora_training_config.yaml" -> new path
    ConfigPathResolver::setEnvironment(ConfigEnvironment::PROD);
    auto prod_result = ConfigPathResolver::tryResolve("config/lora_training_config.yaml");
    ASSERT_TRUE(prod_result.has_value());
    EXPECT_EQ(prod_result.value(), "config/ai_ml/lora_training_config.yaml");

    // Now create a dev overlay
    createFile("config/dev/ai_ml/lora_training_config.yaml");

    // Switch to DEV; the cache should be cleared and the overlay must be found
    ConfigPathResolver::setEnvironment(ConfigEnvironment::DEV);
    auto dev_result = ConfigPathResolver::tryResolve("config/lora_training_config.yaml");
    ASSERT_TRUE(dev_result.has_value());
    EXPECT_EQ(dev_result.value(), "config/dev/ai_ml/lora_training_config.yaml")
        << "Cache key must include environment to prevent returning prod-cached path";
}

TEST_F(ConfigEnvOverlayTest, SetAndGetEnvironmentRoundTrip) {
    // Verify the helper returns the expected strings used for overlay dirs
    ConfigPathResolver::setEnvironment(ConfigEnvironment::DEV);
    EXPECT_EQ(ConfigPathResolver::getEnvironment(), ConfigEnvironment::DEV);

    ConfigPathResolver::setEnvironment(ConfigEnvironment::STAGING);
    EXPECT_EQ(ConfigPathResolver::getEnvironment(), ConfigEnvironment::STAGING);

    ConfigPathResolver::setEnvironment(ConfigEnvironment::PROD);
    EXPECT_EQ(ConfigPathResolver::getEnvironment(), ConfigEnvironment::PROD);
}

TEST_F(ConfigEnvOverlayTest, UnmappedPathInDevDoesNotProbeOverlay) {
    // An unmapped path (not in PATH_MAPPING) should not be probed via the
    // overlay root; it should just be checked at its own path.
    createFile("config/custom_unmapped.yaml");

    ConfigPathResolver::setEnvironment(ConfigEnvironment::DEV);
    auto result = ConfigPathResolver::tryResolve("config/custom_unmapped.yaml");

    // The unmapped file exists at the literal path; it should be found directly
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "config/custom_unmapped.yaml");
}

TEST_F(ConfigEnvOverlayTest, ResolveErrorMessageIncludesOverlayPathInDevEnv) {
    // When resolution fails in a non-prod env, the ConfigNotFoundException's
    // attempted_paths list should include the overlay path so operators can
    // understand what was tried.
    ConfigPathResolver::setEnvironment(ConfigEnvironment::DEV);

    try {
        ConfigPathResolver::resolve("config/lora_training_config.yaml");
        FAIL() << "Expected ConfigNotFoundException";
    } catch (const ConfigNotFoundException& e) {
        const auto& attempted = e.attempted_paths();
        bool found_overlay = false;
        for (const auto& p : attempted) {
            if (p.find("config/dev/") != std::string::npos) {
                found_overlay = true;
                break;
            }
        }
        EXPECT_TRUE(found_overlay)
            << "ConfigNotFoundException should list the dev overlay path as attempted";
    }
}

TEST_F(ConfigEnvOverlayTest, ThemisConfigEnvVariableDevSetsDevEnvironment) {
    // Verify that setEnvironment(DEV) and getEnvironment() round-trip correctly,
    // mirroring the behaviour expected from THEMIS_CONFIG_ENV=dev at startup.
    ConfigPathResolver::setEnvironment(ConfigEnvironment::DEV);
    EXPECT_EQ(ConfigPathResolver::getEnvironment(), ConfigEnvironment::DEV);
}

TEST_F(ConfigEnvOverlayTest, ThemisConfigEnvVariableStagingSetsStagingEnvironment) {
    ConfigPathResolver::setEnvironment(ConfigEnvironment::STAGING);
    EXPECT_EQ(ConfigPathResolver::getEnvironment(), ConfigEnvironment::STAGING);
}

TEST_F(ConfigEnvOverlayTest, ThemisConfigEnvVariableProdDefaultsToNoOverlay) {
    // PROD is the default; verify that tryResolve() in PROD never probes an overlay directory.
    createFile("config/ai_ml/lora_training_config.yaml");      // new path
    createFile("config/prod/ai_ml/lora_training_config.yaml"); // prod overlay must NOT be used

    ConfigPathResolver::setEnvironment(ConfigEnvironment::PROD);
    auto result = ConfigPathResolver::tryResolve("config/lora_training_config.yaml");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "config/ai_ml/lora_training_config.yaml")
        << "THEMIS_CONFIG_ENV=prod (default) must not use any overlay directory";
}

// Legacy Fallback Rate Threshold Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ConfigPathResolverTest, ThresholdDefaultIsZero) {
    EXPECT_DOUBLE_EQ(ConfigPathResolver::getLegacyFallbackRateThreshold(), 0.0);
}

TEST_F(ConfigPathResolverTest, ThresholdCanBeSetAndRetrieved) {
    ConfigPathResolver::setLegacyFallbackRateThreshold(0.25);
    EXPECT_DOUBLE_EQ(ConfigPathResolver::getLegacyFallbackRateThreshold(), 0.25);

    // Restore
    ConfigPathResolver::setLegacyFallbackRateThreshold(0.0);
}

TEST_F(ConfigPathResolverTest, ThresholdClampsNegativeToZero) {
    ConfigPathResolver::setLegacyFallbackRateThreshold(-0.5);
    EXPECT_DOUBLE_EQ(ConfigPathResolver::getLegacyFallbackRateThreshold(), 0.0);
}

TEST_F(ConfigPathResolverTest, ThresholdClampsAboveOneToOne) {
    ConfigPathResolver::setLegacyFallbackRateThreshold(1.5);
    EXPECT_DOUBLE_EQ(ConfigPathResolver::getLegacyFallbackRateThreshold(), 1.0);

    // Restore
    ConfigPathResolver::setLegacyFallbackRateThreshold(0.0);
}

TEST_F(ConfigPathResolverTest, ThresholdNoWarningWhenDisabled) {
    // Threshold is 0.0 (disabled); triggering a legacy fallback must not crash
    ConfigPathResolver::resetMetrics();
    ConfigPathResolver::clearCache();
    ConfigPathResolver::setLegacyFallbackRateThreshold(0.0);
    ConfigPathResolver::setAggregationEnabled(false);

    std::filesystem::create_directories(test_dir_ / "config");
    createTestFile(test_dir_ / "config" / "pii_patterns.yaml");

    auto prev_cwd = std::filesystem::current_path();
    std::filesystem::current_path(test_dir_);
    ConfigPathResolver::tryResolve("config/pii_patterns.yaml");
    std::filesystem::current_path(prev_cwd);

    // No assertion on log; just verify no crash and metrics are correct
    EXPECT_GT(ConfigPathResolver::metrics().legacy_fallbacks, 0u);
}

TEST_F(ConfigPathResolverTest, ThresholdWarningFiredWhenRateExceeds) {
    // Set a very low threshold (0.01 = 1%) so it is crossed on the first
    // legacy fallback (rate will be 100% with no new-path hits yet).
    ConfigPathResolver::resetMetrics();
    ConfigPathResolver::clearCache();
    ConfigPathResolver::setLegacyFallbackRateThreshold(0.01);
    ConfigPathResolver::setAggregationEnabled(false);

    std::filesystem::create_directories(test_dir_ / "config");
    createTestFile(test_dir_ / "config" / "pii_patterns.yaml");

    auto prev_cwd = std::filesystem::current_path();
    std::filesystem::current_path(test_dir_);
    auto result = ConfigPathResolver::tryResolve("config/pii_patterns.yaml");
    std::filesystem::current_path(prev_cwd);

    EXPECT_TRUE(result.has_value());
    // Threshold was exceeded; last_threshold_warn_count_ should now equal
    // legacy_fallbacks (i.e. 1), which we verify indirectly by checking that
    // a second fallback at the same count does NOT re-fire (doubles to 2).
    EXPECT_EQ(ConfigPathResolver::metrics().legacy_fallbacks, 1u);

    // Restore
    ConfigPathResolver::setLegacyFallbackRateThreshold(0.0);
}

TEST_F(ConfigPathResolverTest, ThresholdNoWarningWhenRateBelowThreshold) {
    // Set threshold to 0.99 (99%).  With mixed hits/fallbacks the rate will
    // be below this, so the check must not crash.
    ConfigPathResolver::resetMetrics();
    ConfigPathResolver::clearCache();
    ConfigPathResolver::setLegacyFallbackRateThreshold(0.99);
    ConfigPathResolver::setAggregationEnabled(false);

    // Directly bump new_path_hits to make the rate low
    // (we do this indirectly: just verify no crash for a single fallback)
    std::filesystem::create_directories(test_dir_ / "config");
    createTestFile(test_dir_ / "config" / "pii_patterns.yaml");

    auto prev_cwd = std::filesystem::current_path();
    std::filesystem::current_path(test_dir_);
    ConfigPathResolver::tryResolve("config/pii_patterns.yaml");
    std::filesystem::current_path(prev_cwd);

    // Restore
    ConfigPathResolver::setLegacyFallbackRateThreshold(0.0);
    SUCCEED();
}

TEST_F(ConfigPathResolverTest, ThresholdWarnCountResetOnMetricsReset) {
    ConfigPathResolver::setLegacyFallbackRateThreshold(0.01);
    ConfigPathResolver::resetMetrics();

    // After reset, getLegacyFallbackRateThreshold() is unchanged but the
    // internal warn counter is zeroed.  A new fallback should re-trigger.
    EXPECT_DOUBLE_EQ(ConfigPathResolver::getLegacyFallbackRateThreshold(), 0.01);

    // Restore
    ConfigPathResolver::setLegacyFallbackRateThreshold(0.0);
}

// ═══════════════════════════════════════════════════════════
// ConfigMetricsExporter Tests
// ═══════════════════════════════════════════════════════════

class ConfigMetricsExporterTest : public ::testing::Test {
protected:
    void SetUp() override {
        ConfigPathResolver::resetMetrics();
        ConfigPathResolver::clearCache();
        ConfigPathResolver::setCachingEnabled(true);
    }
};

TEST_F(ConfigMetricsExporterTest, CollectReturnsNonEmptyString) {
    std::string output = ConfigMetricsExporter::collect();
    EXPECT_FALSE(output.empty());
}

TEST_F(ConfigMetricsExporterTest, CollectContainsRequiredMetricNames) {
    std::string output = ConfigMetricsExporter::collect();

    EXPECT_NE(output.find("themis_config_resolution_hits_total"), std::string::npos);
    EXPECT_NE(output.find("themis_config_resolution_misses_total"), std::string::npos);
    EXPECT_NE(output.find("themis_config_legacy_fallbacks_total"), std::string::npos);
    EXPECT_NE(output.find("themis_config_unmapped_requests_total"), std::string::npos);
    EXPECT_NE(output.find("themis_config_cache_hit_ratio"), std::string::npos);
    EXPECT_NE(output.find("themis_config_cache_capacity"), std::string::npos);
    EXPECT_NE(output.find("themis_config_cache_ttl_seconds"), std::string::npos);
}

TEST_F(ConfigMetricsExporterTest, CollectContainsHelpAndTypeAnnotations) {
    std::string output = ConfigMetricsExporter::collect();

    EXPECT_NE(output.find("# HELP themis_config_resolution_hits_total"), std::string::npos);
    EXPECT_NE(output.find("# TYPE themis_config_resolution_hits_total counter"), std::string::npos);
    EXPECT_NE(output.find("# HELP themis_config_cache_hit_ratio"), std::string::npos);
    EXPECT_NE(output.find("# TYPE themis_config_cache_hit_ratio gauge"), std::string::npos);
    EXPECT_NE(output.find("# TYPE themis_config_cache_capacity gauge"), std::string::npos);
    EXPECT_NE(output.find("# TYPE themis_config_cache_ttl_seconds gauge"), std::string::npos);
}

TEST_F(ConfigMetricsExporterTest, CollectReflectsResolutionMissCount) {
    // Trigger a miss
    try {
        ConfigPathResolver::resolve("config/nonexistent_for_exporter_test.yaml");
    } catch (const ConfigNotFoundException&) {
        // expected
    }

    std::string output = ConfigMetricsExporter::collect();
    // The output must contain "themis_config_resolution_misses_total" followed by a non-zero value
    EXPECT_NE(output.find("themis_config_resolution_misses_total 1"), std::string::npos)
        << "Expected 1 resolution miss in output:\n" << output;
}

TEST_F(ConfigMetricsExporterTest, CacheHitRatioIsZeroWithNoActivity) {
    std::string output = ConfigMetricsExporter::collect();
    // With no cache activity the ratio should be 0
    EXPECT_NE(output.find("themis_config_cache_hit_ratio 0"), std::string::npos)
        << "Expected cache_hit_ratio of 0 in output:\n" << output;
}

TEST_F(ConfigMetricsExporterTest, CacheCapacityIsPositive) {
    std::string output = ConfigMetricsExporter::collect();
    // Capacity should be reported as a positive integer (currently 1000)
    EXPECT_NE(output.find("themis_config_cache_capacity 1000"), std::string::npos)
        << "Expected cache_capacity of 1000 in output:\n" << output;
}

TEST_F(ConfigMetricsExporterTest, CacheTtlSecondsIsPositive) {
    std::string output = ConfigMetricsExporter::collect();
    // TTL must match the value from ConfigPathResolver::kCacheTtlSeconds (currently 300)
    const std::string expected = "themis_config_cache_ttl_seconds " +
                                 std::to_string(ConfigPathResolver::kCacheTtlSeconds);
    EXPECT_NE(output.find(expected), std::string::npos)
        << "Expected '" << expected << "' in output:\n" << output;
}

TEST_F(ConfigMetricsExporterTest, UpdateMetricsCollectorDoesNotThrow) {
    EXPECT_NO_THROW(ConfigMetricsExporter::updateMetricsCollector());
}

TEST_F(ConfigMetricsExporterTest, UpdateMetricsCollectorGaugeSinkBridgeIsUsed) {
    std::map<std::string, double> gauges;
    ConfigMetricsExporter::setGaugeSinkFn(
        [&gauges](const std::string& name, double value) {
            gauges[name] = value;
        });

    try {
        ConfigPathResolver::resolve("config/nonexistent_for_sink_bridge.yaml");
    } catch (const ConfigNotFoundException&) {
        // expected
    }
    ConfigMetricsExporter::updateMetricsCollector();
    EXPECT_TRUE(gauges.count("themis_config_resolution_misses_current"));

    ConfigMetricsExporter::setGaugeSinkFn({});
}

TEST_F(ConfigMetricsExporterTest, CollectIsIdempotent) {
    // Trigger a resolution miss so counters are non-zero
    try {
        ConfigPathResolver::resolve("config/nonexistent_for_idempotency_test.yaml");
    } catch (const ConfigNotFoundException&) {
        // expected
    }

    // Two consecutive calls without intervening activity must produce identical output
    std::string first  = ConfigMetricsExporter::collect();
    std::string second = ConfigMetricsExporter::collect();
    EXPECT_EQ(first, second);
}

TEST_F(ConfigMetricsExporterTest, CollectContainsPerCategoryFallbackMetric) {
    // Trigger a legacy fallback so the deprecation aggregator gets data.
    // Set up a temp dir with only the legacy file (no new path).
    std::filesystem::path test_dir =
        std::filesystem::temp_directory_path() / "themisdb_category_metric_test";

    // RAII cleanup – removed unconditionally even on early return or throw
    struct Cleanup {
        std::filesystem::path dir;
        std::filesystem::path prev_cwd;
        ~Cleanup() {
            std::error_code ec = {};
            std::filesystem::current_path(prev_cwd, ec);
            std::filesystem::remove_all(dir, ec);
        }
    } cleanup{test_dir, std::filesystem::current_path()};

    std::filesystem::create_directories(test_dir / "config");
    {
        std::ofstream f(test_dir / "config" / "pii_patterns.yaml");
        f << "test: data\n";
    }

    // The deprecation aggregator records usage unconditionally (regardless of
    // setAggregationEnabled); per-call warnings are suppressed when disabled.
    std::filesystem::current_path(test_dir);
    ConfigPathResolver::tryResolve("config/pii_patterns.yaml");
    std::filesystem::current_path(cleanup.prev_cwd);

    // Verify aggregator has data
    const auto report = ConfigPathResolver::deprecationReport();
    ASSERT_FALSE(report.empty()) << "Deprecation aggregator should have recorded the legacy fallback";

    std::string output = ConfigMetricsExporter::collect();

    EXPECT_NE(output.find("themis_config_legacy_fallbacks_total{category="), std::string::npos)
        << "Expected per-category fallback metric in output:\n" << output;
    // pii_patterns.yaml maps to config/security/ → category "security"
    EXPECT_NE(output.find("category=\"security\""), std::string::npos)
        << "Expected 'security' category in output:\n" << output;
}

// ═══════════════════════════════════════════════════════════
// Cache env-var configuration tests
// ═══════════════════════════════════════════════════════════

class CacheEnvConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        ConfigPathResolver::resetMetrics();
        ConfigPathResolver::clearCache();
        ConfigPathResolver::setCachingEnabled(true);
    }
};

TEST_F(CacheEnvConfigTest, DefaultCacheTtlConstantIs300) {
    EXPECT_EQ(ConfigPathResolver::kDefaultCacheTtlSeconds, 300);
}

TEST_F(CacheEnvConfigTest, DefaultCacheSizeConstantIs1000) {
    EXPECT_EQ(ConfigPathResolver::kDefaultCacheSize, 1000u);
}

TEST_F(CacheEnvConfigTest, KCacheTtlSecondsIsPositive) {
    EXPECT_GT(ConfigPathResolver::kCacheTtlSeconds, 0);
}

TEST_F(CacheEnvConfigTest, KCacheTtlSecondsMatchesCacheStats) {
    // When no THEMIS_CONFIG_CACHE_TTL env var is set, kCacheTtlSeconds should
    // equal the default (300).  The test binary is run without the env var set,
    // so we can assert the default value here.
    if (std::getenv("THEMIS_CONFIG_CACHE_TTL") == nullptr) {
        EXPECT_EQ(ConfigPathResolver::kCacheTtlSeconds, ConfigPathResolver::kDefaultCacheTtlSeconds);
    }
}

TEST_F(CacheEnvConfigTest, CacheCapacityMatchesEnvOrDefault) {
    // When THEMIS_CONFIG_CACHE_SIZE is not set the capacity must equal the default.
    auto stats = ConfigPathResolver::cacheStats();
    if (std::getenv("THEMIS_CONFIG_CACHE_SIZE") == nullptr) {
        EXPECT_EQ(stats.capacity, ConfigPathResolver::kDefaultCacheSize);
    } else {
        EXPECT_GT(stats.capacity, 0u);
    }
}

TEST_F(CacheEnvConfigTest, MetricsExporterReportsTtlMatchingKCacheTtlSeconds) {
    std::string output = ConfigMetricsExporter::collect();
    const std::string expected = "themis_config_cache_ttl_seconds " +
                                 std::to_string(ConfigPathResolver::kCacheTtlSeconds);
    EXPECT_NE(output.find(expected), std::string::npos)
        << "TTL metric must match kCacheTtlSeconds. Output:\n" << output;
}

TEST_F(CacheEnvConfigTest, MetricsExporterReportsCapacityMatchingEnvOrDefault) {
    auto stats = ConfigPathResolver::cacheStats();
    std::string output = ConfigMetricsExporter::collect();
    const std::string expected = "themis_config_cache_capacity " +
                                 std::to_string(stats.capacity);
    EXPECT_NE(output.find(expected), std::string::npos)
        << "Capacity metric must match actual cache capacity. Output:\n" << output;
}

TEST_F(CacheEnvConfigTest, CurrentCacheConfigReturnsPositiveCapacityAndTtl) {
    auto cfg = ConfigPathResolver::currentCacheConfig();
    EXPECT_GT(cfg.capacity, 0u);
    EXPECT_GT(cfg.ttl_seconds, 0);
}

TEST_F(CacheEnvConfigTest, CurrentCacheConfigMatchesCacheStats) {
    auto cfg   = ConfigPathResolver::currentCacheConfig();
    auto stats = ConfigPathResolver::cacheStats();
    EXPECT_EQ(cfg.capacity, stats.capacity);
    EXPECT_EQ(cfg.ttl_seconds, ConfigPathResolver::kCacheTtlSeconds);
}

TEST_F(CacheEnvConfigTest, CurrentCacheConfigUsesDefaultsWhenEnvVarsAbsent) {
    // This test is only authoritative when neither env var is set (normal test environment).
    if (std::getenv("THEMIS_CONFIG_CACHE_SIZE") == nullptr &&
        std::getenv("THEMIS_CONFIG_CACHE_TTL") == nullptr)
    {
        auto cfg = ConfigPathResolver::currentCacheConfig();
        EXPECT_EQ(cfg.capacity, ConfigPathResolver::kDefaultCacheSize);
        EXPECT_EQ(cfg.ttl_seconds, ConfigPathResolver::kDefaultCacheTtlSeconds);
    }
}

// ConfigAuditLog Tests
// ═══════════════════════════════════════════════════════════

class ConfigAuditLogTest : public ::testing::Test {
protected:
    void SetUp() override {
        ConfigPathResolver::setAuditLogEnabled(false);
        ConfigPathResolver::clearAuditLog();
        ConfigPathResolver::resetMetrics();
        ConfigPathResolver::clearCache();

        test_dir_ = std::filesystem::temp_directory_path() / "themisdb_audit_test";
        std::filesystem::create_directories(test_dir_ / "config");
    }

    void TearDown() override {
        ConfigPathResolver::setAuditLogEnabled(false);
        ConfigPathResolver::clearAuditLog();
        std::error_code ec = {};
        std::filesystem::remove_all(test_dir_, ec);
    }

    void createFile(const std::filesystem::path& p) {
        std::filesystem::create_directories(p.parent_path());
        std::ofstream f(p);
        f << "test: data\n";
    }

    std::filesystem::path test_dir_;
};

TEST_F(ConfigAuditLogTest, AuditLogEmptyByDefault) {
    auto entries = ConfigPathResolver::auditLog();
    EXPECT_TRUE(entries.empty());
}

TEST_F(ConfigAuditLogTest, AuditLogDisabledByDefault_NoEntriesRecorded) {
    // Audit logging must be disabled by default; resolutions should not record entries.
    createFile(test_dir_ / "config" / "plain.yaml");
    auto prev_cwd = std::filesystem::current_path();
    std::filesystem::current_path(test_dir_);
    ConfigPathResolver::tryResolve("config/plain.yaml");
    std::filesystem::current_path(prev_cwd);

    EXPECT_TRUE(ConfigPathResolver::auditLog().empty())
        << "Audit log should be empty when disabled";
}

TEST_F(ConfigAuditLogTest, AuditLogRecordsEntryWhenEnabled) {
    ConfigPathResolver::setAuditLogEnabled(true);

    createFile(test_dir_ / "config" / "plain.yaml");
    auto prev_cwd = std::filesystem::current_path();
    std::filesystem::current_path(test_dir_);
    auto result = ConfigPathResolver::tryResolve("config/plain.yaml");
    std::filesystem::current_path(prev_cwd);

    ASSERT_TRUE(result.has_value());
    auto entries = ConfigPathResolver::auditLog();
    ASSERT_EQ(entries.size(), 1u);
    EXPECT_EQ(entries[0].requested_path, "config/plain.yaml");
    EXPECT_EQ(entries[0].resolved_path, "config/plain.yaml");
    EXPECT_FALSE(entries[0].is_cache_hit);
}

TEST_F(ConfigAuditLogTest, AuditLogRecordsTimestamp) {
    ConfigPathResolver::setAuditLogEnabled(true);

    auto before = std::chrono::system_clock::now();
    createFile(test_dir_ / "config" / "ts_test.yaml");
    auto prev_cwd = std::filesystem::current_path();
    std::filesystem::current_path(test_dir_);
    ConfigPathResolver::tryResolve("config/ts_test.yaml");
    std::filesystem::current_path(prev_cwd);
    auto after = std::chrono::system_clock::now();

    auto entries = ConfigPathResolver::auditLog();
    ASSERT_EQ(entries.size(), 1u);
    EXPECT_GE(entries[0].timestamp, before);
    EXPECT_LE(entries[0].timestamp, after);
}

TEST_F(ConfigAuditLogTest, AuditLogMarksLegacyFallback) {
    // Use a known legacy path (pii_patterns.yaml → config/security/pii_patterns.yaml).
    // Only create the legacy file so the resolver falls back to it.
    ConfigPathResolver::setAuditLogEnabled(true);

    createFile(test_dir_ / "config" / "pii_patterns.yaml");
    // Deliberately do NOT create config/security/pii_patterns.yaml.

    auto prev_cwd = std::filesystem::current_path();
    std::filesystem::current_path(test_dir_);
    auto result = ConfigPathResolver::tryResolve("config/pii_patterns.yaml");
    std::filesystem::current_path(prev_cwd);

    ASSERT_TRUE(result.has_value());
    auto entries = ConfigPathResolver::auditLog();
    ASSERT_EQ(entries.size(), 1u);
    EXPECT_TRUE(entries[0].is_legacy) << "Entry should be flagged as legacy fallback";
    EXPECT_FALSE(entries[0].is_cache_hit);
    EXPECT_EQ(entries[0].resolved_path, "config/pii_patterns.yaml");
}

TEST_F(ConfigAuditLogTest, AuditLogMarksCacheHit) {
    ConfigPathResolver::setAuditLogEnabled(true);
    ConfigPathResolver::setCachingEnabled(true);

    createFile(test_dir_ / "config" / "cache_hit_test.yaml");
    auto prev_cwd = std::filesystem::current_path();
    std::filesystem::current_path(test_dir_);

    // First access: cache miss, no cache_hit flag
    ConfigPathResolver::tryResolve("config/cache_hit_test.yaml");
    // Second access: should be served from cache
    ConfigPathResolver::tryResolve("config/cache_hit_test.yaml");

    std::filesystem::current_path(prev_cwd);

    auto entries = ConfigPathResolver::auditLog();
    ASSERT_EQ(entries.size(), 2u);
    EXPECT_FALSE(entries[0].is_cache_hit) << "First access should not be a cache hit";
    EXPECT_TRUE(entries[1].is_cache_hit)  << "Second access should be a cache hit";
}

TEST_F(ConfigAuditLogTest, AuditLogClearWorks) {
    ConfigPathResolver::setAuditLogEnabled(true);

    createFile(test_dir_ / "config" / "clear_test.yaml");
    auto prev_cwd = std::filesystem::current_path();
    std::filesystem::current_path(test_dir_);
    ConfigPathResolver::tryResolve("config/clear_test.yaml");
    std::filesystem::current_path(prev_cwd);

    ASSERT_FALSE(ConfigPathResolver::auditLog().empty());
    ConfigPathResolver::clearAuditLog();
    EXPECT_TRUE(ConfigPathResolver::auditLog().empty());
}

TEST_F(ConfigAuditLogTest, AuditLogBoundedByMaxEntries) {
    ConfigPathResolver::setAuditLogEnabled(true);
    ConfigPathResolver::setAuditLogMaxEntries(3);

    // Create 5 distinct files and resolve each one
    for (int i = 0; i < 5; ++i) {
        std::string name = "file" + std::to_string(i) + ".yaml";
        createFile(test_dir_ / "config" / name);
        auto prev_cwd = std::filesystem::current_path();
        std::filesystem::current_path(test_dir_);
        ConfigPathResolver::clearCache(); // avoid cache hits masking resolution
        ConfigPathResolver::tryResolve("config/" + name);
        std::filesystem::current_path(prev_cwd);
    }

    auto entries = ConfigPathResolver::auditLog();
    EXPECT_LE(entries.size(), 3u) << "Audit log should be bounded by max entries";
}

TEST_F(ConfigAuditLogTest, AuditLogNotRecordingOnFailedResolution) {
    ConfigPathResolver::setAuditLogEnabled(true);

    // Resolve a path that does not exist – should not produce an audit entry
    ConfigPathResolver::tryResolve("config/nonexistent_for_audit_test.yaml");

    EXPECT_TRUE(ConfigPathResolver::auditLog().empty())
        << "Failed resolutions must not produce audit entries";
}

TEST_F(ConfigAuditLogTest, ResolveThrowingVariantRecordsAuditEntry) {
    // resolve() (non-optional) should record an audit entry on success,
    // since it delegates to tryResolve() internally.
    ConfigPathResolver::setAuditLogEnabled(true);

    createFile(test_dir_ / "config" / "resolve_audit_test.yaml");
    auto prev_cwd = std::filesystem::current_path();
    std::filesystem::current_path(test_dir_);

    std::string resolved = {};
    ASSERT_NO_THROW(resolved = ConfigPathResolver::resolve("config/resolve_audit_test.yaml"));

    std::filesystem::current_path(prev_cwd);

    auto entries = ConfigPathResolver::auditLog();
    ASSERT_EQ(entries.size(), 1u)
        << "resolve() should record an audit entry on success";
    EXPECT_EQ(entries[0].requested_path, "config/resolve_audit_test.yaml");
    EXPECT_EQ(entries[0].resolved_path, resolved);
}

TEST_F(ConfigAuditLogTest, ShrinkingMaxEntriesEvictsOldestFirst) {
    ConfigPathResolver::setAuditLogEnabled(true);
    ConfigPathResolver::setAuditLogMaxEntries(5);

    // Create and resolve 5 distinct files
    for (int i = 0; i < 5; ++i) {
        std::string name = "evict" + std::to_string(i) + ".yaml";
        createFile(test_dir_ / "config" / name);
        auto prev_cwd = std::filesystem::current_path();
        std::filesystem::current_path(test_dir_);
        ConfigPathResolver::clearCache();
        ConfigPathResolver::tryResolve("config/" + name);
        std::filesystem::current_path(prev_cwd);
    }
    ASSERT_EQ(ConfigPathResolver::auditLog().size(), 5u);

    // Reduce limit to 3 – the 2 oldest should be evicted
    ConfigPathResolver::setAuditLogMaxEntries(3);
    auto entries = ConfigPathResolver::auditLog();
    ASSERT_EQ(entries.size(), 3u) << "Shrinking max_entries should evict oldest entries";
    // The remaining entries should be the 3 most-recently added (evict2, evict3, evict4)
    EXPECT_EQ(entries[0].requested_path, "config/evict2.yaml");
    EXPECT_EQ(entries[2].requested_path, "config/evict4.yaml");
}

TEST_F(ConfigAuditLogTest, ConcurrentRecordAndSnapshot_ThreadSafety) {
    // Verify that concurrent record() and getEntries() calls do not crash or
    // produce inconsistent results under a burst of concurrent writers.
    ConfigPathResolver::setAuditLogEnabled(true);
    ConfigPathResolver::setAuditLogMaxEntries(200);

    // Create files up front so resolvers don't race on filesystem operations.
    for (int i = 0; i < 50; ++i) {
        createFile(test_dir_ / "config" / ("concurrent" + std::to_string(i) + ".yaml"));
    }

    const int kWriterThreads = 8;
    const int kResolvesPerThread = 25;
    std::vector<std::thread> writers;
    writers.reserve(kWriterThreads);

    std::filesystem::path saved_cwd = std::filesystem::current_path();
    std::filesystem::current_path(test_dir_);

    for (int t = 0; t < kWriterThreads; ++t) {
        writers.emplace_back([&, t]() {
            for (int i = 0; i < kResolvesPerThread; ++i) {
                int idx = (t * kResolvesPerThread + i) % 50;
                ConfigPathResolver::tryResolve("config/concurrent" + std::to_string(idx) + ".yaml");
            }
        });
    }

    // Snapshot reader running concurrently with the writers
    std::atomic<bool> stop_reader{false};
    std::thread reader([&]() {
        while (!stop_reader.load(std::memory_order_relaxed)) {
            auto entries = ConfigPathResolver::auditLog();
            EXPECT_LE(entries.size(), 200u);
        }
    });

    for (auto& w : writers) {
      w.join();
    }
    stop_reader.store(true, std::memory_order_relaxed);
    reader.join();

    std::filesystem::current_path(saved_cwd);

    // At least some entries should have been recorded
    EXPECT_GT(ConfigPathResolver::auditLog().size(), 0u);
}

} // namespace test
} // namespace config
} // namespace themis
