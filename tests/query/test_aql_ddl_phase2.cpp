/**
 * @file test_aql_ddl_phase2.cpp
 * @brief AQL DDL Phase 2 test suite — parser and executor (DDL-01..DDL-32).
 *
 * Validates:
 *   - AQLParser::parseSchemaDDL() for all seven DDL forms.
 *   - DDLExecutor::execute() semantic enforcement (duplicates, existence, etc.).
 *   - SchemaRegistry thread-safety under concurrent DDL.
 *   - End-to-end round-trip: parse → execute → query registry.
 *
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Status: Production Ready
 */


#include <gtest/gtest.h>

#include "query/aql_parser.h"
#include "query/ddl_executor.h"

#include <algorithm>
#include <thread>
#include <vector>

using namespace themis::query;

// ============================================================================
// Fixture
// ============================================================================

class AqlDdlPhase2Test : public ::testing::Test {
protected:
    AQLParser     parser;
    SchemaRegistry registry;

    // Helper: parse + assert success
    SchemaDDL parse_ok(const std::string& stmt) {
        auto result = parser.parseSchemaDDL(stmt);
        EXPECT_TRUE(result.has_value())
            << "parseSchemaDDL failed for: " << stmt
            << "\n  error: " << (result ? "" : result.error().message());
        return result.value_or(SchemaDDL{});
    }

    // Helper: parse + assert failure
    void parse_err(const std::string& stmt) {
        auto result = parser.parseSchemaDDL(stmt);
        EXPECT_FALSE(result.has_value())
            << "Expected parse error for: " << stmt;
    }

    // Helper: execute + assert success
    bool exec_ok(const SchemaDDL& ddl) {
        DDLExecutor exec(registry);
        auto result = exec.execute(ddl);
        EXPECT_TRUE(result.has_value())
            << "execute() failed: "
            << (result ? "" : result.error().message());
        return result.has_value();
    }

    // Helper: execute + assert failure
    void exec_err(const SchemaDDL& ddl) {
        DDLExecutor exec(registry);
        auto result = exec.execute(ddl);
        EXPECT_FALSE(result.has_value())
            << "Expected execute() error for DDL type: " << ddl.typeString();
    }
};

// ============================================================================
// DDL-01..DDL-12 — Parser tests
// ============================================================================

/// DDL-01 — CREATE COLLECTION users → type=CREATE_COLLECTION, name="users"
TEST_F(AqlDdlPhase2Test, DDL01_ParseCreateCollection) {
    auto ddl = parse_ok("CREATE COLLECTION users");
    EXPECT_EQ(ddl.ddl_type, SchemaDDLType::CREATE_COLLECTION);
    EXPECT_EQ(ddl.name, "users");
    EXPECT_FALSE(ddl.if_exists);
}

/// DDL-02 — CREATE COLLECTION users IF NOT EXISTS → if_exists=true
TEST_F(AqlDdlPhase2Test, DDL02_ParseCreateCollectionIfNotExists) {
    auto ddl = parse_ok("CREATE COLLECTION users IF NOT EXISTS");
    EXPECT_EQ(ddl.ddl_type, SchemaDDLType::CREATE_COLLECTION);
    EXPECT_EQ(ddl.name, "users");
    EXPECT_TRUE(ddl.if_exists);
}

/// DDL-03 — DROP COLLECTION users → type=DROP_COLLECTION
TEST_F(AqlDdlPhase2Test, DDL03_ParseDropCollection) {
    auto ddl = parse_ok("DROP COLLECTION users");
    EXPECT_EQ(ddl.ddl_type, SchemaDDLType::DROP_COLLECTION);
    EXPECT_EQ(ddl.name, "users");
    EXPECT_FALSE(ddl.if_exists);
}

/// DDL-04 — DROP COLLECTION users IF EXISTS → if_exists=true
TEST_F(AqlDdlPhase2Test, DDL04_ParseDropCollectionIfExists) {
    auto ddl = parse_ok("DROP COLLECTION users IF EXISTS");
    EXPECT_EQ(ddl.ddl_type, SchemaDDLType::DROP_COLLECTION);
    EXPECT_EQ(ddl.name, "users");
    EXPECT_TRUE(ddl.if_exists);
}

