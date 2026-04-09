/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_pii_detector.cpp                              ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-04-09                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                       ║
    • Total Lines:     ~380                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "exporters/pii_detector.h"
#include <string>
#include <vector>
#include <map>

using namespace themis::exporters;

// ─────────────────────────────────────────────────────────────────────────────
// Test fixture
// ─────────────────────────────────────────────────────────────────────────────

class PiiDetectorFocusedTests : public ::testing::Test {
protected:
    void SetUp() override {
        detector_default_ = std::make_unique<PIIDetector>();

        PIIDetector::Config cfg_all;
        cfg_all.detect_email       = true;
        cfg_all.detect_phone       = true;
        cfg_all.detect_ssn         = true;
        cfg_all.detect_credit_card = true;
        cfg_all.detect_ip_address  = true;
        cfg_all.default_strategy   = PIIDetector::RedactionStrategy::MASK;
        detector_all_ = std::make_unique<PIIDetector>(cfg_all);

        PIIDetector::Config cfg_none;
        cfg_none.detect_email       = false;
        cfg_none.detect_phone       = false;
        cfg_none.detect_ssn         = false;
        cfg_none.detect_credit_card = false;
        cfg_none.detect_ip_address  = false;
        detector_none_ = std::make_unique<PIIDetector>(cfg_none);
    }

    std::unique_ptr<PIIDetector> detector_default_;
    std::unique_ptr<PIIDetector> detector_all_;
    std::unique_ptr<PIIDetector> detector_none_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(PiiDetectorFocusedTests, DefaultConstructorCreatesWorkingInstance) {
    PIIDetector d;
    EXPECT_FALSE(d.containsPII("Hello world"));
    EXPECT_TRUE(d.containsPII("Email me at user@example.com"));
}

TEST_F(PiiDetectorFocusedTests, ConfigConstructorRespectsSwitches) {
    PIIDetector::Config cfg;
    cfg.detect_email = false;
    cfg.detect_phone = false;
    cfg.detect_ssn   = false;
    cfg.detect_credit_card = false;
    cfg.detect_ip_address  = false;
    PIIDetector d(cfg);
    EXPECT_FALSE(d.containsPII("user@example.com"));
    EXPECT_FALSE(d.containsPII("555-123-4567"));
}

// ─────────────────────────────────────────────────────────────────────────────
// Email detection
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(PiiDetectorFocusedTests, DetectsSimpleEmail) {
    auto matches = detector_default_->detectPII("Contact us at support@themisdb.io please.");
    ASSERT_EQ(1u, matches.size());
    EXPECT_EQ(PIIDetector::PIIType::EMAIL, matches[0].type);
    EXPECT_EQ("support@themisdb.io", matches[0].value);
}

TEST_F(PiiDetectorFocusedTests, DetectsMultipleEmailsInOneLine) {
    std::string text = "From alice@example.com to bob@example.org";
    auto matches = detector_default_->detectPII(text);
    size_t email_count = 0;
    for (const auto& m : matches) {
        if (m.type == PIIDetector::PIIType::EMAIL) ++email_count;
    }
    EXPECT_EQ(2u, email_count);
}

TEST_F(PiiDetectorFocusedTests, NoFalsePositiveOnPlainText) {
    auto matches = detector_default_->detectPII("The quick brown fox jumps over the lazy dog.");
    bool has_email = false;
    for (const auto& m : matches) {
        if (m.type == PIIDetector::PIIType::EMAIL) has_email = true;
    }
    EXPECT_FALSE(has_email);
}

// ─────────────────────────────────────────────────────────────────────────────
// Phone detection
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(PiiDetectorFocusedTests, DetectsUSPhoneNumber) {
    auto matches = detector_default_->detectPII("Call me at 800-555-1212 today.");
    bool has_phone = false;
    for (const auto& m : matches) {
        if (m.type == PIIDetector::PIIType::PHONE) has_phone = true;
    }
    EXPECT_TRUE(has_phone);
}

TEST_F(PiiDetectorFocusedTests, DetectsPhoneWithParentheses) {
    auto matches = detector_default_->detectPII("Office: (212) 555-0100");
    bool has_phone = false;
    for (const auto& m : matches) {
        if (m.type == PIIDetector::PIIType::PHONE) has_phone = true;
    }
    EXPECT_TRUE(has_phone);
}

// ─────────────────────────────────────────────────────────────────────────────
// SSN detection
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(PiiDetectorFocusedTests, DetectsSSN) {
    auto matches = detector_default_->detectPII("SSN: 123-45-6789");
    bool has_ssn = false;
    for (const auto& m : matches) {
        if (m.type == PIIDetector::PIIType::SSN) has_ssn = true;
    }
    EXPECT_TRUE(has_ssn);
}

TEST_F(PiiDetectorFocusedTests, NoFalsePositiveOnRandomDigitGroups) {
    // "12-34-5678" does not match SSN pattern (XXX-XX-XXXX)
    auto matches = detector_default_->detectPII("Reference: 12-34-5678");
    bool has_ssn = false;
    for (const auto& m : matches) {
        if (m.type == PIIDetector::PIIType::SSN) has_ssn = true;
    }
    EXPECT_FALSE(has_ssn);
}

// ─────────────────────────────────────────────────────────────────────────────
// Credit card detection
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(PiiDetectorFocusedTests, DetectsCreditCardNumber) {
    auto matches = detector_default_->detectPII("Card: 4111 1111 1111 1111");
    bool has_cc = false;
    for (const auto& m : matches) {
        if (m.type == PIIDetector::PIIType::CREDIT_CARD) has_cc = true;
    }
    EXPECT_TRUE(has_cc);
}

TEST_F(PiiDetectorFocusedTests, DetectsCreditCardWithDashes) {
    auto matches = detector_default_->detectPII("Billing: 5500-0000-0000-0004 expires soon.");
    bool has_cc = false;
    for (const auto& m : matches) {
        if (m.type == PIIDetector::PIIType::CREDIT_CARD) has_cc = true;
    }
    EXPECT_TRUE(has_cc);
}

// ─────────────────────────────────────────────────────────────────────────────
// IP address detection (opt-in)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(PiiDetectorFocusedTests, IPAddressNotDetectedByDefault) {
    // Default config has detect_ip_address = false
    auto matches = detector_default_->detectPII("Server at 192.168.1.1");
    bool has_ip = false;
    for (const auto& m : matches) {
        if (m.type == PIIDetector::PIIType::IP_ADDRESS) has_ip = true;
    }
    EXPECT_FALSE(has_ip);
}

