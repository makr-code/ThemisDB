/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            aql_query_diff_explainer.h                         ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-04-15 18:01:54                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     148                                            ║
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
 * @file aql_query_diff_explainer.h
 * @brief Structural diff explainer for pairs of AQL queries.
 *
 * Performs a clause-level diff between two AQL strings: normalises whitespace,
 * splits each query into its canonical clauses (FOR, LET, FILTER, SORT, LIMIT,
 * RETURN, COLLECT, INSERT, UPDATE, REMOVE, UPSERT, REPLACE), and reports every
 * clause that was added, removed, or changed.
 *
 * No LLM dependency is required — the analysis is purely rule-based and
 * runs in O(n) time where n is the total number of detected clauses.
 *
 * Compile guards: no external dependencies; always compiled.
 *
 * @see IAQLQueryDiffExplainer
 * @see AQLQueryDiffExplainer
 * @see AQLMigrationAssistant  for automated query migration
 */

#pragma once

#include <string>
#include <vector>

namespace themis {
namespace aql {

// ============================================================================
// IAQLQueryDiffExplainer
// ============================================================================

/**
 * @brief A single structural difference between two AQL queries.
 */
struct QueryDiffEntry {
    enum class Kind {
        CLAUSE_ADDED,    ///< A clause present in @c b is absent from @c a
        CLAUSE_REMOVED,  ///< A clause present in @c a is absent from @c b
        CLAUSE_CHANGED,  ///< A clause changed between @c a and @c b
        FILTER_CHANGED,  ///< The filter/predicate logic changed
        SORT_CHANGED,    ///< The sort order or sort keys changed
        LIMIT_CHANGED,   ///< The LIMIT / OFFSET values changed
        RETURN_CHANGED,  ///< The return projection changed
        COLLECTION_CHANGED, ///< A FOR-loop collection reference changed
        FUNCTION_ADDED,  ///< A built-in function call was added
        FUNCTION_REMOVED,///< A built-in function call was removed
        STRUCTURAL,      ///< Other structural difference
    };

    Kind        kind;
    std::string clause_a;    ///< Relevant fragment from query @c a (may be empty)
    std::string clause_b;    ///< Relevant fragment from query @c b (may be empty)
    std::string explanation; ///< Human-readable explanation of this difference
};

/**
 * @brief Aggregated result of a diff between two AQL queries.
 */
struct QueryDiffResult {
    /// Ordered list of structural differences (empty iff queries are equivalent).
    std::vector<QueryDiffEntry> diffs;

    /// Human-readable summary of all differences (single string, newline-separated).
    std::string summary;

    /// True when no structural differences were found.
    bool is_equivalent = false;

    /**
     * @brief Count entries of the given kind.
     */
    int count(QueryDiffEntry::Kind kind) const;
};

/**
 * @brief Explains the structural and semantic differences between two AQL queries.
 *
 * The explainer performs a clause-level diff: it normalises whitespace, splits
 * each query into its canonical clauses (FOR, LET, FILTER, SORT, LIMIT, RETURN,
 * COLLECT, INSERT, UPDATE, REMOVE, UPSERT, REPLACE), and reports every clause
 * that was added, removed, or changed.
 *
 * No LLM dependency is required — the analysis is purely rule-based and
 * runs in O(n) time with respect to the number of clauses.
 *
 * Typical use cases:
 *  - Showing users what changed when an AQL query is auto-migrated
 *  - Diff-view in query history / versioning UI
 *  - Regression tests: assert that an optimised query is semantically equivalent
 *    to the original (zero diffs except SORT/LIMIT)
 *
 * @see AQLMigrationAssistant  for automated query migration
 * @see AQLOptimizerAdvisor    for performance suggestions
 */
class IAQLQueryDiffExplainer {
public:
    virtual ~IAQLQueryDiffExplainer() = default;

    /**
     * @brief Compare two AQL queries and return an ordered list of differences.
     *
     * @param query_a  First AQL query (the "before" version)
     * @param query_b  Second AQL query (the "after" version)
     * @return         QueryDiffResult containing zero or more QueryDiffEntry items
     */
    virtual QueryDiffResult explain(const std::string& query_a,
                                    const std::string& query_b) const = 0;
};

/**
 * @brief Default production implementation of IAQLQueryDiffExplainer.
 *
 * Performs clause-level structural diffing without any external dependencies.
 */
class AQLQueryDiffExplainer : public IAQLQueryDiffExplainer {
public:
    AQLQueryDiffExplainer()  = default;
    ~AQLQueryDiffExplainer() override = default;

    QueryDiffResult explain(const std::string& query_a,
                            const std::string& query_b) const override;
};

} // namespace aql
} // namespace themis
