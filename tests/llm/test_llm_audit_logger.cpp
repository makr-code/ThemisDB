/**
 * @file test_llm_audit_logger.cpp
 * @brief Unit tests for LLMModelAuditLogger — JSON-lines analytics export.
 *
 * Tests cover:
 *  - logEvent() writes a record to the in-memory store
 *  - logPolicyViolation() writes PROMPT_BLOCKED / PROMPT_REDACTED records
 *  - logInference() stores success and failure records
 *  - queryLogs() filters by model_id
 *  - exportAnalytics() writes valid JSON-lines to a stream
 *  - exportAnalytics() filters by model_id
 *  - getModelStats() aggregates correctly
 *  - setEnabled(false) suppresses all writes
 *  - Multiple events accumulate correctly
 *
 * @see docs/llm_roadmap.md — Q4 Audit/Analytics checklist
 */

#include <gtest/gtest.h>
#include "llm/llm_model_audit_logger.h"
#include "utils/audit_logger.h"

#include <sstream>
#include <string>
#include <nlohmann/json.hpp>

using namespace themis::llm;
using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Helper: build an LLMModelAuditLogger with no file output (empty log_path)
// ---------------------------------------------------------------------------
static LLMModelAuditLogger makeLogger() {
    themis::utils::AuditLoggerConfig cfg;
    cfg.log_path = "";  // no file — in-memory only for tests
    return LLMModelAuditLogger(cfg);
}

// ---------------------------------------------------------------------------
// logEvent / queryLogs
// ---------------------------------------------------------------------------

TEST(LLMAuditLoggerTest, LogEvent_AppendedToInMemoryStore) {
    auto logger = makeLogger();
    logger.logEvent(LLMModelAuditEventType::MODEL_LOADED, "mistral-7b", {});
    auto logs = logger.queryLogs("mistral-7b");
    EXPECT_EQ(logs.size(), 1u);
}

TEST(LLMAuditLoggerTest, QueryLogs_FiltersByModelId) {
    auto logger = makeLogger();
    logger.logEvent(LLMModelAuditEventType::MODEL_LOADED,   "model-a", {});
    logger.logEvent(LLMModelAuditEventType::MODEL_UNLOADED, "model-b", {});
    auto a = logger.queryLogs("model-a");
    auto b = logger.queryLogs("model-b");
    EXPECT_EQ(a.size(), 1u);
    EXPECT_EQ(b.size(), 1u);
}

TEST(LLMAuditLoggerTest, QueryLogs_EmptyModelId_ReturnsAll) {
    auto logger = makeLogger();
    logger.logEvent(LLMModelAuditEventType::MODEL_LOADED,   "m1", {});
    logger.logEvent(LLMModelAuditEventType::MODEL_UNLOADED, "m2", {});
    auto all = logger.queryLogs(""); // empty = no model filter
    EXPECT_EQ(all.size(), 2u);
}

TEST(LLMAuditLoggerTest, QueryLogs_RecordContainsEventType) {
    auto logger = makeLogger();
    logger.logEvent(LLMModelAuditEventType::MODEL_LOADED, "m1", {{"key", "val"}});
    auto logs = logger.queryLogs("m1");
    ASSERT_EQ(logs.size(), 1u);
    EXPECT_EQ(logs[0].at("event_type").get<std::string>(), "MODEL_LOADED");
}

// ---------------------------------------------------------------------------
// logPolicyViolation
// ---------------------------------------------------------------------------

TEST(LLMAuditLoggerTest, PolicyViolation_BlockedEventType) {
    auto logger = makeLogger();
    logger.logPolicyViolation("m1", "req-1", "no_jailbreak", "pattern matched", true);
    auto logs = logger.queryLogs("m1");
    ASSERT_EQ(logs.size(), 1u);
    EXPECT_EQ(logs[0].at("event_type").get<std::string>(), "PROMPT_BLOCKED");
}

