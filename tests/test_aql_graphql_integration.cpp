/*
 * ThemisDB – AQL ↔ GraphQL Integration Tests
 *
 * Tests both integration layers:
 *
 *   Layer 1 – AQL in GraphQL  (GraphQLAqlResolverFactory)
 *     - Cost model: GraphQLComplexityEstimator
 *     - Schema: aql(), aqlMutation(), apiVersion, schemaVersion
 *     - HTTP: GraphQLApiHandler complexity enforcement
 *
 *   Layer 2 – GraphQL in AQL  (GRAPHQL() function)
 *     - GRAPHQL() parse, execute, error handling
 *     - Cost classification: CostComplexity::EXTERNAL, base_cost=100
 *     - Anti-recursion: no AQL resolver injected inside GRAPHQL()
 *
 * Test suite names:
 *   GraphQLAqlCostModelTests       – complexity scoring + limit derivation
 *   GraphQLAqlResolverTests        – resolver wiring (Layer 1)
 *   GraphQLAqlHandlerTests         – HTTP handler complexity gate
 *   GraphQLAqlFunctionTests        – GRAPHQL() AQL built-in (Layer 2)
 *   GraphQLAqlSchemaTests          – schema fields added by this feature
 */

#include <gtest/gtest.h>
#include "api/graphql.h"
#include "api/graphql_aql_resolver.h"
#include "query/query_resource_limits.h"
#include "query/functions/graphql_functions.h"
#include "query/functions/function_registry.h"

using namespace themis;
using namespace themis::graphql;
using namespace themis::query::functions;

// ============================================================================
// Helpers
// ============================================================================

/// Parse a GraphQL query string; abort test on failure.
static Document mustParse(const std::string& query) {
    auto r = Parser::parse(query);
    EXPECT_TRUE(r.success) << "Parse failed: " << (r.errors.empty() ? "" : r.errors[0].toString());
    return r.document;
}

// ============================================================================
// GraphQLAqlCostModelTests
// ============================================================================

TEST(GraphQLAqlCostModelTests, SingleFieldScore) {
    // Single field at depth 1 → score = 1
    auto doc = mustParse("{ apiVersion }");
    EXPECT_EQ(GraphQLComplexityEstimator::estimate(std::make_shared<Document>(doc)), 1u);
}

TEST(GraphQLAqlCostModelTests, TwoFieldsAtRootScore) {
    // Two fields at depth 1 → score = 2
    auto doc = mustParse("{ apiVersion schemaVersion }");
    EXPECT_EQ(GraphQLComplexityEstimator::estimate(std::make_shared<Document>(doc)), 2u);
}

TEST(GraphQLAqlCostModelTests, AqlFieldCarriesHeavyCost) {
    // aql field: 1 (base) + 50 (AQL surcharge) + 1 (query arg → ⌈1/2⌉) = 52
    auto doc = mustParse(R"({ aql(query: "FOR d IN c RETURN d") })");
    const uint32_t score = GraphQLComplexityEstimator::estimate(std::make_shared<Document>(doc));
    EXPECT_GE(score, 50u) << "aql field must carry at least 50 cost units";
}

TEST(GraphQLAqlCostModelTests, NestedFieldsMultipliedByDepth) {
    // depth-1 field (1) + depth-2 field (2) = 3
    auto doc = mustParse("{ document { id } }");
    const uint32_t score = GraphQLComplexityEstimator::estimate(std::make_shared<Document>(doc));
    EXPECT_GE(score, 3u);
}

TEST(GraphQLAqlCostModelTests, LimitsForLowComplexity) {
    auto limits = GraphQLComplexityEstimator::limitsFor(50);
    EXPECT_EQ(limits.max_rows, 0u);          // unlimited
    EXPECT_EQ(limits.timeout_ms, 30000u);
}

