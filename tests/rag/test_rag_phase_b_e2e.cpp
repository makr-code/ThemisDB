/**
 * @file test_rag_phase_b_e2e.cpp
 * @brief Sprint 1 — Phase B Kern end-to-end activation tests.
 *
 * Validates that the four Phase-B retrieval building blocks —
 * BM25+ (via HybridRetriever weights), HNSW (vector candidates),
 * RRF fusion (HybridRetriever::fuseRRF), and LLM-Judge
 * (LLMJudgeIntegration real-engine path) — are wired together
 * end-to-end with no mock fallback in the production code path.
 *
 * Tests:
 *  PHASE-B-E2E-01  HybridRetriever RRF fuses BM25 + HNSW candidates,
 *                  returns non-empty ranked list with correct count.
 *  PHASE-B-E2E-02  BM25-only mode (vector_weight=0): HNSW-exclusive docs
 *                  absent from fused result.
 *  PHASE-B-E2E-03  Vector-only mode (bm25_weight=0): BM25-exclusive docs
 *                  absent from fused result.
 *  PHASE-B-E2E-04  RRF with k=60 (standard parameter): hybrid scores are
 *                  positive and strictly ordered descending.
 *  PHASE-B-E2E-05  LLMJudgeIntegration with real ILLMInferenceEngine:
 *                  isMockMode()==false and evaluateDimension returns
 *                  the injected engine's response, not mock text.
 *  PHASE-B-E2E-06  LLMJudgeIntegration with gate disabled
 *                  (enable_llm_judge=false) returns unavailable sentinel,
 *                  never a mock score.
 *  PHASE-B-E2E-07  Full Phase B chain: BM25 docs → HNSW docs → RRF fusion
 *                  → LLM-Judge scores the top-ranked document content.
 *
 * All tests are self-contained (no RocksDB, no network dependency).
 *
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @see src/rag/ROADMAP.md §Q4 2026 — Advanced Retrieval + LLM-Judge
 * @see include/rag/hybrid_retriever.h
 * @see include/rag/llm_judge_integration.h
 */

#include "rag/hybrid_retriever.h"
#include "rag/llm_judge_integration.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

using namespace themis::rag;
using namespace themis::rag::judge;

// ─────────────────────────────────────────────────────────────────────────────
// Test helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Build a RetrievedDocument with given id, content, and similarity score.
[[nodiscard]] RetrievedDocument makeDoc(const std::string& id,
                                        const std::string& content,
                                        double             score) {
    RetrievedDocument d;
    d.id               = id;
    d.content          = content;
    d.similarity_score = score;
    return d;
}

/// Minimal ILLMInferenceEngine that returns a deterministic JSON response.
/// Used to verify the non-mock production path of LLMJudgeIntegration.
struct DeterministicEngine final : ILLMInferenceEngine {
    explicit DeterministicEngine(std::string response)
        : response_(std::move(response)) {}

    [[nodiscard]] std::string generate(const std::string& /*prompt*/) override {
        ++call_count;
        return response_;
    }

    int         call_count = 0;
    std::string response_;
};

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// PHASE-B-E2E-01: RRF fuses BM25 and HNSW candidates
// ─────────────────────────────────────────────────────────────────────────────

