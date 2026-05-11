/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            aql_rollback_suggester.h                           ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-15 18:44:16                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     158                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 6897bb74a5  2026-04-13  docs(aql): Close all remaining ROADMAP items — Doxygen, L... ║
    • e8953e1175  2026-04-13  docs(aql): Close all remaining ROADMAP items — Doxygen, L... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file aql_rollback_suggester.h
 * @brief Rollback query generator for mutating AQL statements.
 *
 * Given a mutation query (INSERT / UPDATE / REPLACE / REMOVE / UPSERT), derives
 * a compensating AQL query that reverses the mutation.  Where a reversal requires
 * pre-mutation data (e.g. document snapshots for REMOVE), the output query uses
 * named bind-parameter placeholders (@snapshot, @old_values) together with a
 * caveat string and manual_steps list.
 *
 * No LLM dependency is required; all logic is rule-based and runs in O(n)
 * time where n is the query length.
 *
 * Compile guards: no external dependencies; always compiled.
 *
 * @see IAQLRollbackSuggester
 * @see AQLRollbackSuggester
 * @see IAQLQueryDiffExplainer  for query comparison / diffing
 */

#pragma once

#include &lt;optional&gt;
#include <string>
#include <vector>

namespace themis {
namespace aql {

// ============================================================================
// IAQLRollbackSuggester
// ============================================================================

/**
 * @brief Mutation type detected in an AQL query.
 */
enum class MutationType {
    INSERT,  ///< FOR … INSERT document INTO collection
    UPDATE,  ///< FOR … UPDATE key WITH changes IN collection
    REPLACE, ///< FOR … REPLACE key WITH document IN collection
    REMOVE,  ///< FOR … REMOVE key IN collection
    UPSERT,  ///< UPSERT searchExpr INSERT insertExpr UPDATE updateExpr IN collection
    NONE,    ///< Query is read-only or empty
};

/**
 * @brief Rollback suggestion for a single mutating AQL query.
 */
struct RollbackSuggestion {
    /// Whether a rollback query could be generated automatically.
    bool is_automatic = false;

    /// The suggested rollback AQL query string.
    /// - For INSERT: `FOR d IN <collection> FILTER <filter_expr> REMOVE d IN <collection>`
    /// - For REMOVE: `INSERT <document_expr> INTO <collection>` (requires snapshot)
    /// - For UPDATE/REPLACE: `FOR d IN <collection> FILTER <filter_expr> UPDATE d WITH <old_values> IN <collection>`
    /// - For UPSERT: `FOR d IN <collection> FILTER <search_expr> REMOVE d._key IN <collection>`
    /// Empty when @c is_automatic is false.
    std::string rollback_query;

    /// Mutation type that was detected in the original query.
    MutationType mutation_type = MutationType::NONE;

    /// The target collection extracted from the original query.
    std::string collection;

    /// Human-readable caveat or limitation note (e.g. "Pre-mutation snapshot required").
    std::string caveat;

    /// Caveats that require manual action before/after the rollback is executed.
    std::vector<std::string> manual_steps;
};

/**
 * @brief Generates rollback queries for mutating AQL statements.
 *
 * Given an AQL mutation query (INSERT / UPDATE / REPLACE / REMOVE / UPSERT),
 * the suggester derives a compensating (rollback) query that reverses the
 * mutation effect.
 *
 * **Limitations (clearly documented):**
 * - REMOVE rollback requires a pre-mutation document snapshot; the suggested
 *   query uses a placeholder `@snapshot` bind parameter.
 * - UPDATE/REPLACE rollback requires the old field values to be known; the
 *   suggester generates a template with a `@old_values` bind parameter.
 * - Queries that use dynamic collection names (bind parameters as collection
 *   refs) yield @c is_automatic = false with a manual instruction.
 * - Nested sub-queries that mix reads and writes are not fully analysed; only
 *   the outermost mutation is considered.
 *
 * No LLM dependency is required; all logic is rule-based and runs in O(n)
 * time.
 *
 * Typical usage:
 * @code
 *   AQLRollbackSuggester suggester;
 *   auto result = suggester.suggest(
 *       "FOR u IN users FILTER u.status == 'trial' "
 *       "UPDATE u WITH { status: 'active' } IN users");
 *   // result.rollback_query:
 *   // FOR u IN users FILTER u.status == 'active'
 *   //   UPDATE u WITH @old_values IN users
 * @endcode
 *
 * @see AQLMigrationAssistant  for query migration / rewriting
 * @see IAQLQueryDiffExplainer for query comparison
 */
class IAQLRollbackSuggester {
public:
    virtual ~IAQLRollbackSuggester() = default;

    /**
     * @brief Analyse a mutation query and return a rollback suggestion.
     *
     * @param aql_query  Mutating AQL statement to analyse
     * @return           RollbackSuggestion (is_automatic=false for read-only queries)
     */
    virtual RollbackSuggestion suggest(const std::string& aql_query) const = 0;
};

/**
 * @brief Default production implementation of IAQLRollbackSuggester.
 *
 * Uses regex-based clause extraction; runs in O(n) without external deps.
 */
class AQLRollbackSuggester : public IAQLRollbackSuggester {
public:
    AQLRollbackSuggester()  = default;
    ~AQLRollbackSuggester() override = default;

    RollbackSuggestion suggest(const std::string& aql_query) const override;
};

} // namespace aql
} // namespace themis
