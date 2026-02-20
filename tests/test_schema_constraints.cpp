// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include <gtest/gtest.h>
#include <map>
#include <string>

#include "metadata/schema_constraints.h"

using namespace themis;

// ============================================================================
// ColumnConstraint factories
// ============================================================================

TEST(ConstraintFactoryTest, MakeNotNull) {
    auto c = ColumnConstraint::makeNotNull("nn_email");
    EXPECT_EQ(c.kind, ColumnConstraint::Kind::NOT_NULL);
    EXPECT_EQ(c.name, "nn_email");
}

TEST(ConstraintFactoryTest, MakeUnique) {
    auto c = ColumnConstraint::makeUnique("uq_email");
    EXPECT_EQ(c.kind, ColumnConstraint::Kind::UNIQUE);
    EXPECT_EQ(c.name, "uq_email");
}

TEST(ConstraintFactoryTest, MakeCheck) {
    auto c = ColumnConstraint::makeCheck("chk_age", "age >= 0");
    EXPECT_EQ(c.kind, ColumnConstraint::Kind::CHECK);
    EXPECT_EQ(c.name, "chk_age");
    ASSERT_TRUE(c.check_expr.has_value());
    EXPECT_EQ(*c.check_expr, "age >= 0");
}

TEST(ConstraintFactoryTest, MakeDefault) {
    auto c = ColumnConstraint::makeDefault("df_active", ColumnValue{true});
    EXPECT_EQ(c.kind, ColumnConstraint::Kind::DEFAULT);
    EXPECT_EQ(c.name, "df_active");
    ASSERT_TRUE(c.default_value.has_value());
    EXPECT_TRUE(std::get<bool>(*c.default_value));
}

TEST(ConstraintFactoryTest, MakeForeignKey) {
    auto c = ColumnConstraint::makeForeignKey("fk_dept", "departments", "id");
    EXPECT_EQ(c.kind, ColumnConstraint::Kind::FOREIGN_KEY);
    ASSERT_TRUE(c.fk_table.has_value());
    EXPECT_EQ(*c.fk_table, "departments");
    ASSERT_TRUE(c.fk_column.has_value());
    EXPECT_EQ(*c.fk_column, "id");
}

// ============================================================================
// Constraint management: add / get / remove
// ============================================================================

class SchemaConstraintsTest : public ::testing::Test {
protected:
    SchemaConstraints sc_;
};

TEST_F(SchemaConstraintsTest, AddAndGetColumnConstraints) {
    sc_.addConstraint("users", "email",
        ColumnConstraint::makeNotNull("nn_users_email"));

    auto cs = sc_.getColumnConstraints("users", "email");
    ASSERT_EQ(cs.size(), 1u);
    EXPECT_EQ(cs[0].kind, ColumnConstraint::Kind::NOT_NULL);
    EXPECT_EQ(cs[0].name, "nn_users_email");
}

TEST_F(SchemaConstraintsTest, AddMultipleConstraintsToColumn) {
    sc_.addConstraint("users", "email",
        ColumnConstraint::makeNotNull("nn_email"));
    sc_.addConstraint("users", "email",
        ColumnConstraint::makeUnique("uq_email"));

    auto cs = sc_.getColumnConstraints("users", "email");
    EXPECT_EQ(cs.size(), 2u);
}

TEST_F(SchemaConstraintsTest, GetTableConstraints) {
    sc_.addConstraint("orders", "id",
        ColumnConstraint::makeNotNull("nn_orders_id"));
    sc_.addConstraint("orders", "amount",
        ColumnConstraint::makeCheck("chk_amount_pos", "amount >= 0"));

    auto all = sc_.getTableConstraints("orders");
    EXPECT_EQ(all.size(), 2u);
}

TEST_F(SchemaConstraintsTest, GetConstraintsMissingTable) {
    auto cs = sc_.getColumnConstraints("nonexistent", "col");
    EXPECT_TRUE(cs.empty());

    auto all = sc_.getTableConstraints("nonexistent");
    EXPECT_TRUE(all.empty());
}

