/**
 * @file test_llm_p2_strategic.cpp
 * @brief P2 strategic architecture tests.
 *
 * Test IDs:
 *
 * Modular RAG Pipeline (P2.1):
 *   MRP-01 — All five stage handlers are called in order
 *   MRP-02 — Pipeline returns success when all stages succeed
 *   MRP-03 — Pipeline halts at error stage when fail_closed=true
 *   MRP-04 — Provenance chain accumulates across stages
 *   MRP-05 — W3C correlation context is auto-generated when absent
 *   MRP-06 — Config validates: null handler throws invalid_argument
 *
 * Reproducibility (P2.3):
 *   RPR-01 — Deterministic snapshot enforces temperature=0, top_k=1, rng_seed
 *   RPR-02 — Audit snapshot enforces rng_seed presence
 *   RPR-03 — Balanced snapshot passes validation with defaults
 *   RPR-04 — Snapshot round-trips via JSON
 *   RPR-05 — Policy blocks Creative mode in production
 *   RPR-06 — Policy blocks disallowed modes
 *   RPR-07 — Mode names round-trip through parseReproducibilityMode
 *
 * Multi-tenant isolation (P2.2):
 *   MTI-01 — BlockTable carries tenant_id field
 *   MTI-02 — tenantKey() produces distinct values for distinct tenants
 *   MTI-03 — tenantKey() is stable (same inputs → same output)
 *   MTI-04 — tenantKey() does not collide with plain user_id
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "rag/modular_rag_pipeline.h"
#include "llm/llm_reproducibility.h"
#include "llm/paged_kv_cache_manager.h"
#include "llm/token_quota_manager.h"
#include "llm/llm_correlation_context.h"

#include <string>
#include <vector>

using namespace themis::rag;
using namespace themis::llm;

// ============================================================================
// Helpers
// ============================================================================

namespace {

/// Build a stage handler that appends its name to provenance and returns success.
RAGStageHandler makePassthroughHandler(
    RAGStageId stage_id,
    std::vector<std::string>& call_log)
{
    return [stage_id, &call_log](ModularRAGContext& ctx) -> StageResult {
        call_log.push_back(ragStageIdName(stage_id));
        ctx.addProvenance(stage_id, "ok");
        StageResult r;
        r.stage  = stage_id;
        r.status = StageStatus::Success;
        if (stage_id == RAGStageId::Generate) {
            InferenceResponse resp;
            resp.success = true;
            resp.text    = "answer";
            r.inference_response = std::move(resp);
        }
        return r;
    };
}

/// Build a stage handler that always returns Error.
RAGStageHandler makeErrorHandler(RAGStageId stage_id) {
    return [stage_id](ModularRAGContext& /*ctx*/) -> StageResult {
        StageResult r;
        r.stage      = stage_id;
        r.status     = StageStatus::Error;
        r.diagnostic = std::string("forced error at ") + ragStageIdName(stage_id);
        return r;
    };
}

/// Build a complete passthrough config with a call log.
ModularRAGPipelineConfig makePassthroughConfig(std::vector<std::string>& log) {
    ModularRAGPipelineConfig cfg;
    cfg.retrieve_fn = makePassthroughHandler(RAGStageId::Retrieve, log);
    cfg.rerank_fn   = makePassthroughHandler(RAGStageId::Rerank,   log);
    cfg.validate_fn = makePassthroughHandler(RAGStageId::Validate, log);
    cfg.assemble_fn = makePassthroughHandler(RAGStageId::Assemble, log);
    cfg.generate_fn = makePassthroughHandler(RAGStageId::Generate, log);
    return cfg;
}

} // anonymous namespace

// ============================================================================
// MRP — Modular RAG Pipeline
// ============================================================================

TEST(P2ModularRAG, AllStagesCalledInOrder) {
    std::vector<std::string> log;
    ModularRAGPipeline pipeline{makePassthroughConfig(log)};

    pipeline.run("test query", "tenant-1");

    ASSERT_EQ(log.size(), 5u);
    EXPECT_EQ(log[0], "Retrieve");
    EXPECT_EQ(log[1], "Rerank");
    EXPECT_EQ(log[2], "Validate");
    EXPECT_EQ(log[3], "Assemble");
    EXPECT_EQ(log[4], "Generate");
}

