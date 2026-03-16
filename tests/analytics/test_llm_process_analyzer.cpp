/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_llm_process_analyzer.cpp                      ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-03-16 04:20:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     562                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 80742c94d  2026-02-27  feat(analytics): sanitize LLM API keys and CSV export data ║
    • e42ef7466  2026-02-27  Add LLMProcessAnalyzer unit tests for analytics coverage ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_llm_process_analyzer.cpp
 * @brief Unit tests for LLMProcessAnalyzer
 *
 * Covers:
 *  - generatePrompt() — all TaskType variants (ANALYZE_PROCESS, PREDICT_NEXT,
 *      VERIFY_5R_RULE, DETECT_FRAUD, other/default path)
 *  - validateResponse() — valid and invalid response schemas for each task type
 *  - analyze() — full round-trip using the built-in simulated callLLM stub;
 *      all four concrete task types plus cache hit path
 *  - Cache management — getCacheStats() hits/misses/hit_rate, clearCache()
 *  - Constructor / destructor (caching disabled path)
 *  - LLMResponse struct population (deviations, compliance_issues,
 *      recommendations, predictions, five_rights_check, fraud_analysis)
 */

#include <gtest/gtest.h>
#include "analytics/llm_process_analyzer.h"
#include <nlohmann/json.hpp>
#include <string>

using namespace themis;

// ---------------------------------------------------------------------------
// Helper: build a minimal valid LLMConfig
// ---------------------------------------------------------------------------
static LLMConfig makeConfig(bool caching = true) {
    LLMConfig cfg;
    cfg.provider       = LLMProvider::LOCAL;
    cfg.enable_caching = caching;
    cfg.max_retries    = 0;   // no real network calls expected
    cfg.retry_delay_ms = 0;
    return cfg;
}

// ---------------------------------------------------------------------------
// Fixtures
// ---------------------------------------------------------------------------

class LLMProcessAnalyzerTest : public ::testing::Test {
protected:
    LLMProcessAnalyzer analyzer{makeConfig()};
};

class LLMProcessAnalyzerNoCacheTest : public ::testing::Test {
protected:
    LLMProcessAnalyzer analyzer{makeConfig(/*caching=*/false)};
};

// ===========================================================================
// generatePrompt() tests
// ===========================================================================

TEST_F(LLMProcessAnalyzerTest, GeneratePromptAnalyzeProcess_ContainsKeywords) {
    nlohmann::json data;
    data["trace"] = {{"activity", "submit"}};
    data["model"] = {{"steps", nlohmann::json::array()}};
    data["context"] = nullptr;

    std::string prompt = analyzer.generatePrompt(TaskType::ANALYZE_PROCESS, data, "administrative");

    EXPECT_FALSE(prompt.empty());
    EXPECT_NE(prompt.find("conformance_score"), std::string::npos);
    EXPECT_NE(prompt.find("deviations"),        std::string::npos);
    EXPECT_NE(prompt.find("administrative"),    std::string::npos);
}

TEST_F(LLMProcessAnalyzerTest, GeneratePromptPredictNext_ContainsPredictions) {
    nlohmann::json data;
    data["trace"] = {{"activity", "approve"}};
    data["model"] = nlohmann::json::object();
    data["context"] = nullptr;

    std::string prompt = analyzer.generatePrompt(TaskType::PREDICT_NEXT, data, "financial");

    EXPECT_FALSE(prompt.empty());
    EXPECT_NE(prompt.find("predictions"), std::string::npos);
}