TEST_F(SchemaConstraintsTest, RemoveColumnConstraints) {
    sc_.addConstraint("t", "c", ColumnConstraint::makeNotNull("nn"));
    ASSERT_EQ(sc_.getColumnConstraints("t", "c").size(), 1u);

    sc_.removeColumnConstraints("t", "c");
    EXPECT_TRUE(sc_.getColumnConstraints("t", "c").empty());
}

TEST_F(SchemaConstraintsTest, RemoveTableConstraints) {
    sc_.addConstraint("t", "c1", ColumnConstraint::makeNotNull("nn1"));
    sc_.addConstraint("t", "c2", ColumnConstraint::makeUnique("uq2"));
    ASSERT_EQ(sc_.getTableConstraints("t").size(), 2u);

    sc_.removeTableConstraints("t");
    EXPECT_TRUE(sc_.getTableConstraints("t").empty());
}

// ============================================================================
// Enforcement – NOT NULL
// ============================================================================

TEST_F(SchemaConstraintsTest, EnforceNotNullViolation) {
    sc_.addConstraint("users", "email",
        ColumnConstraint::makeNotNull("nn_email"));

    // Row without email → violation
    std::map<std::string, ColumnValue> row = {{"name", std::string("Alice")}};
    auto violations = sc_.enforce("users", row);

    ASSERT_EQ(violations.size(), 1u);
    EXPECT_EQ(violations[0].constraint_type, "NOT_NULL");
    EXPECT_EQ(violations[0].column_name,     "email");
    EXPECT_FALSE(violations[0].message.empty());
}

TEST_F(SchemaConstraintsTest, EnforceNotNullExplicitNull) {
    sc_.addConstraint("users", "email",
        ColumnConstraint::makeNotNull("nn_email"));

    std::map<std::string, ColumnValue> row = {
        {"email", ColumnValue{std::monostate{}}}   // Explicit NULL
    };
    auto violations = sc_.enforce("users", row);
    ASSERT_EQ(violations.size(), 1u);
    EXPECT_EQ(violations[0].constraint_type, "NOT_NULL");
}

TEST_F(SchemaConstraintsTest, EnforceNotNullOK) {
    sc_.addConstraint("users", "email",
        ColumnConstraint::makeNotNull("nn_email"));

    std::map<std::string, ColumnValue> row = {
        {"email", std::string("a@b.c")}
    };
    auto violations = sc_.enforce("users", row);
    EXPECT_TRUE(violations.empty());
}

// ============================================================================
// Enforcement – CHECK
// ============================================================================

TEST_F(SchemaConstraintsTest, EnforceCheckViolationGreaterThan) {
    sc_.addConstraint("products", "price",
        ColumnConstraint::makeCheck("chk_price", "price > 0"));

    std::map<std::string, ColumnValue> row = {{"price", double(-1.0)}};
    auto violations = sc_.enforce("products", row);

    ASSERT_EQ(violations.size(), 1u);
    EXPECT_EQ(violations[0].constraint_type, "CHECK");
    EXPECT_EQ(violations[0].constraint_name, "chk_price");
}

TEST_F(SchemaConstraintsTest, EnforceCheckPassGreaterThan) {
    sc_.addConstraint("products", "price",
        ColumnConstraint::makeCheck("chk_price", "price > 0"));

    std::map<std::string, ColumnValue> row = {{"price", double(9.99)}};
    auto violations = sc_.enforce("products", row);
    EXPECT_TRUE(violations.empty());
}

TEST_F(SchemaConstraintsTest, EnforceCheckGreaterOrEqual) {
    sc_.addConstraint("items", "qty",
        ColumnConstraint::makeCheck("chk_qty", "qty >= 0"));

    // Zero should be allowed
    std::map<std::string, ColumnValue> row = {{"qty", int64_t(0)}};
    EXPECT_TRUE(sc_.enforce("items", row).empty());

    // Negative should fail
    row["qty"] = int64_t(-1);
    EXPECT_EQ(sc_.enforce("items", row).size(), 1u);
}

