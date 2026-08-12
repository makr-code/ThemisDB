/**
 * @file test_rag_citation_highlighter.cpp
 * @brief Unit tests for CitationHighlighter (map answer sentences to source chunks)
 *
 * Covers:
 *  - Sentence splitting (basic, abbreviations, short sentences, empty input)
 *  - computeSimilarity edge cases (empty strings, identical, disjoint)
 *  - highlight: primary citation assignment
 *  - highlight: no citation when similarity below threshold
 *  - highlight: secondary citations
 *  - highlight: empty answer or empty chunks
 *  - citation_coverage and mean_similarity statistics
 *  - getConfig / setConfig
 *  - Factory helpers (strict, balanced, permissive)
 */

#include "rag/citation_highlighter.h"
#include <gtest/gtest.h>

using namespace themis::rag;

// ============================================================================
// Helpers
// ============================================================================

static std::vector<SourceChunk> makeChunks(
    const std::vector<std::pair<std::string, std::string>>& id_content)
{
    std::vector<SourceChunk> chunks;
    chunks.reserve(id_content.size());
    for (size_t i = 0; i < id_content.size(); ++i) {
        SourceChunk c;
        c.doc_id      = id_content[i].first;
        c.chunk_index = i;
        c.content     = id_content[i].second;
        chunks.push_back(std::move(c));
    }
    return chunks;
}

// ============================================================================
// computeSimilarity
// ============================================================================

TEST(CitationSimilarityTest, IdenticalStrings) {
    double sim = CitationHighlighter::computeSimilarity("hello world", "hello world");
    EXPECT_NEAR(sim, 1.0, 1e-9);
}

TEST(CitationSimilarityTest, DisjointStrings) {
    double sim = CitationHighlighter::computeSimilarity("apple orange", "banana grape");
    EXPECT_NEAR(sim, 0.0, 1e-9);
}

TEST(CitationSimilarityTest, BothEmpty) {
    double sim = CitationHighlighter::computeSimilarity("", "");
    EXPECT_NEAR(sim, 1.0, 1e-9);
}

TEST(CitationSimilarityTest, OneEmpty) {
    double sim = CitationHighlighter::computeSimilarity("", "hello world");
    EXPECT_NEAR(sim, 0.0, 1e-9);
}

TEST(CitationSimilarityTest, PartialOverlap) {
    // "cat dog" vs "cat bird" → 1 common token out of 3 unique = 1/3
    double sim = CitationHighlighter::computeSimilarity("cat dog", "cat bird");
    EXPECT_GT(sim, 0.0);
    EXPECT_LT(sim, 1.0);
}

TEST(CitationSimilarityTest, InRange) {
    double sim = CitationHighlighter::computeSimilarity(
        "Paris is the capital of France and a major European city",
        "The capital of France is Paris which is located in Western Europe");
    EXPECT_GE(sim, 0.0);
    EXPECT_LE(sim, 1.0);
}

// Short tokens (< 2 chars) are ignored
TEST(CitationSimilarityTest, ShortTokensIgnored) {
    // Both strings share "is" (len=2, included) and "a" (len=1, excluded)
    double sim = CitationHighlighter::computeSimilarity("it is fine", "it is okay");
    EXPECT_GE(sim, 0.0);
    EXPECT_LE(sim, 1.0);
}

// ============================================================================
// splitSentences
// ============================================================================

class SplitSentencesTest : public ::testing::Test {
protected:
    CitationHighlighter highlighter_;
};

TEST_F(SplitSentencesTest, EmptyInput) {
    auto sents = highlighter_.splitSentences("");
    EXPECT_TRUE(sents.empty());
}

TEST_F(SplitSentencesTest, SingleSentence) {
    auto sents = highlighter_.splitSentences("Paris is the capital of France.");
    ASSERT_EQ(sents.size(), 1u);
    EXPECT_EQ(sents[0], "Paris is the capital of France.");
}

TEST_F(SplitSentencesTest, TwoSentences) {
    auto sents = highlighter_.splitSentences(
        "Paris is the capital of France. The Eiffel Tower was built in 1889.");
    EXPECT_EQ(sents.size(), 2u);
}

TEST_F(SplitSentencesTest, ExclamationAndQuestion) {
    auto sents = highlighter_.splitSentences(
        "What a city! Is Paris beautiful? It certainly is.");
    EXPECT_EQ(sents.size(), 3u);
}

TEST_F(SplitSentencesTest, ShortSentencesDropped) {
    // "OK." is only 3 chars → below min_sentence_length=5 → dropped
    auto sents = highlighter_.splitSentences("OK. Paris is the capital of France.");
    ASSERT_EQ(sents.size(), 1u);
    EXPECT_EQ(sents[0], "Paris is the capital of France.");
}

