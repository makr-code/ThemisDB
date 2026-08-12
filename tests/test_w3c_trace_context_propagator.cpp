/*
 * Unit tests for W3CTraceContextPropagator — W3C TraceContext standard
 * distributed context propagation for the core module.
 *
 * Covers:
 *   - extract(): parse valid traceparent, set kTraceId + kSpanId in context
 *   - extract(): missing/invalid traceparent → empty root context (no crash)
 *   - extract(): case-insensitive header lookup (W3C HTTP spec)
 *   - extract(): tracestate forwarded as "w3c.tracestate"
 *   - extract(): all-zeros trace-id rejected (invalid per W3C spec)
 *   - extract(): all-zeros parent-id rejected (invalid per W3C spec)
 *   - extract(): too-short traceparent rejected
 *   - extract(): with parent context → child inherits parent attributes
 *   - inject(): formats traceparent from kTraceId + kSpanId
 *   - inject(): no header when kTraceId / kSpanId absent
 *   - inject(): tracestate forwarded when present in context
 *   - round-trip: extract then inject reproduces the same traceparent
 */

#include "core/concerns/w3c_trace_context_propagator.h"
#include "core/concerns/i_context.h"
#include <gtest/gtest.h>
#include <map>
#include <string>

using namespace themis::core::concerns;

// ─────────────────────────────────────────────────────────────────────────────
// Constants
// ─────────────────────────────────────────────────────────────────────────────

static const std::string kValidTraceparent =
    "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01";
static const std::string kExpectedTraceId = "4bf92f3577b34da6a3ce929d0e0e4736";
static const std::string kExpectedSpanId  = "00f067aa0ba902b7";

// ─────────────────────────────────────────────────────────────────────────────
// extract() – valid traceparent
// ─────────────────────────────────────────────────────────────────────────────

TEST(W3CTraceContextPropagatorTest, ExtractValidTraceparentSetsTraceId) {
    std::map<std::string, std::string> headers{{"traceparent", kValidTraceparent}};
    auto ctx = W3CTraceContextPropagator::extract(headers);
    ASSERT_NE(ctx, nullptr);
    EXPECT_EQ(ctx->get(context_keys::kTraceId).value_or(""), kExpectedTraceId);
}

TEST(W3CTraceContextPropagatorTest, ExtractValidTraceparentSetsSpanId) {
    std::map<std::string, std::string> headers{{"traceparent", kValidTraceparent}};
    auto ctx = W3CTraceContextPropagator::extract(headers);
    ASSERT_NE(ctx, nullptr);
    EXPECT_EQ(ctx->get(context_keys::kSpanId).value_or(""), kExpectedSpanId);
}

TEST(W3CTraceContextPropagatorTest, ExtractReturnsNonNullContext) {
    std::map<std::string, std::string> headers{{"traceparent", kValidTraceparent}};
    EXPECT_NE(W3CTraceContextPropagator::extract(headers), nullptr);
}

// ─────────────────────────────────────────────────────────────────────────────
// extract() – missing / invalid traceparent
// ─────────────────────────────────────────────────────────────────────────────

TEST(W3CTraceContextPropagatorTest, ExtractEmptyHeadersReturnsEmptyContext) {
    std::map<std::string, std::string> headers;
    auto ctx = W3CTraceContextPropagator::extract(headers);
    ASSERT_NE(ctx, nullptr);
    EXPECT_FALSE(ctx->has(context_keys::kTraceId));
    EXPECT_FALSE(ctx->has(context_keys::kSpanId));
}

TEST(W3CTraceContextPropagatorTest, ExtractMissingTraceparentReturnsEmptyContext) {
    std::map<std::string, std::string> headers{{"X-Request-Id", "req-42"}};
    auto ctx = W3CTraceContextPropagator::extract(headers);
    ASSERT_NE(ctx, nullptr);
    EXPECT_FALSE(ctx->has(context_keys::kTraceId));
}

