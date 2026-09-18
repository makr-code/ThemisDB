/**
 * @file aql_query_diff_explainer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.9
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <string>
#include <vector>

namespace themis {
namespace aql {

// ============================================================================
// IAQLQueryDiffExplainer
// ============================================================================

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

struct QueryDiffResult {
    std::vector<QueryDiffEntry> diffs;

    std::string summary;

    bool is_equivalent = false;

    /**
     * @brief Count.
     * @param[in] kind Input parameter.
     * @return Return value.
     */
    int count(QueryDiffEntry::Kind kind) const;
};

class IAQLQueryDiffExplainer {
public:
    /**
     * @brief IAQLQuery Diff Explainer.
     * @return Return value.
     */
    virtual ~IAQLQueryDiffExplainer() = default;

    /**
     * @brief Explain.
     * @param[in] query_a Input parameter.
     * @param[in] query_b Input parameter.
     * @return Return value.
     */
    virtual QueryDiffResult explain(const std::string& query_a,
                                    const std::string& query_b) const = 0;
};

class AQLQueryDiffExplainer : public IAQLQueryDiffExplainer {
public:
    AQLQueryDiffExplainer()  = default;
    ~AQLQueryDiffExplainer() override = default;

    QueryDiffResult explain(const std::string& query_a,
                            const std::string& query_b) const override;
};

} // namespace aql
} // namespace themis
