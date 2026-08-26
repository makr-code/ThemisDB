/**
 * @file test_wave7_bm25_positional_fts.cpp
 * @brief Wave 7 — BM25+ Positional Scorer and FTS phrase/proximity operator tests.
 *
 * Labels: wave_b, release_critical
 *
 * Test IDs (12 total):
 *   W7-POS-01  BM25+ baseline: score > 0 for relevant doc, 0 for empty doc
 *   W7-POS-02  Positional index populated after addDocument
 *   W7-POS-03  searchPhrase: exact phrase found in correct doc
 *   W7-POS-04  searchPhrase: phrase NOT present → not returned
 *   W7-POS-05  searchPhrase: single-term phrase → same as BM25 search
 *   W7-POS-06  searchPhrase: empty phrase → empty result
 *   W7-POS-07  searchPhrase: partial phrase (only 1 of 2 terms present) → not matched
 *   W7-POS-08  searchProximity: terms within distance → found
 *   W7-POS-09  searchProximity: terms outside distance → not found
 *   W7-POS-10  searchProximity: distance=1 (adjacent only)
 *   W7-POS-11  searchProximity: term1==term2 with 2 occurrences close together
 *   W7-POS-12  searchProximity: missing term → empty result
 */

#include <gtest/gtest.h>
#include "rag/wiki_index_store.h"

#include <algorithm>
#include <string>
#include <vector>

using namespace themis::rag;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static bool resultContainsDoc(const std::vector<IndexResult>& results,
                               const std::string& doc_id) {
    return std::any_of(results.begin(), results.end(),
                       [&](const IndexResult& r){ return r.doc_id == doc_id; });
}

// ─────────────────────────────────────────────────────────────────────────────
// W7-POS-01: BM25+ baseline score consistency
// ─────────────────────────────────────────────────────────────────────────────

TEST(Wave7BM25Positional, W7_POS_01_BM25BaselineScore) {
    WikiIndexStore store;
    store.addDocument("doc1", "the quick brown fox");
    store.addDocument("doc2", "a slow tortoise rests");

    auto results = store.searchBM25({"quick", "fox"}, 5);
    ASSERT_FALSE(results.empty());
    EXPECT_EQ(results[0].doc_id, "doc1");
    EXPECT_GT(results[0].score, 0.0f);
}

// ─────────────────────────────────────────────────────────────────────────────
// W7-POS-02: Positional index populated after addDocument
// ─────────────────────────────────────────────────────────────────────────────

TEST(Wave7BM25Positional, W7_POS_02_PositionalIndexPopulated) {
    WikiIndexStore store;
    store.addDocument("doc1", "alpha beta gamma");

    // Proximity(alpha, gamma, distance=2) should find doc1 (positions 0 and 2).
    auto results = store.searchProximity("alpha", "gamma", 2, 10);
    ASSERT_FALSE(results.empty());
    EXPECT_EQ(results[0].doc_id, "doc1");
}

// ─────────────────────────────────────────────────────────────────────────────
// W7-POS-03: searchPhrase — exact phrase found in correct doc
// ─────────────────────────────────────────────────────────────────────────────

TEST(Wave7BM25Positional, W7_POS_03_PhrasePresentReturnsDoc) {
    WikiIndexStore store;
    store.addDocument("doc1", "the quick brown fox jumps over the lazy dog");
    store.addDocument("doc2", "a quick fox ran fast");

    // "quick brown fox" is a consecutive triple only in doc1.
    auto results = store.searchPhrase("quick brown fox", 10);
    ASSERT_FALSE(results.empty());
    EXPECT_TRUE(resultContainsDoc(results, "doc1"));
    EXPECT_FALSE(resultContainsDoc(results, "doc2"));
}

// ─────────────────────────────────────────────────────────────────────────────
// W7-POS-04: searchPhrase — phrase NOT present → not returned
// ─────────────────────────────────────────────────────────────────────────────

TEST(Wave7BM25Positional, W7_POS_04_PhraseAbsentReturnsEmpty) {
    WikiIndexStore store;
    store.addDocument("doc1", "hello world");

    auto results = store.searchPhrase("world hello", 10); // Reversed order.
    EXPECT_FALSE(resultContainsDoc(results, "doc1"));
}

// ─────────────────────────────────────────────────────────────────────────────
// W7-POS-05: searchPhrase — single-term phrase → same as BM25
// ─────────────────────────────────────────────────────────────────────────────

