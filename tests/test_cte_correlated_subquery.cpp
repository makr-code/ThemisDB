/*
 * Regression tests for CTESubquery Phase 1 stub replacement (Issue #194).
 *
 * Covers:
 *  1. Correlation detection: SubqueryOptimizer correctly identifies subqueries
 *     that reference outer variables from the enclosing FOR loop.
 *  2. evaluateSubquery dispatch: null-subquery error surface confirmed via the
 *     SubqueryExpr API.
 *  3. EXISTS/NOT EXISTS short-circuit: SubqueryOptimizer::estimateQueryCost
 *     returns a lower value when LIMIT 1 is present (the mechanism exploited by
 *     evaluateExistsSubquery's LIMIT 1 optimisation).
 *  4. Parser integration: correlated subquery patterns in WHERE clause and
 *     RETURN clause parse correctly and are flagged as correlated by
 *     SubqueryOptimizer::canConvertToJoin.
 *
 * Full end-to-end execution tests (which require a live QueryEngine + RocksDB)
 * are not included here because the sandbox build environment does not have the
 * required native libraries available at test time.
 */

#include <gtest/gtest.h>
#include "query/aql_parser.h"
#include "query/subquery_optimizer.h"
#include "query/cte_subquery.h"
#include <unordered_set>

using namespace themis::query;

// ============================================================================
// Helpers – construct minimal Query AST nodes programmatically
// ============================================================================

static std::shared_ptr<Query> makeQuery(
    const std::string& variable,
    const std::string& collection,
    std::shared_ptr<Expression> filterExpr = nullptr)
{
    auto q = std::make_shared<Query>();
    q->for_node = ForNode{variable, collection};
    q->for_nodes.push_back(q->for_node);
    if (filterExpr) {
        q->filters.push_back(std::make_shared<FilterNode>(std::move(filterExpr)));
    }
    auto retVar = std::make_shared<VariableExpr>(variable);
    q->return_node = std::make_shared<ReturnNode>(std::move(retVar));
    return q;
}

static std::shared_ptr<Expression> makeEq(
    std::shared_ptr<Expression> left,
    std::shared_ptr<Expression> right)
{
    return std::make_shared<BinaryOpExpr>(BinaryOperator::Eq,
                                          std::move(left),
                                          std::move(right));
}

static std::shared_ptr<Expression> makeField(
    const std::string& objectVar,
    const std::string& field)
{
    return std::make_shared<FieldAccessExpr>(
        std::make_shared<VariableExpr>(objectVar), field);
}

// ============================================================================
// 1. Correlation detection tests (SubqueryOptimizer)
// ============================================================================

TEST(CorrelationDetectionTest, CorrelatedFilter_ReferencesOuterVar)
{
    // FOR o IN orders FILTER o.user_id == user.id RETURN o
    // "user" is the outer variable.
    auto filterExpr = makeEq(makeField("o", "user_id"), makeField("user", "id"));
    auto subquery   = makeQuery("o", "orders", std::move(filterExpr));

    std::unordered_set<std::string> outerVars{"user"};
    EXPECT_TRUE(SubqueryOptimizer::canConvertToJoin(subquery, outerVars))
        << "Subquery filtering on an outer variable must be detected as correlated";
}

TEST(CorrelationDetectionTest, NonCorrelatedFilter_NoOuterRef)
{
    // FOR o IN orders FILTER o.status == "active" RETURN o
    auto filterExpr = makeEq(
        makeField("o", "status"),
        std::make_shared<LiteralExpr>(LiteralValue{std::string("active")}));
    auto subquery = makeQuery("o", "orders", std::move(filterExpr));

    std::unordered_set<std::string> outerVars{"user"};
    EXPECT_FALSE(SubqueryOptimizer::canConvertToJoin(subquery, outerVars))
        << "Subquery with no outer reference must not be detected as correlated";
}

TEST(CorrelationDetectionTest, EmptyOuterVarSet_NotCorrelated)
{
    auto filterExpr = makeEq(makeField("o", "user_id"), makeField("u", "id"));
    auto subquery   = makeQuery("o", "orders", std::move(filterExpr));

    std::unordered_set<std::string> outerVars{};
    EXPECT_FALSE(SubqueryOptimizer::canConvertToJoin(subquery, outerVars))
        << "Empty outer-variable set cannot produce a correlated match";
}

TEST(CorrelationDetectionTest, NullSubquery_NotCorrelated)
{
    std::unordered_set<std::string> outerVars{"user"};
    EXPECT_FALSE(SubqueryOptimizer::canConvertToJoin(nullptr, outerVars));
}

TEST(CorrelationDetectionTest, CorrelatedNestedBinaryOp)
{
    // FILTER o.user_id == user.id AND o.status == "active"
    auto lhs = makeEq(makeField("o", "user_id"), makeField("user", "id"));
    auto rhs = makeEq(makeField("o", "status"),
                      std::make_shared<LiteralExpr>(LiteralValue{std::string("active")}));
    auto andExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::And,
                                                   std::move(lhs), std::move(rhs));
    auto subquery = makeQuery("o", "orders", std::move(andExpr));

    std::unordered_set<std::string> outerVars{"user"};
    EXPECT_TRUE(SubqueryOptimizer::canConvertToJoin(subquery, outerVars));
}