TEST(P2ModularRAG, SuccessWhenAllStagesPass) {
    std::vector<std::string> log;
    ModularRAGPipeline pipeline{makePassthroughConfig(log)};

    const auto result = pipeline.run("query", "t1");

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.answer, "answer");
    EXPECT_TRUE(result.error_message.empty());
}

TEST(P2ModularRAG, HaltsAtErrorStageWhenFailClosed) {
    std::vector<std::string> log;
    ModularRAGPipelineConfig cfg = makePassthroughConfig(log);
    // Replace Validate stage with an error handler.
    cfg.validate_fn = makeErrorHandler(RAGStageId::Validate);

    ModularRAGPipeline pipeline{cfg};
    const auto result = pipeline.run("query", "t1");

    EXPECT_FALSE(result.success);
    EXPECT_THAT(result.error_message,
        ::testing::HasSubstr("forced error at Validate"));

    // Assemble and Generate must NOT have been called.
    for (const auto& s : log) {
        EXPECT_NE(s, "Assemble") << "Assemble must not run after Validate error";
        EXPECT_NE(s, "Generate") << "Generate must not run after Validate error";
    }
}

TEST(P2ModularRAG, ProvenanceChainAccumulates) {
    std::vector<std::string> log;
    ModularRAGPipeline pipeline{makePassthroughConfig(log)};

    const auto result = pipeline.run("q", "t");

    ASSERT_EQ(result.provenance_chain.size(), 5u);
    EXPECT_EQ(result.provenance_chain[0], "Retrieve:ok");
    EXPECT_EQ(result.provenance_chain[4], "Generate:ok");
}

TEST(P2ModularRAG, CorrelationAutoGeneratedWhenAbsent) {
    std::vector<std::string> log;
    ModularRAGPipeline pipeline{makePassthroughConfig(log)};

    // Pass an invalid (empty) correlation context.
    LLMCorrelationContext empty;
    EXPECT_FALSE(empty.isValid());

    const auto result = pipeline.run("q", "t", empty);

    EXPECT_TRUE(result.correlation.isValid())
        << "Pipeline must auto-generate a valid correlation context";
}

TEST(P2ModularRAG, NullHandlerThrows) {
    ModularRAGPipelineConfig cfg;
    cfg.rerank_fn   = [](ModularRAGContext&) -> StageResult { return {}; };
    cfg.validate_fn = [](ModularRAGContext&) -> StageResult { return {}; };
    cfg.assemble_fn = [](ModularRAGContext&) -> StageResult { return {}; };
    cfg.generate_fn = [](ModularRAGContext&) -> StageResult { return {}; };
    // retrieve_fn intentionally left null.

    EXPECT_THROW(ModularRAGPipeline{cfg}, std::invalid_argument);
}

// ============================================================================
// RPR — Reproducibility
// ============================================================================

TEST(P2Reproducibility, DeterministicSnapshotEnforcesInvariants) {
    const auto snap = LLMInferenceParameterSnapshot::makeDeterministic(42, "model-x");

    EXPECT_EQ(snap.mode, LLMReproducibilityMode::Deterministic);
    EXPECT_FLOAT_EQ(snap.temperature, 0.0f);
    EXPECT_EQ(snap.top_k, 1);
    EXPECT_TRUE(snap.rng_seed.has_value());
    EXPECT_EQ(*snap.rng_seed, 42u);
    EXPECT_NO_THROW(snap.validate());
}

TEST(P2Reproducibility, AuditSnapshotRequiresSeed) {
    auto snap = LLMInferenceParameterSnapshot::makeAudit(7, "model-y");
    EXPECT_NO_THROW(snap.validate());

    // Remove the seed — validation must fail.
    snap.rng_seed.reset();
    EXPECT_THROW(snap.validate(), std::invalid_argument);
}

