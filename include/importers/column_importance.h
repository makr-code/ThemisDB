/**
 * @file column_importance.h
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
#include <map>
#include <utility>
#include <nlohmann/json.hpp>

namespace themis {
namespace importers {

class ColumnImportanceAnalyzer {
public:
    struct ColumnImportance {
        std::string table_name;
        std::string column_name;
        double entropy{0.0};             ///< Shannon Entropy (bits)
        double mutual_information{0.0};  ///< With target column
        double gini_impurity{0.0};       ///< For classification tasks
        double information_gain{0.0};    ///< ID3 algorithm gain
        std::vector<double> shap_values; ///< SHAP feature importance (approximated)

        /**
         * @brief To Json.
         * @return Return value.
         */
        json toJson() const;
    };

    std::vector<ColumnImportance> analyzeImportance(
        const std::vector<InferenceTableSchema>& schemas,
        const std::vector<SampleData>& samples,
        size_t sample_size = 10000
    );

    std::vector<std::pair<std::string, std::string>>
    findRedundantColumns(
        const std::vector<ColumnImportance>& importance_scores,
        double correlation_threshold = 0.95
    );

private:
    // Entropy of a discrete distribution (value → count)
    static double shannonEntropy(const std::map<std::string, size_t>& freq);
    // Gini impurity of a discrete distribution
    static double giniImpurity(const std::map<std::string, size_t>& freq);
};

} // namespace importers
} // namespace themis
