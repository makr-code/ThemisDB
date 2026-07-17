/**
 * @file test_aql_mutations_parser.cpp
 * @brief Unit tests for EPIC-004 Phase 1: AQL 2.0.0 DML mutation parser.
 *
 * Covers all five mutation types (INSERT, UPDATE, REMOVE/DELETE, REPLACE, UPSERT)
 * in both AQL-native and SQL-style surface forms, plus AqlSafetyValidator
 * ValidationMode::AllowMutations and backward-compatibility guards.
 *
 * @note Phase 1 is parser-only — no execution semantics are tested here.
 */

#include <gtest/gtest.h>
#include "query/aql_parser.h"
#include "query/aql_safety_validator.h"

using namespace themis::query;

// ============================================================================
// Helpers
// ============================================================================

static InsertNode& asInsert(MutationNode& n) {
    return dynamic_cast<InsertNode&>(n);
}
static UpdateNode& asUpdate(MutationNode& n) {
    return dynamic_cast<UpdateNode&>(n);
}
static RemoveNode& asRemove(MutationNode& n) {
    return dynamic_cast<RemoveNode&>(n);
}
static ReplaceNode& asReplace(MutationNode& n) {
    return dynamic_cast<ReplaceNode&>(n);
}
static UpsertNode& asUpsert(MutationNode& n) {
    return dynamic_cast<UpsertNode&>(n);
}

// ============================================================================
// INSERT — SQL-style
// ============================================================================

TEST(AqlMutationsParser, InsertIntoBasic) {
    AQLParser parser;
    auto r = parser.parseMutation(R"(INSERT INTO users VALUES {name: "Alice"})");
    ASSERT_TRUE(r.has_value()) << r.error().message();
    EXPECT_EQ((*r)->getType(), ASTNodeType::Insert);
    auto& n = asInsert(**r);
    EXPECT_EQ(n.collection, "users");
    ASSERT_EQ(n.documents.size(), 1u);
    EXPECT_FALSE(n.return_new);
}

TEST(AqlMutationsParser, InsertIntoReturnNew) {
    AQLParser parser;
    auto r = parser.parseMutation(R"(INSERT INTO orders VALUES {amount: 100} RETURN NEW)");
    ASSERT_TRUE(r.has_value()) << r.error().message();
    auto& n = asInsert(**r);
    EXPECT_EQ(n.collection, "orders");
    EXPECT_TRUE(n.return_new);
}

TEST(AqlMutationsParser, InsertIntoMultipleDocuments) {
    AQLParser parser;
    auto r = parser.parseMutation(
        R"(INSERT INTO items VALUES {id: 1}, {id: 2}, {id: 3})");
    ASSERT_TRUE(r.has_value()) << r.error().message();
    auto& n = asInsert(**r);
    EXPECT_EQ(n.collection, "items");
    EXPECT_EQ(n.documents.size(), 3u);
}

TEST(AqlMutationsParser, InsertIntoKeywordAsCollection) {
    // Collection name that looks like a keyword should still parse.
    AQLParser parser;
    auto r = parser.parseMutation(R"(INSERT INTO products VALUES {name: "Widget"})");
    ASSERT_TRUE(r.has_value()) << r.error().message();
    EXPECT_EQ(asInsert(**r).collection, "products");
}

TEST(AqlMutationsParser, InsertIntoSemicolon) {
    AQLParser parser;
    auto r = parser.parseMutation(R"(INSERT INTO users VALUES {name: "Bob"};)");
    ASSERT_TRUE(r.has_value()) << r.error().message();
    EXPECT_EQ(asInsert(**r).collection, "users");
}

// ============================================================================
// INSERT — AQL-native
// ============================================================================

TEST(AqlMutationsParser, InsertNativeBasic) {
    AQLParser parser;
    auto r = parser.parseMutation(R"(INSERT {name: "Carol"} INTO customers)");
    ASSERT_TRUE(r.has_value()) << r.error().message();
    auto& n = asInsert(**r);
    EXPECT_EQ(n.collection, "customers");
    ASSERT_EQ(n.documents.size(), 1u);
}