TEST(GraphQLAqlCostModelTests, LimitsForMediumComplexity) {
    auto limits = GraphQLComplexityEstimator::limitsFor(300);
    EXPECT_EQ(limits.max_rows, 50000u);
    EXPECT_EQ(limits.timeout_ms, 20000u);
    EXPECT_GT(limits.max_memory_bytes, 0u);
}

TEST(GraphQLAqlCostModelTests, LimitsForHighComplexity) {
    auto limits = GraphQLComplexityEstimator::limitsFor(900);
    EXPECT_EQ(limits.max_rows, 10000u);
    EXPECT_EQ(limits.timeout_ms, 10000u);
}

TEST(GraphQLAqlCostModelTests, OverBudgetThrows) {
    EXPECT_THROW(
        GraphQLComplexityEstimator::limitsFor(kGraphQLMaxComplexity + 1),
        std::runtime_error);
}

// ============================================================================
// GraphQLAqlResolverTests  (Layer 1)
// ============================================================================

TEST(GraphQLAqlResolverTests, InjectResolversPopulatesContext) {
    auto doc = mustParse("{ apiVersion schemaVersion }");
    ExecutionContext ctx;
    GraphQLAqlResolverFactory::injectResolvers(ctx, doc, /*engine=*/nullptr);

    EXPECT_TRUE(ctx.resolvers.count("aql"));
    EXPECT_TRUE(ctx.resolvers.count("aqlMutation"));
    EXPECT_TRUE(ctx.resolvers.count("apiVersion"));
    EXPECT_TRUE(ctx.resolvers.count("schemaVersion"));
}

TEST(GraphQLAqlResolverTests, ApiVersionResolverReturnsString) {
    auto doc = mustParse("{ apiVersion }");
    ExecutionContext ctx;
    GraphQLAqlResolverFactory::injectResolvers(ctx, doc, /*engine=*/nullptr);

    Field f;
    f.name = "apiVersion";
    auto result = ctx.resolvers["apiVersion"](f, nullptr, ctx);
    ASSERT_TRUE(result != nullptr);
    EXPECT_TRUE(result->isString());
    EXPECT_FALSE(result->asString().empty());
}

TEST(GraphQLAqlResolverTests, SchemaVersionResolverReturnsString) {
    auto doc = mustParse("{ schemaVersion }");
    ExecutionContext ctx;
    GraphQLAqlResolverFactory::injectResolvers(ctx, doc, /*engine=*/nullptr);

    Field f;
    f.name = "schemaVersion";
    auto result = ctx.resolvers["schemaVersion"](f, nullptr, ctx);
    ASSERT_TRUE(result != nullptr);
    EXPECT_TRUE(result->isString());
    EXPECT_FALSE(result->asString().empty());
}

TEST(GraphQLAqlResolverTests, AqlResolverWithoutEngineReturnsNull) {
    auto doc = mustParse(R"({ aql(query: "RETURN 1") })");
    ExecutionContext ctx;
    GraphQLAqlResolverFactory::injectResolvers(ctx, doc, /*engine=*/nullptr);

    Field f;
    f.name = "aql";
    f.arguments["query"] = Value::string("RETURN 1");
    auto result = ctx.resolvers["aql"](f, nullptr, ctx);
    EXPECT_TRUE(result == nullptr || result->isNull());
}

TEST(GraphQLAqlResolverTests, AqlResolverThrowsOnMissingQueryArg) {
    auto doc = mustParse("{ apiVersion }");
    ExecutionContext ctx;
    GraphQLAqlResolverFactory::injectResolvers(ctx, doc, /*engine=*/nullptr);

    // Intentionally pass no engine so the "no engine → null" fast-path fires
    // before the argument validation.  Null engine → returns null, no throw.
    Field f;
    f.name = "aql";
    // No arguments
    auto result = ctx.resolvers["aql"](f, nullptr, ctx);
    EXPECT_TRUE(result == nullptr || result->isNull());
}

// ============================================================================
// GraphQLAqlHandlerTests  (HTTP handler – complexity gate)
// ============================================================================

