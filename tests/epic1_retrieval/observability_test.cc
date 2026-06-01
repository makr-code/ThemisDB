/**
 * @file observability_test.cc
 * @brief Contract tests for IRetrievalObservability (EPIC 1, sub-issue 1.7).
 *
 * Validates factory construction (no-op provider), span/trace emission,
 * governance evaluation, counter increments, histogram recording, and
 * governance-block callback registration.
 * Production telemetry backend wiring is tracked in sub-issue 1.7 / #5427.
 */

#include "retrieval/include/retrieval_observability.h"

#include <gtest/gtest.h>
#include <memory>
#include <string>

using namespace themis::retrieval;

namespace {

RetrievalSpan makeSpan(const std::string& trace_id = "t-001") {
    RetrievalSpan s;
    s.trace_id   = trace_id;
    s.span_id    = "s-001";
    s.layer      = RetrievalLayer::AnnFrontdoor;
    s.latency_ms = 1.5;
    s.cache_hit  = false;
    s.confidence = 0.9f;
    return s;
}

GovernanceContext makeGovCtx(bool pii = false) {
    GovernanceContext ctx;
    ctx.user_id          = "user-42";
    ctx.query_hash       = "abc123";
    ctx.namespace_scope  = "ns-default";
    ctx.contains_pii     = pii;
    ctx.estimated_cost   = 0.01f;
    return ctx;
}

} // namespace

class RetrievalObservabilityTest : public ::testing::Test {
protected:
    void SetUp() override {
        obs_ = makeNoopObservability();
        ASSERT_NE(obs_, nullptr);
    }

    std::unique_ptr<IRetrievalObservability> obs_;
};

TEST_F(RetrievalObservabilityTest, FactoryReturnsNonNull) {
    EXPECT_NE(obs_, nullptr);
}

TEST_F(RetrievalObservabilityTest, EmitSpanDoesNotThrow) {
    EXPECT_NO_THROW(obs_->emitSpan(makeSpan()));
}

TEST_F(RetrievalObservabilityTest, EmitTraceDoesNotThrow) {
    RetrievalTrace trace;
    trace.trace_id        = "t-001";
    trace.spans           = {makeSpan("t-001")};
    trace.total_latency_ms = 1.5;
    trace.final_confidence = 0.9f;
    EXPECT_NO_THROW(obs_->emitTrace(trace));
}

TEST_F(RetrievalObservabilityTest, CheckGovernanceDefaultAllow) {
    // No-op provider should allow everything.
    GovernanceDecision d = obs_->checkGovernance(makeGovCtx());
    EXPECT_EQ(d, GovernanceDecision::Allow);
}

TEST_F(RetrievalObservabilityTest, CheckGovernancePiiContextDoesNotThrow) {
    EXPECT_NO_THROW(obs_->checkGovernance(makeGovCtx(/*pii=*/true)));
}

TEST_F(RetrievalObservabilityTest, IncrementCounterDoesNotThrow) {
    EXPECT_NO_THROW(obs_->incrementCounter("test.counter"));
    EXPECT_NO_THROW(obs_->incrementCounter("test.counter", 5.0));
    EXPECT_NO_THROW(obs_->incrementCounter("", 0.0));
}

TEST_F(RetrievalObservabilityTest, RecordHistogramDoesNotThrow) {
    EXPECT_NO_THROW(obs_->recordHistogram("latency_ms", 1.2));
    EXPECT_NO_THROW(obs_->recordHistogram("latency_ms", 0.0));
    EXPECT_NO_THROW(obs_->recordHistogram("", -1.0));
}

TEST_F(RetrievalObservabilityTest, OnGovernanceBlockRegistrationDoesNotThrow) {
    bool called = false;
    EXPECT_NO_THROW(obs_->onGovernanceBlock([&](const GovernanceContext&) {
        called = true;
    }));
    (void)called;
}

TEST_F(RetrievalObservabilityTest, EmitMultipleSpansDoesNotThrow) {
    for (int i = 0; i < 10; ++i) {
        EXPECT_NO_THROW(obs_->emitSpan(makeSpan("t-" + std::to_string(i))));
    }
}
