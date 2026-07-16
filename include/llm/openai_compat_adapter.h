#pragma once

/**
 * @file openai_compat_adapter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Stateless adapter for the OpenAI Chat Completions API.
 *
 * All methods are static; no instantiation is required.
 */
class OpenAICompatAdapter {
public:
    /**
     * @brief Parse an OpenAI @c /v1/chat/completions request body.
     *
     * Converts the @c messages array and generation parameters into a
     * @c InferenceRequest.  The @c stream_callback field is left unset;
     * callers that need streaming must attach the callback themselves after
     * parsing.
     *
     * @param body Parsed JSON body of the HTTP POST request.
     * @return @c InferenceRequest on success, or an error string on failure.
     */
    static std::variant<InferenceRequest, std::string> parseRequest(
        const json& body);

    /**
     * @brief Build an OpenAI-compatible non-streaming response JSON.
     *
     * Produces a @c chat.completion object matching the OpenAI API schema:
     * @code
     * {
     *   "id":      "chatcmpl-<uuid>",
     *   "object":  "chat.completion",
     *   "created": <unix_timestamp>,
     *   "model":   "<model_id>",
     *   "choices": [{"index":0,"message":{"role":"assistant","content":"..."},
     *                "finish_reason":"stop"}],
     *   "usage":   {"prompt_tokens":N,"completion_tokens":M,"total_tokens":N+M}
     * }
     * @endcode
     *
     * @param response    Completed inference response from the engine.
     * @param model_id    Model identifier to embed in the response.
     * @param completion_id Pre-generated completion ID (e.g. "chatcmpl-xxx").
     *                     If empty a new one is generated automatically.
     * @return OpenAI-compatible JSON object.
     */
    static json buildResponse(
        const InferenceResponse& response,
        const std::string& model_id,
        const std::string& completion_id = "");

    /**
     * @brief Format a single streaming delta as an SSE data line.
     *
     * Produces one SSE chunk in the OpenAI streaming format:
     * @code
     *   data: {"id":"chatcmpl-xxx","object":"chat.completion.chunk",
     *           "created":<ts>,"model":"<model>",
     *           "choices":[{"index":0,"delta":{"content":"<token>"},
     *                       "finish_reason":null}]}\n\n
     * @endcode
     *
     * @param token         Token text to include in the delta.
     * @param completion_id Completion ID (e.g. "chatcmpl-xxx").
     * @param model_id      Model name embedded in each chunk.
     * @param created       Unix timestamp for the response; 0 uses current time.
     * @return SSE-formatted string ready to write to the HTTP response body.
     */
    static std::string buildStreamChunk(
        const std::string& token,
        const std::string& completion_id,
        const std::string& model_id,
        int64_t created = 0);

    /**
     * @brief Build the final streaming chunk with @c finish_reason="stop".
     *
     * Signals to the client that token generation is complete:
     * @code
     *   data: {"id":"chatcmpl-xxx","object":"chat.completion.chunk",
     *           "created":<ts>,"model":"<model>",
     *           "choices":[{"index":0,"delta":{},
     *                       "finish_reason":"stop"}]}\n\n
     * @endcode
     *
     * @param completion_id Completion ID.
     * @param model_id      Model name.
     * @param created       Unix timestamp; 0 uses current time.
     * @return SSE-formatted terminal chunk string.
     */
    static std::string buildStreamFinalChunk(
        const std::string& completion_id,
        const std::string& model_id,
        int64_t created = 0);

    /**
     * @brief Return the canonical OpenAI stream termination sentinel.
     *
     * @return The string @c "data: [DONE]\n\n".
     */
    static std::string buildStreamDone();

    /**
     * @brief Build an OpenAI-compatible error JSON body.
     *
     * @param message Human-readable error description.
     * @param type    OpenAI error type string (e.g. "invalid_request_error").
     * @param code    Optional short error code string.
     * @return Error JSON object.
     */
    static json buildError(
        const std::string& message,
        const std::string& type = "invalid_request_error",
        const std::string& code = "");

    /**
     * @brief Generate a unique completion ID of the form @c "chatcmpl-<uuid>".
     *
     * @return A new completion ID string.
     */
    static std::string generateCompletionId();

private:
    /**
     * @brief Extract system_prompt and conversation prompt from messages.
     *
     * The first @c "system" role message becomes @c InferenceRequest::system_prompt.
     * Remaining @c "user" and @c "assistant" messages are concatenated into
     * @c InferenceRequest::prompt using a simple role-prefixed format compatible
     * with the chat templates already used by @c InferenceEngineEnhanced.
     *
     * @param messages JSON array of message objects.
     * @param system_prompt Output for the extracted system prompt (may be empty).
     * @param prompt        Output for the conversation prompt.
     * @return Empty string on success, error description on failure.
     */
    static std::string extractPrompts(
        const json& messages,
        std::optional<std::string>& system_prompt,
        std::string& prompt);
};

} // namespace llm
} // namespace themis

