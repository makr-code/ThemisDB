#include <gtest/gtest.h>
#include "search/distributed_hybrid_search.h"
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <vector>
#include <string>

using namespace themis;

// ============================================================================
// Helpers
// ============================================================================

static DistributedHybridSearch::Config defaultConfig() {
    DistributedHybridSearch::Config cfg;
    cfg.k                    = 10;
    cfg.rrf_k                = 60.0;
    cfg.shard_timeout_ms     = 5000;
    cfg.max_concurrent_shards = 4;
    cfg.skip_failed_shards   = true;
    cfg.local_shard_id       = "shard_local";
    return cfg;
}

static HybridSearch::Result makeResult(const std::string& id,
                                        double hybrid = 0.0,
                                        double bm25   = 0.0,
                                        double vec    = 0.0,
                                        int bm25_rank = -1,
                                        int vec_rank  = -1,
                                        const std::string& content = "") {
    HybridSearch::Result r;
    r.document_id  = id;
    r.hybrid_score = hybrid;
    r.bm25_score   = bm25;
    r.vector_score = vec;
    r.bm25_rank    = bm25_rank;
    r.vector_rank  = vec_rank;
    r.content      = content;
    return r;
}

static DistributedHybridSearch::ShardSearchResult makeShardResult(
    const std::string& shard_id,
    bool success,
    std::vector<HybridSearch::Result> results = {},
    const std::string& error_msg = ""
) {
    DistributedHybridSearch::ShardSearchResult sr;
    sr.shard_id    = shard_id;
    sr.success     = success;
    sr.results     = std::move(results);
    sr.error_msg   = error_msg;
    sr.execution_time_ms = 1;
    return sr;
}

// ============================================================================
// Config validation
// ============================================================================

TEST(DistributedHybridSearchConfig, DefaultConfigIsValid) {
    EXPECT_NO_THROW(DistributedHybridSearch(nullptr, nullptr, nullptr,
                                             defaultConfig()));
}

TEST(DistributedHybridSearchConfig, ZeroKThrows) {
    auto cfg = defaultConfig();
    cfg.k = 0;
    EXPECT_THROW(DistributedHybridSearch(nullptr, nullptr, nullptr, cfg),
                 std::invalid_argument);
}

TEST(DistributedHybridSearchConfig, ZeroRrfKThrows) {
    auto cfg = defaultConfig();
    cfg.rrf_k = 0.0;
    EXPECT_THROW(DistributedHybridSearch(nullptr, nullptr, nullptr, cfg),
                 std::invalid_argument);
}

TEST(DistributedHybridSearchConfig, NegativeRrfKThrows) {
    auto cfg = defaultConfig();
    cfg.rrf_k = -1.0;
    EXPECT_THROW(DistributedHybridSearch(nullptr, nullptr, nullptr, cfg),
                 std::invalid_argument);
}

TEST(DistributedHybridSearchConfig, ZeroMaxConcurrentShardsThrows) {
    auto cfg = defaultConfig();
    cfg.max_concurrent_shards = 0;
    EXPECT_THROW(DistributedHybridSearch(nullptr, nullptr, nullptr, cfg),
                 std::invalid_argument);
}

TEST(DistributedHybridSearchConfig, EmptyEndpointThrows) {
    auto cfg = defaultConfig();
    cfg.search_endpoint = "";
    EXPECT_THROW(DistributedHybridSearch(nullptr, nullptr, nullptr, cfg),
                 std::invalid_argument);
}

TEST(DistributedHybridSearchConfig, GetSetConfig) {
    auto cfg = defaultConfig();
    DistributedHybridSearch dhs(nullptr, nullptr, nullptr, cfg);
    EXPECT_EQ(dhs.getConfig().k, 10u);

    cfg.k = 20;
    dhs.setConfig(cfg);
    EXPECT_EQ(dhs.getConfig().k, 20u);
}

// ============================================================================
// mergeShardResults: empty inputs
// ============================================================================

class DistributedHybridSearchMergeTest : public ::testing::Test {
protected:
    DistributedHybridSearch* dhs_{nullptr};

    void SetUp() override {
        dhs_ = new DistributedHybridSearch(nullptr, nullptr, nullptr,
                                            defaultConfig());
    }
    void TearDown() override {
        delete dhs_;
    }
};

