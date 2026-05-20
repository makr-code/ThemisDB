/*
 * ThemisDB | File: federation_admin_handler.h | Version: 0.0.1
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// Copyright 2026 ThemisDB — Licensed under MIT License
#pragma once

/**
 * @file federation_admin_handler.h
 * @brief DK-7: Admin API handler for federated LoRA coordination.
 *
 * Exposes three programmatic endpoints suitable for wrapping in any HTTP
 * framework:
 *  - `getStats()`     → /admin/federation/stats
 *  - `getRagStats()`  → /admin/federation/rag-stats
 *  - `triggerRound()` → POST /admin/federation/trigger
 *
 * All methods are thread-safe (they delegate to the thread-safe coordinator
 * and merger).
 *
 * Error handling: methods throw `std::runtime_error` with specific messages:
 *  - "DP budget exhausted"          → HTTP 403
 *  - "Cross-border transfer blocked" → HTTP 503
 *  - "Insufficient participants"     → HTTP 400
 */

#include "distributed_knowledge/federated_rag_merger.h"
#include "distributed_knowledge/lora_federation_coordinator.h"

#include <memory>
#include <nlohmann/json.hpp>
#include <string>

namespace themis::api {

/**
 * @brief Admin handler for federation observability and manual control.
 *
 * Design: thin façade over `LoRAFederationCoordinator` and
 * `FederatedRAGMerger`; contains no business logic of its own.
 */
class FederationAdminHandler {
public:
    /**
     * @brief Construct with required dependencies.
     * @param coordinator  Active federation coordinator (must be non-null).
     * @param merger       Optional RAG merger for rag-stats endpoint.
     */
    explicit FederationAdminHandler(
        std::shared_ptr<distributed_knowledge::LoRAFederationCoordinator> coordinator,
        std::shared_ptr<distributed_knowledge::FederatedRAGMerger>        merger = nullptr);

    // ── /admin/federation/stats ───────────────────────────────────────────

    /**
     * @brief Return federation statistics as JSON.
     *
     * Includes `current_round`, `pending_gradients`, `privacy_budget_remaining`,
     * `dp_epsilon_total`, and all fields from `coordinator.getStats()`.
     *
     * @return JSON object suitable for HTTP response body.
     */
    [[nodiscard]] nlohmann::json getStats() const;

    // ── /admin/federation/rag-stats ──────────────────────────────────────

    /**
     * @brief Return RAG merge statistics as JSON.
     *
     * Returns merge stats from `FederatedRAGMerger::getStats()` when a
     * merger is available, or `{"available": false}` otherwise.
     *
     * @return JSON object suitable for HTTP response body.
     */
    [[nodiscard]] nlohmann::json getRagStats() const;

    // ── POST /admin/federation/trigger ───────────────────────────────────

    /**
     * @brief Manually trigger a federation round and return the result.
     *
     * @param algorithm  Optional aggregation algorithm override (e.g. "FedAvg").
     *                   Ignored if empty — config default is used.
     * @return JSON object: `{"round", "participants", "delta_version",
     *                        "epsilon_spent", "status": "success"}`.
     *
     * @throws std::runtime_error("DP budget exhausted")
     *         when `verifyPrivacyBudget()` is false.
     * @throws std::runtime_error("Cross-border transfer blocked: ...")
     *         when GDPR policy blocks a participant's region.
     * @throws std::runtime_error("Insufficient participants: ...")
     *         when fewer than `min_participants` gradients were submitted.
     */
    nlohmann::json triggerRound(const std::string& algorithm = "");

private:
    std::shared_ptr<distributed_knowledge::LoRAFederationCoordinator> coordinator_;
    std::shared_ptr<distributed_knowledge::FederatedRAGMerger>        merger_;
};

} // namespace themis::api
