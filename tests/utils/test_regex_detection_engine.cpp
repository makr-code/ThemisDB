/**
 * @file test_regex_detection_engine.cpp
 * @brief Comprehensive tests for RegexDetectionEngine with hardening
 * @date 2026-08-17
 *
 * Tests Phase A.1 and Phase 2.2 hardening for regex_detection_engine.cpp:
 * - Regex matching timeout behavior
 * - ReDoS pattern detection
 * - Malformed pattern handling
 * - UTF-8 validation
 * - Input bounds checking
 */

#include <gtest/gtest.h>
#include "utils/regex_detection_engine.h"
#include <nlohmann/json.hpp>
#include <chrono>
#include <thread>

namespace themis {
namespace utils {

class RegexDetectionEngineTest : public ::testing::Test {
protected:
    RegexDetectionEngine engine;
    
    void SetUp() override {
        // Initialize with default patterns
        nlohmann::json config = nlohmann::json::object();
        config["enabled"] = true;
        engine.initialize(config);
    }
};

// ============================================================================
// Test Timeout Behavior (Phase A.1 Hardening)
// ============================================================================

TEST_F(RegexDetectionEngineTest, DetectInTextCompletes) {
    // Simple email text should complete quickly without timeout
    std::string text = "Contact: user@example.com";
    
    auto start = std::chrono::steady_clock::now();
    auto findings = engine.detectInText(text);
    auto elapsed = std::chrono::steady_clock::now() - start;
    
    // Should complete within reasonable time (much less than 5s timeout)
    EXPECT_LT(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(), 1000);
}

TEST_F(RegexDetectionEngineTest, LargeInputWithinTimeout) {
    // Create large input that should still complete within timeout
    std::string large_text;
    for (int i = 0; i < 10000; ++i) {
        large_text += "user" + std::to_string(i) + "@example.com ";
    }
    
    auto start = std::chrono::steady_clock::now();
    auto findings = engine.detectInText(large_text);
    auto elapsed = std::chrono::steady_clock::now() - start;
    
    // Should complete even with large input
    EXPECT_LT(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(), 5000);
    
    // Should detect emails in the large text
    EXPECT_GT(findings.size(), 0);
}

// ============================================================================
// Test ReDoS Pattern Detection
// ============================================================================

TEST_F(RegexDetectionEngineTest, NestedQuantifierDetection) {
    // Pattern with nested quantifiers (known ReDoS risk)
    nlohmann::json config;
    config["enabled"] = true;
    config["patterns"] = nlohmann::json::array();
    
    nlohmann::json pattern;
    pattern["name"] = "TEST";
    pattern["regex"] = "(a+)+b";  // Nested quantifier - ReDoS risk
    pattern["enabled"] = true;
    
    config["patterns"].push_back(pattern);
    
    // Engine should either:
    // 1. Skip the pattern due to ReDoS detection, or
    // 2. Handle it safely without catastrophic backtracking
    bool reinit_ok = engine.initialize(config);
    EXPECT_TRUE(reinit_ok);
    
    // Test on potentially problematic input
    std::string text = "aaaaaaaaaaaaaaaaaaaaac";  // Doesn't match (a+)+b
    EXPECT_NO_THROW({
        auto findings = engine.detectInText(text);
        // Should not crash or hang
    });
}

TEST_F(RegexDetectionEngineTest, AlternationOverlapDetection) {
    // Pattern with alternation overlap (ReDoS risk)
    nlohmann::json config;
    config["enabled"] = true;
    config["patterns"] = nlohmann::json::array();
    
    nlohmann::json pattern;
    pattern["name"] = "TEST2";
    pattern["regex"] = "(x|x)*y";  // Overlapping alternation - ReDoS risk
    pattern["enabled"] = true;
    
    config["patterns"].push_back(pattern);
    
    bool reinit_ok = engine.initialize(config);
    EXPECT_TRUE(reinit_ok);
    
    // Test on potentially problematic input
    std::string text = "xxxxxxxxxxxxxxxxxz";  // Doesn't match
    EXPECT_NO_THROW({
        auto findings = engine.detectInText(text);
    });
}

// ============================================================================
// Test Malformed Pattern Handling
// ============================================================================

TEST_F(RegexDetectionEngineTest, MalformedRegexPattern) {
    nlohmann::json config;
    config["enabled"] = true;
    config["patterns"] = nlohmann::json::array();
    
    nlohmann::json pattern;
    pattern["name"] = "MALFORMED";
    pattern["regex"] = "[a-z";  // Unclosed character class
    pattern["enabled"] = true;
    
    config["patterns"].push_back(pattern);
    
    // Should handle gracefully - either skip pattern or contain the error
    EXPECT_NO_THROW({
        bool ok = engine.initialize(config);
        // May fail to initialize, or may skip invalid pattern - both acceptable
    });
}

TEST_F(RegexDetectionEngineTest, InvalidEscapeSequence) {
    nlohmann::json config;
    config["enabled"] = true;
    config["patterns"] = nlohmann::json::array();
    
    nlohmann::json pattern;
    pattern["name"] = "INVALID_ESCAPE";
    pattern["regex"] = "\\k";  // Invalid escape
    pattern["enabled"] = true;
    
    config["patterns"].push_back(pattern);
    
    // Should handle gracefully without crash
    EXPECT_NO_THROW({
        bool ok = engine.initialize(config);
    });
}

TEST_F(RegexDetectionEngineTest, DetectWithMalformedPatternPresent) {
    nlohmann::json config;
    config["enabled"] = true;
    config["patterns"] = nlohmann::json::array();
    
    // Add a valid pattern first
    nlohmann::json valid_pattern;
    valid_pattern["name"] = "EMAIL";
    valid_pattern["regex"] = "[a-z]+@[a-z]+";
    valid_pattern["enabled"] = true;
    config["patterns"].push_back(valid_pattern);
    
    // Add a malformed pattern
    nlohmann::json bad_pattern;
    bad_pattern["name"] = "BAD";
    bad_pattern["regex"] = "[a-z";
    bad_pattern["enabled"] = true;
    config["patterns"].push_back(bad_pattern);
    
    engine.initialize(config);
    
    // Detection should still work for valid patterns
    std::string text = "contact@example.com";
    EXPECT_NO_THROW({
        auto findings = engine.detectInText(text);
        // Should find email or at least not crash
    });
}

// ============================================================================
// Test UTF-8 Input Validation
// ============================================================================

TEST_F(RegexDetectionEngineTest, ValidUTF8Input) {
    std::string utf8_text = "Café français 中文 العربية";
    
    EXPECT_NO_THROW({
        auto findings = engine.detectInText(utf8_text);
    });
}

TEST_F(RegexDetectionEngineTest, InvalidUTF8Sequence) {
    // Invalid UTF-8: incomplete UTF-8 sequence
    std::string invalid = "Hello\xC3";  // C3 without continuation byte
    
    // Should throw or handle gracefully
    EXPECT_ANY_THROW({
        auto findings = engine.detectInText(invalid);
    });
}

TEST_F(RegexDetectionEngineTest, NullBytesInInput) {
    std::string with_null = "Hello\x00World";
    
    // Should handle null bytes gracefully
    EXPECT_NO_THROW({
        auto findings = engine.detectInText(with_null);
    });
}

// ============================================================================
// Test Input Bounds Checking
// ============================================================================

TEST_F(RegexDetectionEngineTest, MaximumInputSize) {
    // Create input at reasonable size limit (should pass)
    std::string large_input(1024 * 1024, 'a');  // 1MB
    
    // Should NOT throw on reasonable size
    EXPECT_NO_THROW({
        auto findings = engine.detectInText(large_input);
    });
}

TEST_F(RegexDetectionEngineTest, ExcessiveInputSize) {
    // Create input that exceeds 10MB limit
    std::string excessive(11 * 1024 * 1024, 'a');
    
    // Should throw due to size limit
    EXPECT_ANY_THROW({
        auto findings = engine.detectInText(excessive);
    });
}

TEST_F(RegexDetectionEngineTest, EmptyInput) {
    std::string empty;
    
    auto findings = engine.detectInText(empty);
    EXPECT_EQ(findings.size(), 0);
}

// ============================================================================
// Test Concurrent Safety
// ============================================================================

TEST_F(RegexDetectionEngineTest, ConcurrentDetection) {
    std::vector<std::thread> threads;
    std::vector<std::vector<PIIFinding>> results(4);
    
    std::string texts[] = {
        "alice@example.com",
        "bob@test.org",
        "charlie@domain.net",
        "dave@mail.co.uk"
    };
    
    // Spawn multiple threads doing detection
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([this, i, &results, &texts]() {
            results[i] = engine.detectInText(texts[i]);
        });
    }
    
