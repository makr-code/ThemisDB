/**
 * @file test_aql_mutations_executor.cpp
 * @brief Unit tests for EPIC-004 Phase 3: MutationExecutor.
 *
 * Tests cover INSERT, UPDATE, REMOVE, REPLACE, and UPSERT execution paths
 * using a MockStorageContext that records all storage calls without requiring
 * a real RocksDB instance.
 */

#include <gtest/gtest.h>
#include "query/mutation_executor.h"
#include "query/aql_translator.h"
#include "query/mutation_execution_plan.h"

#include <atomic>
#include <string>
#include <utility>
#include <vector>

using namespace themis::query;
using namespace themis;

// ============================================================================
// MockStorageContext
// ============================================================================

struct MockStorageContext : MutationExecutor::StorageContext {
    std::vector<std::pair<std::string, std::string>> puts;   ///< (key, value) pairs written
    std::vector<std::string>                         deletes; ///< Keys deleted
    std::vector<nlohmann::json>                      wal_entries;
    std::atomic<int>                                 key_counter{0};
    bool                                             fail_on_put    = false;
    bool                                             fail_on_remove = false;
    bool                                             fail_on_wal    = false;
    bool                                             key_exists     = false; ///< for UPSERT exists()

    bool put(std::string_view /*col*/, std::string_view key, std::string_view val) override {
        if (fail_on_put) {
          return false;
        }
        puts.emplace_back(std::string(key), std::string(val));
        return true;
    }

    bool remove(std::string_view /*col*/, std::string_view key) override {
        if (fail_on_remove) {
          return false;
        }
        deletes.emplace_back(std::string(key));
        return true;
    }

    bool exists(std::string_view /*col*/, std::string_view /*key*/) override {
        return key_exists;
    }

    std::string generateKey(std::string_view col) override {
        return std::string(col) + "_" + std::to_string(++key_counter);
    }

    bool writeWAL(std::string_view /*col*/, const nlohmann::json& entry) override {
        if (fail_on_wal) {
          return false;
        }
        wal_entries.push_back(entry);
        return true;
    }
};

// ============================================================================
// Helpers: build minimal plans for testing
// ============================================================================

static MutationExecutionPlan makeInsertPlan(const std::string& col = "users") {
    AqlMutationTranslator translator;
    auto node = std::make_shared<InsertNode>();
    node->collection = col;
    node->documents.push_back(std::make_shared<VariableExpr>("doc"));
    return translator.translate(node);
}

static MutationExecutionPlan makeUpdatePlan(const std::string& col = "users") {
    AqlMutationTranslator translator;
    auto node = std::make_shared<UpdateNode>();
    node->collection  = col;
    node->update_expr = std::make_shared<VariableExpr>("upd");
    node->filter      = std::make_shared<VariableExpr>("filter");
    return translator.translate(node);
}

static MutationExecutionPlan makeRemovePlan(const std::string& col = "logs") {
    AqlMutationTranslator translator;
    auto node = std::make_shared<RemoveNode>();
    node->collection = col;
    node->filter     = std::make_shared<VariableExpr>("filter");
    return translator.translate(node);
}

static MutationExecutionPlan makeReplacePlan(const std::string& col = "orders") {
    AqlMutationTranslator translator;
    auto node = std::make_shared<ReplaceNode>();
    node->collection  = col;
    node->search_expr = std::make_shared<VariableExpr>("s");
    node->replacement = std::make_shared<VariableExpr>("r");
    return translator.translate(node);
}

static MutationExecutionPlan makeUpsertPlan(const std::string& col = "catalog") {
    AqlMutationTranslator translator;
    auto node = std::make_shared<UpsertNode>();
    node->collection  = col;
    node->search_expr = std::make_shared<VariableExpr>("s");
    node->insert_doc  = std::make_shared<VariableExpr>("i");
    node->update_doc  = std::make_shared<VariableExpr>("u");
    return translator.translate(node);
}

// ============================================================================
// ExecuteInsert — 5 cases
// ============================================================================

TEST(MutationExecutor, InsertSuccessful) {
    MutationExecutor exec;
    MockStorageContext ctx;
    auto plan   = makeInsertPlan();
    auto result = exec.execute(plan, ctx);
    EXPECT_TRUE(result.success);
}

TEST(MutationExecutor, InsertAffectedCountAtLeastOne) {
    MutationExecutor exec;
    MockStorageContext ctx;
    auto result = exec.execute(makeInsertPlan(), ctx);
    EXPECT_GE(result.affected_count, 1);
}

TEST(MutationExecutor, InsertInsertedIdsPopulated) {
    MutationExecutor exec;
    MockStorageContext ctx;
    auto result = exec.execute(makeInsertPlan("items"), ctx);
    EXPECT_TRUE(result.success);
    // At least one key was generated
    EXPECT_FALSE(result.inserted_ids.empty());
}

