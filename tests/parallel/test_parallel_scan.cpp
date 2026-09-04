// Parallel full-table scan tests
//
// Verifies that QueryEngine::executeAndKeysWithFallback (and the
// underlying fullScanAndFilter_ helper) produces correct results both
// on small collections (sequential path) and on collections that exceed
// the ParallelScanConfig::parallel_threshold (parallel morsel path).

#include <gtest/gtest.h>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "index/secondary_index.h"
#include "query/query_engine.h"
#include "query/parallel_scan.h"

using namespace themis;
using namespace themis::query;

namespace {

std::string tmpPath(std::string_view prefix) {
    namespace fs = std::filesystem;
    auto ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() / (std::string(prefix) + std::to_string(ts))).string();
}

// Insert `count` entities into table "items" with field "value" = str(i).
// Returns the set of PKs whose "value" equals `match_value`.
std::vector<std::string> insertItems(SecondaryIndexManager& idx,
                                     size_t count,
                                     const std::string& match_value,
                                     size_t num_matching) {
    std::vector<std::string> expected_pks;
    for (size_t i = 0; i < count; ++i) {
        std::string pk = "item" + std::to_string(i);
        std::string val = (i < num_matching) ? match_value : "other_" + std::to_string(i);
        BaseEntity::FieldMap f{{"value", val}};
        BaseEntity e = BaseEntity::fromFields(pk, f);
        EXPECT_TRUE(idx.put("items", e).ok);
        if (i < num_matching) {
          expected_pks.push_back(pk);
        }
    }
    return expected_pks;
}

} // namespace

// ── ParallelScanConfig ──────────────────────────────────────────────────────

TEST(ParallelScanConfigTest, DefaultValues) {
    ParallelScanConfig cfg;
    EXPECT_GT(cfg.parallel_threshold, 0u);
    EXPECT_GT(cfg.morsel_size, 0u);
}

// ── Sequential path (small collection, below threshold) ────────────────────

TEST(ParallelScanTest, SmallCollection_SequentialPath_NoPredicates) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = tmpPath("ps_small_nopred_");
    cfg.enable_blobdb = false;
    RocksDBWrapper db(cfg);
    ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);

    const size_t N = 10; // well below threshold
    for (size_t i = 0; i < N; ++i) {
        std::string pk = "e" + std::to_string(i);
        BaseEntity e = BaseEntity::fromFields(pk, {{"x", std::to_string(i)}});
        ASSERT_TRUE(idx.put("t", e).ok);
    }

    QueryEngine engine(db, idx);
    ConjunctiveQuery q{"t", {}, {}}; // no predicates → full scan
    auto result = engine.executeAndKeysWithFallback(q);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    EXPECT_EQ(result->size(), N);
    db.close();
}

TEST(ParallelScanTest, SmallCollection_SequentialPath_EqualityPredicate) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = tmpPath("ps_small_eq_");
    cfg.enable_blobdb = false;
    RocksDBWrapper db(cfg);
    ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);

    const size_t N = 20; // below threshold
    const size_t NUM_MATCHING = 3;
    auto expected = insertItems(idx, N, "target", NUM_MATCHING);

    QueryEngine engine(db, idx);
    ConjunctiveQuery q{"items", {{"value", "target"}}, {}};
    auto result = engine.executeAndKeysWithFallback(q);
    ASSERT_TRUE(result.has_value()) << result.error().message();

    auto keys = *result;
    std::sort(keys.begin(), keys.end());
    std::sort(expected.begin(), expected.end());
    EXPECT_EQ(keys, expected);
    db.close();
}

// ── Parallel path (large collection, at/above threshold) ───────────────────

TEST(ParallelScanTest, LargeCollection_ParallelPath_NoPredicates) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = tmpPath("ps_large_nopred_");
    cfg.enable_blobdb = false;
    RocksDBWrapper db(cfg);
    ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);

    // Insert just enough rows to exceed the default parallel_threshold.
    ParallelScanConfig scan_cfg;
    const size_t N = scan_cfg.parallel_threshold + scan_cfg.morsel_size;
    for (size_t i = 0; i < N; ++i) {
        std::string pk = "r" + std::to_string(i);
        BaseEntity e = BaseEntity::fromFields(pk, {{"v", std::to_string(i)}});
        ASSERT_TRUE(idx.put("big", e).ok);
    }

    QueryEngine engine(db, idx);
    ConjunctiveQuery q{"big", {}, {}}; // no predicates → full scan
    auto result = engine.executeAndKeysWithFallback(q);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    EXPECT_EQ(result->size(), N);
    db.close();
}

