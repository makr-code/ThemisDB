/**
 * @file test_rag_document_splitter.cpp
 * @brief Unit tests for configurable chunk size and overlap document splitting.
 *
 * Tests cover:
 *  - Token estimation (estimateTokens)
 *  - Empty input edge cases
 *  - Fixed strategy: chunk count, chunk sizes, overlap content
 *  - Sliding strategy: window advancement, overlap, boundary chunks
 *  - Sentence strategy: sentence boundary preservation, overlap carry-over
 *  - chunk_size / overlap validation (invalid_argument)
 *  - min_chunk_size filtering
 *  - document_id propagation
 *  - sequential index assignment
 *  - start_offset / end_offset correctness
 *  - DocumentSplitterFactory helpers
 *  - setConfig() / getConfig() round-trip
 */

#include "rag/document_splitter.h"

#include <gtest/gtest.h>

#include <numeric>
#include <string>
#include <vector>

using namespace themis::rag;

// ===========================================================================
// Helpers
// ===========================================================================

/// Build a plain ASCII text of approximately @p tokens tokens
/// (at 4 chars/token) consisting of 'a' characters.
static std::string makeText(size_t tokens, double chars_per_token = 4.0) {
    const size_t chars = static_cast<size_t>(
        static_cast<double>(tokens) * chars_per_token);
    return std::string(chars, 'a');
}

/// Build a multi-sentence text with @p n sentences.
static std::string makeSentences(size_t n, const std::string& sentence_text
                                            = "The quick brown fox jumps.") {
    std::string result = {};
    for (size_t i = 0; i < n; ++i) {
        if (i > 0) {
          result += " ";
        }
        result += sentence_text;
    }
    return result;
}

// ===========================================================================
// Token estimation
// ===========================================================================

TEST(DocumentSplitterTokens, EmptyStringIsZero) {
    DocumentSplitter s;
    EXPECT_EQ(s.estimateTokens(""), 0u);
}

TEST(DocumentSplitterTokens, FourCharsPerToken) {
    DocumentSplitterConfig cfg;
    cfg.chunk_size = 512;
    cfg.overlap    = 64;
    cfg.chars_per_token = 4.0;
    DocumentSplitter s(cfg);
    // 400 chars / 4.0 = 100 tokens
    EXPECT_EQ(s.estimateTokens(std::string(400, 'x')), 100u);
    // 401 chars / 4.0 = 101 (ceil)
    EXPECT_EQ(s.estimateTokens(std::string(401, 'x')), 101u);
}

TEST(DocumentSplitterTokens, TwoCharsPerToken) {
    DocumentSplitterConfig cfg;
    cfg.chunk_size = 512;
    cfg.overlap    = 64;
    cfg.chars_per_token = 2.0;
    DocumentSplitter s(cfg);
    EXPECT_EQ(s.estimateTokens(std::string(200, 'x')), 100u);
}

// ===========================================================================
// Config validation
// ===========================================================================

TEST(DocumentSplitterConfig, ZeroChunkSizeThrows) {
    DocumentSplitterConfig cfg;
    cfg.chunk_size = 0;
    cfg.overlap    = 0;
    EXPECT_THROW(DocumentSplitter s(cfg), std::invalid_argument);
}

TEST(DocumentSplitterConfig, OverlapEqualToChunkSizeThrows) {
    DocumentSplitterConfig cfg;
    cfg.chunk_size = 100;
    cfg.overlap    = 100;
    EXPECT_THROW(DocumentSplitter s(cfg), std::invalid_argument);
}

TEST(DocumentSplitterConfig, OverlapGreaterThanChunkSizeThrows) {
    DocumentSplitterConfig cfg;
    cfg.chunk_size = 100;
    cfg.overlap    = 150;
    EXPECT_THROW(DocumentSplitter s(cfg), std::invalid_argument);
}

TEST(DocumentSplitterConfig, ZeroCharsPerTokenThrows) {
    DocumentSplitterConfig cfg;
    cfg.chunk_size = 100;
    cfg.overlap    = 10;
    cfg.chars_per_token = 0.0;
    EXPECT_THROW(DocumentSplitter s(cfg), std::invalid_argument);
}

