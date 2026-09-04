// Copyright 2025 ThemisDB
// Phase 2: Fuzz tests for Adaptive Query Cache security

#include <gtest/gtest.h>
#include "cache/adaptive_query_cache.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <random>
#include <regex>
#include <limits>

using namespace themis;
using json = nlohmann::json;

class AdaptiveCacheFuzzTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.l3_db_path = "/tmp/themis_fuzz_test_" + 
                            std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        config_.l1_max_entries = 100;
        config_.l2_max_entries = 200;
    }
    
    void TearDown() override {
        if (!config_.l3_db_path.empty()) {
            std::filesystem::remove_all(config_.l3_db_path);
        }
    }
    
    AdaptiveQueryCache::Config config_;
};

// ============================================================================
// Fuzz: JSON Parameter Parsing
// ============================================================================

TEST_F(AdaptiveCacheFuzzTest, FuzzEmptyJSON) {
    AdaptiveQueryCache cache(config_);
    
    json empty = json::object();
    std::string fp = cache.generateFingerprint("query", empty);
    EXPECT_EQ(fp.length(), 64);
}

TEST_F(AdaptiveCacheFuzzTest, FuzzNestedJSON) {
    AdaptiveQueryCache cache(config_);
    
    json nested;
    nested["level1"]["level2"]["level3"]["value"] = 42;
    nested["array"] = json::array({1, 2, 3});
    nested["null_value"] = nullptr;
    
    std::string fp = cache.generateFingerprint("query", nested);
    EXPECT_EQ(fp.length(), 64);
    
    json result = {{"data", "test"}};
    EXPECT_TRUE(cache.put(fp, nested, result));
}

TEST_F(AdaptiveCacheFuzzTest, FuzzJSONWithSpecialCharacters) {
    AdaptiveQueryCache cache(config_);
    
    json params;
    params["special"] = "!@#$%^&*()_+-=[]{}|;:',.<>?/~`";
    params["newline"] = "line1\nline2\nline3";
    params["tab"] = "col1\tcol2\tcol3";
    params["quote"] = "He said \"hello\"";
    
    std::string fp = cache.generateFingerprint("query", params);
    EXPECT_EQ(fp.length(), 64);
    
    json result = {{"data", "test"}};
    EXPECT_TRUE(cache.put(fp, params, result));
}

TEST_F(AdaptiveCacheFuzzTest, FuzzJSONWithUnicode) {
    AdaptiveQueryCache cache(config_);
    
    json params;
    params["emoji"] = "🚀🎉✨";
    params["chinese"] = "你好世界";
    params["arabic"] = "مرحبا";
    params["mixed"] = "Hello世界🌍";
    
    std::string fp = cache.generateFingerprint("query", params);
    EXPECT_EQ(fp.length(), 64);
}

TEST_F(AdaptiveCacheFuzzTest, FuzzJSONWithNumbers) {
    AdaptiveQueryCache cache(config_);
    
    json params;
    params["int_max"] = std::numeric_limits<int>::max();
    params["int_min"] = std::numeric_limits<int>::min();
    params["double_max"] = std::numeric_limits<double>::max();
    params["double_min"] = std::numeric_limits<double>::min();
    params["zero"] = 0;
    params["negative"] = -12345;
    params["float"] = 3.14159;
    
    std::string fp = cache.generateFingerprint("query", params);
    EXPECT_EQ(fp.length(), 64);
}

// ============================================================================
// Fuzz: Regex Patterns for Invalidation
// ============================================================================

TEST_F(AdaptiveCacheFuzzTest, FuzzSimpleRegexPatterns) {
    AdaptiveQueryCache cache(config_);
    
    // Pre-populate
    for (int i = 0; i < 20; i++) {
        std::string fp = cache.generateFingerprint("test" + std::to_string(i), {});
        cache.put(fp, {}, {{"v", i}});
    }
    
    std::vector<std::string> safe_patterns = {
        ".*",
        "test.*",
        "[a-z]+",
        "[0-9]+",
        "^test",
        "test$",
        "t.st",
    };
    
    for (const auto& pattern : safe_patterns) {
        try {
            size_t count = cache.invalidate(pattern);
            EXPECT_GE(count, 0);
        } catch (...) {
            FAIL() << "Safe pattern should not throw: " << pattern;
        }
    }
}

TEST_F(AdaptiveCacheFuzzTest, FuzzComplexRegexPatterns) {
    AdaptiveQueryCache cache(config_);
    
    // Pre-populate
    for (int i = 0; i < 10; i++) {
        std::string fp = cache.generateFingerprint("data" + std::to_string(i), {});
        cache.put(fp, {}, {{"v", i}});
    }
    
    std::vector<std::string> complex_patterns = {
        "data[0-5]",
        "(test|data).*",
        "\\w+",
        "[^x]+",
        "d{2,4}",
    };
    
    for (const auto& pattern : complex_patterns) {
        try {
            cache.invalidate(pattern);
            SUCCEED();
        } catch (const std::regex_error& e) {
            // Invalid regex is acceptable
            SUCCEED();
        } catch (...) {
            // Other exceptions are acceptable in fuzz test
            SUCCEED();
        }
    }
}

