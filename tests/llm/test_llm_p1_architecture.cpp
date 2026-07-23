/**
 * @file test_llm_p1_architecture.cpp
 * @brief P1 architecture contract tests:
 *   P1.1 — LLMQueryContext MVCC snapshot context
 *   P1.2 — LLMGenerateOperator as first-class plan operator
 *
 * Test IDs:
 *   CTX-01 — LLMQueryContext::fromSnapshot sets isolation_mode correctly
 *   CTX-02 — LLMQueryContext::withoutSnapshot sets no-snapshot mode
 *   CTX-03 — hasSnapshot() returns true iff snapshot_ts is non-zero
 *   CTX-04 — trace_id / span_id propagated through context factory
 *   CTX-05 — Zero HLCTimestamp is treated as "no snapshot"
 *   OPR-01 — LLMGenerateOperator is never deterministic
 *   OPR-02 — Cost model accessible and mutable
 *   OPR-03 — Budget check returns true within limit and false beyond limit
 *   OPR-04 — Fallback policy override
 *   OPR-05 — toExplainString contains required fields
 *   OPR-06 — requiresSnapshotContext defaults to true, overridable
 *   OPR-07 — Audit enabled by default, disableable
 *   VIS-01 — PlanNodeType::LLMGenerate renders as "LLMGenerate" in visualizer
 */

#include <gtest/gtest.h>

#include "aql/llm_query_context.h"
#include "query/llm_generate_operator.h"
#include "query/query_plan_visualizer.h"
#include "storage/hlc.h"

#include <chrono>

using namespace themis;
using namespace themis::aql;
using namespace themis::query;

// ============================================================================
// CTX — LLMQueryContext tests
// ============================================================================

TEST(P1LLMQueryContext, FromSnapshotSetsIsolationMode) {
    const HLCTimestamp ts(12345678ULL);
    const LLMQueryContext ctx = LLMQueryContext::fromSnapshot(ts);

    EXPECT_EQ(ctx.isolation_mode, "snapshot-isolated");
    EXPECT_EQ(ctx.snapshot_ts.value, 12345678ULL);
    EXPECT_TRUE(ctx.hasSnapshot());
}

TEST(P1LLMQueryContext, WithoutSnapshotSetsNoSnapshotMode) {
    const LLMQueryContext ctx = LLMQueryContext::withoutSnapshot();

    EXPECT_EQ(ctx.isolation_mode, "no-snapshot");
    EXPECT_EQ(ctx.snapshot_ts.value, 0ULL);
    EXPECT_FALSE(ctx.hasSnapshot());
}

TEST(P1LLMQueryContext, HasSnapshotOnlyForNonZeroTimestamp) {
    EXPECT_TRUE(LLMQueryContext::fromSnapshot(HLCTimestamp(1)).hasSnapshot());
    EXPECT_FALSE(LLMQueryContext::fromSnapshot(HLCTimestamp(0)).hasSnapshot());
    EXPECT_FALSE(LLMQueryContext::withoutSnapshot().hasSnapshot());
}

TEST(P1LLMQueryContext, TraceAndSpanIdPropagated) {
    const HLCTimestamp ts(99ULL);
    const LLMQueryContext ctx =
        LLMQueryContext::fromSnapshot(ts, "trace-abc-def", "span-1234");

    EXPECT_EQ(ctx.trace_id, "trace-abc-def");
    EXPECT_EQ(ctx.span_id,  "span-1234");
}

TEST(P1LLMQueryContext, ZeroTimestampIsNoSnapshot) {
    // HLCTimestamp{0} is the default-constructed value.
    const LLMQueryContext ctx = LLMQueryContext::fromSnapshot(HLCTimestamp(0));
    // Even fromSnapshot with ts=0 yields hasSnapshot=false because value == 0.
    EXPECT_FALSE(ctx.hasSnapshot())
        << "Zero HLCTimestamp must not be treated as a meaningful snapshot";
}

// ============================================================================
// OPR — LLMGenerateOperator tests
// ============================================================================

namespace {

LLMGenerateOperator makeOperator(uint32_t budget_ms = 5000) {
    aql::LLMQueryContext ctx =
    aql::LLMQueryContext::fromSnapshot(HLCTimestamp(42ULL));
    LLMOperatorCost cost;
    cost.latency_budget_ms = budget_ms;
    return LLMGenerateOperator(std::move(ctx), cost, "test-model");
}

} // namespace

