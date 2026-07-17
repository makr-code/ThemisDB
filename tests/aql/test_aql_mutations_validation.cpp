/**
 * @file test_aql_mutations_validation.cpp
 * @brief Unit tests for EPIC-004 Phase 2: AQL mutation semantic validation.
 *
 * Tests cover:
 * - AqlMutationValidator collection/field name helpers
 * - Semantic validation for INSERT, UPDATE, REMOVE, REPLACE, UPSERT nodes
 * - AqlSafetyValidator::validateMutationSafety() injection checks
 * - AqlSafetyValidator::validate() in AllowMutations mode
 * - Edge cases (long names, dot-paths, underscore prefixes)
 */

#include <gtest/gtest.h>
#include "query/aql_mutation_validator.h"
#include "query/aql_safety_validator.h"

using namespace themis::query;

// ============================================================================
// Helpers: Build minimal AST nodes for testing
// ============================================================================

static std::shared_ptr<Expression> makeLiteral(std::string s) {
    return std::make_shared<LiteralExpr>(LiteralValue{std::move(s)});
}

static std::shared_ptr<Expression> makeVar(std::string name) {
    return std::make_shared<VariableExpr>(std::move(name));
}

// ============================================================================
// isValidCollectionName — 6 cases
// ============================================================================

TEST(AqlMutationValidator, CollectionNameValid_Simple) {
    AqlMutationValidator v;
    EXPECT_TRUE(v.isValidCollectionName("users"));
}

TEST(AqlMutationValidator, CollectionNameValid_WithUnderscore) {
    AqlMutationValidator v;
    EXPECT_TRUE(v.isValidCollectionName("_users"));
}

TEST(AqlMutationValidator, CollectionNameValid_WithDigits) {
    AqlMutationValidator v;
    EXPECT_TRUE(v.isValidCollectionName("users2024"));
}

TEST(AqlMutationValidator, CollectionNameInvalid_Empty) {
    AqlMutationValidator v;
    EXPECT_FALSE(v.isValidCollectionName(""));
}

TEST(AqlMutationValidator, CollectionNameInvalid_StartsWithDigit) {
    AqlMutationValidator v;
    EXPECT_FALSE(v.isValidCollectionName("1users"));
}

TEST(AqlMutationValidator, CollectionNameInvalid_TooLong) {
    AqlMutationValidator v;
    // 257 characters
    std::string name(257, 'a');
    EXPECT_FALSE(v.isValidCollectionName(name));
}

// ============================================================================
// isValidFieldName — 6 cases
// ============================================================================

TEST(AqlMutationValidator, FieldNameValid_Simple) {
    AqlMutationValidator v;
    EXPECT_TRUE(v.isValidFieldName("name"));
}

TEST(AqlMutationValidator, FieldNameValid_DotPath) {
    AqlMutationValidator v;
    EXPECT_TRUE(v.isValidFieldName("address.city"));
}

TEST(AqlMutationValidator, FieldNameValid_UnderscorePrefix) {
    AqlMutationValidator v;
    EXPECT_TRUE(v.isValidFieldName("_key"));
}

TEST(AqlMutationValidator, FieldNameInvalid_Empty) {
    AqlMutationValidator v;
    EXPECT_FALSE(v.isValidFieldName(""));
}

TEST(AqlMutationValidator, FieldNameInvalid_StartsWithDigit) {
    AqlMutationValidator v;
    EXPECT_FALSE(v.isValidFieldName("9field"));
}

TEST(AqlMutationValidator, FieldNameInvalid_TooLong) {
    AqlMutationValidator v;
    std::string name(257, 'x');
    EXPECT_FALSE(v.isValidFieldName(name));
}

// ============================================================================
// validateInsert — 5 cases
// ============================================================================

TEST(AqlMutationValidator, InsertValid) {
    AqlMutationValidator v;
    InsertNode node;
    node.collection = "orders";
    node.documents.push_back(makeLiteral("{}"));
    auto r = v.validate(node);
    EXPECT_TRUE(r.valid);
    EXPECT_TRUE(r.errors.empty());
}

TEST(AqlMutationValidator, InsertEmptyDocuments) {
    AqlMutationValidator v;
    InsertNode node;
    node.collection = "orders";
    // no documents
    auto r = v.validate(node);
    EXPECT_FALSE(r.valid);
    ASSERT_GE(r.errors.size(), 1u);
    EXPECT_NE(r.errors[0].find("document"), std::string::npos);
}

