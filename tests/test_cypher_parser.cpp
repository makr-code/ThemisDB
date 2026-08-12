// Unit tests for CypherParser and CypherToAQLTranspiler
// 30 tests covering MATCH, WHERE, RETURN, relationship patterns, AQL transpilation

#include <gtest/gtest.h>
#include "query/cypher_parser.h"

using namespace themis::query;

// ============================================================================
// Helpers
// ============================================================================

static CypherASTNode mustParse(const std::string& cypher) {
    CypherParser parser;
    auto result = parser.parse(cypher);
    EXPECT_TRUE(result.has_value())
        << "Expected successful parse but got: "
        << (!result ? result.error().message() : "");
    return result.value();
}

static std::string mustTranspile(const CypherASTNode& ast) {
    CypherToAQLTranspiler t;
    auto result = t.transpile(ast);
    EXPECT_TRUE(result.has_value())
        << "Expected successful transpile but got: "
        << (!result ? result.error().message() : "");
    return result.value();
}

static std::string cypherToAQL(const std::string& cypher) {
    return mustTranspile(mustParse(cypher));
}

static bool parseError(const std::string& cypher) {
    CypherParser parser;
    auto result = parser.parse(cypher);
    return !result.has_value();
}

// ============================================================================
// Section 1 – MATCH node patterns
// ============================================================================

// Test 1: Simple node with label
TEST(CypherParserFocusedTests, MatchNodeWithLabel) {
    auto ast = mustParse("MATCH (n:User) RETURN n");
    ASSERT_EQ(ast.match_patterns.size(), 1u);
    EXPECT_EQ(ast.match_patterns[0].start.variable, "n");
    ASSERT_EQ(ast.match_patterns[0].start.labels.size(), 1u);
    EXPECT_EQ(ast.match_patterns[0].start.labels[0], "User");
    EXPECT_FALSE(ast.return_items.empty());
}

// Test 2: Anonymous node (no variable)
TEST(CypherParserFocusedTests, MatchAnonymousNode) {
    auto ast = mustParse("MATCH (:User) RETURN 1");
    ASSERT_EQ(ast.match_patterns.size(), 1u);
    EXPECT_TRUE(ast.match_patterns[0].start.variable.empty());
    ASSERT_EQ(ast.match_patterns[0].start.labels.size(), 1u);
    EXPECT_EQ(ast.match_patterns[0].start.labels[0], "User");
}

// Test 3: Node with inline property filter
TEST(CypherParserFocusedTests, MatchNodeWithInlineProperty) {
    auto ast = mustParse("MATCH (n:User {name: \"Alice\"}) RETURN n");
    ASSERT_EQ(ast.match_patterns[0].start.properties.size(), 1u);
    EXPECT_EQ(ast.match_patterns[0].start.properties[0].key, "name");
}

// Test 4: Multiple labels on a node
TEST(CypherParserFocusedTests, MatchNodeMultipleLabels) {
    auto ast = mustParse("MATCH (n:Person:Employee) RETURN n");
    const auto& labels = ast.match_patterns[0].start.labels;
    ASSERT_EQ(labels.size(), 2u);
    EXPECT_EQ(labels[0], "Person");
    EXPECT_EQ(labels[1], "Employee");
}

// Test 5: Unlabeled node with variable
TEST(CypherParserFocusedTests, MatchUnlabeledNode) {
    auto ast = mustParse("MATCH (n) RETURN n");
    EXPECT_EQ(ast.match_patterns[0].start.variable, "n");
    EXPECT_TRUE(ast.match_patterns[0].start.labels.empty());
}

// ============================================================================
// Section 2 – Relationship patterns
// ============================================================================

// Test 6: Outbound relationship
TEST(CypherParserFocusedTests, RelationshipOutbound) {
    auto ast = mustParse("MATCH (a:User)-[:FRIEND]->(b:User) RETURN a, b");
    ASSERT_EQ(ast.match_patterns[0].segments.size(), 1u);
    const auto& seg = ast.match_patterns[0].segments[0];
    EXPECT_EQ(seg.rel.direction, CypherRelDirection::Out);
    ASSERT_EQ(seg.rel.types.size(), 1u);
    EXPECT_EQ(seg.rel.types[0], "FRIEND");
}

