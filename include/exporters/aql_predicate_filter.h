/**
 * @file aql_predicate_filter.h
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
#include <memory>
#include <stdexcept>
#include "storage/base_entity.h"

// Forward declarations
namespace themis {
namespace query {
    struct FilterNode;
} // namespace query
} // namespace themis

namespace themis::exporters {

class AqlPredicateFilterException : public std::runtime_error {
public:
    /**
     * @brief Aql Predicate Filter Exception.
     * @param[in] msg Input parameter.
     * @return Return value.
     */
    explicit AqlPredicateFilterException(const std::string& msg)
        : std::runtime_error(msg) {}
};

class AqlPredicateFilter {
public:
    /**
     * @brief Aql Predicate Filter.
     * @param[in] predicate Input parameter.
     * @return Return value.
     */
    explicit AqlPredicateFilter(const std::string& predicate);

    ~AqlPredicateFilter();

    AqlPredicateFilter(const AqlPredicateFilter&) = delete;
    AqlPredicateFilter& operator=(const AqlPredicateFilter&) = delete;
    AqlPredicateFilter(AqlPredicateFilter&&) noexcept = default;
    AqlPredicateFilter& operator=(AqlPredicateFilter&&) noexcept = default;

    /**
     * @brief Evaluate.
     * @param[in] entity Input parameter.
     * @return True when the operation succeeds.
     */
    bool evaluate(const BaseEntity& entity) const;

    const std::string& getPredicate() const { return predicate_; }

private:
    std::string predicate_;
    std::vector<std::shared_ptr<themis::query::FilterNode>> filter_nodes_;
};

} // namespace themis::exporters
