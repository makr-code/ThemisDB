/**
 * @file test_rag_streaming_retriever.cpp
 * @brief Unit tests for streaming retrieval with incremental context window filling
 */

#include "rag/streaming_retriever.h"
#include <gtest/gtest.h>
#include <string>
#include <vector>

using namespace themis::rag::streaming;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static StreamedDocument makeDoc(const std::string& id, const std::string& content,
                                double score, size_t token_count = 0) {
    return StreamedDocument{id, content, score, token_count};
}

// Build a set of N documents each with roughly 100 tokens of content.
static std::vector<StreamedDocument> makeDocs(size_t n, double base_score = 0.9) {
    std::vector<StreamedDocument> docs;
    docs.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        // ~400 chars ≈ 100 tokens at 4 chars/token
        std::string content(400, 'x');
        const double score = base_score - static_cast<double>(i) * 0.01;
        docs.push_back(makeDoc("doc" + std::to_string(i), content, score));
    }
    return docs;
}

// ===========================================================================
// ContextWindowFiller tests
// ===========================================================================

class ContextWindowFillerTest : public ::testing::Test {
protected:
    // 1000-token budget, 4 chars per token
    ContextWindowFiller filler{1000, 4.0};
};

TEST_F(ContextWindowFillerTest, EstimateTokens) {
    // 400 chars / 4.0 chars-per-token = 100 tokens
    EXPECT_EQ(filler.estimateTokens(""), 0u);
    EXPECT_EQ(filler.estimateTokens(std::string(400, 'a')), 100u);
    EXPECT_EQ(filler.estimateTokens(std::string(401, 'a')), 101u);
}

TEST_F(ContextWindowFillerTest, InitiallyEmpty) {
    EXPECT_TRUE(filler.documents().empty());
    EXPECT_TRUE(filler.hasCapacity(1));
    auto s = filler.snapshot();
    EXPECT_EQ(s.total_tokens_used, 0u);
    EXPECT_EQ(s.max_tokens, 1000u);
    EXPECT_DOUBLE_EQ(s.fill_ratio, 0.0);
    EXPECT_FALSE(s.is_full);
}

TEST_F(ContextWindowFillerTest, AddDocumentWithExplicitTokenCount) {
    auto doc = makeDoc("d1", "hello world", 0.9, 50);
    EXPECT_TRUE(filler.tryAdd(doc));
    EXPECT_EQ(filler.documents().size(), 1u);
    EXPECT_EQ(filler.snapshot().total_tokens_used, 50u);
}

TEST_F(ContextWindowFillerTest, AddDocumentWithEstimatedTokenCount) {
    // 400-char content → 100 tokens
    auto doc = makeDoc("d1", std::string(400, 'a'), 0.9, 0);
    EXPECT_TRUE(filler.tryAdd(doc));
    EXPECT_EQ(filler.snapshot().total_tokens_used, 100u);
}

TEST_F(ContextWindowFillerTest, RejectWhenBudgetExceeded) {
    // Fill to 950 tokens
    auto doc1 = makeDoc("d1", "x", 0.9, 950);
    EXPECT_TRUE(filler.tryAdd(doc1));

    // Next doc needs 100 tokens – should be rejected
    auto doc2 = makeDoc("d2", std::string(400, 'a'), 0.8, 0);
    EXPECT_FALSE(filler.tryAdd(doc2));
    EXPECT_EQ(filler.documents().size(), 1u);
}

TEST_F(ContextWindowFillerTest, RejectExactlyOverBudget) {
    auto doc1 = makeDoc("d1", "x", 0.9, 999);
    EXPECT_TRUE(filler.tryAdd(doc1));

    // 2 tokens → exceeds remaining 1
    auto doc2 = makeDoc("d2", "x", 0.8, 2);
    EXPECT_FALSE(filler.tryAdd(doc2));
}

TEST_F(ContextWindowFillerTest, AcceptExactlyFitting) {
    auto doc1 = makeDoc("d1", "x", 0.9, 999);
    EXPECT_TRUE(filler.tryAdd(doc1));

    // 1 token → fits exactly
    auto doc2 = makeDoc("d2", "x", 0.8, 1);
    EXPECT_TRUE(filler.tryAdd(doc2));
    EXPECT_FALSE(filler.hasCapacity(1));
    EXPECT_TRUE(filler.snapshot().is_full);
}

