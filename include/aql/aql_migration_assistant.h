/*
 * ThemisDB | File: aql_migration_assistant.h | Version: 0.0.15 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 177
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=n/a | delta=n/a | status=no_external_data
 * External Severity (v3): C=n/a, H=n/a, M=n/a
 * PR: #2694 feat(aql): AQL query migration assistant (ArangoDB AQL â†’ ThemisDB... (2026-03-12T05:55:29Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include <string>
#include <vector>

namespace themis {
namespace aql {

/**
 * @brief A single migration issue found while converting ArangoDB AQL to ThemisDB AQL.
 */
struct MigrationIssue {
    enum class Severity {
        ERROR,   ///< Construct is unsupported and cannot be automatically migrated
        WARNING, ///< Construct was rewritten but manual review is recommended
        INFO     ///< Informational note about compatibility or style differences
    };

    Severity    severity;
    std::string message;    ///< Human-readable description of the issue
    std::string suggestion; ///< Suggested alternative or manual action
};

/**
 * @brief Result of an ArangoDB AQL → ThemisDB AQL migration.
 */
struct MigrationResult {
    /// The migrated ThemisDB AQL query string.
    /// May still contain ArangoDB constructs when is_fully_automatable is false.
    std::string migrated_query;

    /// True when all constructs were automatically transformed with no manual action needed.
    bool is_fully_automatable = true;

    /// Per-construct migration issues ordered by appearance in the source query.
    std::vector<MigrationIssue> issues;

    /// @return Single-line human-readable summary (e.g. "1 warning, 2 info")
    std::string summary() const;
};

/**
 * @brief Rule-based ArangoDB AQL → ThemisDB AQL migration assistant.
 *
 * Applies a deterministic set of transformation rules to convert ArangoDB AQL
 * queries into ThemisDB AQL queries without requiring a live LLM.
 *
 * ## Transformations applied
 *
 * ### Automatic rewrites (WARNING issued, manual review recommended)
 * - `NEAR(collection, lat, lng, n)` → `ST_DISTANCE()`-based geo query pattern
 * - `WITHIN(collection, lat, lng, radius)` → `ST_DISTANCE()`-based FILTER with `<=` radius check
 * - `FULLTEXT(collection, attr, query)` → `SIMILARITY()`-based full-text search pattern
 * - `DOCUMENT(collection, key)` → inline `FOR`/`FILTER`/`LIMIT 1`/`RETURN` sub-query
 * - `@@collection` bind parameter → `@collection` (ThemisDB convention, one `@`)
 *
 * ### Unsupported constructs (ERROR issued, manual rewrite required)
 * - `V8(expression)` — ArangoDB-only JavaScript evaluation, not available in ThemisDB
 *
 * ### Informational notes (INFO issued, no rewrite)
 * - `IS_STRING()`, `IS_NUMBER()`, `IS_BOOL()`, `IS_NULL()`, `IS_LIST()`, `IS_DOCUMENT()`
 *   → ThemisDB uses `TYPENAME()` for runtime type introspection; consider replacing
 * - `HASH()` → not available; use `SHA256()` or a custom hashing expression
 * - `ATTRIBUTES()` → not available; consider `KEYS()` if applicable
 * - `TRANSLATE()` → not available; use a `FOR`/`FILTER` lookup instead
 *
 * ### Compatible pass-through (no changes)
 * All standard AQL clauses and most built-in functions are compatible:
 * FOR, IN, FILTER, SORT, LIMIT, RETURN, LET, COLLECT, AGGREGATE,
 * GRAPH traversal, OUTBOUND/INBOUND/ANY, UPSERT, INSERT, UPDATE, REPLACE, REMOVE,
 * MERGE(), KEEP(), UNSET(), LENGTH(), CONCAT(), UPPER(), LOWER(), SUM(), AVG(),
 * MIN(), MAX(), COUNT(), ST_DISTANCE(), ST_WITHIN(), SIMILARITY(), PROXIMITY(),
 * _key, _id, _rev, _from, _to.
 *
 * ## Usage example
 * @code
 *   AQLMigrationAssistant assistant;
 *   auto result = assistant.migrate(
 *       "FOR doc IN FULLTEXT(articles, 'title', 'database') RETURN doc"
 *   );
 *   std::cout << result.migrated_query << '\n';
 *   for (auto& issue : result.issues)
 *       std::cout << issue.message << '\n';
 * @endcode
 */
class AQLMigrationAssistant {
public:
    AQLMigrationAssistant()  = default;
    ~AQLMigrationAssistant() = default;

    /**
     * @brief Migrate an ArangoDB AQL query to ThemisDB AQL.
     *
     * Applies all applicable transformation rules in a single pass.
     * The input is not modified; a new string is returned in MigrationResult.
     *
     * @param arango_aql  ArangoDB AQL query string (may span multiple lines).
     * @return MigrationResult containing the migrated query and per-construct issues.
     */
    MigrationResult migrate(const std::string& arango_aql) const;

private:
    /// Rewrite NEAR(collection, lat, lng, n) → ST_DISTANCE-based pattern
    std::string rewriteNear(
        const std::string& query,
        std::vector<MigrationIssue>& issues
    ) const;

    /// Rewrite WITHIN(collection, lat, lng, radius) → ST_DISTANCE-based FILTER
    std::string rewriteWithin(
        const std::string& query,
        std::vector<MigrationIssue>& issues
    ) const;

    /// Rewrite FULLTEXT(collection, attr, searchQuery) → SIMILARITY-based pattern
    std::string rewriteFulltext(
        const std::string& query,
        std::vector<MigrationIssue>& issues
    ) const;

    /// Rewrite DOCUMENT(collection, key) → inline FOR/FILTER/LIMIT 1/RETURN
    std::string rewriteDocument(
        const std::string& query,
        std::vector<MigrationIssue>& issues
    ) const;

    /// Rewrite @@collection bind parameters → @collection
    std::string rewriteDoubleAtBind(
        const std::string& query,
        std::vector<MigrationIssue>& issues
    ) const;

    /// Flag V8() as unsupported (no rewrite)
    void detectV8(
        const std::string& query,
        std::vector<MigrationIssue>& issues
    ) const;

    /// Flag ArangoDB-specific type-check functions (IS_STRING, etc.) as informational
    void detectTypeCheckFunctions(
        const std::string& query,
        std::vector<MigrationIssue>& issues
    ) const;

    /// Flag HASH() as informational (not available in ThemisDB)
    void detectHashFunction(
        const std::string& query,
        std::vector<MigrationIssue>& issues
    ) const;

    /// Flag ATTRIBUTES() as informational
    void detectAttributesFunction(
        const std::string& query,
        std::vector<MigrationIssue>& issues
    ) const;

    /// Flag TRANSLATE() as informational
    void detectTranslateFunction(
        const std::string& query,
        std::vector<MigrationIssue>& issues
    ) const;

    /// Extract a balanced-parentheses argument list starting at position @p open_paren
    /// Returns the content between the parentheses or empty string on failure.
    static std::string extractArgs(const std::string& src, std::size_t open_paren);
};

} // namespace aql
} // namespace themis