TEST(AqlMutationValidator, InsertEmptyCollectionName) {
    AqlMutationValidator v;
    InsertNode node;
    node.collection = "";
    node.documents.push_back(makeLiteral("{}"));
    auto r = v.validate(node);
    EXPECT_FALSE(r.valid);
    ASSERT_GE(r.errors.size(), 1u);
    EXPECT_NE(r.errors[0].find("collection"), std::string::npos);
}

TEST(AqlMutationValidator, InsertInvalidCollectionName) {
    AqlMutationValidator v;
    InsertNode node;
    node.collection = "1invalid";
    node.documents.push_back(makeLiteral("{}"));
    auto r = v.validate(node);
    EXPECT_FALSE(r.valid);
}

TEST(AqlMutationValidator, InsertMultipleDocumentsValid) {
    AqlMutationValidator v;
    InsertNode node;
    node.collection = "items";
    node.documents.push_back(makeLiteral("{}"));
    node.documents.push_back(makeLiteral("{}"));
    node.documents.push_back(makeLiteral("{}"));
    auto r = v.validate(node);
    EXPECT_TRUE(r.valid);
    EXPECT_TRUE(r.errors.empty());
}

// ============================================================================
// validateUpdate — 5 cases
// ============================================================================

TEST(AqlMutationValidator, UpdateValidWithSetClauses) {
    AqlMutationValidator v;
    UpdateNode node;
    node.collection = "users";
    node.filter     = makeVar("u");
    SetClause sc;
    sc.field = "name";
    sc.value = makeLiteral("Alice");
    node.set_clauses.push_back(sc);
    auto r = v.validate(node);
    EXPECT_TRUE(r.valid);
}

TEST(AqlMutationValidator, UpdateValidWithUpdateExpr) {
    AqlMutationValidator v;
    UpdateNode node;
    node.collection  = "products";
    node.update_expr = makeVar("newDoc");
    auto r = v.validate(node);
    EXPECT_TRUE(r.valid);
}

TEST(AqlMutationValidator, UpdateNoClausesOrExpr) {
    AqlMutationValidator v;
    UpdateNode node;
    node.collection = "users";
    // Neither set_clauses nor update_expr
    auto r = v.validate(node);
    EXPECT_FALSE(r.valid);
    ASSERT_GE(r.errors.size(), 1u);
    EXPECT_NE(r.errors[0].find("SET"), std::string::npos);
}

TEST(AqlMutationValidator, UpdateEmptyFieldNameInSet) {
    AqlMutationValidator v;
    UpdateNode node;
    node.collection = "users";
    SetClause sc;
    sc.field = "";  // empty!
    sc.value = makeLiteral("x");
    node.set_clauses.push_back(sc);
    auto r = v.validate(node);
    EXPECT_FALSE(r.valid);
    ASSERT_GE(r.errors.size(), 1u);
}

TEST(AqlMutationValidator, UpdateInvalidFieldNameInSet) {
    AqlMutationValidator v;
    UpdateNode node;
    node.collection = "users";
    SetClause sc;
    sc.field = "9badfield"; // starts with digit
    sc.value = makeLiteral("x");
    node.set_clauses.push_back(sc);
    auto r = v.validate(node);
    EXPECT_FALSE(r.valid);
}

// ============================================================================
// validateRemove — 4 cases
// ============================================================================

TEST(AqlMutationValidator, RemoveValidWithFilter) {
    AqlMutationValidator v;
    RemoveNode node;
    node.collection = "logs";
    node.filter     = makeVar("doc");
    auto r = v.validate(node);
    EXPECT_TRUE(r.valid);
    EXPECT_TRUE(r.warnings.empty());
}

TEST(AqlMutationValidator, RemoveValidWithDocExpr) {
    AqlMutationValidator v;
    RemoveNode node;
    node.collection = "logs";
    node.doc_expr   = makeVar("doc");
    auto r = v.validate(node);
    EXPECT_TRUE(r.valid);
    EXPECT_TRUE(r.warnings.empty());
}

TEST(AqlMutationValidator, RemoveNoFilter_Warning) {
    AqlMutationValidator v;
    RemoveNode node;
    node.collection = "logs";
    // Neither filter nor doc_expr
    auto r = v.validate(node);
    EXPECT_TRUE(r.valid);              // valid (not an error)
    ASSERT_GE(r.warnings.size(), 1u); // but produces a warning
    EXPECT_NE(r.warnings[0].find("REMOVE"), std::string::npos);
}