/// DDL-05 — CREATE INDEX idx_email ON users (email) → CREATE_INDEX, correct fields
TEST_F(AqlDdlPhase2Test, DDL05_ParseCreateIndex) {
    auto ddl = parse_ok("CREATE INDEX idx_email ON users (email)");
    EXPECT_EQ(ddl.ddl_type, SchemaDDLType::CREATE_INDEX);
    EXPECT_EQ(ddl.name, "idx_email");
    EXPECT_EQ(ddl.collection, "users");
    ASSERT_EQ(ddl.index_def.fields.size(), 1u);
    EXPECT_EQ(ddl.index_def.fields[0].name, "email");
    EXPECT_FALSE(ddl.index_def.unique);
}

/// DDL-06 — CREATE UNIQUE INDEX idx_email ON users (email) → unique=true
TEST_F(AqlDdlPhase2Test, DDL06_ParseCreateUniqueIndex) {
    auto ddl = parse_ok("CREATE UNIQUE INDEX idx_email ON users (email)");
    EXPECT_EQ(ddl.ddl_type, SchemaDDLType::CREATE_INDEX);
    EXPECT_EQ(ddl.name, "idx_email");
    EXPECT_EQ(ddl.collection, "users");
    EXPECT_TRUE(ddl.index_def.unique);
    ASSERT_EQ(ddl.index_def.fields.size(), 1u);
    EXPECT_EQ(ddl.index_def.fields[0].name, "email");
}

/// DDL-07 — DROP INDEX idx_email ON users → DROP_INDEX
TEST_F(AqlDdlPhase2Test, DDL07_ParseDropIndex) {
    auto ddl = parse_ok("DROP INDEX idx_email ON users");
    EXPECT_EQ(ddl.ddl_type, SchemaDDLType::DROP_INDEX);
    EXPECT_EQ(ddl.name, "idx_email");
    EXPECT_EQ(ddl.collection, "users");
    EXPECT_FALSE(ddl.if_exists);
}

/// DDL-08 — CREATE VIEW active_users AS FOR u IN users FILTER u.active == true RETURN u
///          → CREATE_VIEW with body
TEST_F(AqlDdlPhase2Test, DDL08_ParseCreateView) {
    const std::string stmt =
        "CREATE VIEW active_users AS FOR u IN users FILTER u.active == true RETURN u";
    auto ddl = parse_ok(stmt);
    EXPECT_EQ(ddl.ddl_type, SchemaDDLType::CREATE_VIEW);
    EXPECT_EQ(ddl.name, "active_users");
    // Body must start with "FOR"
    EXPECT_EQ(ddl.view_body.substr(0, 3), "FOR");
    EXPECT_FALSE(ddl.view_body.empty());
}

/// DDL-09 — DROP VIEW active_users → DROP_VIEW
TEST_F(AqlDdlPhase2Test, DDL09_ParseDropView) {
    auto ddl = parse_ok("DROP VIEW active_users");
    EXPECT_EQ(ddl.ddl_type, SchemaDDLType::DROP_VIEW);
    EXPECT_EQ(ddl.name, "active_users");
    EXPECT_FALSE(ddl.if_exists);
}

/// DDL-10 — ALTER COLLECTION users SET OPTIONS {"waitForSync": true}
///          → ALTER_COLLECTION with options
TEST_F(AqlDdlPhase2Test, DDL10_ParseAlterCollection) {
    auto ddl = parse_ok(R"(ALTER COLLECTION users SET OPTIONS {"waitForSync": true})");
    EXPECT_EQ(ddl.ddl_type, SchemaDDLType::ALTER_COLLECTION);
    EXPECT_EQ(ddl.name, "users");
    ASSERT_TRUE(ddl.options.contains("waitForSync"));
    EXPECT_TRUE(ddl.options["waitForSync"].get<bool>());
}

/// DDL-11 — Case-insensitive: "create collection USERS" parses same as
///          "CREATE COLLECTION USERS"
TEST_F(AqlDdlPhase2Test, DDL11_CaseInsensitiveKeywords) {
    auto ddl_lower = parse_ok("create collection orders");
    auto ddl_mixed = parse_ok("Create Collection orders");
    EXPECT_EQ(ddl_lower.ddl_type, SchemaDDLType::CREATE_COLLECTION);
    EXPECT_EQ(ddl_mixed.ddl_type, SchemaDDLType::CREATE_COLLECTION);
    // Both must parse successfully; type must agree
    EXPECT_EQ(ddl_lower.ddl_type, ddl_mixed.ddl_type);
}

