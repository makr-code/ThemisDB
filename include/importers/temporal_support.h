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

class TemporalDatabaseSupport {
public:
    enum class TemporalModel {
        VALID_TIME,       ///< When the fact was true  (business time)
        TRANSACTION_TIME, ///< When the fact was stored (database time)
        BI_TEMPORAL       ///< Both dimensions together
    };

    /**
     * @brief Temporal Model To String.
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
     * @brief Detect Temporal Dimensions.
     * @param[in] schemas Input parameter.
     * @return Return value.
     */
    std::vector<TemporalSchema> detectTemporalDimensions(
        const std::vector<InferenceTableSchema>& schemas
    );

    // ------------------------------------------------------------------
    // Point-in-time query builder
    // ------------------------------------------------------------------
    class TemporalQueryBuilder {
    public:
        /**
         * @brief Build Point In Time Query.
         * @param[in] temporal Input parameter.
         * @param[in] timestamp Input parameter.
         * @return Return value.
         */
        std::string buildPointInTimeQuery(
            const TemporalSchema& temporal,
            const std::string& timestamp
        );

        /**
         * @brief Build System Time Query.
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
     * @brief Is Valid Time Column.
     * @param[in] col_name Name of the col.
     * @return True when the operation succeeds.
     */
    static bool isValidTimeColumn(const std::string& col_name);
    /**
     * @brief Is Transaction Time Column.
     * @param[in] col_name Name of the col.
     * @return True when the operation succeeds.
     */
    static bool isTransactionTimeColumn(const std::string& col_name);
};

} // namespace importers
} // namespace themis