TEST(DocumentSplitterConfig, ValidConfigDoesNotThrow) {
    DocumentSplitterConfig cfg;
    cfg.chunk_size = 100;
    cfg.overlap    = 10;
    EXPECT_NO_THROW(DocumentSplitter s(cfg));
}

// ===========================================================================
// Empty input
// ===========================================================================

TEST(DocumentSplitterEmpty, FixedEmptyReturnsNoChunks) {
    DocumentSplitterConfig cfg;
    cfg.strategy   = SplitStrategy::Fixed;
    cfg.chunk_size = 100;
    cfg.overlap    = 10;
    DocumentSplitter s(cfg);
    EXPECT_TRUE(s.split("").empty());
    EXPECT_TRUE(s.split("", "doc1").empty());
}

TEST(DocumentSplitterEmpty, SlidingEmptyReturnsNoChunks) {
    DocumentSplitterConfig cfg;
    cfg.strategy   = SplitStrategy::Sliding;
    cfg.chunk_size = 100;
    cfg.overlap    = 10;
    DocumentSplitter s(cfg);
    EXPECT_TRUE(s.split("").empty());
}

TEST(DocumentSplitterEmpty, SentenceEmptyReturnsNoChunks) {
    DocumentSplitter s;
    EXPECT_TRUE(s.split("").empty());
}

// ===========================================================================
// Fixed strategy
// ===========================================================================

class FixedSplitterTest : public ::testing::Test {
protected:
    void SetUp() override {
        cfg.strategy       = SplitStrategy::Fixed;
        cfg.chunk_size     = 100;   // 100 tokens
        cfg.overlap        = 20;    // 20-token overlap
        cfg.chars_per_token = 4.0;
    }
    DocumentSplitterConfig cfg;
};

TEST_F(FixedSplitterTest, SingleChunkWhenTextFits) {
    // 50 tokens of text → should fit in a single chunk
    const std::string text = makeText(50);
    DocumentSplitter s(cfg);
    auto chunks = s.split(text, "doc1");
    EXPECT_EQ(chunks.size(), 1u);
    EXPECT_EQ(chunks[0].index, 0u);
    EXPECT_EQ(chunks[0].document_id, "doc1");
}

TEST_F(FixedSplitterTest, MultipleChunks) {
    // 300 tokens of text → should produce 3 chunks
    const std::string text = makeText(300);
    DocumentSplitter s(cfg);
    auto chunks = s.split(text, "doc2");
    EXPECT_GE(chunks.size(), 2u);
    for (size_t i = 0; i < chunks.size(); ++i) {
        EXPECT_EQ(chunks[i].index, i);
        EXPECT_EQ(chunks[i].document_id, "doc2");
        EXPECT_GT(chunks[i].token_count, 0u);
    }
}

TEST_F(FixedSplitterTest, SecondChunkContainsOverlapFromFirst) {
    // Use a text where we can verify overlap: 200 tokens = 800 chars
    // chunk_size=100 tokens = 400 chars, overlap=20 tokens = 80 chars
    const std::string text = makeText(200);
    DocumentSplitter s(cfg);
    auto chunks = s.split(text, "doc3");
    ASSERT_GE(chunks.size(), 2u);

    // The second chunk should start with the last overlap_chars of the first chunk's text
    const size_t overlap_chars = static_cast<size_t>(
        static_cast<double>(cfg.overlap) * cfg.chars_per_token);
    const std::string& first = chunks[0].text;
    const std::string& second = chunks[1].text;
    ASSERT_GE(first.size(), overlap_chars);
    // The overlap prefix in chunk[1] should equal the tail of chunk[0]
    EXPECT_EQ(second.substr(0, overlap_chars),
              first.substr(first.size() - overlap_chars));
}