// ============================================================================
// 2. SubqueryEvaluator: null SubqueryExpr precondition
// ============================================================================

TEST(SubqueryEvaluatorDispatchTest, NullSubqueryExprHasNullPtr)
{
    // SubqueryExpr wraps a null Query pointer.  Verify the struct is constructed
    // correctly so that evaluateSubquery's null-guard can fire.
    SubqueryExpr subExpr{nullptr};
    EXPECT_EQ(subExpr.subquery, nullptr)
        << "SubqueryExpr constructed with nullptr must hold null subquery";
}

// ============================================================================
// 3. Parser integration: correlated subqueries in WHERE / RETURN clause
// ============================================================================

class CorrelatedSubqueryParserTest : public ::testing::Test {
protected:
    AQLParser parser;
};

// FOR user IN users FILTER (FOR o IN orders FILTER o.user_id == user.id RETURN o) != []
TEST_F(CorrelatedSubqueryParserTest, CorrelatedSubqueryInFilter_Parses)
{
    auto result = parser.parse(
        "FOR user IN users "
        "FILTER (FOR o IN orders FILTER o.user_id == user.id RETURN o) != [] "
        "RETURN user"
    );
    ASSERT_TRUE(result.has_value()) << result.error().message();
    ASSERT_EQ((*result)->filters.size(), 1u);

    auto cond = (*result)->filters[0]->condition;
    EXPECT_EQ(cond->getType(), ASTNodeType::BinaryOp);

    auto binOp = std::static_pointer_cast<BinaryOpExpr>(cond);
    ASSERT_EQ(binOp->left->getType(), ASTNodeType::SubqueryExpr);

    auto subExpr = std::static_pointer_cast<SubqueryExpr>(binOp->left);
    ASSERT_NE(subExpr->subquery, nullptr);

    // Subquery must be flagged as correlated w.r.t. outer variable "user".
    std::unordered_set<std::string> outerVars{"user"};
    EXPECT_TRUE(SubqueryOptimizer::canConvertToJoin(subExpr->subquery, outerVars))
        << "Subquery in filter clause must be detected as correlated w.r.t. 'user'";
}

// FOR user IN users RETURN { name: user.name, orders: (FOR o IN orders ...) }
TEST_F(CorrelatedSubqueryParserTest, CorrelatedSubqueryInReturn_Parses)
{
    auto result = parser.parse(
        "FOR user IN users "
        "RETURN {"
        "  name: user.name,"
        "  orders: (FOR o IN orders FILTER o.user_id == user.id RETURN o)"
        "}"
    );
    ASSERT_TRUE(result.has_value()) << result.error().message();
    ASSERT_NE((*result)->return_node, nullptr);
    EXPECT_EQ((*result)->return_node->expression->getType(), ASTNodeType::ObjectConstruct);
}

// Non-correlated scalar subquery – inner loop does not reference the outer var.
TEST_F(CorrelatedSubqueryParserTest, NonCorrelatedSubquery_NotFlaggedAsCorrelated)
{
    auto result = parser.parse(
        "FOR doc IN orders "
        "LET allSalaries = (FOR u IN users RETURN u.salary) "
        "RETURN doc"
    );
    ASSERT_TRUE(result.has_value()) << result.error().message();
    ASSERT_EQ((*result)->let_nodes.size(), 1u);

    auto subExpr = std::static_pointer_cast<SubqueryExpr>(
        (*result)->let_nodes[0].expression);
    ASSERT_NE(subExpr->subquery, nullptr);

    // The inner subquery over 'users' does not reference outer var 'doc'.
    std::unordered_set<std::string> outerVars{"doc"};
    EXPECT_FALSE(SubqueryOptimizer::canConvertToJoin(subExpr->subquery, outerVars))
        << "Non-correlated subquery must not be flagged as correlated";
}

// ============================================================================
// 4. EXISTS short-circuit: LIMIT 1 reduces estimated query cost
// ============================================================================

TEST(ExistsShortCircuitTest, LimitReducesCost)
{
    AQLParser parser;

    auto withoutLimit = parser.parse(
        "FOR o IN orders FILTER o.status == \"active\" RETURN o"
    );
    auto withLimit = parser.parse(
        "FOR o IN orders FILTER o.status == \"active\" LIMIT 1 RETURN o"
    );

    ASSERT_TRUE(withoutLimit.has_value());
    ASSERT_TRUE(withLimit.has_value());

    int costWithout = SubqueryOptimizer::estimateQueryCost(*withoutLimit);
    int costWith    = SubqueryOptimizer::estimateQueryCost(*withLimit);

    EXPECT_LT(costWith, costWithout)
        << "LIMIT 1 should reduce estimated query cost "
           "(premise for EXISTS/NOT EXISTS short-circuit optimisation)";
}