// Test 7: Inbound relationship
TEST(CypherParserFocusedTests, RelationshipInbound) {
    auto ast = mustParse("MATCH (a)<-[:FOLLOWS]-(b) RETURN a");
    const auto& seg = ast.match_patterns[0].segments[0];
    EXPECT_EQ(seg.rel.direction, CypherRelDirection::In);
    EXPECT_EQ(seg.rel.types[0], "FOLLOWS");
}

// Test 8: Undirected relationship
TEST(CypherParserFocusedTests, RelationshipUndirected) {
    auto ast = mustParse("MATCH (a)-[:KNOWS]-(b) RETURN a");
    const auto& seg = ast.match_patterns[0].segments[0];
    EXPECT_EQ(seg.rel.direction, CypherRelDirection::Both);
}

// Test 9: Variable-length relationship
TEST(CypherParserFocusedTests, RelationshipVariableLength) {
    auto ast = mustParse("MATCH (a)-[:FRIEND*1..3]->(b) RETURN b");
    const auto& seg = ast.match_patterns[0].segments[0];
    EXPECT_EQ(seg.rel.min_hops.value_or(-1), 1);
    EXPECT_EQ(seg.rel.max_hops.value_or(-1), 3);
}

// Test 10: Named relationship variable
TEST(CypherParserFocusedTests, RelationshipWithVariable) {
    auto ast = mustParse("MATCH (a)-[r:KNOWS]->(b) RETURN r");
    const auto& seg = ast.match_patterns[0].segments[0];
    EXPECT_EQ(seg.rel.variable, "r");
}

// Test 11: Anonymous relationship
TEST(CypherParserFocusedTests, RelationshipAnonymous) {
    auto ast = mustParse("MATCH (a)-->(b) RETURN b");
    ASSERT_EQ(ast.match_patterns[0].segments.size(), 1u);
    const auto& seg = ast.match_patterns[0].segments[0];
    EXPECT_EQ(seg.rel.direction, CypherRelDirection::Out);
    EXPECT_TRUE(seg.rel.variable.empty());
    EXPECT_TRUE(seg.rel.types.empty());
}

// ============================================================================
// Section 3 – WHERE expressions
// ============================================================================

// Test 12: Simple equality WHERE
TEST(CypherParserFocusedTests, WhereEquality) {
    auto ast = mustParse("MATCH (n:User) WHERE n.age = 30 RETURN n");
    ASSERT_NE(ast.where, nullptr);
    auto* binop = dynamic_cast<CypherBinaryOpExpr*>(ast.where.get());
    ASSERT_NE(binop, nullptr);
    EXPECT_EQ(binop->op, "=");
}

// Test 13: WHERE with AND
TEST(CypherParserFocusedTests, WhereAndOr) {
    auto ast = mustParse("MATCH (n) WHERE n.a = 1 AND n.b = 2 RETURN n");
    ASSERT_NE(ast.where, nullptr);
    auto* binop = dynamic_cast<CypherBinaryOpExpr*>(ast.where.get());
    ASSERT_NE(binop, nullptr);
    EXPECT_EQ(binop->op, "AND");
}

// Test 14: WHERE NOT
TEST(CypherParserFocusedTests, WhereNot) {
    auto ast = mustParse("MATCH (n) WHERE NOT n.active = true RETURN n");
    ASSERT_NE(ast.where, nullptr);
    auto* unop = dynamic_cast<CypherUnaryOpExpr*>(ast.where.get());
    ASSERT_NE(unop, nullptr);
    EXPECT_EQ(unop->op, "NOT");
}

// Test 15: WHERE IS NULL
TEST(CypherParserFocusedTests, WhereIsNull) {
    auto ast = mustParse("MATCH (n) WHERE n.email IS NULL RETURN n");
    ASSERT_NE(ast.where, nullptr);
    auto* unop = dynamic_cast<CypherUnaryOpExpr*>(ast.where.get());
    ASSERT_NE(unop, nullptr);
    EXPECT_EQ(unop->op, "IS NULL");
}

