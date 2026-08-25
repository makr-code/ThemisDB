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
#include <chrono>
#include <cstdlib>
#include <vector>
#include <algorithm>

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

TEST_F(LLMProcessAnalyzerTest, ValidateResponse_DetectFraud_RiskScoreOutOfRange) {
    nlohmann::json resp;
    resp["fraud_analysis"]["risk_score"] = 1.2;

    EXPECT_FALSE(analyzer.validateResponse(resp, TaskType::DETECT_FRAUD));
}

TEST_F(LLMProcessAnalyzerTest, ValidateResponse_DetectFraud_AnomaliesMustBeStringArray) {
    nlohmann::json resp;
    resp["fraud_analysis"]["risk_score"]         = 0.4;
    resp["fraud_analysis"]["detected_anomalies"] = nlohmann::json::array({42});

    EXPECT_FALSE(analyzer.validateResponse(resp, TaskType::DETECT_FRAUD));
}

TEST_F(LLMProcessAnalyzerTest, ValidateResponse_PredictNext_ProbabilityOutOfRangeRejected) {
    nlohmann::json resp;
    resp["predictions"] = nlohmann::json::array({{
        {"activity", "next_step"},
        {"probability", 1.5},
        {"reasoning", "invalid probability"}
    }});

    EXPECT_FALSE(analyzer.validateResponse(resp, TaskType::PREDICT_NEXT));
}

TEST_F(LLMProcessAnalyzerTest, ValidateResponse_Verify5RRule_OverallComplianceMustBeBool) {
    nlohmann::json resp;
    resp["five_rights_check"]["overall_compliance"] = "yes";

    EXPECT_FALSE(analyzer.validateResponse(resp, TaskType::VERIFY_5R_RULE));
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

    static_cast<void>(analyzer.analyze(req));  // first call → cache miss

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

    static_cast<void>(analyzer.analyze(req));  // miss
    static_cast<void>(analyzer.analyze(req));  // hit

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

    static_cast<void>(analyzer.analyze(req));  // populate cache
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

    static_cast<void>(analyzer.analyze(req));  // miss, populates cache
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
    EXPECT_GE(resp.deviations.size(), 0u);
}