TEST(W3CTraceContextPropagatorTest, ExtractMalformedTraceparentIgnored) {
    std::map<std::string, std::string> headers{{"traceparent", "not-a-valid-value"}};
    auto ctx = W3CTraceContextPropagator::extract(headers);
    ASSERT_NE(ctx, nullptr);
    EXPECT_FALSE(ctx->has(context_keys::kTraceId));
    EXPECT_FALSE(ctx->has(context_keys::kSpanId));
}

// ─────────────────────────────────────────────────────────────────────────────
// extract() – W3C spec invalid values
// ─────────────────────────────────────────────────────────────────────────────

TEST(W3CTraceContextPropagatorTest, ExtractAllZerosTraceIdRejected) {
    // All-zeros trace-id is explicitly invalid per W3C TraceContext spec
    std::map<std::string, std::string> headers{
        {"traceparent", "00-00000000000000000000000000000000-00f067aa0ba902b7-01"}
    };
    auto ctx = W3CTraceContextPropagator::extract(headers);
    ASSERT_NE(ctx, nullptr);
    EXPECT_FALSE(ctx->has(context_keys::kTraceId));
    EXPECT_FALSE(ctx->has(context_keys::kSpanId));
}

TEST(W3CTraceContextPropagatorTest, ExtractAllZerosParentIdRejected) {
    // All-zeros parent-id is explicitly invalid per W3C TraceContext spec
    std::map<std::string, std::string> headers{
        {"traceparent", "00-4bf92f3577b34da6a3ce929d0e0e4736-0000000000000000-01"}
    };
    auto ctx = W3CTraceContextPropagator::extract(headers);
    ASSERT_NE(ctx, nullptr);
    EXPECT_FALSE(ctx->has(context_keys::kTraceId));
    EXPECT_FALSE(ctx->has(context_keys::kSpanId));
}

TEST(W3CTraceContextPropagatorTest, ExtractTooShortTraceparentRejected) {
    // One hex digit too short in the trace-id field
    std::map<std::string, std::string> headers{
        {"traceparent", "00-4bf92f3577b34da6a3ce929d0e0e473-00f067aa0ba902b7-01"}
    };
    auto ctx = W3CTraceContextPropagator::extract(headers);
    ASSERT_NE(ctx, nullptr);
    EXPECT_FALSE(ctx->has(context_keys::kTraceId));
}

TEST(W3CTraceContextPropagatorTest, ExtractVersionFFRejected) {
    // W3C spec: version "ff" is the ONLY version explicitly reserved as invalid
    std::map<std::string, std::string> headers{
        {"traceparent", "ff-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01"}
    };
    auto ctx = W3CTraceContextPropagator::extract(headers);
    ASSERT_NE(ctx, nullptr);
    EXPECT_FALSE(ctx->has(context_keys::kTraceId));
}

TEST(W3CTraceContextPropagatorTest, ExtractFutureVersionAccepted) {
    // W3C spec: implementations MUST accept future versions (01-fe) for
    // forward compatibility when the header meets minimum length requirements.
    std::map<std::string, std::string> headers{
        {"traceparent", "01-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01"}
    };
    auto ctx = W3CTraceContextPropagator::extract(headers);
    ASSERT_NE(ctx, nullptr);
    EXPECT_EQ(ctx->get(context_keys::kTraceId).value_or(""), kExpectedTraceId);
    EXPECT_EQ(ctx->get(context_keys::kSpanId).value_or(""),  kExpectedSpanId);
}

// ─────────────────────────────────────────────────────────────────────────────
// extract() – case-insensitive header lookup (W3C HTTP spec)
// ─────────────────────────────────────────────────────────────────────────────

TEST(W3CTraceContextPropagatorTest, ExtractCaseInsensitiveUppercase) {
    std::map<std::string, std::string> headers{{"Traceparent", kValidTraceparent}};
    auto ctx = W3CTraceContextPropagator::extract(headers);
    ASSERT_NE(ctx, nullptr);
    EXPECT_EQ(ctx->get(context_keys::kTraceId).value_or(""), kExpectedTraceId);
}

