// Unit tests for GremlinParser and GremlinToAQLTranspiler
// (Gremlin dialect compatibility layer – traversal parsing and AQL transpilation)

#include <gtest/gtest.h>
#include "query/gremlin_parser.h"

using namespace themis::query;

// ============================================================================
// Helpers
// ============================================================================

static GremlinASTNode mustParse(const std::string& gremlin) {
    GremlinParser parser;
    auto result = parser.parse(gremlin);
    EXPECT_TRUE(result.has_value()) << (!result ? result.error().message() : "");
    return result.value();
}

static std::string mustTranspile(const GremlinASTNode& ast) {
    GremlinToAQLTranspiler t;
    auto result = t.transpile(ast);
    EXPECT_TRUE(result.has_value()) << (!result ? result.error().message() : "");
    return result.value();
}

static std::string gremlinToAQL(const std::string& gremlin) {
    return mustTranspile(mustParse(gremlin));
}

// ============================================================================
// GremlinParser – basic traversal start
// ============================================================================

TEST(GremlinParserTest, VertexStart) {
    auto ast = mustParse("g.V()");
    ASSERT_EQ(ast.steps.size(), 1u);
    EXPECT_EQ(ast.steps[0].kind, GremlinStepKind::V);
    EXPECT_TRUE(ast.steps[0].values.empty());
}

TEST(GremlinParserTest, VertexStartWithId) {
    auto ast = mustParse("g.V('alice')");
    ASSERT_EQ(ast.steps[0].kind, GremlinStepKind::V);
    ASSERT_EQ(ast.steps[0].values.size(), 1u);
    EXPECT_EQ(std::get<std::string>(ast.steps[0].values[0]), "alice");
}

TEST(GremlinParserTest, VertexStartWithIntId) {
    auto ast = mustParse("g.V(42)");
    ASSERT_EQ(ast.steps[0].kind, GremlinStepKind::V);
    ASSERT_EQ(ast.steps[0].values.size(), 1u);
    EXPECT_EQ(std::get<int64_t>(ast.steps[0].values[0]), 42);
}

TEST(GremlinParserTest, EdgeStart) {
    auto ast = mustParse("g.E()");
    ASSERT_EQ(ast.steps.size(), 1u);
    EXPECT_EQ(ast.steps[0].kind, GremlinStepKind::E);
}

// ============================================================================
// GremlinParser – filter steps
// ============================================================================

TEST(GremlinParserTest, HasLabel) {
    auto ast = mustParse("g.V().hasLabel('User')");
    ASSERT_EQ(ast.steps.size(), 2u);
    EXPECT_EQ(ast.steps[1].kind, GremlinStepKind::HasLabel);
    ASSERT_EQ(ast.steps[1].strings.size(), 1u);
    EXPECT_EQ(ast.steps[1].strings[0], "User");
}

TEST(GremlinParserTest, HasKeyValue) {
    auto ast = mustParse("g.V().has('name', 'Alice')");
    ASSERT_EQ(ast.steps.size(), 2u);
    EXPECT_EQ(ast.steps[1].kind, GremlinStepKind::Has);
    ASSERT_EQ(ast.steps[1].strings.size(), 1u);
    EXPECT_EQ(ast.steps[1].strings[0], "name");
    ASSERT_EQ(ast.steps[1].values.size(), 1u);
    EXPECT_EQ(std::get<std::string>(ast.steps[1].values[0]), "Alice");
}

TEST(GremlinParserTest, HasWithEqPredicate) {
    auto ast = mustParse("g.V().has('age', P.eq(30))");
    ASSERT_EQ(ast.steps[1].kind, GremlinStepKind::Has);
    ASSERT_TRUE(ast.steps[1].predicate.has_value());
    EXPECT_EQ(ast.steps[1].predicate->op, GremlinPredOp::Eq);
    EXPECT_EQ(std::get<int64_t>(ast.steps[1].predicate->values[0]), 30);
}

TEST(GremlinParserTest, HasWithGtPredicate) {
    auto ast = mustParse("g.V().has('age', P.gt(18))");
    ASSERT_TRUE(ast.steps[1].predicate.has_value());
    EXPECT_EQ(ast.steps[1].predicate->op, GremlinPredOp::Gt);
}

TEST(GremlinParserTest, HasWithLtePredicate) {
    auto ast = mustParse("g.V().has('score', P.lte(100))");
    ASSERT_TRUE(ast.steps[1].predicate.has_value());
    EXPECT_EQ(ast.steps[1].predicate->op, GremlinPredOp::Lte);
}

TEST(GremlinParserTest, HasWithBarePredicate) {
    auto ast = mustParse("g.V().has('age', gt(25))");
    ASSERT_TRUE(ast.steps[1].predicate.has_value());
    EXPECT_EQ(ast.steps[1].predicate->op, GremlinPredOp::Gt);
}

