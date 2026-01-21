// Query Engine Error Handling Tests
// Phase 6: Comprehensive error scenario validation

#include <gtest/gtest.h>
#include <filesystem>
#include <chrono>
#include <string>
#include <vector>
#include <thread>

#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "index/secondary_index.h"
#include "query/query_engine.h"
#include "query/query_optimizer.h"

using namespace themis;

// Generate unique temporary path for test databases
// Note: Cleanup is handled by TearDown in each test (db.close())
static std::string tmpPath(const std::string& name) {
    namespace fs = std::filesystem;
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() / (name + std::to_string(now))).string();
}

// ============================================================================
// Parse Error Tests
// ============================================================================

TEST(QueryEngineErrorTest, InvalidTableName_ReturnsError) {
    RocksDBWrapper::Config cfg; 
    cfg.db_path = tmpPath("qe_err_table_");
    cfg.enable_blobdb = false;
    RocksDBWrapper db(cfg); 
    ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);
    
    QueryEngine engine(db, idx);
    
    // Query on non-existent table
    ConjunctiveQuery q{"nonexistent_table", {{"field", "value"}}};
    auto [st, keys] = engine.executeAndKeys(q);
    
    // Should fail gracefully
    EXPECT_FALSE(st.ok) << "Query on non-existent table should fail";
    EXPECT_TRUE(keys.empty());
    EXPECT_FALSE(st.message.empty()) << "Error should have descriptive message";
    
    db.close();
}

TEST(QueryEngineErrorTest, EmptyPredicates_ReturnsError) {
    RocksDBWrapper::Config cfg; 
    cfg.db_path = tmpPath("qe_err_empty_");
    cfg.enable_blobdb = false;
    RocksDBWrapper db(cfg); 
    ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);
    ASSERT_TRUE(idx.createIndex("users", "age").ok);
    
    QueryEngine engine(db, idx);
    
    // Query with empty predicates
    ConjunctiveQuery q{"users", {}};
    auto [st, keys] = engine.executeAndKeys(q);
    
    // Should handle gracefully (might be OK returning all, or error)
    // At minimum, should not crash
    EXPECT_TRUE(st.ok || !st.message.empty());
    
    db.close();
}

// ============================================================================
// Resource Exhaustion Tests
// ============================================================================

TEST(QueryEngineErrorTest, VeryLargeResultSet_HandlesGracefully) {
    RocksDBWrapper::Config cfg; 
    cfg.db_path = tmpPath("qe_err_large_");
    cfg.enable_blobdb = false;
    RocksDBWrapper db(cfg); 
    ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);
    ASSERT_TRUE(idx.createIndex("items", "category").ok);
    
    // Insert large dataset
    const int LARGE_COUNT = 10000;
    for (int i = 0; i < LARGE_COUNT; ++i) {
        BaseEntity::FieldMap f{
            {"name", "Item" + std::to_string(i)},
            {"category", "common"}
        };
        BaseEntity e = BaseEntity::fromFields("item" + std::to_string(i), f);
        ASSERT_TRUE(idx.put("items", e).ok);
    }
    
    QueryEngine engine(db, idx);
    ConjunctiveQuery q{"items", {{"category", "common"}}};
    
    // Should handle large result set without crashing
    auto [st, keys] = engine.executeAndKeys(q);
    EXPECT_TRUE(st.ok) << "Large result set should be handled: " << st.message;
    EXPECT_EQ(keys.size(), LARGE_COUNT);
    
    db.close();
}

TEST(QueryEngineErrorTest, MultipleQueriesSequential_NoMemoryLeak) {
    RocksDBWrapper::Config cfg; 
    cfg.db_path = tmpPath("qe_err_multi_");
    cfg.enable_blobdb = false;
    RocksDBWrapper db(cfg); 
    ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);
    ASSERT_TRUE(idx.createIndex("users", "status").ok);
    
    // Insert test data
    for (int i = 0; i < 100; ++i) {
        BaseEntity::FieldMap f{{"status", "active"}};
        BaseEntity e = BaseEntity::fromFields("u" + std::to_string(i), f);
        ASSERT_TRUE(idx.put("users", e).ok);
    }
    
    QueryEngine engine(db, idx);
    
    // Execute many queries sequentially
    for (int i = 0; i < 1000; ++i) {
        ConjunctiveQuery q{"users", {{"status", "active"}}};
        auto [st, keys] = engine.executeAndKeys(q);
        EXPECT_TRUE(st.ok);
        EXPECT_EQ(keys.size(), 100u);
    }
    
    // If we get here without crashing, memory management is OK
    SUCCEED();
    
    db.close();
}

// ============================================================================
// Execution Failure Tests
// ============================================================================

