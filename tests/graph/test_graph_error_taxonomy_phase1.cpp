/**
 * @file test_graph_error_taxonomy_phase1.cpp
 * @brief Unit tests for Graph Error Taxonomy Phase 1 contracts
 *
 * Validates:
 * - Error code classification (DENIAL, FALLBACK, REASONING_CONFLICT)
 * - Error description lookup completeness
 * - Category consistency
 * - Determinism of error functions
 */

#include "gtest/gtest.h"
#include "graph/graph_error_taxonomy.h"
#include <unordered_set>

namespace themis {
namespace graph {
namespace test {

// ─────────────────────────────────────────────────────────────────────────────
// Test Fixture
// ─────────────────────────────────────────────────────────────────────────────

class GraphErrorTaxonomyTest : public ::testing::Test {
protected:
    // Phase 1 freeze date
    static constexpr std::string_view PHASE1_DATE = "2026-08-01";
};

// ─────────────────────────────────────────────────────────────────────────────
// Test: Error Category Classification (Frozen Contract)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(GraphErrorTaxonomyTest, DENIALCategoryClassification) {
    // All DENIAL codes should be classified as DENIAL
    const auto denial_codes = {
        GraphErrorCode::OPT_INVALID_QUERY_AST,
        GraphErrorCode::OPT_UNSUPPORTED_QUERY_PATTERN,
        GraphErrorCode::OPT_MISSING_GRAPH_STATISTICS,
        GraphErrorCode::OPT_COST_CALC_OVERFLOW,
        GraphErrorCode::TRAV_VERTEX_NOT_FOUND,
        GraphErrorCode::TRAV_INVALID_EDGE_FILTER,
        GraphErrorCode::TRAV_FRONTIER_OVERFLOW,
        GraphErrorCode::REASON_INVALID_RULE_SYNTAX,
        GraphErrorCode::TENSOR_INVALID_SHAPE,
        GraphErrorCode::DIST_INVALID_SHARD_CONFIG,
    };

