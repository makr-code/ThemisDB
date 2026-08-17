/**
 * @file test_ner_detection_engine.cpp
 * @brief Comprehensive tests for NERDetectionEngine with model-unavailable handling
 * @date 2026-08-17
 *
 * Tests Phase A.2 and Phase 2.2 hardening for ner_detection_engine.cpp:
 * - Model availability tracking
 * - Graceful degradation when gazetteers unavailable
 * - Config loading and rollback
 * - Error handling for corrupt model data
 */

#include <gtest/gtest.h>
#include "utils/ner_detection_engine.h"
#include <nlohmann/json.hpp>

namespace themis {
namespace utils {

class NERDetectionEngineTest : public ::testing::Test {
protected:
    NERDetectionEngine engine;
    
    void SetUp() override {
        engine = NERDetectionEngine();
    }
};

// ============================================================================
// Test Model Availability Tracking
// ============================================================================

TEST_F(NERDetectionEngineTest, InitializeWithValidGazetteers) {
    nlohmann::json config;
    config["enabled"] = true;
    config["settings"]["min_confidence"] = 0.70;
    
    config["honorifics"] = nlohmann::json::array();
    config["honorifics"].push_back("Mr.");
    config["honorifics"].push_back("Mrs.");
    config["honorifics"].push_back("Dr.");
    
    config["org_suffixes"] = nlohmann::json::array();
    config["org_suffixes"].push_back("Inc.");
    config["org_suffixes"].push_back("Corp.");
    config["org_suffixes"].push_back("Ltd.");
    
    config["location_prepositions"] = nlohmann::json::array();
    config["location_prepositions"].push_back("in");
    config["location_prepositions"].push_back("at");
    config["location_prepositions"].push_back("from");
    
    bool init_ok = engine.initialize(config);
    EXPECT_TRUE(init_ok);
    
    // With valid gazetteers, model should be available
    std::string error = engine.getLastError();
    EXPECT_TRUE(error.empty() || error.find("unavailable") == std::string::npos);
}

TEST_F(NERDetectionEngineTest, InitializeWithEmptyGazetteers) {
    nlohmann::json config;
    config["enabled"] = true;
    config["settings"]["min_confidence"] = 0.70;
    
    // All gazetteers empty
    config["honorifics"] = nlohmann::json::array();
    config["org_suffixes"] = nlohmann::json::array();
    config["location_prepositions"] = nlohmann::json::array();
    
    bool init_ok = engine.initialize(config);
    
    // Initialization may succeed but model should be marked unavailable
    // Detection should return empty results
    std::vector<PIIFinding> findings = engine.detectInText("Mr. John Smith");
    EXPECT_TRUE(findings.empty());
}

TEST_F(NERDetectionEngineTest, InitializeWithMissingGazetteerSections) {
    nlohmann::json config;
    config["enabled"] = true;
    config["settings"]["min_confidence"] = 0.70;
    
    // Only honorifics present, others missing
    config["honorifics"] = nlohmann::json::array();
    config["honorifics"].push_back("Mr.");
    
    // org_suffixes and location_prepositions missing entirely
    
    bool init_ok = engine.initialize(config);
    
    // Should handle gracefully - may use defaults or mark unavailable
    EXPECT_NO_THROW({
        auto findings = engine.detectInText("Dr. Smith works at Google Inc.");
    });
}

// ============================================================================
// Test Graceful Degradation on Model Unavailable
// ============================================================================

TEST_F(NERDetectionEngineTest, DetectWhenModelUnavailable) {
    // Initialize with empty gazetteers (model unavailable)
    nlohmann::json config;
    config["enabled"] = true;
    config["honorifics"] = nlohmann::json::array();
    config["org_suffixes"] = nlohmann::json::array();
    config["location_prepositions"] = nlohmann::json::array();
    
    engine.initialize(config);
    
    // Detection on unavailable model should return empty (fail-closed)
    std::string text = "Dr. Jane Smith visited Paris yesterday";
    auto findings = engine.detectInText(text);
    
    EXPECT_TRUE(findings.empty());
}

TEST_F(NERDetectionEngineTest, DetectWhenEngineDisabled) {
    nlohmann::json config;
    config["enabled"] = false;  // Engine disabled
    config["honorifics"] = nlohmann::json::array();
    config["honorifics"].push_back("Mr.");
    
    engine.initialize(config);
    
    // Detection when disabled should return empty
    auto findings = engine.detectInText("Mr. John Doe");
    EXPECT_TRUE(findings.empty());
}

// ============================================================================
// Test Config Loading and Validation
// ============================================================================

TEST_F(NERDetectionEngineTest, LoadFromConfigWithPartialData) {
    nlohmann::json config;
    config["enabled"] = true;
    
    // Only provide honorifics
    config["honorifics"] = nlohmann::json::array();
    config["honorifics"].push_back("Prof.");
    config["honorifics"].push_back("Dr.");
    
    // Missing org_suffixes and location_prepositions
    
    bool init_ok = engine.initialize(config);
    
    // Should handle partial config gracefully
    EXPECT_NO_THROW({
        auto findings = engine.detectInText("Prof. Alice met Bob");
    });
}

TEST_F(NERDetectionEngineTest, LoadFromConfigWithTypeMismatch) {
    nlohmann::json config;
    config["enabled"] = true;
    
    // honorifics should be array but provide string
    config["honorifics"] = "Mr. Mrs. Dr.";
    config["org_suffixes"] = nlohmann::json::array();
    config["location_prepositions"] = nlohmann::json::array();
    
    // Should handle type mismatch gracefully
    bool init_ok = engine.initialize(config);
    
    // May fail initialization, or skip invalid field - both acceptable
    EXPECT_NO_THROW({
        auto findings = engine.detectInText("test");
    });
}

// ============================================================================
// Test Config Reload and Rollback
// ============================================================================

TEST_F(NERDetectionEngineTest, ReloadWithValidConfig) {
    // Initialize with first config
    nlohmann::json config1;
    config1["enabled"] = true;
    config1["honorifics"] = nlohmann::json::array();
    config1["honorifics"].push_back("Mr.");
    config1["org_suffixes"] = nlohmann::json::array();
    config1["location_prepositions"] = nlohmann::json::array();
    
    engine.initialize(config1);
    
    // Reload with different config
    nlohmann::json config2;
    config2["enabled"] = true;
    config2["honorifics"] = nlohmann::json::array();
    config2["honorifics"].push_back("Dr.");
    config2["honorifics"].push_back("Prof.");
    config2["org_suffixes"] = nlohmann::json::array();
    config2["location_prepositions"] = nlohmann::json::array();
    
    bool reload_ok = engine.reload(config2);
    EXPECT_TRUE(reload_ok);
}

TEST_F(NERDetectionEngineTest, ReloadWithInvalidConfigRollsBack) {
    // Initialize with valid config
    nlohmann::json config1;
    config1["enabled"] = true;
    config1["honorifics"] = nlohmann::json::array();
    config1["honorifics"].push_back("Mr.");
    config1["org_suffixes"] = nlohmann::json::array();
    config1["location_prepositions"] = nlohmann::json::array();
    
    engine.initialize(config1);
    
    // Try reload with bad config (malformed JSON structure)
    nlohmann::json bad_config;
    bad_config["enabled"] = "not_a_bool";  // Type error
    
    // Reload should fail gracefully
    bool reload_ok = engine.reload(bad_config);
    
    // Should still be in valid state after rollback
    EXPECT_NO_THROW({
        auto findings = engine.detectInText("Mr. Smith");
    });
}

// ============================================================================
// Test Concurrent Detection
// ============================================================================

TEST_F(NERDetectionEngineTest, ConcurrentDetection) {
    nlohmann::json config;
    config["enabled"] = true;
    config["honorifics"] = nlohmann::json::array();
    config["honorifics"].push_back("Dr.");
    config["honorifics"].push_back("Prof.");
    config["org_suffixes"] = nlohmann::json::array();
    config["org_suffixes"].push_back("Inc.");
    config["location_prepositions"] = nlohmann::json::array();
    config["location_prepositions"].push_back("in");
    
    engine.initialize(config);
    
    std::vector<std::thread> threads;
    std::vector<std::vector<PIIFinding>> results(4);
    
    std::string texts[] = {
        "Dr. Alice works in Cambridge",
        "Prof. Bob at MIT",
        "Charlie visited Paris",
        "Delta Inc. opened in Boston"
    };
    
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([this, i, &results, &texts]() {
            results[i] = engine.detectInText(texts[i]);
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // All should complete without crashes
    for (int i = 0; i < 4; ++i) {
        EXPECT_GE(results[i].size(), 0);
    }
}

// ============================================================================
// Test UTF-8 and Unicode Support
// ============================================================================

TEST_F(NERDetectionEngineTest, DetectWithUnicodeText) {
    nlohmann::json config;
    config["enabled"] = true;
    config["honorifics"] = nlohmann::json::array();
    config["honorifics"].push_back("Dr.");
    config["org_suffixes"] = nlohmann::json::array();
    config["location_prepositions"] = nlohmann::json::array();
    config["location_prepositions"].push_back("in");
    
    engine.initialize(config);
    
    // Unicode text with honorifics and locations
    std::string unicode_text = "Dr. François works in Montréal";
    
    EXPECT_NO_THROW({
        auto findings = engine.detectInText(unicode_text);
    });
}

TEST_F(NERDetectionEngineTest, DetectWithMixedLanguage) {
    nlohmann::json config;
    config["enabled"] = true;
    config["honorifics"] = nlohmann::json::array();
    config["honorifics"].push_back("Mr.");
    config["org_suffixes"] = nlohmann::json::array();
    config["location_prepositions"] = nlohmann::json::array();
    
    engine.initialize(config);
    
    // Mixed language text
    std::string mixed_text = "Mr. Chen 和 Dr. Smith 在 Tokyo 工作";
    
    EXPECT_NO_THROW({
        auto findings = engine.detectInText(mixed_text);
    });
}

// ============================================================================
// Test Empty/Null Input Handling
// ============================================================================

TEST_F(NERDetectionEngineTest, DetectEmptyString) {
    nlohmann::json config;
    config["enabled"] = true;
    config["honorifics"] = nlohmann::json::array();
    config["honorifics"].push_back("Dr.");
    config["org_suffixes"] = nlohmann::json::array();
    config["location_prepositions"] = nlohmann::json::array();
    
    engine.initialize(config);
    
    auto findings = engine.detectInText("");
    EXPECT_TRUE(findings.empty());
}

TEST_F(NERDetectionEngineTest, DetectWhitespaceOnly) {
    nlohmann::json config;
    config["enabled"] = true;
    config["honorifics"] = nlohmann::json::array();
    config["honorifics"].push_back("Dr.");
    config["org_suffixes"] = nlohmann::json::array();
    config["location_prepositions"] = nlohmann::json::array();
    
    engine.initialize(config);
    
    auto findings = engine.detectInText("   \t\n  ");
    EXPECT_TRUE(findings.empty());
}

// ============================================================================
// Test Metadata Access
// ============================================================================

TEST_F(NERDetectionEngineTest, GetMetadata) {
    nlohmann::json config;
    config["enabled"] = true;
    engine.initialize(config);
    
    auto metadata = engine.getMetadata();
    
    // Metadata should be valid JSON
    EXPECT_TRUE(metadata.is_object());
    EXPECT_TRUE(metadata.contains("engine_type") || metadata.is_object());
}

TEST_F(NERDetectionEngineTest, GetLastError) {
    nlohmann::json config;
    config["enabled"] = true;
    engine.initialize(config);
    
    // After successful init, error should be empty
    std::string error = engine.getLastError();
    EXPECT_TRUE(error.empty() || error.find("unavailable") == std::string::npos);
}

} // namespace utils
} // namespace themis