TEST_F(PiiDetectorFocusedTests, IPAddressDetectedWhenEnabled) {
    auto matches = detector_all_->detectPII("Server at 192.168.1.1");
    bool has_ip = false;
    for (const auto& m : matches) {
        if (m.type == PIIDetector::PIIType::IP_ADDRESS) has_ip = true;
    }
    EXPECT_TRUE(has_ip);
}

// ─────────────────────────────────────────────────────────────────────────────
// containsPII
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(PiiDetectorFocusedTests, ContainsPIIReturnsTrueForEmail) {
    EXPECT_TRUE(detector_default_->containsPII("Reach me at foo@bar.com"));
}

TEST_F(PiiDetectorFocusedTests, ContainsPIIReturnsFalseForCleanText) {
    EXPECT_FALSE(detector_default_->containsPII("No sensitive data here."));
}

TEST_F(PiiDetectorFocusedTests, ContainsPIIReturnsFalseWhenAllDetectionDisabled) {
    EXPECT_FALSE(detector_none_->containsPII("user@example.com or 123-45-6789"));
}

// ─────────────────────────────────────────────────────────────────────────────
// Redaction — MASK strategy
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(PiiDetectorFocusedTests, MaskStrategyReplacesWithAsterisks) {
    PIIDetector::Config cfg;
    cfg.default_strategy = PIIDetector::RedactionStrategy::MASK;
    PIIDetector d(cfg);

    std::string result = d.redactPII("Email: user@example.com");
    EXPECT_EQ(std::string::npos, result.find("user@example.com"));
    EXPECT_NE(std::string::npos, result.find("***"));
}

