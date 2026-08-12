/**
 * @file graphql_aql_resolver.h
 * @brief Bridge between GraphQL queries and ThemisDB's internal AQL engine.
 *
 * @details Translates GraphQL operations (queries, mutations, subscriptions)
 * into AQL (Attribute Query Language) commands executed against the database.
 *
 * Core components:
 *  - `QueryEngine`: Translates GraphQL queries to AQL, enforces limits
 *  - `QueryResourceLimits`: Configuration for max depth, fields, complexity
 *  - Field resolvers for scalar types, objects, and collections
 *
 * Resolution process:
 *  1. Parse GraphQL query into Operation AST
 *  2. Validate against resource limits (depth, field count, complexity)
 *  3. Translate field selections to AQL projections
 *  4. Translate filters/arguments to AQL predicates
 *  5. Execute AQL command against database
 *  6. Map results back to GraphQL response shape
 *
 * Resource limits prevent DoS attacks:
 *  - max_query_depth: prevents deeply nested selections (e.g., user → friend → friend → ...)
 *  - max_field_count: limits total fields across all selections
 *  - max_complexity_score: bounds expensive operations (joins, aggregations)
 *
 * ### Thread safety
 * `QueryEngine` instances are typically per-request (not shared).
 * Stateless field resolvers are safe for concurrent calls.
 *
 * ### Error handling
 * - Invalid field references return ERR_GRAPHQL_INVALID_SELECTION
 * - Complexity violations return ERR_GRAPHQL_QUERY_TOO_COMPLEX
 * - AQL execution errors propagate with context
 *
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
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
#include <memory>
#include <string>
#include <cstdint>
#include <functional>

// Forward declarations to prevent namespace pollution
namespace themis {
namespace query {
class QueryEngine;
struct QueryResourceLimits;
/** @brief Expected. */
template <typename T, typename E> class expected;
struct QueryError;
}
using QueryEngine = query::QueryEngine;
}

namespace themis {
namespace graphql {

// Forward declarations to avoid header bloat
struct Document;
struct Field;
struct Value;
struct ExecutionContext;
struct SelectionSet;

// ============================================================================
// Complexity Estimation
// ============================================================================

/// Maximum allowed GraphQL complexity score before a query is rejected.
constexpr uint32_t kGraphQLMaxComplexity = 1000;

/// Build error message for complexity violations.
std::string makeComplexityErrorMessage(uint32_t actual, uint32_t budget);

/**
 * @brief Translates a parsed GraphQL document into a complexity score.
 * 
 * The score determines AQL resource limits (max_rows, timeout_ms).
 */
class GraphQLComplexityEstimator {
public:
    /// Compute complexity score for a GraphQL Document.
    static uint32_t estimate(const std::shared_ptr<Document>& doc);

    /// Derive QueryResourceLimits from a complexity score.
    /// @throws std::runtime_error if complexity > kGraphQLMaxComplexity
    static ::themis::query::QueryResourceLimits limitsFor(uint32_t complexity);

private:
    static uint32_t scoreSelectionSet(const std::shared_ptr<SelectionSet>& set,
                                     uint32_t depth);
};

// ============================================================================
// Conversion helpers (forward declarations)
// ============================================================================

/// Convert nlohmann::json to graphql::Value tree.
std::shared_ptr<Value> jsonToGqlValue(const nlohmann::json& j);

/// Convert graphql::Value tree to nlohmann::json.
nlohmann::json gqlValueToJson(const std::shared_ptr<Value>& v);

// ============================================================================
// Resolver Factory
// ============================================================================

/**
 * @brief Factory producing ExecutionContext::Resolver callbacks.
 * 
 * Wires AQL query engine into GraphQL execution context.
 * Resource limits are derived from GraphQL document complexity.
 */
class GraphQLAqlResolverFactory {
public:
    explicit GraphQLAqlResolverFactory(::themis::QueryEngine* engine = nullptr)
        : engine_(engine) {}

    /// Resolver for `query { aql(query: String!, variables: JSON): JSON }`.
    ExecutionContext::Resolver makeAqlQueryResolver(const Document& doc) const;

    /// Resolver for `mutation { aqlMutation(query: String!, variables: JSON): JSON }`.
    ExecutionContext::Resolver makeAqlMutationResolver(const Document& doc) const;

    /// Resolver for `query { apiVersion: String! }`.
    static ExecutionContext::Resolver makeApiVersionResolver();

    /// Resolver for `query { schemaVersion: String! }`.
    static ExecutionContext::Resolver makeSchemaVersionResolver();

    /// Convenience: inject all resolvers at once.
    static void injectResolvers(ExecutionContext& ctx,
                                const Document& doc,
                                ::themis::QueryEngine* eng);

private:
    ::themis::QueryEngine* engine_;

    std::string extractStringArg(const Field& field,
                                const std::string& argName) const;

    ::tl::expected<nlohmann::json, ::themis::query::QueryError> 
    executeAqlWithLimits(
        const std::string& aql,
        ::themis::QueryEngine& eng,
        const ::themis::query::QueryResourceLimits& limits) const;
};

} // namespace graphql
} // namespace themis