TEST(W3CTraceContextPropagatorTest, ExtractCaseInsensitiveAllCaps) {
    std::map<std::string, std::string> headers{{"TRACEPARENT", kValidTraceparent}};
    auto ctx = W3CTraceContextPropagator::extract(headers);
    ASSERT_NE(ctx, nullptr);
    EXPECT_EQ(ctx->get(context_keys::kTraceId).value_or(""), kExpectedTraceId);
}

// ─────────────────────────────────────────────────────────────────────────────
// extract() – tracestate forwarding
// ─────────────────────────────────────────────────────────────────────────────

TEST(W3CTraceContextPropagatorTest, ExtractTracestateStored) {
    std::map<std::string, std::string> headers{
        {"traceparent", kValidTraceparent},
        {"tracestate",  "vendor=value1,app=value2"}
    };
    auto ctx = W3CTraceContextPropagator::extract(headers);
    ASSERT_NE(ctx, nullptr);
    EXPECT_EQ(ctx->get("w3c.tracestate").value_or(""), "vendor=value1,app=value2");
}

TEST(W3CTraceContextPropagatorTest, ExtractNoTracestateKeyAbsent) {
    std::map<std::string, std::string> headers{{"traceparent", kValidTraceparent}};
    auto ctx = W3CTraceContextPropagator::extract(headers);
    ASSERT_NE(ctx, nullptr);
    EXPECT_FALSE(ctx->has("w3c.tracestate"));
}

// ─────────────────────────────────────────────────────────────────────────────
// extract() – with parent context
// ─────────────────────────────────────────────────────────────────────────────

TEST(W3CTraceContextPropagatorTest, ExtractWithParentCreatesChild) {
    auto parent = SimpleContext::create();
    parent->set(context_keys::kService, "upstream-service");
    parent->set(context_keys::kRequestId, "req-100");

    std::map<std::string, std::string> headers{{"traceparent", kValidTraceparent}};
    auto ctx = W3CTraceContextPropagator::extract(headers, parent);
    ASSERT_NE(ctx, nullptr);

    // W3C trace IDs overwrite/populate the child.
    EXPECT_EQ(ctx->get(context_keys::kTraceId).value_or(""), kExpectedTraceId);
    EXPECT_EQ(ctx->get(context_keys::kSpanId).value_or(""),  kExpectedSpanId);

    // Parent attributes are inherited.
    EXPECT_EQ(ctx->get(context_keys::kService).value_or(""),   "upstream-service");
    EXPECT_EQ(ctx->get(context_keys::kRequestId).value_or(""), "req-100");
}

TEST(W3CTraceContextPropagatorTest, ExtractChildWriteDoesNotMutateParent) {
    auto parent = SimpleContext::create();
    parent->set(context_keys::kService, "svc");

    std::map<std::string, std::string> headers{{"traceparent", kValidTraceparent}};
    auto child = W3CTraceContextPropagator::extract(headers, parent);

    // Parent must not have trace context set by child extraction.
    EXPECT_FALSE(parent->has(context_keys::kTraceId));
    EXPECT_FALSE(parent->has(context_keys::kSpanId));
}

// ─────────────────────────────────────────────────────────────────────────────
// inject() – basic
// ─────────────────────────────────────────────────────────────────────────────

TEST(W3CTraceContextPropagatorTest, InjectFormatsTraceparentHeader) {
    auto ctx = SimpleContext::create();
    ctx->set(context_keys::kTraceId, kExpectedTraceId);
    ctx->set(context_keys::kSpanId,  kExpectedSpanId);

    std::map<std::string, std::string> headers;
    W3CTraceContextPropagator::inject(*ctx, headers);

    ASSERT_NE(headers.count("traceparent"), 0u);
    EXPECT_EQ(headers.at("traceparent"),
              "00-" + kExpectedTraceId + "-" + kExpectedSpanId + "-01");
}

TEST(W3CTraceContextPropagatorTest, InjectNoHeaderWhenTraceIdAbsent) {
    auto ctx = SimpleContext::create();
    // kSpanId set but no kTraceId
    ctx->set(context_keys::kSpanId, kExpectedSpanId);

    std::map<std::string, std::string> headers;
    W3CTraceContextPropagator::inject(*ctx, headers);
    EXPECT_EQ(headers.count("traceparent"), 0u);
}

