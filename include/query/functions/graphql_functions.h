/**
 * @file graphql_functions.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * ThemisDB – GRAPHQL() AQL Built-in Function
 *
 * File:    graphql_functions.h
 * Version: 1.0.0
 *
 * Registers the GRAPHQL() scalar function in the AQL function registry.
 * This is Layer 2 of the AQL ↔ GraphQL integration: calling GraphQL
 * from within an AQL query.
 *
 * Syntax:
 *   GRAPHQL(query [, variables])
 *
 *   query      – String – a valid GraphQL query / mutation document
 *   variables  – Object – optional JSON object of variable bindings
 *
 * Returns the "data" portion of the GraphQL execution result as JSON,
 * or null on error (errors are surfaced as a {"_graphql_errors":[...]}
 * key in the returned object so AQL callers can inspect them via
 * FILTER result._graphql_errors == null).
 *
 * ## Cost model
 *
 * GRAPHQL() is classified as CostComplexity::EXTERNAL with a high
 * base_cost (100) because it:
 *   1. Parses the embedded GraphQL document.
 *   2. Scores it via GraphQLComplexityEstimator (same budget rules as
 *      the HTTP GraphQL endpoint – max complexity 1000).
 *   3. Executes the document through the internal GraphQL executor.
 *
 * The query optimizer sees the high cost and will not push GRAPHQL()
 * inside tight inner loops unless no cheaper alternative exists.
 * Queries that exceed kGraphQLMaxComplexity are rejected at parse time
 * so they never reach the optimizer.
 *
 * ## Security
 *
 * The internal GraphQL executor runs without an injected QueryEngine
 * resolver, so the embedded query cannot trigger a recursive AQL
 * execution loop.  It can only resolve fields served by the static
 * schema (introspection, versioning) – not the `aql` or `aqlMutation`
 * resolver fields.
 */

#pragma once

#include "query/functions/function_registry.h"
#include "api/graphql.h"
#include "api/graphql_aql_resolver.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <stdexcept>

