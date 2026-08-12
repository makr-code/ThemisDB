/// @file test_index_manager_di.cpp
/// @brief Integration tests for IndexManager SecondaryIndexAdapter,
///        including partial (filtered) index support.

#include <gtest/gtest.h>
#include <filesystem>
#include <chrono>
#include <string>
#include <vector>

#include "index/index_manager.h"
#include "index/secondary_index.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "themis/base/interfaces/index_interface.h"

using namespace themis;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string makeTempDbPath(const std::string& name) {
    namespace fs = std::filesystem;
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() / (name + std::to_string(now))).string();
}

/// Build an IndexManager backed by a fresh RocksDB instance.
static std::pair<std::shared_ptr<RocksDBWrapper>,
                 std::shared_ptr<IndexManager>>
makeIndexManager(const std::string& db_path) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = db_path;
    auto db = std::make_shared<RocksDBWrapper>(cfg);
    if (!db->open()) return {nullptr, nullptr};

    auto mgr = IndexManager::createDefault();
    mgr->setRocksDB(db);
    return {db, mgr};
}

static Result<ISecondaryIndex*> createSecondaryIndexWithConfig(
    const std::shared_ptr<IndexManager>& mgr,
    std::string_view name,
    std::string_view field_name,
    const std::string& config) {
    auto fn = static_cast<Result<ISecondaryIndex*>(IndexManager::*)(
        std::string_view,
        std::string_view,
        const std::string&)>(&IndexManager::createSecondaryIndex);
    return (mgr.get()->*fn)(name, field_name, config);
}

// ===========================================================================
// No-RocksDB: createSecondaryIndex returns ERR_INDEX_NOT_INITIALIZED
// ===========================================================================

TEST(IndexManagerDI, NoRocksDB_CreateSecondaryIndex_ReturnsError) {
    auto mgr = IndexManager::createDefault();
    auto result = mgr->createSecondaryIndex("users", "email");
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), errors::ErrorCode::ERR_INDEX_NOT_INITIALIZED);
}

// ===========================================================================
// SecondaryIndexAdapter: regular index
// ===========================================================================

TEST(IndexManagerDI, CreateSecondaryIndex_ReturnsNonNullAdapter) {
    auto [db, mgr] = makeIndexManager(makeTempDbPath("idx_mgr_create_"));
    ASSERT_NE(db, nullptr);

    auto result = mgr->createSecondaryIndex("users", "email");
    ASSERT_TRUE(result.has_value()) << result.error().message();
    EXPECT_NE(*result, nullptr);

    auto* idx = *result;
    EXPECT_EQ(idx->getName(), "users");
    EXPECT_EQ(idx->getFieldName(), "email");
}

TEST(IndexManagerDI, SecondaryIndex_InsertAndLookup) {
    auto [db, mgr] = makeIndexManager(makeTempDbPath("idx_mgr_lookup_"));
    ASSERT_NE(db, nullptr);

    auto result = mgr->createSecondaryIndex("orders", "customer_id");
    ASSERT_TRUE(result.has_value()) << result.error().message();
    auto* idx = *result;
    ASSERT_NE(idx, nullptr);

    EXPECT_TRUE(idx->insert("cust42", "order1"));
    EXPECT_TRUE(idx->insert("cust42", "order2"));
    EXPECT_TRUE(idx->insert("cust99", "order3"));

    auto keys42 = idx->lookup("cust42");
    EXPECT_EQ(keys42.size(), 2u);

    auto keys99 = idx->lookup("cust99");
    EXPECT_EQ(keys99.size(), 1u);
    EXPECT_EQ(keys99[0], "order3");
}

TEST(IndexManagerDI, SecondaryIndex_RemoveEntry) {
    auto [db, mgr] = makeIndexManager(makeTempDbPath("idx_mgr_remove_"));
    ASSERT_NE(db, nullptr);

    auto result = mgr->createSecondaryIndex("sessions", "user_id");
    ASSERT_TRUE(result.has_value()) << result.error().message();
    auto* idx = *result;
    ASSERT_NE(idx, nullptr);

    EXPECT_TRUE(idx->insert("u1", "sess1"));

    auto before = idx->lookup("u1");
    ASSERT_EQ(before.size(), 1u);

    EXPECT_TRUE(idx->remove("u1", "sess1"));

    auto after = idx->lookup("u1");
    EXPECT_TRUE(after.empty());
}

TEST(IndexManagerDI, CreateSecondaryIndex_Duplicate_ReturnsSameAdapter) {
    auto [db, mgr] = makeIndexManager(makeTempDbPath("idx_mgr_dup_"));
    ASSERT_NE(db, nullptr);

    auto r1 = mgr->createSecondaryIndex("products", "sku");
    auto r2 = mgr->createSecondaryIndex("products", "sku");
    ASSERT_TRUE(r1.has_value());
    ASSERT_TRUE(r2.has_value());
    EXPECT_EQ(*r1, *r2) << "Duplicate create should return the same adapter";
}