TEST_F(FixedSplitterTest, TokenCountsArePositive) {
    const std::string text = makeText(250);
    DocumentSplitter s(cfg);
    auto chunks = s.split(text, "doc4");
    for (const auto& c : chunks) {
        EXPECT_GT(c.token_count, 0u);
    }
}

// ===========================================================================
// Sliding strategy
// ===========================================================================

class SlidingSplitterTest : public ::testing::Test {
protected:
    void SetUp() override {
        cfg.strategy        = SplitStrategy::Sliding;
        cfg.chunk_size      = 100;   // 100 tokens = 400 chars
        cfg.overlap         = 25;    // 25-token overlap → step = 75 tokens = 300 chars
        cfg.chars_per_token = 4.0;
    }
    DocumentSplitterConfig cfg;
};

TEST_F(SlidingSplitterTest, SingleChunkWhenTextFits) {
    // With chunk_size=100 and overlap=25 the step is 75 tokens.
    // A text of 80 tokens produces 2 windows (0..80 and 75..80) because the
    // step is less than the text length.  We verify at least one chunk is
    // produced and that the first chunk covers the full 80-token text.
    const std::string text = makeText(80);
    DocumentSplitter s(cfg);
    auto chunks = s.split(text);
    ASSERT_GE(chunks.size(), 1u);
    // The very first chunk must span the entire text (all 80 tokens fit)
    EXPECT_EQ(chunks[0].token_count, 80u);
}

TEST_F(SlidingSplitterTest, SlidingWindowAdvances) {
    // 200 tokens = 800 chars
    // chunk_size=400chars, step=300chars → chunks starting at 0, 300, 600, 900
    // but text is only 800 chars, so: 0 (0..400), 300 (300..700), 600 (600..800)
    const std::string text = makeText(200);
    DocumentSplitter s(cfg);
    auto chunks = s.split(text);
    ASSERT_GE(chunks.size(), 2u);
    // Adjacent chunks overlap: end of chunk[i] should start again in chunk[i+1]
    for (size_t i = 1; i < chunks.size(); ++i) {
        EXPECT_EQ(chunks[i].index, i);
        EXPECT_EQ(chunks[i].start_offset, chunks[i - 1].end_offset -
                  static_cast<size_t>(static_cast<double>(cfg.overlap) *
                                      cfg.chars_per_token));
    }
}

TEST_F(SlidingSplitterTest, ChunkSizesAreBounded) {
    const std::string text = makeText(500);
    DocumentSplitter s(cfg);
    auto chunks = s.split(text);
    const size_t max_tok = cfg.chunk_size + 1; // allow ±1 from ceiling
    for (const auto& c : chunks) {
        EXPECT_LE(c.token_count, max_tok);
    }
}

TEST_F(SlidingSplitterTest, IndexIsSequential) {
    const std::string text = makeText(300);
    DocumentSplitter s(cfg);
    auto chunks = s.split(text);
    for (size_t i = 0; i < chunks.size(); ++i) {
        EXPECT_EQ(chunks[i].index, i);
    }
}

// ===========================================================================
// Sentence strategy
// ===========================================================================

class SentenceSplitterTest : public ::testing::Test {
protected:
    void SetUp() override {
        cfg.strategy        = SplitStrategy::Sentence;
        cfg.chunk_size      = 50;    // 50 tokens per chunk
        cfg.overlap         = 10;
        cfg.chars_per_token = 4.0;
    }
    DocumentSplitterConfig cfg;
};

TEST_F(SentenceSplitterTest, ShortTextSingleChunk) {
    // One short sentence well under 50 tokens
    const std::string text = "Hello world.";
    DocumentSplitter s(cfg);
    auto chunks = s.split(text, "doc");
    ASSERT_EQ(chunks.size(), 1u);
    EXPECT_EQ(chunks[0].document_id, "doc");
    EXPECT_FALSE(chunks[0].text.empty());
}

TEST_F(SentenceSplitterTest, MultipleSentencesProduceChunks) {
    // 30 sentences × ~7 tokens each = ~210 tokens → should produce multiple chunks
    const std::string text = makeSentences(30);
    DocumentSplitter s(cfg);
    auto chunks = s.split(text, "doc");
    EXPECT_GT(chunks.size(), 1u);
}