TEST(LLMAuditLoggerTest, PolicyViolation_RedactedEventType) {
    auto logger = makeLogger();
    logger.logPolicyViolation("m1", "req-2", "phone_number", "PII redacted", false);
    auto logs = logger.queryLogs("m1");
    ASSERT_EQ(logs.size(), 1u);
    EXPECT_EQ(logs[0].at("event_type").get<std::string>(), "PROMPT_REDACTED");
}

TEST(LLMAuditLoggerTest, PolicyViolation_DetailsContainRuleName) {
    auto logger = makeLogger();
    logger.logPolicyViolation("m1", "req-3", "injection_rule", "reason text", true);
    auto logs = logger.queryLogs("m1");
    ASSERT_EQ(logs.size(), 1u);
    EXPECT_TRUE(logs[0].contains("rule_name"));
    EXPECT_EQ(logs[0].at("rule_name").get<std::string>(), "injection_rule");
}

// ---------------------------------------------------------------------------
// logInference
// ---------------------------------------------------------------------------

TEST(LLMAuditLoggerTest, LogInference_Success_RecordedAsCompleted) {
    auto logger = makeLogger();
    LLMModelInferenceAudit audit;
    audit.model_id   = "llama-3";
    audit.request_id = "r1";
    audit.success    = true;
    logger.logInference(audit);
    auto logs = logger.queryLogs("llama-3");
    ASSERT_EQ(logs.size(), 1u);
    EXPECT_EQ(logs[0].at("event_type").get<std::string>(), "INFERENCE_COMPLETED");
}

TEST(LLMAuditLoggerTest, LogInference_Failure_RecordedAsFailed) {
    auto logger = makeLogger();
    LLMModelInferenceAudit audit;
    audit.model_id   = "llama-3";
    audit.request_id = "r2";
    audit.success    = false;
    logger.logInference(audit);
    auto logs = logger.queryLogs("llama-3");
    ASSERT_EQ(logs.size(), 1u);
    EXPECT_EQ(logs[0].at("event_type").get<std::string>(), "INFERENCE_FAILED");
}

// ---------------------------------------------------------------------------
// exportAnalytics — JSON-lines output
// ---------------------------------------------------------------------------

TEST(LLMAuditLoggerTest, ExportAnalytics_WritesValidJsonLines) {
    auto logger = makeLogger();
    logger.logEvent(LLMModelAuditEventType::MODEL_LOADED, "m1", {});
    logger.logEvent(LLMModelAuditEventType::MODEL_LOADED, "m2", {});

    std::ostringstream oss = {};
    size_t count = logger.exportAnalytics(oss);

    EXPECT_EQ(count, 2u);

    // Each line must be valid JSON
    std::istringstream iss(oss.str());
    std::string line = {};
    size_t line_count = 0;
    while (std::getline(iss, line)) {
        if (line.empty()) {
          continue;
        }
        EXPECT_NO_THROW({
            auto parsed = json::parse(line);
            static_cast<void>(parsed);
        }) << "Invalid JSON line: " << line;
        ++line_count;
    }
    EXPECT_EQ(line_count, 2u);
}

TEST(LLMAuditLoggerTest, ExportAnalytics_ContainsRequiredFields) {
    auto logger = makeLogger();
    logger.logEvent(LLMModelAuditEventType::MODEL_LOADED, "m1", {{"extra", 42}});

    std::ostringstream oss = {};
    logger.exportAnalytics(oss);

    auto obj = json::parse(oss.str());
    EXPECT_TRUE(obj.contains("timestamp_iso8601"));
    EXPECT_TRUE(obj.contains("event_type"));
    EXPECT_TRUE(obj.contains("model_id"));
    EXPECT_TRUE(obj.contains("details"));
}

