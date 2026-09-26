/**
 * @file llm_client_default.cpp
 * @brief Default LLM client — delegates to LLMPluginManager when a plugin is
 *        registered; falls back to a deterministic keyword-based mock when no
 *        plugin is available (useful for unit tests and offline development).
 *
 * Production path: LLMPluginManager::instance().generate() is called with an
 * InferenceRequest built from the GenerationOptions parameters.  The result is
 * mapped back to GenerationResult.
 *
 * Fallback path: active only when LLMPluginManager reports no default plugin.
 * Returns a predictable AQL string derived from keyword matching in the prompt.
 * No random latency is simulated so test output is deterministic.
 *
 * @author ThemisDB Team
 * @date 2026-06-18
 */

#include "llm/llm_client.h"
#include "llm/llm_plugin_manager.h"
#include "llm/llm_plugin_interface.h"
#include <spdlog/spdlog.h>
#include <array>
#include <filesystem>

namespace themis::llm {

namespace {

bool tryBootstrapDefaultLLMBackend() {
    auto& manager = LLMPluginManager::instance();
    if (manager.getDefaultPlugin() != nullptr) {
        return true;
    }

    // Pre-register the backend when LLM support is compiled in and a model path
    // is configured. This avoids the default keyword fallback masking a valid
    // runtime backend that has not been registered yet.
#if defined(THEMIS_ENABLE_LLM)
    if (themis::llm::createLlamaWrapper("llamacpp", "", nlohmann::json::object())) {
        if (manager.getDefaultPlugin() != nullptr) {
            return true;
        }
    }

    const std::array<const char*, 2> model_envs = {
        "THEMIS_DEMO_LLM_MODEL_PATH",
        "THEMIS_LLM_DEFAULT_MODEL_PATH"
    };

    for (const char* env_name : model_envs) {
        const char* env_value = std::getenv(env_name);
        if (!env_value || env_value[0] == '\0') {
            continue;
        }

        const std::filesystem::path model_path(env_value);
        if (!std::filesystem::exists(model_path)) {
            spdlog::warn("DefaultLLMClient: {} points to a missing model path '{}'",
                         env_name, model_path.string());
            continue;
        }

        if (themis::llm::createLlamaWrapper("llamacpp", "", nlohmann::json::object()) &&
            manager.loadModel("default", model_path.string())) {
            return true;
        }
    }
#endif

    return manager.getDefaultPlugin() != nullptr;
}

} // namespace

class DefaultLLMClient : public LLMClient {
public:
    DefaultLLMClient() : ready_(true) {
        spdlog::debug("DefaultLLMClient initialized (real plugin bootstrap attempted before keyword fallback)");
    }

    ~DefaultLLMClient() override = default;

    GenerationResult generate(
        const std::string& prompt,
        const GenerationOptions& options) override
    {
        GenerationResult result = {};

        if (!ready_) {
            result.success = false;
            result.error_message = "Client not ready";
            spdlog::warn("LLM generation failed: client not ready");
            return result;
        }

        // ── Production path: delegate to registered plugin via LLMPluginManager ──
        if (tryBootstrapDefaultLLMBackend()) {
            ILLMPlugin* plugin = LLMPluginManager::instance().getDefaultPlugin();
            if (plugin != nullptr && plugin->isModelLoaded()) {
                InferenceRequest req;
                req.prompt      = prompt;
                req.model_id    = "default";
                req.max_tokens  = options.max_tokens;
                req.temperature = options.temperature;

            try {
                InferenceResponse resp = LLMPluginManager::instance().generate(req);

                result.success           = resp.success;
                result.text              = resp.text;
                result.error_message     = resp.error_message;
                result.prompt_tokens     = estimateTokens(prompt);
                result.completion_tokens = estimateTokens(resp.text);
                result.finish_reason     = resp.success ? "stop" : "error";

                    spdlog::debug("DefaultLLMClient: plugin generate() ok, {} completion tokens",
                                  result.completion_tokens);
                    return result;
                } catch (const std::exception& e) {
                    spdlog::error("DefaultLLMClient: plugin generate() threw: {}", e.what());
                    result.success       = false;
                    result.error_message = e.what();
                    result.finish_reason = "error";
                    return result;
                }
            }
        }

        // ── Fallback path: no plugin registered ─────────────────────────────────
        // Deterministic keyword-based AQL mock.  Active only when no plugin is
        // available (e.g., unit tests, offline development).  Not suitable for
        // production inference.
        spdlog::debug("DefaultLLMClient: no plugin registered, using keyword-based fallback");

        std::string mock_aql = "FOR doc IN data RETURN doc";

        if (prompt.find("user") != std::string::npos) {
            mock_aql = "FOR user IN users RETURN user";
        } else if (prompt.find("order") != std::string::npos) {
            mock_aql = "FOR order IN orders RETURN order";
        } else if (prompt.find("product") != std::string::npos) {
            mock_aql = "FOR product IN products RETURN product";
        }

        result.success           = true;
        result.text              = mock_aql;
        result.prompt_tokens     = estimateTokens(prompt);
        result.completion_tokens = estimateTokens(mock_aql);
        result.finish_reason     = "stop";

        spdlog::debug("DefaultLLMClient: fallback AQL: {} completion tokens",
                      result.completion_tokens);
        return result;
    }
    
    GenerationResult generateAQL(
        const std::string& nl_query,
        const std::string& schema_context,
        const GenerationOptions& options) override
    {
        spdlog::debug("LLM generating AQL for NL: '{}' (schema_context: {} chars)",
                      nl_query,schema_context.size());
        
        // Construct prompt from NL query + schema context
        std::string prompt = "Generate a ThemisDB AQL query for: " + nl_query;
        if (!schema_context.empty()) {
            prompt += "\n\nSchema:\n" + schema_context;
        }
        
        return generate(prompt, options);
    }
    
    size_t estimateTokens(const std::string& text) const override {
        // Rough heuristic: ~4 characters per token
        return (text.size() + 3) / 4;
    }
    
    std::string getProviderName() const override {
        // Report "llm_plugin_manager" when a plugin is wired, "keyword_fallback"
        // when running in offline / no-plugin mode.
        ILLMPlugin* plugin = LLMPluginManager::instance().getDefaultPlugin();
        return (plugin != nullptr) ? "llm_plugin_manager" : "keyword_fallback";
    }
    
    bool isReady() const override {
        return ready_;
    }

private:
    bool ready_;
};

} // namespace themis::llm

/**
 * @brief Factory function for creating the default LLM client.
 * @return A shared pointer to the default LLMClient implementation.
 * @details This is the canonical factory symbol used by LLM wiring code.
 */
namespace themis::llm {
std::shared_ptr<LLMClient> createDefaultLLMClient() {
    return std::make_shared<DefaultLLMClient>();
}

// Compatibility alias for legacy callers that still reference the older
// snake_case helper from the aql integration layer.
std::shared_ptr<LLMClient> create_default_llm_client() {
    return createDefaultLLMClient();
}
} // namespace themis::llm
