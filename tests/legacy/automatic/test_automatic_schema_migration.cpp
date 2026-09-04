// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors
//
// Focused unit + integration tests for SchemaMigration (v1.7.0, Issue #127).
// Covers:
//   1. Version tracking and operationCount() bookkeeping
//   2. addColumn: metadata written, backfill tracking
//   3. renameColumn: metadata renamed, dual-write marker written
//   4. addIndex: metadata written, online/blocking flag
//   5. dropColumn: drop marker written, grace period recorded
//   6. Custom migration callback: invoked and able to read/write storage
//   7. Rollback (AUTOMATIC): storage keys restored on failure
//   8. Rollback (MANUAL): rollback() unwinds after partial apply
//   9. Error paths: empty table, empty column name, empty index name, etc.
//  10. Phase progression: SHADOW_CREATE → DUAL_WRITE → BACKFILL → … → CLEANUP
//  11. Fluent builder: chained calls return the same object
//  12. Multiple operations in one migration applied in order
//  13. addIndex with no columns returns failure
//  14. Grace period stored correctly for dropColumn

#include <gtest/gtest.h>
#include <map>
#include <string>
#include <vector>

#include "updates/schema_migration.h"

using namespace themis;
using namespace themis::updates;

// ============================================================================
// Helpers: in-memory IMigrationStorage
// ============================================================================

namespace {  // anonymous namespace prevents ODR conflicts with InMemoryStorage
             // defined in other test TUs (e.g. test_ab_test_manager.cpp).

/**
 * @brief Simple in-memory key-value store for testing.
 *
 * put() / get() / remove() all operate on a std::map, making it easy to
 * inspect what the migration wrote.
 */
class InMemoryStorage final : public IMigrationStorage {
public:
    std::map<std::string, std::string> store;

    bool put(const std::string& key, const std::string& value) override
    {
        store[key] = value;
        return true;
    }

    bool get(const std::string& key, std::string& value) override
    {
        auto it = store.find(key);
        if (it == store.end()) {
          return false;
        }
        value = it->second;
        return true;
    }

    bool remove(const std::string& key) override
    {
        store.erase(key);
        return true;
    }

    bool listKeys(std::vector<std::string>& out) override
    {
        out.reserve(out.size() + store.size());
        for (const auto& kv : store) {
            out.push_back(kv.first);
        }
        return true;
    }

    bool has(const std::string& key) const
    {
        return store.count(key) > 0;
    }

    std::string get_or(const std::string& key,
                       const std::string& default_val = "") const
    {
        auto it = store.find(key);
        return it != store.end() ? it->second : default_val;
    }
};

/**
 * @brief Storage that always fails put() — used to test failure paths.
 */
class FailingStorage final : public IMigrationStorage {
public:
    bool put(const std::string&, const std::string&) override { return false; }
    bool get(const std::string&, std::string&) override { return false; }
    bool remove(const std::string&) override { return true; }
};

}  // anonymous namespace

// ============================================================================
// Test suite: AutomaticSchemaMigrationFocusedTests
// ============================================================================

// ----------------------------------------------------------------------------
// 1. Construction and version / operationCount
// ----------------------------------------------------------------------------

TEST(AutomaticSchemaMigrationFocusedTests, ConstructionSetsVersion)
{
    SchemaMigration m("1.5.0");
    EXPECT_EQ(m.version(), "1.5.0");
}

TEST(AutomaticSchemaMigrationFocusedTests, InitialOperationCountIsZero)
{
    SchemaMigration m("1.5.0");
    EXPECT_EQ(m.operationCount(), 0u);
}

TEST(AutomaticSchemaMigrationFocusedTests, InitialPhaseIsIdle)
{
    SchemaMigration m("1.5.0");
    EXPECT_EQ(m.currentPhase(), OnlineDDLPhase::IDLE);
}

// ----------------------------------------------------------------------------
// 2. Fluent builder returns *this
// ----------------------------------------------------------------------------

