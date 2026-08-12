/**
 * @file schema_migration_tester.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 87/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "updates/schema_migration_tester.h"
#include "utils/logger.h"

#define LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define LOG_INFO(...)  SPDLOG_INFO(__VA_ARGS__)
#define LOG_WARN(...)  SPDLOG_WARN(__VA_ARGS__)

#include <chrono>
#include <filesystem>
#include <set>
#include <sstream>

namespace themis {
namespace updates {

namespace fs = std::filesystem;

// ============================================================================
// MigrationTestResult helpers
// ============================================================================

size_t MigrationTestResult::passedCount() const {
    size_t n = 0;
    for (const auto& r : test_results)
        if (r.passed) ++n;
    return n;
}

size_t MigrationTestResult::failedCount() const {
    return test_results.size() - passedCount();
}

// ============================================================================
// SchemaMigrationTester
// ============================================================================

SchemaMigrationTester::SchemaMigrationTester()
    : SchemaMigrationTester(Config{}) {}

SchemaMigrationTester::SchemaMigrationTester(const Config& config)
    : config_(config) {}

void SchemaMigrationTester::addTestCase(MigrationTestCase tc) {
    user_test_cases_.push_back(std::move(tc));
}

std::string SchemaMigrationTester::makeStagingPath() const {
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::ostringstream oss;
    oss << config_.staging_directory << "/staging_" << now;
    return oss.str();
}

// ----------------------------------------------------------------------------
// testMigration
// ----------------------------------------------------------------------------

MigrationTestResult SchemaMigrationTester::testMigration(
    const std::string& table_name,
    const SchemaManager::TableSchema& from_schema,
    const SchemaManager::TableSchema& to_schema
) {
    MigrationTestResult result;
    result.staging_db_path = makeStagingPath();

    // --- Create staging directory ---
    try {
        fs::create_directories(result.staging_db_path);
    } catch (const std::exception& ex) {
        result.error_message = std::string("Failed to create staging directory: ") + ex.what();
        LOG_ERROR("{}", result.error_message);
        return result;
    }

    // --- Open staging RocksDB ---
    RocksDBWrapper::Config db_cfg;
    db_cfg.db_path       = result.staging_db_path + "/db";
    db_cfg.memtable_size_mb     = 16;   // small footprint for staging
    db_cfg.block_cache_size_mb  = 32;

    auto staging_db = std::make_unique<RocksDBWrapper>(db_cfg);
    if (!staging_db->open()) {
        result.error_message = "Failed to open staging RocksDB at: " + db_cfg.db_path;
        LOG_ERROR("{}", result.error_message);
        if (config_.cleanup_staging_on_failure)
            fs::remove_all(result.staging_db_path);
        return result;
    }

    // --- Construct staging schema & version managers ---
    auto staging_idx     = std::make_unique<SecondaryIndexManager>(*staging_db);
    auto staged_schema   = std::make_unique<SchemaManager>(*staging_db, staging_idx.get());
    auto staged_version  = std::make_unique<SchemaVersionManager>(*staging_db, *staged_schema);

    // --- Seed the staging DB with from_schema so we have a baseline version ---
    if (!from_schema.name.empty()) {
        staged_schema->setTableSchema(table_name, from_schema);
        staged_version->createSchemaVersion(table_name, "staging-seed", "baseline");
    }

    // --- Run built-in validations ---
    bool builtins_ok = runBuiltinTests(
        table_name, from_schema, to_schema,
        *staged_schema, *staged_version,
        result.test_results, result.migration_script
    );

    if (!builtins_ok) {
        result.error_message = "Built-in validation failed: " +
            (result.test_results.empty() ? "unknown" : result.test_results.back().error_message);
        staging_db->close();
        if (config_.cleanup_staging_on_failure)
            fs::remove_all(result.staging_db_path);
        return result;
    }

    // --- Run user-supplied test cases ---
    for (const auto& tc : user_test_cases_) {
        MigrationTestCaseResult tr;
        tr.name = tc.name;
        try {
            tr.passed = tc.test_fn(*staged_schema, *staged_version, tr.error_message);
        } catch (const std::exception& ex) {
            tr.passed        = false;
            tr.error_message = std::string("Exception: ") + ex.what();
        }
        result.test_results.push_back(tr);
        if (!tr.passed) {
            result.error_message = "User test case '" + tc.name + "' failed: " + tr.error_message;
            staging_db->close();
            if (config_.cleanup_staging_on_failure)
                fs::remove_all(result.staging_db_path);
            return result;
        }
    }

    staging_db->close();

    // Cleanup on success
    if (config_.cleanup_staging_on_success) {
        fs::remove_all(result.staging_db_path);
        result.staging_db_path.clear();
    }

    result.success = true;
    LOG_INFO("Schema migration staging tests passed for table '{}' ({} tests)",
             table_name, result.test_results.size());
    return result;
}

// ----------------------------------------------------------------------------
// runBuiltinTests
// ----------------------------------------------------------------------------

bool SchemaMigrationTester::runBuiltinTests(
    const std::string& table_name,
    const SchemaManager::TableSchema& from_schema,
    const SchemaManager::TableSchema& to_schema,
    SchemaManager& staged_schema,
    SchemaVersionManager& staged_version,
    std::vector<MigrationTestCaseResult>& results,
    std::string& migration_script_out
) {
    auto addResult = [&](const std::string& name, bool passed, const std::string& err = "") {
        MigrationTestCaseResult r;
        r.name          = name;
        r.passed        = passed;
        r.error_message = err;
        results.push_back(r);
        return passed;
    };

    // 1. No duplicate column names in to_schema (early, clear error before validateMigration)
    {
        std::set<std::string> seen;
        std::string dup;
        for (const auto& p : to_schema.properties) {
            if (!seen.insert(p.name).second) { dup = p.name; break; }
        }
        if (!dup.empty())
            return addResult("no_duplicate_columns", false, "Duplicate column: '" + dup + "'");
        addResult("no_duplicate_columns", true);
    }

    // 2. validateMigration() against the baseline (from_schema) before applying
    auto val_result = staged_version.validateMigration(table_name, to_schema);
    if (!val_result.ok)
        return addResult("validate_migration", false, val_result.error_message);
    addResult("validate_migration", true);

    // 3. Register the target schema in the staging SchemaManager
    if (!staged_schema.setTableSchema(table_name, to_schema))
        return addResult("staging_schema_apply", false, "setTableSchema failed for '" + table_name + "'");
    addResult("staging_schema_apply", true);

    // 4. Create a version snapshot of the target schema
    auto ver_result = staged_version.createSchemaVersion(table_name, "staging-tester", "migration-under-test");
    if (!ver_result.ok)
        return addResult("staging_version_snapshot", false, ver_result.error_message);
    addResult("staging_version_snapshot", true);

    // 5. generateMigrationScript() produces a non-empty result when schemas differ
    if (!from_schema.name.empty()) {
        uint64_t from_ver = 1;   // baseline seeded in testMigration()
        uint64_t to_ver   = ver_result.value;
        auto script_result = staged_version.generateMigrationScript(table_name, from_ver, to_ver);
        if (!script_result.ok)
            return addResult("generate_migration_script", false, script_result.error_message);
        migration_script_out = script_result.value;
        addResult("generate_migration_script", true);
    } else {
        addResult("generate_migration_script", true);  // skipped (no baseline)
    }

    return true;
}

// ----------------------------------------------------------------------------
// promoteToProduction
// ----------------------------------------------------------------------------

bool SchemaMigrationTester::promoteToProduction(
    const MigrationTestResult& result,
    RocksDBWrapper& /*production_db*/,
    SchemaManager& production_schema,
    SchemaVersionManager& production_version,
    const std::string& table_name,
    const SchemaManager::TableSchema& to_schema,
    const std::string& author
) {
    if (!result.success) {
        LOG_ERROR("promoteToProduction called with a failed staging result – aborting");
        return false;
    }

    if (!production_schema.setTableSchema(table_name, to_schema)) {
        LOG_ERROR("Failed to apply schema to production for table '{}'", table_name);
        return false;
    }

    auto ver = production_version.createSchemaVersion(table_name, author, "promoted from staging");
    if (!ver.ok) {
        LOG_ERROR("Failed to create production version snapshot: {}", ver.error_message);
        return false;
    }

    LOG_INFO("Schema migration promoted to production: table='{}', version={}, author='{}'",
             table_name, ver.value, author);
    return true;
}

} // namespace updates
} // namespace themis
