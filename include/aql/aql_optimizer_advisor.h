/**
 * @file aql_optimizer_advisor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>

#include "aql/aql_query_validator.h"

namespace themis {
namespace aql {

class AQLOptimizerAdvisor {
public:
    AQLOptimizerAdvisor()  = default;
    ~AQLOptimizerAdvisor() = default;

    /**
     * @brief Suggest.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    std::vector<ValidationIssue> suggest(const std::string& query) const;
};

} // namespace aql
} // namespace themis
