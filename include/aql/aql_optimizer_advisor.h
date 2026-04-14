/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            aql_optimizer_advisor.h                            ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-04-14 18:36:51                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     80                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • fb7ab4631c  2026-02-23  feat(aql): integrate AQL module with query optimizer for ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
