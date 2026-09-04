/**
 * @file federation_admin_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "api/federation_admin_handler.h"

#include <stdexcept>

namespace themis::api {

/**
 * @brief Constructs the FederationAdminHandler.
 * 
 * Initializes the handler with necessary system components.
 * The coordinator must be non-null to ensure basic federation capabilities.
 * 
 * @param coordinator A shared pointer managing global coordination state within the network.
 * @param merger A shared pointer handling RAG (Retrieval-Augmented Generation) data merging logic.
 */
FederationAdminHandler::FederationAdminHandler(
    std::shared_ptr<distributed_knowledge::LoRAFederationCoordinator> coordinator,
    std::shared_ptr<distributed_knowledge::FederatedRAGMerger>        merger)
    : coordinator_(std::move(coordinator))
    , merger_(std::move(merger))
{
    if (!coordinator_) {
        throw std::invalid_argument("FederationAdminHandler: coordinator must not be null");
    }
}

/**
 * @brief Retrieves a comprehensive set of system statistics across all federated modules.
 *
 * The handler enriches the coordinator payload with privacy-budget fields so the
 * admin endpoint can expose a complete observability snapshot.
 *
 * @return JSON object containing the coordinator stats plus privacy metadata.
 */
nlohmann::json FederationAdminHandler::getStats() const {
    auto stats = coordinator_->getStats();
    // Enrich with privacy-budget fields
    stats["privacy_budget_remaining"] = coordinator_->privacyBudgetRemaining();
    stats["budget_verified"]          = coordinator_->verifyPrivacyBudget();
    return stats;
}

/**
 * @brief Retrieves a consolidated statistics report about the system's Read/Archive/Gateway status.
 *
 * @return JSON object with merger statistics or `{"available": false}` when no merger is configured.
 */
nlohmann::json FederationAdminHandler::getRagStats() const {
    if (!merger_) {
        return {{"available", false}};
    }
    const auto& cfg = merger_->config();
    return {{"available",       true},
            {"merge_strategy",  static_cast<int>(cfg.strategy)},
            {"top_k",           cfg.top_k},
            {"deduplicate",     cfg.deduplicate},
            {"rrf_constant",    cfg.rrf_constant}};
}

/**
 * @brief Triggers the full cycle of data synchronization and round processing across connected federations.
 *
 * @param algorithm Optional aggregation algorithm override. The current coordinator
 *                  implementation uses its configured default, so non-empty values
 *                  are accepted for API compatibility but not yet interpreted here.
 * @return JSON object describing the triggered round.
 */
nlohmann::json FederationAdminHandler::triggerRound(const std::string& algorithm) {
    (void)algorithm;

    // verifyPrivacyBudget() is also checked inside triggerAggregation(); we
    // surface the same check here first to allow callers to map exceptions to
    // HTTP status codes before the coordinator performs its own guard.
    if (!coordinator_->verifyPrivacyBudget()) {
        throw std::runtime_error("DP budget exhausted");
    }

    const auto delta = coordinator_->triggerAggregation();

    return {{"round",          delta.round},
            {"participants",   delta.participants},
            {"delta_version",  delta.version},
            {"epsilon_spent",  delta.epsilon_spent},
            {"algorithm",      delta.algorithm},
            {"status",         "success"}};
}

} // namespace themis::api

