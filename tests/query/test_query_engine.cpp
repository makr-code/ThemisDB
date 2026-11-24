// QueryEngine index-backed equality tests

#include <gtest/gtest.h>
#include <algorithm>
#include <filesystem>
#include <chrono>
#include <string>
#include <utility>
#include <vector>

#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "index/secondary_index.h"
#include "query/query_engine.h"
#include "query/query_optimizer.h"

using namespace themis;
using namespace themis::query;

static std::string tmpPath(const std::string& name) {
    namespace fs = std::filesystem;
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() / (name + std::to_string(now))).string();
}

TEST(QueryEngineTest, AndQuery_UsesSecondaryIndexes) {
    RocksDBWrapper::Config cfg; cfg.db_path = tmpPath("vccdb_qe_"); cfg.enable_blobdb=false;
    RocksDBWrapper db(cfg); ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);
    ASSERT_TRUE(idx.createIndex("users","age").ok);
    ASSERT_TRUE(idx.createIndex("users","city").ok);

    // Data
    auto put = [&](const std::string& pk, int age, const std::string& city){
        BaseEntity::FieldMap f{{"name","N"+pk},{"age", int64_t(age)},{"city", city}};
        BaseEntity e = BaseEntity::fromFields(pk, f);
        ASSERT_TRUE(idx.put("users", e).ok);
    };
    put("u1", 30, "Berlin");
    put("u2", 31, "Berlin");
    put("u3", 30, "Munich");

    QueryEngine engine(db, idx);
    ConjunctiveQuery q{"users", {{"age","30"},{"city","Berlin"}}};

    // Parallel execution
    auto resultK = engine.executeAndKeys(q);
    ASSERT_TRUE(resultK);
    auto& keys = *resultK;
    ASSERT_EQ(keys.size(), 1u);
    EXPECT_EQ(keys[0], "u1");

    auto resultE = engine.executeAndEntities(q);
    ASSERT_TRUE(resultE);
    auto& ents = *resultE;
    ASSERT_EQ(ents.size(), 1u);

    auto resultC = engine.executeAndCount(q);
    ASSERT_TRUE(resultC);
    EXPECT_EQ(*resultC, 1u);
    db.close();
}

TEST(QueryEngineTest, OptimizedSequentialOrder) {
    RocksDBWrapper::Config cfg; cfg.db_path = tmpPath("vccdb_qe_opt_");
    RocksDBWrapper db(cfg); ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);
    ASSERT_TRUE(idx.createIndex("users","age").ok);
    ASSERT_TRUE(idx.createIndex("users","city").ok);

    for (int i=0;i<50;++i) {
        BaseEntity::FieldMap f{{"name","N"+std::to_string(i)},{"age", int64_t(30)},{"city","Berlin"}};
        BaseEntity e = BaseEntity::fromFields("u"+std::to_string(i), f);
        ASSERT_TRUE(idx.put("users", e).ok);
    }
    // make a rare predicate
    BaseEntity::FieldMap f{{"name","Rare"},{"age", int64_t(99)},{"city","Berlin"}};
    ASSERT_TRUE(idx.put("users", BaseEntity::fromFields("rare", f)).ok);

    QueryEngine engine(db, idx);
    QueryOptimizer opt(idx);
    ConjunctiveQuery q{"users", {{"age","99"},{"city","Berlin"}}};

    auto plan = opt.chooseOrderForAndQuery(q, 10);
    ASSERT_EQ(plan.orderedPredicates.size(), 2u);
    // age=99 should be first due to low estimate
    EXPECT_EQ(plan.orderedPredicates[0].column, "age");

    auto keys_result = opt.executeOptimizedKeys(engine, q, plan);
    ASSERT_TRUE(keys_result.has_value()) << keys_result.error().message();
    ASSERT_EQ(keys_result->size(), 1u);
    EXPECT_EQ((*keys_result)[0], "rare");

    auto count_result = opt.executeOptimizedCount(engine, q, plan);
    ASSERT_TRUE(count_result.has_value()) << count_result.error().message();
    EXPECT_EQ(*count_result, 1u);
    db.close();
}

