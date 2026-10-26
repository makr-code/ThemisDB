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
#include "server/graphql_api_handler.h"
#include <limits>

using namespace themis;
using namespace themis::graphql;
using namespace themis::query::functions;
using themis::server::GraphQLApiHandler;

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

    Field f;
    f.name = "aql";
    EXPECT_THROW(ctx.resolvers["aql"](f, nullptr, ctx), std::runtime_error);
}

TEST(GraphQLAqlResolverTests, AqlResolverRejectsNonObjectVariablesArgument) {
    auto doc = mustParse("{ apiVersion }");
    ExecutionContext ctx;
    GraphQLAqlResolverFactory::injectResolvers(ctx, doc, /*engine=*/nullptr);

    Field f;
    f.name = "aql";
    f.arguments["query"] = Value::string("RETURN 1");
    f.arguments["variables"] = Value::string("not-an-object");

    EXPECT_THROW(ctx.resolvers["aql"](f, nullptr, ctx), std::runtime_error);
}

TEST(GraphQLAqlResolverTests, AqlResolverAcceptsObjectVariablesWithoutEngine) {
    auto doc = mustParse("{ apiVersion }");
    ExecutionContext ctx;
    GraphQLAqlResolverFactory::injectResolvers(ctx, doc, /*engine=*/nullptr);

    Field f;
    f.name = "aql";
    f.arguments["query"] = Value::string("FOR d IN users FILTER d.id == @id RETURN d");
    f.arguments["variables"] = Value::object({{"id", Value::string("user-1")}});

    EXPECT_NO_THROW({
        auto result = ctx.resolvers["aql"](f, nullptr, ctx);
        EXPECT_TRUE(result == nullptr || result->isNull());
    });
}

TEST(GraphQLAqlResolverTests, AqlResolverRejectsNonStringCollectionBindVariable) {
    auto doc = mustParse("{ apiVersion }");
    ExecutionContext ctx;
    GraphQLAqlResolverFactory::injectResolvers(ctx, doc, /*engine=*/nullptr);

    Field f;
    f.name = "aql";
    f.arguments["query"] =
        Value::string("FOR d IN @@coll FILTER d.id == @id RETURN d");
    f.arguments["variables"] = Value::object({
        {"@coll", Value::integer(42)},
        {"id", Value::string("u1")}
    });

    EXPECT_THROW(ctx.resolvers["aql"](f, nullptr, ctx), std::runtime_error);
}

// ============================================================================
// GraphQLApiHandlerVariableMappingTests
// ============================================================================

TEST(GraphQLApiHandlerVariableMappingTests, ConvertsNestedObjectsAndArrays) {
    const nlohmann::json input = {
        {"tenant", "acme"},
        {"limit", 25},
        {"flags", nlohmann::json::array({"fast", "strict"})},
        {"filter", {{"status", "active"}, {"score", 0.95}}}
    };

    auto value = GraphQLApiHandler::jsonToGraphQLValue(input);
    ASSERT_NE(value, nullptr);
    ASSERT_TRUE(value->isObject());

    const auto& root = value->asObject();
    auto tenant_it = root.find("tenant");
    ASSERT_NE(tenant_it, root.end());
    ASSERT_TRUE(tenant_it->second->isString());
    EXPECT_EQ(tenant_it->second->asString(), "acme");

    auto limit_it = root.find("limit");
    ASSERT_NE(limit_it, root.end());
    ASSERT_TRUE(limit_it->second->isInt());
    EXPECT_EQ(limit_it->second->asInt(), 25);

    auto flags_it = root.find("flags");
    ASSERT_NE(flags_it, root.end());
    ASSERT_TRUE(flags_it->second->isList());
    ASSERT_EQ(flags_it->second->asList().size(), 2u);
    EXPECT_EQ(flags_it->second->asList()[0]->asString(), "fast");
    EXPECT_EQ(flags_it->second->asList()[1]->asString(), "strict");

    auto filter_it = root.find("filter");
    ASSERT_NE(filter_it, root.end());
    ASSERT_TRUE(filter_it->second->isObject());
    const auto& filter_obj = filter_it->second->asObject();
    auto status_it = filter_obj.find("status");
    auto score_it = filter_obj.find("score");
    ASSERT_NE(status_it, filter_obj.end());
    ASSERT_NE(score_it, filter_obj.end());
    EXPECT_EQ(status_it->second->asString(), "active");
    EXPECT_DOUBLE_EQ(score_it->second->asFloat(), 0.95);
}

TEST(GraphQLApiHandlerVariableMappingTests, ConvertsLargeUnsignedToFloat) {
    const nlohmann::json input = {
        {"u64", std::numeric_limits<uint64_t>::max()}
    };

    auto value = GraphQLApiHandler::jsonToGraphQLValue(input);
    ASSERT_NE(value, nullptr);
    ASSERT_TRUE(value->isObject());

    const auto& obj = value->asObject();
    auto it = obj.find("u64");
    ASSERT_NE(it, obj.end());
    EXPECT_TRUE(it->second->isFloat());
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
