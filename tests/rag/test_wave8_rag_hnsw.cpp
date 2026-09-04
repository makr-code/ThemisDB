/**
 * @file test_wave8_rag_hnsw.cpp
 * @brief Unit tests for Wave 8 RAG enhancements: HNSW injection bridge,
 *        embedding cache, and hybrid BM25+/HNSW search with RRF fusion.
 *
 * Test IDs: W8-RAG-01 .. W8-RAG-15
 *
 * Covers:
 *  - WikiIndexStoreConfig HNSW and cache parameter fields (W8-20)
 *  - addVector(): dimension enforcement and enable_hnsw guard (W8-18)
 *  - searchHNSW(): cosine ranking, dimension mismatch, disabled guard (W8-18)
 *  - cacheEmbedding() / retrieveEmbedding(): insert, hit, miss, LRU eviction (W8-19)
 *  - searchHybrid(): BM25+ only fallback, combined RRF fusion (W8-21)
 */

#include <gtest/gtest.h>
#include "rag/wiki_index_store.h"

using namespace themis::rag;

namespace {

/// Build a simple WikiIndexStore with BM25 docs and optional HNSW.
WikiIndexStore buildStore(bool enable_hnsw = false,
                           size_t max_cache = 100) {
    WikiIndexStoreConfig cfg;
    cfg.enable_hnsw    = enable_hnsw;
    cfg.max_cache_size = max_cache;
    WikiIndexStore store{cfg};
    store.addDocument("doc_a", "apple banana cherry");
    store.addDocument("doc_b", "banana date elderberry");
    store.addDocument("doc_c", "cherry fig grape");
    return store;
}

/// Returns a unit-norm vector with a 1 in position @p dim_one_hot and rest 0.
std::vector<float> oneHotVec(size_t dim, size_t pos) {
    std::vector<float> v(dim, 0.0f);
    if (pos < dim) {
      v[pos] = 1.0f;
    }
    return v;
}

/// Random-ish unit vector (deterministic from seed).
std::vector<float> pseudoVec(size_t dim, float seed) {
    std::vector<float> v(dim);
    float norm = 0.0f;
    for (size_t i = 0; i < dim; ++i) {
        v[i] = std::sin(seed + static_cast<float>(i) * 0.37f);
        norm += v[i] * v[i];
    }
    norm = std::sqrt(norm);
    for (auto& x : v) {
      x /= norm;
    }
    return v;
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// W8-20: Config field presence
// ─────────────────────────────────────────────────────────────────────────────

/// W8-RAG-01: Default config has HNSW disabled and sane defaults.
TEST(WikiIndexStoreWave8Config, DefaultConfigHNSWDisabled) {
    WikiIndexStoreConfig cfg;
    EXPECT_FALSE(cfg.enable_hnsw);
    EXPECT_EQ(cfg.hnsw_m, 16u);
    EXPECT_EQ(cfg.hnsw_ef, 200u);
    EXPECT_EQ(cfg.hnsw_ef_construction, 200u);
    EXPECT_EQ(cfg.hnsw_max_m0, 32u);
    EXPECT_EQ(cfg.cache_ttl_seconds, 3600);
    EXPECT_EQ(cfg.max_cache_size, 10000u);
    EXPECT_TRUE(cfg.cache_dir.empty());
}

/// W8-RAG-02: Config fields can be set and survive construction.
TEST(WikiIndexStoreWave8Config, ConfigFieldsRoundTrip) {
    WikiIndexStoreConfig cfg;
    cfg.enable_hnsw         = true;
    cfg.hnsw_m              = 32;
    cfg.hnsw_ef             = 400;
    cfg.hnsw_ef_construction = 400;
    cfg.hnsw_max_m0         = 64;
    cfg.cache_dir           = "/tmp/test_cache";
    cfg.cache_ttl_seconds   = 7200;
    cfg.max_cache_size      = 500;

    WikiIndexStore store{cfg};
    EXPECT_EQ(store.size(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// W8-18: addVector / searchHNSW
// ─────────────────────────────────────────────────────────────────────────────

/// W8-RAG-03: addVector no-ops gracefully when HNSW is disabled.
TEST(WikiIndexStoreWave8HNSW, AddVectorNoOpWhenDisabled) {
    auto store = buildStore(/*enable_hnsw=*/false);
    // Should not throw; silently ignored.
    EXPECT_NO_THROW(store.addVector("doc_a", oneHotVec(4, 0)));
    // searchHNSW returns empty when disabled.
    auto results = store.searchHNSW(oneHotVec(4, 0), 5);
    EXPECT_TRUE(results.empty());
}

/// W8-RAG-04: addVector rejects empty embeddings.
TEST(WikiIndexStoreWave8HNSW, AddVectorRejectsEmpty) {
    WikiIndexStoreConfig cfg;
    cfg.enable_hnsw = true;
    WikiIndexStore store{cfg};
    EXPECT_THROW(store.addVector("doc_a", {}), std::invalid_argument);
}

/// W8-RAG-05: addVector enforces consistent dimensionality.
TEST(WikiIndexStoreWave8HNSW, AddVectorEnforcesDim) {
    WikiIndexStoreConfig cfg;
    cfg.enable_hnsw = true;
    WikiIndexStore store{cfg};
    store.addVector("doc_a", oneHotVec(4, 0)); // sets dim = 4
    EXPECT_THROW(store.addVector("doc_b", oneHotVec(8, 0)), // dim 8 ≠ 4
                 std::invalid_argument);
}

/// W8-RAG-06: searchHNSW ranks by cosine similarity.
TEST(WikiIndexStoreWave8HNSW, SearchHNSWCosineSimilarityRanking) {
    WikiIndexStoreConfig cfg;
    cfg.enable_hnsw = true;
    WikiIndexStore store{cfg};
    // Three orthogonal dimensions; best match is e[0].
    store.addVector("doc_a", oneHotVec(3, 0));
    store.addVector("doc_b", oneHotVec(3, 1));
    store.addVector("doc_c", oneHotVec(3, 2));

    auto results = store.searchHNSW(oneHotVec(3, 0), 3);
    ASSERT_EQ(results.size(), 3u);
    EXPECT_EQ(results[0].doc_id, "doc_a");
    EXPECT_NEAR(results[0].score, 1.0f, 1e-5f); // same vector ≈ cos=1
    EXPECT_LE(results[1].score, results[0].score);
    EXPECT_LE(results[2].score, results[1].score);
}

/// W8-RAG-07: searchHNSW respects top_k limit.
TEST(WikiIndexStoreWave8HNSW, SearchHNSWTopKRespected) {
    WikiIndexStoreConfig cfg;
    cfg.enable_hnsw = true;
    WikiIndexStore store{cfg};
    for (int i = 0; i < 10; ++i) {
        store.addVector("doc_" + std::to_string(i), pseudoVec(8, static_cast<float>(i)));
    }
    auto results = store.searchHNSW(pseudoVec(8, 0.0f), 3);
    EXPECT_LE(results.size(), 3u);
}

/// W8-RAG-08: searchHNSW returns empty on dimension mismatch.
TEST(WikiIndexStoreWave8HNSW, SearchHNSWDimMismatchReturnsEmpty) {
    WikiIndexStoreConfig cfg;
    cfg.enable_hnsw = true;
    WikiIndexStore store{cfg};
    store.addVector("doc_a", oneHotVec(4, 0));
    auto results = store.searchHNSW(oneHotVec(8, 0), 5); // wrong dim
    EXPECT_TRUE(results.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// W8-19: cacheEmbedding / retrieveEmbedding
// ─────────────────────────────────────────────────────────────────────────────

/// W8-RAG-09: Cache miss returns empty vector.
TEST(WikiIndexStoreWave8Cache, CacheMissReturnsEmpty) {
    auto store = buildStore();
    auto result = store.retrieveEmbedding("nonexistent_key");
    EXPECT_TRUE(result.empty());
}

/// W8-RAG-10: Cached embedding is retrievable.
TEST(WikiIndexStoreWave8Cache, CacheInsertAndRetrieve) {
    auto store = buildStore(/*enable_hnsw=*/false, /*max_cache=*/100);
    const auto emb = pseudoVec(4, 1.0f);
    store.cacheEmbedding("key1", emb);
    auto retrieved = store.retrieveEmbedding("key1");
    ASSERT_EQ(retrieved.size(), emb.size());
    // Stored as unit-norm; original is already unit-norm.
    for (size_t i = 0; i < emb.size(); ++i) {
        EXPECT_NEAR(retrieved[i], emb[i], 1e-5f);
    }
}

/// W8-RAG-11: LRU eviction triggers at max_cache_size.
TEST(WikiIndexStoreWave8Cache, LRUEvictionAtCapacity) {
    WikiIndexStoreConfig cfg;
    cfg.max_cache_size = 3; // small cap
    WikiIndexStore store{cfg};

    store.cacheEmbedding("k1", pseudoVec(4, 1.0f));
    store.cacheEmbedding("k2", pseudoVec(4, 2.0f));
    store.cacheEmbedding("k3", pseudoVec(4, 3.0f));
    // Inserting k4 should evict the LRU entry (k1).
    store.cacheEmbedding("k4", pseudoVec(4, 4.0f));

    EXPECT_TRUE(store.retrieveEmbedding("k1").empty());  // evicted
    EXPECT_FALSE(store.retrieveEmbedding("k2").empty()); // still present
    EXPECT_FALSE(store.retrieveEmbedding("k3").empty());
    EXPECT_FALSE(store.retrieveEmbedding("k4").empty());
}

/// W8-RAG-12: Empty key and empty embedding are silently ignored.
TEST(WikiIndexStoreWave8Cache, EmptyKeyAndEmbeddingIgnored) {
    auto store = buildStore();
    EXPECT_NO_THROW(store.cacheEmbedding("", pseudoVec(4, 1.0f)));
    EXPECT_NO_THROW(store.cacheEmbedding("key", {}));
    EXPECT_TRUE(store.retrieveEmbedding("").empty());
    EXPECT_TRUE(store.retrieveEmbedding("key").empty()); // not inserted
}

// ─────────────────────────────────────────────────────────────────────────────
// W8-21: searchHybrid
// ─────────────────────────────────────────────────────────────────────────────

/// W8-RAG-13: searchHybrid falls back to BM25+ when HNSW is disabled.
TEST(WikiIndexStoreWave8Hybrid, FallbackToBM25WhenHNSWDisabled) {
    auto store = buildStore(/*enable_hnsw=*/false);
    auto hybrid = store.searchHybrid({"banana"}, oneHotVec(3, 0), 5);
    auto bm25   = store.searchBM25({"banana"}, 5);
    ASSERT_EQ(hybrid.size(), bm25.size());
    for (size_t i = 0; i < hybrid.size(); ++i) {
        EXPECT_EQ(hybrid[i].doc_id, bm25[i].doc_id);
    }
}

/// W8-RAG-14: searchHybrid falls back to BM25+ when query embedding is empty.
TEST(WikiIndexStoreWave8Hybrid, FallbackToBM25WhenEmptyEmbedding) {
    WikiIndexStoreConfig cfg;
    cfg.enable_hnsw = true;
    WikiIndexStore store{cfg};
    store.addDocument("doc_a", "apple banana");
    store.addDocument("doc_b", "banana date");
    store.addVector("doc_a", oneHotVec(3, 0));
    store.addVector("doc_b", oneHotVec(3, 1));

    // Empty embedding → HNSW path skipped → pure BM25+.
    auto hybrid = store.searchHybrid({"banana"}, {}, 5);
    EXPECT_FALSE(hybrid.empty());
}

/// W8-RAG-15: searchHybrid with both lists produces fused non-empty results.
TEST(WikiIndexStoreWave8Hybrid, HybridFusionProducesResults) {
    WikiIndexStoreConfig cfg;
    cfg.enable_hnsw = true;
    WikiIndexStore store{cfg};
    store.addDocument("doc_a", "apple banana cherry");
    store.addDocument("doc_b", "banana date elderberry");
    store.addDocument("doc_c", "cherry fig grape");
    store.addVector("doc_a", oneHotVec(3, 0));
    store.addVector("doc_b", oneHotVec(3, 1));
    store.addVector("doc_c", oneHotVec(3, 2));

    // BM25+ should rank doc_a and doc_b for "banana"; HNSW ranks doc_a first.
    auto results = store.searchHybrid({"banana"}, oneHotVec(3, 0), 3);
    EXPECT_FALSE(results.empty());
    EXPECT_LE(results.size(), 3u);
    // doc_a should appear in the result (top BM25+ and top HNSW).
    auto it = std::find_if(results.begin(), results.end(),
                           [](const IndexResult& r) { return r.doc_id == "doc_a"; });
    EXPECT_NE(it, results.end());
}