TEST(IndexManagerDI, DropSecondaryIndex_RemovesFromRegistry) {
    auto [db, mgr] = makeIndexManager(makeTempDbPath("idx_mgr_drop_"));
    ASSERT_NE(db, nullptr);

    ASSERT_TRUE(mgr->createSecondaryIndex("items", "tag").has_value());

    auto list_before = mgr->listIndexes();
    EXPECT_EQ(list_before.size(), 1u);

    ASSERT_TRUE(mgr->dropIndex("items").has_value());

    auto list_after = mgr->listIndexes();
    EXPECT_TRUE(list_after.empty());
}

// ===========================================================================
// SecondaryIndexAdapter: partial (filtered) index
// ===========================================================================

TEST(IndexManagerDI, PartialIndex_AdapterIsNonNull) {
    auto [db, mgr] = makeIndexManager(makeTempDbPath("idx_mgr_partial_nn_"));
    ASSERT_NE(db, nullptr);

    auto result = createSecondaryIndexWithConfig(
        mgr, "users", "email", "partial:status = 'active'");
    ASSERT_TRUE(result.has_value()) << result.error().message();
    EXPECT_NE(*result, nullptr);

    auto* idx = *result;
    EXPECT_EQ(idx->getName(), "users");
    EXPECT_EQ(idx->getFieldName(), "email");
}

TEST(IndexManagerDI, PartialIndex_LookupOnlyMatchingRows) {
    auto [db, mgr] = makeIndexManager(makeTempDbPath("idx_mgr_partial_lookup_"));
    ASSERT_NE(db, nullptr);

    // Create a partial index via the IndexManager facade.
    auto result = createSecondaryIndexWithConfig(
        mgr, "users", "email", "partial:status = 'active'");
    ASSERT_TRUE(result.has_value()) << result.error().message();
    auto* idx = *result;
    ASSERT_NE(idx, nullptr);

    // Use the underlying SecondaryIndexManager to insert full entities so that
    // the predicate evaluator can inspect the "status" field.
    auto sim = mgr->getSecondaryIndexManager();
    ASSERT_NE(sim, nullptr);

    BaseEntity::FieldMap f1{{"email", std::string("alice@example.com")},
                             {"status", std::string("active")}};
    BaseEntity::FieldMap f2{{"email", std::string("bob@example.com")},
                             {"status", std::string("active")}};
    BaseEntity::FieldMap f3{{"email", std::string("carol@example.com")},
                             {"status", std::string("inactive")}};

    ASSERT_TRUE(sim->put("users", BaseEntity::fromFields("u1", f1)).ok);
    ASSERT_TRUE(sim->put("users", BaseEntity::fromFields("u2", f2)).ok);
    ASSERT_TRUE(sim->put("users", BaseEntity::fromFields("u3", f3)).ok);

    // Lookup through the ISecondaryIndex adapter: only active users should appear.
    auto keys_alice = idx->lookup("alice@example.com");
    ASSERT_EQ(keys_alice.size(), 1u);
    EXPECT_EQ(keys_alice[0], "u1");

    auto keys_bob = idx->lookup("bob@example.com");
    ASSERT_EQ(keys_bob.size(), 1u);
    EXPECT_EQ(keys_bob[0], "u2");

    // Inactive user carol must NOT be in the partial index.
    auto keys_carol = idx->lookup("carol@example.com");
    EXPECT_TRUE(keys_carol.empty());
}

TEST(IndexManagerDI, PartialIndex_DropWorks) {
    auto [db, mgr] = makeIndexManager(makeTempDbPath("idx_mgr_partial_drop_"));
    ASSERT_NE(db, nullptr);

    ASSERT_TRUE(createSecondaryIndexWithConfig(
        mgr, "events", "category", "partial:published = '1'").has_value());

    ASSERT_EQ(mgr->listIndexes().size(), 1u);
    ASSERT_TRUE(mgr->dropIndex("events").has_value());
    EXPECT_TRUE(mgr->listIndexes().empty());
}

TEST(IndexManagerDI, PartialIndex_Statistics_ContainsType) {
    auto [db, mgr] = makeIndexManager(makeTempDbPath("idx_mgr_partial_stats_"));
    ASSERT_NE(db, nullptr);

    auto result = createSecondaryIndexWithConfig(
        mgr, "docs", "title", "partial:visible = '1'");
    ASSERT_TRUE(result.has_value()) << result.error().message();
    auto* idx = *result;
    ASSERT_NE(idx, nullptr);

    auto stats = idx->getStatistics();
    EXPECT_FALSE(stats.empty());
    // Statistics JSON must contain the index type and predicate.
    EXPECT_NE(stats.find("partial"), std::string::npos);
    EXPECT_NE(stats.find("visible = '1'"), std::string::npos);
}

// ===========================================================================
// getSecondaryIndex after creation
// ===========================================================================

TEST(IndexManagerDI, GetSecondaryIndex_AfterCreate_ReturnsAdapter) {
    auto [db, mgr] = makeIndexManager(makeTempDbPath("idx_mgr_get_"));
    ASSERT_NE(db, nullptr);

    ASSERT_TRUE(mgr->createSecondaryIndex("catalog", "sku").has_value());

    auto get_result = mgr->getSecondaryIndex("catalog");
    ASSERT_TRUE(get_result.has_value()) << get_result.error().message();
    EXPECT_NE(*get_result, nullptr);
}

