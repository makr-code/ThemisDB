// Copyright 2026 ThemisDB — Licensed under MIT License

/**
 * @file federation_admin_handler.cpp
 * @brief DK-7: Admin API handler implementation.
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
            {"merge_strategy",  static_cast&lt;int&gt;(cfg.strategy)},
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
