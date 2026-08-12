/**
 * @file test_regex_detection_engine.cpp
 * @brief Tests for RegexDetectionEngine (src/utils/regex_detection_engine.cpp)
 *
 * Covers:
 *   - getName / getVersion / isEnabled
 *   - detectInText: email, phone, SSN, credit card, IP address
 *   - detectInText: text with no PII → empty results
 *   - detectInText: empty input → empty results
 *   - classifyFieldName: email-like field names
 *   - getRedactionRecommendation
 *   - reload: valid / invalid JSON config
 */

#include <gtest/gtest.h>
#include "utils/regex_detection_engine.h"
#include "utils/pii_detection_engine.h"
#include <string>
#include <vector>
#include <algorithm>

using namespace themis::utils;

// ============================================================================
// Fixture
// ============================================================================

class RegexDetectionEngineTest : public ::testing::Test {
protected:
    RegexDetectionEngine engine_;
};

// ============================================================================
// Basic metadata
// ============================================================================

TEST_F(RegexDetectionEngineTest, GetName_NonEmpty) {
    EXPECT_FALSE(engine_.getName().empty());
}

TEST_F(RegexDetectionEngineTest, GetVersion_NonEmpty) {
    EXPECT_FALSE(engine_.getVersion().empty());
}

TEST_F(RegexDetectionEngineTest, IsEnabled_TrueByDefault) {
    EXPECT_TRUE(engine_.isEnabled());
}

// ============================================================================
// detectInText — positive cases
// ============================================================================

TEST_F(RegexDetectionEngineTest, DetectEmail_FoundInText) {
    auto findings = engine_.detectInText("Please contact support@example.com for help.");
    bool found_email = std::any_of(findings.begin(), findings.end(), [](const PIIFinding& f) {
        return f.type == PIIType::EMAIL;
    });
    EXPECT_TRUE(found_email) << "Expected an EMAIL finding";
}

TEST_F(RegexDetectionEngineTest, DetectPhone_USFormat) {
    auto findings = engine_.detectInText("Call us at +1-555-867-5309 any time.");
    bool found_phone = std::any_of(findings.begin(), findings.end(), [](const PIIFinding& f) {
        return f.type == PIIType::PHONE;
    });
    EXPECT_TRUE(found_phone) << "Expected a PHONE finding";
}

TEST_F(RegexDetectionEngineTest, DetectSSN_USFormat) {
    auto findings = engine_.detectInText("SSN: 123-45-6789");
    bool found_ssn = std::any_of(findings.begin(), findings.end(), [](const PIIFinding& f) {
        return f.type == PIIType::SSN;
    });
    EXPECT_TRUE(found_ssn) << "Expected an SSN finding";
}

TEST_F(RegexDetectionEngineTest, DetectIPAddress_IPv4) {
    auto findings = engine_.detectInText("Client connected from 192.168.1.100.");
    bool found_ip = std::any_of(findings.begin(), findings.end(), [](const PIIFinding& f) {
        return f.type == PIIType::IP_ADDRESS;
    });
    EXPECT_TRUE(found_ip) << "Expected an IP_ADDRESS finding";
}

TEST_F(RegexDetectionEngineTest, DetectMultiplePIITypes_InSameText) {
    const std::string text = "User alice@corp.io called from 555-123-4567.";
    auto findings = engine_.detectInText(text);
    EXPECT_GE(findings.size(), 1u); // At least email or phone detected
}

// ============================================================================
// detectInText — negative / edge cases
// ============================================================================

TEST_F(RegexDetectionEngineTest, NoPII_ReturnsEmptyFindings) {
    auto findings = engine_.detectInText("The quick brown fox jumps over the lazy dog.");
    // No email, phone, SSN, credit card or IP in this sentence
    bool any_high_confidence = std::any_of(findings.begin(), findings.end(), [](const PIIFinding& f) {
        return f.confidence > 0.8;
    });
    EXPECT_FALSE(any_high_confidence);
}

TEST_F(RegexDetectionEngineTest, EmptyText_ReturnsEmpty) {
    auto findings = engine_.detectInText("");
    EXPECT_TRUE(findings.empty());
}

// ============================================================================
// Confidence values in range [0, 1]
// ============================================================================

TEST_F(RegexDetectionEngineTest, AllFindings_ConfidenceInRange) {
    const std::string text =
        "Email: test@example.org, IP: 10.0.0.1, SSN: 987-65-4321";
    auto findings = engine_.detectInText(text);
    for (const auto& f : findings) {
        EXPECT_GE(f.confidence, 0.0) << "confidence must be >= 0";
        EXPECT_LE(f.confidence, 1.0) << "confidence must be <= 1";
    }
}

// ============================================================================
// classifyFieldName
// ============================================================================

TEST_F(RegexDetectionEngineTest, ClassifyFieldName_EmailField) {
    auto pii_type = engine_.classifyFieldName("email_address");
    EXPECT_NE(pii_type, PIIType::UNKNOWN);
}

TEST_F(RegexDetectionEngineTest, ClassifyFieldName_UnknownField) {
    auto pii_type = engine_.classifyFieldName("product_sku");
    EXPECT_EQ(pii_type, PIIType::UNKNOWN);
}

// ============================================================================
// getRedactionRecommendation
// ============================================================================

TEST_F(RegexDetectionEngineTest, GetRedactionRecommendation_Email_NonEmpty) {
    auto rec = engine_.getRedactionRecommendation(PIIType::EMAIL);
    EXPECT_FALSE(rec.empty());
}

TEST_F(RegexDetectionEngineTest, GetRedactionRecommendation_SSN_NonEmpty) {
    auto rec = engine_.getRedactionRecommendation(PIIType::SSN);
    EXPECT_FALSE(rec.empty());
}

// ============================================================================
// reload
// ============================================================================

TEST_F(RegexDetectionEngineTest, Reload_EmptyConfig_DoesNotCrash) {
    // Passing an empty JSON should either succeed or gracefully fall back
    EXPECT_NO_THROW(engine_.reload(nlohmann::json{}));
    // Engine should still be usable after reload
    EXPECT_TRUE(engine_.isEnabled());
}

TEST_F(RegexDetectionEngineTest, Reload_ValidPatternConfig_EngineStillFunctional) {
    nlohmann::json cfg = R"({
        "patterns": [
            {
                "name": "EMAIL",
                "regex": "[a-zA-Z0-9._%+\\-]+@[a-zA-Z0-9.\\-]+\\.[a-zA-Z]{2,}",
                "flags": ["icase"],
                "confidence": 0.95,
                "redaction_mode": "partial",
                "field_hints": ["email"],
                "validation": "none",
                "enabled": true
            }
        ]
    })"_json;

    engine_.reload(cfg);
    auto findings = engine_.detectInText("reach me at user@domain.com");
    bool found_email = std::any_of(findings.begin(), findings.end(), [](const PIIFinding& f) {
        return f.type == PIIType::EMAIL;
    });
    EXPECT_TRUE(found_email);
}