// Test 16: WHERE IS NOT NULL
TEST(CypherParserFocusedTests, WhereIsNotNull) {
    auto ast = mustParse("MATCH (n) WHERE n.email IS NOT NULL RETURN n");
    ASSERT_NE(ast.where, nullptr);
    auto* unop = dynamic_cast<CypherUnaryOpExpr*>(ast.where.get());
    ASSERT_NE(unop, nullptr);
    EXPECT_EQ(unop->op, "IS NOT NULL");
}

// Test 17: WHERE STARTS WITH
TEST(CypherParserFocusedTests, WhereStartsWith) {
    auto ast = mustParse("MATCH (n:User) WHERE n.name STARTS WITH \"Al\" RETURN n");
    ASSERT_NE(ast.where, nullptr);
    auto* binop = dynamic_cast<CypherBinaryOpExpr*>(ast.where.get());
    ASSERT_NE(binop, nullptr);
    EXPECT_EQ(binop->op, "STARTS WITH");
}

// Test 18: WHERE CONTAINS
TEST(CypherParserFocusedTests, WhereContains) {
    auto ast = mustParse("MATCH (n) WHERE n.bio CONTAINS \"developer\" RETURN n");
    auto* binop = dynamic_cast<CypherBinaryOpExpr*>(ast.where.get());
    ASSERT_NE(binop, nullptr);
    EXPECT_EQ(binop->op, "CONTAINS");
}

// Test 19: WHERE IN list
TEST(CypherParserFocusedTests, WhereIn) {
    auto ast = mustParse("MATCH (n) WHERE n.role IN [\"admin\", \"mod\"] RETURN n");
    auto* binop = dynamic_cast<CypherBinaryOpExpr*>(ast.where.get());
    ASSERT_NE(binop, nullptr);
    EXPECT_EQ(binop->op, "IN");
}

// ============================================================================
// Section 4 – RETURN clause variants
// ============================================================================

// Test 20: RETURN *
TEST(CypherParserFocusedTests, ReturnStar) {
    auto ast = mustParse("MATCH (n:User) RETURN *");
    ASSERT_FALSE(ast.return_items.empty());
    EXPECT_TRUE(ast.return_items[0].star);
}

// Test 21: RETURN DISTINCT
TEST(CypherParserFocusedTests, ReturnDistinct) {
    auto ast = mustParse("MATCH (n:User) RETURN DISTINCT n.name");
    EXPECT_TRUE(ast.return_distinct);
}

// Test 22: RETURN with AS alias
TEST(CypherParserFocusedTests, ReturnWithAlias) {
    auto ast = mustParse("MATCH (n:User) RETURN n.name AS username");
    ASSERT_FALSE(ast.return_items.empty());
    EXPECT_EQ(ast.return_items[0].alias, "username");
}

// Test 23: ORDER BY ASC / DESC
TEST(CypherParserFocusedTests, OrderBy) {
    auto ast = mustParse("MATCH (n:User) RETURN n ORDER BY n.age DESC");
    ASSERT_FALSE(ast.order_by.empty());
    EXPECT_FALSE(ast.order_by[0].ascending);
}

// Test 24: SKIP and LIMIT
TEST(CypherParserFocusedTests, SkipAndLimit) {
    auto ast = mustParse("MATCH (n:User) RETURN n SKIP 10 LIMIT 5");
    EXPECT_EQ(ast.skip.value_or(-1), 10);
    EXPECT_EQ(ast.limit.value_or(-1), 5);
}

// ============================================================================
// Section 5 – AQL transpilation
// ============================================================================

// Test 25: Simple node → FOR … IN … RETURN
TEST(CypherParserFocusedTests, TranspileSimpleNode) {
    std::string aql = cypherToAQL("MATCH (n:User) RETURN n");
    EXPECT_NE(aql.find("FOR n IN User"), std::string::npos);
    EXPECT_NE(aql.find("RETURN"), std::string::npos);
}

// Test 26: Node with WHERE → FILTER
TEST(CypherParserFocusedTests, TranspileWhereToFilter) {
    std::string aql = cypherToAQL("MATCH (n:User) WHERE n.age > 18 RETURN n.name");
    EXPECT_NE(aql.find("FILTER"), std::string::npos);
    EXPECT_NE(aql.find("n.age"), std::string::npos);
}

