#include <gtest/gtest.h>
#include "query/aql_parser.h"
#include "query/let_evaluator.h"
#include <nlohmann/json.hpp>
#include <memory>

using namespace themis::query;
using json = nlohmann::json;

// ============================================================================
// Helpers
// ============================================================================

namespace {

/// Build a GeoJSON Point literal expression.
std::shared_ptr<Expression> makePointLiteral(double x, double y) {
    json pt;
    pt["type"]          = "Point";
    pt["coordinates"]   = json::array({x, y});
    return std::make_shared<LiteralExpr>(LiteralValue(pt));
}

/// Build a simple closed GeoJSON Polygon (box) literal expression.
/// Vertices (CCW): (minx,miny) → (maxx,miny) → (maxx,maxy) → (minx,maxy) → close.
std::shared_ptr<Expression> makePolygonLiteral(
    double minx, double miny, double maxx, double maxy)
{
    json ring = json::array({
        json::array({minx, miny}),
        json::array({maxx, miny}),
        json::array({maxx, maxy}),
        json::array({minx, maxy}),
        json::array({minx, miny})   // close ring
    });
    json poly;
    poly["type"]        = "Polygon";
    poly["coordinates"] = json::array({ring});
    return std::make_shared<LiteralExpr>(LiteralValue(poly));
}

/// Make a FunctionCallExpr for a named function.
template<typename... Args>
std::shared_ptr<FunctionCallExpr> makeCall(
    const std::string& name, Args&&... args)
{
    std::vector<std::shared_ptr<Expression>> argVec{std::forward<Args>(args)...};
    return std::make_shared<FunctionCallExpr>(name, std::move(argVec));
}

/// Evaluate a single expression against an empty document.
json eval(const std::shared_ptr<Expression>& expr) {
    LetEvaluator ev = {};
    return ev.evaluateExpression(expr, json::object());
}

} // anonymous namespace

// ============================================================================
// Suite 1 — Parser acceptance: FILTER context
// ============================================================================

class AQLGeoParserFilterTest : public ::testing::Test {
protected:
    AQLParser parser;
};

TEST_F(AQLGeoParserFilterTest, FilterAccepts_ST_Distance_LessThan) {
    // ST_Distance returning a numeric value compared against a literal.
    auto ast = parser.parse(
        "FOR doc IN places "
        "FILTER ST_Distance(doc.location, doc.reference) < 100.0 "
        "RETURN doc");

    ASSERT_TRUE(ast.has_value()) << ast.error().message();
    ASSERT_EQ((*ast)->filters.size(), 1u);

    // Outermost condition should be BinaryOp(Lt, FunctionCall, Literal)
    auto& cond = (*ast)->filters[0]->condition;
    ASSERT_EQ(cond->getType(), ASTNodeType::BinaryOp);
    auto bin = std::static_pointer_cast<BinaryOpExpr>(cond);
    EXPECT_EQ(bin->op, BinaryOperator::Lt);
    ASSERT_EQ(bin->left->getType(), ASTNodeType::FunctionCall);
    auto fn = std::static_pointer_cast<FunctionCallExpr>(bin->left);
    EXPECT_EQ(fn->name, "ST_Distance");
    EXPECT_EQ(fn->arguments.size(), 2u);
}

TEST_F(AQLGeoParserFilterTest, FilterAccepts_ST_Within) {
    auto ast = parser.parse(
        "FOR doc IN places "
        "FILTER ST_Within(doc.location, zone) "
        "RETURN doc");

    ASSERT_TRUE(ast.has_value()) << ast.error().message();
    ASSERT_EQ((*ast)->filters.size(), 1u);

    auto& cond = (*ast)->filters[0]->condition;
    ASSERT_EQ(cond->getType(), ASTNodeType::FunctionCall);
    auto fn = std::static_pointer_cast<FunctionCallExpr>(cond);
    EXPECT_EQ(fn->name, "ST_Within");
    EXPECT_EQ(fn->arguments.size(), 2u);
}

TEST_F(AQLGeoParserFilterTest, FilterAccepts_ST_Contains) {
    auto ast = parser.parse(
        "FOR doc IN regions "
        "FILTER ST_Contains(doc.boundary, targetPoint) "
        "RETURN doc");

    ASSERT_TRUE(ast.has_value()) << ast.error().message();
    ASSERT_EQ((*ast)->filters.size(), 1u);

    auto& cond = (*ast)->filters[0]->condition;
    ASSERT_EQ(cond->getType(), ASTNodeType::FunctionCall);
    auto fn = std::static_pointer_cast<FunctionCallExpr>(cond);
    EXPECT_EQ(fn->name, "ST_Contains");
    EXPECT_EQ(fn->arguments.size(), 2u);
}

