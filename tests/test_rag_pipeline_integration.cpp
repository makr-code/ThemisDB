/**
 * @file test_rag_pipeline_integration.cpp
 * @brief Integration tests for the full RAG pipeline:
 *        Document split → HybridRetriever fusion → RAGJudge evaluation
 *        without a live LLM backend (heuristic / fast mode only).
 *
 * Tests cover:
 *  - DocumentSplitter produces non-empty chunks from a multi-sentence doc
 *  - HybridRetriever fuses BM25 and vector candidates, returns top-k results
 *  - RAGJudge (FAST mode, no LLM) scores an answer against retrieved docs
 *  - All dimension scores are in [0, 1]
 *  - Chained pipeline: split → retrieve → evaluate end-to-end
 *  - EvaluationCache caches RAGJudge results across repeated identical calls
 *  - BatchEvaluator processes multiple inputs and aggregates correctly
 *  - Empty document corpus still produces valid (zero-score) evaluation result
 *  - EvaluationCache miss on first call; hit on second call
 *  - BatchEvaluator average scores consistent with individual evaluations
 */

#include "rag/document_splitter.h"
#include "rag/hybrid_retriever.h"
#include "rag/rag_judge.h"
#include "rag/evaluation_cache.h"
#include "rag/batch_evaluator.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

using namespace themis::rag;
using namespace themis::rag::judge;

// ─────────────────────────────────────────────────────────────────────────────
// Shared helpers
// ─────────────────────────────────────────────────────────────────────────────

/// Create a RAGJudge that operates in FAST mode with heuristics only (no LLM).
static std::shared_ptr<RAGJudge> makeFastJudge() {
    RAGJudgeConfig cfg;
    cfg.mode                    = EvaluationMode::FAST;
    cfg.enable_ethical_evaluation = false;
    cfg.use_nli_verifier          = false;
    cfg.use_geval_scoring         = false;
    cfg.cache_evaluations         = false;
    return std::make_shared<RAGJudge>(cfg);
}

/// Build a RetrievedDocument from plain strings.
static RetrievedDocument makeDoc(const std::string& id,
                                  const std::string& content,
                                  double score = 0.9) {
    RetrievedDocument d;
    d.id               = id;
    d.content          = content;
    d.similarity_score = score;
    return d;
}

/// Sample corpus (enough sentences to create multiple chunks).
static const std::string kCorpus =
    "Paris is the capital of France. "
    "The city is famous for the Eiffel Tower, built in 1889. "
    "Paris has a population of over 2 million people. "
    "The Louvre Museum is one of the largest art museums in the world. "
    "French cuisine is renowned globally and Paris is its culinary heart. "
    "The Seine river flows through the centre of Paris. "
    "Notre-Dame Cathedral is a celebrated Gothic masterpiece. "
    "Paris hosted the 1900 and 1924 Summer Olympics. "
    "The Palace of Versailles is located just outside Paris. "
    "Paris is often called the City of Light due to its role in the Enlightenment.";

// ─────────────────────────────────────────────────────────────────────────────
// 1. DocumentSplitter
// ─────────────────────────────────────────────────────────────────────────────

TEST(RAGPipelineIntegrationTest, DocumentSplitterProducesChunks) {
    DocumentSplitterConfig cfg;
    cfg.strategy   = SplitStrategy::Sentence;
    cfg.chunk_size = 100;
    cfg.overlap    = 0;
    DocumentSplitter splitter(cfg);

    auto chunks = splitter.split(kCorpus, "doc-paris");

    EXPECT_FALSE(chunks.empty());
    for (const auto& chunk : chunks) {
        EXPECT_FALSE(chunk.text.empty());
        EXPECT_EQ(chunk.document_id, "doc-paris");
    }
}