TEST(AqlMutationsParser, InsertNativeReturnNew) {
    AQLParser parser;
    auto r = parser.parseMutation(R"(INSERT {x: 42} INTO log RETURN NEW)");
    ASSERT_TRUE(r.has_value()) << r.error().message();
    auto& n = asInsert(**r);
    EXPECT_EQ(n.collection, "log");
    EXPECT_TRUE(n.return_new);
}

TEST(AqlMutationsParser, InsertCaseInsensitive) {
    AQLParser parser;
    auto r = parser.parseMutation(R"(insert into users values {name: "Dave"})");
    ASSERT_TRUE(r.has_value()) << r.error().message();
    EXPECT_EQ(asInsert(**r).collection, "users");
}

// ============================================================================
// INSERT — toJSON
// ============================================================================

TEST(AqlMutationsParser, InsertToJson) {
    AQLParser parser;
    auto r = parser.parseMutation(R"(INSERT INTO users VALUES {name: "Eve"})");
    ASSERT_TRUE(r.has_value()) << r.error().message();
    auto j = (*r)->toJSON();
    EXPECT_EQ(j["type"], "INSERT");
    EXPECT_EQ(j["collection"], "users");
    EXPECT_FALSE(j["return_new"]);
}

// ============================================================================
// UPDATE — SQL-style
// ============================================================================

TEST(AqlMutationsParser, UpdateSetBasic) {
    AQLParser parser;
    auto r = parser.parseMutation(R"(UPDATE users SET age = 30)");
    ASSERT_TRUE(r.has_value()) << r.error().message();
    EXPECT_EQ((*r)->getType(), ASTNodeType::Update);
    auto& n = asUpdate(**r);
    EXPECT_EQ(n.collection, "users");
    ASSERT_EQ(n.set_clauses.size(), 1u);
    EXPECT_EQ(n.set_clauses[0].field, "age");
    EXPECT_FALSE(n.filter);
}

TEST(AqlMutationsParser, UpdateSetWithWhere) {
    AQLParser parser;
    auto r = parser.parseMutation(R"(UPDATE users SET active = true WHERE age > 18)");
    ASSERT_TRUE(r.has_value()) << r.error().message();
    auto& n = asUpdate(**r);
    EXPECT_EQ(n.collection, "users");
    ASSERT_TRUE(n.filter);
    ASSERT_EQ(n.set_clauses.size(), 1u);
    EXPECT_EQ(n.set_clauses[0].field, "active");
}

TEST(AqlMutationsParser, UpdateSetMultipleClauses) {
    AQLParser parser;
    auto r = parser.parseMutation(R"(UPDATE products SET price = 9.99, stock = 100)");
    ASSERT_TRUE(r.has_value()) << r.error().message();
    auto& n = asUpdate(**r);
    ASSERT_EQ(n.set_clauses.size(), 2u);
    EXPECT_EQ(n.set_clauses[0].field, "price");
    EXPECT_EQ(n.set_clauses[1].field, "stock");
}

TEST(AqlMutationsParser, UpdateSetReturnNew) {
    AQLParser parser;
    auto r = parser.parseMutation(R"(UPDATE items SET qty = 0 RETURN NEW)");
    ASSERT_TRUE(r.has_value()) << r.error().message();
    EXPECT_TRUE(asUpdate(**r).return_new);
}

TEST(AqlMutationsParser, UpdateSetReturnOld) {
    AQLParser parser;
    auto r = parser.parseMutation(R"(UPDATE items SET qty = 0 RETURN OLD)");
    ASSERT_TRUE(r.has_value()) << r.error().message();
    EXPECT_TRUE(asUpdate(**r).return_old);
}

TEST(AqlMutationsParser, UpdateSetLimitClause) {
    AQLParser parser;
    auto r = parser.parseMutation(R"(UPDATE items SET visible = false WHERE qty = 0 LIMIT 50)");
    ASSERT_TRUE(r.has_value()) << r.error().message();
    auto& n = asUpdate(**r);
    ASSERT_TRUE(n.limit.has_value());
    EXPECT_EQ(*n.limit, 50);
}

// ============================================================================
// UPDATE — AQL-native
// ============================================================================