TEST(AutomaticSchemaMigrationFocusedTests, FluentBuilderChainsAddColumn)
{
    SchemaMigration m("1.5.0");
    SchemaMigration& ref =
        m.addColumn("users", {.name = "phone", .type = "VARCHAR(20)"});
    EXPECT_EQ(&ref, &m);
    EXPECT_EQ(m.operationCount(), 1u);
}

TEST(AutomaticSchemaMigrationFocusedTests, FluentBuilderChainsMultipleOps)
{
    SchemaMigration m("1.5.0");
    m.addColumn("users", {.name = "phone", .type = "VARCHAR(20)"})
     .renameColumn("users", "email", "email_address")
     .addIndex("users", {.name = "idx_email", .columns = {"email_address"}})
     .dropColumn("users", "old_col");
    EXPECT_EQ(m.operationCount(), 4u);
}

// ----------------------------------------------------------------------------
// 3. addColumn: metadata written, backfill recorded
// ----------------------------------------------------------------------------

TEST(AutomaticSchemaMigrationFocusedTests, AddColumn_MetadataWrittenToStorage)
{
    InMemoryStorage storage;
    SchemaMigration m("1.5.0");
    m.addColumn("users", {.name          = "phone_number",
                          .type          = "VARCHAR(20)",
                          .nullable      = true,
                          .default_value = "NULL"});

    auto result = m.apply(storage);

    EXPECT_TRUE(result.success) << result.error_message;
    EXPECT_TRUE(storage.has("users:__schema__:col:phone_number"));
    const std::string meta = storage.get_or("users:__schema__:col:phone_number");
    EXPECT_NE(meta.find("VARCHAR(20)"), std::string::npos);
    EXPECT_NE(meta.find("nullable"), std::string::npos);
}

TEST(AutomaticSchemaMigrationFocusedTests, AddColumn_RecordedInBackfilledColumns)
{
    InMemoryStorage storage;
    SchemaMigration m("1.5.0");
    m.addColumn("users", {.name = "phone", .type = "VARCHAR(20)"});
    auto result = m.apply(storage);

    EXPECT_TRUE(result.success);
    ASSERT_EQ(result.backfilled_columns.size(), 1u);
    EXPECT_EQ(result.backfilled_columns[0], "phone");
}

TEST(AutomaticSchemaMigrationFocusedTests, AddColumn_NonNullableMetadata)
{
    InMemoryStorage storage;
    SchemaMigration m("2.0.0");
    m.addColumn("orders", {.name = "total", .type = "DECIMAL(10,2)", .nullable = false});
    auto result = m.apply(storage);

    EXPECT_TRUE(result.success);
    const std::string meta = storage.get_or("orders:__schema__:col:total");
    EXPECT_NE(meta.find("not_null"), std::string::npos);
}

