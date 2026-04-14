/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            federated_learning.cpp                             ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-04-14 07:01:44                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     156                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 9efa3acd76  2026-03-11  feat(importers): add PostgreSQL Importer v2.1+ with 12 ne... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "importers/federated_learning.h"
#include <cmath>
#include <random>
#include <stdexcept>

namespace themis {
namespace importers {

// ---------------------------------------------------------------------------
// FederatedAggregator
// ---------------------------------------------------------------------------

json FederatedImportCoordinator::FederatedAggregator::aggregateUpdates(
    const std::vector<ParticipantUpdate>& updates,
    const std::string& aggregation_algorithm)
{
    if (updates.empty()) return json::object();

    if (aggregation_algorithm == "FedAvg" || aggregation_algorithm == "median") {
        // FedAvg: average numeric statistics fields across all participants
        json aggregated = json::object();

        // Collect all numeric keys from the first update's statistics
        if (!updates[0].statistics.is_object()) return json::object();

        for (auto it = updates[0].statistics.begin();
             it != updates[0].statistics.end(); ++it)
        {
            const std::string& key = it.key();
            if (!it.value().is_number()) continue;

            double sum = 0.0;
            size_t cnt = 0;
            std::vector<double> values;

            for (const auto& upd : updates) {
                if (upd.statistics.contains(key) &&
                    upd.statistics.at(key).is_number()) {
                    double v = upd.statistics.at(key).get<double>();
                    sum += v;
                    values.push_back(v);
                    ++cnt;
                }
            }

            if (cnt == 0) continue;

            if (aggregation_algorithm == "median" && !values.empty()) {
                std::sort(values.begin(), values.end());
                size_t mid = values.size() / 2;
                aggregated[key] = values.size() % 2 == 0
                    ? (values[mid - 1] + values[mid]) / 2.0
                    : values[mid];
            } else {
                aggregated[key] = sum / cnt; // FedAvg
            }
        }

        // Merge schema contributions (union of all field names)
        json schema_union = json::object();
        for (const auto& upd : updates) {
            if (upd.schema_contribution.is_object()) {
                for (auto it = upd.schema_contribution.begin();
                     it != upd.schema_contribution.end(); ++it) {
                    if (!schema_union.contains(it.key())) {
                        schema_union[it.key()] = it.value();
                    }
                }
            }
        }

        aggregated["_schema"] = schema_union;
        aggregated["_participants"] = updates.size();
        return aggregated;
    }

    // FedProx or unknown: fall back to FedAvg
    return aggregateUpdates(updates, "FedAvg");
}

// ---------------------------------------------------------------------------
// DifferentialPrivacyManager
// ---------------------------------------------------------------------------

json FederatedImportCoordinator::DifferentialPrivacyManager::addDifferentialPrivacy(
    const json& statistics,
    double epsilon,
    double delta)
{
    if (epsilon <= 0.0 || delta <= 0.0 || delta >= 1.0) {
        throw std::invalid_argument(
            "Differential privacy requires epsilon > 0 and 0 < delta < 1");
    }

    // Gaussian mechanism: noise sigma = sensitivity * sqrt(2 * ln(1.25/delta)) / epsilon
    // We use sensitivity = 1 (L2-sensitivity for counts/averages)
    double sensitivity = 1.0;
    double sigma = sensitivity * std::sqrt(2.0 * std::log(1.25 / delta)) / epsilon;

    std::random_device rd;
    std::mt19937_64 rng(rd());
    std::normal_distribution<double> noise(0.0, sigma);

    json noisy = statistics;
    for (auto it = noisy.begin(); it != noisy.end(); ++it) {
        if (it.value().is_number()) {
            double v = it.value().get<double>() + noise(rng);
            it.value() = v;
        }
    }

    return noisy;
}

bool FederatedImportCoordinator::DifferentialPrivacyManager::verifyPrivacyBudget(
    double epsilon_total,
    double delta)
{
    // Simple composition: budget not exceeded if epsilon_total <= some threshold
    // Standard practice: epsilon_total <= 1.0 for "strong" privacy
    return epsilon_total <= 1.0 && delta <= 1e-5 && delta > 0.0;
}

void FederatedImportCoordinator::DifferentialPrivacyManager::spendBudget(
    double epsilon_used)
{
    if (epsilon_used < 0.0) {
        throw std::invalid_argument("epsilon_used must be non-negative");
    }
    epsilon_spent_ += epsilon_used;
}

} // namespace importers
} // namespace themis