TEST(W3CTraceContextPropagatorTest, InjectNoHeaderWhenSpanIdAbsent) {
    auto ctx = SimpleContext::create();
    // kTraceId set but no kSpanId
    ctx->set(context_keys::kTraceId, kExpectedTraceId);

    std::map<std::string, std::string> headers;
    W3CTraceContextPropagator::inject(*ctx, headers);
    EXPECT_EQ(headers.count("traceparent"), 0u);
}

TEST(W3CTraceContextPropagatorTest, InjectNoHeaderWhenContextEmpty) {
    auto ctx = SimpleContext::create();
    std::map<std::string, std::string> headers;
    W3CTraceContextPropagator::inject(*ctx, headers);
    EXPECT_EQ(headers.count("traceparent"), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// inject() – tracestate forwarding
// ─────────────────────────────────────────────────────────────────────────────

TEST(W3CTraceContextPropagatorTest, InjectForwardsTracestate) {
    auto ctx = SimpleContext::create();
    ctx->set(context_keys::kTraceId, kExpectedTraceId);
    ctx->set(context_keys::kSpanId,  kExpectedSpanId);
    ctx->set("w3c.tracestate", "vendor=abc");

    std::map<std::string, std::string> headers;
    W3CTraceContextPropagator::inject(*ctx, headers);

    ASSERT_NE(headers.count("tracestate"), 0u);
    EXPECT_EQ(headers.at("tracestate"), "vendor=abc");
}

TEST(W3CTraceContextPropagatorTest, InjectNoTracestateHeaderWhenAbsent) {
    auto ctx = SimpleContext::create();
    ctx->set(context_keys::kTraceId, kExpectedTraceId);
    ctx->set(context_keys::kSpanId,  kExpectedSpanId);

    std::map<std::string, std::string> headers;
    W3CTraceContextPropagator::inject(*ctx, headers);
    EXPECT_EQ(headers.count("tracestate"), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Round-trip: extract → inject
// ─────────────────────────────────────────────────────────────────────────────

TEST(W3CTraceContextPropagatorTest, RoundTripExtractInjectPreservesTraceparent) {
    std::map<std::string, std::string> inbound{{"traceparent", kValidTraceparent}};
    auto ctx = W3CTraceContextPropagator::extract(inbound);

    std::map<std::string, std::string> outbound;
    W3CTraceContextPropagator::inject(*ctx, outbound);

    ASSERT_NE(outbound.count("traceparent"), 0u);
    EXPECT_EQ(outbound.at("traceparent"), kValidTraceparent);
}

TEST(W3CTraceContextPropagatorTest, RoundTripWithTracestatePreservesState) {
    std::map<std::string, std::string> inbound{
        {"traceparent", kValidTraceparent},
        {"tracestate",  "rojo=00f067aa0ba902b7,congo=t61rcWkgMzE"}
    };
    auto ctx = W3CTraceContextPropagator::extract(inbound);

    std::map<std::string, std::string> outbound;
    W3CTraceContextPropagator::inject(*ctx, outbound);

    EXPECT_EQ(outbound.at("tracestate"),
              "rojo=00f067aa0ba902b7,congo=t61rcWkgMzE");
}

// ─────────────────────────────────────────────────────────────────────────────
// toTraceContext() integration — kSpanId now surfaces in TraceContext
// ─────────────────────────────────────────────────────────────────────────────

TEST(W3CTraceContextPropagatorTest, ToTraceContextPopulatesSpanId) {
    std::map<std::string, std::string> headers{{"traceparent", kValidTraceparent}};
    auto ctx = W3CTraceContextPropagator::extract(headers);

    TraceContext tc = ctx->toTraceContext();
    EXPECT_EQ(tc.trace_id, kExpectedTraceId);
    EXPECT_EQ(tc.span_id,  kExpectedSpanId);
}