TEST(GremlinParserTest, HasWithinPredicate) {
    auto ast = mustParse("g.V().has('role', P.within('admin', 'mod'))");
    ASSERT_TRUE(ast.steps[1].predicate.has_value());
    EXPECT_EQ(ast.steps[1].predicate->op, GremlinPredOp::Within);
    EXPECT_EQ(ast.steps[1].predicate->values.size(), 2u);
}

TEST(GremlinParserTest, HasNot) {
    auto ast = mustParse("g.V().hasNot('deleted')");
    ASSERT_EQ(ast.steps[1].kind, GremlinStepKind::HasNot);
    EXPECT_EQ(ast.steps[1].strings[0], "deleted");
}

// ============================================================================
// GremlinParser – traversal steps
// ============================================================================

TEST(GremlinParserTest, OutStep) {
    auto ast = mustParse("g.V().out('FRIEND')");
    EXPECT_EQ(ast.steps[1].kind, GremlinStepKind::Out);
    EXPECT_EQ(ast.steps[1].strings[0], "FRIEND");
}

TEST(GremlinParserTest, InStep) {
    auto ast = mustParse("g.V().in('FOLLOWS')");
    EXPECT_EQ(ast.steps[1].kind, GremlinStepKind::In);
}

TEST(GremlinParserTest, BothStep) {
    auto ast = mustParse("g.V().both('KNOWS')");
    EXPECT_EQ(ast.steps[1].kind, GremlinStepKind::Both);
}

TEST(GremlinParserTest, OutNoEdgeLabel) {
    auto ast = mustParse("g.V().out()");
    EXPECT_EQ(ast.steps[1].kind, GremlinStepKind::Out);
    EXPECT_TRUE(ast.steps[1].strings.empty());
}

TEST(GremlinParserTest, ValuesStep) {
    auto ast = mustParse("g.V().values('name')");
    EXPECT_EQ(ast.steps[1].kind, GremlinStepKind::Values);
    EXPECT_EQ(ast.steps[1].strings[0], "name");
}

TEST(GremlinParserTest, ValueMapStep) {
    auto ast = mustParse("g.V().valueMap()");
    EXPECT_EQ(ast.steps[1].kind, GremlinStepKind::ValueMap);
}

TEST(GremlinParserTest, AsStep) {
    auto ast = mustParse("g.V().as('x')");
    EXPECT_EQ(ast.steps[1].kind, GremlinStepKind::As);
    EXPECT_EQ(ast.steps[1].strings[0], "x");
}

TEST(GremlinParserTest, SelectStep) {
    auto ast = mustParse("g.V().as('a').out().as('b').select('a', 'b')");
    auto& sel = ast.steps.back();
    EXPECT_EQ(sel.kind, GremlinStepKind::Select);
    ASSERT_EQ(sel.strings.size(), 2u);
    EXPECT_EQ(sel.strings[0], "a");
    EXPECT_EQ(sel.strings[1], "b");
}

TEST(GremlinParserTest, DedupStep) {
    auto ast = mustParse("g.V().dedup()");
    EXPECT_EQ(ast.steps[1].kind, GremlinStepKind::Dedup);
}

TEST(GremlinParserTest, CountStep) {
    auto ast = mustParse("g.V().count()");
    EXPECT_EQ(ast.steps[1].kind, GremlinStepKind::Count);
}

TEST(GremlinParserTest, LimitStep) {
    auto ast = mustParse("g.V().limit(10)");
    EXPECT_EQ(ast.steps[1].kind, GremlinStepKind::Limit);
    ASSERT_TRUE(ast.steps[1].count.has_value());
    EXPECT_EQ(*ast.steps[1].count, 10);
}

TEST(GremlinParserTest, RangeStep) {
    auto ast = mustParse("g.V().range(5, 15)");
    EXPECT_EQ(ast.steps[1].kind, GremlinStepKind::Range);
    ASSERT_TRUE(ast.steps[1].count.has_value());
    ASSERT_TRUE(ast.steps[1].count2.has_value());
    EXPECT_EQ(*ast.steps[1].count, 5);
    EXPECT_EQ(*ast.steps[1].count2, 15);
}

TEST(GremlinParserTest, OrderByStep) {
    auto ast = mustParse("g.V().order().by('name')");
    EXPECT_EQ(ast.steps[1].kind, GremlinStepKind::Order);
    EXPECT_EQ(ast.steps[2].kind, GremlinStepKind::By);
    EXPECT_EQ(ast.steps[2].strings[0], "name");
    EXPECT_TRUE(ast.steps[2].ascending);
}