TEST_F(LLMProcessAnalyzerTest, GeneratePromptVerify5RRule_ContainsFiveRights) {
    nlohmann::json data;
    data["trace"] = {{"patient_id", "P-001"}, {"medication", "Aspirin"}};
    data["model"] = nlohmann::json::object();
    data["context"] = nullptr;

    std::string prompt = analyzer.generatePrompt(TaskType::VERIFY_5R_RULE, data, "healthcare");

    EXPECT_FALSE(prompt.empty());
    EXPECT_NE(prompt.find("five_rights_check"), std::string::npos);
    // Should mention all five rights
    EXPECT_NE(prompt.find("right_patient"),    std::string::npos);
    EXPECT_NE(prompt.find("right_medication"), std::string::npos);
    EXPECT_NE(prompt.find("right_dose"),       std::string::npos);
    EXPECT_NE(prompt.find("right_time"),       std::string::npos);
    EXPECT_NE(prompt.find("right_route"),      std::string::npos);
}

TEST_F(LLMProcessAnalyzerTest, GeneratePromptDetectFraud_ContainsFraudAnalysis) {
    nlohmann::json data;
    data["trace"]   = {{"amount", 5000}, {"vendor", "ACME"}};
    data["model"]   = nlohmann::json::object();
    data["context"] = {{"avg_invoice", 500}};

    std::string prompt = analyzer.generatePrompt(TaskType::DETECT_FRAUD, data, "financial");

    EXPECT_FALSE(prompt.empty());
    EXPECT_NE(prompt.find("fraud_analysis"), std::string::npos);
    EXPECT_NE(prompt.find("risk_score"),     std::string::npos);
}

TEST_F(LLMProcessAnalyzerTest, GeneratePromptDefaultPath_ReturnsNonEmpty) {
    nlohmann::json data;
    data["trace"] = nlohmann::json::object();

    // TaskType::COMPLIANCE_CHECK falls through to the default branch
    std::string prompt = analyzer.generatePrompt(TaskType::COMPLIANCE_CHECK, data, "general");

    EXPECT_FALSE(prompt.empty());
}

// ===========================================================================
// validateResponse() tests
// ===========================================================================

TEST_F(LLMProcessAnalyzerTest, ValidateResponse_AnalyzeProcess_Valid) {
    nlohmann::json resp;
    resp["conformance_score"] = 0.9;
    resp["deviations"]        = nlohmann::json::array();
    resp["compliance_issues"] = nlohmann::json::array();
    resp["recommendations"]   = nlohmann::json::array();

    EXPECT_TRUE(analyzer.validateResponse(resp, TaskType::ANALYZE_PROCESS));
}

TEST_F(LLMProcessAnalyzerTest, ValidateResponse_AnalyzeProcess_MissingField) {
    nlohmann::json resp;
    resp["conformance_score"] = 0.9;
    // Missing: deviations, compliance_issues, recommendations

    EXPECT_FALSE(analyzer.validateResponse(resp, TaskType::ANALYZE_PROCESS));
}

TEST_F(LLMProcessAnalyzerTest, ValidateResponse_PredictNext_Valid) {
    nlohmann::json resp;
    resp["predictions"] = nlohmann::json::array();

    EXPECT_TRUE(analyzer.validateResponse(resp, TaskType::PREDICT_NEXT));
}

TEST_F(LLMProcessAnalyzerTest, ValidateResponse_PredictNext_PredictionsNotArray) {
    nlohmann::json resp;
    resp["predictions"] = "not-an-array";

    EXPECT_FALSE(analyzer.validateResponse(resp, TaskType::PREDICT_NEXT));
}

TEST_F(LLMProcessAnalyzerTest, ValidateResponse_PredictNext_MissingField) {
    nlohmann::json resp;
    resp["other_field"] = 42;

    EXPECT_FALSE(analyzer.validateResponse(resp, TaskType::PREDICT_NEXT));
}

TEST_F(LLMProcessAnalyzerTest, ValidateResponse_Verify5RRule_Valid) {
    nlohmann::json resp;
    resp["five_rights_check"]["overall_compliance"] = true;

    EXPECT_TRUE(analyzer.validateResponse(resp, TaskType::VERIFY_5R_RULE));
}