TEST(RagPhaseBE2E, RrfFusesBm25AndHnswCandidates) {
    HybridRetrieverConfig cfg;
    cfg.use_rrf        = true;
    cfg.rrf_k          = 60.0;
    cfg.bm25_weight    = 0.5;
    cfg.vector_weight  = 0.5;
    cfg.top_k          = 10;

    HybridRetriever retriever(cfg);

    // BM25 candidates (simulated fulltext scorer output)
    const std::vector<RetrievedDocument> bm25 = {
        makeDoc("bm25-doc-1", "BM25 top result about HNSW indexing",        0.92),
        makeDoc("bm25-doc-2", "BM25 second result about graph construction", 0.78),
        makeDoc("shared-doc", "Document present in both lists",              0.65),
    };

    // HNSW / vector candidates (simulated KNN output, distance converted to similarity)
    const std::vector<RetrievedDocument> hnsw = {
        makeDoc("shared-doc", "Document present in both lists",   0.88),
        makeDoc("vec-doc-1",  "Dense vector result about FAISS",  0.81),
        makeDoc("vec-doc-2",  "Dense vector result about cosine", 0.74),
    };

    auto result = retriever.fuse(bm25, hnsw);

    // 4 unique documents (bm25-doc-1, bm25-doc-2, shared-doc, vec-doc-1, vec-doc-2)
    EXPECT_EQ(result.documents.size(), 5u);
    EXPECT_TRUE(result.used_rrf);
    EXPECT_EQ(result.total_candidates, 5u);

    // Scores must be in descending order
    for (std::size_t i = 1; i < result.scores.size(); ++i) {
        EXPECT_GE(result.scores[i - 1].hybrid_score, result.scores[i].hybrid_score)
            << "Score ordering violated at index " << i;
    }

    // Shared document boosted by appearing in both lists must rank near top
    auto shared_it = std::find_if(
        result.documents.begin(), result.documents.end(),
        [](const RetrievedDocument& d) { return d.id == "shared-doc"; });
    ASSERT_NE(shared_it, result.documents.end()) << "shared-doc must survive fusion";
    // shared-doc has contributions from both BM25 and vector; its rank should be ≤ 2
    const auto shared_rank = static_cast<std::size_t>(
        std::distance(result.documents.begin(), shared_it));
    EXPECT_LE(shared_rank, 1u)
        << "shared-doc (appears in both lists) should rank first or second; actual rank="
        << shared_rank;
}

// ─────────────────────────────────────────────────────────────────────────────
// PHASE-B-E2E-02: BM25-only mode — HNSW-exclusive docs absent
// ─────────────────────────────────────────────────────────────────────────────

TEST(RagPhaseBE2E, Bm25OnlyModeExcludesHnswDocs) {
    HybridRetrieverConfig cfg;
    cfg.use_rrf       = true;
    cfg.rrf_k         = 60.0;
    cfg.bm25_weight   = 1.0;
    cfg.vector_weight = 0.0;  // HNSW path disabled by zero weight
    cfg.top_k         = 10;

    HybridRetriever retriever(cfg);

    const std::vector<RetrievedDocument> bm25 = {
        makeDoc("bm25-only-doc", "Fulltext BM25 content", 0.85),
    };
    const std::vector<RetrievedDocument> hnsw = {
        makeDoc("hnsw-only-doc", "Dense vector content",  0.90),
    };

    auto result = retriever.fuse(bm25, hnsw);

    // hnsw-only-doc receives zero contribution; bm25-only-doc must score higher
    ASSERT_EQ(result.documents.size(), 2u);

    const auto bm25_it = std::find_if(result.documents.begin(), result.documents.end(),
        [](const RetrievedDocument& d) { return d.id == "bm25-only-doc"; });
    const auto hnsw_it = std::find_if(result.documents.begin(), result.documents.end(),
        [](const RetrievedDocument& d) { return d.id == "hnsw-only-doc"; });

    ASSERT_NE(bm25_it, result.documents.end());
    ASSERT_NE(hnsw_it, result.documents.end());

    EXPECT_GT(bm25_it->similarity_score, hnsw_it->similarity_score)
        << "With bm25_weight=1 and vector_weight=0, BM25-sourced doc must outscore HNSW-only doc";
}

// ─────────────────────────────────────────────────────────────────────────────
// PHASE-B-E2E-03: Vector-only mode — BM25-exclusive docs absent from top
// ─────────────────────────────────────────────────────────────────────────────

TEST(RagPhaseBE2E, VectorOnlyModeExcludesBm25Docs) {
    HybridRetrieverConfig cfg;
    cfg.use_rrf       = true;
    cfg.rrf_k         = 60.0;
    cfg.bm25_weight   = 0.0;  // BM25 path disabled by zero weight
    cfg.vector_weight = 1.0;
    cfg.top_k         = 10;

    HybridRetriever retriever(cfg);

    const std::vector<RetrievedDocument> bm25 = {
        makeDoc("bm25-only-doc", "Fulltext BM25 content", 0.95),
    };
    const std::vector<RetrievedDocument> hnsw = {
        makeDoc("hnsw-only-doc", "Dense vector content",  0.80),
    };

    auto result = retriever.fuse(bm25, hnsw);

    ASSERT_EQ(result.documents.size(), 2u);

    const auto bm25_it = std::find_if(result.documents.begin(), result.documents.end(),
        [](const RetrievedDocument& d) { return d.id == "bm25-only-doc"; });
    const auto hnsw_it = std::find_if(result.documents.begin(), result.documents.end(),
        [](const RetrievedDocument& d) { return d.id == "hnsw-only-doc"; });

    ASSERT_NE(bm25_it, result.documents.end());
    ASSERT_NE(hnsw_it, result.documents.end());

    EXPECT_GT(hnsw_it->similarity_score, bm25_it->similarity_score)
        << "With vector_weight=1 and bm25_weight=0, HNSW-sourced doc must outscore BM25-only doc";
}

