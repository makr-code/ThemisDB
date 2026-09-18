/**
 * @file federated_learning.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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
        json addDifferentialPrivacy(
            const json& statistics,
            double epsilon = 0.1,
            double delta = 1e-5
        );

        /**
         * @brief Verify Privacy Budget.
         * @param[in] epsilon_total Input parameter.
         * @param[in] delta Input parameter.
         * @return True when the operation succeeds.
         */
        bool verifyPrivacyBudget(double epsilon_total, double delta);

        /**
         * @brief Spend Budget.
         * @param[in] epsilon_used Input parameter.
         */
        void spendBudget(double epsilon_used);

        double totalEpsilonSpent() const { return epsilon_spent_; }

    private:
        double epsilon_spent_{0.0};
    };

    // ------------------------------------------------------------------
    // Secure aggregation primitive (Wave C C2 optional HE-style stub)
    // ------------------------------------------------------------------
    class SecureAggregationManager {
    public:
        /**
         * @brief Mask Gradient.
         * @param[in] gradient Input parameter.
         * @param[in] participant_id Identifier of the participant.
         * @param[in] round_id Identifier of the round.
         * @return Return value.
         */
        std::vector<double> maskGradient(
            const std::vector<double>& gradient,
            const std::string& participant_id,
            const std::string& round_id
        ) const;

        /**
         * @brief Unmask Aggregated Gradient.
         * @param[in] masked_sum Input parameter.
         * @param[in] participant_ids Input parameter.
         * @param[in] round_id Identifier of the round.
         * @return Return value.
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
