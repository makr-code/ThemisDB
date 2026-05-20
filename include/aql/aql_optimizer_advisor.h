/*
 * ThemisDB | File: aql_optimizer_advisor.h | Version: 0.0.15
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=1; TODO=0, Stub=0, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include <string>
#include <vector>

#include "aql/aql_query_validator.h"

namespace themis {
namespace aql {

/**
 * @brief Cost-aware query suggestion advisor for AQL queries.
 *
 * Bridges the AQL module with the query optimizer's cost models to provide
 * actionable performance hints alongside structural validation.
 *
 * The advisor operates purely on the query text — no live index or storage
 * access is required.  It uses:
 *   - NlpTextAnalyzer for complexity estimation and index pattern detection
 *   - QueryOptimizer static cost models (VectorGeo, ContentGeo, GraphPath)
 *     to recommend execution plan ordering for hybrid query patterns
 *
 * All suggestions are returned as ValidationIssue objects with severity
 * INFO or WARNING so they can be merged into a ValidationResult or
 * presented independently.
 *
 * @see AQLQueryValidator  for structural/syntactic checks
 * @see QueryOptimizer     for the underlying cost models
 */
class AQLOptimizerAdvisor {
public:
    AQLOptimizerAdvisor()  = default;
    ~AQLOptimizerAdvisor() = default;

    /**
     * @brief Generate cost-aware suggestions for an AQL query string.
     *
     * Analyses the query text and returns zero or more ValidationIssue
     * objects with severity INFO or WARNING.  No ERROR issues are ever
     * produced — structural errors remain the domain of AQLQueryValidator.
     *
     * Checks performed:
     *  - Query complexity (WARNING when estimated complexity > 0.7)
     *  - Missing index opportunities (INFO when fulltext, vector, or spatial
     *    patterns are detected but no relevant index is named)
     *  - Vector + Geo hybrid ordering (INFO recommending the cheaper plan)
     *  - Fulltext + Geo hybrid ordering (INFO recommending the cheaper plan)
     *  - Graph traversal depth risk (WARNING when max depth is very large)
     *
     * @param query  AQL query string to analyse
     * @return       Vector of suggestions (may be empty)
     */
    std::vector<ValidationIssue> suggest(const std::string& query) const;
};

} // namespace aql
} // namespace themis