// Test 27: Relationship → graph traversal AQL
TEST(CypherParserFocusedTests, TranspileRelationshipToTraversal) {
    std::string aql = cypherToAQL("MATCH (a:User)-[:FRIEND]->(b:User) RETURN a, b");
    EXPECT_NE(aql.find("OUTBOUND"), std::string::npos);
}

// Test 28: STARTS WITH → STARTS_WITH() in AQL
TEST(CypherParserFocusedTests, TranspileStartsWith) {
    std::string aql = cypherToAQL("MATCH (n:User) WHERE n.name STARTS WITH \"Al\" RETURN n");
    EXPECT_NE(aql.find("STARTS_WITH"), std::string::npos);
}

// Test 29: ORDER BY / LIMIT in transpiled AQL
TEST(CypherParserFocusedTests, TranspileOrderByLimit) {
    std::string aql = cypherToAQL("MATCH (n:User) RETURN n ORDER BY n.name ASC LIMIT 10");
    EXPECT_NE(aql.find("SORT"), std::string::npos);
    EXPECT_NE(aql.find("LIMIT"), std::string::npos);
}

// ============================================================================
// Section 6 – Error cases
// ============================================================================

// Test 30: Missing MATCH → parse error
TEST(CypherParserFocusedTests, MissingMatchIsError) {
    EXPECT_TRUE(parseError("RETURN n"));
}

// ============================================================================
// Section 7 – Hop-count validation (issue #5177)
// ============================================================================

// Test 31: Valid variable-length relationship is accepted
TEST(CypherParserFocusedTests, ValidHopRange) {
    EXPECT_FALSE(parseError("MATCH (a)-[*1..3]->(b) RETURN a, b"));
}

// Test 32: Negative min_hops must be rejected
TEST(CypherParserFocusedTests, NegativeMinHopsIsError) {
    EXPECT_TRUE(parseError("MATCH (a)-[*-1..3]->(b) RETURN a, b"));
}

// Test 33: Hop count exceeding kMaxHops (1000) must be rejected
TEST(CypherParserFocusedTests, ExcessiveHopCountIsError) {
    EXPECT_TRUE(parseError("MATCH (a)-[*1..1001]->(b) RETURN a, b"));
}

// Test 34: Max hop equal to kMaxHops boundary (1000) is accepted
TEST(CypherParserFocusedTests, HopCountAtBoundaryIsAccepted) {
    EXPECT_FALSE(parseError("MATCH (a)-[*1..1000]->(b) RETURN a, b"));
}

// ============================================================================
// Section 8 – Numeric overflow guards (REL-10..12, issue #5177)
// ============================================================================

// Test 35: SKIP with an integer value that exceeds int64 range is rejected
TEST(CypherParserFocusedTests, SkipOverflowIsError) {
    // "99999999999999999999" is larger than INT64_MAX → must be a parse error
    EXPECT_TRUE(parseError("MATCH (n:User) RETURN n SKIP 99999999999999999999 LIMIT 10"));
}

// Test 36: LIMIT with an out-of-range integer is rejected
TEST(CypherParserFocusedTests, LimitOverflowIsError) {
    EXPECT_TRUE(parseError("MATCH (n:User) RETURN n LIMIT 99999999999999999999"));
}

// Test 37: Integer literal overflow in a WHERE expression is rejected
TEST(CypherParserFocusedTests, IntLiteralOverflowIsError) {
    EXPECT_TRUE(parseError("MATCH (n:User) WHERE n.id = 99999999999999999999 RETURN n"));
}

// Test 38: Float literal overflow in a WHERE expression is rejected
TEST(CypherParserFocusedTests, FloatLiteralOverflowIsError) {
    // 1e99999 exceeds double range and must be rejected
    EXPECT_TRUE(parseError("MATCH (n:User) WHERE n.score = 1e99999 RETURN n"));
}

// Test 39: Hop count with an out-of-range integer is rejected
TEST(CypherParserFocusedTests, HopCountOverflowIsError) {
    EXPECT_TRUE(parseError("MATCH (a)-[*99999999999999999999..99999999999999999999]->(b) RETURN a, b"));
}

// Test 40: Valid SKIP/LIMIT are still accepted after adding the guard
TEST(CypherParserFocusedTests, ValidSkipLimitStillAccepted) {
    EXPECT_FALSE(parseError("MATCH (n:User) RETURN n SKIP 10 LIMIT 50"));
}
