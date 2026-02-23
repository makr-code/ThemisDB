/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            aql_query_builder.h                                ║
  Version:         0.0.24                                             ║
  Last Modified:   2026-02-23 03:57:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     220                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a629043ab  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "aql/aql_query_validator.h"
#include "aql/aql_schema_provider.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace themis {

// Forward declarations
namespace aql {
class LLMAQLHandler;
} // namespace aql

namespace aql {

/**
 * @brief Interactive AQL query builder with LLM suggestions
 *
 * Provides a fluent builder API for constructing AQL queries step-by-step,
 * with optional LLM-powered suggestions at each stage.
 *
 * Usage example:
 * @code
 * AQLQueryBuilder builder;
 * std::string query = builder
 *     .forIn("user", "users")
 *     .filter("user.age > 18")
 *     .sort("user.name")
 *     .limit(10)
 *     .ret("user")
 *     .build();
 * // query == "FOR user IN users\n  FILTER user.age > 18\n  SORT user.name ASC\n  LIMIT 10\n  RETURN user"
 * @endcode
 */
class AQLQueryBuilder {
public:
    AQLQueryBuilder();
    ~AQLQueryBuilder();

    // Non-copyable, movable
    AQLQueryBuilder(const AQLQueryBuilder&) = delete;
    AQLQueryBuilder& operator=(const AQLQueryBuilder&) = delete;
    AQLQueryBuilder(AQLQueryBuilder&&) noexcept;
    AQLQueryBuilder& operator=(AQLQueryBuilder&&) noexcept;

    // =========================================================================
    // Fluent builder API
    // =========================================================================

    /**
     * @brief Add a FOR ... IN ... clause
     * @param variable Loop variable name (e.g., "user")
     * @param collection Collection name or expression (e.g., "users")
     */
    AQLQueryBuilder& forIn(const std::string& variable, const std::string& collection);

    /**
     * @brief Add a FILTER clause
     * @param condition Filter expression (e.g., "user.age > 18")
     */
    AQLQueryBuilder& filter(const std::string& condition);

    /**
     * @brief Add a SORT clause
     * @param field Field or expression to sort by (e.g., "user.name")
     * @param ascending Sort direction (true = ASC, false = DESC)
     */
    AQLQueryBuilder& sort(const std::string& field, bool ascending = true);

    /**
     * @brief Add a LIMIT clause
     * @param count Maximum number of results
     * @param offset Number of results to skip (default 0)
     */
    AQLQueryBuilder& limit(int count, int offset = 0);

    /**
     * @brief Set the RETURN expression
     * @param expression Return expression (e.g., "user", "{name: user.name}")
     */
    AQLQueryBuilder& ret(const std::string& expression);

    /**
     * @brief Add a LET variable binding
     * @param variable Variable name
     * @param expression Value expression
     */
    AQLQueryBuilder& let(const std::string& variable, const std::string& expression);

    /**
     * @brief Add a COLLECT clause for grouping/aggregation
     * @param variable Grouping variable
     * @param expression Group expression
     */
    AQLQueryBuilder& collect(const std::string& variable, const std::string& expression);

    /**
     * @brief Reset the builder to initial empty state
     */
    AQLQueryBuilder& reset();

    // =========================================================================
    // Query output
    // =========================================================================

    /**
     * @brief Build and return the complete AQL query string
     * @throws std::logic_error if the query is missing required clauses (FOR or RETURN)
     */
    std::string build() const;

    /**
     * @brief Return the partial (possibly incomplete) query string built so far
     */
    std::string getPartialQuery() const;

    // =========================================================================
    // Validation and state
    // =========================================================================

    /**
     * @brief Check whether the query has at least a FOR and a RETURN clause
     */
    bool isComplete() const;

    /**
     * @brief Check whether the query structure is valid (clauses in correct order)
     */
    bool isValid() const;