TEST_F(ContextWindowFillerTest, Reset) {
    auto doc = makeDoc("d1", "x", 0.9, 100);
    EXPECT_TRUE(filler.tryAdd(doc));
    EXPECT_EQ(filler.documents().size(), 1u);

    filler.reset();
    EXPECT_TRUE(filler.documents().empty());
    EXPECT_EQ(filler.snapshot().total_tokens_used, 0u);
    EXPECT_FALSE(filler.snapshot().is_full);
}

TEST_F(ContextWindowFillerTest, FillRatio) {
    auto doc = makeDoc("d1", "x", 0.9, 500);
    filler.tryAdd(doc);
    EXPECT_DOUBLE_EQ(filler.snapshot().fill_ratio, 0.5);
}

TEST_F(ContextWindowFillerTest, HasCapacityAfterPartialFill) {
    auto doc = makeDoc("d1", "x", 0.9, 500);
    filler.tryAdd(doc);
    EXPECT_TRUE(filler.hasCapacity(500));
    EXPECT_TRUE(filler.hasCapacity(1));
    EXPECT_FALSE(filler.hasCapacity(501));
}

// ===========================================================================
// StreamingRetriever – basic behavior
// ===========================================================================

class StreamingRetrieverTest : public ::testing::Test {
protected:
    StreamingRetrieverConfig cfg;

    void SetUp() override {
        cfg.max_context_tokens         = 1000;   // 10 docs of ~100 tokens each
        cfg.chars_per_token            = 4.0;
        cfg.sort_by_relevance          = true;
        cfg.min_relevance_score        = 0.0;
        cfg.enable_mmr_deduplication   = false;
        cfg.max_documents_to_consider  = 0;
    }
};

TEST_F(StreamingRetrieverTest, EmptyCandidatesReturnsEmptyResult) {
    StreamingRetriever retriever(cfg);
    auto result = retriever.stream("q", {});
    EXPECT_EQ(result.documents_added, 0u);
    EXPECT_EQ(result.documents_considered, 0u);
    EXPECT_TRUE(result.selected_documents.empty());
    EXPECT_FALSE(result.cancelled);
}

TEST_F(StreamingRetrieverTest, AllDocumentsFitInWindow) {
    StreamingRetriever retriever(cfg);
    auto docs = makeDocs(5);  // 5 × 100 tokens = 500 ≤ 1000
    auto result = retriever.stream("q", docs);
    EXPECT_EQ(result.documents_added, 5u);
    EXPECT_TRUE(result.skipped_documents.empty());
    EXPECT_EQ(result.total_tokens_used, 500u);
    EXPECT_FALSE(result.cancelled);
}

TEST_F(StreamingRetrieverTest, ExcessDocumentsAreSkipped) {
    StreamingRetriever retriever(cfg);
    auto docs = makeDocs(15);  // 15 × 100 tokens – window fits only 10
    auto result = retriever.stream("q", docs);
    EXPECT_EQ(result.documents_added, 10u);
    EXPECT_EQ(result.skipped_documents.size(), 5u);
    EXPECT_LE(result.total_tokens_used, cfg.max_context_tokens);
}

TEST_F(StreamingRetrieverTest, DocumentsAreOrderedByRelevance) {
    StreamingRetriever retriever(cfg);

    // Provide in ascending score order; retriever should reverse them.
    std::vector<StreamedDocument> docs = {
        makeDoc("low",  std::string(200, 'a'), 0.3, 50),
        makeDoc("mid",  std::string(200, 'b'), 0.6, 50),
        makeDoc("high", std::string(200, 'c'), 0.9, 50),
    };

    std::vector<std::string> order;
    retriever.setDocumentAcceptedCallback(
        [&order](const StreamedDocument& doc, const ContextWindowState&) {
            order.push_back(doc.id);
        });

    retriever.stream("q", docs);

    ASSERT_EQ(order.size(), 3u);
    EXPECT_EQ(order[0], "high");
    EXPECT_EQ(order[1], "mid");
    EXPECT_EQ(order[2], "low");
}