/// DDL-12 — Invalid statement returns an error
TEST_F(AqlDdlPhase2Test, DDL12_InvalidStatementReturnsError) {
    parse_err("");
    parse_err("SELECT * FROM users");
    parse_err("CREATE");
    parse_err("DROP");
    parse_err("ALTER");
}

// ============================================================================
// DDL-13..DDL-22 — Executor tests
// ============================================================================

/// DDL-13 — CREATE COLLECTION → registry has collection
TEST_F(AqlDdlPhase2Test, DDL13_CreateCollectionRegistered) {
    auto ddl = parse_ok("CREATE COLLECTION orders");
    exec_ok(ddl);
    EXPECT_TRUE(registry.hasCollection("orders"));
}

/// DDL-14 — CREATE COLLECTION duplicate → error (without IF NOT EXISTS)
TEST_F(AqlDdlPhase2Test, DDL14_CreateCollectionDuplicateErrors) {
    auto ddl = parse_ok("CREATE COLLECTION products");
    exec_ok(ddl);                          // first time: ok
    exec_err(ddl);                         // second time: must fail
}

/// DDL-15 — CREATE COLLECTION duplicate IF NOT EXISTS → Ok (idempotent)
TEST_F(AqlDdlPhase2Test, DDL15_CreateCollectionIfNotExistsIdempotent) {
    auto ddl = parse_ok("CREATE COLLECTION items IF NOT EXISTS");
    exec_ok(ddl); // first
    exec_ok(ddl); // second — must not error
    EXPECT_TRUE(registry.hasCollection("items"));
}

/// DDL-16 — DROP COLLECTION existing → registry removes collection
TEST_F(AqlDdlPhase2Test, DDL16_DropCollectionRemovesIt) {
    exec_ok(parse_ok("CREATE COLLECTION remove_me"));
    ASSERT_TRUE(registry.hasCollection("remove_me"));
    exec_ok(parse_ok("DROP COLLECTION remove_me"));
    EXPECT_FALSE(registry.hasCollection("remove_me"));
}

/// DDL-17 — DROP COLLECTION non-existent → error (without IF EXISTS)
TEST_F(AqlDdlPhase2Test, DDL17_DropCollectionNonExistentErrors) {
    exec_err(parse_ok("DROP COLLECTION ghost_collection"));
}

/// DDL-18 — DROP COLLECTION non-existent IF EXISTS → Ok (idempotent)
TEST_F(AqlDdlPhase2Test, DDL18_DropCollectionIfExistsIdempotent) {
    exec_ok(parse_ok("DROP COLLECTION ghost_collection IF EXISTS"));
}

/// DDL-19 — CREATE INDEX on existing collection → registry has index
TEST_F(AqlDdlPhase2Test, DDL19_CreateIndexOnExistingCollection) {
    exec_ok(parse_ok("CREATE COLLECTION catalog"));
    exec_ok(parse_ok("CREATE INDEX idx_sku ON catalog (sku)"));
    EXPECT_TRUE(registry.hasIndex("catalog", "idx_sku"));
}

/// DDL-20 — CREATE INDEX on non-existent collection → error
TEST_F(AqlDdlPhase2Test, DDL20_CreateIndexOnMissingCollectionErrors) {
    exec_err(parse_ok("CREATE INDEX idx_x ON no_such_collection (x)"));
}

/// DDL-21 — DROP INDEX existing → removes from registry
TEST_F(AqlDdlPhase2Test, DDL21_DropIndexRemovesIt) {
    exec_ok(parse_ok("CREATE COLLECTION warehouse"));
    exec_ok(parse_ok("CREATE INDEX idx_loc ON warehouse (location)"));
    ASSERT_TRUE(registry.hasIndex("warehouse", "idx_loc"));
    exec_ok(parse_ok("DROP INDEX idx_loc ON warehouse"));
    EXPECT_FALSE(registry.hasIndex("warehouse", "idx_loc"));
}

/// DDL-22 — DROP INDEX non-existent IF EXISTS → Ok
TEST_F(AqlDdlPhase2Test, DDL22_DropIndexIfExistsIdempotent) {
    exec_ok(parse_ok("CREATE COLLECTION base_coll"));
    exec_ok(parse_ok("DROP INDEX no_such_idx ON base_coll IF EXISTS"));
}

