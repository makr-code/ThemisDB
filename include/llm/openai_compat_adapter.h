#pragma once

/**
 * @file openai_compat_adapter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "llm/llm_plugin_interface.h"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <variant>

namespace themis {
namespace llm {

using json = nlohmann::json;

class OpenAICompatAdapter {
public:
    static std::variant<InferenceRequest, std::string> parseRequest(
        const json& body);

    static json buildResponse(
        const InferenceResponse& response,
        const std::string& model_id,
        const std::string& completion_id = "");

    static std::string buildStreamChunk(
        const std::string& token,
        const std::string& completion_id,
        const std::string& model_id,
        int64_t created = 0);

    static std::string buildStreamFinalChunk(
        const std::string& completion_id,
        const std::string& model_id,
        int64_t created = 0);

    /**
     * @brief Build Stream Done.
     * @return Return value.
     */
    static std::string buildStreamDone();

    static json buildError(
        const std::string& message,
        const std::string& type = "invalid_request_error",
        const std::string& code = "");

    /**
     * @brief Generate Completion Id.
     * @return Return value.
     */
    static std::string generateCompletionId();

private:
    /**
     * @brief Extract Prompts.
     * @param[in] messages Input parameter.
     * @param[in,out] system_prompt Input/output parameter.
     * @param[in,out] prompt Input/output parameter.
     * @return Return value.
     */
    static std::string extractPrompts(
        const json& messages,
        std::optional<std::string>& system_prompt,
        std::string& prompt);
};

} // namespace llm
} // namespace themis

