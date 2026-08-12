/**
 * @file federated_learning.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <cstddef>
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
    /** @brief Federated aggregator. */
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
    /** @brief Differential privacy. */
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

    // ------------------------------------------------------------------
    // Secure aggregation primitive (Wave C C2 optional HE-style stub)
    // ------------------------------------------------------------------
    /** @brief Secure aggregation primitive (Wave C C2 optional HE-style stub). */
    class SecureAggregationManager {
    public:
        /**
         * @brief Apply deterministic per-participant mask to a gradient vector.
         *
         * The mask is derived from (participant_id, round_id) and can be
         * subtracted after summation to recover the original aggregate.
         * This models secure aggregation flow without introducing full HE.
         */
        std::vector<double> maskGradient(
            const std::vector<double>& gradient,
            const std::string& participant_id,
            const std::string& round_id
        ) const;

        /**
         * @brief Remove aggregate mask from summed masked gradients.
         */
        std::vector<double> unmaskAggregatedGradient(
            const std::vector<double>& masked_sum,
            const std::vector<std::string>& participant_ids,
            const std::string& round_id
        ) const;
    };

    // ------------------------------------------------------------------
    // Federated training coordinator (synchronized SGD rounds)
    // ------------------------------------------------------------------
    /** @brief Federated training coordinator (synchronized SGD rounds). */
    class FederatedTrainingCoordinator {
    public:
        struct ParticipantGradient {
            std::string participant_id;
            std::vector<double> gradient;
            std::size_t sample_count{1};
        };

        struct RoundAggregationResult {
            std::vector<double> aggregated_gradient;
            std::size_t participants{0};
            std::size_t total_samples{0};
            std::string algorithm_used{"FedAvg"};
            bool secure_aggregation_used{false};
        };

        /**
         * @brief Aggregate one synchronized SGD round.
         *
         * Supported algorithms:
         *   - "FedAvg" (sample-count-weighted mean)
         *   - "median" (element-wise median)
         *   - "trimmed_mean" (element-wise trimmed mean)
         */
        RoundAggregationResult aggregateRound(
            const std::vector<ParticipantGradient>& updates,
            const std::string& aggregation_algorithm = "FedAvg",
            bool use_secure_aggregation = false,
            const std::string& round_id = "round-0",
            double trim_ratio = 0.1
        ) const;
    };
};

} // namespace importers
} // namespace themis