TEST(IndexManagerDI, GetSecondaryIndex_NotFound_ReturnsError) {
    auto [db, mgr] = makeIndexManager(makeTempDbPath("idx_mgr_get_nf_"));
    ASSERT_NE(db, nullptr);

    auto result = mgr->getSecondaryIndex("nonexistent");
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), errors::ErrorCode::ERR_INDEX_NOT_FOUND);
}

// ===========================================================================
// exportIndexStats (Issue #1866)
// ===========================================================================

TEST(IndexManagerDI, ExportIndexStats_NoRocksDB_ReturnsEmpty) {
    // Without a wired RocksDB the secondary manager is absent –
    // exportIndexStats must return an empty vector rather than crashing.
    auto mgr = IndexManager::createDefault();
    auto stats = mgr->exportIndexStats("users");
    EXPECT_TRUE(stats.empty());
}

TEST(IndexManagerDI, ExportIndexStats_NoIndexes_ReturnsEmpty) {
    auto [db, mgr] = makeIndexManager(makeTempDbPath("idx_export_empty_"));
    ASSERT_NE(db, nullptr);

    // No indexes created for "orders" → stats must be empty.
    auto stats = mgr->exportIndexStats("orders");
    EXPECT_TRUE(stats.empty());
}

TEST(IndexManagerDI, ExportIndexStats_SingleIndex_ReturnsOne) {
    auto [db, mgr] = makeIndexManager(makeTempDbPath("idx_export_single_"));
    ASSERT_NE(db, nullptr);

    ASSERT_TRUE(mgr->createSecondaryIndex("products", "name").has_value());

    auto stats = mgr->exportIndexStats("products");
    ASSERT_EQ(stats.size(), 1u);
    EXPECT_EQ(stats[0].table, "products");
    EXPECT_EQ(stats[0].column, "name");
    EXPECT_FALSE(stats[0].type.empty());
}

TEST(IndexManagerDI, ExportIndexStats_MultipleIndexes_ReturnsAll) {
    auto [db, mgr] = makeIndexManager(makeTempDbPath("idx_export_multi_"));
    ASSERT_NE(db, nullptr);

    ASSERT_TRUE(mgr->createSecondaryIndex("employees", "email").has_value());
    ASSERT_TRUE(mgr->createSecondaryIndex("employees", "department").has_value());

    auto stats = mgr->exportIndexStats("employees");
    EXPECT_EQ(stats.size(), 2u);

    bool found_email = false, found_dept = false;
    for (const auto& s : stats) {
        EXPECT_EQ(s.table, "employees");
        if (s.column == "email")      found_email = true;
        if (s.column == "department") found_dept  = true;
    }
    EXPECT_TRUE(found_email);
    EXPECT_TRUE(found_dept);
}

TEST(IndexManagerDI, ExportIndexStats_WrongTable_ReturnsEmpty) {
    auto [db, mgr] = makeIndexManager(makeTempDbPath("idx_export_wrong_"));
    ASSERT_NE(db, nullptr);

    ASSERT_TRUE(mgr->createSecondaryIndex("invoices", "status").has_value());

    // Stats for a table that has no indexes must be empty.
    auto stats = mgr->exportIndexStats("shipments");
    EXPECT_TRUE(stats.empty());
}

TEST(IndexManagerDI, ExportIndexStats_EntryCountReflectsInserts) {
    auto [db, mgr] = makeIndexManager(makeTempDbPath("idx_export_count_"));
    ASSERT_NE(db, nullptr);

    auto result = mgr->createSecondaryIndex("events", "type");
    ASSERT_TRUE(result.has_value()) << result.error().message();
    auto* idx = *result;
    ASSERT_NE(idx, nullptr);

    // Before any inserts the entry count must be 0.
    {
        auto stats = mgr->exportIndexStats("events");
        ASSERT_EQ(stats.size(), 1u);
        EXPECT_EQ(stats[0].entry_count, 0u);
    }

    // Insert a few entries.
    ASSERT_TRUE(idx->insert("click", "evt-1"));
    ASSERT_TRUE(idx->insert("view",  "evt-2"));
    ASSERT_TRUE(idx->insert("click", "evt-3"));

    // After inserts the entry count must reflect the stored entries.
    {
        auto stats = mgr->exportIndexStats("events");
        ASSERT_EQ(stats.size(), 1u);
        EXPECT_GE(stats[0].entry_count, 2u);
    }
}

TEST(IndexManagerDI, ExportIndexStats_PartialIndex_TypeContainsPartial) {
    auto [db, mgr] = makeIndexManager(makeTempDbPath("idx_export_partial_"));
    ASSERT_NE(db, nullptr);

    ASSERT_TRUE(createSecondaryIndexWithConfig(
        mgr, "articles", "category", "partial:published = '1'").has_value());

    auto stats = mgr->exportIndexStats("articles");
    ASSERT_EQ(stats.size(), 1u);
    // The SecondaryIndexManager must report the index type as "partial".
    EXPECT_NE(stats[0].type.find("partial"), std::string::npos);
}