TEST(AqlMutationValidator, RemoveEmptyCollection) {
    AqlMutationValidator v;
    RemoveNode node;
    node.collection = "";
    auto r = v.validate(node);
    EXPECT_FALSE(r.valid);
}

// ============================================================================
// validateReplace — 4 cases
// ============================================================================

TEST(AqlMutationValidator, ReplaceValid) {
    AqlMutationValidator v;
    ReplaceNode node;
    node.collection  = "orders";
    node.search_expr = makeVar("search");
    node.replacement = makeVar("newDoc");
    auto r = v.validate(node);
    EXPECT_TRUE(r.valid);
}

TEST(AqlMutationValidator, ReplaceMissingSearchExpr) {
    AqlMutationValidator v;
    ReplaceNode node;
    node.collection  = "orders";
    node.replacement = makeVar("newDoc");
    // search_expr is null
    auto r = v.validate(node);
    EXPECT_FALSE(r.valid);
    ASSERT_GE(r.errors.size(), 1u);
    EXPECT_NE(r.errors[0].find("search"), std::string::npos);
}

TEST(AqlMutationValidator, ReplaceMissingReplacement) {
    AqlMutationValidator v;
    ReplaceNode node;
    node.collection  = "orders";
    node.search_expr = makeVar("search");
    // replacement is null
    auto r = v.validate(node);
    EXPECT_FALSE(r.valid);
    ASSERT_GE(r.errors.size(), 1u);
    EXPECT_NE(r.errors[0].find("replacement"), std::string::npos);
}

TEST(AqlMutationValidator, ReplaceEmptyCollection) {
    AqlMutationValidator v;
    ReplaceNode node;
    node.collection  = "";
    node.search_expr = makeVar("s");
    node.replacement = makeVar("r");
    auto r = v.validate(node);
    EXPECT_FALSE(r.valid);
}

// ============================================================================
// validateUpsert — 5 cases
// ============================================================================

TEST(AqlMutationValidator, UpsertValid) {
    AqlMutationValidator v;
    UpsertNode node;
    node.collection  = "products";
    node.search_expr = makeVar("search");
    node.insert_doc  = makeVar("ins");
    node.update_doc  = makeVar("upd");
    auto r = v.validate(node);
    EXPECT_TRUE(r.valid);
}

TEST(AqlMutationValidator, UpsertMissingSearchExpr) {
    AqlMutationValidator v;
    UpsertNode node;
    node.collection = "products";
    node.insert_doc = makeVar("ins");
    node.update_doc = makeVar("upd");
    auto r = v.validate(node);
    EXPECT_FALSE(r.valid);
    ASSERT_GE(r.errors.size(), 1u);
    EXPECT_NE(r.errors[0].find("search"), std::string::npos);
}

TEST(AqlMutationValidator, UpsertMissingInsertDoc) {
    AqlMutationValidator v;
    UpsertNode node;
    node.collection  = "products";
    node.search_expr = makeVar("search");
    node.update_doc  = makeVar("upd");
    auto r = v.validate(node);
    EXPECT_FALSE(r.valid);
    ASSERT_GE(r.errors.size(), 1u);
    EXPECT_NE(r.errors[0].find("insert"), std::string::npos);
}

TEST(AqlMutationValidator, UpsertMissingUpdateDoc) {
    AqlMutationValidator v;
    UpsertNode node;
    node.collection  = "products";
    node.search_expr = makeVar("search");
    node.insert_doc  = makeVar("ins");
    auto r = v.validate(node);
    EXPECT_FALSE(r.valid);
    ASSERT_GE(r.errors.size(), 1u);
    EXPECT_NE(r.errors[0].find("update"), std::string::npos);
}

TEST(AqlMutationValidator, UpsertEmptyCollection) {
    AqlMutationValidator v;
    UpsertNode node;
    node.collection  = "";
    node.search_expr = makeVar("s");
    node.insert_doc  = makeVar("i");
    node.update_doc  = makeVar("u");
    auto r = v.validate(node);
    EXPECT_FALSE(r.valid);
}

// ============================================================================
// SafetyValidator injection checks — 5 cases
// ============================================================================