// ============================================================================
// DDL-23..DDL-28 — View and ALTER tests
// ============================================================================

/// DDL-23 — CREATE VIEW → registry has view
TEST_F(AqlDdlPhase2Test, DDL23_CreateViewRegistered) {
    const std::string stmt =
        "CREATE VIEW active_users AS FOR u IN users FILTER u.active == true RETURN u";
    exec_ok(parse_ok(stmt));
    EXPECT_TRUE(registry.hasView("active_users"));
}

/// DDL-24 — DROP VIEW existing → removes view
TEST_F(AqlDdlPhase2Test, DDL24_DropViewRemovesIt) {
    exec_ok(parse_ok(
        "CREATE VIEW temp_view AS FOR x IN col RETURN x"));
    ASSERT_TRUE(registry.hasView("temp_view"));
    exec_ok(parse_ok("DROP VIEW temp_view"));
    EXPECT_FALSE(registry.hasView("temp_view"));
}

/// DDL-25 — DROP VIEW non-existent IF EXISTS → Ok
TEST_F(AqlDdlPhase2Test, DDL25_DropViewIfExistsIdempotent) {
    exec_ok(parse_ok("DROP VIEW nonexistent_view IF EXISTS"));
}

/// DDL-26 — ALTER COLLECTION updates options
TEST_F(AqlDdlPhase2Test, DDL26_AlterCollectionUpdatesOptions) {
    exec_ok(parse_ok("CREATE COLLECTION config_coll"));
    exec_ok(parse_ok(R"(ALTER COLLECTION config_coll SET OPTIONS {"replicationFactor": 3})"));
    auto opts = registry.collectionOptions("config_coll");
    ASSERT_TRUE(opts.contains("replicationFactor"));
    EXPECT_EQ(opts["replicationFactor"].get<int>(), 3);
}

/// DDL-27 — ALTER COLLECTION non-existent → error
TEST_F(AqlDdlPhase2Test, DDL27_AlterNonExistentCollectionErrors) {
    exec_err(parse_ok(R"(ALTER COLLECTION no_such_coll SET OPTIONS {"x": 1})"));
}

/// DDL-28 — CREATE VIEW duplicate without IF NOT EXISTS → error
TEST_F(AqlDdlPhase2Test, DDL28_CreateViewDuplicateErrors) {
    const std::string stmt =
        "CREATE VIEW dupe_view AS FOR y IN col RETURN y";
    exec_ok(parse_ok(stmt));
    exec_err(parse_ok(stmt));
}

// ============================================================================
// DDL-29..DDL-32 — Integration tests
// ============================================================================

/// DDL-29 — Full cycle: CREATE COLLECTION → CREATE INDEX → DROP INDEX → DROP COLLECTION
TEST_F(AqlDdlPhase2Test, DDL29_FullCycle) {
    exec_ok(parse_ok("CREATE COLLECTION lifecycle_coll"));
    EXPECT_TRUE(registry.hasCollection("lifecycle_coll"));

    exec_ok(parse_ok("CREATE INDEX idx_lc ON lifecycle_coll (field1, field2)"));
    EXPECT_TRUE(registry.hasIndex("lifecycle_coll", "idx_lc"));

    exec_ok(parse_ok("DROP INDEX idx_lc ON lifecycle_coll"));
    EXPECT_FALSE(registry.hasIndex("lifecycle_coll", "idx_lc"));

    exec_ok(parse_ok("DROP COLLECTION lifecycle_coll"));
    EXPECT_FALSE(registry.hasCollection("lifecycle_coll"));
}

/// DDL-30 — Concurrent CREATE/DROP on different collections (thread-safety)
TEST_F(AqlDdlPhase2Test, DDL30_ConcurrentCreateDropDifferentCollections) {
    constexpr int kThreads = 8;
    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&, i]() {
            std::string cname = "concurrent_coll_" + std::to_string(i);
            DDLExecutor exec(registry);

            auto cddl = parser.parseSchemaDDL("CREATE COLLECTION " + cname);
            if (cddl) exec.execute(*cddl);

            auto dddl = parser.parseSchemaDDL("DROP COLLECTION " + cname + " IF EXISTS");
            if (dddl) exec.execute(*dddl);
        });
    }
    for (auto& t : threads) t.join();

    // All transient collections should be gone; no crashes or corruption
    for (int i = 0; i < kThreads; ++i) {
        std::string cname = "concurrent_coll_" + std::to_string(i);
        EXPECT_FALSE(registry.hasCollection(cname));
    }
}