TEST(ParallelScanTest, LargeCollection_ParallelPath_EqualityPredicate) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = tmpPath("ps_large_eq_");
    cfg.enable_blobdb = false;
    RocksDBWrapper db(cfg);
    ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);

    ParallelScanConfig scan_cfg;
    const size_t N = scan_cfg.parallel_threshold + scan_cfg.morsel_size * 2;
    const size_t NUM_MATCHING = 42;
    auto expected = insertItems(idx, N, "match", NUM_MATCHING);

    QueryEngine engine(db, idx);
    ConjunctiveQuery q{"items", {{"value", "match"}}, {}};
    auto result = engine.executeAndKeysWithFallback(q);
    ASSERT_TRUE(result.has_value()) << result.error().message();

    auto keys = *result;
    std::sort(keys.begin(), keys.end());
    std::sort(expected.begin(), expected.end());
    EXPECT_EQ(keys, expected);
    db.close();
}

// ── Correctness: parallel and sequential results must agree ────────────────

TEST(ParallelScanTest, ParallelAndSequentialResultsAgree) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = tmpPath("ps_agree_");
    cfg.enable_blobdb = false;
    RocksDBWrapper db(cfg);
    ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);

    // Use a count above threshold so fullScanAndFilter_ picks the parallel path.
    ParallelScanConfig scan_cfg;
    const size_t N = scan_cfg.parallel_threshold + 100;
    const size_t NUM_MATCHING = 50;

    std::vector<std::string> expected;
    for (size_t i = 0; i < N; ++i) {
        std::string pk = "k" + std::to_string(i);
        std::string val = (i < NUM_MATCHING) ? "yes" : "no";
        BaseEntity e = BaseEntity::fromFields(pk, {{"flag", val}});
        ASSERT_TRUE(idx.put("data", e).ok);
        if (i < NUM_MATCHING) {
          expected.push_back(pk);
        }
    }

    QueryEngine engine(db, idx);
    ConjunctiveQuery q{"data", {{"flag", "yes"}}, {}};
    auto result = engine.executeAndKeysWithFallback(q);
    ASSERT_TRUE(result.has_value()) << result.error().message();

    auto keys = *result;
    std::sort(keys.begin(), keys.end());
    std::sort(expected.begin(), expected.end());
    EXPECT_EQ(keys, expected);
    EXPECT_EQ(keys.size(), NUM_MATCHING);
    db.close();
}

// ── Range predicates work correctly through the parallel path ──────────────

TEST(ParallelScanTest, LargeCollection_RangePredicate) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = tmpPath("ps_range_");
    cfg.enable_blobdb = false;
    RocksDBWrapper db(cfg);
    ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);

    ParallelScanConfig scan_cfg;
    const size_t N = scan_cfg.parallel_threshold + scan_cfg.morsel_size;
    for (size_t i = 0; i < N; ++i) {
        std::string pk = "n" + std::to_string(i);
        BaseEntity e = BaseEntity::fromFields(pk, {{"score", int64_t(i)}});
        ASSERT_TRUE(idx.put("scores", e).ok);
    }

    // Select rows with score in [100, 200)
    QueryEngine engine(db, idx);
    PredicateRange range;
    range.column = "score";
    range.lower  = "100";
    range.upper  = "200";
    range.includeLower = true;
    range.includeUpper = false;
    ConjunctiveQuery q{"scores", {}, {range}};

    auto result = engine.executeAndKeysWithFallback(q);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    EXPECT_EQ(result->size(), 100u); // [100,200) → exactly 100 rows
    db.close();
}

// ── Empty table returns empty result ───────────────────────────────────────

TEST(ParallelScanTest, EmptyTable_ReturnsEmpty) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = tmpPath("ps_empty_");
    cfg.enable_blobdb = false;
    RocksDBWrapper db(cfg);
    ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);

    QueryEngine engine(db, idx);
    ConjunctiveQuery q{"empty_table", {}, {}};
    auto result = engine.executeAndKeysWithFallback(q);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    EXPECT_TRUE(result->empty());
    db.close();
}