TEST_F(DistributedHybridSearchMergeTest, EmptyShardListReturnsEmpty) {
    auto merged = dhs_->mergeShardResults({});
    EXPECT_TRUE(merged.empty());
}

TEST_F(DistributedHybridSearchMergeTest, AllFailedShardsReturnsEmpty) {
    std::vector<DistributedHybridSearch::ShardSearchResult> shard_results = {
        makeShardResult("s1", false, {}, "timeout"),
        makeShardResult("s2", false, {}, "connection refused"),
    };
    auto merged = dhs_->mergeShardResults(shard_results);
    EXPECT_TRUE(merged.empty());
}

TEST_F(DistributedHybridSearchMergeTest, SingleShardResultPassThrough) {
    std::vector<HybridSearch::Result> results = {
        makeResult("doc1", 0.9),
        makeResult("doc2", 0.7),
        makeResult("doc3", 0.5),
    };
    std::vector<DistributedHybridSearch::ShardSearchResult> shard_results = {
        makeShardResult("s1", true, results),
    };

    auto merged = dhs_->mergeShardResults(shard_results);
    EXPECT_EQ(merged.size(), 3u);
    EXPECT_EQ(merged[0].document_id, "doc1");
    EXPECT_EQ(merged[1].document_id, "doc2");
    EXPECT_EQ(merged[2].document_id, "doc3");
}

// ============================================================================
// mergeShardResults: RRF ranking correctness
// ============================================================================

TEST_F(DistributedHybridSearchMergeTest, DocumentInMultipleShardsRanksHigher) {
    // "overlap_doc" appears rank-1 in both shards → its RRF score is 2x
    // "unique_doc_s1" only in shard 1 at rank 2
    // "unique_doc_s2" only in shard 2 at rank 2

    std::vector<DistributedHybridSearch::ShardSearchResult> shard_results = {
        makeShardResult("s1", true, {
            makeResult("overlap_doc", 0.9),  // rank 1 in s1
            makeResult("unique_doc_s1", 0.8),// rank 2 in s1
        }),
        makeShardResult("s2", true, {
            makeResult("overlap_doc", 0.85), // rank 1 in s2
            makeResult("unique_doc_s2", 0.7),// rank 2 in s2
        }),
    };

    auto merged = dhs_->mergeShardResults(shard_results);

    ASSERT_GE(merged.size(), 1u);
    // overlap_doc should rank first because it gets RRF contributions from both shards
    EXPECT_EQ(merged[0].document_id, "overlap_doc");
}

TEST_F(DistributedHybridSearchMergeTest, RRFScoresDecreaseWithRank) {
    // Higher rank (lower number) → higher RRF score contribution
    std::vector<DistributedHybridSearch::ShardSearchResult> shard_results = {
        makeShardResult("s1", true, {
            makeResult("rank1_doc"),
            makeResult("rank2_doc"),
            makeResult("rank3_doc"),
        }),
    };

    auto merged = dhs_->mergeShardResults(shard_results);

    ASSERT_EQ(merged.size(), 3u);
    EXPECT_EQ(merged[0].document_id, "rank1_doc");
    EXPECT_EQ(merged[1].document_id, "rank2_doc");
    EXPECT_EQ(merged[2].document_id, "rank3_doc");
    // Verify monotonically decreasing hybrid scores
    EXPECT_GT(merged[0].hybrid_score, merged[1].hybrid_score);
    EXPECT_GT(merged[1].hybrid_score, merged[2].hybrid_score);
}

TEST_F(DistributedHybridSearchMergeTest, ResultsLimitedToK) {
    auto cfg = defaultConfig();
    cfg.k = 3;
    DistributedHybridSearch dhs(nullptr, nullptr, nullptr, cfg);

    std::vector<HybridSearch::Result> results = {};

    for (int i = 0; i < 10; ++i) {
        results.push_back(makeResult("doc" + std::to_string(i), 1.0 - i * 0.05));
    }
    auto merged = dhs.mergeShardResults({makeShardResult("s1", true, results)});
    EXPECT_EQ(merged.size(), 3u);
}

