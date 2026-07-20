/**
 * @file test_llm_p1_observability.cpp
 * @brief P1.3 observability and audit correlation tests.
 *
 * Test IDs:
 *   COR-01 — LLMCorrelationContext::generate() produces valid W3C IDs
 *   COR-02 — fromTraceparent() parses a valid header
 *   COR-03 — fromTraceparent() returns invalid context for malformed input
 *   COR-04 — ensure() returns self when already valid; generates new when not
 *   COR-05 — toTraceparent() round-trips correctly
 *   AUD-01 — AIDecisionAudit::toJson() serialises trace_id and span_id
 *   AUD-02 — AIDecisionAudit::fromJson() deserialises trace_id and span_id
 *   AUD-03 — fromJson() tolerates missing trace fields (backward compat)
 *   AUD-04 — Trace fields survive a full to/from JSON round-trip
 */

#include <gtest/gtest.h>

#include "llm/llm_correlation_context.h"
#include "llm/ai_decision_auditor.h"

#include <string>

using themis::llm::LLMCorrelationContext;
using themis::llm::AIDecisionAudit;

// ============================================================================
// COR — LLMCorrelationContext tests
// ============================================================================

TEST(P1CorrelationContext, GenerateProducesValidIds) {
    const auto ctx = LLMCorrelationContext::generate();

    EXPECT_TRUE(ctx.isValid())
        << "Generated context must be valid (trace_id=32 chars, span_id=16 chars)";
    EXPECT_EQ(ctx.trace_id.size(), 32u);
    EXPECT_EQ(ctx.span_id.size(),  16u);

    // All characters must be lower-case hex.
    for (char c : ctx.trace_id) {
        EXPECT_TRUE(std::isxdigit(static_cast<unsigned char>(c)) != 0)
            << "Non-hex char '" << c << "' in trace_id";
    }
    for (char c : ctx.span_id) {
        EXPECT_TRUE(std::isxdigit(static_cast<unsigned char>(c)) != 0)
            << "Non-hex char '" << c << "' in span_id";
    }
}

TEST(P1CorrelationContext, TwoGeneratedContextsDiffer) {
    // IDs should not collide in practice; verify two consecutive calls differ.
    const auto a = LLMCorrelationContext::generate();
    const auto b = LLMCorrelationContext::generate();
    EXPECT_NE(a.trace_id, b.trace_id) << "Consecutive generated trace IDs should differ";
}

TEST(P1CorrelationContext, FromTraceparentParsesValidHeader) {
    const std::string header =
        "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01";

    const auto ctx = LLMCorrelationContext::fromTraceparent(header);

    EXPECT_TRUE(ctx.isValid());
    EXPECT_EQ(ctx.trace_id, "4bf92f3577b34da6a3ce929d0e0e4736");
    EXPECT_EQ(ctx.span_id,  "00f067aa0ba902b7");
}

TEST(P1CorrelationContext, FromTraceparentInvalidInputReturnsEmpty) {
    // Short/malformed inputs must not produce a valid context.
    EXPECT_FALSE(LLMCorrelationContext::fromTraceparent("").isValid());
    EXPECT_FALSE(LLMCorrelationContext::fromTraceparent("not-a-traceparent").isValid());
    EXPECT_FALSE(LLMCorrelationContext::fromTraceparent("00-tooshort-abc-01").isValid());
}

TEST(P1CorrelationContext, EnsureReturnsSelfWhenValid) {
    const auto ctx = LLMCorrelationContext::generate();
    const auto ensured = ctx.ensure();

    EXPECT_EQ(ctx.trace_id, ensured.trace_id);
    EXPECT_EQ(ctx.span_id,  ensured.span_id);
}

TEST(P1CorrelationContext, EnsureGeneratesWhenInvalid) {
    LLMCorrelationContext empty;
    EXPECT_FALSE(empty.isValid());

    const auto ensured = empty.ensure();
    EXPECT_TRUE(ensured.isValid())
        << "ensure() on an empty context must produce a valid generated context";
}

TEST(P1CorrelationContext, ToTraceparentRoundTrips) {
    const auto original = LLMCorrelationContext::generate();
    const std::string header = original.toTraceparent();

    EXPECT_FALSE(header.empty());
    EXPECT_EQ(header.substr(0, 3), "00-");

    const auto parsed = LLMCorrelationContext::fromTraceparent(header);
    EXPECT_TRUE(parsed.isValid());
    EXPECT_EQ(parsed.trace_id, original.trace_id);
    EXPECT_EQ(parsed.span_id,  original.span_id);
}

TEST(P1CorrelationContext, ToTraceparentEmptyForInvalidContext) {
    LLMCorrelationContext empty;
    EXPECT_TRUE(empty.toTraceparent().empty())
        << "An invalid context must produce an empty traceparent string";
}

// ============================================================================
// AUD — AIDecisionAudit trace field tests
// ============================================================================

TEST(P1AuditCorrelation, ToJsonSerializesTraceFields) {
    AIDecisionAudit audit;
    audit.decision_id  = "d-001";
    audit.trace_id     = "4bf92f3577b34da6a3ce929d0e0e4736";
    audit.span_id      = "00f067aa0ba902b7";
    audit.query        = "test query";
    audit.response     = "test response";

    const auto j = audit.toJson();

    EXPECT_EQ(j.value("trace_id", std::string{}), "4bf92f3577b34da6a3ce929d0e0e4736");
    EXPECT_EQ(j.value("span_id",  std::string{}), "00f067aa0ba902b7");
}

TEST(P1AuditCorrelation, FromJsonDeserializesTraceFields) {
    nlohmann::json j;
    j["decision_id"]  = "d-002";
    j["trace_id"]     = "aabbccddeeff00112233445566778899";
    j["span_id"]      = "1122334455667788";
    j["query"]        = "q";
    j["response"]     = "r";
    j["latency_ms"]   = 42;
    j["token_count"]  = 10;

    const auto audit = AIDecisionAudit::fromJson(j);

    EXPECT_EQ(audit.trace_id, "aabbccddeeff00112233445566778899");
    EXPECT_EQ(audit.span_id,  "1122334455667788");
}

TEST(P1AuditCorrelation, FromJsonToleratesMissingTraceFields) {
    // Older audit records without trace_id/span_id must deserialise cleanly.
    nlohmann::json j;
    j["decision_id"] = "d-003";
    j["query"]       = "q";
    j["response"]    = "r";
    // Note: no "trace_id" or "span_id" keys.

    ASSERT_NO_THROW({
        const auto audit = AIDecisionAudit::fromJson(j);
        EXPECT_TRUE(audit.trace_id.empty())
            << "Missing trace_id in JSON must default to empty string";
        EXPECT_TRUE(audit.span_id.empty())
            << "Missing span_id in JSON must default to empty string";
    });
}

TEST(P1AuditCorrelation, TraceFieldsRoundTripViaJson) {
    AIDecisionAudit original;
    original.decision_id = "d-004";
    original.trace_id    = "deadbeefdeadbeefdeadbeefdeadbeef";
    original.span_id     = "cafebabe12345678";
    original.query       = "round-trip check";
    original.response    = "ok";

    const auto restored = AIDecisionAudit::fromJson(original.toJson());

    EXPECT_EQ(restored.trace_id, original.trace_id);
    EXPECT_EQ(restored.span_id,  original.span_id);
    EXPECT_EQ(restored.decision_id, original.decision_id);
}