TEST(GremlinParserTest, OrderByDescStep) {
    auto ast = mustParse("g.V().order().by('age', Order.decr)");
    EXPECT_EQ(ast.steps[2].kind, GremlinStepKind::By);
    EXPECT_FALSE(ast.steps[2].ascending);
}

TEST(GremlinParserTest, IdStep) {
    auto ast = mustParse("g.V().id()");
    EXPECT_EQ(ast.steps[1].kind, GremlinStepKind::Id);
}

TEST(GremlinParserTest, LabelStep) {
    auto ast = mustParse("g.V().label()");
    EXPECT_EQ(ast.steps[1].kind, GremlinStepKind::Label);
}

// ============================================================================
// GremlinParser – error cases
// ============================================================================

TEST(GremlinParserTest, ErrorNotStartingWithG) {
    GremlinParser parser;
    auto result = parser.parse("V().hasLabel('User')");
    EXPECT_FALSE(result.has_value());
}

TEST(GremlinParserTest, ErrorStartingWithMethod) {
    GremlinParser parser;
    auto result = parser.parse("g.hasLabel('User')");
    EXPECT_FALSE(result.has_value());
}

TEST(GremlinParserTest, ErrorUnknownStep) {
    GremlinParser parser;
    auto result = parser.parse("g.V().unknownStep()");
    EXPECT_FALSE(result.has_value());
}

TEST(GremlinParserTest, ErrorEmptyQuery) {
    GremlinParser parser;
    auto result = parser.parse("");
    EXPECT_FALSE(result.has_value());
}

TEST(GremlinParserTest, ParseErrorDoesNotThrow) {
    GremlinParser parser;
    EXPECT_NO_THROW({
        auto r = parser.parse("g.V(.hasLabel('User')"); // malformed
        (void)r;
    });
}

// ============================================================================
// GremlinToAQLTranspiler – basic
// ============================================================================

TEST(GremlinTranspilerTest, AllVertices) {
    std::string aql = gremlinToAQL("g.V()");
    EXPECT_NE(aql.find("FOR _v IN"), std::string::npos);
    EXPECT_NE(aql.find("RETURN _v"), std::string::npos);
}

TEST(GremlinTranspilerTest, VertexById) {
    std::string aql = gremlinToAQL("g.V('alice')");
    EXPECT_NE(aql.find("FILTER"), std::string::npos);
    EXPECT_NE(aql.find("\"alice\""), std::string::npos);
}

TEST(GremlinTranspilerTest, HasLabelFilter) {
    std::string aql = gremlinToAQL("g.V().hasLabel('User')");
    EXPECT_NE(aql.find("FOR _v IN User"), std::string::npos);
    EXPECT_NE(aql.find("RETURN _v"), std::string::npos);
}

TEST(GremlinTranspilerTest, HasPropertyFilter) {
    std::string aql = gremlinToAQL("g.V().hasLabel('User').has('name', 'Alice')");
    EXPECT_NE(aql.find("FOR _v IN User"), std::string::npos);
    EXPECT_NE(aql.find("FILTER _v.name == \"Alice\""), std::string::npos);
}

TEST(GremlinTranspilerTest, HasPredicateGt) {
    std::string aql = gremlinToAQL("g.V().hasLabel('User').has('age', P.gt(18))");
    EXPECT_NE(aql.find("_v.age > 18"), std::string::npos);
}

TEST(GremlinTranspilerTest, HasPredicateLte) {
    std::string aql = gremlinToAQL("g.V().has('score', P.lte(100))");
    EXPECT_NE(aql.find("_v.score <= 100"), std::string::npos);
}

TEST(GremlinTranspilerTest, HasWithinPredicate) {
    std::string aql = gremlinToAQL("g.V().has('role', P.within('admin', 'mod'))");
    EXPECT_NE(aql.find(" IN "), std::string::npos);
    EXPECT_NE(aql.find("\"admin\""), std::string::npos);
}

TEST(GremlinTranspilerTest, HasNotFilter) {
    std::string aql = gremlinToAQL("g.V().hasNot('deleted')");
    EXPECT_NE(aql.find("FILTER"), std::string::npos);
    EXPECT_NE(aql.find("deleted"), std::string::npos);
}

TEST(GremlinTranspilerTest, OutTraversal) {
    std::string aql = gremlinToAQL("g.V().hasLabel('User').out('FRIEND')");
    EXPECT_NE(aql.find("OUTBOUND"), std::string::npos);
    EXPECT_NE(aql.find("FRIEND"), std::string::npos);
}