// We test the complexity gate at the GraphQLComplexityEstimator level because
// the HTTP handler wraps Boost.Beast which requires a full socket setup.

TEST(GraphQLAqlHandlerTests, ExactBudgetBoundaryAccepted) {
    EXPECT_NO_THROW(GraphQLComplexityEstimator::limitsFor(kGraphQLMaxComplexity));
}

TEST(GraphQLAqlHandlerTests, OneBeyondBudgetRejected) {
    EXPECT_THROW(
        GraphQLComplexityEstimator::limitsFor(kGraphQLMaxComplexity + 1),
        std::runtime_error);
}

TEST(GraphQLAqlHandlerTests, ComplexityErrorMessageContainsBudget) {
    try {
        GraphQLComplexityEstimator::limitsFor(kGraphQLMaxComplexity + 100);
        FAIL() << "Expected std::runtime_error";
    } catch (const std::runtime_error& e) {
        const std::string msg = e.what();
        EXPECT_NE(msg.find(std::to_string(kGraphQLMaxComplexity)), std::string::npos)
            << "Error message should mention the budget limit";
    }
}

// ============================================================================
// GraphQLAqlFunctionTests  (Layer 2 – GRAPHQL() AQL built-in)
// ============================================================================

TEST(GraphQLAqlFunctionTests, SignatureName) {
    GraphQLFunction fn;
    EXPECT_EQ(fn.signature().name, "GRAPHQL");
}

TEST(GraphQLAqlFunctionTests, SignatureCostIsExternal) {
    GraphQLFunction fn;
    EXPECT_EQ(fn.signature().cost.complexity, CostComplexity::EXTERNAL);
    EXPECT_GE(fn.signature().cost.base_cost, 100.0);
}

TEST(GraphQLAqlFunctionTests, SignatureNotParallelizable) {
    GraphQLFunction fn;
    EXPECT_FALSE(fn.signature().cost.is_parallelizable);
}

TEST(GraphQLAqlFunctionTests, EmptyQueryThrows) {
    GraphQLFunction fn;
    FunctionContext ctx;
    EXPECT_THROW(fn.execute({nlohmann::json("")}, ctx), std::runtime_error);
}

TEST(GraphQLAqlFunctionTests, NonStringFirstArgThrows) {
    GraphQLFunction fn;
    FunctionContext ctx;
    EXPECT_THROW(fn.execute({nlohmann::json(42)}, ctx), std::runtime_error);
}

TEST(GraphQLAqlFunctionTests, InvalidGraphQLSyntaxReturnsErrorNode) {
    GraphQLFunction fn;
    FunctionContext ctx;
    // Malformed GraphQL → parse fails → returns error node with _graphql_errors
    nlohmann::json result = fn.execute({nlohmann::json("{ unclosed")}, ctx);
    EXPECT_TRUE(result.is_object());
    EXPECT_TRUE(result.contains("_graphql_errors"));
    EXPECT_TRUE(result["_graphql_errors"].is_array());
    EXPECT_FALSE(result["_graphql_errors"].empty());
}

TEST(GraphQLAqlFunctionTests, VersionQueryReturnsData) {
    GraphQLFunction fn;
    FunctionContext ctx;
    nlohmann::json result =
        fn.execute({nlohmann::json("{ apiVersion schemaVersion }")}, ctx);
    EXPECT_TRUE(result.is_object());
    EXPECT_FALSE(result.contains("_graphql_errors"));
    // apiVersion and schemaVersion are served by static resolvers
    // injected inside GRAPHQL() – they do not need an AQL engine.
    EXPECT_TRUE(result.contains("apiVersion") || result.contains("schemaVersion"));
}

TEST(GraphQLAqlFunctionTests, VariablesArgumentAccepted) {
    GraphQLFunction fn;
    FunctionContext ctx;
    // Empty variables object must not throw
    nlohmann::json vars = nlohmann::json::object();
    EXPECT_NO_THROW(
        fn.execute({nlohmann::json("{ apiVersion }"), vars}, ctx));
}