TEST_F(LLMProcessAnalyzerTest, ValidateResponse_Verify5RRule_MissingOverallCompliance) {
    nlohmann::json resp;
    resp["five_rights_check"]["right_patient"] = true;
    // Missing: overall_compliance

    EXPECT_FALSE(analyzer.validateResponse(resp, TaskType::VERIFY_5R_RULE));
}

TEST_F(LLMProcessAnalyzerTest, ValidateResponse_Verify5RRule_MissingFiveRightsCheck) {
    nlohmann::json resp;
    resp["other"] = 1;

    EXPECT_FALSE(analyzer.validateResponse(resp, TaskType::VERIFY_5R_RULE));
}

TEST_F(LLMProcessAnalyzerTest, ValidateResponse_DetectFraud_Valid) {
    nlohmann::json resp;
    resp["fraud_analysis"]["risk_score"] = 0.3;

    EXPECT_TRUE(analyzer.validateResponse(resp, TaskType::DETECT_FRAUD));
}

TEST_F(LLMProcessAnalyzerTest, ValidateResponse_DetectFraud_MissingRiskScore) {
    nlohmann::json resp;
    resp["fraud_analysis"]["detected_anomalies"] = nlohmann::json::array();

    EXPECT_FALSE(analyzer.validateResponse(resp, TaskType::DETECT_FRAUD));
}

TEST_F(LLMProcessAnalyzerTest, ValidateResponse_Default_AlwaysTrue) {
    // Unknown task types should pass basic validation
    nlohmann::json resp;
    resp["any_field"] = "any_value";

    EXPECT_TRUE(analyzer.validateResponse(resp, TaskType::CLASSIFY_INCIDENT));
    EXPECT_TRUE(analyzer.validateResponse(resp, TaskType::SENTIMENT_ANALYSIS));
    EXPECT_TRUE(analyzer.validateResponse(resp, TaskType::OPTIMIZE_PROCESS));
    EXPECT_TRUE(analyzer.validateResponse(resp, TaskType::COMPLIANCE_CHECK));
}

// ===========================================================================
// analyze() round-trip tests (uses simulated callLLM internally)
// ===========================================================================

TEST_F(LLMProcessAnalyzerTest, AnalyzeProcess_SuccessAndConformanceScore) {
    LLMRequest req;
    req.task_type     = TaskType::ANALYZE_PROCESS;
    req.domain        = "administrative";
    req.process_trace = {{"activity", "submit"}};
    req.ideal_model   = {{"steps", nlohmann::json::array()}};

    auto [ok, resp] = analyzer.analyze(req);

    EXPECT_TRUE(ok);
    EXPECT_TRUE(resp.success);
    EXPECT_GE(resp.conformance_score, 0.0);
    EXPECT_LE(resp.conformance_score, 1.0);
    EXPECT_GE(resp.response_time_ms, 0);
}

TEST_F(LLMProcessAnalyzerTest, AnalyzeProcess_RecommendationsPopulated) {
    LLMRequest req;
    req.task_type     = TaskType::ANALYZE_PROCESS;
    req.domain        = "financial";
    req.process_trace = {{"activity", "approve"}};
    req.ideal_model   = nlohmann::json::object();

    auto [ok, resp] = analyzer.analyze(req);

    EXPECT_TRUE(ok);
    // The simulated response includes a recommendation
    EXPECT_FALSE(resp.recommendations.empty());
    EXPECT_FALSE(resp.recommendations[0].type.empty());
    EXPECT_FALSE(resp.recommendations[0].priority.empty());
}

TEST_F(LLMProcessAnalyzerTest, PredictNext_PredictionsPopulated) {
    LLMRequest req;
    req.task_type     = TaskType::PREDICT_NEXT;
    req.domain        = "administrative";
    req.process_trace = {{"activity", "review"}};
    req.ideal_model   = nlohmann::json::object();

    auto [ok, resp] = analyzer.analyze(req);

    EXPECT_TRUE(ok);
    EXPECT_FALSE(resp.predictions.empty());
    EXPECT_FALSE(resp.predictions[0].activity.empty());
    EXPECT_GE(resp.predictions[0].probability, 0.0);
    EXPECT_LE(resp.predictions[0].probability, 1.0);
}

