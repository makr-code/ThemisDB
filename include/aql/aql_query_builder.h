/**
 * @file aql_query_builder.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.39
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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
     * @brief For In.
     * @param[in] variable Input parameter.
     * @param[in] collection Input parameter.
     * @return Return value.
     */
    AQLQueryBuilder& forIn(const std::string& variable, const std::string& collection);

    /**
     * @brief Filter.
     * @param[in] condition Input parameter.
     * @return Return value.
     */
    AQLQueryBuilder& filter(const std::string& condition);

    AQLQueryBuilder& sort(const std::string& field, bool ascending = true);

    AQLQueryBuilder& limit(int count, int offset = 0);

    /**
     * @brief Ret.
     * @param[in] expression Input parameter.
     * @return Return value.
     */
    AQLQueryBuilder& ret(const std::string& expression);

    /**
     * @brief Let.
     * @param[in] variable Input parameter.
     * @param[in] expression Input parameter.
     * @return Return value.
     */
    AQLQueryBuilder& let(const std::string& variable, const std::string& expression);

    /**
     * @brief Collect.
     * @param[in] variable Input parameter.
     * @param[in] expression Input parameter.
     * @return Return value.
     */
    AQLQueryBuilder& collect(const std::string& variable, const std::string& expression);

    // =========================================================================
    // Graph traversal
    // =========================================================================

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
     * @brief Insert Into.
     * @param[in] collection Input parameter.
     * @param[in] doc_expr Input parameter.
     * @return Return value.
     */
    AQLQueryBuilder& insertInto(const std::string& collection, const std::string& doc_expr);

    /**
     * @brief Update In.
     * @param[in] collection Input parameter.
     * @param[in] doc_expr Input parameter.
     * @return Return value.
     */
    AQLQueryBuilder& updateIn(const std::string& collection, const std::string& doc_expr);

    /**
     * @brief Remove In.
     * @param[in] collection Input parameter.
     * @param[in] doc_expr Input parameter.
     * @return Return value.
     */
    AQLQueryBuilder& removeIn(const std::string& collection, const std::string& doc_expr);

    /**
     * @brief Upsert In.
     * @param[in] collection Input parameter.
     * @param[in] filter_expr Input parameter.
     * @param[in] insert_expr Input parameter.
     * @param[in] update_expr Input parameter.
     * @return Return value.
     */
    AQLQueryBuilder& upsertIn(
        const std::string& collection,
        const std::string& filter_expr,
        const std::string& insert_expr,
        const std::string& update_expr
    );

    /**
     * @brief Replace In.
     * @param[in] collection Input parameter.
     * @param[in] doc_expr Input parameter.
     * @return Return value.
     */
    AQLQueryBuilder& replaceIn(const std::string& collection, const std::string& doc_expr);

    // =========================================================================
    // Ingestion enrichment flag (opt-in)
    // =========================================================================

    AQLQueryBuilder& withIngestionEnrichment(bool enabled = true);

    /**
     * @brief Has Ingestion Enrichment.
     * @return True when the operation succeeds.
     */
    bool hasIngestionEnrichment() const;

    // =========================================================================
    // WINDOW analytics clause
    // =========================================================================

    /**
     * @brief Window.
     * @param[in] partition_expr Input parameter.
     * @param[in] window_spec Input parameter.
     * @return Return value.
     */
    AQLQueryBuilder& window(const std::string& partition_expr, const std::string& window_spec);

    // =========================================================================
    // Subquery support
    // =========================================================================

    /**
     * @brief Subquery.
     * @param[in] variable Input parameter.
     * @param[in] inner Input parameter.
     * @return Return value.
     */
    AQLQueryBuilder& subquery(const std::string& variable, const AQLQueryBuilder& inner);

    /**
     * @brief Reset the modification detection flag.
     * @return None.
     */
    AQLQueryBuilder& reset();

    // =========================================================================
    // Query output
    // =========================================================================

    /**
     * @brief Build.
     * @return Return value.
     */
    std::string build() const;

    /**
     * @brief Get Partial Query.
     * @return Return value.
     */
    std::string getPartialQuery() const;

    // =========================================================================
    // Validation and state
    // =========================================================================

    /**
     * @brief Is Complete.
     * @return True when the operation succeeds.
     */
    bool isComplete() const;

    /**
     * @brief Is Valid.
     * @return True when the operation succeeds.
     */
    bool isValid() const;

    /**
     * @brief Validate.
     * @return Return value.
     */
    ValidationResult validate() const;

    /**
     * @brief Validate.
     * @param[in] schema Input parameter.
     * @return Return value.
     */
    ValidationResult validate(const std::vector<CollectionMetadata>& schema) const;

    /**
     * @brief ========================================================================= Schema-aware query generation (live collection metadata) =========================================================================
     * @param[in] schema Input parameter.
     * @return Return value.
     */

    AQLQueryBuilder& setSchema(const std::vector<CollectionMetadata>& schema);

    /**
     * @brief Get Schema Context.
     * @return Return value.
     */
    std::string getSchemaContext() const;

    /**
     * @brief Get Fields For Collection.
     * @param[in] collection Input parameter.
     * @return Return value.
     */
    std::vector<std::string> getFieldsForCollection(const std::string& collection) const;

    // =========================================================================
    // Suggestions
    // =========================================================================

    /**
     * @brief Get Next Steps.
     * @return Return value.
     */
    std::vector<std::string> getNextSteps() const;

    std::vector<std::string> getCompletionSuggestions(
        LLMAQLHandler& handler,
        const std::string& schema_context = "",
        int max_suggestions = 3
    ) const;

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