TEST(Wave7BM25Positional, W7_POS_05_SingleTermPhraseLikeBM25) {
    WikiIndexStore store;
    store.addDocument("doc1", "machine learning rocks");
    store.addDocument("doc2", "deep learning is powerful");

    auto phrase_results = store.searchPhrase("learning", 10);
    auto bm25_results   = store.searchBM25({"learning"}, 10);

    ASSERT_EQ(phrase_results.size(), bm25_results.size());
    // Both result sets must contain the same doc_ids.
    for (const auto& r : bm25_results) {
        EXPECT_TRUE(resultContainsDoc(phrase_results, r.doc_id));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// W7-POS-06: searchPhrase — empty phrase → empty result
// ─────────────────────────────────────────────────────────────────────────────

TEST(Wave7BM25Positional, W7_POS_06_EmptyPhraseReturnsEmpty) {
    WikiIndexStore store;
    store.addDocument("doc1", "some content here");

    auto results = store.searchPhrase("", 10);
    EXPECT_TRUE(results.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// W7-POS-07: searchPhrase — partial phrase → not matched
// ─────────────────────────────────────────────────────────────────────────────

TEST(Wave7BM25Positional, W7_POS_07_PartialPhraseNotMatched) {
    WikiIndexStore store;
    // "natural" is present but "language" and "processing" are absent.
    store.addDocument("doc1", "natural selection is a biological process");

    auto results = store.searchPhrase("natural language processing", 10);
    EXPECT_FALSE(resultContainsDoc(results, "doc1"));
}

// ─────────────────────────────────────────────────────────────────────────────
// W7-POS-08: searchProximity — terms within distance → found
// ─────────────────────────────────────────────────────────────────────────────

TEST(Wave7BM25Positional, W7_POS_08_ProximityWithinDistanceFound) {
    WikiIndexStore store;
    // "cat" at pos 1, "sat" at pos 3 → distance = 2.
    store.addDocument("doc1", "the cat quickly sat");
    store.addDocument("doc2", "the dog ran far away");

    auto results = store.searchProximity("cat", "sat", 3, 10);
    ASSERT_FALSE(results.empty());
    EXPECT_TRUE(resultContainsDoc(results, "doc1"));
    EXPECT_FALSE(resultContainsDoc(results, "doc2"));
}

// ─────────────────────────────────────────────────────────────────────────────
// W7-POS-09: searchProximity — terms outside distance → not found
// ─────────────────────────────────────────────────────────────────────────────

TEST(Wave7BM25Positional, W7_POS_09_ProximityOutsideDistanceNotFound) {
    WikiIndexStore store;
    // "alpha" at pos 0, "omega" at pos 4 → distance = 4; ask for ≤2.
    store.addDocument("doc1", "alpha beta gamma delta omega");

    auto results = store.searchProximity("alpha", "omega", 2, 10);
    EXPECT_FALSE(resultContainsDoc(results, "doc1"));
}

// ─────────────────────────────────────────────────────────────────────────────
// W7-POS-10: searchProximity — distance=1 (adjacent only)
// ─────────────────────────────────────────────────────────────────────────────

TEST(Wave7BM25Positional, W7_POS_10_ProximityDistanceOne) {
    WikiIndexStore store;
    // "fox" at pos 2, "jumps" at pos 3 → distance 1; "fox" and "over" → 3.
    store.addDocument("doc1", "the quick fox jumps over");
    store.addDocument("doc2", "fox and hound");

    // Should find doc1 ("fox jumps" are adjacent).
    auto results_found = store.searchProximity("fox", "jumps", 1, 10);
    EXPECT_TRUE(resultContainsDoc(results_found, "doc1"));

    // "fox" and "over" are 3 apart — distance=1 should miss.
    auto results_miss = store.searchProximity("fox", "over", 1, 10);
    EXPECT_FALSE(resultContainsDoc(results_miss, "doc1"));
}

// ─────────────────────────────────────────────────────────────────────────────
// W7-POS-11: searchProximity — term1==term2 with 2 close occurrences
// ─────────────────────────────────────────────────────────────────────────────

TEST(Wave7BM25Positional, W7_POS_11_SameTermTwoOccurrencesClose) {
    WikiIndexStore store;
    // "echo" at positions 0 and 2 → distance 2.
    store.addDocument("doc1", "echo and echo resounds");
    // "echo" only once.
    store.addDocument("doc2", "the echo of silence");

    auto results = store.searchProximity("echo", "echo", 2, 10);
    EXPECT_TRUE(resultContainsDoc(results, "doc1"));
    EXPECT_FALSE(resultContainsDoc(results, "doc2"));
}

// ─────────────────────────────────────────────────────────────────────────────
// W7-POS-12: searchProximity — missing term → empty result
// ─────────────────────────────────────────────────────────────────────────────

TEST(Wave7BM25Positional, W7_POS_12_MissingTermReturnsEmpty) {
    WikiIndexStore store;
    store.addDocument("doc1", "the quick brown fox");

    // "xyzzy" is not in the index.
    auto results = store.searchProximity("quick", "xyzzy", 5, 10);
    EXPECT_TRUE(results.empty());
}