TEST(AqlMutationsParser, UpdateNativeWithIn) {
    AQLParser parser;
    auto r = parser.parseMutation(R"(UPDATE {_key: "abc"} WITH {name: "new"} IN users)");
    ASSERT_TRUE(r.has_value()) << r.error().message();
    auto& n = asUpdate(**r);
    EXPECT_EQ(n.collection, "users");
    ASSERT_TRUE(n.search_expr);
    ASSERT_TRUE(n.update_expr);
}

TEST(AqlMutationsParser, UpdateNativeReturnNew) {
    AQLParser parser;
    auto r = parser.parseMutation(R"(UPDATE {_key: "x"} WITH {v: 1} IN col RETURN NEW)");
    ASSERT_TRUE(r.has_value()) << r.error().message();
    EXPECT_TRUE(asUpdate(**r).return_new);
}

// ============================================================================
// UPDATE — toJSON
// ============================================================================

TEST(AqlMutationsParser, UpdateToJson) {
    AQLParser parser;
    auto r = parser.parseMutation(R"(UPDATE users SET name = "x")");
    ASSERT_TRUE(r.has_value()) << r.error().message();
    auto j = (*r)->toJSON();
    EXPECT_EQ(j["type"], "UPDATE");
    EXPECT_EQ(j["collection"], "users");
}

// ============================================================================
// REMOVE — AQL-native
// ============================================================================

TEST(AqlMutationsParser, RemoveBasic) {
    AQLParser parser;
    auto r = parser.parseMutation(R"(REMOVE {_key: "abc"} IN users)");
    ASSERT_TRUE(r.has_value()) << r.error().message();
    EXPECT_EQ((*r)->getType(), ASTNodeType::Remove);
    auto& n = asRemove(**r);
    EXPECT_EQ(n.collection, "users");
    ASSERT_TRUE(n.doc_expr);
    EXPECT_FALSE(n.return_removed);
}

TEST(AqlMutationsParser, RemoveReturnOld) {
    AQLParser parser;
    auto r = parser.parseMutation(R"(REMOVE {_key: "k1"} IN logs RETURN OLD)");
    ASSERT_TRUE(r.has_value()) << r.error().message();
    EXPECT_TRUE(asRemove(**r).return_removed);
}

TEST(AqlMutationsParser, RemoveCaseInsensitive) {
    AQLParser parser;
    auto r = parser.parseMutation(R"(remove {_key: "k"} in col)");
    ASSERT_TRUE(r.has_value()) << r.error().message();
    EXPECT_EQ(asRemove(**r).collection, "col");
}

// ============================================================================
// DELETE — SQL-style alias for REMOVE
// ============================================================================

TEST(AqlMutationsParser, DeleteFromBasic) {
    AQLParser parser;
    auto r = parser.parseMutation(R"(DELETE FROM sessions)");
    ASSERT_TRUE(r.has_value()) << r.error().message();
    EXPECT_EQ((*r)->getType(), ASTNodeType::Remove);
    auto& n = asRemove(**r);
    EXPECT_EQ(n.collection, "sessions");
    EXPECT_FALSE(n.filter);
}

TEST(AqlMutationsParser, DeleteFromWhere) {
    AQLParser parser;
    auto r = parser.parseMutation(R"(DELETE FROM sessions WHERE expired == true)");
    ASSERT_TRUE(r.has_value()) << r.error().message();
    auto& n = asRemove(**r);
    EXPECT_EQ(n.collection, "sessions");
    ASSERT_TRUE(n.filter);
}

TEST(AqlMutationsParser, DeleteFromWhereLimit) {
    AQLParser parser;
    auto r = parser.parseMutation(R"(DELETE FROM cache WHERE ttl < 0 LIMIT 1000)");
    ASSERT_TRUE(r.has_value()) << r.error().message();
    auto& n = asRemove(**r);
    ASSERT_TRUE(n.limit.has_value());
    EXPECT_EQ(*n.limit, 1000);
}

TEST(AqlMutationsParser, DeleteFromReturnOld) {
    AQLParser parser;
    auto r = parser.parseMutation(R"(DELETE FROM temp WHERE active == false RETURN OLD)");
    ASSERT_TRUE(r.has_value()) << r.error().message();
    EXPECT_TRUE(asRemove(**r).return_removed);
}

// ============================================================================
// REPLACE
// ============================================================================