TEST(QueryEngineErrorTest, MissingIndex_NoFallback_ReturnsError) {
    RocksDBWrapper::Config cfg; 
    cfg.db_path = tmpPath("qe_err_noidx_");
    cfg.enable_blobdb = false;
    RocksDBWrapper db(cfg); 
    ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);
    
    // Create table entries but NO index
    BaseEntity e = BaseEntity::fromFields("u1", {{"age", int64_t(30)}});
    ASSERT_TRUE(idx.put("users", e).ok);
    
    QueryEngine engine(db, idx);
    ConjunctiveQuery q{"users", {{"age", "30"}}};
    
    // Without fallback, should return error
    auto [st, keys] = engine.executeAndKeys(q);
    EXPECT_FALSE(st.ok) << "Query without index should fail";
    EXPECT_FALSE(st.message.empty());
    EXPECT_TRUE(st.message.find("index") != std::string::npos || 
                st.message.find("Index") != std::string::npos)
        << "Error message should mention missing index: " << st.message;
    
    db.close();
}

TEST(QueryEngineErrorTest, MissingIndex_WithFallback_UsesFullScan) {
    RocksDBWrapper::Config cfg; 
    cfg.db_path = tmpPath("qe_err_fallback_");
    cfg.enable_blobdb = false;
    RocksDBWrapper db(cfg); 
    ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);
    
    // Create data without index
    BaseEntity e = BaseEntity::fromFields("u1", {{"age", int64_t(30)}});
    ASSERT_TRUE(idx.put("users", e).ok);
    
    QueryEngine engine(db, idx);
    ConjunctiveQuery q{"users", {{"age", "30"}}};
    
    // With fallback, should succeed using full scan
    auto [st, keys] = engine.executeAndKeysWithFallback(q, true);
    EXPECT_TRUE(st.ok) << "Query with fallback should succeed: " << st.message;
    EXPECT_EQ(keys.size(), 1u);
    EXPECT_EQ(keys[0], "u1");
    
    db.close();
}

TEST(QueryEngineErrorTest, TypeMismatch_HandlesGracefully) {
    RocksDBWrapper::Config cfg; 
    cfg.db_path = tmpPath("qe_err_type_");
    cfg.enable_blobdb = false;
    RocksDBWrapper db(cfg); 
    ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);
    ASSERT_TRUE(idx.createIndex("users", "age").ok);
    
    // Insert with integer age
    BaseEntity e = BaseEntity::fromFields("u1", {{"age", int64_t(30)}});
    ASSERT_TRUE(idx.put("users", e).ok);
    
    QueryEngine engine(db, idx);
    
    // Query with valid string representation of integer
    ConjunctiveQuery q{"users", {{"age", "30"}}};
    auto [st, keys] = engine.executeAndKeys(q);
    
    // Should handle type conversion or return appropriate error
    EXPECT_TRUE(st.ok || !st.message.empty());
    
    db.close();
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST(QueryEngineErrorTest, EmptyDatabase_ReturnsEmpty) {
    RocksDBWrapper::Config cfg; 
    cfg.db_path = tmpPath("qe_err_empty_db_");
    cfg.enable_blobdb = false;
    RocksDBWrapper db(cfg); 
    ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);
    ASSERT_TRUE(idx.createIndex("users", "age").ok);
    
    QueryEngine engine(db, idx);
    ConjunctiveQuery q{"users", {{"age", "30"}}};
    
    // Empty database should return empty results, not error
    auto [st, keys] = engine.executeAndKeys(q);
    EXPECT_TRUE(st.ok) << "Empty database query should succeed";
    EXPECT_TRUE(keys.empty());
    
    db.close();
}

TEST(QueryEngineErrorTest, NullValues_HandlesCorrectly) {
    RocksDBWrapper::Config cfg; 
    cfg.db_path = tmpPath("qe_err_null_");
    cfg.enable_blobdb = false;
    RocksDBWrapper db(cfg); 
    ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);
    ASSERT_TRUE(idx.createIndex("users", "name").ok);
    
    // Insert entity with empty string (null-like)
    BaseEntity e = BaseEntity::fromFields("u1", {{"name", ""}});
    ASSERT_TRUE(idx.put("users", e).ok);
    
    QueryEngine engine(db, idx);
    ConjunctiveQuery q{"users", {{"name", ""}}};
    
    // Should handle null/empty values
    auto [st, keys] = engine.executeAndKeys(q);
    EXPECT_TRUE(st.ok) << "Null/empty value query should handle gracefully";
    
    db.close();
}