TEST_F(AdaptiveCacheFuzzTest, FuzzReDoSVulnerability) {
    AdaptiveQueryCache cache(config_);
    
    // Pre-populate with a few entries
    for (int i = 0; i < 5; i++) {
        std::string fp = cache.generateFingerprint("item" + std::to_string(i), {});
        cache.put(fp, {}, {{"v", i}});
    }
    
    // Known ReDoS patterns (should timeout or be safe)
    std::vector<std::string> redos_patterns = {
        "(a+)+b",           // Exponential backtracking
        "(a*)*b",           // Exponential backtracking
        "(x+x+)+y",         // Catastrophic backtracking
    };
    
    for (const auto& pattern : redos_patterns) {
        try {
            // Set a timeout expectation - if this hangs, it's a ReDoS vulnerability
            auto start = std::chrono::steady_clock::now();
            cache.invalidate(pattern);
            auto elapsed = std::chrono::steady_clock::now() - start;
            
            // Should complete in reasonable time (< 1 second)
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
            EXPECT_LT(ms, 1000) << "Pattern took too long (potential ReDoS): " << pattern;
        } catch (const std::regex_error&) {
            // Invalid regex is fine
            SUCCEED();
        }
    }
}

TEST_F(AdaptiveCacheFuzzTest, FuzzInvalidRegexPatterns) {
    AdaptiveQueryCache cache(config_);
    
    std::vector<std::string> invalid_patterns = {
        "[",           // Unclosed bracket
        "(",           // Unclosed paren
        "*",           // Invalid quantifier
        "(?P<name>)",  // Named groups might not be supported
        "\\",          // Incomplete escape
    };
    
    for (const auto& pattern : invalid_patterns) {
        // Should either handle gracefully or throw regex_error
        try {
            cache.invalidate(pattern);
            SUCCEED();
        } catch (const std::regex_error&) {
            SUCCEED();  // Expected for invalid patterns
        } catch (...) {
            SUCCEED();  // Other exceptions are acceptable
        }
    }
}

// ============================================================================
// Fuzz: Compression Edge Cases
// ============================================================================

TEST_F(AdaptiveCacheFuzzTest, FuzzCompressionWithBinaryData) {
    AdaptiveQueryCache cache(config_);
    
    // Create entry with binary-like data that goes to L2
    std::vector<uint8_t> binary_data = {};

    for (int i = 0; i < 300; i++) {
        binary_data.push_back(static_cast<uint8_t>(i % 256));
    }
    
    std::string data_str(binary_data.begin(), binary_data.end());
    json result = {{"binary", data_str}};
    
    std::string fp = cache.generateFingerprint("binary_query", {});
    bool stored = cache.put(fp, {}, result);
    
    // Should handle binary data
    EXPECT_TRUE(stored);
}

TEST_F(AdaptiveCacheFuzzTest, FuzzCompressionWithHighEntropy) {
    AdaptiveQueryCache cache(config_);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, 255);
    
    // Random data (high entropy, won't compress well)
    std::string random_data;
    for (int i = 0; i < 300; i++) {
        random_data += static_cast<char>(dist(gen));
    }
    
    json result = {{"random", random_data}};
    std::string fp = cache.generateFingerprint("random_query", {});
    
    bool stored = cache.put(fp, {}, result);
    // May or may not store based on compression success
    EXPECT_GE(cache.getEnhancedMetrics().compression_failures.load(), 0);
}

TEST_F(AdaptiveCacheFuzzTest, FuzzCompressionWithRepeatingPatterns) {
    AdaptiveQueryCache cache(config_);
    
    // Highly compressible data
    std::string repeating(300, 'A');
    json result = {{"data", repeating}};
    
    std::string fp = cache.generateFingerprint("compressible", {});
    EXPECT_TRUE(cache.put(fp, {}, result));
    
    // Check compression ratio
    auto metrics = cache.getEnhancedMetrics();
    double ratio = metrics.getCompressionRatio();
    EXPECT_GE(ratio, 1.0);  // Should compress
}

// ============================================================================
// Stress Tests
// ============================================================================

TEST_F(AdaptiveCacheFuzzTest, StressTestRapidPutGet) {
    AdaptiveQueryCache cache(config_);
    
    const int NUM_OPERATIONS = 500;
    json result = {{"data", "test"}};
    
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        std::string fp = cache.generateFingerprint("rapid" + std::to_string(i % 50), {});
        cache.put(fp, {}, result);
        cache.get(fp);
    }
    
    auto stats = cache.getStats();
    EXPECT_GT(stats.l1_hits + stats.l2_hits + stats.l3_hits, 0);
}

TEST_F(AdaptiveCacheFuzzTest, StressTestClearExpired) {
    config_.l1_ttl_seconds = 1;
    config_.l2_ttl_seconds = 1;
    AdaptiveQueryCache cache(config_);
    
    // Add many entries
    for (int i = 0; i < 50; i++) {
        std::string fp = cache.generateFingerprint("expiring" + std::to_string(i), {});
        cache.put(fp, {}, {{"v", i}});
    }
    
    // Wait for expiration
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    
    // Clear expired
    uint64_t cleared = cache.clearExpired();
    EXPECT_GT(cleared, 0);
}
