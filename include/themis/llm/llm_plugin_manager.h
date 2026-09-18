// Lightweight interface for the LLM plugin manager used by callers that
// must not link the full themis_llm implementation.
#pragma once

#include <memory>
#include <string>
#include "themis/llm/llm_interfaces.h"

namespace themis {
namespace llm {

class ILLMPluginManager {
public:
    /**
     * @brief TBD: Describe ~ILLMPluginManager.
     * @return Return value.
     */
    virtual ~ILLMPluginManager() = default;
    /**
     * @brief TBD: Describe generate.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    virtual InferenceResponse generate(const InferenceRequest& req) = 0;
    /**
     * @brief TBD: Describe pluginVersion.
     * @return Return value.
     */
    virtual std::string pluginVersion() const = 0;
};

} // namespace llm
} // namespace themis
