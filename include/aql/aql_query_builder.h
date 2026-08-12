/**
 * @file aql_query_builder.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.39
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

    // =========================================================================
    // Graph traversal
    // =========================================================================

    /**
     * @brief Add a graph traversal clause: FOR vertex_var, edge_var, path_var IN min..max direction start GRAPH graph
     * @param vertex_var Vertex loop variable (e.g., "v")
     * @param edge_var   Edge loop variable (e.g., "e")
     * @param path_var   Path loop variable (e.g., "p")
     * @param start      Start vertex id expression (e.g., "\"users/1\"")
     * @param graph      Graph name (e.g., "myGraph")
     * @param direction  Traversal direction: "OUTBOUND", "INBOUND", or "ANY" (default "OUTBOUND")
     * @param min_depth  Minimum traversal depth (default 1)
     * @param max_depth  Maximum traversal depth (default 1)
     * @throws std::invalid_argument if any required string is empty or min_depth > max_depth
     */
    AQLQueryBuilder& forTraverse(
        const std::string& vertex_var,
        const std::string& edge_var,
        const std::string& path_var,
        const std::string& start,
        const std::string& graph,
        const std::string& direction = "OUTBOUND",
        int min_depth = 1,
        int max_depth = 1
    );

    // =========================================================================
    // DML (data manipulation) clauses
    // =========================================================================

    /**
     * @brief Add an INSERT clause: INSERT doc_expr INTO collection
     * @param collection Target collection name
     * @param doc_expr   Document expression to insert (e.g., "{name: \"Alice\"}")
     */
    AQLQueryBuilder& insertInto(const std::string& collection, const std::string& doc_expr);

    /**
     * @brief Add an UPDATE clause: UPDATE doc_expr IN collection
     * @param collection Target collection name
     * @param doc_expr   Document/key expression to update (e.g., "u WITH {active: false}")
     */
    AQLQueryBuilder& updateIn(const std::string& collection, const std::string& doc_expr);

    /**
     * @brief Add a REMOVE clause: REMOVE doc_expr IN collection
     * @param collection Target collection name
     * @param doc_expr   Document/key expression to remove (e.g., "u" or "u._key")
     */
    AQLQueryBuilder& removeIn(const std::string& collection, const std::string& doc_expr);

    /**
     * @brief Add an UPSERT clause: UPSERT filter_expr INSERT insert_expr UPDATE update_expr IN collection
     * @param collection  Target collection name
     * @param filter_expr Search/lookup expression (e.g., "{name: \"Alice\"}")
     * @param insert_expr Document expression for insert branch
     * @param update_expr Document expression for update branch
     */
    AQLQueryBuilder& upsertIn(
        const std::string& collection,
        const std::string& filter_expr,
        const std::string& insert_expr,
        const std::string& update_expr
    );

    /**
     * @brief Add a REPLACE clause: REPLACE doc_expr IN collection
     * @param collection Target collection name
     * @param doc_expr   Document/key expression to replace (e.g., "u WITH {name: \"Bob\"}")
     */
    AQLQueryBuilder& replaceIn(const std::string& collection, const std::string& doc_expr);

    // =========================================================================
    // Ingestion enrichment flag (opt-in)
    // =========================================================================

    /**
     * @brief Enable or disable automatic ingestion enrichment for DML clauses.
     *
     * When enrichment is enabled **and** an `AQLIngestionBridge` has been made
     * available to the query executor, every `INSERT`/`UPSERT`/`REPLACE`
     * document payload is passed through the `WorkflowEngine` before being
     * written to the database.  Extracted entities are appended to the document
     * under the key `"_entities"` and are simultaneously written to the graph
     * store (if a sink was configured on the bridge).
     *
     * This flag is purely advisory — the executor is responsible for honouring
     * it.  It does not affect query generation (i.e. `build()` output is
     * unchanged), and it is off by default to preserve existing behaviour.
     *
     * @param enabled  `true` to activate enrichment (default), `false` to
     *                 deactivate.
     * @return Reference to `*this` for fluent chaining.
     */
    AQLQueryBuilder& withIngestionEnrichment(bool enabled = true);

    /**
     * @brief Return `true` when ingestion enrichment has been requested.
     */
    bool hasIngestionEnrichment() const;

    // =========================================================================
    // WINDOW analytics clause
    // =========================================================================

    /**
     * @brief Add a WINDOW analytics clause for timeseries queries.
     *
     * Renders as: WINDOW partition_expr WITH window_spec
     * If partition_expr is empty, renders as: WINDOW window_spec
     *
     * @param partition_expr Range expression (e.g., "t.time"); pass "" for row-based windows
     * @param window_spec    Window specification object (e.g., "{ preceding: \"PT30M\" }")
     */
    AQLQueryBuilder& window(const std::string& partition_expr, const std::string& window_spec);

    // =========================================================================
    // Subquery support
    // =========================================================================

    /**
     * @brief Add a subquery as a LET binding: LET variable = ( <inner query> )
     * @param variable Variable name for the subquery result
     * @param inner    Builder holding the inner query (rendered via getPartialQuery())
     * @throws std::invalid_argument if variable is empty or inner has no clauses
     */
    AQLQueryBuilder& subquery(const std::string& variable, const AQLQueryBuilder& inner);

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

    /**
     * @brief Validate the current builder state against an explicit schema snapshot.
     *
     * Runs all structural checks (same as @c validate()), then additionally:
     *  - Warns when a collection in a FOR clause is absent from @p schema.
     *  - Warns when a field access (@c variable.field) refers to a field not
     *    present in the schema for that collection.
     *
     * The @p schema passed here takes precedence over any schema previously
     * attached via @c setSchema().  The internally attached schema is not
     * consulted for schema-aware checks when an explicit @p schema is provided.
     *
     * @param schema  Collection metadata snapshot to validate against.
     * @return ValidationResult with all structural and schema issues found.
     */
    ValidationResult validate(const std::vector<CollectionMetadata>& schema) const;

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