TEST_F(LLMProcessAnalyzerTest, Verify5RRule_FiveRightsCheckPopulated) {
    LLMRequest req;
    req.task_type     = TaskType::VERIFY_5R_RULE;
    req.domain        = "healthcare";
    req.process_trace = {{"patient_id", "P-001"}, {"medication", "Aspirin"}};
    req.ideal_model   = nlohmann::json::object();

    auto [ok, resp] = analyzer.analyze(req);

    EXPECT_TRUE(ok);
    ASSERT_TRUE(resp.five_rights_check.has_value());
    EXPECT_TRUE(resp.five_rights_check->right_patient);
    EXPECT_TRUE(resp.five_rights_check->right_medication);
    EXPECT_TRUE(resp.five_rights_check->right_dose);
    EXPECT_TRUE(resp.five_rights_check->right_time);
    EXPECT_TRUE(resp.five_rights_check->right_route);
    EXPECT_TRUE(resp.five_rights_check->overall_compliance);
    EXPECT_EQ(resp.five_rights_check->risk_level, "low");
}

TEST_F(LLMProcessAnalyzerTest, DetectFraud_FraudAnalysisPopulated) {
    LLMRequest req;
    req.task_type     = TaskType::DETECT_FRAUD;
    req.domain        = "financial";
    req.process_trace = {{"amount", 5000}, {"vendor", "ACME"}};
    req.ideal_model   = nlohmann::json::object();

    auto [ok, resp] = analyzer.analyze(req);

    EXPECT_TRUE(ok);
    ASSERT_TRUE(resp.fraud_analysis.has_value());
    EXPECT_GE(resp.fraud_analysis->risk_score, 0.0);
    EXPECT_LE(resp.fraud_analysis->risk_score, 1.0);
    EXPECT_FALSE(resp.fraud_analysis->flags.duplicate);
    EXPECT_FALSE(resp.fraud_analysis->flags.unusual_amount);
    EXPECT_FALSE(resp.fraud_analysis->recommended_action.empty());
}

TEST_F(LLMProcessAnalyzerTest, AnalyzeProcess_NotFromCacheOnFirstCall) {
    LLMRequest req;
    req.task_type     = TaskType::ANALYZE_PROCESS;
    req.domain        = "admin";
    req.process_trace = {{"x", 1}};
    req.ideal_model   = nlohmann::json::object();

    auto [ok, resp] = analyzer.analyze(req);

    EXPECT_TRUE(ok);
    EXPECT_FALSE(resp.from_cache);
}

TEST_F(LLMProcessAnalyzerTest, AnalyzeProcess_SecondCallFromCache) {
    LLMRequest req;
    req.task_type     = TaskType::ANALYZE_PROCESS;
    req.domain        = "admin";
    req.process_trace = {{"x", 2}};
    req.ideal_model   = nlohmann::json::object();

    auto [ok1, resp1] = analyzer.analyze(req);
    ASSERT_TRUE(ok1);
    EXPECT_FALSE(resp1.from_cache);

    auto [ok2, resp2] = analyzer.analyze(req);
    ASSERT_TRUE(ok2);
    EXPECT_TRUE(resp2.from_cache);
}

// ===========================================================================
// Cache management tests
// ===========================================================================

TEST_F(LLMProcessAnalyzerTest, CacheStats_InitiallyZero) {
    auto stats = analyzer.getCacheStats();
    EXPECT_EQ(stats.hits,   0u);
    EXPECT_EQ(stats.misses, 0u);
    EXPECT_EQ(stats.evictions, 0u);
    EXPECT_DOUBLE_EQ(stats.hit_rate(), 0.0);
}

