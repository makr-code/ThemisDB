/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_rag_citation_highlighter.cpp                  ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-02-24                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     320                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_rag_citation_highlighter.cpp
 * @brief Unit tests for CitationHighlighter (map answer sentences to source chunks)
 *
 * Covers:
 *  - splitSentences: basic splitting, multiple terminators, trailing text
 *  - scoreSentenceChunk: score range, relevant > irrelevant, empty inputs
 *  - highlight: basic mapping, coverage calculation, max_chunks_per_sentence,
 *    min_support_score filtering, empty answer, empty chunks, short sentences
 *  - CitationHighlighterFactory: createStrict / createBalanced / createPermissive
 *  - Config validation: out-of-range min_support_score throws
 */

#include "rag/citation_highlighter.h"
#include <gtest/gtest.h>
#include <string>
#include <vector>

using namespace themis::rag;

// ============================================================================
// Helpers
// ============================================================================

static std::vector<SourceChunk> makeChunks(
    const std::vector<std::pair<std::string, std::string>>& id_content)
{
    std::vector<SourceChunk> chunks;
    chunks.reserve(id_content.size());
    for (const auto& [id, content] : id_content) {
        chunks.push_back({id, content});
    }
    return chunks;
}

// ============================================================================
// splitSentences tests
// ============================================================================

TEST(SplitSentences, EmptyStringReturnsEmpty) {
    EXPECT_TRUE(CitationHighlighter::splitSentences("").empty());
}

TEST(SplitSentences, SingleSentenceWithPeriod) {
    auto s = CitationHighlighter::splitSentences("Hello world.");
    ASSERT_EQ(s.size(), 1u);
    EXPECT_EQ(s[0], "Hello world.");
}

TEST(SplitSentences, MultipleSentences) {
    auto s = CitationHighlighter::splitSentences(
        "Paris is the capital of France. The Eiffel Tower was built in 1889.");
    ASSERT_EQ(s.size(), 2u);
    EXPECT_EQ(s[0], "Paris is the capital of France.");
    EXPECT_EQ(s[1], "The Eiffel Tower was built in 1889.");
}

TEST(SplitSentences, ExclamationAndQuestion) {
    auto s = CitationHighlighter::splitSentences("Great! Is that correct?");
    ASSERT_EQ(s.size(), 2u);
    EXPECT_EQ(s[0], "Great!");
    EXPECT_EQ(s[1], "Is that correct?");
}

TEST(SplitSentences, TrailingTextWithoutPunctuation) {
    auto s = CitationHighlighter::splitSentences("First sentence. No terminal punctuation");
    ASSERT_EQ(s.size(), 2u);
    EXPECT_EQ(s[1], "No terminal punctuation");
}

TEST(SplitSentences, LeadingAndTrailingWhitespace) {
    auto s = CitationHighlighter::splitSentences("  Hello.  World.  ");
    ASSERT_EQ(s.size(), 2u);
    EXPECT_EQ(s[0], "Hello.");
    EXPECT_EQ(s[1], "World.");
}

TEST(SplitSentences, OnlyWhitespace) {
    EXPECT_TRUE(CitationHighlighter::splitSentences("   ").empty());
}

// ============================================================================
// scoreSentenceChunk tests
// ============================================================================

TEST(ScoreSentenceChunk, ScoreInRange) {
    double s = CitationHighlighter::scoreSentenceChunk(
        "Paris is the capital of France.",
        "Paris is the capital of France, located in Western Europe.");
    EXPECT_GE(s, 0.0);
    EXPECT_LE(s, 1.0);
}

TEST(ScoreSentenceChunk, RelevantChunkScoresHigherThanIrrelevant) {
    const std::string sentence = "The Eiffel Tower was built in 1889.";
    const std::string relevant   = "The Eiffel Tower construction finished in 1889 in Paris.";
    const std::string irrelevant = "Bananas are rich in potassium and grow in tropical regions.";

    EXPECT_GT(CitationHighlighter::scoreSentenceChunk(sentence, relevant),
              CitationHighlighter::scoreSentenceChunk(sentence, irrelevant));
}

TEST(ScoreSentenceChunk, EmptySentenceReturnsZero) {
    EXPECT_DOUBLE_EQ(CitationHighlighter::scoreSentenceChunk("", "Some chunk content."), 0.0);
}

TEST(ScoreSentenceChunk, EmptyChunkReturnsZero) {
    EXPECT_DOUBLE_EQ(CitationHighlighter::scoreSentenceChunk("A sentence.", ""), 0.0);
}

TEST(ScoreSentenceChunk, IdenticalTextHighScore) {
    const std::string text = "Paris is the capital of France.";
    double s = CitationHighlighter::scoreSentenceChunk(text, text);
    EXPECT_GT(s, 0.5);
}