TEST_F(AQLGeoParserFilterTest, FilterAccepts_ST_Intersects) {
    auto ast = parser.parse(
        "FOR doc IN features "
        "FILTER ST_Intersects(doc.geom, searchArea) "
        "RETURN doc");

    ASSERT_TRUE(ast.has_value()) << ast.error().message();
    ASSERT_EQ((*ast)->filters.size(), 1u);

    auto& cond = (*ast)->filters[0]->condition;
    ASSERT_EQ(cond->getType(), ASTNodeType::FunctionCall);
    auto fn = std::static_pointer_cast<FunctionCallExpr>(cond);
    EXPECT_EQ(fn->name, "ST_Intersects");
    EXPECT_EQ(fn->arguments.size(), 2u);
}

TEST_F(AQLGeoParserFilterTest, FilterAccepts_ST_DWithin) {
    auto ast = parser.parse(
        "FOR doc IN places "
        "FILTER ST_DWithin(doc.location, centerPoint, 500.0) "
        "RETURN doc");

    ASSERT_TRUE(ast.has_value()) << ast.error().message();
    ASSERT_EQ((*ast)->filters.size(), 1u);

    auto& cond = (*ast)->filters[0]->condition;
    ASSERT_EQ(cond->getType(), ASTNodeType::FunctionCall);
    auto fn = std::static_pointer_cast<FunctionCallExpr>(cond);
    EXPECT_EQ(fn->name, "ST_DWithin");
    EXPECT_EQ(fn->arguments.size(), 3u);
}

TEST_F(AQLGeoParserFilterTest, FilterAccepts_ST_GeomFromGeoJSON_Nested) {
    // ST_GeomFromGeoJSON as the first argument to ST_Within
    auto ast = parser.parse(
        "FOR doc IN features "
        "FILTER ST_Within(ST_GeomFromGeoJSON(doc.rawGeom), searchZone) "
        "RETURN doc");

    ASSERT_TRUE(ast.has_value()) << ast.error().message();
    ASSERT_EQ((*ast)->filters.size(), 1u);

    auto& cond = (*ast)->filters[0]->condition;
    ASSERT_EQ(cond->getType(), ASTNodeType::FunctionCall);
    auto outer = std::static_pointer_cast<FunctionCallExpr>(cond);
    EXPECT_EQ(outer->name, "ST_Within");
    ASSERT_EQ(outer->arguments.size(), 2u);

    // First argument is ST_GeomFromGeoJSON(...)
    ASSERT_EQ(outer->arguments[0]->getType(), ASTNodeType::FunctionCall);
    auto inner = std::static_pointer_cast<FunctionCallExpr>(outer->arguments[0]);
    EXPECT_EQ(inner->name, "ST_GeomFromGeoJSON");
    EXPECT_EQ(inner->arguments.size(), 1u);
}

TEST_F(AQLGeoParserFilterTest, FilterAccepts_ST_Within_CombinedWithAND) {
    // ST_Within combined with a scalar comparison via AND.
    auto ast = parser.parse(
        "FOR doc IN places "
        "FILTER ST_Within(doc.location, zone) AND doc.active == true "
        "RETURN doc");

    ASSERT_TRUE(ast.has_value()) << ast.error().message();
    ASSERT_EQ((*ast)->filters.size(), 1u);

    auto& cond = (*ast)->filters[0]->condition;
    ASSERT_EQ(cond->getType(), ASTNodeType::BinaryOp);
    auto bin = std::static_pointer_cast<BinaryOpExpr>(cond);
    EXPECT_EQ(bin->op, BinaryOperator::And);
    ASSERT_EQ(bin->left->getType(), ASTNodeType::FunctionCall);
    auto fn = std::static_pointer_cast<FunctionCallExpr>(bin->left);
    EXPECT_EQ(fn->name, "ST_Within");
}

// ============================================================================
// Suite 2 — Parser acceptance: SORT and RETURN contexts
// ============================================================================

class AQLGeoParserContextTest : public ::testing::Test {
protected:
    AQLParser parser;
};