/// DDL-31 — Parse + Execute round-trip for all 7 DDL types
TEST_F(AqlDdlPhase2Test, DDL31_RoundTripAllSevenTypes) {
    // 1. CREATE COLLECTION
    {
        auto ddl = parse_ok("CREATE COLLECTION rt_coll");
        EXPECT_EQ(ddl.ddl_type, SchemaDDLType::CREATE_COLLECTION);
        exec_ok(ddl);
        EXPECT_TRUE(registry.hasCollection("rt_coll"));
    }
    // 2. CREATE INDEX
    {
        auto ddl = parse_ok("CREATE INDEX rt_idx ON rt_coll (field_a)");
        EXPECT_EQ(ddl.ddl_type, SchemaDDLType::CREATE_INDEX);
        exec_ok(ddl);
        EXPECT_TRUE(registry.hasIndex("rt_coll", "rt_idx"));
    }
    // 3. ALTER COLLECTION
    {
        auto ddl = parse_ok(R"(ALTER COLLECTION rt_coll SET OPTIONS {"journalSize": 1024})");
        EXPECT_EQ(ddl.ddl_type, SchemaDDLType::ALTER_COLLECTION);
        exec_ok(ddl);
        EXPECT_TRUE(registry.collectionOptions("rt_coll").contains("journalSize"));
    }
    // 4. CREATE VIEW
    {
        auto ddl = parse_ok(
            "CREATE VIEW rt_view AS FOR d IN rt_coll RETURN d");
        EXPECT_EQ(ddl.ddl_type, SchemaDDLType::CREATE_VIEW);
        exec_ok(ddl);
        EXPECT_TRUE(registry.hasView("rt_view"));
    }
    // 5. DROP VIEW
    {
        auto ddl = parse_ok("DROP VIEW rt_view");
        EXPECT_EQ(ddl.ddl_type, SchemaDDLType::DROP_VIEW);
        exec_ok(ddl);
        EXPECT_FALSE(registry.hasView("rt_view"));
    }
    // 6. DROP INDEX
    {
        auto ddl = parse_ok("DROP INDEX rt_idx ON rt_coll");
        EXPECT_EQ(ddl.ddl_type, SchemaDDLType::DROP_INDEX);
        exec_ok(ddl);
        EXPECT_FALSE(registry.hasIndex("rt_coll", "rt_idx"));
    }
    // 7. DROP COLLECTION
    {
        auto ddl = parse_ok("DROP COLLECTION rt_coll");
        EXPECT_EQ(ddl.ddl_type, SchemaDDLType::DROP_COLLECTION);
        exec_ok(ddl);
        EXPECT_FALSE(registry.hasCollection("rt_coll"));
    }
}

/// DDL-32 — SchemaRegistry::collections() and views() return correct names
TEST_F(AqlDdlPhase2Test, DDL32_RegistryListsReturnCorrectNames) {
    exec_ok(parse_ok("CREATE COLLECTION alpha"));
    exec_ok(parse_ok("CREATE COLLECTION beta"));
    exec_ok(parse_ok("CREATE COLLECTION gamma"));
    exec_ok(parse_ok("CREATE VIEW view_alpha AS FOR a IN alpha RETURN a"));
    exec_ok(parse_ok("CREATE VIEW view_beta AS FOR b IN beta RETURN b"));

    auto colls = registry.collections();
    EXPECT_EQ(colls.size(), 3u);
    EXPECT_NE(std::find(colls.begin(), colls.end(), "alpha"), colls.end());
    EXPECT_NE(std::find(colls.begin(), colls.end(), "beta"),  colls.end());
    EXPECT_NE(std::find(colls.begin(), colls.end(), "gamma"), colls.end());

    auto vws = registry.views();
    EXPECT_EQ(vws.size(), 2u);
    EXPECT_NE(std::find(vws.begin(), vws.end(), "view_alpha"), vws.end());
    EXPECT_NE(std::find(vws.begin(), vws.end(), "view_beta"),  vws.end());
}