namespace themis {
namespace query {
namespace functions {

// ============================================================================
// GRAPHQL() function
// ============================================================================

/**
 * @brief GRAPHQL(query, variables?) — execute an embedded GraphQL document.
 *
 * Cost: CostComplexity::EXTERNAL, base_cost=100.0
 *   The optimizer treats every call as expensive external I/O.  Field
 *   pushdown and predicate pushdown across GRAPHQL() boundaries are
 *   therefore disabled — the optimizer will not place a GRAPHQL() call
 *   inside an inner FOR loop unless the calling query has been explicitly
 *   structured that way.
 *
 * Complexity guard:
 *   The embedded GraphQL document is scored by GraphQLComplexityEstimator
 *   before execution.  If the score exceeds kGraphQLMaxComplexity
 *   (1 000) the function throws, aborting the enclosing AQL query.
 *
 * Example AQL:
 * @code
 *   LET result = GRAPHQL("query { apiVersion schemaVersion }")
 *   RETURN result
 *   // → { "apiVersion": "1.8.0-rc1", "schemaVersion": "2.0.0" }
 *
 *   LET res = GRAPHQL(
 *     "query GetUser($id: ID!) { user(id: $id) { name } }",
 *     { "id": "42" }
 *   )
 *   FILTER res._graphql_errors == null
 *   RETURN res.user.name
 * @endcode
 */
class GraphQLFunction : public IFunction {
public:
    ~GraphQLFunction() override = default;
    FunctionSignature signature() const override {
        return FunctionSignature{
            .name        = "GRAPHQL",
            .category    = "Integration",
            .description =
                "Execute an embedded GraphQL query against the ThemisDB "
                "internal schema. Returns the 'data' portion of the "
                "GraphQL response as JSON. On error the result contains "
                "a '_graphql_errors' array.",
            .arguments = {
                { "query",     ArgType::STRING, /*required=*/true,  nullptr,
                  "GraphQL query / mutation document string" },
                { "variables", ArgType::OBJECT, /*required=*/false, nullptr,
                  "Optional variable bindings object" },
            },
            .return_type      = ArgType::ANY,
            .is_deterministic = false, // schema may evolve
            .is_aggregate     = false,
            .examples = {
                R"(GRAPHQL("query { apiVersion schemaVersion }"))",
                R"(GRAPHQL("query { apiVersion }", {}))",
            },
            .cost = FunctionCost{
                .complexity         = CostComplexity::EXTERNAL,
                .base_cost          = 100.0,   // heavyweight: full parse + execute
                .per_element_cost   = 0.0,
                .can_use_index      = false,
                .is_parallelizable  = false,
                .index_type         = ""
            }
        };
    }

    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& /*ctx*/) const override
    {
        if (args.empty() || !args[0].is_string()) {
            throw std::runtime_error(
                "GRAPHQL: first argument must be a non-empty string "
                "(GraphQL query document)");
        }

        const std::string query_str = args[0].get<std::string>();
        if (query_str.empty()) {
            throw std::runtime_error(
                "GRAPHQL: query document string must not be empty");
        }

        // ── 1. Parse with structural limits ──────────────────────────────
        const graphql::QueryLimits limits = graphql::QueryLimits::defaults();
        auto parse_result = graphql::Parser::parse(query_str, limits);
        if (!parse_result.success) {
            nlohmann::json err = nlohmann::json::object();
            nlohmann::json errors = nlohmann::json::array();
            for (const auto& pe : parse_result.errors) {
                errors.push_back({ {"message", pe.toString()} });
            }
            err["_graphql_errors"] = std::move(errors);
            return err;
        }

        // ── 2. Complexity guard (cost model enforcement) ──────────────────
        const uint32_t complexity =
            graphql::GraphQLComplexityEstimator::estimate(
                std::make_shared<graphql::Document>(parse_result.document));
        if (complexity > graphql::kGraphQLMaxComplexity) {
            throw std::runtime_error(
                graphql::makeComplexityErrorMessage(
                    complexity, graphql::kGraphQLMaxComplexity));
        }

        // ── 3. Build execution context ────────────────────────────────────
        //
        // NOTE: No AQL resolver is injected here intentionally.
        // Allowing GRAPHQL() to call back into the AQL engine would create
        // unbounded recursion.  The internal executor can resolve versioning
        // and static schema fields only.
        graphql::ExecutionContext exec_ctx;

        // Populate GraphQL variables from the optional second argument
        if (args.size() >= 2 && args[1].is_object()) {
            for (auto it = args[1].begin(); it != args[1].end(); ++it) {
                exec_ctx.variables[it.key()] =
                    graphql::jsonToGqlValue(it.value());
            }
        }

        // Static versioning resolvers (no engine required)
        exec_ctx.resolvers["apiVersion"]    =
            graphql::GraphQLAqlResolverFactory::makeApiVersionResolver();
        exec_ctx.resolvers["schemaVersion"] =
            graphql::GraphQLAqlResolverFactory::makeSchemaVersionResolver();

        // ── 4. Execute ────────────────────────────────────────────────────
        graphql::Executor executor;
        auto exec_result = executor.execute(parse_result.document, exec_ctx);

        // ── 5. Build return value ─────────────────────────────────────────
        if (exec_result.hasErrors()) {
            nlohmann::json result = graphql::gqlValueToJson(exec_result.data);
            if (result.is_null()) result = nlohmann::json::object();
            nlohmann::json errors = nlohmann::json::array();
            for (const auto& me : exec_result.errors) {
                errors.push_back({ {"message", me.message},
                                   {"code",    me.code} });
            }
            result["_graphql_errors"] = std::move(errors);
            return result;
        }

        return graphql::gqlValueToJson(exec_result.data);
    }
};

// ============================================================================
// Registration helper
// ============================================================================

/**
 * @brief Register all GraphQL integration AQL functions.
 *
 * Call once from registerBuiltinFunctions() in function_registry.cpp.
 *
 * Currently registers:
 *   - GRAPHQL(query [, variables]) → JSON
 */
inline void registerGraphQLFunctions(FunctionRegistry& registry) {
    registry.registerFunction(std::make_unique<GraphQLFunction>());
}

} // namespace functions
} // namespace query
} // namespace themis