TEST_F(LLMProcessAnalyzerTest, CacheStats_AfterOneMiss) {
    LLMRequest req;
    req.task_type     = TaskType::ANALYZE_PROCESS;
    req.domain        = "hr";
    req.process_trace = {{"step", "onboard"}};
    req.ideal_model   = nlohmann::json::object();

    analyzer.analyze(req);  // first call → cache miss

    auto stats = analyzer.getCacheStats();
    EXPECT_EQ(stats.misses, 1u);
    EXPECT_EQ(stats.hits,   0u);
    EXPECT_DOUBLE_EQ(stats.hit_rate(), 0.0);
}

TEST_F(LLMProcessAnalyzerTest, CacheStats_AfterHitAndMiss) {
    LLMRequest req;
    req.task_type     = TaskType::PREDICT_NEXT;
    req.domain        = "it";
    req.process_trace = {{"step", "resolve"}};
    req.ideal_model   = nlohmann::json::object();

    analyzer.analyze(req);  // miss
    analyzer.analyze(req);  // hit

    auto stats = analyzer.getCacheStats();
    EXPECT_EQ(stats.misses, 1u);
    EXPECT_EQ(stats.hits,   1u);
    EXPECT_DOUBLE_EQ(stats.hit_rate(), 0.5);
}

TEST_F(LLMProcessAnalyzerTest, ClearCache_ResetsStats) {
    LLMRequest req;
    req.task_type     = TaskType::DETECT_FRAUD;
    req.domain        = "finance";
    req.process_trace = {{"inv", 999}};
    req.ideal_model   = nlohmann::json::object();

    analyzer.analyze(req);  // populate cache
    analyzer.clearCache();

    auto stats = analyzer.getCacheStats();
    EXPECT_EQ(stats.hits,   0u);
    EXPECT_EQ(stats.misses, 0u);
    EXPECT_EQ(stats.evictions, 0u);
}

TEST_F(LLMProcessAnalyzerTest, ClearCache_SecondCallIsAMiss) {
    LLMRequest req;
    req.task_type     = TaskType::DETECT_FRAUD;
    req.domain        = "finance";
    req.process_trace = {{"inv", 777}};
    req.ideal_model   = nlohmann::json::object();

    analyzer.analyze(req);  // miss, populates cache
    analyzer.clearCache();
    auto [ok, resp] = analyzer.analyze(req);  // should miss again after clear

    EXPECT_TRUE(ok);
    EXPECT_FALSE(resp.from_cache);
}

// ===========================================================================
// No-cache path
// ===========================================================================

TEST_F(LLMProcessAnalyzerNoCacheTest, AnalyzeWithCachingDisabled_NeverFromCache) {
    LLMRequest req;
    req.task_type     = TaskType::ANALYZE_PROCESS;
    req.domain        = "legal";
    req.process_trace = {{"act", "sign"}};
    req.ideal_model   = nlohmann::json::object();

    auto [ok1, resp1] = analyzer.analyze(req);
    auto [ok2, resp2] = analyzer.analyze(req);

    EXPECT_TRUE(ok1);
    EXPECT_TRUE(ok2);
    EXPECT_FALSE(resp1.from_cache);
    EXPECT_FALSE(resp2.from_cache);

    // Stats should show 2 misses, 0 hits (caching disabled, misses still counted)
    auto stats = analyzer.getCacheStats();
    EXPECT_EQ(stats.hits, 0u);
}

// ===========================================================================
// Response struct field tests
// ===========================================================================

TEST_F(LLMProcessAnalyzerTest, AnalyzeProcess_DeviationsField) {
    // The default simulated response has an empty deviations array.
    LLMRequest req;
    req.task_type     = TaskType::ANALYZE_PROCESS;
    req.domain        = "admin";
    req.process_trace = {{"a", "b"}};
    req.ideal_model   = nlohmann::json::object();

    auto [ok, resp] = analyzer.analyze(req);

    EXPECT_TRUE(ok);
    // deviations should be initialized (may be empty in simulation)
    // What matters is no crash and the vector is accessible
    EXPECT_NO_FATAL_FAILURE(resp.deviations.size());
}