TEST(AqlMutationsParser, ReplaceBasic) {
    AQLParser parser;
    auto r = parser.parseMutation(R"(REPLACE {_key: "k1"} WITH {name: "new"} IN users)");
    ASSERT_TRUE(r.has_value()) << r.error().message();
    EXPECT_EQ((*r)->getType(), ASTNodeType::Replace);
    auto& n = asReplace(**r);
    EXPECT_EQ(n.collection, "users");
    ASSERT_TRUE(n.search_expr);
    ASSERT_TRUE(n.replacement);
}

TEST(AqlMutationsParser, ReplaceReturnNew) {
    AQLParser parser;
    auto r = parser.parseMutation(R"(REPLACE {_key: "x"} WITH {v: 2} IN docs RETURN NEW)");
    ASSERT_TRUE(r.has_value()) << r.error().message();
    EXPECT_TRUE(asReplace(**r).return_new);
}

TEST(AqlMutationsParser, ReplaceReturnOld) {
    AQLParser parser;
    auto r = parser.parseMutation(R"(REPLACE {_key: "x"} WITH {v: 2} IN docs RETURN OLD)");
    ASSERT_TRUE(r.has_value()) << r.error().message();
    EXPECT_TRUE(asReplace(**r).return_old);
}

TEST(AqlMutationsParser, ReplaceCaseInsensitive) {
    AQLParser parser;
    auto r = parser.parseMutation(R"(replace {_key: "x"} with {v: 1} in docs)");
    ASSERT_TRUE(r.has_value()) << r.error().message();
    EXPECT_EQ(asReplace(**r).collection, "docs");
}

// ============================================================================
// REPLACE — toJSON
// ============================================================================

TEST(AqlMutationsParser, ReplaceToJson) {
    AQLParser parser;
    auto r = parser.parseMutation(R"(REPLACE {_key: "k"} WITH {x: 1} IN col)");
    ASSERT_TRUE(r.has_value()) << r.error().message();
    auto j = (*r)->toJSON();
    EXPECT_EQ(j["type"], "REPLACE");
    EXPECT_EQ(j["collection"], "col");
}

// ============================================================================
// UPSERT
// ============================================================================

TEST(AqlMutationsParser, UpsertBasic) {
    AQLParser parser;
    auto r = parser.parseMutation(
        R"(UPSERT {email: "a@b.com"} INSERT {email: "a@b.com", n: 1} UPDATE {n: 2} IN users)");
    ASSERT_TRUE(r.has_value()) << r.error().message();
    EXPECT_EQ((*r)->getType(), ASTNodeType::Upsert);
    auto& n = asUpsert(**r);
    EXPECT_EQ(n.collection, "users");
    ASSERT_TRUE(n.search_expr);
    ASSERT_TRUE(n.insert_doc);
    ASSERT_TRUE(n.update_doc);
}

TEST(AqlMutationsParser, UpsertReturnNew) {
    AQLParser parser;
    auto r = parser.parseMutation(
        R"(UPSERT {_key: "k"} INSERT {_key: "k"} UPDATE {v: 1} IN col RETURN NEW)");
    ASSERT_TRUE(r.has_value()) << r.error().message();
    EXPECT_TRUE(asUpsert(**r).return_new);
}

TEST(AqlMutationsParser, UpsertReturnOld) {
    AQLParser parser;
    auto r = parser.parseMutation(
        R"(UPSERT {_key: "k"} INSERT {_key: "k"} UPDATE {v: 1} IN col RETURN OLD)");
    ASSERT_TRUE(r.has_value()) << r.error().message();
    EXPECT_TRUE(asUpsert(**r).return_old);
}

TEST(AqlMutationsParser, UpsertCaseInsensitive) {
    AQLParser parser;
    auto r = parser.parseMutation(
        R"(upsert {_key: "k"} insert {_key: "k", v: 1} update {v: 2} in docs)");
    ASSERT_TRUE(r.has_value()) << r.error().message();
    EXPECT_EQ(asUpsert(**r).collection, "docs");
}

// ============================================================================
// UPSERT — toJSON
// ============================================================================