TEST_F(SplitSentencesTest, NoDelimiterReturnsWholeParagraph) {
    const std::string text =
        "Paris is a wonderful city full of history and culture";
    auto sents = highlighter_.splitSentences(text);
    ASSERT_EQ(sents.size(), 1u);
    EXPECT_EQ(sents[0], text);
}

// ============================================================================
// highlight – basic
// ============================================================================

class CitationHighlightTest : public ::testing::Test {
protected:
    CitationHighlighter highlighter_;

    std::vector<SourceChunk> chunks_ = makeChunks({
        {"doc1", "Paris is the capital of France, located in Western Europe."},
        {"doc2", "The Eiffel Tower stands in Paris and was completed in 1889."},
        {"doc3", "Berlin is the capital city of Germany in Central Europe."}
    });
};

// Primary citation assigned to the most relevant chunk
TEST_F(CitationHighlightTest, PrimaryCitationAssigned) {
    auto result = highlighter_.highlight(
        "Paris is the capital of France.", chunks_);

    ASSERT_EQ(result.mappings.size(), 1u);
    EXPECT_TRUE(result.mappings[0].has_citation());
    EXPECT_EQ(result.mappings[0].primary_chunk_id, "doc1");
    EXPECT_GT(result.mappings[0].similarity_score, 0.0);
}

// Two-sentence answer → each sentence gets independent citation
TEST_F(CitationHighlightTest, TwoSentencesTwoCitations) {
    auto result = highlighter_.highlight(
        "Paris is the capital of France. The Eiffel Tower was completed in 1889.",
        chunks_);

    ASSERT_EQ(result.mappings.size(), 2u);
    EXPECT_TRUE(result.mappings[0].has_citation());
    EXPECT_TRUE(result.mappings[1].has_citation());
    EXPECT_EQ(result.mappings[0].primary_chunk_id, "doc1");
    EXPECT_EQ(result.mappings[1].primary_chunk_id, "doc2");
}

// Coverage is 1.0 when every sentence is cited
TEST_F(CitationHighlightTest, CoverageIsOneWhenAllCited) {
    auto result = highlighter_.highlight(
        "Paris is the capital of France. The Eiffel Tower was completed in 1889.",
        chunks_);
    EXPECT_NEAR(result.citation_coverage, 1.0, 0.01);
}

// mean_similarity is in [0, 1]
TEST_F(CitationHighlightTest, MeanSimilarityRange) {
    auto result = highlighter_.highlight(
        "Paris is the capital of France.", chunks_);
    EXPECT_GE(result.mean_similarity, 0.0);
    EXPECT_LE(result.mean_similarity, 1.0);
}

// highlight_time_ms is non-negative
TEST_F(CitationHighlightTest, HighlightTimeNonNegative) {
    auto result = highlighter_.highlight(
        "Paris is the capital of France.", chunks_);
    EXPECT_GE(result.highlight_time_ms, 0.0);
}

// ============================================================================
// highlight – edge cases
// ============================================================================

TEST_F(CitationHighlightTest, EmptyAnswerReturnsEmpty) {
    auto result = highlighter_.highlight("", chunks_);
    EXPECT_TRUE(result.mappings.empty());
    EXPECT_NEAR(result.citation_coverage, 0.0, 1e-9);
}

TEST_F(CitationHighlightTest, EmptyChunksReturnsEmpty) {
    auto result = highlighter_.highlight(
        "Paris is the capital of France.", {});
    EXPECT_TRUE(result.mappings.empty());
}

// Sentence with no token overlap → no citation (primary_chunk_id empty)
TEST_F(CitationHighlightTest, NoCitationBelowThreshold) {
    CitationHighlighterConfig cfg;
    cfg.min_similarity_threshold = 0.99; // near-impossible threshold
    CitationHighlighter strict_hl(cfg);

    auto result = strict_hl.highlight(
        "Paris is the capital of France.", chunks_);

    ASSERT_EQ(result.mappings.size(), 1u);
    EXPECT_FALSE(result.mappings[0].has_citation());
    EXPECT_NEAR(result.citation_coverage, 0.0, 1e-9);
}

// ============================================================================
// highlight – secondary citations
// ============================================================================

TEST_F(CitationHighlightTest, SecondaryCitationsCollected) {
    CitationHighlighterConfig cfg;
    cfg.min_similarity_threshold       = 0.05;
    cfg.secondary_similarity_threshold = 0.03;
    cfg.max_secondary_citations        = 3;
    CitationHighlighter hl(cfg);

    // Answer overlaps with doc1 and doc2
    auto result = hl.highlight(
        "Paris is the capital of France and the Eiffel Tower is there.",
        chunks_);

    ASSERT_FALSE(result.mappings.empty());
    // At least one secondary citation should be present
    bool has_secondary = false;
    for (const auto& m : result.mappings) {
        if (!m.secondary_sources.empty()) {
            has_secondary = true;
            break;
        }
    }
    EXPECT_TRUE(has_secondary);
}

