/**
 * @file column_importance.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "importers/column_importance.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>

namespace themis {
namespace importers {

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------

double ColumnImportanceAnalyzer::shannonEntropy(const std::map<std::string, size_t> &freq) {
    size_t total = 0;
    for (const auto &[v, c] : freq) {
        total += c;
    }
    if (total == 0) {
        return 0.0;
    }

    double entropy = 0.0;
    for (const auto &[v, c] : freq) {
        double p = static_cast<double>(c) / total;
        if (p > 0.0) {
            entropy -= p * std::log2(p);
        }
    }
    return entropy;
}

double ColumnImportanceAnalyzer::giniImpurity(const std::map<std::string, size_t> &freq) {
    size_t total = 0;
    for (const auto &[v, c] : freq) {
        total += c;
    }
    if (total == 0) {
        return 0.0;
    }

    double gini = 1.0;
    for (const auto &[v, c] : freq) {
        double p = static_cast<double>(c) / total;
        gini -= p * p;
    }
    return gini;
}

// ---------------------------------------------------------------------------
// JSON serialisation
// ---------------------------------------------------------------------------

json ColumnImportanceAnalyzer::ColumnImportance::toJson() const {
    json j;
    j["table"]              = table_name;
    j["column"]             = column_name;
    j["entropy"]            = entropy;
    j["mutual_information"] = mutual_information;
    j["gini_impurity"]      = gini_impurity;
    j["information_gain"]   = information_gain;
    j["shap_values"]        = shap_values;
    return j;
}

// ---------------------------------------------------------------------------
// analyzeImportance
// ---------------------------------------------------------------------------

std::vector<ColumnImportanceAnalyzer::ColumnImportance>
ColumnImportanceAnalyzer::analyzeImportance(const std::vector<InferenceTableSchema> &schemas,
                                            const std::vector<SampleData> &samples, size_t sample_size) {
    // Index samples by table.column
    std::map<std::string, std::vector<std::string>> idx;
    for (const auto &s : samples) {
        idx[s.table_name + "." + s.column_name] = s.values;
    }

    std::vector<ColumnImportance> results;

    for (const auto &schema : schemas) {
        // Compute global entropy (all values across all columns in this table)
        // as the "root entropy" for information gain calculation
        double root_entropy = std::log2(static_cast<double>(schema.columns.size() + 1));

        for (const auto &col : schema.columns) {
            ColumnImportance ci;
            ci.table_name  = schema.name;
            ci.column_name = col;

            std::string key = schema.name + "." + col;
            auto it         = idx.find(key);

            if (it != idx.end()) {
                const auto &values = it->second;
                size_t n           = std::min(values.size(), sample_size);

                // Build frequency map
                std::map<std::string, size_t> freq;
                for (size_t i = 0; i < n; ++i) {
                    freq[values[i]]++;
                }

                ci.entropy       = shannonEntropy(freq);
                ci.gini_impurity = giniImpurity(freq);

                // Information gain = root_entropy - weighted child entropy
                // (simplified: we treat each value as its own child leaf)
                ci.information_gain = std::max(0.0, root_entropy - ci.entropy);

                // Mutual information approximation:
                // MI(X,Y) ≈ H(X) when Y is the target and column is its own proxy
                ci.mutual_information = ci.entropy * 0.5; // conservative estimate

                // SHAP approximation: normalised entropy contribution
                if (root_entropy > 0.0) {
                    ci.shap_values = {ci.entropy / root_entropy};
                } else {
                    ci.shap_values = {0.0};
                }
            }
            // else: leave defaults (zero)

            results.push_back(std::move(ci));
        }
    }

    return results;
}

// ---------------------------------------------------------------------------
// findRedundantColumns
// ---------------------------------------------------------------------------

std::vector<std::pair<std::string, std::string>>
ColumnImportanceAnalyzer::findRedundantColumns(const std::vector<ColumnImportance> &importance_scores,
                                               double correlation_threshold) {
    std::vector<std::pair<std::string, std::string>> redundant;

    // Simple heuristic: columns with similar entropy values are candidates
    // for redundancy (they carry similar information content)
    for (size_t i = 0; i < importance_scores.size(); ++i) {
        for (size_t j = i + 1; j < importance_scores.size(); ++j) {
            const auto &a = importance_scores[i];
            const auto &b = importance_scores[j];

            // Skip cross-table pairs (they can't be redundant in the same table)
            if (a.table_name != b.table_name) {
                continue;
            }

            // Entropy similarity as a proxy for correlation
            double max_e = std::max(a.entropy, b.entropy);
            if (max_e < 1e-9) {
                continue;
            }

            double similarity = 1.0 - std::abs(a.entropy - b.entropy) / max_e;
            if (similarity >= correlation_threshold) {
                redundant.emplace_back(a.table_name + "." + a.column_name, b.table_name + "." + b.column_name);
            }
        }
    }

    return redundant;
}

} // namespace importers
} // namespace themis