TEST(AqlSafetyValidatorMutation, NulByteInjection) {
    AqlSafetyValidator v;
    std::string q = "FOR u IN users RETURN u";
    q[10] = '\0';  // embed NUL
    auto viol = v.validateMutationSafety(q);
    ASSERT_TRUE(viol.has_value());
    EXPECT_EQ(viol->keyword, "NUL_INJECTION");
}

TEST(AqlSafetyValidatorMutation, MultiStatementDropInjection) {
    AqlSafetyValidator v;
    auto viol = v.validateMutationSafety(
        "INSERT INTO users VALUES {name:'x'}; DROP users");
    ASSERT_TRUE(viol.has_value());
    EXPECT_NE(viol->keyword.find("DROP"), std::string::npos);
}

TEST(AqlSafetyValidatorMutation, MultiStatementDeleteInjection) {
    AqlSafetyValidator v;
    auto viol = v.validateMutationSafety(
        "INSERT INTO items VALUES {id:1}; DELETE FROM items");
    ASSERT_TRUE(viol.has_value());
}

TEST(AqlSafetyValidatorMutation, LargeLimitBlocked) {
    AqlSafetyValidator v;
    auto viol = v.validateMutationSafety(
        "FOR u IN users REMOVE u IN users LIMIT 999999");
    ASSERT_TRUE(viol.has_value());
    EXPECT_EQ(viol->keyword, "LARGE_LIMIT");
}

TEST(AqlSafetyValidatorMutation, SafeQueryPassesAllChecks) {
    AqlSafetyValidator v;
    // Clean mutation with filter and small limit
    auto viol = v.validateMutationSafety(
        "UPDATE users SET active = 1 WHERE id == 42 LIMIT 1");
    EXPECT_FALSE(viol.has_value());
}

// ============================================================================
// SafetyValidator AllowMutations mode — 4 cases
// ============================================================================

TEST(AqlSafetyValidatorAllowMutations, ValidMutationPasses) {
    AqlSafetyValidator v(AqlSafetyValidator::ValidationMode::AllowMutations);
    // Valid mutation with filter — should pass
    auto viol = v.validate(std::string{"UPDATE users SET name = 'Bob' WHERE id == 1"});
    EXPECT_FALSE(viol.has_value());
}

TEST(AqlSafetyValidatorAllowMutations, NulByteBlockedInAllowMode) {
    AqlSafetyValidator v(AqlSafetyValidator::ValidationMode::AllowMutations);
    std::string q = "INSERT INTO users VALUES {name:'ok'}";
    q[5] = '\0';
    auto viol = v.validate(q);
    ASSERT_TRUE(viol.has_value());
    EXPECT_EQ(viol->keyword, "NUL_INJECTION");
}

TEST(AqlSafetyValidatorAllowMutations, MultiStatementInjectionBlockedInAllowMode) {
    AqlSafetyValidator v(AqlSafetyValidator::ValidationMode::AllowMutations);
    auto viol = v.validate(std::string{"INSERT INTO items VALUES {x:1}; DROP items"});
    ASSERT_TRUE(viol.has_value());
}

TEST(AqlSafetyValidatorAllowMutations, ReadOnlyModeStillBlocksMutations) {
    AqlSafetyValidator v(AqlSafetyValidator::ValidationMode::ReadOnly);
    auto viol = v.validate(std::string{"INSERT INTO users VALUES {name:'Alice'}"});
    ASSERT_TRUE(viol.has_value());
    EXPECT_EQ(viol->keyword, "INSERT");
}

// ============================================================================
// Edge cases — 4 cases
// ============================================================================

TEST(AqlMutationValidatorEdgeCases, CollectionNameExactly256Chars) {
    AqlMutationValidator v;
    // 256 chars: valid (starts with 'a', all alpha)
    std::string name(256, 'a');
    EXPECT_TRUE(v.isValidCollectionName(name));
}

TEST(AqlMutationValidatorEdgeCases, CollectionName257CharsInvalid) {
    AqlMutationValidator v;
    std::string name(257, 'a');
    EXPECT_FALSE(v.isValidCollectionName(name));
}

TEST(AqlMutationValidatorEdgeCases, FieldNameWithDeepDotPath) {
    AqlMutationValidator v;
    EXPECT_TRUE(v.isValidFieldName("a.b.c.d.e"));
}

TEST(AqlMutationValidatorEdgeCases, UnderscoreOnlyCollectionName) {
    AqlMutationValidator v;
    // Just "_" — valid: starts with underscore, no invalid chars, length 1
    EXPECT_TRUE(v.isValidCollectionName("_"));
}