TEST(GremlinTranspilerTest, InTraversal) {
    std::string aql = gremlinToAQL("g.V().hasLabel('User').in('FOLLOWS')");
    EXPECT_NE(aql.find("INBOUND"), std::string::npos);
    EXPECT_NE(aql.find("FOLLOWS"), std::string::npos);
}

TEST(GremlinTranspilerTest, BothTraversal) {
    std::string aql = gremlinToAQL("g.V().hasLabel('User').both('KNOWS')");
    EXPECT_NE(aql.find("ANY"), std::string::npos);
}

TEST(GremlinTranspilerTest, ValuesProjection) {
    std::string aql = gremlinToAQL("g.V().hasLabel('User').values('name')");
    EXPECT_NE(aql.find(".name"), std::string::npos);
}

TEST(GremlinTranspilerTest, LimitClause) {
    std::string aql = gremlinToAQL("g.V().hasLabel('User').limit(5)");
    EXPECT_NE(aql.find("LIMIT 5"), std::string::npos);
}

TEST(GremlinTranspilerTest, RangeClause) {
    std::string aql = gremlinToAQL("g.V().range(10, 20)");
    EXPECT_NE(aql.find("LIMIT 10,"), std::string::npos);
}

TEST(GremlinTranspilerTest, CountWrapped) {
    std::string aql = gremlinToAQL("g.V().hasLabel('User').count()");
    EXPECT_NE(aql.find("LENGTH("), std::string::npos);
}

TEST(GremlinTranspilerTest, DedupDistinct) {
    std::string aql = gremlinToAQL("g.V().hasLabel('User').dedup()");
    EXPECT_NE(aql.find("DISTINCT"), std::string::npos);
}

TEST(GremlinTranspilerTest, OrderByAsc) {
    std::string aql = gremlinToAQL("g.V().hasLabel('User').order().by('name')");
    EXPECT_NE(aql.find("SORT"), std::string::npos);
    EXPECT_NE(aql.find("ASC"), std::string::npos);
}

TEST(GremlinTranspilerTest, IdProjection) {
    std::string aql = gremlinToAQL("g.V().hasLabel('User').id()");
    EXPECT_NE(aql.find("._key"), std::string::npos);
}

TEST(GremlinTranspilerTest, EmptyASTIsError) {
    GremlinASTNode ast;
    GremlinToAQLTranspiler t;
    auto result = t.transpile(ast);
    EXPECT_FALSE(result.has_value());
}

TEST(GremlinTranspilerTest, ChainedTraversal) {
    std::string aql = gremlinToAQL(
        "g.V().hasLabel('User').has('name', 'Alice').out('FRIEND').values('name')");
    EXPECT_NE(aql.find("FOR _v IN User"), std::string::npos);
    EXPECT_NE(aql.find("FILTER _v.name == \"Alice\""), std::string::npos);
    EXPECT_NE(aql.find("OUTBOUND"), std::string::npos);
    EXPECT_NE(aql.find("FRIEND"), std::string::npos);
    EXPECT_NE(aql.find(".name"), std::string::npos);
}

// ============================================================================
// GremlinParser – Numeric overflow guards (REL-19, issue #5177)
// ============================================================================

static bool gremlinParseError(const std::string& gremlin) {
    GremlinParser parser;
    auto result = parser.parse(gremlin);
    return !result.has_value();
}

// limit() with out-of-range integer is rejected
TEST(GremlinParserTest, LimitOverflowIsError) {
    EXPECT_TRUE(gremlinParseError("g.V().hasLabel('User').limit(99999999999999999999)"));
}

// range() with an out-of-range start value is rejected
TEST(GremlinParserTest, RangeStartOverflowIsError) {
    EXPECT_TRUE(gremlinParseError("g.V().hasLabel('User').range(99999999999999999999, 10)"));
}

// range() with an out-of-range end value is rejected
TEST(GremlinParserTest, RangeEndOverflowIsError) {
    EXPECT_TRUE(gremlinParseError("g.V().hasLabel('User').range(0, 99999999999999999999)"));
}

// V() vertex-ID lookup with an out-of-range integer is rejected
TEST(GremlinParserTest, VertexIdOverflowIsError) {
    EXPECT_TRUE(gremlinParseError("g.V(99999999999999999999)"));
}

// Integer literal overflow in a has() predicate value is rejected
TEST(GremlinParserTest, HasValueIntOverflowIsError) {
    EXPECT_TRUE(gremlinParseError("g.V().has('age', 99999999999999999999)"));
}

// Valid limit / range are still accepted after adding the guard
TEST(GremlinParserTest, ValidLimitAndRangeStillAccepted) {
    EXPECT_FALSE(gremlinParseError("g.V().hasLabel('User').limit(10)"));
    EXPECT_FALSE(gremlinParseError("g.V().hasLabel('User').range(0, 10)"));
}