TEST(MutationExecutor, InsertKeyContainsCollectionPrefix) {
    MutationExecutor exec;
    MockStorageContext ctx;
    auto result = exec.execute(makeInsertPlan("products"), ctx);
    ASSERT_FALSE(result.inserted_ids.empty());
    // Generated key should start with collection name
    EXPECT_NE(result.inserted_ids[0].find("products"), std::string::npos);
}

TEST(MutationExecutor, InsertWALEntryRecorded) {
    MutationExecutor exec;
    MockStorageContext ctx;
    auto r = exec.execute(makeInsertPlan("users"), ctx);
    (void)r;
    ASSERT_FALSE(ctx.wal_entries.empty());
    EXPECT_EQ(ctx.wal_entries[0]["operation"].get<std::string>(), "INSERT");
}

// ============================================================================
// ExecuteUpdate — 5 cases
// ============================================================================

TEST(MutationExecutor, UpdateSuccessful) {
    MutationExecutor exec;
    MockStorageContext ctx;
    auto result = exec.execute(makeUpdatePlan(), ctx);
    EXPECT_TRUE(result.success);
}

TEST(MutationExecutor, UpdateAffectedCountOne) {
    MutationExecutor exec;
    MockStorageContext ctx;
    auto result = exec.execute(makeUpdatePlan(), ctx);
    EXPECT_EQ(result.affected_count, 1);
}

TEST(MutationExecutor, UpdateNoInsertedIds) {
    MutationExecutor exec;
    MockStorageContext ctx;
    auto result = exec.execute(makeUpdatePlan(), ctx);
    EXPECT_TRUE(result.inserted_ids.empty());
}

TEST(MutationExecutor, UpdatePutWasCalled) {
    MutationExecutor exec;
    MockStorageContext ctx;
    auto r = exec.execute(makeUpdatePlan("products"), ctx);
    (void)r;
    EXPECT_FALSE(ctx.puts.empty());
}

TEST(MutationExecutor, UpdateWALEntryRecorded) {
    MutationExecutor exec;
    MockStorageContext ctx;
    auto r = exec.execute(makeUpdatePlan(), ctx);
    (void)r;
    bool found = false;
    for (const auto& e : ctx.wal_entries) {
        if (e.contains("operation") && e["operation"] == "UPDATE") { found = true; break; }
    }
    EXPECT_TRUE(found);
}

// ============================================================================
// ExecuteRemove — 5 cases
// ============================================================================

TEST(MutationExecutor, RemoveSuccessful) {
    MutationExecutor exec;
    MockStorageContext ctx;
    auto result = exec.execute(makeRemovePlan(), ctx);
    EXPECT_TRUE(result.success);
}

TEST(MutationExecutor, RemoveAffectedCountOne) {
    MutationExecutor exec;
    MockStorageContext ctx;
    auto result = exec.execute(makeRemovePlan(), ctx);
    EXPECT_EQ(result.affected_count, 1);
}

TEST(MutationExecutor, RemoveDeleteWasCalled) {
    MutationExecutor exec;
    MockStorageContext ctx;
    auto r = exec.execute(makeRemovePlan("archive"), ctx);
    (void)r;
    EXPECT_FALSE(ctx.deletes.empty());
}

TEST(MutationExecutor, RemoveNoInsertedIds) {
    MutationExecutor exec;
    MockStorageContext ctx;
    auto result = exec.execute(makeRemovePlan(), ctx);
    EXPECT_TRUE(result.inserted_ids.empty());
}

TEST(MutationExecutor, RemoveWALEntryRecorded) {
    MutationExecutor exec;
    MockStorageContext ctx;
    auto r = exec.execute(makeRemovePlan(), ctx);
    (void)r;
    bool found = false;
    for (const auto& e : ctx.wal_entries) {
        if (e.contains("operation") && e["operation"] == "REMOVE") { found = true; break; }
    }
    EXPECT_TRUE(found);
}

// ============================================================================
// ExecuteReplace — 4 cases
// ============================================================================

TEST(MutationExecutor, ReplaceSuccessful) {
    MutationExecutor exec;
    MockStorageContext ctx;
    auto result = exec.execute(makeReplacePlan(), ctx);
    EXPECT_TRUE(result.success);
}

TEST(MutationExecutor, ReplaceAffectedCountOne) {
    MutationExecutor exec;
    MockStorageContext ctx;
    auto result = exec.execute(makeReplacePlan(), ctx);
    EXPECT_EQ(result.affected_count, 1);
}

TEST(MutationExecutor, ReplacePutWasCalled) {
    MutationExecutor exec;
    MockStorageContext ctx;
    auto r = exec.execute(makeReplacePlan("orders"), ctx);
    (void)r;
    EXPECT_FALSE(ctx.puts.empty());
}