TEST(RAGPipelineIntegrationTest, DocumentSplitterChunkSizeBounded) {
    DocumentSplitterConfig cfg;
    cfg.strategy   = SplitStrategy::Fixed;
    cfg.chunk_size = 50;
    cfg.overlap    = 10;
    DocumentSplitter splitter(cfg);

    auto chunks = splitter.split(kCorpus, "doc1");
    EXPECT_FALSE(chunks.empty());
    // Each chunk text should not vastly exceed chunk_size
    for (const auto& chunk : chunks) {
        EXPECT_LE(static_cast<int>(chunk.text.size()), cfg.chunk_size * 3);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// 2. HybridRetriever fusion
// ─────────────────────────────────────────────────────────────────────────────

TEST(RAGPipelineIntegrationTest, HybridRetrieverFusesResults) {
    auto retriever = HybridRetrieverFactory::createBalanced();

    std::vector<RetrievedDocument> bm25 = {
        makeDoc("doc1", "Paris is the capital of France.", 0.9),
        makeDoc("doc2", "The Eiffel Tower is in Paris.", 0.7),
    };
    std::vector<RetrievedDocument> vec = {
        makeDoc("doc1", "Paris is the capital of France.", 0.95),
        makeDoc("doc3", "France has a rich history.", 0.6),
    };

    auto result = retriever.fuse(bm25, vec);

    EXPECT_FALSE(result.documents.empty());

    // Scores should be in [0, 1]
    for (const auto& doc : result.documents) {
        EXPECT_GE(doc.similarity_score, 0.0);
        EXPECT_LE(doc.similarity_score, 1.0);
    }
}

TEST(RAGPipelineIntegrationTest, HybridRetrieverDeduplicates) {
    auto retriever = HybridRetrieverFactory::createBalanced();

    // Same doc-id in both lists
    std::vector<RetrievedDocument> bm25 = {makeDoc("dup", "content", 0.8)};
    std::vector<RetrievedDocument> vec  = {makeDoc("dup", "content", 0.9)};

    auto result = retriever.fuse(bm25, vec);

    // Duplicate id should appear at most once
    size_t dup_count = 0;
    for (const auto& doc : result.documents) {
        if (doc.id == "dup") {
          ++dup_count;
        }
    }
    EXPECT_LE(dup_count, 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// 3. RAGJudge evaluation (heuristic only)
// ─────────────────────────────────────────────────────────────────────────────

TEST(RAGPipelineIntegrationTest, RAGJudgeScoresInRange) {
    auto judge = makeFastJudge();

    std::vector<RetrievedDocument> docs = {
        makeDoc("d1", "Paris is the capital of France.", 0.95),
        makeDoc("d2", "The Eiffel Tower is in Paris.", 0.85),
    };
    const std::string answer =
        "The capital of France is Paris, known for the Eiffel Tower.";

    auto result = judge->evaluate("What is the capital of France?", docs, answer);

    EXPECT_GE(result.faithfulness_score,  0.0);
    EXPECT_LE(result.faithfulness_score,  1.0);
    EXPECT_GE(result.relevance_score,     0.0);
    EXPECT_LE(result.relevance_score,     1.0);
    EXPECT_GE(result.completeness_score,  0.0);
    EXPECT_LE(result.completeness_score,  1.0);
    EXPECT_GE(result.coherence_score,     0.0);
    EXPECT_LE(result.coherence_score,     1.0);
    EXPECT_GE(result.overall_score,       0.0);
    EXPECT_LE(result.overall_score,       1.0);
}

TEST(RAGPipelineIntegrationTest, RAGJudgeEmptyCorpus) {
    auto judge = makeFastJudge();
    std::vector<RetrievedDocument> empty_docs;

    // Should not throw; scores may be low but valid
    EXPECT_NO_THROW({
        auto result = judge->evaluate("query", empty_docs, "answer");
        EXPECT_GE(result.overall_score, 0.0);
        EXPECT_LE(result.overall_score, 1.0);
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// 4. Full pipeline: split → retrieve → evaluate
// ─────────────────────────────────────────────────────────────────────────────

TEST(RAGPipelineIntegrationTest, FullPipelineRetrieveThenEvaluate) {
    // Step 1: Split corpus into chunks
    DocumentSplitterConfig split_cfg;
    split_cfg.strategy   = SplitStrategy::Sentence;
    split_cfg.chunk_size = 120;
    split_cfg.overlap    = 20;
    DocumentSplitter splitter(split_cfg);
    auto chunks = splitter.split(kCorpus, "doc-paris");
    ASSERT_FALSE(chunks.empty());

    // Step 2: Create mock BM25 and vector candidate lists from chunks
    //         (simulate a retrieval system by converting chunks to RetrievedDocuments)
    std::vector<RetrievedDocument> bm25_candidates;
    std::vector<RetrievedDocument> vec_candidates = {};

    for (size_t i = 0; i < chunks.size(); ++i) {
        double score = 1.0 - static_cast<double>(i) * 0.05;
        bm25_candidates.push_back(makeDoc("c" + std::to_string(i), chunks[i].text, score));
        vec_candidates.push_back(
            makeDoc("c" + std::to_string(i), chunks[i].text, score * 0.9)
        );
    }

    // Step 3: Fuse candidates using HybridRetriever
    auto retriever = HybridRetrieverFactory::createSemanticFocused();
    auto fused     = retriever.fuse(bm25_candidates, vec_candidates);
    ASSERT_FALSE(fused.documents.empty());

    // Step 4: Evaluate the retrieved context with RAGJudge (FAST mode)
    auto judge  = makeFastJudge();
    const auto& docs   = fused.documents;
    const std::string answer =
        "Paris is the capital of France. It is famous for the Eiffel Tower.";

    auto result = judge->evaluate("What is Paris famous for?", docs, answer);

    EXPECT_GE(result.overall_score, 0.0);
    EXPECT_LE(result.overall_score, 1.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// 5. EvaluationCache integration
// ─────────────────────────────────────────────────────────────────────────────

TEST(RAGPipelineIntegrationTest, EvaluationCacheMissThenHit) {
    CacheConfig ccfg;
    ccfg.max_entries = 100;
    ccfg.ttl         = std::chrono::seconds(3600);
    EvaluationCache cache(ccfg);

    auto judge = makeFastJudge();
    std::vector<RetrievedDocument> docs = {
        makeDoc("d1", "Paris is the capital of France.", 0.95),
    };
    const std::string query  = "What is the capital of France?";
    const std::string answer = "Paris is the capital of France.";

    // First call: cache miss
    EXPECT_EQ(cache.get(query, answer), nullptr);
    auto result = judge->evaluate(query, docs, answer);
    cache.put(query, answer, result);

    // Second call: cache hit
    const EvaluationResult* cached = cache.get(query, answer);
    ASSERT_NE(cached, nullptr);
    EXPECT_NEAR(cached->overall_score, result.overall_score, 1e-9);
}

TEST(RAGPipelineIntegrationTest, EvaluationCacheStatsAfterMixedAccess) {
    CacheConfig ccfg;
    ccfg.max_entries = 10;
    ccfg.ttl         = std::chrono::seconds(3600);
    EvaluationCache cache(ccfg);

    auto judge = makeFastJudge();
    std::vector<RetrievedDocument> docs = {makeDoc("d1", "France capital is Paris.", 0.9)};

    // Populate cache
    for (int i = 0; i < 3; ++i) {
        std::string q = "query" + std::to_string(i);
        auto r = judge->evaluate(q, docs, "answer" + std::to_string(i));
        cache.put(q, "answer" + std::to_string(i), r);
    }

    // 3 hits + 1 miss
    for (int i = 0; i < 3; ++i) {
        cache.get("query" + std::to_string(i), "answer" + std::to_string(i));
    }
    cache.get("query99", "nope");

    auto stats = cache.getStatistics();
    EXPECT_EQ(stats.cache_hits,   3u);
    EXPECT_EQ(stats.cache_misses, 1u);
    EXPECT_NEAR(stats.hit_rate, 0.75, 1e-6);
}

// ─────────────────────────────────────────────────────────────────────────────
// 6. BatchEvaluator integration
// ─────────────────────────────────────────────────────────────────────────────

TEST(RAGPipelineIntegrationTest, BatchEvaluatorPipelineConsistency) {
    auto judge = makeFastJudge();
    BatchEvaluatorConfig bcfg;
    bcfg.num_workers = 1;
    BatchEvaluator batch(judge, bcfg);

    std::vector<RetrievedDocument> docs = {
        makeDoc("d1", "Paris is the capital of France.", 0.95),
        makeDoc("d2", "The Eiffel Tower stands in Paris.", 0.80),
    };

    std::vector<EvaluationInput> inputs = {};

    for (int i = 0; i < 4; ++i) {
        EvaluationInput in;
        in.query            = "What is the capital of France?";
        in.documents        = docs;
        in.generated_answer = "Paris is the capital of France.";
        inputs.push_back(in);
    }

    auto batch_result = batch.evaluateBatch(inputs);

    EXPECT_EQ(batch_result.results.size(), 4u);

    // Average should be consistent with individual scores
    double sum_overall = 0.0;
    for (const auto& r : batch_result.results) {
        sum_overall += r.overall_score;
    }
    double expected_avg = sum_overall / 4.0;
    EXPECT_NEAR(batch_result.average_overall_score, expected_avg, 1e-6);
}

TEST(RAGPipelineIntegrationTest, BatchEvaluatorEmptyInput) {
    auto judge = makeFastJudge();
    BatchEvaluator batch(judge);

    auto result = batch.evaluateBatch(std::vector<EvaluationInput>{});
    EXPECT_TRUE(result.results.empty());
    EXPECT_EQ(result.progress.total_items, 0u);
}