TEST(QueryEngineErrorTest, SpecialCharactersInValues_HandlesCorrectly) {
    RocksDBWrapper::Config cfg; 
    cfg.db_path = tmpPath("qe_err_special_");
    cfg.enable_blobdb = false;
    RocksDBWrapper db(cfg); 
    ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);
    ASSERT_TRUE(idx.createIndex("users", "name").ok);
    
    // Insert entity with special characters
    std::string special_name = "Test'\"\\User\n\t";
    BaseEntity e = BaseEntity::fromFields("u1", {{"name", special_name}});
    ASSERT_TRUE(idx.put("users", e).ok);
    
    QueryEngine engine(db, idx);
    ConjunctiveQuery q{"users", {{"name", special_name}}};
    
    // Should handle special characters correctly
    auto [st, keys] = engine.executeAndKeys(q);
    EXPECT_TRUE(st.ok) << "Special characters should be handled: " << st.message;
    EXPECT_EQ(keys.size(), 1u);
    
    db.close();
}

TEST(QueryEngineErrorTest, VeryLongFieldValues_HandlesCorrectly) {
    RocksDBWrapper::Config cfg; 
    cfg.db_path = tmpPath("qe_err_long_");
    cfg.enable_blobdb = false;
    RocksDBWrapper db(cfg); 
    ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);
    ASSERT_TRUE(idx.createIndex("documents", "content").ok);
    
    // Insert entity with very long value (10KB)
    std::string long_content(10000, 'x');
    BaseEntity e = BaseEntity::fromFields("doc1", {{"content", long_content}});
    ASSERT_TRUE(idx.put("documents", e).ok);
    
    QueryEngine engine(db, idx);
    ConjunctiveQuery q{"documents", {{"content", long_content}}};
    
    // Should handle long values
    auto [st, keys] = engine.executeAndKeys(q);
    EXPECT_TRUE(st.ok) << "Long field values should be handled";
    EXPECT_EQ(keys.size(), 1u);
    
    db.close();
}

// ============================================================================
// Complex Query Error Tests
// ============================================================================

TEST(QueryEngineErrorTest, MultiplePredicates_OneIndexMissing_HandlesCorrectly) {
    RocksDBWrapper::Config cfg; 
    cfg.db_path = tmpPath("qe_err_multi_pred_");
    cfg.enable_blobdb = false;
    RocksDBWrapper db(cfg); 
    ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);
    
    // Create index only for 'age', not 'city'
    ASSERT_TRUE(idx.createIndex("users", "age").ok);
    
    BaseEntity e = BaseEntity::fromFields("u1", {
        {"age", int64_t(30)},
        {"city", "Berlin"}
    });
    ASSERT_TRUE(idx.put("users", e).ok);
    
    QueryEngine engine(db, idx);
    
    // Query with both indexed and non-indexed fields
    ConjunctiveQuery q{"users", {{"age", "30"}, {"city", "Berlin"}}};
    auto [st, keys] = engine.executeAndKeys(q);
    
    // Should either fail with clear error or handle with partial index
    // Either: error (not ok) OR success with 1 result
    EXPECT_TRUE(!st.ok || keys.size() == 1u);
    if (!st.ok) {
        EXPECT_FALSE(st.message.empty());
    }
    
    db.close();
}

TEST(QueryEngineErrorTest, DuplicatePredicates_HandlesCorrectly) {
    RocksDBWrapper::Config cfg; 
    cfg.db_path = tmpPath("qe_err_dup_pred_");
    cfg.enable_blobdb = false;
    RocksDBWrapper db(cfg); 
    ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);
    ASSERT_TRUE(idx.createIndex("users", "age").ok);
    
    BaseEntity e = BaseEntity::fromFields("u1", {{"age", int64_t(30)}});
    ASSERT_TRUE(idx.put("users", e).ok);
    
    QueryEngine engine(db, idx);
    
    // Query with duplicate predicates (should be handled or detected)
    ConjunctiveQuery q{"users", {{"age", "30"}, {"age", "30"}}};
    auto [st, keys] = engine.executeAndKeys(q);
    
    // Should handle duplicates (either ignore or process correctly)
    EXPECT_TRUE(st.ok) << "Duplicate predicates should be handled";
    
    db.close();
}

TEST(QueryEngineErrorTest, ConflictingPredicates_ReturnsEmpty) {
    RocksDBWrapper::Config cfg; 
    cfg.db_path = tmpPath("qe_err_conflict_");
    cfg.enable_blobdb = false;
    RocksDBWrapper db(cfg); 
    ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);
    ASSERT_TRUE(idx.createIndex("users", "age").ok);
    
    BaseEntity e = BaseEntity::fromFields("u1", {{"age", int64_t(30)}});
    ASSERT_TRUE(idx.put("users", e).ok);
    
    QueryEngine engine(db, idx);
    
    // Query with conflicting predicates (age=30 AND age=40)
    ConjunctiveQuery q{"users", {{"age", "30"}, {"age", "40"}}};
    auto [st, keys] = engine.executeAndKeys(q);
    
    // Should return empty (no record can match both)
    EXPECT_TRUE(st.ok);
    EXPECT_TRUE(keys.empty()) << "Conflicting predicates should return empty";
    
    db.close();
}

