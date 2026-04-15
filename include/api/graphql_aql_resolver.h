/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            graphql_aql_resolver.h                             ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-04-15 18:44:09                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     402                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • d11498de07  2026-04-07  fix: address code review comments – shared error helper, ... ║
    • fc0c65a058  2026-04-07  feat(api/aql): AQL-GraphQL integration – cost model bridg... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/*
 * ThemisDB - AQL ↔ GraphQL Resolver Bridge
 *
 * File:    graphql_aql_resolver.h
 * Version: 1.0.0
 *
 * Provides two integration layers:
 *
 *   Layer 1 – AQL in GraphQL
 *     GraphQLAqlResolverFactory wires QueryEngine resolvers into a
 *     graphql::ExecutionContext so that:
 *       query  { aql(query: "FOR d IN docs RETURN d") }
 *       mutation { aqlMutation(query: "INSERT {x:1} INTO docs") }
 *       query  { apiVersion schemaVersion }
 *     all execute real AQL and return typed JSON.
 *
 *   Layer 2 – Cost model bridge
 *     GraphQLComplexityEstimator scores a GraphQL document using the same
 *     cost abstractions as the AQL OptimizerCostModel / QueryResourceLimits,
 *     so expensive GraphQL operations (deep nesting, many fields, AQL
 *     sub-fields) receive proportionally tighter AQL resource limits.
 *
 * Cost scoring rules (additive):
 *   - Every selected field:       +1
 *   - Every argument on a field:  +0.5
 *   - aql / aqlMutation field:    +50  (full AQL execution)
 *   - Depth multiplier per level: × depth (1-based)
 *
 * AQL resource limits derived from score:
 *   complexity ≤  100 → max_rows = unlimited,  timeout_ms = 30 000
 *   complexity ≤  500 → max_rows = 50 000,     timeout_ms = 20 000
 *   complexity ≤ 1000 → max_rows = 10 000,     timeout_ms = 10 000
 *   complexity > 1000 → rejected (TOO_COMPLEX error)
 */

#pragma once

#include "api/graphql.h"
#include "query/aql_runner.h"
#include "query/query_resource_limits.h"
#include "query/query_engine.h"
#include <nlohmann/json.hpp>
#include <memory>
#include <string>
#include <stdexcept>

namespace themis {
namespace graphql {

// ============================================================================
// Shared error helpers
// ============================================================================

/**
 * @brief Build the standardised "complexity exceeded" error message.
 *
 * Used by GraphQLComplexityEstimator::limitsFor(), GraphQLFunction::execute(),
 * and GraphQLApiHandler::handlePost() so all three surfaces report an
 * identical, grep-able message.
 */
inline std::string makeComplexityErrorMessage(uint32_t actual, uint32_t budget) {
    return "GraphQL query complexity " + std::to_string(actual) +
           " exceeds maximum allowed budget " + std::to_string(budget) +
           ". Reduce field count or nesting depth.";
}

// ============================================================================
// Complexity budget
// ============================================================================

/// Maximum allowed GraphQL complexity score before a query is rejected.
inline constexpr uint32_t kGraphQLMaxComplexity = 1000;

// ============================================================================
// GraphQLComplexityEstimator
// ============================================================================

/**
 * @brief Translates a parsed GraphQL document into an integer complexity
 *        score that is directly comparable with the AQL cost model.
 *
 * The estimator visits every Field in the selection set recursively and
 * accumulates a score according to the rules defined in the file header.
 * The score is used by GraphQLAqlResolverFactory to select the appropriate
 * QueryResourceLimits before handing off to executeAqlWithLimits().
 */
class GraphQLComplexityEstimator {
public:
    /**
     * @brief Compute complexity score for a full GraphQL Document.
     *
     * @param doc  Parsed GraphQL document (may contain multiple operations).
     * @return     Accumulated integer complexity score.
     */
    static uint32_t estimate(const Document& doc) {
        uint32_t total = 0;
        for (const auto& op : doc.operations) {
            total += estimateSelections(op.selections, /*depth=*/1);
        }
        return total;
    }