TEST_F(AQLGeoParserContextTest, SortBy_ST_Distance) {
    auto ast = parser.parse(
        "FOR doc IN places "
        "SORT ST_Distance(doc.location, refPoint) ASC "
        "RETURN doc");

    ASSERT_TRUE(ast.has_value()) << ast.error().message();
    ASSERT_NE((*ast)->sort, nullptr);
    ASSERT_EQ((*ast)->sort->specifications.size(), 1u);

    auto& spec = (*ast)->sort->specifications[0];
    ASSERT_EQ(spec.expression->getType(), ASTNodeType::FunctionCall);
    auto fn = std::static_pointer_cast<FunctionCallExpr>(spec.expression);
    EXPECT_EQ(fn->name, "ST_Distance");
    EXPECT_EQ(fn->arguments.size(), 2u);
    EXPECT_TRUE(spec.ascending);
}

TEST_F(AQLGeoParserContextTest, ReturnWith_ST_AsGeoJSON) {
    auto ast = parser.parse(
        "FOR doc IN places "
        "RETURN ST_AsGeoJSON(doc.geom)");

    ASSERT_TRUE(ast.has_value()) << ast.error().message();
    ASSERT_NE((*ast)->return_node, nullptr);

    auto& retExpr = (*ast)->return_node->expression;
    ASSERT_EQ(retExpr->getType(), ASTNodeType::FunctionCall);
    auto fn = std::static_pointer_cast<FunctionCallExpr>(retExpr);
    EXPECT_EQ(fn->name, "ST_AsGeoJSON");
    EXPECT_EQ(fn->arguments.size(), 1u);
}

TEST_F(AQLGeoParserContextTest, LET_Plus_Filter_ST_Point) {
    // ST_Point used in LET, then the LET variable used in FILTER via ST_DWithin.
    auto ast = parser.parse(
        "FOR doc IN places "
        "LET center = ST_Point(13.405, 52.52) "
        "FILTER ST_DWithin(doc.location, center, 1000.0) "
        "RETURN doc");

    ASSERT_TRUE(ast.has_value()) << ast.error().message();
    ASSERT_EQ((*ast)->let_nodes.size(), 1u);
    ASSERT_EQ((*ast)->filters.size(), 1u);

    // LET has ST_Point call
    auto& letExpr = (*ast)->let_nodes[0].expression;
    ASSERT_EQ(letExpr->getType(), ASTNodeType::FunctionCall);
    EXPECT_EQ(std::static_pointer_cast<FunctionCallExpr>(letExpr)->name, "ST_Point");

    // FILTER has ST_DWithin call
    auto& filterCond = (*ast)->filters[0]->condition;
    ASSERT_EQ(filterCond->getType(), ASTNodeType::FunctionCall);
    EXPECT_EQ(std::static_pointer_cast<FunctionCallExpr>(filterCond)->name, "ST_DWithin");
}

// ============================================================================
// Suite 3 — Evaluation: ST_* functions produce correct results via
//           LetEvaluator::evaluateExpression() (no database required)
// ============================================================================

class AQLGeoFilterEvalTest : public ::testing::Test {
protected:
    LetEvaluator evaluator;
};

TEST_F(AQLGeoFilterEvalTest, STDistance_3_4_5_Triangle) {
    // Points (0,0) and (3,4) → Euclidean distance = 5.0
    auto expr = makeCall("ST_Distance",
        makePointLiteral(0.0, 0.0),
        makePointLiteral(3.0, 4.0));

    json result = eval(expr);
    ASSERT_TRUE(result.is_number());
    EXPECT_DOUBLE_EQ(result.get<double>(), 5.0);
}

TEST_F(AQLGeoFilterEvalTest, STDistance_SamePoint_IsZero) {
    auto expr = makeCall("ST_Distance",
        makePointLiteral(10.0, 20.0),
        makePointLiteral(10.0, 20.0));

    json result = eval(expr);
    ASSERT_TRUE(result.is_number());
    EXPECT_DOUBLE_EQ(result.get<double>(), 0.0);
}

TEST_F(AQLGeoFilterEvalTest, STWithin_PointInsideBox_ReturnsTrue) {
    // Unit box [0,0]→[10,10]; point (5,5) is inside.
    auto expr = makeCall("ST_Within",
        makePointLiteral(5.0, 5.0),
        makePolygonLiteral(0.0, 0.0, 10.0, 10.0));

    json result = eval(expr);
    ASSERT_TRUE(result.is_boolean());
    EXPECT_TRUE(result.get<bool>());
}