TEST_F(SchemaConstraintsTest, EnforceCheckEqual) {
    sc_.addConstraint("settings", "version",
        ColumnConstraint::makeCheck("chk_ver", "version = 1"));

    std::map<std::string, ColumnValue> ok_row  = {{"version", int64_t(1)}};
    std::map<std::string, ColumnValue> bad_row = {{"version", int64_t(2)}};

    EXPECT_TRUE(sc_.enforce("settings", ok_row).empty());
    EXPECT_EQ(sc_.enforce("settings", bad_row).size(), 1u);
}

TEST_F(SchemaConstraintsTest, EnforceMultipleViolations) {
    sc_.addConstraint("orders", "id",
        ColumnConstraint::makeNotNull("nn_id"));
    sc_.addConstraint("orders", "amount",
        ColumnConstraint::makeCheck("chk_amount", "amount >= 0"));

    // Both id (missing) and amount (negative) violate
    std::map<std::string, ColumnValue> row = {{"amount", double(-5.0)}};
    auto violations = sc_.enforce("orders", row);
    EXPECT_EQ(violations.size(), 2u);
}

TEST_F(SchemaConstraintsTest, EnforceNoConstraintsForTable) {
    std::map<std::string, ColumnValue> row = {{"x", int64_t(1)}};
    auto violations = sc_.enforce("unconstrained_table", row);
    EXPECT_TRUE(violations.empty());
}

// ============================================================================
// Default values
// ============================================================================

TEST_F(SchemaConstraintsTest, ApplyDefaultsMissingColumn) {
    sc_.addConstraint("users", "active",
        ColumnConstraint::makeDefault("df_active", ColumnValue{true}));

    std::map<std::string, ColumnValue> row = {{"name", std::string("Bob")}};
    auto filled = sc_.applyDefaults("users", row);

    ASSERT_TRUE(filled.count("active"));
    EXPECT_TRUE(std::get<bool>(filled.at("active")));
}

TEST_F(SchemaConstraintsTest, ApplyDefaultsNullColumn) {
    sc_.addConstraint("users", "role",
        ColumnConstraint::makeDefault("df_role", ColumnValue{std::string("viewer")}));

    std::map<std::string, ColumnValue> row = {
        {"name", std::string("Eve")},
        {"role", ColumnValue{std::monostate{}}}  // explicit NULL
    };
    auto filled = sc_.applyDefaults("users", row);
    EXPECT_EQ(std::get<std::string>(filled.at("role")), "viewer");
}

TEST_F(SchemaConstraintsTest, ApplyDefaultsDoesNotOverwriteExistingValue) {
    sc_.addConstraint("users", "active",
        ColumnConstraint::makeDefault("df_active", ColumnValue{true}));

    std::map<std::string, ColumnValue> row = {{"active", ColumnValue{false}}};
    auto filled = sc_.applyDefaults("users", row);
    // existing value false should be kept
    EXPECT_FALSE(std::get<bool>(filled.at("active")));
}

TEST_F(SchemaConstraintsTest, ApplyDefaultsStringDefault) {
    sc_.addConstraint("docs", "status",
        ColumnConstraint::makeDefault("df_status", ColumnValue{std::string("draft")}));

    std::map<std::string, ColumnValue> row;
    auto filled = sc_.applyDefaults("docs", row);
    ASSERT_TRUE(filled.count("status"));
    EXPECT_EQ(std::get<std::string>(filled.at("status")), "draft");
}

TEST_F(SchemaConstraintsTest, ApplyDefaultsIntegerDefault) {
    sc_.addConstraint("counters", "value",
        ColumnConstraint::makeDefault("df_val", ColumnValue{int64_t(0)}));

    std::map<std::string, ColumnValue> row;
    auto filled = sc_.applyDefaults("counters", row);
    ASSERT_TRUE(filled.count("value"));
    EXPECT_EQ(std::get<int64_t>(filled.at("value")), 0);
}

// ============================================================================
// JSON serialisation round-trip
// ============================================================================

