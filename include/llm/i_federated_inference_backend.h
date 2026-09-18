/**
 * @file i_federated_inference_backend.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 *
 * @note **Plugin Interface**: Defines abstract interface for federated LLM inference backends.
 *       No .cpp implementation needed. Implementations provided by plugin system.
 *       See llm_plugin_manager.cpp for plugin lifecycle management.
 */


#pragma once

#include "llm/llm_plugin_interface.h"
#include <string>
#include <vector>

namespace themis::llm {

struct FanOutInstanceResult {
    /**
     * @brief Fan Out Instance Result.
     * @return Return value.
     */
    virtual ~FanOutInstanceResult() = default;
    std::string instance_id;     ///< Instance that was targeted
    InferenceResponse response;  ///< Populated when success == true
    bool success = false;        ///< True when the instance returned a valid response
    std::string error_code;      ///< Stable machine-readable failure class on error
    std::string error;           ///< Non-empty on failure (network error, timeout, …)
    int attempts = 0;            ///< How many attempts were made (≥ 1)
    int64_t dispatch_time_ms = 0;///< Wall-clock dispatch time for this instance
};

class IFederatedInferenceBackend {
public:
    /**
     * @brief IFederated Inference Backend.
     * @return Return value.
     */
    virtual ~IFederatedInferenceBackend() = default;

    /**
     * @brief Execute.
     * @param[in] instance_ids Input parameter.
     * @param[in] request Input parameter.
     * @return Return value.
     */
    virtual std::vector<FanOutInstanceResult> execute(
        const std::vector<std::string>& instance_ids,
        const InferenceRequest&         request) = 0;
};

} // namespace themis::llm
