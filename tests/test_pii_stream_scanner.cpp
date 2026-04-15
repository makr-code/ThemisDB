/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_pii_stream_scanner.cpp                        ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-15 04:28:40                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟡 RELEASE-CANDIDATE                            ║
    • Quality Score:   65.0/100                                       ║
    • Total Lines:     164                                            ║
    • Open Issues:     TODOs: 0, Stubs: 11                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 25f9a09910  2026-04-02  Refactor tests and improve assertions   ║
    • edcfeb9848  2026-03-11  feat: add scripts for auditing and reconciling GitHub iss... ║
    • eea8f803ba  2026-03-09  feat(utils): implement HashChainAuditWriter/AuditLogVerif... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ⚠️  Needs Work                                              ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "utils/pii_detection_engine.h"

#include <memory>
#include <string>
#include <vector>

using namespace themis::utils;

// ---------------------------------------------------------------------------
// Minimal stub engine that detects the literal token "SECRET" as PII
// ---------------------------------------------------------------------------

class StubEngine : public IPIIDetectionEngine {
public:
    std::string getName()    const override { return "stub"; }
    std::string getVersion() const override { return "1.0.0"; }
    bool        isEnabled()  const override { return true; }
    PluginSignature getSignature() const override { return {}; }
    bool initialize(const nlohmann::json&) override { return true; }
    bool reload(const nlohmann::json&)     override { return true; }
    std::string getLastError()             const override { return {}; }
    nlohmann::json getMetadata()           const override { return {}; }
    PIIType classifyFieldName(const std::string&) const override { return PIIType::UNKNOWN; }
    std::string getRedactionRecommendation(PIIType) const override { return "partial"; }

    std::vector<PIIFinding> detectInText(const std::string& text) const override {
        std::vector<PIIFinding> results;
        const std::string token = "SECRET";
        size_t pos = 0;
        while ((pos = text.find(token, pos)) != std::string::npos) {
            PIIFinding f;
            f.type         = PIIType::UNKNOWN;
            f.value        = token;
            f.start_offset = pos;
            f.end_offset   = pos + token.size();
            f.confidence   = 1.0;
            f.engine_name  = "stub";
            results.push_back(f);
            pos += token.size();
        }
        return results;
    }
};

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

// ============================================================================
// Construction
// ============================================================================

TEST(PIIStreamScanner, NullEngineThrows) {
    EXPECT_THROW(PIIStreamScanner(nullptr), std::invalid_argument);
}

TEST(PIIStreamScanner, ValidConstructionDoesNotThrow) {
    auto detector = std::make_shared<StubEngine>();
    EXPECT_NO_THROW({
        auto scanner = std::make_unique<PIIStreamScanner>(detector, PIIStreamScannerConfig{});
        EXPECT_NE(scanner, nullptr);
    });
}

// ============================================================================
// Single-chunk scan
// ============================================================================

TEST(PIIStreamScanner, DetectsTokenInSingleChunk) {
    auto engine  = std::make_shared<StubEngine>();
    PIIStreamScanner scanner(engine);

    auto findings = scanner.scan_chunk("Hello SECRET world", /*is_last=*/true);
    ASSERT_EQ(findings.size(), 1u);
    EXPECT_EQ(findings[0].value, "SECRET");
}

TEST(PIIStreamScanner, NoFindingsOnCleanText) {
    auto engine  = std::make_shared<StubEngine>();
    PIIStreamScanner scanner(engine);

    auto findings = scanner.scan_chunk("Hello clean world", true);
    EXPECT_TRUE(findings.empty());
}

// ============================================================================
// Multi-chunk: boundary handling
// ============================================================================

TEST(PIIStreamScanner, DetectsTokenSplitAcrossChunks) {
    auto engine  = std::make_shared<StubEngine>();
    PIIStreamScannerConfig cfg;
    // Keep exactly the trailing "SECR" so the next chunk can complete "SECRET".
    cfg.lookahead_bytes = 4;
    PIIStreamScanner scanner(engine, cfg);

    // "SECR" in first chunk, "ET" in second — lookahead must bridge them.
    auto f1 = scanner.scan_chunk("Hello SECR");
    auto f2 = scanner.scan_chunk("ET world", /*is_last=*/true);

    size_t total = f1.size() + f2.size();
    EXPECT_EQ(total, 1u) << "Expected exactly one SECRET finding across boundary";
}

TEST(PIIStreamScanner, DetectsMultipleTokensAcrossChunks) {
    auto engine  = std::make_shared<StubEngine>();
    PIIStreamScanner scanner(engine);

    auto f1 = scanner.scan_chunk("first SECRET chunk ");
    auto f2 = scanner.scan_chunk("second SECRET chunk", true);

    size_t total = f1.size() + f2.size();
    EXPECT_EQ(total, 2u);
}

// ============================================================================
// Reset
// ============================================================================

TEST(PIIStreamScanner, ResetResetsState) {
    auto engine  = std::make_shared<StubEngine>();
    PIIStreamScanner scanner(engine);

    scanner.scan_chunk("some SECRET data", true);
    EXPECT_GT(scanner.bytes_processed(), 0u);

    scanner.reset();
    EXPECT_EQ(scanner.bytes_processed(), 0u);
}

TEST(PIIStreamScanner, BytesProcessedAccumulates) {
    auto engine  = std::make_shared<StubEngine>();
    PIIStreamScanner scanner(engine);

    scanner.scan_chunk("abc");
    scanner.scan_chunk("def", true);
    // After two chunks the processed count must be at least 6
    EXPECT_GE(scanner.bytes_processed(), 6u);
}