TEST_F(SchemaConstraintsTest, ToJSONAndFromJSON) {
    sc_.addConstraint("users", "email",
        ColumnConstraint::makeNotNull("nn_email"));
    sc_.addConstraint("users", "age",
        ColumnConstraint::makeCheck("chk_age", "age >= 0"));
    sc_.addConstraint("users", "role",
        ColumnConstraint::makeDefault("df_role", ColumnValue{std::string("user")}));
    sc_.addConstraint("orders", "product_id",
        ColumnConstraint::makeForeignKey("fk_product", "products", "id"));

    auto j = sc_.toJSON();
    EXPECT_TRUE(j.contains("users"));
    EXPECT_TRUE(j.contains("orders"));

    auto sc2 = SchemaConstraints::fromJSON(j);
    auto user_email = sc2.getColumnConstraints("users", "email");
    ASSERT_EQ(user_email.size(), 1u);
    EXPECT_EQ(user_email[0].kind, ColumnConstraint::Kind::NOT_NULL);

    auto user_age = sc2.getColumnConstraints("users", "age");
    ASSERT_EQ(user_age.size(), 1u);
    EXPECT_EQ(user_age[0].kind, ColumnConstraint::Kind::CHECK);
    ASSERT_TRUE(user_age[0].check_expr.has_value());
    EXPECT_EQ(*user_age[0].check_expr, "age >= 0");

    auto user_role = sc2.getColumnConstraints("users", "role");
    ASSERT_EQ(user_role.size(), 1u);
    EXPECT_EQ(user_role[0].kind, ColumnConstraint::Kind::DEFAULT);
    ASSERT_TRUE(user_role[0].default_value.has_value());
    EXPECT_EQ(std::get<std::string>(*user_role[0].default_value), "user");

    auto order_fk = sc2.getColumnConstraints("orders", "product_id");
    ASSERT_EQ(order_fk.size(), 1u);
    EXPECT_EQ(order_fk[0].kind, ColumnConstraint::Kind::FOREIGN_KEY);
    ASSERT_TRUE(order_fk[0].fk_table.has_value());
    EXPECT_EQ(*order_fk[0].fk_table, "products");
}

TEST_F(SchemaConstraintsTest, ConstraintViolationToJSON) {
    ConstraintViolation v;
    v.table_name       = "users";
    v.column_name      = "email";
    v.constraint_name  = "nn_email";
    v.constraint_type  = "NOT_NULL";
    v.message          = "email cannot be null";

    auto j = v.toJSON();
    EXPECT_EQ(j["table_name"],      "users");
    EXPECT_EQ(j["column_name"],     "email");
    EXPECT_EQ(j["constraint_type"], "NOT_NULL");
    EXPECT_EQ(j["message"],         "email cannot be null");
}

TEST_F(SchemaConstraintsTest, ColumnConstraintToJSONNotNull) {
    auto c = ColumnConstraint::makeNotNull("nn_x");
    auto j = c.toJSON();
    EXPECT_EQ(j["kind"], "NOT_NULL");
    EXPECT_EQ(j["name"], "nn_x");
}

TEST_F(SchemaConstraintsTest, ColumnConstraintToJSONCheck) {
    auto c = ColumnConstraint::makeCheck("chk_x", "x > 0");
    auto j = c.toJSON();
    EXPECT_EQ(j["kind"],       "CHECK");
    EXPECT_EQ(j["check_expr"], "x > 0");
}

TEST_F(SchemaConstraintsTest, ColumnConstraintToJSONDefault) {
    auto c = ColumnConstraint::makeDefault("df_x", ColumnValue{int64_t(42)});
    auto j = c.toJSON();
    EXPECT_EQ(j["kind"],          "DEFAULT");
    EXPECT_EQ(j["default_value"], 42);
}

TEST_F(SchemaConstraintsTest, ColumnConstraintToJSONForeignKey) {
    auto c = ColumnConstraint::makeForeignKey("fk_x", "refs", "id");
    auto j = c.toJSON();
    EXPECT_EQ(j["kind"],      "FOREIGN_KEY");
    EXPECT_EQ(j["fk_table"],  "refs");
    EXPECT_EQ(j["fk_column"], "id");
}
