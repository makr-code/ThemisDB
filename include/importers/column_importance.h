/**
 * @file column_importance.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Information-theoretic column importance analysis.
 *
 * Implements Shannon Entropy, Mutual Information, Gini Impurity, and
 * Information Gain (ID3 algorithm) to rank columns by predictive value
 * and identify redundant candidates for denormalisation.
 *
 * References:
 *   - Breiman (2001) "Feature Importance Using the Permutation Method"
 *   - Tibshirani (1996) "The Lasso: A Shrinkage and Selection Method"
 */
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

        json toJson() const;
    };

    /**
     * @brief Analyse column importance across all sampled tables.
     *
     * @param schemas   Table schema descriptions.
     * @param samples   Sampled column values (up to sample_size rows per column).
     * @param sample_size  Maximum number of rows used per column analysis.
     */
    std::vector<ColumnImportance> analyzeImportance(
        const std::vector<InferenceTableSchema>& schemas,
        const std::vector<SampleData>& samples,
        size_t sample_size = 10000
    );

    /**
     * @brief Identify column pairs with Pearson correlation above threshold.
     *
     * These pairs are candidates for denormalisation or index elimination.
     *
     * @param correlation_threshold  Default 0.95 (95 % correlation).
     */
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