TEST(QueryEngineTest, NoIndexReturnsError) {
    RocksDBWrapper::Config cfg; cfg.db_path = tmpPath("vccdb_qe_noidx_");
    RocksDBWrapper db(cfg); ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);
    // Kein Index angelegt
    // Daten schreiben direkt via Indexmanager (pflegt hier keine Indizes da keine Meta-Keys existieren, aber entspr. Relationseintrag wird geschrieben)
    BaseEntity e = BaseEntity::fromFields("u1", {{"age", int64_t(30)}});
    // Um Relationseintrag zu erstellen, nutzen wir put auf Tabelle users (ohne Index keine Index-Keys)
    ASSERT_TRUE(idx.put("users", e).ok);

    QueryEngine engine(db, idx);
    ConjunctiveQuery q{"users", {{"age","30"}}};
    auto resultK = engine.executeAndKeys(q);
    EXPECT_FALSE(resultK);
    auto resultC = engine.executeAndCount(q);
    EXPECT_FALSE(resultC);
    db.close();
}

TEST(QueryEngineTest, NoMatchReturnsEmpty) {
    RocksDBWrapper::Config cfg; cfg.db_path = tmpPath("vccdb_qe_nomatch_");
    RocksDBWrapper db(cfg); ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);
    ASSERT_TRUE(idx.createIndex("users","age").ok);

    BaseEntity e = BaseEntity::fromFields("u1", {{"age", int64_t(30)}});
    ASSERT_TRUE(idx.put("users", e).ok);

    QueryEngine engine(db, idx);
    ConjunctiveQuery q{"users", {{"age","99"}}};
    auto resultK = engine.executeAndKeys(q);
    ASSERT_TRUE(resultK);
    auto& keys = *resultK;
    EXPECT_TRUE(keys.empty());

    auto resultC = engine.executeAndCount(q);
    ASSERT_TRUE(resultC);
    EXPECT_EQ(*resultC, 0u);
    db.close();
}

TEST(QueryEngineTest, RangePredicatesUseExactFilteringForNegativeDecimals) {
    RocksDBWrapper::Config cfg; cfg.db_path = tmpPath("vccdb_qe_range_negative_decimal_");
    RocksDBWrapper db(cfg); ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);
    ASSERT_TRUE(idx.createRangeIndex("measurements", "value").ok);

    for (const auto& [pk, value] : std::vector<std::pair<std::string, std::string>>{
             {"minus-ten", "-10"},
             {"minus-two", "-2"},
             {"minus-one-point-five", "-1.5"},
             {"minus-one", "-1"},
             {"minus-half", "-0.5"}}) {
        auto entity = BaseEntity::fromFields(pk, {{"value", value}});
        ASSERT_TRUE(idx.put("measurements", entity).ok);
    }

    QueryEngine engine(db, idx);
    ConjunctiveQuery q;
    q.table = "measurements";
    q.rangePredicates.push_back({
        "value",
        std::make_optional<std::string>("-2"),
        std::make_optional<std::string>("-1"),
        true,
        true});

    auto result = engine.executeAndKeys(q);
    ASSERT_TRUE(result) << result.error().message();
    std::sort(result->begin(), result->end());
    EXPECT_EQ(*result, (std::vector<std::string>{
        "minus-one", "minus-one-point-five", "minus-two"}));
    db.close();
}

TEST(QueryEngineTest, RangeOrderByUsesExactNumericOrderingAndCursor) {
    RocksDBWrapper::Config cfg; cfg.db_path = tmpPath("vccdb_qe_range_order_");
    RocksDBWrapper db(cfg); ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);
    ASSERT_TRUE(idx.createRangeIndex("measurements", "value").ok);

    for (const auto& [pk, value] : std::vector<std::pair<std::string, std::string>>{
             {"minus-ten", "-10"},
             {"minus-two", "-2"},
             {"minus-one-point-five", "-1.5"},
             {"minus-one", "-1"},
             {"minus-half", "-0.5"},
             {"ordinary-text", "alpha"}}) {
        ASSERT_TRUE(idx.put("measurements", BaseEntity::fromFields(pk, {{"value", value}})).ok);
    }

    QueryEngine engine(db, idx);
    ConjunctiveQuery q;
    q.table = "measurements";
    q.rangePredicates.push_back({
        "value",
        std::make_optional<std::string>("-2"),
        std::make_optional<std::string>("-1"),
        true,
        true});
    q.orderBy = OrderBy{"value", false, 10};

    auto ascending = engine.executeAndKeys(q);
    ASSERT_TRUE(ascending) << ascending.error().message();
    EXPECT_EQ(*ascending, (std::vector<std::string>{
        "minus-two", "minus-one-point-five", "minus-one"}));

    q.orderBy->desc = true;
    auto descending = engine.executeAndKeys(q);
    ASSERT_TRUE(descending) << descending.error().message();
    EXPECT_EQ(*descending, (std::vector<std::string>{
        "minus-one", "minus-one-point-five", "minus-two"}));

    q.orderBy->desc = false;
    q.orderBy->cursor_value = "-1.5";
    q.orderBy->cursor_pk = "minus-one-point-five";
    auto after_cursor = engine.executeAndKeys(q);
    ASSERT_TRUE(after_cursor) << after_cursor.error().message();
    EXPECT_EQ(*after_cursor, (std::vector<std::string>{"minus-one"}));

    q.rangePredicates[0].lower.reset();
    q.rangePredicates[0].upper.reset();
    q.orderBy->cursor_value.reset();
    q.orderBy->cursor_pk.reset();
    auto mixed_values = engine.executeAndKeys(q);
    ASSERT_TRUE(mixed_values) << mixed_values.error().message();
    EXPECT_EQ(*mixed_values, (std::vector<std::string>{
        "minus-ten", "minus-two", "minus-one-point-five", "minus-one",
        "minus-half", "ordinary-text"}));
    db.close();
}