TEST(LLMAuditLoggerTest, ExportAnalytics_FiltersByModelId) {
    auto logger = makeLogger();
    logger.logEvent(LLMModelAuditEventType::MODEL_LOADED, "alpha", {});
    logger.logEvent(LLMModelAuditEventType::MODEL_LOADED, "beta",  {});

    std::ostringstream oss = {};
    size_t count = logger.exportAnalytics(oss, "alpha");

    EXPECT_EQ(count, 1u);
    auto obj = json::parse(oss.str());
    EXPECT_EQ(obj.at("model_id").get<std::string>(), "alpha");
}

TEST(LLMAuditLoggerTest, ExportAnalytics_EmptyFilter_ReturnsAll) {
    auto logger = makeLogger();
    logger.logEvent(LLMModelAuditEventType::MODEL_LOADED,   "m1", {});
    logger.logEvent(LLMModelAuditEventType::MODEL_UNLOADED, "m2", {});
    logger.logEvent(LLMModelAuditEventType::INFERENCE_COMPLETED, "m1", {});

    std::ostringstream oss = {};
    size_t count = logger.exportAnalytics(oss);
    EXPECT_EQ(count, 3u);
}

TEST(LLMAuditLoggerTest, ExportAnalytics_TimestampIsISO8601) {
    auto logger = makeLogger();
    logger.logEvent(LLMModelAuditEventType::MODEL_LOADED, "m1", {});

    std::ostringstream oss = {};
    logger.exportAnalytics(oss);
    auto obj = json::parse(oss.str());

    const std::string ts = obj.at("timestamp_iso8601").get<std::string>();
    // ISO-8601 UTC: "YYYY-MM-DDTHH:MM:SSZ" — validate structural characters
    // rather than exact length so variant formats don't fail the check.
    EXPECT_GE(ts.size(), 20u);
    EXPECT_EQ(ts.back(), 'Z');
    // Year-month separator at position 4
    EXPECT_EQ(ts[4],  '-');
    // Month-day separator at position 7
    EXPECT_EQ(ts[7],  '-');
    // Date-time separator at position 10
    EXPECT_EQ(ts[10], 'T');
    // Hour-minute separator at position 13
    EXPECT_EQ(ts[13], ':');
}

// ---------------------------------------------------------------------------
// getModelStats
// ---------------------------------------------------------------------------

TEST(LLMAuditLoggerTest, GetModelStats_AggregatesCorrectly) {
    auto logger = makeLogger();

    LLMModelInferenceAudit ok; ok.model_id = "m1"; ok.success = true;
    LLMModelInferenceAudit fail; fail.model_id = "m1"; fail.success = false;
    logger.logInference(ok);
    logger.logInference(ok);
    logger.logInference(fail);
    logger.logPolicyViolation("m1", "r", "rule", "reason", true);

    auto stats = logger.getModelStats("m1");
    EXPECT_EQ(stats.at("inferences").get<int>(),    2);
    EXPECT_EQ(stats.at("failures").get<int>(),      1);
    EXPECT_EQ(stats.at("policy_blocks").get<int>(), 1);
    EXPECT_EQ(stats.at("total_events").get<int>(),  4);
}

// ---------------------------------------------------------------------------
// setEnabled suppresses writes
// ---------------------------------------------------------------------------

TEST(LLMAuditLoggerTest, SetEnabled_False_SuppressesWrites) {
    auto logger = makeLogger();
    logger.setEnabled(false);
    logger.logEvent(LLMModelAuditEventType::MODEL_LOADED, "m1", {});

    auto logs = logger.queryLogs("");
    EXPECT_EQ(logs.size(), 0u);
}

TEST(LLMAuditLoggerTest, SetEnabled_ReEnabled_ResumesWrites) {
    auto logger = makeLogger();
    logger.setEnabled(false);
    logger.logEvent(LLMModelAuditEventType::MODEL_LOADED, "m1", {});
    logger.setEnabled(true);
    logger.logEvent(LLMModelAuditEventType::MODEL_UNLOADED, "m1", {});

    auto logs = logger.queryLogs("m1");
    EXPECT_EQ(logs.size(), 1u); // only the second event
}
