/**
 * @file temporal_support.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "importers/schema_inference.h"
#include <string>
#include <vector>

namespace themis {
namespace importers {

/**
 * @brief SQL:2011 Temporal Database support for import pipelines.
 *
 * Detects temporal dimensions in existing PostgreSQL schemas and emits
 * point-in-time reconstruction queries.
 *
 * Standard: ISO/IEC 9075-2:2011 SQL:2011 (Part 2: Foundation)
 */
class TemporalDatabaseSupport {
public:
    enum class TemporalModel {
        VALID_TIME,       ///< When the fact was true  (business time)
        TRANSACTION_TIME, ///< When the fact was stored (database time)
        BI_TEMPORAL       ///< Both dimensions together
    };

    /**
     * @brief TBD: Describe temporalModelToString.
     * @param[in] m Input parameter.
     * @return Return value.
     */
    static std::string temporalModelToString(TemporalModel m);

    struct TemporalSchema {
        std::string table_name;
        TemporalModel temporal_model{TemporalModel::VALID_TIME};

        // VALID TIME columns
        std::string valid_from_column;
        std::string valid_to_column;
        bool infinity_supported{true};  ///< NULL represents "forever"

        // TRANSACTION TIME (system-maintained)
        std::string transaction_from_column;
        std::string transaction_to_column;
    };

    /**
     * @brief Detect temporal columns in a set of schemas.
     *
     * Matches common naming conventions:
     *   valid_from, valid_to, effective_date, expiry_date,
     *   created_at, updated_at, deleted_at,
     *   sys_period_start, sys_period_end.
     * @param[in] schemas Input parameter.
     * @return Return value.
     */
    std::vector<TemporalSchema> detectTemporalDimensions(
        const std::vector<InferenceTableSchema>& schemas
    );

    // ------------------------------------------------------------------
    // Point-in-time query builder
    // ------------------------------------------------------------------
    /** @brief Point-in-time query builder. */
    class TemporalQueryBuilder {
    public:
        /**
         * @brief Generate SQL to reconstruct table state at a given instant.
         *
         * Example:
         *   SELECT * FROM orders
         *   WHERE valid_from <= '2023-01-15'
         *     AND (valid_to IS NULL OR valid_to > '2023-01-15')
         *
         * @param temporal   Detected temporal schema for the table.
         * @param timestamp  ISO 8601 timestamp string.
         * @return Return value.
         */
        std::string buildPointInTimeQuery(
            const TemporalSchema& temporal,
            const std::string& timestamp
        );

        /**
         * @brief Generate a SQL:2011 FOR SYSTEM_TIME AS OF query.
         * @param[in] temporal Input parameter.
         * @param[in] timestamp Input parameter.
         * @return Return value.
         */
        std::string buildSystemTimeQuery(
            const TemporalSchema& temporal,
            const std::string& timestamp
        );
    };

private:
    /**
     * @brief TBD: Describe isValidTimeColumn.
     * @param[in] col_name Input parameter.
     * @return True on success.
     */
    static bool isValidTimeColumn(const std::string& col_name);
    /**
     * @brief TBD: Describe isTransactionTimeColumn.
     * @param[in] col_name Input parameter.
     * @return True on success.
     */
    static bool isTransactionTimeColumn(const std::string& col_name);
};

} // namespace importers
} // namespace themis