TEST_F(DistributedHybridSearchMergeTest, BestScoresPreservedAcrossShards) {
    // doc1 appears in both shards with different bm25/vector scores
    // The merge should keep the best (highest) individual scores seen
    std::vector<DistributedHybridSearch::ShardSearchResult> shard_results = {
        makeShardResult("s1", true, {
            makeResult("doc1", 0.0, /*bm25=*/0.8, /*vec=*/0.2, 1, -1),
        }),
        makeShardResult("s2", true, {
            makeResult("doc1", 0.0, /*bm25=*/0.3, /*vec=*/0.9, -1, 1),
        }),
    };

    auto merged = dhs_->mergeShardResults(shard_results);

    ASSERT_EQ(merged.size(), 1u);
    EXPECT_DOUBLE_EQ(merged[0].bm25_score, 0.8);    // best from s1
    EXPECT_DOUBLE_EQ(merged[0].vector_score, 0.9);  // best from s2
}

TEST_F(DistributedHybridSearchMergeTest, ContentPopulatedFromFirstAvailableShard) {
    std::vector<DistributedHybridSearch::ShardSearchResult> shard_results = {
        makeShardResult("s1", true, {
            makeResult("doc1", 0.9, 0.0, 0.0, -1, -1, "hello from shard 1"),
        }),
        makeShardResult("s2", true, {
            makeResult("doc1", 0.8, 0.0, 0.0, -1, -1, "hello from shard 2"),
        }),
    };

    auto merged = dhs_->mergeShardResults(shard_results);

    ASSERT_EQ(merged.size(), 1u);
    // content should be non-empty (from whichever shard returned it first)
    EXPECT_FALSE(merged[0].content.empty());
}

TEST_F(DistributedHybridSearchMergeTest, FailedShardResultsSkipped) {
    std::vector<DistributedHybridSearch::ShardSearchResult> shard_results = {
        makeShardResult("s1", true,  { makeResult("doc1", 0.9) }),
        makeShardResult("s2", false, {}, "timeout"),
        makeShardResult("s3", true,  { makeResult("doc2", 0.8) }),
    };

    auto merged = dhs_->mergeShardResults(shard_results);

    // Both doc1 and doc2 should appear; s2's (empty) failure is skipped
    ASSERT_EQ(merged.size(), 2u);
    // doc1 should rank first (rank 1 in s1)
    EXPECT_EQ(merged[0].document_id, "doc1");
    EXPECT_EQ(merged[1].document_id, "doc2");
}

TEST_F(DistributedHybridSearchMergeTest, NoSkip_FailedShardResultsIncluded) {
    // When skip_failed_shards=false, failed shards still produce no results
    // (their results list is empty), so the merge just processes the success ones.
    // The setting only matters in search() where it can short-circuit.
    auto cfg = defaultConfig();
    cfg.skip_failed_shards = false;
    DistributedHybridSearch dhs(nullptr, nullptr, nullptr, cfg);

    std::vector<DistributedHybridSearch::ShardSearchResult> shard_results = {
        makeShardResult("s1", true,  { makeResult("doc1", 0.9) }),
        makeShardResult("s2", false, {}, "timeout"),
    };

    // With skip_failed_shards=false, failed shards are NOT skipped in merge
    // so we get doc1 from s1 and nothing from s2 (its result list is empty)
    auto merged = dhs.mergeShardResults(shard_results);
    // doc1 comes from s1 (success); s2 has no results even if not skipped
    EXPECT_EQ(merged.size(), 1u);
    EXPECT_EQ(merged[0].document_id, "doc1");
}

TEST_F(DistributedHybridSearchMergeTest, EmptyDocumentIdSkipped) {
    // Results with empty document_id should be ignored
    HybridSearch::Result bad_result;
    bad_result.document_id = "";
    bad_result.hybrid_score = 0.99;

    std::vector<DistributedHybridSearch::ShardSearchResult> shard_results = {
        makeShardResult("s1", true, {
            bad_result,
            makeResult("good_doc", 0.5),
        }),
    };

    auto merged = dhs_->mergeShardResults(shard_results);
    ASSERT_EQ(merged.size(), 1u);
    EXPECT_EQ(merged[0].document_id, "good_doc");
}

// ============================================================================
// mergeShardResults: multi-shard deduplication
// ============================================================================