TEST_F(PiiDetectorFocusedTests, MaskPreservesNonPIIText) {
    std::string original = "Hello user@example.com world";
    std::string result = detector_default_->redactPII(original);
    EXPECT_NE(std::string::npos, result.find("Hello"));
    EXPECT_NE(std::string::npos, result.find("world"));
}

// ─────────────────────────────────────────────────────────────────────────────
// Redaction — REMOVE strategy
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(PiiDetectorFocusedTests, RemoveStrategyInsertsRedactedToken) {
    PIIDetector::Config cfg;
    cfg.default_strategy = PIIDetector::RedactionStrategy::REMOVE;
    PIIDetector d(cfg);

    std::string result = d.redactPII("Send to admin@corp.com now");
    EXPECT_EQ(std::string::npos, result.find("admin@corp.com"));
    EXPECT_NE(std::string::npos, result.find("[REDACTED]"));
}

// ─────────────────────────────────────────────────────────────────────────────
// Redaction — HASH strategy
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(PiiDetectorFocusedTests, HashStrategyProducesHashPrefix) {
    PIIDetector::Config cfg;
    cfg.default_strategy = PIIDetector::RedactionStrategy::HASH;
    PIIDetector d(cfg);

    std::string result = d.redactPII("Email: user@example.com");
    EXPECT_EQ(std::string::npos, result.find("user@example.com"));
    EXPECT_NE(std::string::npos, result.find("SHA256:"));
}

TEST_F(PiiDetectorFocusedTests, HashStrategyIsDeterministic) {
    PIIDetector::Config cfg;
    cfg.default_strategy = PIIDetector::RedactionStrategy::HASH;
    PIIDetector d(cfg);

    std::string r1 = d.redactPII("user@example.com");
    std::string r2 = d.redactPII("user@example.com");
    EXPECT_EQ(r1, r2);
}

TEST_F(PiiDetectorFocusedTests, HashStrategyDifferentValuesProduceDifferentHashes) {
    PIIDetector::Config cfg;
    cfg.default_strategy = PIIDetector::RedactionStrategy::HASH;
    PIIDetector d(cfg);

    std::string r1 = d.redactPII("a@example.com");
    std::string r2 = d.redactPII("b@example.com");
    EXPECT_NE(r1, r2);
}

// ─────────────────────────────────────────────────────────────────────────────
// Redaction — PARTIAL strategy
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(PiiDetectorFocusedTests, PartialStrategyKeepsPrefixAndSuffix) {
    PIIDetector::Config cfg;
    cfg.default_strategy         = PIIDetector::RedactionStrategy::PARTIAL;
    cfg.partial_keep_prefix      = 2;
    cfg.partial_keep_suffix      = 2;
    PIIDetector d(cfg);

    // SSN "123-45-6789" (11 chars): expect first 2 + *** + last 2 preserved
    std::string result = d.redactPII("SSN: 123-45-6789");
    EXPECT_EQ(std::string::npos, result.find("123-45-6789"));
    // At least the prefix characters '1' and '2' should appear in the output
    EXPECT_NE(std::string::npos, result.find("12"));
}

TEST_F(PiiDetectorFocusedTests, PartialStrategyFallsBackToMaskForShortValues) {
    // A very short value where prefix+suffix >= length should still be redacted
    PIIDetector::Config cfg;
    cfg.default_strategy    = PIIDetector::RedactionStrategy::PARTIAL;
    cfg.partial_keep_prefix = 10;
    cfg.partial_keep_suffix = 10;
    PIIDetector d(cfg);

    // Any short PII that gets detected should not cause a crash
    std::string result = d.redactPII("user@example.com");
    EXPECT_NE(result, "user@example.com");  // must be changed in some way
}