TEST(MutationExecutor, ReplaceWALEntryRecorded) {
    MutationExecutor exec;
    MockStorageContext ctx;
    auto r = exec.execute(makeReplacePlan(), ctx);
    (void)r;
    bool found = false;
    for (const auto& e : ctx.wal_entries) {
        if (e.contains("operation") && e["operation"] == "REPLACE") { found = true; break; }
    }
    EXPECT_TRUE(found);
}

// ============================================================================
// ExecuteUpsert — 5 cases
// ============================================================================

TEST(MutationExecutor, UpsertInsertBranchWhenKeyNotExists) {
    MutationExecutor exec;
    MockStorageContext ctx;
    ctx.key_exists = false;
    auto result = exec.execute(makeUpsertPlan(), ctx);
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.inserted_ids.empty());
}

TEST(MutationExecutor, UpsertUpdateBranchWhenKeyExists) {
    MutationExecutor exec;
    MockStorageContext ctx;
    ctx.key_exists = true;
    auto result = exec.execute(makeUpsertPlan(), ctx);
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.inserted_ids.empty()); // update branch — no new ids
}

TEST(MutationExecutor, UpsertAffectedCountOne) {
    MutationExecutor exec;
    MockStorageContext ctx;
    auto result = exec.execute(makeUpsertPlan(), ctx);
    EXPECT_EQ(result.affected_count, 1);
}

TEST(MutationExecutor, UpsertWALEntryRecorded) {
    MutationExecutor exec;
    MockStorageContext ctx;
    auto r = exec.execute(makeUpsertPlan(), ctx);
    (void)r;
    bool found = false;
    for (const auto& e : ctx.wal_entries) {
        if (e.contains("operation") && e["operation"] == "UPSERT") { found = true; break; }
    }
    EXPECT_TRUE(found);
}

TEST(MutationExecutor, UpsertInsertBranchPutWasCalled) {
    MutationExecutor exec;
    MockStorageContext ctx;
    ctx.key_exists = false;
    auto r = exec.execute(makeUpsertPlan("catalog"), ctx);
    (void)r;
    EXPECT_FALSE(ctx.puts.empty());
}

// ============================================================================
// Failure path — 4 cases
// ============================================================================

TEST(MutationExecutor, InsertFailOnPutReturnsFailure) {
    MutationExecutor exec;
    MockStorageContext ctx;
    ctx.fail_on_put = true;
    auto result = exec.execute(makeInsertPlan(), ctx);
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_code.empty());
}

TEST(MutationExecutor, UpdateFailOnPutReturnsFailure) {
    MutationExecutor exec;
    MockStorageContext ctx;
    ctx.fail_on_put = true;
    auto result = exec.execute(makeUpdatePlan(), ctx);
    EXPECT_FALSE(result.success);
}

TEST(MutationExecutor, RemoveFailOnRemoveReturnsFailure) {
    MutationExecutor exec;
    MockStorageContext ctx;
    ctx.fail_on_remove = true;
    auto result = exec.execute(makeRemovePlan(), ctx);
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_code.empty());
}

TEST(MutationExecutor, InsertFailOnWALReturnsFailure) {
    MutationExecutor exec;
    MockStorageContext ctx;
    ctx.fail_on_wal = true;
    auto result = exec.execute(makeInsertPlan(), ctx);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_code, "WAL_WRITE_FAILURE");
}

// ============================================================================
// WAL entries recorded for all mutation types — 2 cases
// ============================================================================

TEST(MutationExecutor, AllMutationTypesRecordWALEntries) {
    MutationExecutor exec;

    auto testWAL = [&](auto planFn, const std::string& expectedOp) {
        MockStorageContext ctx;
        auto r = exec.execute(planFn(), ctx);
        (void)r;
        bool found = false;
        for (const auto& e : ctx.wal_entries) {
            if (e.contains("operation") && e["operation"] == expectedOp) {
                found = true; break;
            }
        }
        EXPECT_TRUE(found) << "WAL entry for " << expectedOp << " not found";
    };

    testWAL([]() { return makeInsertPlan(); },  "INSERT");
    testWAL([]() { return makeUpdatePlan(); },  "UPDATE");
    testWAL([]() { return makeRemovePlan(); },  "REMOVE");
    testWAL([]() { return makeReplacePlan(); }, "REPLACE");
    testWAL([]() { return makeUpsertPlan(); },  "UPSERT");
}

TEST(MutationExecutor, WALEntryContainsCollectionName) {
    MutationExecutor exec;
    MockStorageContext ctx;
    auto r = exec.execute(makeInsertPlan("widgets"), ctx);
    (void)r;
    ASSERT_FALSE(ctx.wal_entries.empty());
    EXPECT_EQ(ctx.wal_entries[0]["collection"].get<std::string>(), "widgets");
}