    for (auto code : denial_codes) {
        EXPECT_EQ(getErrorCategory(code), ErrorCategory::DENIAL)
            << "Code " << getErrorCodeHex(code) << " should be DENIAL";
    }
}

TEST_F(GraphErrorTaxonomyTest, FALLBACKCategoryClassification) {
    // All FALLBACK codes should enable recovery
    const auto fallback_codes = {
        GraphErrorCode::OPT_GPU_UNAVAILABLE,
        GraphErrorCode::OPT_DISTRIBUTED_PLANNER_OFFLINE,
        GraphErrorCode::TRAV_GPU_MEMORY_EXHAUSTED,
        GraphErrorCode::TRAV_GPU_KERNEL_FAILED,
        GraphErrorCode::TRAV_SHARD_UNAVAILABLE,
        GraphErrorCode::TENSOR_GPU_OP_UNAVAILABLE,
        GraphErrorCode::DIST_SHARD_PEER_OFFLINE,
        GraphErrorCode::DIST_RPC_TIMEOUT,
        GraphErrorCode::CACHE_MISS,
    };

    for (auto code : fallback_codes) {
        EXPECT_EQ(getErrorCategory(code), ErrorCategory::FALLBACK)
            << "Code " << getErrorCodeHex(code) << " should be FALLBACK";
        EXPECT_TRUE(isRecoverableFallback(code))
            << "Code " << getErrorCodeHex(code) << " should be recoverable";
    }
}

TEST_F(GraphErrorTaxonomyTest, REASONINGCONFLICTCategoryClassification) {
    // All REASONING_CONFLICT codes should require operator intervention
    const auto conflict_codes = {
        GraphErrorCode::OPT_UNSATISFIABLE_CONSTRAINTS,
        GraphErrorCode::TRAV_CONSTRAINT_VIOLATION,
        GraphErrorCode::REASON_INFERENCE_CONFLICT,
        GraphErrorCode::REASON_CYCLIC_RULE,
    };

    for (auto code : conflict_codes) {
        EXPECT_EQ(getErrorCategory(code), ErrorCategory::REASONING_CONFLICT)
            << "Code " << getErrorCodeHex(code) << " should be REASONING_CONFLICT";
        EXPECT_TRUE(isReasoningConflict(code))
            << "Code " << getErrorCodeHex(code) << " should be conflict";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Test: Error Description Completeness
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(GraphErrorTaxonomyTest, AllErrorCodesHaveDescriptions) {
    // Every error code except SUCCESS must have a description
    const auto all_codes = {
        GraphErrorCode::OPT_INVALID_QUERY_AST,
        GraphErrorCode::OPT_UNSUPPORTED_QUERY_PATTERN,
        GraphErrorCode::OPT_MISSING_GRAPH_STATISTICS,
        GraphErrorCode::OPT_COST_CALC_OVERFLOW,
        GraphErrorCode::OPT_GPU_UNAVAILABLE,
        GraphErrorCode::OPT_DISTRIBUTED_PLANNER_OFFLINE,
        GraphErrorCode::OPT_UNSATISFIABLE_CONSTRAINTS,
        GraphErrorCode::TRAV_VERTEX_NOT_FOUND,
        GraphErrorCode::TRAV_INVALID_EDGE_FILTER,
        GraphErrorCode::TRAV_FRONTIER_OVERFLOW,
        GraphErrorCode::TRAV_MAX_DEPTH_EXCEEDED,
        GraphErrorCode::TRAV_TIMEOUT,
        GraphErrorCode::TRAV_THREAD_CREATION_FAILED,
        GraphErrorCode::TRAV_GPU_MEMORY_EXHAUSTED,
        GraphErrorCode::TRAV_GPU_KERNEL_FAILED,
        GraphErrorCode::TRAV_SHARD_UNAVAILABLE,
        GraphErrorCode::TRAV_CONSTRAINT_VIOLATION,
        GraphErrorCode::REASON_INVALID_RULE_SYNTAX,
        GraphErrorCode::REASON_ONTOLOGY_LOAD_FAILED,
        GraphErrorCode::REASON_BINDING_FAILED,
        GraphErrorCode::REASON_INFERENCE_CONFLICT,
        GraphErrorCode::REASON_CYCLIC_RULE,
        GraphErrorCode::TENSOR_INVALID_SHAPE,
        GraphErrorCode::TENSOR_FINGERPRINT_FAILED,
        GraphErrorCode::TENSOR_INVALID_THRESHOLD,
        GraphErrorCode::TENSOR_GPU_OP_UNAVAILABLE,
        GraphErrorCode::DIST_INVALID_SHARD_CONFIG,
        GraphErrorCode::DIST_MERGE_FAILED,
        GraphErrorCode::DIST_VERTEX_UNHASHED,
        GraphErrorCode::DIST_SHARD_PEER_OFFLINE,
        GraphErrorCode::DIST_RPC_TIMEOUT,
        GraphErrorCode::CACHE_PLAN_CACHE_FULL,
        GraphErrorCode::POOL_RESOURCE_EXHAUSTED,
        GraphErrorCode::LB_DECISION_FAILED,
        GraphErrorCode::CACHE_MISS,
        GraphErrorCode::GENERIC_ERROR,
        GraphErrorCode::NOT_IMPLEMENTED,
        GraphErrorCode::INTERNAL_INVARIANT_FAILED,
        GraphErrorCode::SYSTEM_RESOURCE_UNAVAILABLE,
    };

    for (auto code : all_codes) {
        auto desc = getErrorDescription(code);
        EXPECT_FALSE(desc.empty())
            << "Code " << getErrorCodeHex(code) << " must have non-empty description";
        EXPECT_NE(desc, "Unknown error code (not in Phase 1 taxonomy)")
            << "Code " << getErrorCodeHex(code) << " description not properly registered";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Test: Error Code Validation
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(GraphErrorTaxonomyTest, ValidErrorCodesPass) {
    // All Phase 1 error codes should validate
    const auto valid_codes = {
        GraphErrorCode::OPT_INVALID_QUERY_AST,
        GraphErrorCode::TRAV_VERTEX_NOT_FOUND,
        GraphErrorCode::REASON_INFERENCE_CONFLICT,
        GraphErrorCode::TENSOR_INVALID_SHAPE,
    };

    for (auto code : valid_codes) {
        EXPECT_TRUE(isValidErrorCode(code))
            << "Valid code " << getErrorCodeHex(code) << " should pass validation";
    }
}

TEST_F(GraphErrorTaxonomyTest, InvalidErrorCodesFail) {
    // Invalid codes should not validate
    EXPECT_FALSE(isValidErrorCode(static_cast<GraphErrorCode>(0xDEADBEEF)));
    EXPECT_FALSE(isValidErrorCode(static_cast<GraphErrorCode>(0xFFFFFFFF)));
    EXPECT_FALSE(isValidErrorCode(static_cast<GraphErrorCode>(0x08010001))); // Invalid component
}

TEST_F(GraphErrorTaxonomyTest, SuccessCodesValidate) {
    // SUCCESS should be valid
    EXPECT_TRUE(isValidErrorCode(GraphErrorCode::SUCCESS));
}

// ─────────────────────────────────────────────────────────────────────────────
// Test: Determinism (Frozen Contract)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(GraphErrorTaxonomyTest, ClassificationDeterminism) {
    // Multiple calls with same input must produce identical output
    auto code = GraphErrorCode::TRAV_GPU_MEMORY_EXHAUSTED;
    
    auto cat1 = getErrorCategory(code);
    auto cat2 = getErrorCategory(code);
    auto cat3 = getErrorCategory(code);
    
    EXPECT_EQ(cat1, cat2);
    EXPECT_EQ(cat2, cat3);
    
    auto fallback1 = isRecoverableFallback(code);
    auto fallback2 = isRecoverableFallback(code);
    
    EXPECT_EQ(fallback1, fallback2);
    EXPECT_TRUE(fallback1);
}

TEST_F(GraphErrorTaxonomyTest, DescriptionDeterminism) {
    // Descriptions must be identical on repeated calls
    auto code = GraphErrorCode::OPT_COST_CALC_OVERFLOW;
    
    auto desc1 = getErrorDescription(code);
    auto desc2 = getErrorDescription(code);
    auto desc3 = getErrorDescription(code);
    
    EXPECT_EQ(desc1, desc2);
    EXPECT_EQ(desc2, desc3);
}

// ─────────────────────────────────────────────────────────────────────────────
// Test: Error Context Diagnostics
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(GraphErrorTaxonomyTest, ErrorContextDiagnosticFormatting) {
    ErrorContext ctx;
    ctx.code = GraphErrorCode::TRAV_GPU_MEMORY_EXHAUSTED;
    ctx.component = "parallel_traversal";
    ctx.operation = "multiSourceBFS";
    ctx.context = "query_id=q123, sources=[A,B,C]";
    
    auto diagnostic = ctx.formatDiagnostic();
    
    EXPECT_NE(diagnostic.find("[GRAPH ERROR]"), std::string::npos);
    EXPECT_NE(diagnostic.find("FALLBACK"), std::string::npos);
    EXPECT_NE(diagnostic.find("parallel_traversal"), std::string::npos);
    EXPECT_NE(diagnostic.find("GPU memory exhausted"), std::string::npos);
}

TEST_F(GraphErrorTaxonomyTest, RecoveryRecommendationByCategory) {
    // DENIAL error
    {
        ErrorContext denial_ctx;
        denial_ctx.code = GraphErrorCode::TRAV_VERTEX_NOT_FOUND;
        auto rec = denial_ctx.getRecoveryRecommendation();
        EXPECT_NE(rec.find("DENIAL"), std::string::npos);
        EXPECT_NE(rec.find("Fix preconditions"), std::string::npos);
    }

    // FALLBACK error
    {
        ErrorContext fallback_ctx;
        fallback_ctx.code = GraphErrorCode::TRAV_GPU_MEMORY_EXHAUSTED;
        auto rec = fallback_ctx.getRecoveryRecommendation();
        EXPECT_NE(rec.find("FALLBACK"), std::string::npos);
        EXPECT_NE(rec.find("CPU"), std::string::npos);
    }

    // REASONING_CONFLICT error
    {
        ErrorContext conflict_ctx;
        conflict_ctx.code = GraphErrorCode::REASON_INFERENCE_CONFLICT;
        auto rec = conflict_ctx.getRecoveryRecommendation();
        EXPECT_NE(rec.find("CONFLICT"), std::string::npos);
        EXPECT_NE(rec.find("Operator"), std::string::npos);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Test: Error Hexadecimal Formatting
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(GraphErrorTaxonomyTest, ErrorCodeHexFormatting) {
    auto hex = getErrorCodeHex(GraphErrorCode::OPT_INVALID_QUERY_AST);
    EXPECT_EQ(hex, "0x01010001");
    
    hex = getErrorCodeHex(GraphErrorCode::TRAV_GPU_MEMORY_EXHAUSTED);
    EXPECT_EQ(hex, "0x02020001");
    
    hex = getErrorCodeHex(GraphErrorCode::REASON_INFERENCE_CONFLICT);
    EXPECT_EQ(hex, "0x03030001");
}

// ─────────────────────────────────────────────────────────────────────────────
// Test: Component Code Extraction
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(GraphErrorTaxonomyTest, ComponentExtractionFromCode) {
    // Codes starting with 01 = Optimizer
    EXPECT_EQ(static_cast<uint32_t>(GraphErrorCode::OPT_INVALID_QUERY_AST) >> 24, 0x01);
    
    // Codes starting with 02 = Traversal
    EXPECT_EQ(static_cast<uint32_t>(GraphErrorCode::TRAV_VERTEX_NOT_FOUND) >> 24, 0x02);
    
    // Codes starting with 03 = Reasoning
    EXPECT_EQ(static_cast<uint32_t>(GraphErrorCode::REASON_INVALID_RULE_SYNTAX) >> 24, 0x03);
}

} // namespace test
} // namespace graph
} // namespace themis