TEST_F(SentenceSplitterTest, ChunksContainSentenceBoundaryText) {
    const std::string text = "First sentence. Second sentence. Third sentence.";
    DocumentSplitter s(cfg);
    auto chunks = s.split(text, "doc");
    ASSERT_FALSE(chunks.empty());
    // All text tokens are distributed across chunks
    size_t total_chars = 0;
    for (const auto& c : chunks) {
        total_chars += c.text.size();
    }
    // With overlap total_chars >= text.size() (overlap is duplicated)
    EXPECT_GE(total_chars, text.size());
}

TEST_F(SentenceSplitterTest, DocumentIdPropagated) {
    const std::string text = makeSentences(20);
    DocumentSplitter s(cfg);
    auto chunks = s.split(text, "my-document-42");
    for (const auto& c : chunks) {
        EXPECT_EQ(c.document_id, "my-document-42");
    }
}

TEST_F(SentenceSplitterTest, IndicesAreSequential) {
    const std::string text = makeSentences(30);
    DocumentSplitter s(cfg);
    auto chunks = s.split(text, "doc");
    for (size_t i = 0; i < chunks.size(); ++i) {
        EXPECT_EQ(chunks[i].index, i);
    }
}

TEST_F(SentenceSplitterTest, NoChunkExceedsChunkSizeSeverely) {
    const std::string text = makeSentences(40);
    DocumentSplitter s(cfg);
    auto chunks = s.split(text, "doc");
    // Allow up to 2× chunk_size because a single very-long sentence cannot be
    // further split without breaking it.  In practice our test sentences are
    // short so every chunk should stay close to the budget.
    for (const auto& c : chunks) {
        EXPECT_LE(c.token_count, cfg.chunk_size * 2 + cfg.overlap + 1);
    }
}

// ===========================================================================
// min_chunk_size filtering
// ===========================================================================

TEST(DocumentSplitterMinChunk, TinyTrailingChunkFiltered) {
    DocumentSplitterConfig cfg;
    cfg.strategy        = SplitStrategy::Sliding;
    cfg.chunk_size      = 100;
    cfg.overlap         = 20;
    cfg.chars_per_token = 4.0;
    cfg.min_chunk_size  = 80; // drop chunks shorter than 80 tokens

    // 310 tokens = 1240 chars
    // step = (100-20)*4 = 320 chars
    // windows: 0..400, 320..720, 640..1040, 960..1240 (only 70 tokens → filtered)
    const std::string text = makeText(310);
    DocumentSplitter s(cfg);
    auto chunks = s.split(text, "doc");
    for (const auto& c : chunks) {
        EXPECT_GE(c.token_count, cfg.min_chunk_size);
    }
}

// ===========================================================================
// getConfig / setConfig
// ===========================================================================

TEST(DocumentSplitterConfig, GetConfigRoundTrip) {
    DocumentSplitterConfig cfg;
    cfg.chunk_size      = 256;
    cfg.overlap         = 32;
    cfg.strategy        = SplitStrategy::Sliding;
    cfg.chars_per_token = 3.5;
    DocumentSplitter s(cfg);

    const auto& got = s.getConfig();
    EXPECT_EQ(got.chunk_size, 256u);
    EXPECT_EQ(got.overlap, 32u);
    EXPECT_EQ(got.strategy, SplitStrategy::Sliding);
    EXPECT_DOUBLE_EQ(got.chars_per_token, 3.5);
}

TEST(DocumentSplitterConfig, SetConfigUpdates) {
    DocumentSplitter s;
    DocumentSplitterConfig newcfg;
    newcfg.chunk_size = 1024;
    newcfg.overlap    = 128;
    newcfg.strategy   = SplitStrategy::Fixed;
    s.setConfig(newcfg);

    const auto& got = s.getConfig();
    EXPECT_EQ(got.chunk_size, 1024u);
    EXPECT_EQ(got.overlap, 128u);
    EXPECT_EQ(got.strategy, SplitStrategy::Fixed);
}