TEST(AqlMutationsParser, UpsertToJson) {
    AQLParser parser;
    auto r = parser.parseMutation(
        R"(UPSERT {_key: "x"} INSERT {_key: "x"} UPDATE {v: 0} IN col)");
    ASSERT_TRUE(r.has_value()) << r.error().message();
    auto j = (*r)->toJSON();
    EXPECT_EQ(j["type"], "UPSERT");
    EXPECT_EQ(j["collection"], "col");
}

// ============================================================================
// Error cases
// ============================================================================

TEST(AqlMutationsParser, ErrorUnknownLeadingKeyword) {
    AQLParser parser;
    auto r = parser.parseMutation("FOR x IN col RETURN x");
    EXPECT_FALSE(r.has_value());
}

TEST(AqlMutationsParser, ErrorInsertMissingInto) {
    AQLParser parser;
    auto r = parser.parseMutation(R"(INSERT {x: 1} users)");
    EXPECT_FALSE(r.has_value());
}

TEST(AqlMutationsParser, ErrorInsertIntoMissingValues) {
    AQLParser parser;
    auto r = parser.parseMutation("INSERT INTO users");
    EXPECT_FALSE(r.has_value());
}

TEST(AqlMutationsParser, ErrorUpdateMissingSetOrWith) {
    AQLParser parser;
    // SQL-style: collection is IDENTIFIER but followed by non-SET token
    auto r = parser.parseMutation("UPDATE users WHERE age > 0");
    // The collection name "users" is IDENTIFIER, next is WHERE (not SET) and
    // peek() is WHERE (not SET) so parser falls to AQL-native path which
    // tries to parse "users" as an expression then expects WITH — should fail.
    EXPECT_FALSE(r.has_value());
}

TEST(AqlMutationsParser, ErrorRemoveMissingIn) {
    AQLParser parser;
    auto r = parser.parseMutation(R"(REMOVE {_key: "x"} users)");
    EXPECT_FALSE(r.has_value());
}

TEST(AqlMutationsParser, ErrorDeleteFromMissingCollection) {
    AQLParser parser;
    auto r = parser.parseMutation("DELETE FROM");
    EXPECT_FALSE(r.has_value());
}

TEST(AqlMutationsParser, ErrorReplaceNoWith) {
    AQLParser parser;
    auto r = parser.parseMutation(R"(REPLACE {_key: "k"} {v: 1} IN col)");
    EXPECT_FALSE(r.has_value());
}

TEST(AqlMutationsParser, ErrorReplaceNoIn) {
    AQLParser parser;
    auto r = parser.parseMutation(R"(REPLACE {_key: "k"} WITH {v: 1} col)");
    EXPECT_FALSE(r.has_value());
}

TEST(AqlMutationsParser, ErrorUpsertNoInsert) {
    AQLParser parser;
    auto r = parser.parseMutation(R"(UPSERT {_key: "k"} {v: 1} UPDATE {v: 2} IN col)");
    EXPECT_FALSE(r.has_value());
}

TEST(AqlMutationsParser, ErrorUpsertNoUpdate) {
    AQLParser parser;
    auto r = parser.parseMutation(R"(UPSERT {_key: "k"} INSERT {_key: "k"} {v: 2} IN col)");
    EXPECT_FALSE(r.has_value());
}

TEST(AqlMutationsParser, ErrorTrailingGarbage) {
    AQLParser parser;
    auto r = parser.parseMutation(R"(INSERT INTO users VALUES {x: 1} EXTRA_GARBAGE)");
    EXPECT_FALSE(r.has_value());
}

// ============================================================================
// AqlSafetyValidator — ValidationMode::AllowMutations
// ============================================================================

TEST(AqlSafetyValidatorMode, DefaultModeBlocksMutations) {
    AqlSafetyValidator v; // ReadOnly by default
    EXPECT_TRUE(v.validate(R"(INSERT INTO users VALUES {x: 1})").has_value());
    EXPECT_TRUE(v.validate("UPDATE users SET x = 1").has_value());
    EXPECT_TRUE(v.validate(R"(REMOVE {_key: "k"} IN col)").has_value());
    EXPECT_TRUE(v.validate("REPLACE {_key: \"k\"} WITH {v: 1} IN col").has_value());
    EXPECT_TRUE(v.validate("UPSERT {x:1} INSERT {x:1} UPDATE {x:2} IN col").has_value());
    EXPECT_TRUE(v.validate("DELETE FROM sessions").has_value());
}