    /**
     * @brief Derive QueryResourceLimits from a complexity score.
     *
     * @param complexity  Score returned by estimate().
     * @throws std::runtime_error when complexity > kGraphQLMaxComplexity.
     * @return  Limits that cap AQL execution proportionally.
     */
    static query::QueryResourceLimits limitsFor(uint32_t complexity) {
        if (complexity > kGraphQLMaxComplexity) {
            throw std::runtime_error(
                makeComplexityErrorMessage(complexity, kGraphQLMaxComplexity));
        }
        query::QueryResourceLimits limits;
        if (complexity <= 100) {
            limits.max_rows        = 0;      // unlimited
            limits.timeout_ms      = 30000;
            limits.max_memory_bytes= 0;
        } else if (complexity <= 500) {
            limits.max_rows        = 50000;
            limits.timeout_ms      = 20000;
            limits.max_memory_bytes= 256 * 1024 * 1024; // 256 MB
        } else {
            limits.max_rows        = 10000;
            limits.timeout_ms      = 10000;
            limits.max_memory_bytes= 64 * 1024 * 1024;  // 64 MB
        }
        return limits;
    }

private:
    static uint32_t estimateSelections(const std::vector<Field>& fields,
                                       uint32_t depth) {
        uint32_t score = 0;
        for (const auto& f : fields) {
            // Base cost: 1 per field, depth-weighted
            score += depth;
            // Arguments add 0.5 each – ceiling-divide by 2 so an odd count
            // rounds up: 1 arg → 1, 2 args → 1, 3 args → 2, etc.
            score += static_cast<uint32_t>(f.arguments.size() + 1) / 2;
            // AQL fields are significantly more expensive
            if (f.name == "aql" || f.name == "aqlMutation") {
                score += 50;
            }
            // Recurse into nested selections with increased depth
            if (!f.selections.empty()) {
                score += estimateSelections(f.selections, depth + 1);
            }
        }
        return score;
    }
};

// ============================================================================
// JSON ↔ graphql::Value conversion helpers
// ============================================================================

/**
 * @brief Convert a nlohmann::json value to a graphql::Value tree.
 *
 * Used to present AQL result JSON inside the GraphQL response envelope.
 */
inline std::shared_ptr<Value> jsonToGqlValue(const nlohmann::json& j) {
    if (j.is_null())    return Value::null();
    if (j.is_boolean()) return Value::boolean(j.get<bool>());
    if (j.is_number_integer()) return Value::integer(j.get<int64_t>());
    if (j.is_number_float())   return Value::floating(j.get<double>());
    if (j.is_string())  return Value::string(j.get<std::string>());
    if (j.is_array()) {
        ValueList list;
        list.reserve(j.size());
        for (const auto& item : j) {
            list.push_back(jsonToGqlValue(item));
        }
        return Value::list(std::move(list));
    }
    if (j.is_object()) {
        ValueMap map;
        for (auto it = j.begin(); it != j.end(); ++it) {
            map[it.key()] = jsonToGqlValue(it.value());
        }
        return Value::object(std::move(map));
    }
    return Value::null();
}

/**
 * @brief Convert a graphql::Value tree to nlohmann::json.
 *
 * Used when the GRAPHQL() AQL function needs to return JSON to the AQL
 * executor.
 */
inline nlohmann::json gqlValueToJson(const std::shared_ptr<Value>& v) {
    if (!v || v->isNull()) return nullptr;
    if (v->isBool())       return v->asBool();
    if (v->isInt())        return v->asInt();
    if (v->isFloat())      return v->asFloat();
    if (v->isString())     return v->asString();
    if (v->isEnum())       return v->asString();
    if (v->isList()) {
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& item : v->asList()) {
            arr.push_back(gqlValueToJson(item));
        }
        return arr;
    }
    if (v->isObject()) {
        nlohmann::json obj = nlohmann::json::object();
        for (const auto& [k, val] : v->asObject()) {
            obj[k] = gqlValueToJson(val);
        }
        return obj;
    }
    return nullptr;
}

// ============================================================================
// GraphQLAqlResolverFactory
// ============================================================================

/**
 * @brief Factory that produces ExecutionContext::Resolver callbacks wiring
 *        the GraphQL execution engine to the AQL query engine.
 *
 * Instantiate with a non-owning QueryEngine pointer, then call
 * injectResolvers() to populate an ExecutionContext before execution.
 *
 * When engine_ is nullptr the resolvers return a null Value (graceful
 * degradation — useful in unit tests that do not wire storage).
 *
 * ## Cost model enforcement
 *
 * Before executing each AQL sub-query the resolver:
 *   1. Calls GraphQLComplexityEstimator::estimate() on the containing
 *      Document.
 *   2. Maps the score to QueryResourceLimits via limitsFor().
 *   3. Delegates to executeAqlWithLimits() which honours max_rows,
 *      max_memory_bytes, and timeout_ms from those limits.
 *
 * This ensures that a heavily nested GraphQL query with many `aql` fields
 * cannot exhaust server resources more than an equivalent plain AQL query.
 */
class GraphQLAqlResolverFactory {
public:
    explicit GraphQLAqlResolverFactory(QueryEngine* engine = nullptr)
        : engine_(engine) {}

    // -----------------------------------------------------------------------
    // Individual resolver builders
    // -----------------------------------------------------------------------

