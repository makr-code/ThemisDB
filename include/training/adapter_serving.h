/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            adapter_serving.h                                  ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-04-13 04:21:34                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     108                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ac63c2ec8d  2026-04-12  [WIP] Update developer documentation for module training ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

/**
 * @file adapter_serving.h
 * @brief Adapter serving integration for the training module.
 *
 * Defines the abstract interface between the training module and the LLM
 * inference layer so that trained LoRA adapters can be deployed and
 * traffic-split without a hard compile-time dependency on llm headers.
 *
 * Key types:
 *  - ILLMRouter          – pure-virtual interface the LLM layer implements
 *  - DeployResult        – result of a deploy or rollback operation
 */

#include <string>

namespace themis {
namespace training {

// ============================================================================
// ILLMRouter — abstract LLM router interface
// ============================================================================

/**
 * @brief Abstract interface for the LLM inference router.
 *
 * The training module calls this interface to propagate adapter version
 * weights to the live inference layer.  Production callers inject a
 * concrete implementation (e.g., backed by MultiLoRAManager); tests inject
 * a mock.
 *
 * Threading contract: implementations must be thread-safe; calls may arrive
 * from the deployment thread concurrently with ongoing inference requests.
 * The routing weight update must be atomic from the request handler's
 * perspective (no mid-request split change is observable).
 */
class ILLMRouter {
public:
    virtual ~ILLMRouter();

    /**
     * @brief Update the traffic weight for the named adapter version.
     *
     * Sets the fraction of incoming inference requests that should be
     * routed to @p version.  Implementations must normalise weights across
     * all registered versions so that the total sums to 1.0.
     *
     * @param version Adapter version identifier (e.g., "legal_v1.1").
     * @param weight  Desired traffic fraction in [0.0, 1.0].
     * @return true on success; false if the version is unknown to the router
     *         (the caller may retry after loading the adapter).
     */
    virtual bool setAdapterWeight(const std::string& version, float weight) = 0;

    /**
     * @brief Whether the router is reachable and ready to accept weight updates.
     */
    virtual bool isAvailable() const = 0;

    /**
     * @brief Return the version identifier that currently receives 100% of
     *        traffic, or an empty string if no version is fully active.
     */
    virtual std::string activeVersion() const = 0;
};

// ============================================================================
// DeployResult
// ============================================================================

/**
 * @brief Result of a deployVersion() or rollbackVersion() operation.
 *
 * Returned by IncrementalLoRATrainer::deployVersionEx() and
 * IncrementalLoRATrainer::rollbackVersionEx().
 */
struct DeployResult {
    bool        success        = false;  ///< Whether the operation succeeded
    std::string active_version;          ///< Version that now receives traffic
    float       split_applied  = 0.0f;  ///< Traffic fraction routed to active_version
    std::string error;                   ///< Non-empty on failure ("version_not_found", etc.)

    DeployResult() = default;

    /// Convenience: create a successful result.
    static DeployResult ok(const std::string& version, float split) {
        DeployResult r;
        r.success        = true;
        r.active_version = version;
        r.split_applied  = split;
        return r;
    }

    /// Convenience: create a failed result.
    static DeployResult fail(const std::string& reason) {
        DeployResult r;
        r.success = false;
        r.error   = reason;
        return r;
    }
};

} // namespace training
} // namespace themis