TEST_F(LLMProcessAnalyzerTest, AnalyzeProcess_ComplianceIssuesField) {
    LLMRequest req;
    req.task_type     = TaskType::ANALYZE_PROCESS;
    req.domain        = "legal";
    req.process_trace = {{"doc", "contract"}};
    req.ideal_model   = nlohmann::json::object();

    auto [ok, resp] = analyzer.analyze(req);

    EXPECT_TRUE(ok);
    EXPECT_NO_FATAL_FAILURE(resp.compliance_issues.size());
}

TEST_F(LLMProcessAnalyzerTest, ResponseTimeIsNonNegative) {
    LLMRequest req;
    req.task_type     = TaskType::PREDICT_NEXT;
    req.domain        = "ops";
    req.process_trace = {{"step", "deploy"}};
    req.ideal_model   = nlohmann::json::object();

    auto [ok, resp] = analyzer.analyze(req);

    EXPECT_TRUE(ok);
    EXPECT_GE(resp.response_time_ms, 0);
}

// ===========================================================================
// CacheStats::hit_rate() edge cases
// ===========================================================================

TEST(CacheStatsTest, HitRateZeroWhenNoRequests) {
    LLMProcessAnalyzer::CacheStats stats;
    EXPECT_DOUBLE_EQ(stats.hit_rate(), 0.0);
}

TEST(CacheStatsTest, HitRateOneWhenAllHits) {
    LLMProcessAnalyzer::CacheStats stats;
    stats.hits   = 5;
    stats.misses = 0;
    EXPECT_DOUBLE_EQ(stats.hit_rate(), 1.0);
}

TEST(CacheStatsTest, HitRateHalfWhenEqualHitsAndMisses) {
    LLMProcessAnalyzer::CacheStats stats;
    stats.hits   = 3;
    stats.misses = 3;
    EXPECT_DOUBLE_EQ(stats.hit_rate(), 0.5);
}

// ===========================================================================
// sanitizeApiKey() tests
// ===========================================================================

TEST(SanitizeApiKeyTest, EmptyKeyReturnsNotSet) {
    EXPECT_EQ(sanitizeApiKey(""), "<not set>");
}

TEST(SanitizeApiKeyTest, ShortKeyFullyMasked) {
    // 8 chars or fewer: fully masked
    EXPECT_EQ(sanitizeApiKey("abcd"),     "****");
    EXPECT_EQ(sanitizeApiKey("12345678"), "********");
}

TEST(SanitizeApiKeyTest, LongKeyShowsPrefixAndSuffix) {
    // Key of 20 chars: first 4 and last 4 visible
    std::string key = "sk-abcdefghij1234xyz";  // 20 chars
    std::string masked = sanitizeApiKey(key);

    EXPECT_EQ(masked.substr(0, 4), "sk-a");
    EXPECT_NE(masked.find("***...***"), std::string::npos);
    EXPECT_EQ(masked.substr(masked.size() - 4), "4xyz");
}

TEST(SanitizeApiKeyTest, RawKeyDoesNotAppearInMasked) {
    std::string raw_key = "sk-supersecretapikey123456";
    std::string masked = sanitizeApiKey(raw_key);

    // The full raw key must not appear in the masked output
    EXPECT_EQ(masked.find(raw_key), std::string::npos);
    // Middle portion (not prefix or suffix) must not appear
    std::string middle = raw_key.substr(4, raw_key.size() - 8);
    EXPECT_EQ(masked.find(middle), std::string::npos);
}

TEST(SanitizeApiKeyTest, ExactlyNineLengthIsNotFullyMasked) {
    // 9 chars: 4 visible + "***...***" + 4 visible, not fully masked
    std::string key = "abcde6789";  // 9 chars
    std::string masked = sanitizeApiKey(key);
    EXPECT_EQ(masked.substr(0, 4), "abcd");
    EXPECT_EQ(masked.substr(masked.size() - 4), "6789");
}