TEST(P2Reproducibility, BalancedSnapshotPassesValidation) {
    const auto snap = LLMInferenceParameterSnapshot::makeBalanced("model-z");
    EXPECT_NO_THROW(snap.validate());
    EXPECT_EQ(snap.mode, LLMReproducibilityMode::Balanced);
}

TEST(P2Reproducibility, SnapshotRoundTripsViaJson) {
    const auto original = LLMInferenceParameterSnapshot::makeDeterministic(99, "test-model");
    const auto restored = LLMInferenceParameterSnapshot::fromJson(original.toJson());

    EXPECT_EQ(restored.mode, original.mode);
    EXPECT_FLOAT_EQ(restored.temperature, original.temperature);
    EXPECT_EQ(restored.top_k, original.top_k);
    ASSERT_TRUE(restored.rng_seed.has_value());
    EXPECT_EQ(*restored.rng_seed, 99u);
    EXPECT_EQ(restored.model_id, "test-model");
}

TEST(P2Reproducibility, PolicyBlocksCreativeInProduction) {
    LLMReproducibilityPolicy policy;
    policy.disallow_creative_in_production = true;

    auto snap = LLMInferenceParameterSnapshot::makeBalanced();
    snap.mode      = LLMReproducibilityMode::Creative;
    snap.mode_name = reproducibilityModeName(snap.mode);
    snap.temperature = 1.2f;

    const std::string err = policy.check(snap);
    EXPECT_FALSE(err.empty()) << "Policy must reject Creative mode in production";
    EXPECT_THAT(err, ::testing::HasSubstr("Creative"));
}

TEST(P2Reproducibility, PolicyBlocksDisallowedMode) {
    LLMReproducibilityPolicy policy;
    policy.allowed_modes = {LLMReproducibilityMode::Deterministic,
                            LLMReproducibilityMode::Audit};

    auto balanced = LLMInferenceParameterSnapshot::makeBalanced();
    const std::string err = policy.check(balanced);
    EXPECT_FALSE(err.empty()) << "Balanced mode not in allowed list — must be rejected";
}

TEST(P2Reproducibility, ModeNamesRoundTrip) {
    const std::vector<LLMReproducibilityMode> all_modes = {
        LLMReproducibilityMode::Deterministic,
        LLMReproducibilityMode::Audit,
        LLMReproducibilityMode::Balanced,
        LLMReproducibilityMode::Creative,
        LLMReproducibilityMode::Custom,
    };
    for (const auto mode : all_modes) {
        const std::string name = reproducibilityModeName(mode);
        EXPECT_EQ(parseReproducibilityMode(name), mode)
            << "Mode name '" << name << "' did not round-trip correctly";
    }
}

// ============================================================================
// MTI — Multi-tenant isolation
// ============================================================================

TEST(P2MultiTenantIsolation, BlockTableCarriesTenantId) {
    PagedKVCacheManager::BlockTable bt;
    bt.tenant_id = "acme-corp";

    EXPECT_EQ(bt.tenant_id, "acme-corp");
    EXPECT_EQ(bt.sequence_id, 0u);  // other fields still default-initialised
}

TEST(P2MultiTenantIsolation, TenantKeyIsDistinctAcrossTenants) {
    const auto key_a = TokenQuotaManager::tenantKey("acme", "model-x");
    const auto key_b = TokenQuotaManager::tenantKey("beta", "model-x");
    EXPECT_NE(key_a, key_b);
}

TEST(P2MultiTenantIsolation, TenantKeyIsStable) {
    EXPECT_EQ(TokenQuotaManager::tenantKey("t1", "m"),
              TokenQuotaManager::tenantKey("t1", "m"));
}

TEST(P2MultiTenantIsolation, TenantKeyDoesNotCollideWithUserId) {
    const auto tenant_key = TokenQuotaManager::tenantKey("alice", "model-x");
    // Plain usage with "alice" as user_id would not have the "__tenant__:" prefix.
    EXPECT_THAT(tenant_key, ::testing::HasSubstr("__tenant__:"));
    // Should not equal a plain user_id key.
    EXPECT_NE(tenant_key, "alice");
}
