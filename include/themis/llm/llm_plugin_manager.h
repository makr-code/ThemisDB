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
    virtual ~ILLMPluginManager() = default;
    virtual InferenceResponse generate(const InferenceRequest& req) = 0;
    virtual std::string pluginVersion() const = 0;
};

} // namespace llm
} // namespace themis