TEST_F(AQLGeoFilterEvalTest, STWithin_PointOutsideBox_ReturnsFalse) {
    // Unit box [0,0]→[10,10]; point (15,15) is outside.
    auto expr = makeCall("ST_Within",
        makePointLiteral(15.0, 15.0),
        makePolygonLiteral(0.0, 0.0, 10.0, 10.0));

    json result = eval(expr);
    ASSERT_TRUE(result.is_boolean());
    EXPECT_FALSE(result.get<bool>());
}

TEST_F(AQLGeoFilterEvalTest, STContains_BoxContainsPoint_ReturnsTrue) {
    // Box [0,0]→[10,10] MBR contains point (5,5) MBR.
    auto expr = makeCall("ST_Contains",
        makePolygonLiteral(0.0, 0.0, 10.0, 10.0),
        makePointLiteral(5.0, 5.0));

    json result = eval(expr);
    ASSERT_TRUE(result.is_boolean());
    EXPECT_TRUE(result.get<bool>());
}

TEST_F(AQLGeoFilterEvalTest, STContains_SmallBoxInsideLargeBox_ReturnsTrue) {
    // Large box [0,0]→[20,20] contains small box [5,5]→[15,15].
    auto expr = makeCall("ST_Contains",
        makePolygonLiteral(0.0, 0.0, 20.0, 20.0),
        makePolygonLiteral(5.0, 5.0, 15.0, 15.0));

    json result = eval(expr);
    ASSERT_TRUE(result.is_boolean());
    EXPECT_TRUE(result.get<bool>());
}

TEST_F(AQLGeoFilterEvalTest, STContains_BoxDoesNotContainFarPoint_ReturnsFalse) {
    auto expr = makeCall("ST_Contains",
        makePolygonLiteral(0.0, 0.0, 5.0, 5.0),
        makePointLiteral(10.0, 10.0));

    json result = eval(expr);
    ASSERT_TRUE(result.is_boolean());
    EXPECT_FALSE(result.get<bool>());
}

TEST_F(AQLGeoFilterEvalTest, STIntersects_SamePoint_ReturnsTrue) {
    // Two identical points intersect.
    auto expr = makeCall("ST_Intersects",
        makePointLiteral(3.0, 4.0),
        makePointLiteral(3.0, 4.0));

    json result = eval(expr);
    ASSERT_TRUE(result.is_boolean());
    EXPECT_TRUE(result.get<bool>());
}

TEST_F(AQLGeoFilterEvalTest, STIntersects_DifferentPoints_ReturnsFalse) {
    auto expr = makeCall("ST_Intersects",
        makePointLiteral(0.0, 0.0),
        makePointLiteral(1.0, 1.0));

    json result = eval(expr);
    ASSERT_TRUE(result.is_boolean());
    EXPECT_FALSE(result.get<bool>());
}

TEST_F(AQLGeoFilterEvalTest, STDWithin_NearbyPoint_ReturnsTrue) {
    // Points (0,0) and (0.3,0.4) → distance = 0.5 ≤ 1.0
    auto expr = makeCall("ST_DWithin",
        makePointLiteral(0.0, 0.0),
        makePointLiteral(0.3, 0.4),
        std::make_shared<LiteralExpr>(LiteralValue(1.0)));

    json result = eval(expr);
    ASSERT_TRUE(result.is_boolean());
    EXPECT_TRUE(result.get<bool>());
}

TEST_F(AQLGeoFilterEvalTest, STDWithin_FarPoint_ReturnsFalse) {
    // Points (0,0) and (30,40) → distance = 50 > 10
    auto expr = makeCall("ST_DWithin",
        makePointLiteral(0.0, 0.0),
        makePointLiteral(30.0, 40.0),
        std::make_shared<LiteralExpr>(LiteralValue(10.0)));

    json result = eval(expr);
    ASSERT_TRUE(result.is_boolean());
    EXPECT_FALSE(result.get<bool>());
}

TEST_F(AQLGeoFilterEvalTest, STGeomFromGeoJSON_JsonObjectPassthrough) {
    // Passing an existing GeoJSON object returns it unchanged.
    json pt;
    pt["type"]        = "Point";
    pt["coordinates"] = json::array({13.405, 52.52});
    auto expr = makeCall("ST_GeomFromGeoJSON",
        std::make_shared<LiteralExpr>(LiteralValue(pt)));

    json result = eval(expr);
    ASSERT_TRUE(result.is_object());
    EXPECT_EQ(result["type"], "Point");
    EXPECT_DOUBLE_EQ(result["coordinates"][0].get<double>(), 13.405);
    EXPECT_DOUBLE_EQ(result["coordinates"][1].get<double>(), 52.52);
}