TEST_F(DistributedHybridSearchMergeTest, DeduplicatesAcrossShards) {
    // Same document appearing in 3 shards should be deduplicated to 1 result
    std::vector<DistributedHybridSearch::ShardSearchResult> shard_results = {
        makeShardResult("s1", true, { makeResult("shared_doc", 0.9) }),
        makeShardResult("s2", true, { makeResult("shared_doc", 0.8) }),
        makeShardResult("s3", true, { makeResult("shared_doc", 0.7) }),
    };

    auto merged = dhs_->mergeShardResults(shard_results);
    EXPECT_EQ(merged.size(), 1u);
    EXPECT_EQ(merged[0].document_id, "shared_doc");
}

TEST_F(DistributedHybridSearchMergeTest, ThreeShardOverlapRanksAboveTwoShardOverlap) {
    // "triple_doc" is rank-1 in all 3 shards
    // "double_doc" is rank-1 in only 2 shards
    // "triple_doc" should have a higher merged RRF score
    std::vector<DistributedHybridSearch::ShardSearchResult> shard_results = {
        makeShardResult("s1", true, {
            makeResult("triple_doc"),
            makeResult("double_doc"),
        }),
        makeShardResult("s2", true, {
            makeResult("triple_doc"),
            makeResult("double_doc"),
        }),
        makeShardResult("s3", true, {
            makeResult("triple_doc"),
            makeResult("only_s3_doc"),
        }),
    };

    auto merged = dhs_->mergeShardResults(shard_results);
    ASSERT_GE(merged.size(), 2u);
    EXPECT_EQ(merged[0].document_id, "triple_doc");
}

// ============================================================================
// search(): null-safe execution (no network calls)
// ============================================================================

TEST(DistributedHybridSearchSearch, NullLocalSearchReturnsEmpty) {
    DistributedHybridSearch dhs(nullptr, nullptr, nullptr, defaultConfig());

    DistributedHybridSearch::SearchStats stats;
    auto results = dhs.search("machine learning", {}, &stats);

    // No local search, no resolver → empty
    EXPECT_TRUE(results.empty());
    EXPECT_EQ(stats.shards_queried, 1u);    // local "shard" is always counted
    EXPECT_EQ(stats.shards_succeeded, 1u);  // null local_search = silent success
    EXPECT_EQ(stats.shards_failed, 0u);
    EXPECT_FALSE(stats.partial_result);
}

TEST(DistributedHybridSearchSearch, NoResolverOnlyRunsLocal) {
    // With null resolver, only the local shard is queried (and local_search is
    // null so results are empty)
    DistributedHybridSearch dhs(nullptr, nullptr, nullptr, defaultConfig());

    DistributedHybridSearch::SearchStats stats;
    dhs.search("test query", {}, &stats);

    EXPECT_EQ(stats.shards_queried, 1u);
}

TEST(DistributedHybridSearchSearch, EmptyQueryRunsSafely) {
    DistributedHybridSearch dhs(nullptr, nullptr, nullptr, defaultConfig());

    // Must not throw
    EXPECT_NO_THROW(dhs.search(""));
    EXPECT_NO_THROW(dhs.search("", {}));
}

TEST(DistributedHybridSearchSearch, StatsReflectNoFailures) {
    DistributedHybridSearch dhs(nullptr, nullptr, nullptr, defaultConfig());

    DistributedHybridSearch::SearchStats stats;
    dhs.search("test", {}, &stats);

    EXPECT_GE(stats.shards_queried,   1u);
    EXPECT_GE(stats.shards_succeeded, 1u);
    EXPECT_EQ(stats.shards_failed, 0u);
}

// ============================================================================
// Fault tolerance: skip_failed_shards flag
// ============================================================================

TEST(DistributedHybridSearchFaultTolerance, SkipFailedShardsDefaultTrue) {
    auto cfg = defaultConfig();
    EXPECT_TRUE(cfg.skip_failed_shards);
}

TEST(DistributedHybridSearchFaultTolerance, MergeWithAllFailedSkipped) {
    auto cfg = defaultConfig();
    cfg.skip_failed_shards = true;
    DistributedHybridSearch dhs(nullptr, nullptr, nullptr, cfg);

    std::vector<DistributedHybridSearch::ShardSearchResult> shard_results = {
        makeShardResult("s1", false, {}, "timeout"),
        makeShardResult("s2", false, {}, "connection refused"),
    };

    auto merged = dhs.mergeShardResults(shard_results);
    EXPECT_TRUE(merged.empty());
}

