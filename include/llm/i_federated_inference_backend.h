/**
 * @file i_federated_inference_backend.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=2, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 *
 * @note **Plugin Interface**: Defines abstract interface for federated LLM inference backends.
 *       No .cpp implementation needed. Implementations provided by plugin system.
 *       See llm_plugin_manager.cpp for plugin lifecycle management.
 */

/*
 * ThemisDB | File: i_federated_inference_backend.h | Version: 1.0.0 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 96/100 | Lines: 72
 * Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=2, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include "llm/llm_plugin_interface.h"
#include <string>
#include <vector>

namespace themis::llm {

/**
 * @brief Per-instance result from a fan-out inference dispatch.
 *
 * A `FanOutInstanceResult` captures whether the request succeeded on a
 * particular remote instance and, if not, which error occurred.  Callers
 * inspect the per-instance status to implement merge strategies (first-wins,
 * all-must-succeed, majority-vote, …) or aggregate partial results.
 */
struct FanOutInstanceResult {
    virtual ~FanOutInstanceResult() = default;
    std::string instance_id;     ///< Instance that was targeted
    InferenceResponse response;  ///< Populated when success == true
    bool success = false;        ///< True when the instance returned a valid response
    std::string error;           ///< Non-empty on failure (network error, timeout, …)
    int attempts = 0;            ///< How many attempts were made (≥ 1)
};

/**
 * @brief Injectable backend for federated (cross-instance) inference fan-out.
 *
 * `InferenceEngineEnhanced` checks for an attached `IFederatedInferenceBackend`
 * at request time.  When one is present **and** the request carries a
 * non-empty `target_instance_ids` list, execution is delegated here instead
 * of going through the local model pipeline.
 *
 * Implement this interface to route requests to remote ThemisDB shard
 * instances via mTLS/RemoteExecutor, mock in unit tests, or integrate a
 * custom dispatch mechanism.
 *
 * Thread-safety guarantee: `execute()` must be safe to call concurrently from
 * multiple worker threads.
 */
class IFederatedInferenceBackend {
public:
    virtual ~IFederatedInferenceBackend() = default;

    /**
     * @brief Fan out @p request to every instance in @p instance_ids.
     *
     * The implementation is responsible for:
     * - Dispatching the request to each instance (in parallel if desired).
     * - Performing bounded retries on transient failures.
     * - Returning exactly one `FanOutInstanceResult` per entry in
     *   @p instance_ids (order need not be preserved).
     *
     * @param instance_ids  Non-empty list of target instance / shard IDs.
     * @param request       Inference request payload (prompt, params, …).
     * @return Vector of per-instance results; size == `instance_ids.size()`.
     */
    virtual std::vector<FanOutInstanceResult> execute(
        const std::vector<std::string>& instance_ids,
        const InferenceRequest&         request) = 0;
};

} // namespace themis::llm
