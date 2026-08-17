/**
 * @file aql_predicate_filter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/// Exception thrown when an AQL predicate expression cannot be parsed.
class AqlPredicateFilterException : public std::runtime_error {
public:
    explicit AqlPredicateFilterException(const std::string& msg)
        : std::runtime_error(msg) {}
};

/**
 * @brief Evaluates an AQL FILTER predicate against a BaseEntity.
 *
 * Accepts a predicate expression using the loop variable `doc`, for example:
 *   - `doc.category == "active"`
 *   - `doc.age > 18 AND doc.country == "DE"`
 *   - `doc.score >= 0.5`
 *
 * The expression is parsed once at construction time and reused for every
 * call to evaluate(), making batch filtering efficient.
 *
 * Usage:
 * @code
 *   AqlPredicateFilter filter("doc.category == \"active\"");
 *   for (const auto& entity : entities) {
 *       if (filter.evaluate(entity)) {
 *           // entity matches predicate
 *       }
 *   }
 * @endcode
 */
class AqlPredicateFilter {
public:
    /**
     * @brief Construct and compile the predicate.
     * @param predicate AQL FILTER expression (e.g. `doc.age > 18`)
     * @throws AqlPredicateFilterException if the predicate cannot be parsed
     */
    explicit AqlPredicateFilter(const std::string& predicate);

    ~AqlPredicateFilter();

    /// Non-copyable, movable
    AqlPredicateFilter(const AqlPredicateFilter&) = delete;
    AqlPredicateFilter& operator=(const AqlPredicateFilter&) = delete;
    AqlPredicateFilter(AqlPredicateFilter&&) noexcept noexcept = default;
    AqlPredicateFilter& operator=(AqlPredicateFilter&&) noexcept noexcept = default;

    /**
     * @brief Evaluate the predicate against a single entity.
     * @param entity The entity to test
     * @return true if the entity satisfies the predicate, false otherwise
     */
    bool evaluate(const BaseEntity& entity) const;

    /// Returns the original predicate string.
    const std::string& getPredicate() const { return predicate_; }

private:
    std::string predicate_;
    std::vector<std::shared_ptr<themis::query::FilterNode>> filter_nodes_;
};

} // namespace themis::exporters