TEST(AutomaticSchemaMigrationFocusedTests, AddColumn_EmptyTableName_Fails)
{
    InMemoryStorage storage;
    SchemaMigration m("1.5.0");
    m.addColumn("", {.name = "col", .type = "INT"});
    auto result = m.apply(storage);
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

TEST(AutomaticSchemaMigrationFocusedTests, AddColumn_EmptyColumnName_Fails)
{
    InMemoryStorage storage;
    SchemaMigration m("1.5.0");
    m.addColumn("users", {.name = "", .type = "INT"});
    auto result = m.apply(storage);
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

// ----------------------------------------------------------------------------
// 4. renameColumn: metadata renamed, dual-write marker written
// ----------------------------------------------------------------------------

TEST(AutomaticSchemaMigrationFocusedTests, RenameColumn_NewMetadataKeyPresent)
{
    InMemoryStorage storage;
    // Pre-populate old column metadata.
    storage.put("users:__schema__:col:email", "VARCHAR(255)|nullable|");

    SchemaMigration m("1.5.0");
    m.renameColumn("users", "email", "email_address");
    auto result = m.apply(storage);

    EXPECT_TRUE(result.success) << result.error_message;
    EXPECT_TRUE(storage.has("users:__schema__:col:email_address"));
    EXPECT_FALSE(storage.has("users:__schema__:col:email"));
}

TEST(AutomaticSchemaMigrationFocusedTests, RenameColumn_DualWriteMarkerWritten)
{
    InMemoryStorage storage;
    SchemaMigration m("1.5.0");
    m.renameColumn("users", "email", "email_address");
    auto result = m.apply(storage);

    EXPECT_TRUE(result.success);
    // A rename marker must exist so dual-write can translate writes.
    EXPECT_TRUE(storage.has("users:__schema__:rename:email"));
    EXPECT_EQ(storage.get_or("users:__schema__:rename:email"), "email_address");
}

TEST(AutomaticSchemaMigrationFocusedTests, RenameColumn_EmptyOldName_Fails)
{
    InMemoryStorage storage;
    SchemaMigration m("1.5.0");
    m.renameColumn("users", "", "email_address");
    auto result = m.apply(storage);
    EXPECT_FALSE(result.success);
}

TEST(AutomaticSchemaMigrationFocusedTests, RenameColumn_EmptyNewName_Fails)
{
    InMemoryStorage storage;
    SchemaMigration m("1.5.0");
    m.renameColumn("users", "email", "");
    auto result = m.apply(storage);
    EXPECT_FALSE(result.success);
}

// ----------------------------------------------------------------------------
// 5. addIndex: metadata written, online/blocking flag
// ----------------------------------------------------------------------------

TEST(AutomaticSchemaMigrationFocusedTests, AddIndex_MetadataWrittenToStorage)
{
    InMemoryStorage storage;
    SchemaMigration m("1.5.0");
    m.addIndex("users", {.name         = "idx_email",
                         .columns      = {"email_address"},
                         .unique       = false,
                         .build_online = true});
    auto result = m.apply(storage);

    EXPECT_TRUE(result.success) << result.error_message;
    EXPECT_TRUE(storage.has("users:__schema__:idx:idx_email"));
    const std::string meta = storage.get_or("users:__schema__:idx:idx_email");
    EXPECT_NE(meta.find("email_address"), std::string::npos);
    EXPECT_NE(meta.find("online"), std::string::npos);
    EXPECT_NE(meta.find("non_unique"), std::string::npos);
}

TEST(AutomaticSchemaMigrationFocusedTests, AddIndex_UniqueFlag)
{
    InMemoryStorage storage;
    SchemaMigration m("1.5.0");
    m.addIndex("users", {.name = "idx_uniq_email", .columns = {"email"}, .unique = true});
    auto result = m.apply(storage);

    EXPECT_TRUE(result.success);
    const std::string meta = storage.get_or("users:__schema__:idx:idx_uniq_email");
    EXPECT_NE(meta.find("unique"), std::string::npos);
}

TEST(AutomaticSchemaMigrationFocusedTests, AddIndex_OnlineBuildRecordedInResult)
{
    InMemoryStorage storage;
    SchemaMigration m("1.5.0");
    m.addIndex("users", {.name = "idx_e", .columns = {"e"}, .build_online = true});
    auto result = m.apply(storage);

    EXPECT_TRUE(result.success);
    ASSERT_EQ(result.indexes_built_online.size(), 1u);
    EXPECT_EQ(result.indexes_built_online[0], "idx_e");
}

TEST(AutomaticSchemaMigrationFocusedTests, AddIndex_BlockingBuildNotInOnlineList)
{
    InMemoryStorage storage;
    SchemaMigration m("1.5.0");
    m.addIndex("users", {.name = "idx_e", .columns = {"e"}, .build_online = false});
    auto result = m.apply(storage);

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.indexes_built_online.empty());
}

TEST(AutomaticSchemaMigrationFocusedTests, AddIndex_NoColumns_Fails)
{
    InMemoryStorage storage;
    SchemaMigration m("1.5.0");
    m.addIndex("users", {.name = "idx_bad", .columns = {}});
    auto result = m.apply(storage);
    EXPECT_FALSE(result.success);
}

TEST(AutomaticSchemaMigrationFocusedTests, AddIndex_EmptyName_Fails)
{
    InMemoryStorage storage;
    SchemaMigration m("1.5.0");
    m.addIndex("users", {.name = "", .columns = {"email"}});
    auto result = m.apply(storage);
    EXPECT_FALSE(result.success);
}

// ----------------------------------------------------------------------------
// 6. dropColumn: drop marker written, grace period recorded
// ----------------------------------------------------------------------------

TEST(AutomaticSchemaMigrationFocusedTests, DropColumn_MarkerWrittenToStorage)
{
    InMemoryStorage storage;
    storage.put("users:__schema__:col:old_col", "INT|nullable|");

    SchemaMigration m("1.5.0");
    m.dropColumn("users", "old_col");
    auto result = m.apply(storage);

    EXPECT_TRUE(result.success) << result.error_message;
    EXPECT_TRUE(storage.has("users:__schema__:dropped:old_col"));
    // Column metadata removed (column is hidden).
    EXPECT_FALSE(storage.has("users:__schema__:col:old_col"));
}

TEST(AutomaticSchemaMigrationFocusedTests, DropColumn_GracePeriodStoredInMarker)
{
    InMemoryStorage storage;
    SchemaMigration m("1.5.0");
    m.dropColumn("users", "old_col",
                 {.grace_period = std::chrono::hours(24 * 7)});
    auto result = m.apply(storage);

    EXPECT_TRUE(result.success);
    const std::string marker =
        storage.get_or("users:__schema__:dropped:old_col");
    EXPECT_NE(marker.find("grace_hours:168"), std::string::npos);
}

TEST(AutomaticSchemaMigrationFocusedTests, DropColumn_ZeroGracePeriod)
{
    InMemoryStorage storage;
    SchemaMigration m("1.5.0");
    m.dropColumn("users", "col", {.grace_period = std::chrono::hours(0)});
    auto result = m.apply(storage);

    EXPECT_TRUE(result.success);
    const std::string marker = storage.get_or("users:__schema__:dropped:col");
    EXPECT_NE(marker.find("grace_hours:0"), std::string::npos);
}

// ----------------------------------------------------------------------------
// 7. Custom migration callback
// ----------------------------------------------------------------------------

TEST(AutomaticSchemaMigrationFocusedTests, CustomMigration_CallbackInvoked)
{
    InMemoryStorage storage;
    bool callback_invoked = false;

    SchemaMigration m("1.5.0");
    m.addCustomMigration([&callback_invoked](MigrationContext& /*ctx*/) {
        callback_invoked = true;
        return true;
    });
    auto result = m.apply(storage);

    EXPECT_TRUE(result.success) << result.error_message;
    EXPECT_TRUE(callback_invoked);
}

TEST(AutomaticSchemaMigrationFocusedTests, CustomMigration_CanWriteToStorage)
{
    InMemoryStorage storage;

    SchemaMigration m("1.5.0");
    m.addCustomMigration([](MigrationContext& ctx) {
        ctx.storage->put("custom:flag", "applied");
        return true;
    });
    auto result = m.apply(storage);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(storage.get_or("custom:flag"), "applied");
}

TEST(AutomaticSchemaMigrationFocusedTests, CustomMigration_FalseReturnFails)
{
    InMemoryStorage storage;

    SchemaMigration m("1.5.0");
    m.addCustomMigration([](MigrationContext& /*ctx*/) { return false; });
    auto result = m.apply(storage);

    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

TEST(AutomaticSchemaMigrationFocusedTests, CustomMigration_ContextHasCorrectVersion)
{
    InMemoryStorage storage;
    std::string captured_version = {};

    SchemaMigration m("2.3.1");
    m.addCustomMigration([&captured_version](MigrationContext& ctx) {
        captured_version = ctx.version;
        return true;
    });
    m.apply(storage);

    EXPECT_EQ(captured_version, "2.3.1");
}

// ----------------------------------------------------------------------------
// 8. Automatic rollback on failure
// ----------------------------------------------------------------------------

TEST(AutomaticSchemaMigrationFocusedTests, AutoRollback_RestoresStorageOnFailure)
{
    InMemoryStorage storage;
    storage.put("users:__schema__:col:existing", "TEXT|nullable|");

    SchemaMigration m("1.5.0");
    m.setRollbackStrategy(RollbackStrategy::AUTOMATIC);
    // First op succeeds, second op fails (empty column name).
    m.addColumn("users", {.name = "new_col", .type = "INT"});
    m.addColumn("users", {.name = "", .type = "FAIL"});  // will fail

    auto result = m.apply(storage);

    EXPECT_FALSE(result.success);
    // After auto-rollback the new_col metadata must be gone.
    EXPECT_FALSE(storage.has("users:__schema__:col:new_col"));
    // Pre-existing key must be intact.
    EXPECT_TRUE(storage.has("users:__schema__:col:existing"));
}

TEST(AutomaticSchemaMigrationFocusedTests, AutoRollback_PhaseBecomesRolledBack)
{
    InMemoryStorage storage;

    SchemaMigration m("1.5.0");
    m.addColumn("users", {.name = "", .type = "FAIL"});  // immediate failure
    m.apply(storage);

    EXPECT_EQ(m.currentPhase(), OnlineDDLPhase::ROLLED_BACK);
}

// ----------------------------------------------------------------------------
// 9. Manual rollback
// ----------------------------------------------------------------------------

TEST(AutomaticSchemaMigrationFocusedTests, ManualRollback_ExplicitCallbackUndoes)
{
    InMemoryStorage storage;

    SchemaMigration m("1.5.0");
    m.setRollbackStrategy(RollbackStrategy::MANUAL);
    m.addColumn("users", {.name = "temp_col", .type = "INT"});
    m.addColumn("users", {.name = "", .type = "FAIL"});  // will fail

    auto apply_result = m.apply(storage);
    EXPECT_FALSE(apply_result.success);
    // With MANUAL strategy the temp_col key IS present (not auto-rolled back).
    EXPECT_TRUE(storage.has("users:__schema__:col:temp_col"));

    // Explicit rollback.
    auto rb_result = m.rollback();
    EXPECT_TRUE(rb_result.success) << rb_result.error_message;
    EXPECT_FALSE(storage.has("users:__schema__:col:temp_col"));
}

TEST(AutomaticSchemaMigrationFocusedTests, ManualRollback_IdleStateIsSuccessful)
{
    SchemaMigration m("1.5.0");
    // rollback() before any apply() must succeed immediately.
    auto rb = m.rollback();
    EXPECT_TRUE(rb.success);
}

// ----------------------------------------------------------------------------
// 10. Phase progression for successful migration
// ----------------------------------------------------------------------------

TEST(AutomaticSchemaMigrationFocusedTests, SuccessfulMigration_PhaseIsCleanup)
{
    InMemoryStorage storage;
    SchemaMigration m("1.5.0");
    m.addColumn("users", {.name = "col", .type = "TEXT"});
    auto result = m.apply(storage);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(m.currentPhase(), OnlineDDLPhase::CLEANUP);
}

TEST(AutomaticSchemaMigrationFocusedTests, SuccessfulMigration_ResultPhaseIsCleanup)
{
    InMemoryStorage storage;
    SchemaMigration m("1.5.0");
    m.addColumn("users", {.name = "col", .type = "TEXT"});
    auto result = m.apply(storage);

    EXPECT_EQ(result.phase_reached, OnlineDDLPhase::CLEANUP);
}

// ----------------------------------------------------------------------------
// 11. Multiple operations applied in order
// ----------------------------------------------------------------------------

TEST(AutomaticSchemaMigrationFocusedTests, MultipleOps_AllAppliedInOrder)
{
    InMemoryStorage storage;
    SchemaMigration m("1.5.0");
    m.addColumn("users", {.name = "phone", .type = "VARCHAR(20)"})
     .renameColumn("users", "email", "email_address")
     .addIndex("users", {.name = "idx_email", .columns = {"email_address"},
                         .build_online = true})
     .dropColumn("users", "legacy_col");

    auto result = m.apply(storage);

    EXPECT_TRUE(result.success) << result.error_message;
    EXPECT_TRUE(storage.has("users:__schema__:col:phone"));
    EXPECT_TRUE(storage.has("users:__schema__:col:email_address"));
    EXPECT_TRUE(storage.has("users:__schema__:idx:idx_email"));
    EXPECT_TRUE(storage.has("users:__schema__:dropped:legacy_col"));
}

TEST(AutomaticSchemaMigrationFocusedTests, MultipleOps_BackfilledColumnsAllRecorded)
{
    InMemoryStorage storage;
    SchemaMigration m("1.5.0");
    m.addColumn("tbl", {.name = "c1", .type = "INT"})
     .addColumn("tbl", {.name = "c2", .type = "TEXT"})
     .addColumn("tbl", {.name = "c3", .type = "BOOL"});
    auto result = m.apply(storage);

    EXPECT_TRUE(result.success);
    ASSERT_EQ(result.backfilled_columns.size(), 3u);
    EXPECT_EQ(result.backfilled_columns[0], "c1");
    EXPECT_EQ(result.backfilled_columns[1], "c2");
    EXPECT_EQ(result.backfilled_columns[2], "c3");
}

// ----------------------------------------------------------------------------
// 12. Version recorded in result
// ----------------------------------------------------------------------------

TEST(AutomaticSchemaMigrationFocusedTests, MigrationResult_VersionMatchesMigration)
{
    InMemoryStorage storage;
    SchemaMigration m("3.1.4");
    m.addColumn("t", {.name = "x", .type = "INT"});
    auto result = m.apply(storage);

    EXPECT_EQ(result.version, "3.1.4");
}

// ----------------------------------------------------------------------------
// 13. Empty migration (no operations) succeeds
// ----------------------------------------------------------------------------

TEST(AutomaticSchemaMigrationFocusedTests, EmptyMigration_Succeeds)
{
    InMemoryStorage storage;
    SchemaMigration m("1.5.0");
    // No operations registered.
    auto result = m.apply(storage);
    EXPECT_TRUE(result.success) << result.error_message;
}

// ----------------------------------------------------------------------------
// 14. setRollbackStrategy does not affect successful migrations
// ----------------------------------------------------------------------------

TEST(AutomaticSchemaMigrationFocusedTests, SetRollbackStrategy_DoesNotBreakSuccess)
{
    InMemoryStorage storage;
    SchemaMigration m("1.5.0");
    m.setRollbackStrategy(RollbackStrategy::MANUAL);
    m.addColumn("users", {.name = "col", .type = "INT"});
    auto result = m.apply(storage);
    EXPECT_TRUE(result.success);
}

// ----------------------------------------------------------------------------
// 15. addIndex with multiple columns
// ----------------------------------------------------------------------------

TEST(AutomaticSchemaMigrationFocusedTests, AddIndex_MultiColumnIndex)
{
    InMemoryStorage storage;
    SchemaMigration m("1.5.0");
    m.addIndex("orders", {.name    = "idx_composite",
                          .columns = {"customer_id", "created_at"},
                          .unique  = false});
    auto result = m.apply(storage);

    EXPECT_TRUE(result.success);
    const std::string meta = storage.get_or("orders:__schema__:idx:idx_composite");
    EXPECT_NE(meta.find("customer_id"), std::string::npos);
    EXPECT_NE(meta.find("created_at"),  std::string::npos);
}