    /**
     * @brief Validate the current builder state using AQLQueryValidator.
     *
     * Returns a ValidationResult containing all structural and lint issues found.
     * Does not require an LLM connection.
     *
     * @return ValidationResult with errors, warnings, and hints
     */
    ValidationResult validate() const;

    // =========================================================================
    // Schema-aware query generation (live collection metadata)
    // =========================================================================

    /**
     * @brief Attach a collection metadata snapshot to the builder.
     *
     * Once set, the builder uses the metadata to:
     *  - Automatically supply schema context to LLM suggestion methods when no
     *    explicit context string is passed.
     *  - Emit warnings in @c validate() for collections that are not found in the
     *    snapshot.
     *  - Return field name lists via @c getFieldsForCollection().
     *
     * Pass an empty vector to detach any previously attached schema.
     *
     * @param schema  Snapshot of collection metadata (e.g. built with
     *                @c aql::fromTableSchema() from `metadata/aql_schema_bridge.h`)
     */
    AQLQueryBuilder& setSchema(const std::vector<CollectionMetadata>& schema);

    /**
     * @brief Return the formatted schema context string derived from the attached
     *        metadata snapshot.
     *
     * The returned string is formatted by @c formatSchemaContext() and is suitable
     * for direct use as the @p schema_context argument of @c getCompletionSuggestions()
     * or @c getLLMSuggestion().
     *
     * @return Formatted schema string; empty if no schema was attached.
     */
    std::string getSchemaContext() const;

    /**
     * @brief Return the known field names for a given collection.
     *
     * Searches the attached metadata snapshot for the named collection and
     * returns the names of all its fields.  If no schema has been attached or the
     * collection is not found, returns an empty vector.
     *
     * @param collection  Collection name to look up (case-sensitive)
     * @return List of field names; empty if collection is unknown
     */
    std::vector<std::string> getFieldsForCollection(const std::string& collection) const;

    // =========================================================================
    // Suggestions
    // =========================================================================

    /**
     * @brief Return a list of AQL clause names that are valid next steps.
     *
     * Uses rule-based logic derived from the AQL grammar. No LLM required.
     * Returned strings describe which clauses can logically follow the current state.
     *
     * Example return values: {"FILTER", "SORT", "LIMIT", "LET", "COLLECT", "RETURN"}
     */
    std::vector<std::string> getNextSteps() const;

    /**
     * @brief Use an LLM to generate natural-language completion suggestions.
     *
     * Sends the partial query and schema context to the LLM handler and returns a
     * list of AQL snippet suggestions.  When @p schema_context is empty and a
     * metadata snapshot has been attached via @c setSchema(), the builder
     * automatically derives the context from that snapshot.
     *
     * @param handler  LLMAQLHandler instance (must not be null)
     * @param schema_context Optional description of available collections/fields;
     *                       leave empty to use attached metadata automatically
     * @param max_suggestions Maximum number of suggestions to return (default 3)
     * @return Vector of suggested AQL snippets; empty on failure
     */
    std::vector<std::string> getCompletionSuggestions(
        LLMAQLHandler& handler,
        const std::string& schema_context = "",
        int max_suggestions = 3
    ) const;

    /**
     * @brief Use an LLM to suggest a complete query from a natural-language intent.
     *
     * Given a user's intent (e.g., "find active users sorted by age") and optional
     * schema context, asks the LLM to produce a full AQL query.  The current
     * partial query built so far is passed as additional context.  When
     * @p schema_context is empty and a metadata snapshot has been attached via
     * @c setSchema(), the builder automatically derives the context from that snapshot.
     *
     * @param handler  LLMAQLHandler instance (must not be null)
     * @param intent   Natural-language description of the desired query
     * @param schema_context Optional description of available collections/fields;
     *                       leave empty to use attached metadata automatically
     * @return Suggested AQL query string; empty string on failure
     */
    std::string getLLMSuggestion(
        LLMAQLHandler& handler,
        const std::string& intent,
        const std::string& schema_context = ""
    ) const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace aql
} // namespace themis