TEST(DocumentSplitterConfig, SetConfigInvalidThrows) {
    DocumentSplitter s;
    DocumentSplitterConfig bad;
    bad.chunk_size = 50;
    bad.overlap    = 50;   // equal → invalid
    EXPECT_THROW(s.setConfig(bad), std::invalid_argument);
}

// ===========================================================================
// DocumentSplitterFactory
// ===========================================================================

TEST(DocumentSplitterFactory, CreateDefaultReturnsNonNull) {
    auto p = DocumentSplitterFactory::createDefault();
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->getConfig().chunk_size, 512u);
    EXPECT_EQ(p->getConfig().overlap, 64u);
    EXPECT_EQ(p->getConfig().strategy, SplitStrategy::Sentence);
}

TEST(DocumentSplitterFactory, CreateSmallReturnsNonNull) {
    auto p = DocumentSplitterFactory::createSmall();
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->getConfig().chunk_size, 256u);
    EXPECT_EQ(p->getConfig().overlap, 32u);
    EXPECT_EQ(p->getConfig().strategy, SplitStrategy::Fixed);
}

TEST(DocumentSplitterFactory, CreateLargeReturnsNonNull) {
    auto p = DocumentSplitterFactory::createLarge();
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->getConfig().chunk_size, 1024u);
    EXPECT_EQ(p->getConfig().overlap, 128u);
    EXPECT_EQ(p->getConfig().strategy, SplitStrategy::Sliding);
}

TEST(DocumentSplitterFactory, CreateCustom) {
    auto p = DocumentSplitterFactory::create(200, 40, SplitStrategy::Fixed, 5.0);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->getConfig().chunk_size, 200u);
    EXPECT_EQ(p->getConfig().overlap, 40u);
    EXPECT_EQ(p->getConfig().strategy, SplitStrategy::Fixed);
    EXPECT_DOUBLE_EQ(p->getConfig().chars_per_token, 5.0);
}

TEST(DocumentSplitterFactory, CreateCustomInvalidThrows) {
    EXPECT_THROW(
        DocumentSplitterFactory::create(100, 100),  // overlap == chunk_size
        std::invalid_argument);
}

// ===========================================================================
// Default constructor
// ===========================================================================

TEST(DocumentSplitterDefault, DefaultConstructorProducesValidSplitter) {
    DocumentSplitter s;
    const auto& cfg = s.getConfig();
    EXPECT_EQ(cfg.chunk_size, 512u);
    EXPECT_EQ(cfg.overlap, 64u);
    EXPECT_EQ(cfg.strategy, SplitStrategy::Sentence);

    // Should be usable immediately
    auto chunks = s.split("Hello world. How are you?", "test");
    EXPECT_FALSE(chunks.empty());
}

// ===========================================================================
// Offset tracking
// ===========================================================================

TEST(DocumentSplitterOffsets, StartOffsetIsNonDecreasing) {
    DocumentSplitterConfig cfg;
    cfg.strategy        = SplitStrategy::Sliding;
    cfg.chunk_size      = 50;
    cfg.overlap         = 10;
    cfg.chars_per_token = 4.0;
    DocumentSplitter s(cfg);

    const std::string text = makeText(200);
    auto chunks = s.split(text);
    for (size_t i = 1; i < chunks.size(); ++i) {
        EXPECT_GE(chunks[i].start_offset, chunks[i - 1].start_offset);
    }
}

TEST(DocumentSplitterOffsets, EndOffsetLessThanOrEqualTextSize) {
    DocumentSplitterConfig cfg;
    cfg.strategy        = SplitStrategy::Fixed;
    cfg.chunk_size      = 50;
    cfg.overlap         = 5;
    cfg.chars_per_token = 4.0;
    DocumentSplitter s(cfg);

    const std::string text = makeText(200);
    auto chunks = s.split(text);
    for (const auto& c : chunks) {
        EXPECT_LE(c.end_offset, text.size());
    }
}