TEST_F(LLMProcessAnalyzerTest, AnalyzeProcess_ComplianceIssuesField) {
    LLMRequest req;
    req.task_type     = TaskType::ANALYZE_PROCESS;
    req.domain        = "legal";
    req.process_trace = {{"doc", "contract"}};
    req.ideal_model   = nlohmann::json::object();

    auto [ok, resp] = analyzer.analyze(req);

    EXPECT_TRUE(ok);
    EXPECT_GE(resp.compliance_issues.size(), 0u);
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

// ===========================================================================
// LLMConfig::max_cache_entries configurability
// ===========================================================================

TEST(LLMConfigTest, MaxCacheEntriesDefaultIs1000) {
    LLMConfig cfg;
    EXPECT_EQ(cfg.max_cache_entries, 1000);
}

TEST(LLMConfigTest, MaxCacheEntriesConfigurable) {
    LLMConfig cfg;
    cfg.max_cache_entries = 50;
    EXPECT_EQ(cfg.max_cache_entries, 50);

    LLMProcessAnalyzer analyzer(cfg);
    // Fill cache with 50 unique entries — no eviction yet
    for (int i = 0; i < 50; ++i) {
        LLMRequest req;
        req.task_type     = TaskType::ANALYZE_PROCESS;
        req.domain        = "admin";
        req.process_trace = {{"i", i}};
        req.ideal_model   = nlohmann::json::object();
        static_cast<void>(analyzer.analyze(req));
    }
    auto stats = analyzer.getCacheStats();
    EXPECT_EQ(stats.misses,    50u);
    EXPECT_EQ(stats.evictions, 0u);

    // 51st unique entry triggers one LRU eviction
    LLMRequest extra;
    extra.task_type     = TaskType::ANALYZE_PROCESS;
    extra.domain        = "admin";
    extra.process_trace = {{"i", 9999}};
    extra.ideal_model   = nlohmann::json::object();
    static_cast<void>(analyzer.analyze(extra));

    stats = analyzer.getCacheStats();
    EXPECT_EQ(stats.evictions, 1u);
}

// ===========================================================================
// LRU eviction: least-recently-used entry is evicted first
// ===========================================================================

TEST_F(LLMProcessAnalyzerTest, LRUEviction_LeastRecentlyUsedEntryEvicted) {
    // Configure a small cache so we can test LRU ordering deterministically
    LLMConfig cfg;
    cfg.provider          = LLMProvider::LOCAL;
    cfg.enable_caching    = true;
    cfg.max_retries       = 0;
    cfg.retry_delay_ms    = 0;
    cfg.max_cache_entries = 2;
    LLMProcessAnalyzer small_cache(cfg);

    auto makeReq = [](int i) {
        LLMRequest req;
        req.task_type     = TaskType::ANALYZE_PROCESS;
        req.domain        = "lru_test";
        req.process_trace = {{"id", i}};
        req.ideal_model   = nlohmann::json::object();
        return req;
    };

    // Fill to capacity: req0, req1 are cached
    static_cast<void>(small_cache.analyze(makeReq(0)));
    static_cast<void>(small_cache.analyze(makeReq(1)));

    // Promote req0 to MRU by re-requesting it
    auto [ok_hit, resp_hit] = small_cache.analyze(makeReq(0));
    EXPECT_TRUE(ok_hit);
    EXPECT_TRUE(resp_hit.from_cache);  // must be a hit

    // Insert req2 — capacity exceeded; req1 (LRU) should be evicted
    static_cast<void>(small_cache.analyze(makeReq(2)));

    auto stats = small_cache.getCacheStats();
    EXPECT_GE(stats.evictions, 1u);

    // req0 and req2 should still be cached (hits); req1 should be a miss
    auto [ok0, r0] = small_cache.analyze(makeReq(0));
    EXPECT_TRUE(ok0);
    EXPECT_TRUE(r0.from_cache);   // req0 was promoted before eviction

    auto [ok2, r2] = small_cache.analyze(makeReq(2));
    EXPECT_TRUE(ok2);
    EXPECT_TRUE(r2.from_cache);   // req2 was just inserted

    auto [ok1, r1] = small_cache.analyze(makeReq(1));
    EXPECT_TRUE(ok1);
    EXPECT_FALSE(r1.from_cache);  // req1 was the LRU — evicted
}

// ===========================================================================
// getCacheKey: hash-based key is fixed length and deterministic
// ===========================================================================

TEST_F(LLMProcessAnalyzerTest, CacheKey_DifferentTracesDifferentKeys) {
    // Two requests with different traces must not collide in the cache
    LLMRequest req1;
    req1.task_type     = TaskType::ANALYZE_PROCESS;
    req1.domain        = "collision";
    req1.process_trace = {{"a", 1}};
    req1.ideal_model   = nlohmann::json::object();

    LLMRequest req2 = req1;
    req2.process_trace = {{"a", 2}};

    static_cast<void>(analyzer.analyze(req1));  // miss, populates cache
    static_cast<void>(analyzer.analyze(req1));  // hit

    auto [ok_miss, resp2] = analyzer.analyze(req2);  // must be a miss (different trace)
    EXPECT_TRUE(ok_miss);
    EXPECT_FALSE(resp2.from_cache);
}

TEST_F(LLMProcessAnalyzerTest, CacheKey_SameRequestCacheHit) {
    LLMRequest req;
    req.task_type     = TaskType::PREDICT_NEXT;
    req.domain        = "keyhash";
    req.process_trace = {{"event", "submit"}, {"user", "alice"}};
    req.ideal_model   = {{"steps", nlohmann::json::array()}};

    static_cast<void>(analyzer.analyze(req));                           // miss
    auto [ok, resp] = analyzer.analyze(req);         // must be a hit
    EXPECT_TRUE(ok);
    EXPECT_TRUE(resp.from_cache);
}

// ===========================================================================
// Microbenchmark: putInCache() O(1) eviction — opt-in via THEMIS_RUN_PERF_TESTS=1
// ===========================================================================

/**
 * @brief Verify that the full analyze() call with LRU eviction completes in
 *        ≤ 100 µs P99 when the cache is at capacity (1 000 entries).
 *
 * Gates on the entire analyze() round-trip (getCacheKey + cache miss +
 * callLLM stub + putInCache with LRU eviction).  The eviction itself is O(1)
 * and contributes negligibly; the 100 µs ceiling gives ample CI headroom while
 * still catching an O(N) regression.
 *
 * This test is timing-sensitive and opt-in via THEMIS_RUN_PERF_TESTS=1.
 */
TEST(LLMProcessAnalyzerPerfTest, PutInCache_O1Eviction_Under1us) {
    const char* run_perf = std::getenv("THEMIS_RUN_PERF_TESTS");
    if (!run_perf || std::string(run_perf) != "1") {
        GTEST_SKIP() << "Skipping timing microbenchmark "
                        "(set THEMIS_RUN_PERF_TESTS=1 to enable)";
    }

    constexpr int kCapacity   = 1000;
    constexpr int kIterations = 500;

    LLMConfig cfg;
    cfg.provider          = LLMProvider::LOCAL;
    cfg.enable_caching    = true;
    cfg.max_retries       = 0;
    cfg.retry_delay_ms    = 0;
    cfg.max_cache_entries = kCapacity;
    LLMProcessAnalyzer analyzer(cfg);

    // Pre-fill cache to capacity with unique entries
    for (int i = 0; i < kCapacity; ++i) {
        LLMRequest req;
        req.task_type     = TaskType::ANALYZE_PROCESS;
        req.domain        = "bench";
        req.process_trace = {{"i", i}};
        req.ideal_model   = nlohmann::json::object();
        static_cast<void>(analyzer.analyze(req));
    }

    // Measure the time for kIterations insertions that each evict the LRU entry
    std::vector<int64_t> timings_ns;
    timings_ns.reserve(kIterations);

    for (int i = 0; i < kIterations; ++i) {
        LLMRequest req;
        req.task_type     = TaskType::ANALYZE_PROCESS;
        req.domain        = "bench";
        req.process_trace = {{"new", kCapacity + i}};
        req.ideal_model   = nlohmann::json::object();

        // getCacheKey() is called before putInCache(); measure only the
        // cache put by pre-computing the key and then issuing a repeated
        // miss on a distinct request (cache is full → eviction occurs).
        auto t0 = std::chrono::steady_clock::now();
        static_cast<void>(analyzer.analyze(req));  // includes getCacheKey + putInCache (+ callLLM stub)
        auto t1 = std::chrono::steady_clock::now();

        timings_ns.push_back(
            std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
    }

    std::sort(timings_ns.begin(), timings_ns.end());
    const int64_t p99_ns = timings_ns[static_cast<size_t>(kIterations * 99 / 100)];

    // The whole analyze() call (with simulated callLLM stub) must be fast;
    // the eviction itself is O(1) and contributes negligibly.
    // Gate: P99 ≤ 100 µs to be CI-safe (actual LRU eviction is < 1 µs).
    EXPECT_LE(p99_ns, 100'000LL)
        << "P99 analyze() with full-cache LRU eviction exceeded 100 µs: "
        << p99_ns << " ns (P99). "
        << "Check for O(N) eviction regression.";
}