TEST(DistributedHybridSearchFaultTolerance, PartialFailureStillReturnsResults) {
    auto cfg = defaultConfig();
    cfg.skip_failed_shards = true;
    DistributedHybridSearch dhs(nullptr, nullptr, nullptr, cfg);

    std::vector<DistributedHybridSearch::ShardSearchResult> shard_results = {
        makeShardResult("s1", true, {
            makeResult("doc_a", 0.9),
            makeResult("doc_b", 0.7),
        }),
        makeShardResult("s2", false, {}, "network error"),
        makeShardResult("s3", true, {
            makeResult("doc_c", 0.85),
        }),
    };

    auto merged = dhs.mergeShardResults(shard_results);
    EXPECT_EQ(merged.size(), 3u);  // doc_a, doc_b, doc_c
}

// ============================================================================
// RRF constant (rrf_k) effect on scoring
// ============================================================================

TEST(DistributedHybridSearchRRF, LargerRrfKFlattensScores) {
    // With a large rrf_k the score differences between ranks are smaller
    auto cfg_small = defaultConfig();
    cfg_small.rrf_k = 1.0;
    DistributedHybridSearch dhs_small(nullptr, nullptr, nullptr, cfg_small);

    auto cfg_large = defaultConfig();
    cfg_large.rrf_k = 1000.0;
    DistributedHybridSearch dhs_large(nullptr, nullptr, nullptr, cfg_large);

    std::vector<DistributedHybridSearch::ShardSearchResult> shard_results = {
        makeShardResult("s1", true, {
            makeResult("doc1"),
            makeResult("doc2"),
        }),
    };

    auto merged_small = dhs_small.mergeShardResults(shard_results);
    auto merged_large = dhs_large.mergeShardResults(shard_results);

    ASSERT_EQ(merged_small.size(), 2u);
    ASSERT_EQ(merged_large.size(), 2u);

    double diff_small = merged_small[0].hybrid_score - merged_small[1].hybrid_score;
    double diff_large = merged_large[0].hybrid_score - merged_large[1].hybrid_score;

    // Small rrf_k → bigger score spread
    EXPECT_GT(diff_small, diff_large);
}

// ============================================================================
// parseShardResponse (tested via mergeShardResults with pre-built results)
// ============================================================================

TEST(DistributedHybridSearchParse, JsonResponseRoundTripViaShardResult) {
    // Build a ShardSearchResult manually (as if parseShardResponse produced it)
    DistributedHybridSearch::ShardSearchResult sr;
    sr.shard_id = "s1";
    sr.success  = true;
    sr.results  = {
        makeResult("doc_json", 0.88, 0.75, 0.95, 1, 2, "sample content"),
    };

    auto cfg = defaultConfig();
    DistributedHybridSearch dhs(nullptr, nullptr, nullptr, cfg);
    auto merged = dhs.mergeShardResults({sr});

    ASSERT_EQ(merged.size(), 1u);
    EXPECT_EQ(merged[0].document_id, "doc_json");
    EXPECT_DOUBLE_EQ(merged[0].bm25_score,   0.75);
    EXPECT_DOUBLE_EQ(merged[0].vector_score, 0.95);
    EXPECT_EQ(merged[0].content, "sample content");
}

// ============================================================================
// parseShardResponse: direct JSON deserialization tests
// ============================================================================

TEST(DistributedHybridSearchParseShardResponse, EmptyArrayReturnsEmpty) {
    auto results = DistributedHybridSearch::parseShardResponse(
        nlohmann::json::array());
    EXPECT_TRUE(results.empty());
}

TEST(DistributedHybridSearchParseShardResponse, WrappedResultsField) {
    nlohmann::json payload = {
        {"results", nlohmann::json::array({
            {{"document_id", "doc1"}, {"bm25_score", 0.8}, {"hybrid_score", 0.9}}
        })}
    };
    auto results = DistributedHybridSearch::parseShardResponse(payload);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].document_id, "doc1");
    EXPECT_DOUBLE_EQ(results[0].bm25_score, 0.8);
    EXPECT_DOUBLE_EQ(results[0].hybrid_score, 0.9);
}

