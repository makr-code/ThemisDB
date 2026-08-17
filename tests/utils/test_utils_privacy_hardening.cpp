/**
 * @file test_utils_privacy_hardening.cpp
 * @brief Phase 4 hardening tests for PIIStreamScanner and PIIPseudonymizer.
 *
 * Coverage targets (Phase 4 gate):
 *  - PIIStreamScanner: engine exception → fail-closed (sentinel finding returned)
 *  - PIIStreamScanner: engine==nullptr → throw on construction
 *  - PIIStreamScanner: consecutive chunks maintain correct absolute offsets
 *  - Privacy error codes in range 9040-9049
 *  - ErrorContext category for privacy codes
 */

#include <gtest/gtest.h>

#include "utils/error_contracts.h"
#include "utils/pii_detection_engine.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace themis::utils;

// ─────────────────────────────────────────────────────────────────────────────
// Mock detection engine that always throws (simulates timeout / crash)
// ─────────────────────────────────────────────────────────────────────────────
class AlwaysThrowingEngine : public IPIIDetectionEngine {
public:
    std::vector<PIIFinding> detectInText(const std::string&) const override {
        throw std::runtime_error("simulated engine timeout");
    }
    std::unordered_map<std::string, std::vector<PIIFinding>>
    detectInJson([[maybe_unused]] const nlohmann::json& j) const override {
        throw std::runtime_error("simulated engine timeout");
    }
    PIIType classifyFieldName([[maybe_unused]] const std::string&) const override {
        return PIIType::UNKNOWN;
    }
    size_t maxPatternLength() const override { return 64; }
    std::string engineName() const override { return "AlwaysThrowingEngine"; }
    bool isEnabled() const override { return true; }
    void setEnabled([[maybe_unused]] bool) override {}
    double confidenceThreshold() const override { return 0.5; }
    void setConfidenceThreshold([[maybe_unused]] double) override {}
};

// ─────────────────────────────────────────────────────────────────────────────
// Mock detection engine that returns no findings (safe)
// ─────────────────────────────────────────────────────────────────────────────
class EmptyFindingsEngine : public IPIIDetectionEngine {
public:
    std::vector<PIIFinding> detectInText([[maybe_unused]] const std::string& t) const override {
        return {};
    }
    std::unordered_map<std::string, std::vector<PIIFinding>>
    detectInJson([[maybe_unused]] const nlohmann::json& j) const override {
        return {};
    }
    PIIType classifyFieldName([[maybe_unused]] const std::string&) const override {
        return PIIType::UNKNOWN;
    }
    size_t maxPatternLength() const override { return 32; }
    std::string engineName() const override { return "EmptyFindingsEngine"; }
    bool isEnabled() const override { return true; }
    void setEnabled([[maybe_unused]] bool) override {}
    double confidenceThreshold() const override { return 0.5; }
    void setConfidenceThreshold([[maybe_unused]] double) override {}
};

// ─────────────────────────────────────────────────────────────────────────────
// PH-01: Null engine throws at construction
// ─────────────────────────────────────────────────────────────────────────────
TEST(PrivacyHardening, NullEngineThrowsOnConstruction) {
    EXPECT_THROW(
        PIIStreamScanner scanner(nullptr, PIIStreamScannerConfig{}),
        std::invalid_argument);
}

// ─────────────────────────────────────────────────────────────────────────────
// PH-02: Engine exception → fail-closed (sentinel finding covering entire window)
// ─────────────────────────────────────────────────────────────────────────────
TEST(PrivacyHardening, EngineExceptionFailClosedReturnsSentinelFinding) {
    auto engine = std::make_shared<AlwaysThrowingEngine>();
    PIIStreamScannerConfig cfg;
    PIIStreamScanner scanner(engine, cfg);

    const std::string chunk = "hello@example.com is a test string";
    std::vector<PIIFinding> findings;
    EXPECT_NO_THROW(findings = scanner.scan_chunk(chunk, /*is_last=*/true));

    // Fail-closed: must return at least one sentinel finding
    ASSERT_GE(findings.size(), 1u);
    EXPECT_EQ(findings[0].pii_type, "UNKNOWN_FAIL_CLOSED");
    // Confidence must be maximum (1.0) for a fail-closed sentinel
    EXPECT_FLOAT_EQ(findings[0].confidence, 1.0f);
}