TEST_F(StreamingRetrieverTest, NoSortWhenDisabled) {
    cfg.sort_by_relevance = false;
    StreamingRetriever retriever(cfg);

    std::vector<StreamedDocument> docs = {
        makeDoc("a", std::string(200, 'a'), 0.3, 50),
        makeDoc("b", std::string(200, 'b'), 0.9, 50),
    };

    std::vector<std::string> order;
    retriever.setDocumentAcceptedCallback(
        [&order](const StreamedDocument& doc, const ContextWindowState&) {
            order.push_back(doc.id);
        });

    retriever.stream("q", docs);

    ASSERT_EQ(order.size(), 2u);
    EXPECT_EQ(order[0], "a");  // original order preserved
    EXPECT_EQ(order[1], "b");
}

TEST_F(StreamingRetrieverTest, RelevanceFilterApplied) {
    cfg.min_relevance_score = 0.5;
    StreamingRetriever retriever(cfg);

    std::vector<StreamedDocument> docs = {
        makeDoc("below", "content", 0.3, 50),
        makeDoc("above", "content", 0.7, 50),
    };

    auto result = retriever.stream("q", docs);
    EXPECT_EQ(result.documents_added, 1u);
    EXPECT_EQ(result.selected_documents[0].id, "above");
}

TEST_F(StreamingRetrieverTest, MaxDocumentsToConsiderCap) {
    cfg.max_documents_to_consider = 3;
    StreamingRetriever retriever(cfg);

    auto docs = makeDocs(10);
    auto result = retriever.stream("q", docs);

    EXPECT_LE(result.documents_added, 3u);
    EXPECT_EQ(result.documents_considered, 10u);  // reflects original count
}

TEST_F(StreamingRetrieverTest, CallbacksAreInvoked) {
    StreamingRetriever retriever(cfg);

    int accepted_count = 0;
    int skipped_count  = 0;
    bool window_full   = false;

    retriever.setDocumentAcceptedCallback(
        [&accepted_count](const StreamedDocument&, const ContextWindowState&) {
            ++accepted_count;
        });
    retriever.setDocumentSkippedCallback(
        [&skipped_count](const StreamedDocument&, const ContextWindowState&) {
            ++skipped_count;
        });
    retriever.setWindowFullCallback(
        [&window_full](const ContextWindowState&) {
            window_full = true;
        });

    auto docs = makeDocs(15);  // 10 accepted, 5 skipped
    retriever.stream("q", docs);

    EXPECT_EQ(accepted_count, 10);
    EXPECT_EQ(skipped_count,  5);
    EXPECT_TRUE(window_full);
}

TEST_F(StreamingRetrieverTest, WindowFullCallbackReceivesCorrectState) {
    StreamingRetriever retriever(cfg);

    ContextWindowState captured;
    retriever.setWindowFullCallback(
        [&captured](const ContextWindowState& state) {
            captured = state;
        });

    auto docs = makeDocs(15);
    retriever.stream("q", docs);

    EXPECT_TRUE(captured.is_full);
    EXPECT_EQ(captured.max_tokens, cfg.max_context_tokens);
    EXPECT_LE(captured.total_tokens_used, cfg.max_context_tokens);
}

TEST_F(StreamingRetrieverTest, ElapsedTimeIsPositive) {
    StreamingRetriever retriever(cfg);
    auto docs = makeDocs(5);
    auto result = retriever.stream("q", docs);
    EXPECT_GE(result.elapsed_ms, 0.0);
}

TEST_F(StreamingRetrieverTest, IsStreamingFalseAfterCompletion) {
    StreamingRetriever retriever(cfg);
    auto docs = makeDocs(5);
    retriever.stream("q", docs);
    EXPECT_FALSE(retriever.isStreaming());
}