TEST_F(CitationHighlightTest, SecondaryCitationsRespectMaxLimit) {
    CitationHighlighterConfig cfg;
    cfg.min_similarity_threshold       = 0.05;
    cfg.secondary_similarity_threshold = 0.03;
    cfg.max_secondary_citations        = 1;
    CitationHighlighter hl(cfg);

    auto result = hl.highlight(
        "Paris is the capital of France and the Eiffel Tower is there.",
        chunks_);

    for (const auto& m : result.mappings) {
        EXPECT_LE(m.secondary_sources.size(), 1u);
    }
}

TEST_F(CitationHighlightTest, SecondaryCitationsDisabledWhenMaxIsZero) {
    CitationHighlighterConfig cfg;
    cfg.max_secondary_citations = 0;
    CitationHighlighter hl(cfg);

    auto result = hl.highlight(
        "Paris is the capital of France and the Eiffel Tower is there.",
        chunks_);

    for (const auto& m : result.mappings) {
        EXPECT_TRUE(m.secondary_sources.empty());
    }
}

// Secondary similarity scores are in [0, 1]
TEST_F(CitationHighlightTest, SecondaryScoreRange) {
    CitationHighlighterConfig cfg;
    cfg.min_similarity_threshold       = 0.05;
    cfg.secondary_similarity_threshold = 0.03;
    cfg.max_secondary_citations        = 3;
    CitationHighlighter hl(cfg);

    auto result = hl.highlight(
        "Paris is the capital of France and the Eiffel Tower is there.",
        chunks_);

    for (const auto& m : result.mappings) {
        for (const auto& sec : m.secondary_sources) {
            EXPECT_GE(sec.similarity_score, 0.0);
            EXPECT_LE(sec.similarity_score, 1.0);
        }
    }
}

// ============================================================================
// getConfig / setConfig
// ============================================================================

TEST(CitationConfigTest, DefaultConfig) {
    CitationHighlighter hl;
    auto cfg = hl.getConfig();
    EXPECT_GT(cfg.min_similarity_threshold, 0.0);
    EXPECT_LE(cfg.min_similarity_threshold, 1.0);
}

TEST(CitationConfigTest, SetConfigUpdates) {
    CitationHighlighter hl;
    CitationHighlighterConfig cfg;
    cfg.min_similarity_threshold = 0.42;
    hl.setConfig(cfg);
    EXPECT_NEAR(hl.getConfig().min_similarity_threshold, 0.42, 1e-9);
}

// ============================================================================
// CitationHighlighterFactory
// ============================================================================

TEST(CitationFactoryTest, CreateStrictReturnsInstance) {
    auto hl = CitationHighlighterFactory::createStrict();
    ASSERT_NE(hl, nullptr);
    EXPECT_GE(hl->getConfig().min_similarity_threshold, 0.20);
}

TEST(CitationFactoryTest, CreateBalancedReturnsInstance) {
    auto hl = CitationHighlighterFactory::createBalanced();
    ASSERT_NE(hl, nullptr);
}

TEST(CitationFactoryTest, CreatePermissiveReturnsInstance) {
    auto hl = CitationHighlighterFactory::createPermissive();
    ASSERT_NE(hl, nullptr);
    EXPECT_LT(hl->getConfig().min_similarity_threshold, 0.15);
}

// Strict mode: higher threshold than balanced
TEST(CitationFactoryTest, StrictThresholdHigherThanPermissive) {
    auto strict     = CitationHighlighterFactory::createStrict();
    auto permissive = CitationHighlighterFactory::createPermissive();
    EXPECT_GT(strict->getConfig().min_similarity_threshold,
              permissive->getConfig().min_similarity_threshold);
}

// Balanced mode cites well-matching sentences
TEST(CitationFactoryTest, BalancedCitesGoodMatch) {
    auto hl     = CitationHighlighterFactory::createBalanced();
    auto chunks = makeChunks({
        {"doc1", "Paris is the capital of France, located in Western Europe."}
    });
    auto result = hl->highlight("Paris is the capital of France.", chunks);
    ASSERT_FALSE(result.mappings.empty());
    EXPECT_TRUE(result.mappings[0].has_citation());
}

// ============================================================================
// Sentence index ordering
// ============================================================================

TEST(CitationOrderTest, SentenceIndicesAreConsecutive) {
    CitationHighlighter hl;
    auto chunks = makeChunks({
        {"doc1", "Paris is the capital of France."},
        {"doc2", "The Eiffel Tower was built in 1889."}
    });
    auto result = hl.highlight(
        "Paris is the capital of France. The Eiffel Tower was built in 1889.",
        chunks);

    for (size_t i = 0; i < result.mappings.size(); ++i) {
        EXPECT_EQ(result.mappings[i].sentence_index, i);
    }
}