TEST(QueryEngineTest, RangePredicatesCompareExponentsAndLargeIntegersExactly) {
    RocksDBWrapper::Config cfg; cfg.db_path = tmpPath("vccdb_qe_range_precision_");
    RocksDBWrapper db(cfg); ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);
    ASSERT_TRUE(idx.createRangeIndex("measurements", "value").ok);

    for (const auto& [pk, value] : std::vector<std::pair<std::string, std::string>>{
             {"exponent-low", "9.99e2"},
             {"exponent-middle", "1e3"},
             {"exponent-high", "1.001e3"},
             {"integer-low", "9007199254740992"},
             {"integer-high", "9007199254740993"}}) {
        ASSERT_TRUE(idx.put("measurements", BaseEntity::fromFields(pk, {{"value", value}})).ok);
    }
    ASSERT_TRUE(idx.put(
        "measurements", BaseEntity::fromFields("double-small", {{"value", 1e-7}})).ok);

    QueryEngine engine(db, idx);
    ConjunctiveQuery exponent_query;
    exponent_query.table = "measurements";
    exponent_query.rangePredicates.push_back({
        "value",
        std::make_optional<std::string>("999"),
        std::make_optional<std::string>("1001"),
        true,
        true});
    auto exponent_result = engine.executeAndKeys(exponent_query);
    ASSERT_TRUE(exponent_result) << exponent_result.error().message();
    std::sort(exponent_result->begin(), exponent_result->end());
    EXPECT_EQ(*exponent_result, (std::vector<std::string>{
        "exponent-high", "exponent-low", "exponent-middle"}));

    exponent_query.rangePredicates[0].lower = "9007199254740992";
    exponent_query.rangePredicates[0].upper = "9007199254740993";
    auto integer_result = engine.executeAndKeys(exponent_query);
    ASSERT_TRUE(integer_result) << integer_result.error().message();
    std::sort(integer_result->begin(), integer_result->end());
    EXPECT_EQ(*integer_result, (std::vector<std::string>{
        "integer-high", "integer-low"}));

    exponent_query.rangePredicates[0].lower = "1e-8";
    exponent_query.rangePredicates[0].upper = "1e-6";
    auto double_result = engine.executeAndKeys(exponent_query);
    ASSERT_TRUE(double_result) << double_result.error().message();
    EXPECT_EQ(*double_result, (std::vector<std::string>{"double-small"}));
    db.close();
}

TEST(QueryEngineTest, NoIndexWithFallbackReturnsKeys) {
    RocksDBWrapper::Config cfg; cfg.db_path = tmpPath("vccdb_qe_fallback_");
    RocksDBWrapper db(cfg); ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);
    // Kein Index
    BaseEntity e = BaseEntity::fromFields("u1", {{"age", int64_t(30)}, {"city","Berlin"}});
    ASSERT_TRUE(idx.put("users", e).ok);

    QueryEngine engine(db, idx);
    ConjunctiveQuery q{"users", {{"age","30"}, {"city","Berlin"}}};
    auto result = engine.executeAndKeysWithFallback(q, true);
    ASSERT_TRUE(result.has_value());
    auto keys = *result;
    ASSERT_EQ(keys.size(), 1u);
    EXPECT_EQ(keys[0], "u1");
    db.close();
}