TEST_F(AQLGeoFilterEvalTest, STGeomFromGeoJSON_JsonStringParsed) {
    // A JSON-serialised GeoJSON string is parsed back to an object.
    json pt;
    pt["type"]        = "Point";
    pt["coordinates"] = json::array({7.0, 51.0});
    std::string serialized = pt.dump();

    auto expr = makeCall("ST_GeomFromGeoJSON",
        std::make_shared<LiteralExpr>(LiteralValue(serialized)));

    json result = eval(expr);
    ASSERT_TRUE(result.is_object());
    EXPECT_EQ(result["type"], "Point");
    EXPECT_DOUBLE_EQ(result["coordinates"][0].get<double>(), 7.0);
}

// ============================================================================
// Suite 4 — Parse-then-evaluate: FILTER condition extracted from parsed AST
//           and evaluated with LetEvaluator against a concrete document.
// ============================================================================

class AQLGeoFilterParseEvalTest : public ::testing::Test {
protected:
    AQLParser parser;
    LetEvaluator evaluator;
};

TEST_F(AQLGeoFilterParseEvalTest, STDistance_FilterExpr_EvaluatesCorrectly) {
    // Query uses two field accesses; document provides both geometries.
    auto ast = parser.parse(
        "FOR doc IN places "
        "FILTER ST_Distance(doc.a, doc.b) < 6.0 "
        "RETURN doc");

    ASSERT_TRUE(ast.has_value()) << ast.error().message();
    auto& filterCond = (*ast)->filters[0]->condition;

    // Build document: a=(0,0), b=(3,4) → distance 5 < 6 → true
    json pt_a, pt_b;
    pt_a["type"]        = "Point"; pt_a["coordinates"] = json::array({0.0, 0.0});
    pt_b["type"]        = "Point"; pt_b["coordinates"] = json::array({3.0, 4.0});
    json doc{{"a", pt_a}, {"b", pt_b}};

    // Bind "doc" via the bindings map so the VariableExpr("doc") resolves.
    LetNode docBinding{"doc", std::make_shared<LiteralExpr>(LiteralValue(doc))};
    evaluator.evaluateLet(docBinding, json::object());

    json result = evaluator.evaluateExpression(filterCond, doc);
    ASSERT_TRUE(result.is_boolean());
    EXPECT_TRUE(result.get<bool>());
}

TEST_F(AQLGeoFilterParseEvalTest, STDWithin_FilterExpr_PointInRange) {
    // doc.location is within 1.0 units of doc.center.
    auto ast = parser.parse(
        "FOR doc IN places "
        "FILTER ST_DWithin(doc.location, doc.center, 1.0) "
        "RETURN doc");

    ASSERT_TRUE(ast.has_value()) << ast.error().message();
    auto& filterCond = (*ast)->filters[0]->condition;

    json loc, center;
    loc["type"]         = "Point"; loc["coordinates"]    = json::array({0.3, 0.4});
    center["type"]      = "Point"; center["coordinates"] = json::array({0.0, 0.0});
    json doc{{"location", loc}, {"center", center}};

    // distance(0.3,0.4, 0.0,0.0) = 0.5 ≤ 1.0 → true
    json result = evaluator.evaluateExpression(filterCond, doc);
    ASSERT_TRUE(result.is_boolean());
    EXPECT_TRUE(result.get<bool>());
}

TEST_F(AQLGeoFilterParseEvalTest, STDWithin_FilterExpr_PointOutOfRange) {
    auto ast = parser.parse(
        "FOR doc IN places "
        "FILTER ST_DWithin(doc.location, doc.center, 1.0) "
        "RETURN doc");

    ASSERT_TRUE(ast.has_value()) << ast.error().message();
    auto& filterCond = (*ast)->filters[0]->condition;

    json loc, center;
    loc["type"]         = "Point"; loc["coordinates"]    = json::array({30.0, 40.0});
    center["type"]      = "Point"; center["coordinates"] = json::array({0.0, 0.0});
    json doc{{"location", loc}, {"center", center}};

    // distance = 50 > 1.0 → false
    json result = evaluator.evaluateExpression(filterCond, doc);
    ASSERT_TRUE(result.is_boolean());
    EXPECT_FALSE(result.get<bool>());
}