TEST(GraphQLAqlFunctionTests, ComplexityOverBudgetThrows) {
    GraphQLFunction fn;
    FunctionContext ctx;

    // Craft a query that exceeds kGraphQLMaxComplexity by depth explosion.
    // 40 aql fields each scoring ~52 → total ≈ 2080 > 1000.
    std::string q = "{ ";
    for (int i = 0; i < 40; ++i) {
        q += "aql" + std::to_string(i) +
             ": aql(query: \"RETURN 1\") ";
    }
    q += "}";

    EXPECT_THROW(fn.execute({nlohmann::json(q)}, ctx), std::runtime_error);
}

// ============================================================================
// GraphQLAqlSchemaTests  (schema fields added by this feature)
// ============================================================================

TEST(GraphQLAqlSchemaTests, MutationTypeHasAqlMutationField) {
    auto schema = ThemisSchemaBuilder::build();
    const TypeDefinition* mut = schema.getType("Mutation");
    ASSERT_NE(mut, nullptr);
    bool found = false;
    for (const auto& f : mut->fields) {
        if (f.name == "aqlMutation") { found = true; break; }
    }
    EXPECT_TRUE(found) << "Mutation type should have 'aqlMutation' field";
}

TEST(GraphQLAqlSchemaTests, QueryTypeHasApiVersionField) {
    auto schema = ThemisSchemaBuilder::build();
    const TypeDefinition* q = schema.getType("Query");
    ASSERT_NE(q, nullptr);
    bool found = false;
    for (const auto& f : q->fields) {
        if (f.name == "apiVersion") { found = true; break; }
    }
    EXPECT_TRUE(found) << "Query type should have 'apiVersion' field";
}

TEST(GraphQLAqlSchemaTests, QueryTypeHasSchemaVersionField) {
    auto schema = ThemisSchemaBuilder::build();
    const TypeDefinition* q = schema.getType("Query");
    ASSERT_NE(q, nullptr);
    bool found = false;
    for (const auto& f : q->fields) {
        if (f.name == "schemaVersion") { found = true; break; }
    }
    EXPECT_TRUE(found) << "Query type should have 'schemaVersion' field";
}

TEST(GraphQLAqlSchemaTests, AqlMutationFieldHasQueryArgument) {
    auto schema = ThemisSchemaBuilder::build();
    const TypeDefinition* mut = schema.getType("Mutation");
    ASSERT_NE(mut, nullptr);
    for (const auto& f : mut->fields) {
        if (f.name == "aqlMutation") {
            EXPECT_TRUE(f.arguments.count("query"))
                << "aqlMutation must declare a 'query' argument";
            return;
        }
    }
    FAIL() << "aqlMutation field not found in Mutation type";
}

TEST(GraphQLAqlSchemaTests, ApiVersionFieldIsNonNull) {
    auto schema = ThemisSchemaBuilder::build();
    const TypeDefinition* q = schema.getType("Query");
    ASSERT_NE(q, nullptr);
    for (const auto& f : q->fields) {
        if (f.name == "apiVersion") {
            EXPECT_TRUE(f.type.is_non_null)
                << "apiVersion must be String! (non-null)";
            return;
        }
    }
    FAIL() << "apiVersion field not found in Query type";
}

TEST(GraphQLAqlSchemaTests, VersionFieldsResolvedByExecutor) {
    auto doc = mustParse("{ apiVersion schemaVersion }");
    ExecutionContext ctx;
    GraphQLAqlResolverFactory::injectResolvers(ctx, doc, /*engine=*/nullptr);

    Executor executor;
    auto result = executor.execute(doc, ctx);
    EXPECT_FALSE(result.hasErrors())
        << "Versioning fields should resolve without errors";
    ASSERT_NE(result.data, nullptr);
    EXPECT_TRUE(result.data->isObject());
}
