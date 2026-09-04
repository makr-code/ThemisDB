/**
 * @file openai_compat_adapter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "llm/openai_compat_adapter.h"
#include <nlohmann/json.hpp>
#include <chrono>
#include <random>
#include <sstream>
#include <iomanip>
#include <variant>

namespace themis {
namespace llm {

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Return the current Unix timestamp as int64_t.
int64_t nowUnix() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

/// Generate a short hex random suffix for completion IDs.
std::string randomHex([[maybe_unused]] size_t bytes = 12) {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dist;

    std::ostringstream oss;
    while (bytes > 0) {
        uint64_t val = dist(gen);
        size_t chunk = std::min(bytes, size_t{8});
        for (size_t i = 0; i < chunk; ++i) {
            oss << std::hex << std::setw(2) << std::setfill('0')
                << ((val >> (i * 8)) & 0xFF);
        }
        bytes -= chunk;
    }
    return oss.str();
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

std::string OpenAICompatAdapter::generateCompletionId() {
    return "chatcmpl-" + randomHex(12);
}

// ─────────────────────────────────────────────────────────────────────────────

std::variant<InferenceRequest, std::string>
OpenAICompatAdapter::parseRequest(const json& body) {
    InferenceRequest req;

    // ── model ──────────────────────────────────────────────────────────────
    if (body.contains("model") && body["model"].is_string()) {
        req.model_id = body["model"].get<std::string>();
    }

    // ── messages (required) ────────────────────────────────────────────────
    if (!body.contains("messages") || !body["messages"].is_array()) {
        return std::string{"Missing or invalid 'messages' array"};
    }
    const json& messages = body["messages"];
    if (messages.empty()) {
        return std::string{"'messages' array must not be empty"};
    }

    std::optional<std::string> system_prompt;
    std::string prompt;
    std::string err = extractPrompts(messages, system_prompt, prompt);
    if (!err.empty()) {
        return err;
    }
    req.system_prompt = std::move(system_prompt);
    req.prompt        = std::move(prompt);

    // ── temperature ────────────────────────────────────────────────────────
    if (body.contains("temperature")) {
        const auto& t = body["temperature"];
        if (t.is_number()) {
            req.temperature = static_cast<float>(t.get<double>());
        }
    }

    // ── max_tokens ─────────────────────────────────────────────────────────
    if (body.contains("max_tokens")) {
        const auto& m = body["max_tokens"];
        if (m.is_number_integer()) {
            req.max_tokens = m.get<int>();
        }
    }
    // OpenAI spec also accepts max_completion_tokens (o1-series alias)
    if (body.contains("max_completion_tokens")) {
        const auto& m = body["max_completion_tokens"];
        if (m.is_number_integer()) {
            req.max_tokens = m.get<int>();
        }
    }

    // ── top_p ──────────────────────────────────────────────────────────────
    if (body.contains("top_p")) {
        const auto& v = body["top_p"];
        if (v.is_number()) {
            req.top_p = static_cast<float>(v.get<double>());
        }
    }

    // ── stop ───────────────────────────────────────────────────────────────
    if (body.contains("stop")) {
        const auto& s = body["stop"];
        if (s.is_string()) {
            req.stop_sequences.push_back(s.get<std::string>());
        } else if (s.is_array()) {
            for (const auto& item : s) {
                if (item.is_string()) {
                    req.stop_sequences.push_back(item.get<std::string>());
                }
            }
        }
    }

    // ── tools ──────────────────────────────────────────────────────────────
    // The OpenAI tools format:
    //   [{"type":"function","function":{"name":"...","description":"...","parameters":{...}}}]
    if (body.contains("tools") && body["tools"].is_array()) {
        for (const auto& tool_obj : body["tools"]) {
            if (!tool_obj.is_object()) {
              continue;
            }

            // Unwrap the optional "function" wrapper
            const json* fn = nullptr;
            if (tool_obj.contains("function") && tool_obj["function"].is_object()) {
                fn = &tool_obj["function"];
            } else {
                fn = &tool_obj;
            }

            ToolDefinition td;
            if (fn->contains("name") && (*fn)["name"].is_string()) {
                td.name = (*fn)["name"].get<std::string>();
            }
            if (fn->contains("description") && (*fn)["description"].is_string()) {
                td.description = (*fn)["description"].get<std::string>();
            }
            if (fn->contains("parameters") && (*fn)["parameters"].is_object()) {
                td.parameters = (*fn)["parameters"];
            }

            if (!td.name.empty()) {
                req.tools.push_back(std::move(td));
            }
        }
    }

    return req;
}

// ─────────────────────────────────────────────────────────────────────────────

json OpenAICompatAdapter::buildResponse(
    const InferenceResponse& response,
    const std::string& model_id,
    const std::string& completion_id) {

    const std::string id = completion_id.empty()
        ? generateCompletionId()
        : completion_id;

    const int64_t created = nowUnix();

    // Finish reason: if tool calls were generated use "tool_calls", else "stop"
    std::string finish_reason = "stop";

    // Build the assistant message
    json message;
    message["role"] = "assistant";

    if (!response.tool_calls.empty()) {
        finish_reason = "tool_calls";
        message["content"] = nullptr;  // OpenAI spec: null when tool_calls present

        json tool_calls_arr = json::array();
        for (size_t i = 0; i < response.tool_calls.size(); ++i) {
            const auto& tc = response.tool_calls[i];
            tool_calls_arr.push_back({
                {"id",   "call_" + randomHex(8)},
                {"type", "function"},
                {"function", {
                    {"name",      tc.name},
                    {"arguments", tc.arguments.dump()}
                }}
            });
        }
        message["tool_calls"] = std::move(tool_calls_arr);
    } else {
        message["content"] = response.text;
    }

    json choice;
    choice["index"]         = 0;
    choice["message"]       = std::move(message);
    choice["finish_reason"] = finish_reason;
    choice["logprobs"]      = nullptr;

    const int prompt_tokens     = response.tokens_prompt;
    const int completion_tokens = response.tokens_generated;

    return json{
        {"id",      id},
        {"object",  "chat.completion"},
        {"created", created},
        {"model",   model_id.empty() ? response.model_id : model_id},
        {"choices", json::array({std::move(choice)})},
        {"usage",   json{
            {"prompt_tokens",     prompt_tokens},
            {"completion_tokens", completion_tokens},
            {"total_tokens",      prompt_tokens + completion_tokens}
        }},
        {"system_fingerprint", nullptr}
    };
}

// ─────────────────────────────────────────────────────────────────────────────

std::string OpenAICompatAdapter::buildStreamChunk(
    const std::string& token,
    const std::string& completion_id,
    const std::string& model_id,
    int64_t created) {

    if (created == 0) {
      created = nowUnix();
    }

    json chunk{
        {"id",      completion_id},
        {"object",  "chat.completion.chunk"},
        {"created", created},
        {"model",   model_id},
        {"choices", json::array({
            json{
                {"index",         0},
                {"delta",         json{{"content", token}}},
                {"finish_reason", nullptr},
                {"logprobs",      nullptr}
            }
        })}
    };
    return "data: " + chunk.dump() + "\n\n";
}

// ─────────────────────────────────────────────────────────────────────────────

std::string OpenAICompatAdapter::buildStreamFinalChunk(
    const std::string& completion_id,
    const std::string& model_id,
    int64_t created) {

    if (created == 0) {
      created = nowUnix();
    }

    json chunk{
        {"id",      completion_id},
        {"object",  "chat.completion.chunk"},
        {"created", created},
        {"model",   model_id},
        {"choices", json::array({
            json{
                {"index",         0},
                {"delta",         json::object()},
                {"finish_reason", "stop"},
                {"logprobs",      nullptr}
            }
        })}
    };
    return "data: " + chunk.dump() + "\n\n";
}

// ─────────────────────────────────────────────────────────────────────────────

std::string OpenAICompatAdapter::buildStreamDone() {
    return "data: [DONE]\n\n";
}

// ─────────────────────────────────────────────────────────────────────────────

json OpenAICompatAdapter::buildError(
    const std::string& message,
    const std::string& type,
    const std::string& code) {

    json err_obj;
    err_obj["message"] = message;
    err_obj["type"]    = type;
    err_obj["param"]   = nullptr;
    if (!code.empty()) {
        err_obj["code"] = code;
    } else {
        err_obj["code"] = nullptr;
    }
    return json{{"error", std::move(err_obj)}};
}

// ─────────────────────────────────────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────────────────────────────────────

std::string OpenAICompatAdapter::extractPrompts(
    const json& messages,
    std::optional<std::string>& system_prompt,
    std::string& prompt) {

    // The first "system" role becomes system_prompt; all subsequent messages
    // are formatted into the prompt using a simple Human/Assistant convention
    // that works across the chat templates already used by the engine.
    std::ostringstream conv;
    bool first_non_system = true;

    for (const auto& msg : messages) {
        if (!msg.is_object()) {
            return "Each element of 'messages' must be an object";
        }
        if (!msg.contains("role") || !msg["role"].is_string()) {
            return "Each message must have a string 'role' field";
        }
        if (!msg.contains("content")) {
            return "Each message must have a 'content' field";
        }

        const std::string role = msg["role"].get<std::string>();

        // Content can be a string or an array of content parts (vision etc.)
        std::string content_text;
        const auto& content = msg["content"];
        if (content.is_string()) {
            content_text = content.get<std::string>();
        } else if (content.is_array()) {
            // Concatenate text parts only
            for (const auto& part : content) {
                if (part.is_object() &&
                    part.contains("type") && part["type"] == "text" &&
                    part.contains("text") && part["text"].is_string()) {
                    content_text += part["text"].get<std::string>();
                }
            }
        } else if (content.is_null()) {
            // tool result messages may have null content
            content_text = "";
        } else {
            return "Message 'content' must be a string, array, or null";
        }

        if (role == "system") {
            if (!system_prompt.has_value()) {
                system_prompt = content_text;
            } else {
                // Additional system messages are appended to the first
                *system_prompt += "\n" + content_text;
            }
        } else if (role == "user" || role == "human") {
            if (!first_non_system) {
                conv << "\n";
            }
            conv << "User: " << content_text;
            first_non_system = false;
        } else if (role == "assistant") {
            if (!first_non_system) {
                conv << "\n";
            }
            conv << "Assistant: " << content_text;
            first_non_system = false;
        } else if (role == "tool") {
            // Function call result — include as assistant context
            if (!first_non_system) {
                conv << "\n";
            }
            conv << "Tool result: " << content_text;
            first_non_system = false;
        }
        // Unknown roles are silently skipped to allow forward-compatibility
    }

    prompt = conv.str();

    // Guard: if all messages were system messages, prompt would be empty
    if (prompt.empty() && !system_prompt.has_value()) {
        return "No actionable messages found in 'messages' array";
    }

    return {};  // success
}

} // namespace llm
} // namespace themis

