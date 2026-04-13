/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            federated_learning.h                               ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-04-13 04:15:39                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     119                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 9efa3acd76  2026-03-11  feat(importers): add PostgreSQL Importer v2.1+ with 12 ne... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace themis {
namespace importers {

using json = nlohmann::json;

/**
 * @brief Federated import coordination with differential privacy.
 *
 * Enables schema learning and statistics aggregation across multiple
 * PostgreSQL instances without centralising raw data.
 *
 * References:
 *   - Kairouz et al. (2021) "Federated Learning: Challenges, Methods,
 *     and Future Directions" (JMLR)
 *   - McMahan et al. (2018) "Learning Differentially Private Recurrent
 *     Language Models"
 *   - Dwork et al. – ε-δ Differential Privacy foundations
 */
class FederatedImportCoordinator {
public:
    // ------------------------------------------------------------------
    // Federated aggregator
    // ------------------------------------------------------------------
    class FederatedAggregator {
    public:
        struct ParticipantUpdate {
            std::string participant_id;
            json schema_contribution;    ///< Schema fragment from participant
            json statistics;             ///< Aggregate statistics (never raw data)
            json encrypted_gradient;     ///< DP-SGD gradient (opaque blob)
        };

        /**
         * @brief Aggregate updates from all participants using FedAvg or
         *        another algorithm.
         *
         * Only statistical summaries and schema contributions are exchanged;
         * raw row data never leaves the participant.
         *
         * @param updates               Per-participant contributions.
         * @param aggregation_algorithm "FedAvg" (default) | "FedProx" | "median"
         * @return Aggregated global model as JSON.
         */
        json aggregateUpdates(
            const std::vector<ParticipantUpdate>& updates,
            const std::string& aggregation_algorithm = "FedAvg"
        );
    };

    // ------------------------------------------------------------------
    // Differential privacy
    // ------------------------------------------------------------------
    class DifferentialPrivacyManager {
    public:
        /**
         * @brief Add calibrated Gaussian noise to statistics to achieve
         *        (epsilon, delta)-differential privacy.
         *
         * Noise standard deviation = sensitivity * sqrt(2*ln(1.25/delta)) / epsilon.
         *
         * @param statistics  JSON object with numeric fields.
         * @param epsilon     Privacy budget (smaller → stronger privacy).
         * @param delta       Failure probability (≤ 1e-5 recommended).
         * @return Statistics with added noise.
         */
        json addDifferentialPrivacy(
            const json& statistics,
            double epsilon = 0.1,
            double delta = 1e-5
        );

        /**
         * @brief Check whether the accumulated privacy spend is within budget.
         * @param epsilon_total  Total epsilon consumed so far.
         * @param delta          Delta parameter.
         * @return true if within acceptable privacy budget.
         */
        bool verifyPrivacyBudget(double epsilon_total, double delta);

        /** @brief Track privacy spend. */
        void spendBudget(double epsilon_used);

        double totalEpsilonSpent() const { return epsilon_spent_; }

    private:
        double epsilon_spent_{0.0};
    };
};

} // namespace importers
} // namespace themis