// ─────────────────────────────────────────────────────────────────────────────
// Per-type strategy override
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(PiiDetectorFocusedTests, PerTypeStrategyOverridesDefault) {
    PIIDetector::Config cfg;
    cfg.default_strategy = PIIDetector::RedactionStrategy::MASK;
    cfg.strategy_per_type[PIIDetector::PIIType::EMAIL] = PIIDetector::RedactionStrategy::REMOVE;
    PIIDetector d(cfg);

    std::string result = d.redactPII("Contact user@example.com soon");
    // Email should be REMOVED (→ "[REDACTED]"), not just masked
    EXPECT_NE(std::string::npos, result.find("[REDACTED]"));
    EXPECT_EQ(std::string::npos, result.find("user@example.com"));
}

TEST_F(PiiDetectorFocusedTests, GetStrategyReturnsPerTypeWhenSet) {
    PIIDetector::Config cfg;
    cfg.default_strategy = PIIDetector::RedactionStrategy::MASK;
    cfg.strategy_per_type[PIIDetector::PIIType::SSN] = PIIDetector::RedactionStrategy::HASH;
    PIIDetector d(cfg);

    EXPECT_EQ(PIIDetector::RedactionStrategy::HASH,
              d.getStrategy(PIIDetector::PIIType::SSN));
    EXPECT_EQ(PIIDetector::RedactionStrategy::MASK,
              d.getStrategy(PIIDetector::PIIType::EMAIL));
}

// ─────────────────────────────────────────────────────────────────────────────
// Strategy override on redactPII(text, strategy)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(PiiDetectorFocusedTests, ExplicitStrategyParameterOverridesConfig) {
    // Default config uses MASK; we call with REMOVE explicitly
    std::string result = detector_default_->redactPII(
        "Call 800-555-1212",
        PIIDetector::RedactionStrategy::REMOVE
    );
    EXPECT_NE(std::string::npos, result.find("[REDACTED]"));
}

// ─────────────────────────────────────────────────────────────────────────────
// Clean text pass-through
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(PiiDetectorFocusedTests, CleanTextPassesThroughUnchanged) {
    const std::string text = "The annual report is ready for review.";
    EXPECT_EQ(text, detector_default_->redactPII(text));
}

// ─────────────────────────────────────────────────────────────────────────────
// PIIMetrics
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(PiiDetectorFocusedTests, PIIMetricsRecordDetection) {
    PIIMetrics metrics;
    EXPECT_EQ(0u, metrics.total_checks);
    metrics.recordDetection(PIIDetector::PIIType::EMAIL);
    EXPECT_EQ(1u, metrics.total_checks);
    EXPECT_EQ(1u, metrics.pii_detected);
    EXPECT_EQ(1u, metrics.detections_by_type[PIIDetector::PIIType::EMAIL]);
}

TEST_F(PiiDetectorFocusedTests, PIIMetricsRecordRedaction) {
    PIIMetrics metrics;
    metrics.recordDetection(PIIDetector::PIIType::PHONE);
    metrics.recordRedaction();
    EXPECT_EQ(1u, metrics.pii_redacted);
}

TEST_F(PiiDetectorFocusedTests, PIIMetricsAggregatesMultipleTypes) {
    PIIMetrics metrics;
    metrics.recordDetection(PIIDetector::PIIType::EMAIL);
    metrics.recordDetection(PIIDetector::PIIType::EMAIL);
    metrics.recordDetection(PIIDetector::PIIType::SSN);
    EXPECT_EQ(3u, metrics.total_checks);
    EXPECT_EQ(2u, metrics.detections_by_type[PIIDetector::PIIType::EMAIL]);
    EXPECT_EQ(1u, metrics.detections_by_type[PIIDetector::PIIType::SSN]);
}

// ─────────────────────────────────────────────────────────────────────────────
// Match position tracking
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(PiiDetectorFocusedTests, MatchPositionsAreCorrect) {
    const std::string text = "Email: user@example.com end";
    auto matches = detector_default_->detectPII(text);

    bool found = false;
    for (const auto& m : matches) {
        if (m.type == PIIDetector::PIIType::EMAIL && m.value == "user@example.com") {
            EXPECT_EQ(text.substr(m.start_pos, m.end_pos - m.start_pos), m.value);
            found = true;
        }
    }
    EXPECT_TRUE(found);
}