TEST_F(StreamingRetrieverTest, ExplicitTokenCountHonoured) {
    cfg.max_context_tokens = 200;
    StreamingRetriever retriever(cfg);

    // Each doc has an explicit token count of 100; two should fit, third not.
    std::vector<StreamedDocument> docs = {
        makeDoc("d1", "content", 0.9, 100),
        makeDoc("d2", "content", 0.8, 100),
        makeDoc("d3", "content", 0.7, 100),
    };

    auto result = retriever.stream("q", docs);
    EXPECT_EQ(result.documents_added, 2u);
    EXPECT_EQ(result.total_tokens_used, 200u);
}

// ===========================================================================
// StreamingRetriever – MMR deduplication
// ===========================================================================

class StreamingRetrieverMMRTest : public ::testing::Test {
protected:
    StreamingRetrieverConfig cfg;

    void SetUp() override {
        cfg.max_context_tokens        = 10000;
        cfg.chars_per_token           = 4.0;
        cfg.sort_by_relevance         = true;
        cfg.enable_mmr_deduplication  = true;
        cfg.mmr_similarity_threshold  = 0.8;
        cfg.min_relevance_score       = 0.0;
        cfg.max_documents_to_consider = 0;
    }
};

TEST_F(StreamingRetrieverMMRTest, IdenticalDocumentsDeduped) {
    StreamingRetriever retriever(cfg);

    // Both docs have identical content → second should be skipped.
    std::string shared_content = "The quick brown fox jumps over the lazy dog.";
    std::vector<StreamedDocument> docs = {
        makeDoc("d1", shared_content, 0.9, 50),
        makeDoc("d2", shared_content, 0.8, 50),
    };

    auto result = retriever.stream("q", docs);
    EXPECT_EQ(result.documents_added, 1u);
    EXPECT_EQ(result.skipped_documents.size(), 1u);
}

TEST_F(StreamingRetrieverMMRTest, DifferentDocumentsNotDeduped) {
    StreamingRetriever retriever(cfg);

    std::vector<StreamedDocument> docs = {
        makeDoc("d1", "Paris is the capital of France.", 0.9, 50),
        makeDoc("d2", "The Amazon rainforest covers most of Brazil.", 0.8, 50),
    };

    auto result = retriever.stream("q", docs);
    EXPECT_EQ(result.documents_added, 2u);
    EXPECT_TRUE(result.skipped_documents.empty());
}

// ===========================================================================
// StreamingRetriever – config access
// ===========================================================================

TEST(StreamingRetrieverConfigTest, GetSetConfig) {
    StreamingRetrieverConfig cfg;
    cfg.max_context_tokens = 2048;
    cfg.sort_by_relevance  = false;

    StreamingRetriever retriever(cfg);
    auto retrieved = retriever.getConfig();
    EXPECT_EQ(retrieved.max_context_tokens, 2048u);
    EXPECT_FALSE(retrieved.sort_by_relevance);

    StreamingRetrieverConfig new_cfg;
    new_cfg.max_context_tokens = 4096;
    retriever.setConfig(new_cfg);
    EXPECT_EQ(retriever.getConfig().max_context_tokens, 4096u);
}

// ===========================================================================
// ContextWindowFiller – edge cases
// ===========================================================================

TEST(ContextWindowFillerEdgeTest, ZeroTokenDocumentAccepted) {
    ContextWindowFiller filler(100, 4.0);
    // empty content → 0 tokens → should fit anywhere
    auto doc = makeDoc("d1", "", 0.9, 0);
    EXPECT_TRUE(filler.tryAdd(doc));
    EXPECT_EQ(filler.snapshot().total_tokens_used, 0u);
}

TEST(ContextWindowFillerEdgeTest, SingleCharContent) {
    ContextWindowFiller filler(100, 4.0);
    auto doc = makeDoc("d1", "x", 0.9, 0);
    EXPECT_TRUE(filler.tryAdd(doc));
    EXPECT_EQ(filler.snapshot().total_tokens_used, 1u);
}

TEST(ContextWindowFillerEdgeTest, MaxTokensBudgetZero) {
    ContextWindowFiller filler(0, 4.0);
    auto doc = makeDoc("d1", "content", 0.9, 1);
    EXPECT_FALSE(filler.tryAdd(doc));
    EXPECT_TRUE(filler.snapshot().is_full);
}
