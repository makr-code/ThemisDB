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

namespace themis::llm {

/**
 * @brief Default LLM client implementation.
 *
 * Routes generation requests through LLMPluginManager::instance() when a
 * plugin is available.  Degrades gracefully to a deterministic keyword-based
 * AQL mock when no plugin is registered, enabling offline unit tests without
 * requiring a live model.
 *
 * ### Thread safety
 * All public methods are thread-safe.  Plugin-path safety is delegated to
 * LLMPluginManager (which guards its plugin map with a mutex).
 *
 * ### Production delta (fallback path)
 * The keyword-based AQL fallback does NOT perform real inference.  It is
 * only active when `LLMPluginManager::instance().getDefaultPlugin() == nullptr`.
 * Any deployment that expects real inference results MUST register a plugin
 * before calling generate().
 */
class DefaultLLMClient : public LLMClient {
public:
    DefaultLLMClient() : ready_(true) {
        spdlog::debug("DefaultLLMClient initialized");
    }

    ~DefaultLLMClient() override = default;

    GenerationResult generate(
        const std::string& prompt,
        const GenerationOptions& options) override
    {
        GenerationResult result;

        if (!ready_) {
            result.success = false;
            result.error_message = "Client not ready";
            spdlog::warn("LLM generation failed: client not ready");
            return result;
        }

        // ── Production path: delegate to registered plugin via LLMPluginManager ──
        ILLMPlugin* plugin = LLMPluginManager::instance().getDefaultPlugin();
        if (plugin != nullptr) {
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

        // ── Fallback path: no plugin registered ─────────────────────────────────
        // Deterministic keyword-based AQL mock.  Active only when no plugin is
        // available (e.g., unit tests, offline development).  Not suitable for
        // production inference.
        spdlog::debug("DefaultLLMClient: no plugin registered, using keyword-based fallback");

        std::string mock_aql = "SELECT * FROM data WHERE 1=1";

        if (prompt.find("user") != std::string::npos) {
            mock_aql = "SELECT id, name FROM users WHERE status = 'active'";
        } else if (prompt.find("order") != std::string::npos) {
            mock_aql = "SELECT * FROM orders WHERE created_date >= NOW() - '30d'";
        } else if (prompt.find("product") != std::string::npos) {
            mock_aql = "SELECT * FROM products WHERE category = 'electronics'";
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
                      nl_query, schema_context.size());
        
        // Construct prompt from NL query + schema context
        std::string prompt = "Generate a ThemisDB AQL query for: " + nl_query;
        if (!schema_context.empty()) {
            prompt += "\n\nSchema:\n" + schema_context;
        }
        
        return generate(prompt, options);
    }
    
    size_t estimateTokens(cons[[maybe_unused]] t st[[maybe_unused]] d::string& [[maybe_unused]] text) const override {
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

// Factory function for creating default LLM client
std::shared_ptr<themis::llm::LLMClient> createDefaultLLMClient() {
    return std::make_shared<themis::llm::DefaultLLMClient>();
}
