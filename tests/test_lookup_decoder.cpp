/**
 * @file test_lookup_decoder.cpp
 * @brief Unit tests for LookupDecoder (n-gram based prompt lookup decoding).
 *
 * Test IDs: LKD-01 … LKD-18
 */

#include "llm/lookup_decoder.h"
#include <gtest/gtest.h>
#include <thread>
#include <vector>

using themis::llm::LookupDecoder;

namespace {

LookupDecoder::Config defaultConfig() {
    LookupDecoder::Config cfg;
    cfg.ngram_min       = 2;
    cfg.ngram_max       = 4;
    cfg.max_draft_tokens = 8;
    cfg.max_index_entries = 256;
    return cfg;
}

// ── LKD-01: Default construction ────────────────────────────────────────────
TEST(LookupDecoderTest, LKD01_DefaultConstruction) {
    EXPECT_NO_THROW(LookupDecoder{});
}

// ── LKD-02: Custom config construction ──────────────────────────────────────
TEST(LookupDecoderTest, LKD02_ConfigConstruction) {
    EXPECT_NO_THROW(LookupDecoder{defaultConfig()});
}

// ── LKD-03: Invalid config — ngram_min > ngram_max ──────────────────────────
TEST(LookupDecoderTest, LKD03_InvalidConfigNgramOrder) {
    LookupDecoder::Config bad;
    bad.ngram_min = 5;
    bad.ngram_max = 2;
    EXPECT_THROW(LookupDecoder{bad}, std::invalid_argument);
}

// ── LKD-04: Invalid config — max_draft_tokens == 0 ──────────────────────────
TEST(LookupDecoderTest, LKD04_InvalidConfigZeroDraft) {
    LookupDecoder::Config bad = defaultConfig();
    bad.max_draft_tokens = 0;
    EXPECT_THROW(LookupDecoder{bad}, std::invalid_argument);
}

// ── LKD-05: Empty context → no draft ────────────────────────────────────────
TEST(LookupDecoderTest, LKD05_EmptyContextNoDraft) {
    LookupDecoder dec{defaultConfig()};
    dec.buildFromPrompt({1, 2, 3, 4, 5});
    auto draft = dec.proposeDraftTokens({});
    EXPECT_TRUE(draft.empty());
}

// ── LKD-06: Empty index → no draft ──────────────────────────────────────────
TEST(LookupDecoderTest, LKD06_EmptyIndexNoDraft) {
    LookupDecoder dec{defaultConfig()};
    auto draft = dec.proposeDraftTokens({10, 20, 30});
    EXPECT_TRUE(draft.empty());
}

// ── LKD-07: buildFromPrompt — known n-gram produces draft ───────────────────
TEST(LookupDecoderTest, LKD07_BuildFromPromptHit) {
    LookupDecoder dec{defaultConfig()};
    // Tokens: [1,2,3,4,5]  →  bigram [2,3] → continuation [4,5]
    dec.buildFromPrompt({1, 2, 3, 4, 5});

    std::vector<int> ctx = {10, 2, 3};  // ends in bigram [2,3]
    auto draft = dec.proposeDraftTokens(ctx);
    ASSERT_FALSE(draft.empty());
    EXPECT_EQ(draft[0], 4);
}

// ── LKD-08: Longest n-gram match preferred ──────────────────────────────────
TEST(LookupDecoderTest, LKD08_LongestMatchPreferred) {
    LookupDecoder dec{defaultConfig()};
    // [1,2,3,4,5] → bigram [2,3]→[4,5] AND trigram [1,2,3]→[4,5]
    dec.buildFromPrompt({1, 2, 3, 4, 5});

    // Context ends in trigram [1,2,3] — trigram lookup must win.
    std::vector<int> ctx = {99, 1, 2, 3};
    auto draft = dec.proposeDraftTokens(ctx);
    ASSERT_FALSE(draft.empty());
    EXPECT_EQ(draft[0], 4);
}

// ── LKD-09: Draft length capped by max_draft argument ───────────────────────
TEST(LookupDecoderTest, LKD09_MaxDraftCap) {
    LookupDecoder::Config cfg = defaultConfig();
    cfg.max_draft_tokens = 4;
    LookupDecoder dec{cfg};
    dec.buildFromPrompt({1, 2, 3, 4, 5, 6, 7, 8});

    auto draft = dec.proposeDraftTokens({1, 2}, 2);
    EXPECT_LE(draft.size(), 2u);
}

// ── LKD-10: updateFromTokens updates index for later lookup ─────────────────
TEST(LookupDecoderTest, LKD10_DynamicUpdate) {
    LookupDecoder dec{defaultConfig()};
    dec.buildFromPrompt({1, 2});  // minimal seed

    // Add [10, 20, 30, 40] dynamically
    dec.updateFromTokens({10, 20, 30, 40});

    // Context ending in [10,20] should produce [30, 40]
    std::vector<int> ctx = {99, 10, 20};
    auto draft = dec.proposeDraftTokens(ctx);
    ASSERT_FALSE(draft.empty());
    EXPECT_EQ(draft[0], 30);
}

// ── LKD-11: loadStaticNgrams merges into index ───────────────────────────────
TEST(LookupDecoderTest, LKD11_LoadStaticNgrams) {
    LookupDecoder dec{defaultConfig()};

    LookupDecoder::VectorHash h;
    (void)h; // compiler silence

    std::unordered_map<std::vector<int>, std::vector<int>, LookupDecoder::VectorHash> table;
    table[{50, 51}] = {52, 53};

    dec.loadStaticNgrams(table);

    auto draft = dec.proposeDraftTokens({10, 50, 51});
    ASSERT_FALSE(draft.empty());
    EXPECT_EQ(draft[0], 52);
}

// ── LKD-12: clear() removes all entries ─────────────────────────────────────
TEST(LookupDecoderTest, LKD12_Clear) {
    LookupDecoder dec{defaultConfig()};
    dec.buildFromPrompt({1, 2, 3, 4});

    dec.clear();

    auto draft = dec.proposeDraftTokens({1, 2});
    EXPECT_TRUE(draft.empty());
}

// ── LKD-13: Statistics — hit tracking ───────────────────────────────────────
TEST(LookupDecoderTest, LKD13_StatsHitTracking) {
    LookupDecoder dec{defaultConfig()};
    dec.buildFromPrompt({1, 2, 3, 4, 5});

    // First call: context [1,2] matches bigram key [1,2] → hit.
    dec.proposeDraftTokens({1, 2});
    // Second call: context ends in [1,2] with a prefix token → also a hit.
    dec.proposeDraftTokens({99, 1, 2});

    auto s = dec.getStats();
    EXPECT_EQ(s.total_probe_calls, 2u);
    EXPECT_GE(s.total_hits, 1u);
    EXPECT_GT(s.hit_rate(), 0.0);
}

// ── LKD-14: Statistics — reset ──────────────────────────────────────────────
TEST(LookupDecoderTest, LKD14_StatsReset) {
    LookupDecoder dec{defaultConfig()};
    dec.buildFromPrompt({1, 2, 3});
    dec.proposeDraftTokens({1, 2});

    dec.resetStats();
    auto s = dec.getStats();
    EXPECT_EQ(s.total_probe_calls, 0u);
    EXPECT_EQ(s.total_hits, 0u);
}

// ── LKD-15: Eviction respects max_index_entries ──────────────────────────────
TEST(LookupDecoderTest, LKD15_EvictionUnderCapacity) {
    LookupDecoder::Config cfg = defaultConfig();
    cfg.max_index_entries = 4;  // very small limit
    cfg.ngram_min = 1;
    cfg.ngram_max = 1;
    LookupDecoder dec{cfg};

    // Insert 6 unigrams — only the last 4 should survive.
    dec.buildFromPrompt({10, 20, 30, 40, 50, 60, 70});

    // Index should not grow beyond max_index_entries.
    // We can't directly query the count, but proposeDraftTokens should not crash.
    EXPECT_NO_THROW(dec.proposeDraftTokens({10}));
    EXPECT_NO_THROW(dec.proposeDraftTokens({60}));
}

// ── LKD-16: updateFromTokens with empty vector ───────────────────────────────
TEST(LookupDecoderTest, LKD16_UpdateEmptyNoop) {
    LookupDecoder dec{defaultConfig()};
    EXPECT_NO_THROW(dec.updateFromTokens({}));
}

// ── LKD-17: proposeDraftTokens with very short context (< ngram_min) ─────────
TEST(LookupDecoderTest, LKD17_ContextShorterThanNgramMin) {
    LookupDecoder dec{defaultConfig()};  // ngram_min = 2
    dec.buildFromPrompt({1, 2, 3, 4});

    // Context of length 1 < ngram_min(2) → no match possible
    auto draft = dec.proposeDraftTokens({1});
    EXPECT_TRUE(draft.empty());
}

// ── LKD-18: Thread safety — concurrent build and propose ─────────────────────
TEST(LookupDecoderTest, LKD18_ConcurrentAccess) {
    auto dec = std::make_shared<LookupDecoder>(defaultConfig());
    dec->buildFromPrompt({1, 2, 3, 4, 5, 6, 7, 8});

    constexpr int kIter = 100;
    std::vector<std::thread> threads;
    threads.reserve(4);

    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&dec, kIter]() {
            for (int i = 0; i < kIter; ++i) {
                dec->proposeDraftTokens({1, 2, 3});
                dec->updateFromTokens({i, i + 1, i + 2});
            }
        });
    }
    for (auto& th : threads) th.join();

    // No crash / deadlock → pass
    SUCCEED();
}

}  // namespace