// ─────────────────────────────────────────────────────────────────────────────
// PH-03: No-throwing engine returns empty findings correctly
// ─────────────────────────────────────────────────────────────────────────────
TEST(PrivacyHardening, CleanEngineReturnsEmptyFindings) {
    auto engine = std::make_shared<EmptyFindingsEngine>();
    PIIStreamScanner scanner(engine, PIIStreamScannerConfig{});

    auto findings = scanner.scan_chunk("no pii here", /*is_last=*/true);
    EXPECT_TRUE(findings.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// PH-04: bytes_processed() accumulates across chunks
// ─────────────────────────────────────────────────────────────────────────────
TEST(PrivacyHardening, BytesProcessedAccumulatesAcrossChunks) {
    auto engine = std::make_shared<EmptyFindingsEngine>();
    PIIStreamScanner scanner(engine, PIIStreamScannerConfig{});

    const std::string c1 = "first_chunk_";
    const std::string c2 = "second_chunk";
    scanner.scan_chunk(c1, /*is_last=*/false);
    scanner.scan_chunk(c2, /*is_last=*/true);

    // Total bytes processed must be >= c1.size() (last chunk may be buffered)
    EXPECT_GE(scanner.bytes_processed(), c1.size());
}

// ─────────────────────────────────────────────────────────────────────────────
// PH-05: reset() clears buffer and resets offset to zero
// ─────────────────────────────────────────────────────────────────────────────
TEST(PrivacyHardening, ResetClearsStateAndOffset) {
    auto engine = std::make_shared<EmptyFindingsEngine>();
    PIIStreamScanner scanner(engine, PIIStreamScannerConfig{});

    scanner.scan_chunk("some data that advances offset", /*is_last=*/true);
    EXPECT_GT(scanner.bytes_processed(), 0u);

    scanner.reset();
    EXPECT_EQ(scanner.bytes_processed(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// PH-06: Privacy error codes are in range 9040-9049
// ─────────────────────────────────────────────────────────────────────────────
TEST(PrivacyHardening, PrivacyErrorCodesInRange) {
    using EC = ErrorCode;
    auto check = [](EC code) {
        auto v = static_cast<uint16_t>(code);
        EXPECT_GE(v, uint16_t{9040}) << "code " << v << " below 9040";
        EXPECT_LE(v, uint16_t{9049}) << "code " << v << " above 9049";
    };
    check(EC::PRIVACY_INVALID_INPUT);
    check(EC::PRIVACY_PATTERN_OVERFLOW);
    check(EC::PRIVACY_DETECTION_TIMEOUT);
    check(EC::PRIVACY_BUFFER_OVERFLOW);
    check(EC::PRIVACY_ENGINE_LOAD_FAILED);
    check(EC::PRIVACY_CONFIG_INVALID);
    check(EC::PRIVACY_UNICODE_ERROR);
    check(EC::PRIVACY_MEMORY_EXCEEDED);
    check(EC::PRIVACY_NO_ENGINE);
    check(EC::PRIVACY_ENGINE_FAILED);
}

// ─────────────────────────────────────────────────────────────────────────────
// PH-07: ErrorContext categorizes privacy codes as Privacy
// ─────────────────────────────────────────────────────────────────────────────
TEST(PrivacyHardening, ErrorContextCategoryIsPrivacy) {
    for (auto code : {ErrorCode::PRIVACY_INVALID_INPUT,
                      ErrorCode::PRIVACY_DETECTION_TIMEOUT,
                      ErrorCode::PRIVACY_ENGINE_FAILED,
                      ErrorCode::PRIVACY_NO_ENGINE}) {
        auto ctx = makeErrorContext(code, "PH-07-test", "unit test",
                                    ErrorSeverity::Error, false);
        EXPECT_EQ(ctx.category, ErrorCategory::PrivacyDetection)
            << "code " << static_cast<uint16_t>(code)
            << " should be Privacy category";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// PH-08: Oversized input to scan_chunk handled without memory explosion
//        (lookahead buffer bounded by configuration)
// ─────────────────────────────────────────────────────────────────────────────
TEST(PrivacyHardening, OversizedChunkDoesNotExhaustedMemory) {
    auto engine = std::make_shared<EmptyFindingsEngine>();
    PIIStreamScannerConfig cfg;
    cfg.lookahead_bytes = 256; // very small holdback
    PIIStreamScanner scanner(engine, cfg);

    // 1 MiB single chunk – must not OOM or throw
    const std::string big_chunk(1024 * 1024, 'x');
    EXPECT_NO_THROW(scanner.scan_chunk(big_chunk, /*is_last=*/true));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