TEST(DistributedHybridSearchParseShardResponse, DirectArrayFormat) {
    nlohmann::json payload = nlohmann::json::array({
        {{"document_id", "doc_a"}, {"vector_score", 0.7}, {"bm25_rank", 2}},
        {{"document_id", "doc_b"}, {"vector_score", 0.5}, {"vector_rank", 1}},
    });
    auto results = DistributedHybridSearch::parseShardResponse(payload);
    ASSERT_EQ(results.size(), 2u);
    EXPECT_EQ(results[0].document_id, "doc_a");
    EXPECT_DOUBLE_EQ(results[0].vector_score, 0.7);
    EXPECT_EQ(results[0].bm25_rank, 2);
    EXPECT_EQ(results[1].document_id, "doc_b");
    EXPECT_EQ(results[1].vector_rank, 1);
}

TEST(DistributedHybridSearchParseShardResponse, AllFieldsPopulated) {
    nlohmann::json payload = nlohmann::json::array({
        {
            {"document_id",  "full_doc"},
            {"bm25_score",   0.75},
            {"vector_score", 0.92},
            {"hybrid_score", 0.88},
            {"bm25_rank",    1},
            {"vector_rank",  2},
            {"content",      "some text content"},
        }
    });
    auto results = DistributedHybridSearch::parseShardResponse(payload);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].document_id, "full_doc");
    EXPECT_DOUBLE_EQ(results[0].bm25_score,   0.75);
    EXPECT_DOUBLE_EQ(results[0].vector_score, 0.92);
    EXPECT_DOUBLE_EQ(results[0].hybrid_score, 0.88);
    EXPECT_EQ(results[0].bm25_rank,   1);
    EXPECT_EQ(results[0].vector_rank, 2);
    EXPECT_EQ(results[0].content, "some text content");
}

TEST(DistributedHybridSearchParseShardResponse, MissingFieldsDefaultToZero) {
    // Only document_id present; all score fields should default to 0
    nlohmann::json payload = nlohmann::json::array({
        {{"document_id", "sparse_doc"}}
    });
    auto results = DistributedHybridSearch::parseShardResponse(payload);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].document_id, "sparse_doc");
    EXPECT_DOUBLE_EQ(results[0].bm25_score,   0.0);
    EXPECT_DOUBLE_EQ(results[0].vector_score, 0.0);
    EXPECT_DOUBLE_EQ(results[0].hybrid_score, 0.0);
    EXPECT_EQ(results[0].bm25_rank,   -1);  // default int value from HybridSearch::Result
    EXPECT_TRUE(results[0].content.empty());
}

TEST(DistributedHybridSearchParseShardResponse, EmptyDocumentIdSkipped) {
    nlohmann::json payload = nlohmann::json::array({
        {{"document_id", ""}, {"hybrid_score", 0.99}},  // empty id -> skip
        {{"document_id", "valid_doc"}, {"hybrid_score", 0.5}},
    });
    auto results = DistributedHybridSearch::parseShardResponse(payload);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].document_id, "valid_doc");
}

TEST(DistributedHybridSearchParseShardResponse, NonObjectItemsSkipped) {
    nlohmann::json payload = nlohmann::json::array({
        "not an object",
        42,
        nullptr,
        {{"document_id", "ok_doc"}, {"hybrid_score", 0.7}},
    });
    auto results = DistributedHybridSearch::parseShardResponse(payload);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].document_id, "ok_doc");
}

TEST(DistributedHybridSearchParseShardResponse, UnrecognizedFormatReturnsEmpty) {
    // Not an array and not an object with "results" key
    nlohmann::json payload = {{"foo", "bar"}};
    auto results = DistributedHybridSearch::parseShardResponse(payload);
    EXPECT_TRUE(results.empty());
}

TEST(DistributedHybridSearchParseShardResponse, NullJsonReturnsEmpty) {
    auto results = DistributedHybridSearch::parseShardResponse(nullptr);
    EXPECT_TRUE(results.empty());
}

TEST(DistributedHybridSearchParseShardResponse, MultipleResultsPreserveOrder) {
    nlohmann::json payload = nlohmann::json::array({
        {{"document_id", "first"},  {"hybrid_score", 0.9}},
        {{"document_id", "second"}, {"hybrid_score", 0.7}},
        {{"document_id", "third"},  {"hybrid_score", 0.5}},
    });
    auto results = DistributedHybridSearch::parseShardResponse(payload);
    ASSERT_EQ(results.size(), 3u);
    EXPECT_EQ(results[0].document_id, "first");
    EXPECT_EQ(results[1].document_id, "second");
    EXPECT_EQ(results[2].document_id, "third");
}