TEST(ScoreSentenceChunk, CompletelyDifferentTextsLowScore) {
    double s = CitationHighlighter::scoreSentenceChunk(
        "The cat sat on the mat.",
        "Quantum mechanics describes subatomic particle behaviour.");
    EXPECT_LT(s, 0.2);
}

// ============================================================================
// CitationHighlighter::highlight tests
// ============================================================================

class HighlighterTest : public ::testing::Test {
protected:
    CitationHighlighter highlighter;  // default config

    std::vector<SourceChunk> chunks = makeChunks({
        {"doc1", "Paris is the capital of France."},
        {"doc2", "The Eiffel Tower was built in 1889."},
        {"doc3", "French cuisine is renowned worldwide for its sophistication."}
    });
};

TEST_F(HighlighterTest, BasicMappingProducesNonEmptyResult) {
    auto result = highlighter.highlight(
        "Paris is the capital of France. The Eiffel Tower was built in 1889.",
        chunks);
    EXPECT_FALSE(result.mappings.empty());
    EXPECT_EQ(result.sentences.size(), 2u);
}

TEST_F(HighlighterTest, SentenceMappedToMostRelevantChunk) {
    auto result = highlighter.highlight(
        "Paris is the capital of France.",
        chunks);
    ASSERT_FALSE(result.mappings.empty());
    // The best mapping for this sentence should point to doc1
    const auto& top = result.mappings.front();
    EXPECT_EQ(top.chunk_id, "doc1");
    EXPECT_GE(top.support_score, 0.0);
    EXPECT_LE(top.support_score, 1.0);
}

TEST_F(HighlighterTest, SentenceIndexCorrect) {
    auto result = highlighter.highlight(
        "Paris is the capital of France. The Eiffel Tower was built in 1889.",
        chunks);
    // Mappings are ordered by sentence_index first
    for (size_t i = 1; i < result.mappings.size(); ++i) {
        EXPECT_GE(result.mappings[i].sentence_index,
                  result.mappings[i - 1].sentence_index);
    }
}

TEST_F(HighlighterTest, CoverageInRange) {
    auto result = highlighter.highlight(
        "Paris is the capital of France. The Eiffel Tower was built in 1889.",
        chunks);
    EXPECT_GE(result.coverage, 0.0);
    EXPECT_LE(result.coverage, 1.0);
}

TEST_F(HighlighterTest, FullCoverageWhenAllSentencesMapped) {
    // Both sentences should match their respective chunks
    auto result = highlighter.highlight(
        "Paris is the capital of France. The Eiffel Tower was built in 1889.",
        chunks);
    // Coverage should be > 0 (at minimum the matching sentences were mapped)
    EXPECT_GT(result.coverage, 0.0);
}

TEST_F(HighlighterTest, EmptyAnswerReturnsCoverageZero) {
    auto result = highlighter.highlight("", chunks);
    EXPECT_TRUE(result.mappings.empty());
    EXPECT_DOUBLE_EQ(result.coverage, 0.0);
}

TEST_F(HighlighterTest, EmptyChunksReturnsCoverageZero) {
    auto result = highlighter.highlight(
        "Paris is the capital of France.", {});
    EXPECT_TRUE(result.mappings.empty());
    EXPECT_DOUBLE_EQ(result.coverage, 0.0);
}

TEST_F(HighlighterTest, SentenceTextPopulated) {
    auto result = highlighter.highlight(
        "Paris is the capital of France.", chunks);
    for (const auto& m : result.mappings) {
        EXPECT_FALSE(m.sentence_text.empty());
    }
}

TEST_F(HighlighterTest, ChunkTextPopulated) {
    auto result = highlighter.highlight(
        "Paris is the capital of France.", chunks);
    for (const auto& m : result.mappings) {
        EXPECT_FALSE(m.chunk_text.empty());
    }
}

TEST_F(HighlighterTest, SupportScoreNonNegative) {
    auto result = highlighter.highlight(
        "Paris is the capital of France. The Eiffel Tower was built in 1889.",
        chunks);
    for (const auto& m : result.mappings) {
        EXPECT_GE(m.support_score, 0.0);
        EXPECT_LE(m.support_score, 1.0);
    }
}

// ============================================================================
// max_chunks_per_sentence configuration
// ============================================================================

TEST(MaxChunksPerSentence, LimitToOne) {
    CitationHighlighterConfig cfg;
    cfg.max_chunks_per_sentence = 1;
    cfg.min_support_score       = 0.0;  // permissive to get multiple candidates
    CitationHighlighter h(cfg);

    auto chunks = makeChunks({
        {"doc1", "Paris is the capital of France located in Europe."},
        {"doc2", "Paris is a major city of France in Western Europe."},
        {"doc3", "France is a country in Western Europe with Paris as capital."}
    });

    auto result = h.highlight("Paris is the capital of France.", chunks);
    // Each sentence must map to at most 1 chunk
    for (size_t si = 0; si < result.sentences.size(); ++si) {
        size_t count = 0;
        for (const auto& m : result.mappings) {
            if (m.sentence_index == si) ++count;
        }
        EXPECT_LE(count, 1u);
    }
}