TEST(AqlSafetyValidatorMode, ReadOnlyModeAllowsReadQueries) {
    AqlSafetyValidator v(AqlSafetyValidator::ValidationMode::ReadOnly);
    EXPECT_FALSE(v.validate("FOR doc IN users FILTER doc.age > 18 RETURN doc").has_value());
}

TEST(AqlSafetyValidatorMode, AllowMutationsModePassesDml) {
    AqlSafetyValidator v(AqlSafetyValidator::ValidationMode::AllowMutations);
    EXPECT_FALSE(v.validate(R"(INSERT INTO users VALUES {x: 1})").has_value());
    EXPECT_FALSE(v.validate("UPDATE users SET x = 1").has_value());
    EXPECT_FALSE(v.validate(R"(REMOVE {_key: "k"} IN col)").has_value());
    EXPECT_FALSE(v.validate("DELETE FROM sessions").has_value());
}

TEST(AqlSafetyValidatorMode, AllowMutationsModeAlsoAllowsReadQueries) {
    AqlSafetyValidator v(AqlSafetyValidator::ValidationMode::AllowMutations);
    EXPECT_FALSE(v.validate("FOR doc IN users RETURN doc").has_value());
}

TEST(AqlSafetyValidatorMode, IsSafeReadOnly) {
    AqlSafetyValidator v;
    EXPECT_FALSE(v.isSafe(R"(INSERT INTO t VALUES {x: 1})"));
    EXPECT_TRUE(v.isSafe("FOR x IN col RETURN x"));
}

TEST(AqlSafetyValidatorMode, IsSafeAllowMutations) {
    AqlSafetyValidator v(AqlSafetyValidator::ValidationMode::AllowMutations);
    EXPECT_TRUE(v.isSafe(R"(INSERT INTO t VALUES {x: 1})"));
}

// ============================================================================
// Backward-compatibility: existing AQLParser::parse() still works
// ============================================================================

TEST(AqlMutationsParser, BackwardCompatParseReadOnly) {
    AQLParser parser;
    auto r = parser.parse("FOR doc IN users FILTER doc.age > 18 RETURN doc");
    ASSERT_TRUE(r.has_value()) << r.error().message();
    EXPECT_EQ((*r)->for_node.collection, "users");
}

TEST(AqlMutationsParser, BackwardCompatTransactionBlock) {
    AQLParser parser;
    auto r = parser.parseTransactionBlock(
        "BEGIN FOR x IN col RETURN x COMMIT");
    ASSERT_TRUE(r.has_value()) << r.error().message();
    EXPECT_EQ(r->action, AqlTransactionAction::Commit);
}

// ============================================================================
// ASTNodeType sanity checks
// ============================================================================

TEST(AqlMutationsParser, AstTypeInsert) {
    AQLParser parser;
    auto r = parser.parseMutation(R"(INSERT {v: 1} INTO col)");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ((*r)->getType(), ASTNodeType::Insert);
}

TEST(AqlMutationsParser, AstTypeUpdate) {
    AQLParser parser;
    auto r = parser.parseMutation(R"(UPDATE col SET v = 1)");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ((*r)->getType(), ASTNodeType::Update);
}

TEST(AqlMutationsParser, AstTypeRemoveViaDelete) {
    AQLParser parser;
    auto r = parser.parseMutation("DELETE FROM col");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ((*r)->getType(), ASTNodeType::Remove);
}

TEST(AqlMutationsParser, AstTypeRemoveDirect) {
    AQLParser parser;
    auto r = parser.parseMutation(R"(REMOVE {_key: "k"} IN col)");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ((*r)->getType(), ASTNodeType::Remove);
}

TEST(AqlMutationsParser, AstTypeReplace) {
    AQLParser parser;
    auto r = parser.parseMutation(R"(REPLACE {_key: "k"} WITH {v: 1} IN col)");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ((*r)->getType(), ASTNodeType::Replace);
}

TEST(AqlMutationsParser, AstTypeUpsert) {
    AQLParser parser;
    auto r = parser.parseMutation(
        R"(UPSERT {_key: "k"} INSERT {_key: "k"} UPDATE {v: 1} IN col)");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ((*r)->getType(), ASTNodeType::Upsert);
}