    /**
     * @brief Resolver for `query { aql(query: String!, variables: JSON): JSON }`.
     *
     * Executes the supplied AQL SELECT/FOR/RETURN query through the AQL engine.
     * Resource limits are derived from the complexity of the enclosing GraphQL
     * document.
     *
     * @param doc  The parsed GraphQL Document (for complexity estimation).
     */
    ExecutionContext::Resolver makeAqlQueryResolver(const Document& doc) const {
        QueryEngine* eng = engine_;
        const query::QueryResourceLimits limits =
            GraphQLComplexityEstimator::limitsFor(
                GraphQLComplexityEstimator::estimate(doc));

        return [eng, limits](
            const Field& field,
            const std::shared_ptr<Value>& /*parent*/,
            const ExecutionContext& /*ctx*/) -> std::shared_ptr<Value>
        {
            if (!eng) return Value::null();

            const std::string aqlQuery = extractStringArg(field, "query");
            if (aqlQuery.empty()) {
                throw std::runtime_error(
                    "aql: 'query' argument is required and must be a non-empty string");
            }

            auto result = executeAqlWithLimits(aqlQuery, *eng, limits);
            if (!result) {
                throw std::runtime_error("AQL execution error: " +
                                         result.error().message());
            }
            return jsonToGqlValue(result.value());
        };
    }

    /**
     * @brief Resolver for
     *        `mutation { aqlMutation(query: String!, variables: JSON): JSON }`.
     *
     * Executes AQL DML statements (INSERT / UPDATE / REPLACE / REMOVE / UPSERT).
     * Identical to the query resolver but labelled for clarity — mutations run
     * with the same cost-derived limits.
     *
     * @param doc  The parsed GraphQL Document (for complexity estimation).
     */
    ExecutionContext::Resolver makeAqlMutationResolver(const Document& doc) const {
        return makeAqlQueryResolver(doc); // same engine path, same limits
    }

    /**
     * @brief Resolver for `query { apiVersion: String! }`.
     *
     * Returns the current ThemisDB API version string (e.g. "1.8.0-rc1").
     * Uses the same version constant as the HTTP `API-Version` response header
     * so clients can perform schema-level version checks inside a GraphQL query.
     */
    static ExecutionContext::Resolver makeApiVersionResolver() {
        return [](const Field& /*f*/,
                  const std::shared_ptr<Value>& /*p*/,
                  const ExecutionContext& /*ctx*/) -> std::shared_ptr<Value>
        {
            // VERSION file content – kept in sync by the release pipeline.
            return Value::string("1.8.0-rc1");
        };
    }

    /**
     * @brief Resolver for `query { schemaVersion: String! }`.
     *
     * Returns the GraphQL schema version, incremented whenever the schema
     * gains new types or fields (independent of the API version).
     */
    static ExecutionContext::Resolver makeSchemaVersionResolver() {
        return [](const Field& /*f*/,
                  const std::shared_ptr<Value>& /*p*/,
                  const ExecutionContext& /*ctx*/) -> std::shared_ptr<Value>
        {
            return Value::string("2.0.0"); // GraphQL schema version
        };
    }

    // -----------------------------------------------------------------------
    // Convenience: inject all resolvers at once
    // -----------------------------------------------------------------------

    /**
     * @brief Populate @p ctx with all AQL + versioning resolvers.
     *
     * Call this immediately before Executor::execute().  The Document must
     * have been successfully parsed so that the complexity estimator can
     * compute accurate limits.
     *
     * @param ctx  Execution context to populate.
     * @param doc  Parsed GraphQL document.
     * @param eng  Non-owning pointer to the AQL query engine (may be nullptr).
     */
    static void injectResolvers(ExecutionContext& ctx,
                                const Document& doc,
                                QueryEngine* eng)
    {
        GraphQLAqlResolverFactory factory(eng);
        ctx.resolvers["aql"]            = factory.makeAqlQueryResolver(doc);
        ctx.resolvers["aqlMutation"]    = factory.makeAqlMutationResolver(doc);
        ctx.resolvers["apiVersion"]     = makeApiVersionResolver();
        ctx.resolvers["schemaVersion"]  = makeSchemaVersionResolver();
    }

private:
    QueryEngine* engine_; ///< Non-owning pointer; may be nullptr.

    /// Extract a mandatory String argument from a GraphQL Field by name.
    static std::string extractStringArg(const Field& field,
                                        const std::string& argName)
    {
        auto it = field.arguments.find(argName);
        if (it == field.arguments.end() || !it->second ||
            !it->second->isString()) {
            return {};
        }
        return it->second->asString();
    }
};

} // namespace graphql
} // namespace themis