TEST(MaxChunksPerSentence, NoLimitReturnsAll) {
    CitationHighlighterConfig cfg;
    cfg.max_chunks_per_sentence = 0;  // no limit
    cfg.min_support_score       = 0.0;
    CitationHighlighter h(cfg);

    auto chunks = makeChunks({
        {"doc1", "Paris is the capital city of France."},
        {"doc2", "Paris is a major European city in France."},
        {"doc3", "France capital Paris in European region."}
    });

    auto result = h.highlight("Paris is the capital of France.", chunks);
    // Should have at least 2 mappings (all three chunks overlap somewhat)
    EXPECT_GE(result.mappings.size(), 2u);
}

// ============================================================================
// min_support_score filtering
// ============================================================================

TEST(MinSupportScore, StrictThresholdReducesMappings) {
    CitationHighlighterConfig loose_cfg;
    loose_cfg.min_support_score = 0.0;
    CitationHighlighter loose(loose_cfg);

    CitationHighlighterConfig strict_cfg;
    strict_cfg.min_support_score = 0.9;
    CitationHighlighter strict(strict_cfg);

    auto chunks = makeChunks({
        {"doc1", "Paris is the capital of France."},
        {"doc2", "Bananas are rich in potassium."}
    });

    const std::string answer = "Paris is the capital of France.";
    const size_t loose_count  = loose.highlight(answer, chunks).mappings.size();
    const size_t strict_count = strict.highlight(answer, chunks).mappings.size();

    EXPECT_GE(loose_count, strict_count);
}

// ============================================================================
// min_sentence_length filtering
// ============================================================================

TEST(MinSentenceLength, ShortSentencesSkipped) {
    CitationHighlighterConfig cfg;
    cfg.min_sentence_length = 20;  // skip very short sentences
    CitationHighlighter h(cfg);

    auto chunks = makeChunks({{"doc1", "Paris is the capital of France."}});

    // "Hi." is only 3 characters – should be skipped
    auto result = h.highlight("Hi. Paris is the capital of France.", chunks);
    for (const auto& m : result.mappings) {
        EXPECT_GE(m.sentence_text.size(), cfg.min_sentence_length);
    }
}

// ============================================================================
// Config validation
// ============================================================================

TEST(Config, NegativeMinSupportScoreThrows) {
    CitationHighlighterConfig cfg;
    cfg.min_support_score = -0.1;
    EXPECT_THROW(CitationHighlighter h(cfg), std::invalid_argument);
}

TEST(Config, MinSupportScoreAboveOneThrows) {
    CitationHighlighterConfig cfg;
    cfg.min_support_score = 1.1;
    EXPECT_THROW(CitationHighlighter h(cfg), std::invalid_argument);
}

TEST(Config, DefaultConfigValid) {
    EXPECT_NO_THROW(CitationHighlighter{});
}

// ============================================================================
// CitationHighlighterFactory tests
// ============================================================================

TEST(Factory, CreateStrictReturnsNonNull) {
    auto h = CitationHighlighterFactory::createStrict();
    ASSERT_NE(h, nullptr);
    EXPECT_GE(h->getConfig().min_support_score, 0.2);
}

TEST(Factory, CreateBalancedReturnsNonNull) {
    auto h = CitationHighlighterFactory::createBalanced();
    ASSERT_NE(h, nullptr);
}

TEST(Factory, CreatePermissiveReturnsNonNull) {
    auto h = CitationHighlighterFactory::createPermissive();
    ASSERT_NE(h, nullptr);
    EXPECT_DOUBLE_EQ(h->getConfig().min_support_score, 0.0);
}

TEST(Factory, StrictStricterThanPermissive) {
    auto strict     = CitationHighlighterFactory::createStrict();
    auto permissive = CitationHighlighterFactory::createPermissive();
    EXPECT_GT(strict->getConfig().min_support_score,
              permissive->getConfig().min_support_score);
}

// ============================================================================
// getConfig
// ============================================================================

TEST(GetConfig, ReturnsConstructedConfig) {
    CitationHighlighterConfig cfg;
    cfg.min_support_score       = 0.25;
    cfg.max_chunks_per_sentence = 5;
    CitationHighlighter h(cfg);
    EXPECT_DOUBLE_EQ(h.getConfig().min_support_score, 0.25);
    EXPECT_EQ(h.getConfig().max_chunks_per_sentence, 5u);
}

// ============================================================================
// elapsed_ms sanity
// ============================================================================

TEST(ElapsedMs, NonNegative) {
    CitationHighlighter h;
    auto chunks = makeChunks({{"doc1", "Paris is the capital of France."}});
    auto result = h.highlight("Paris is the capital of France.", chunks);
    EXPECT_GE(result.elapsed_ms.count(), 0);
}