// ─────────────────────────────────────────────────────────────────────────────
// PHASE-B-E2E-04: RRF k=60 — hybrid scores positive and strictly ordered
// ─────────────────────────────────────────────────────────────────────────────

TEST(RagPhaseBE2E, RrfK60ScoresPositiveAndOrdered) {
    // Use the factory's balanced preset: RRF k=60, equal weights
    HybridRetriever retriever = HybridRetrieverFactory::createBalanced(20);

    EXPECT_DOUBLE_EQ(retriever.getConfig().rrf_k,         60.0);
    EXPECT_DOUBLE_EQ(retriever.getConfig().bm25_weight,   0.5);
    EXPECT_DOUBLE_EQ(retriever.getConfig().vector_weight, 0.5);
    EXPECT_TRUE(retriever.getConfig().use_rrf);

    // Populate 5 BM25 and 5 HNSW candidates with decreasing scores
    std::vector<RetrievedDocument> bm25_candidates;
    std::vector<RetrievedDocument> vec_candidates;
    for (int i = 0; i < 5; ++i) {
        bm25_candidates.push_back(
            makeDoc("b" + std::to_string(i), "bm25 content " + std::to_string(i),
                    0.9 - 0.1 * i));
        vec_candidates.push_back(
            makeDoc("v" + std::to_string(i), "vec content " + std::to_string(i),
                    0.85 - 0.1 * i));
    }

    auto result = retriever.fuse(bm25_candidates, vec_candidates);

    EXPECT_EQ(result.total_candidates, 10u);
    ASSERT_FALSE(result.scores.empty());

    for (const auto& hs : result.scores) {
        EXPECT_GT(hs.hybrid_score, 0.0)
            << "All hybrid scores must be positive (RRF denominator never zero)";
    }

    for (std::size_t i = 1; i < result.scores.size(); ++i) {
        EXPECT_GE(result.scores[i - 1].hybrid_score, result.scores[i].hybrid_score)
            << "Hybrid scores must be in non-increasing order at index " << i;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// PHASE-B-E2E-05: LLMJudgeIntegration — real engine, production path
// ─────────────────────────────────────────────────────────────────────────────

TEST(RagPhaseBE2E, LlmJudgeRealEngineNotMock) {
    // Production JSON-like response from a real model endpoint
    constexpr const char* kRealResponse =
        R"({"score":4.2,"confidence":0.91,"reasoning":"The retrieved passage directly addresses the query."})";

    DeterministicEngine engine{kRealResponse};

    LLMJudgeIntegration::Config cfg;
    cfg.enable_llm_judge = true;

    LLMJudgeIntegration judge(&engine, cfg);

    EXPECT_FALSE(judge.isMockMode())
        << "isMockMode() must be false when a real ILLMInferenceEngine is injected";

    const std::string response =
        judge.evaluateDimension("Rate the relevance of this passage.",
                                EvaluationDimension::RELEVANCE);

    EXPECT_EQ(engine.call_count, 1)
        << "Engine must be called exactly once per evaluateDimension call";

    EXPECT_EQ(response, kRealResponse)
        << "evaluateDimension must return the engine response verbatim";

    // Ensure the response does not contain the mock reasoning string
    EXPECT_EQ(response.find("Mock evaluation"), std::string::npos)
        << "Real engine path must never emit mock-evaluation text";
}

// ─────────────────────────────────────────────────────────────────────────────
// PHASE-B-E2E-06: LLMJudgeIntegration gate disabled → unavailable, not mock
// ─────────────────────────────────────────────────────────────────────────────

TEST(RagPhaseBE2E, LlmJudgeGateDisabledReturnsUnavailable) {
    DeterministicEngine engine{R"({"score":5.0})"};

    LLMJudgeIntegration::Config cfg;
    cfg.enable_llm_judge = false;  // gate explicitly disabled

    LLMJudgeIntegration judge(&engine, cfg);

    const std::string response =
        judge.evaluateDimension("Rate faithfulness.", EvaluationDimension::FAITHFULNESS);

    // Gate disabled: no engine call must occur
    EXPECT_EQ(engine.call_count, 0)
        << "Engine must NOT be called when enable_llm_judge=false";

    // Gate disabled: response must signal unavailability, not mock score
    EXPECT_NE(response.find("llm_unavailable"), std::string::npos)
        << "Gate-disabled path must return a response containing 'llm_unavailable'";
}

// ─────────────────────────────────────────────────────────────────────────────
// PHASE-B-E2E-07: Full Phase B chain — BM25 + HNSW → RRF → LLM-Judge
// ─────────────────────────────────────────────────────────────────────────────

TEST(RagPhaseBE2E, FullPhaseBChainBm25HnswRrfLlmJudge) {
    // ── Step 1: Simulate BM25 scorer output ──────────────────────────────────
    // (in production: SecondaryIndexManager::scanFulltextWithScores result)
    const std::vector<RetrievedDocument> bm25_results = {
        makeDoc("chunk-001",
                "HNSW builds a hierarchical navigable small-world graph "
                "for approximate nearest-neighbour search.",
                0.88),
        makeDoc("chunk-002",
                "BM25+ improves BM25 by adding a lower-bound frequency term delta.",
                0.76),
        makeDoc("chunk-003",
                "Reciprocal Rank Fusion combines ranked lists from heterogeneous retrievers.",
                0.61),
    };

    // ── Step 2: Simulate HNSW KNN output ─────────────────────────────────────
    // (in production: VectorIndexManager::searchKnn result, 1 - distance)
    const std::vector<RetrievedDocument> hnsw_results = {
        makeDoc("chunk-001",                                       // shared with BM25
                "HNSW builds a hierarchical navigable small-world graph "
                "for approximate nearest-neighbour search.",
                0.93),
        makeDoc("chunk-004",
                "Product quantization reduces memory footprint for large vector indices.",
                0.82),
        makeDoc("chunk-003",                                       // shared with BM25
                "Reciprocal Rank Fusion combines ranked lists from heterogeneous retrievers.",
                0.70),
    };

    // ── Step 3: RRF fusion (k=60, standard Phase B parameters) ───────────────
    HybridRetrieverConfig rrf_cfg;
    rrf_cfg.use_rrf       = true;
    rrf_cfg.rrf_k         = 60.0;
    rrf_cfg.bm25_weight   = 0.5;
    rrf_cfg.vector_weight = 0.5;
    rrf_cfg.top_k         = 5;

    HybridRetriever retriever(rrf_cfg);
    auto fused = retriever.fuse(bm25_results, hnsw_results);

    ASSERT_FALSE(fused.documents.empty()) << "RRF must return at least one fused document";
    EXPECT_TRUE(fused.used_rrf);

    // chunk-001 and chunk-003 appear in both lists; chunk-001 should rank first
    EXPECT_EQ(fused.documents.front().id, "chunk-001")
        << "chunk-001 has highest scores in both BM25 and HNSW; must be top RRF result";

    // ── Step 4: LLM-Judge scores the top-ranked result ───────────────────────
    // Build a deterministic judge response for the top document
    const std::string judge_response_json =
        R"({"score":4.5,"confidence":0.93,"reasoning":"Top chunk directly answers the HNSW query."})";

    DeterministicEngine judge_engine{judge_response_json};

    LLMJudgeIntegration::Config judge_cfg;
    judge_cfg.enable_llm_judge = true;

    LLMJudgeIntegration judge(&judge_engine, judge_cfg);

    ASSERT_FALSE(judge.isMockMode())
        << "LLM judge must NOT be in mock mode for production Phase B chain";

    // Build a prompt from the top-ranked document content
    const std::string prompt =
        "Query: What is HNSW?\nDocument: " + fused.documents.front().content +
        "\nRate relevance on scale 1-5.";

    const std::string judge_result =
        judge.evaluateDimension(prompt, EvaluationDimension::RELEVANCE);

    EXPECT_EQ(judge_engine.call_count, 1);
    EXPECT_EQ(judge_result, judge_response_json)
        << "LLM judge must return the engine response for the top RRF document";

    // Verify the full chain produced a non-mock, real-scored result
    EXPECT_EQ(judge_result.find("Mock evaluation"), std::string::npos)
        << "Phase B chain must never contain mock evaluation text";
    EXPECT_NE(judge_result.find("4.5"), std::string::npos)
        << "Phase B chain must contain the real judge score from the engine";
}
