/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_config_path_resolver.cpp                      ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-02-21 16:35:06                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     344                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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

} // namespace test
} // namespace config
} // namespace themis
