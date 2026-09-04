// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include <gtest/gtest.h>
#include <filesystem>
#include <chrono>

#include "metadata/schema_constraints.h"
#include "storage/rocksdb_wrapper.h"

using namespace themis;

static std::string makeTempDbPath(const std::string& prefix) {
    namespace fs = std::filesystem;
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() / (prefix + std::to_string(now))).string();
}

class SchemaConstraintsPersistenceTest : public ::testing::Test {
protected:
    void SetUp() override {
        RocksDBWrapper::Config cfg;
        cfg.db_path       = makeTempDbPath("test_sc_persist_");
        cfg.enable_blobdb = false;

        db_ = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open()) << "Failed to open test database";
    }

    void TearDown() override {
        if (db_) {
          db_->close();
        }
    }

    std::unique_ptr<RocksDBWrapper> db_;
};

// ============================================================================
// persistTo / loadFrom
// ============================================================================

TEST_F(SchemaConstraintsPersistenceTest, PersistAndReloadAllTables) {
    SchemaConstraints sc;
    sc.addConstraint("users",  "email", ColumnConstraint::makeNotNull("nn_email"));
    sc.addConstraint("orders", "amount",
        ColumnConstraint::makeCheck("chk_amount", "amount >= 0"));

    ASSERT_TRUE(sc.persistTo(*db_));

    SchemaConstraints sc2;
    size_t loaded = sc2.loadFrom(*db_);
    EXPECT_GE(loaded, 2u);

    auto user_cs = sc2.getColumnConstraints("users", "email");
    ASSERT_EQ(user_cs.size(), 1u);
    EXPECT_EQ(user_cs[0].kind, ColumnConstraint::Kind::NOT_NULL);

    auto order_cs = sc2.getColumnConstraints("orders", "amount");
    ASSERT_EQ(order_cs.size(), 1u);
    EXPECT_EQ(order_cs[0].kind, ColumnConstraint::Kind::CHECK);
    ASSERT_TRUE(order_cs[0].check_expr.has_value());
    EXPECT_EQ(*order_cs[0].check_expr, "amount >= 0");
}

TEST_F(SchemaConstraintsPersistenceTest, PersistTableAndReload) {
    SchemaConstraints sc;
    sc.addConstraint("products", "price",
        ColumnConstraint::makeCheck("chk_price", "price > 0"));
    sc.addConstraint("products", "name",
        ColumnConstraint::makeNotNull("nn_name"));

    ASSERT_TRUE(sc.persistTableTo(*db_, "products"));

    SchemaConstraints sc2;
    ASSERT_TRUE(sc2.loadTableFrom(*db_, "products"));

    EXPECT_EQ(sc2.getColumnConstraints("products", "price").size(), 1u);
    EXPECT_EQ(sc2.getColumnConstraints("products", "name").size(), 1u);
}

TEST_F(SchemaConstraintsPersistenceTest, LoadTableNotFound) {
    SchemaConstraints sc;
    bool loaded = sc.loadTableFrom(*db_, "nonexistent");
    EXPECT_FALSE(loaded);
}

TEST_F(SchemaConstraintsPersistenceTest, LoadFromEmptyDb) {
    SchemaConstraints sc;
    size_t loaded = sc.loadFrom(*db_);
    EXPECT_EQ(loaded, 0u);
}

TEST_F(SchemaConstraintsPersistenceTest, PersistEmptyConstraints) {
    SchemaConstraints sc;  // No constraints
    // persistTo on empty should succeed without error
    EXPECT_TRUE(sc.persistTo(*db_));
}

TEST_F(SchemaConstraintsPersistenceTest, PersistAndReloadDefaultValue) {
    SchemaConstraints sc;
    sc.addConstraint("docs", "status",
        ColumnConstraint::makeDefault("df_status",
                                      ColumnValue{std::string("draft")}));

    ASSERT_TRUE(sc.persistTo(*db_));

    SchemaConstraints sc2;
    sc2.loadFrom(*db_);

    auto cs = sc2.getColumnConstraints("docs", "status");
    ASSERT_EQ(cs.size(), 1u);
    EXPECT_EQ(cs[0].kind, ColumnConstraint::Kind::DEFAULT);
    ASSERT_TRUE(cs[0].default_value.has_value());
    EXPECT_EQ(std::get<std::string>(*cs[0].default_value), "draft");
}

TEST_F(SchemaConstraintsPersistenceTest, PersistAndReloadForeignKey) {
    SchemaConstraints sc;
    sc.addConstraint("orders", "product_id",
        ColumnConstraint::makeForeignKey("fk_product", "products", "id"));

    ASSERT_TRUE(sc.persistTo(*db_));

    SchemaConstraints sc2;
    sc2.loadFrom(*db_);

    auto cs = sc2.getColumnConstraints("orders", "product_id");
    ASSERT_EQ(cs.size(), 1u);
    EXPECT_EQ(cs[0].kind, ColumnConstraint::Kind::FOREIGN_KEY);
    ASSERT_TRUE(cs[0].fk_table.has_value());
    EXPECT_EQ(*cs[0].fk_table, "products");
    ASSERT_TRUE(cs[0].fk_column.has_value());
    EXPECT_EQ(*cs[0].fk_column, "id");
}

TEST_F(SchemaConstraintsPersistenceTest, PersistReloadAndEnforce) {
    SchemaConstraints sc;
    sc.addConstraint("users", "age",
        ColumnConstraint::makeCheck("chk_age", "age >= 0"));

    ASSERT_TRUE(sc.persistTo(*db_));

    SchemaConstraints sc2;
    sc2.loadFrom(*db_);

    // Valid row
    std::map<std::string, ColumnValue> good = {{"age", int64_t(25)}};
    EXPECT_TRUE(sc2.enforce("users", good).empty());

    // Invalid row
    std::map<std::string, ColumnValue> bad = {{"age", int64_t(-1)}};
    EXPECT_EQ(sc2.enforce("users", bad).size(), 1u);
}