TEST(P1LLMGenerateOperator, IsNeverDeterministic) {
    const auto op = makeOperator();
    EXPECT_FALSE(op.isDeterministic())
        << "LLM operators are inherently probabilistic and must never report deterministic=true";
}

TEST(P1LLMGenerateOperator, CostModelAccessibleAndMutable) {
    auto op = makeOperator();
    op.cost().estimated_input_tokens  = 1024;
    op.cost().estimated_output_tokens = 512;

    EXPECT_EQ(op.cost().estimated_input_tokens,  1024u);
    EXPECT_EQ(op.cost().estimated_output_tokens, 512u);
}

TEST(P1LLMGenerateOperator, BudgetCheckWithinLimit) {
    const auto op = makeOperator(/*budget_ms=*/2000);
    using ms = std::chrono::milliseconds;

    EXPECT_TRUE(op.withinBudget(ms{0}));
    EXPECT_TRUE(op.withinBudget(ms{1999}));
    EXPECT_FALSE(op.withinBudget(ms{2000}));
    EXPECT_FALSE(op.withinBudget(ms{9999}));
}

TEST(P1LLMGenerateOperator, UnlimitedBudgetNeverExpires) {
    const auto op = makeOperator(/*budget_ms=*/0);  // 0 = unlimited
    using ms = std::chrono::milliseconds;

    EXPECT_TRUE(op.withinBudget(ms{999999}))
        << "Budget ms=0 must be treated as unlimited";
}

TEST(P1LLMGenerateOperator, FallbackPolicyOverride) {
    auto op = makeOperator();
    EXPECT_EQ(op.fallbackPolicy(), LLMFallbackPolicy::ReturnRetrievalOnly)
        << "Default fallback must be ReturnRetrievalOnly";

    op.setFallbackPolicy(LLMFallbackPolicy::PropagateError);
    EXPECT_EQ(op.fallbackPolicy(), LLMFallbackPolicy::PropagateError);
}

TEST(P1LLMGenerateOperator, ExplainStringContainsRequiredFields) {
    auto op = makeOperator(/*budget_ms=*/10000);
    op.setFallbackPolicy(LLMFallbackPolicy::ReturnRetrievalOnly);

    const std::string explain = op.toExplainString();

    EXPECT_NE(explain.find("LLMGenerate"),        std::string::npos) << "Must contain operator name";
    EXPECT_NE(explain.find("model=test-model"),   std::string::npos) << "Must contain model id";
    EXPECT_NE(explain.find("budget=10000ms"),     std::string::npos) << "Must contain budget";
    EXPECT_NE(explain.find("deterministic=false"),std::string::npos) << "Must state non-deterministic";
    EXPECT_NE(explain.find("snapshot="),          std::string::npos) << "Must contain snapshot mode";
    EXPECT_NE(explain.find("fallback="),          std::string::npos) << "Must contain fallback policy";
    EXPECT_NE(explain.find("audit=on"),           std::string::npos) << "Must contain audit flag";
}

TEST(P1LLMGenerateOperator, RequiresSnapshotContextDefaultsToTrue) {
    auto op = makeOperator();
    EXPECT_TRUE(op.requiresSnapshotContext())
        << "P1.1: snapshot context must be required by default";

    op.setRequireSnapshotContext(false);
    EXPECT_FALSE(op.requiresSnapshotContext());
}

TEST(P1LLMGenerateOperator, AuditEnabledByDefault) {
    auto op = makeOperator();
    EXPECT_TRUE(op.auditEnabled())
        << "Audit must be on by default for governance compliance";

    op.setAuditEnabled(false);
    EXPECT_FALSE(op.auditEnabled());
}

// ============================================================================
// VIS — QueryPlanVisualizer LLMGenerate node type
// ============================================================================

TEST(P1QueryPlanVisualizer, LLMGenerateRendersAsLLMGenerate) {
    // planNodeTypeName is a static method; call it directly via the enum value.
    const std::string name =
        QueryPlanVisualizer::planNodeTypeName(PlanNodeType::LLMGenerate);

    EXPECT_EQ(name, "LLMGenerate")
        << "PlanNodeType::LLMGenerate must render as the string 'LLMGenerate'";
}

TEST(P1QueryPlanVisualizer, TensorContractionRendersCorrectly) {
    // Regression guard: TensorContraction was missing from the switch before P1.
    const std::string name =
        QueryPlanVisualizer::planNodeTypeName(PlanNodeType::TensorContraction);
    EXPECT_EQ(name, "TensorContraction");
}