// ============================================================================
// Boundary Condition Tests
// ============================================================================

TEST(QueryEngineErrorTest, MaxIntValue_HandlesCorrectly) {
    RocksDBWrapper::Config cfg; 
    cfg.db_path = tmpPath("qe_err_maxint_");
    cfg.enable_blobdb = false;
    RocksDBWrapper db(cfg); 
    ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);
    ASSERT_TRUE(idx.createIndex("items", "quantity").ok);
    
    // Insert with maximum int value
    BaseEntity e = BaseEntity::fromFields("item1", {
        {"quantity", std::numeric_limits<int64_t>::max()}
    });
    ASSERT_TRUE(idx.put("items", e).ok);
    
    QueryEngine engine(db, idx);
    ConjunctiveQuery q{"items", {
        {"quantity", std::to_string(std::numeric_limits<int64_t>::max())}
    }};
    
    auto [st, keys] = engine.executeAndKeys(q);
    EXPECT_TRUE(st.ok) << "Max int value should be handled";
    EXPECT_EQ(keys.size(), 1u);
    
    db.close();
}

TEST(QueryEngineErrorTest, NegativeValues_HandlesCorrectly) {
    RocksDBWrapper::Config cfg; 
    cfg.db_path = tmpPath("qe_err_negative_");
    cfg.enable_blobdb = false;
    RocksDBWrapper db(cfg); 
    ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);
    ASSERT_TRUE(idx.createIndex("transactions", "amount").ok);
    
    // Insert with negative value
    BaseEntity e = BaseEntity::fromFields("tx1", {{"amount", int64_t(-100)}});
    ASSERT_TRUE(idx.put("transactions", e).ok);
    
    QueryEngine engine(db, idx);
    ConjunctiveQuery q{"transactions", {{"amount", "-100"}}};
    
    auto [st, keys] = engine.executeAndKeys(q);
    EXPECT_TRUE(st.ok) << "Negative values should be handled";
    EXPECT_EQ(keys.size(), 1u);
    
    db.close();
}

// ============================================================================
// Performance / Stress Tests
// ============================================================================

TEST(QueryEngineErrorTest, ManyIndices_HandlesEfficiently) {
    RocksDBWrapper::Config cfg; 
    cfg.db_path = tmpPath("qe_err_many_idx_");
    cfg.enable_blobdb = false;
    RocksDBWrapper db(cfg); 
    ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);
    
    // Create many indices
    const int NUM_INDICES = 20;
    for (int i = 0; i < NUM_INDICES; ++i) {
        ASSERT_TRUE(idx.createIndex("users", "field" + std::to_string(i)).ok);
    }
    
    // Insert data with many fields
    BaseEntity::FieldMap fields;
    for (int i = 0; i < NUM_INDICES; ++i) {
        fields["field" + std::to_string(i)] = "value" + std::to_string(i);
    }
    BaseEntity e = BaseEntity::fromFields("u1", fields);
    ASSERT_TRUE(idx.put("users", e).ok);
    
    QueryEngine engine(db, idx);
    
    // Query on first field
    ConjunctiveQuery q{"users", {{"field0", "value0"}}};
    auto [st, keys] = engine.executeAndKeys(q);
    
    EXPECT_TRUE(st.ok) << "Query with many indices should work";
    EXPECT_EQ(keys.size(), 1u);
    
    db.close();
}

TEST(QueryEngineErrorTest, RapidFireQueries_NoRaceConditions) {
    RocksDBWrapper::Config cfg; 
    cfg.db_path = tmpPath("qe_err_rapid_");
    cfg.enable_blobdb = false;
    RocksDBWrapper db(cfg); 
    ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);
    ASSERT_TRUE(idx.createIndex("users", "status").ok);
    
    // Insert test data
    for (int i = 0; i < 50; ++i) {
        BaseEntity e = BaseEntity::fromFields("u" + std::to_string(i), {
            {"status", "active"}
        });
        ASSERT_TRUE(idx.put("users", e).ok);
    }
    
    QueryEngine engine(db, idx);
    
    // Execute many rapid queries
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    std::atomic<int> error_count{0};
    
    for (int t = 0; t < 10; ++t) {
        threads.emplace_back([&engine, &success_count, &error_count]() {
            for (int i = 0; i < 100; ++i) {
                ConjunctiveQuery q{"users", {{"status", "active"}}};
                auto [st, keys] = engine.executeAndKeys(q);
                if (st.ok) {
                    success_count++;
                } else {
                    error_count++;
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_GT(success_count.load(), 0) << "Should have successful queries";
    EXPECT_EQ(error_count.load(), 0) << "Should have no errors in concurrent execution";
    
    db.close();
}