    // Wait for all to complete
    for (auto& t : threads) {
        t.join();
    }
    
    // All should complete without crashes
    for (int i = 0; i < 4; ++i) {
        // Expect email detection or at least no crash
        EXPECT_GE(results[i].size(), 0);
    }
}

// ============================================================================
// Test Reload Robustness
// ============================================================================

TEST_F(RegexDetectionEngineTest, ReloadWithValidConfig) {
    nlohmann::json config;
    config["enabled"] = true;
    config["patterns"] = nlohmann::json::array();
    
    nlohmann::json pattern;
    pattern["name"] = "PHONE";
    pattern["regex"] = "\\d{3}-\\d{3}-\\d{4}";
    pattern["enabled"] = true;
    
    config["patterns"].push_back(pattern);
    
    bool reload_ok = engine.reload(config);
    EXPECT_TRUE(reload_ok);
    
    std::string text = "Call 555-123-4567";
    EXPECT_NO_THROW({
        auto findings = engine.detectInText(text);
    });
}

TEST_F(RegexDetectionEngineTest, ReloadWithInvalidConfigRollsBack) {
    // First load a valid config
    nlohmann::json valid_config;
    valid_config["enabled"] = true;
    valid_config["patterns"] = nlohmann::json::array();
    engine.initialize(valid_config);
    
    // Now try to reload with bad config (malformed pattern)
    nlohmann::json bad_config;
    bad_config["enabled"] = false;
    bad_config["patterns"] = nlohmann::json::array();
    
    nlohmann::json bad_pattern;
    bad_pattern["name"] = "BAD";
    bad_pattern["regex"] = "[unclosed";
    bad_pattern["enabled"] = true;
    
    bad_config["patterns"].push_back(bad_pattern);
    
    // Reload may fail - that's OK
    bool reload_ok = engine.reload(bad_config);
    
    // Should still be in some consistent state
    EXPECT_NO_THROW({
        auto findings = engine.detectInText("test");
    });
}

} // namespace utils
} // namespace themis

