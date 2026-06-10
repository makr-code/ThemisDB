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

nlohmann::json FederationAdminHandler::getStats() const {
    auto stats = coordinator_->getStats();
    // Enrich with privacy-budget fields
    stats["privacy_budget_remaining"] = coordinator_->privacyBudgetRemaining();
    stats["budget_verified"]          = coordinator_->verifyPrivacyBudget();
    return stats;
}

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

nlohmann::json FederationAdminHandler::triggerRound(const std::string& /*algorithm*/) {
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

