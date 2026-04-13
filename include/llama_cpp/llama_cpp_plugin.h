/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            llama_cpp_plugin.h                                 ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-04-13 04:16:22                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   98.0/100                                       ║
    • Total Lines:     134                                            ║
    • Open Issues:     TODOs: 0, Stubs: 3                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • df59ab8148  2026-04-12  feat(llm): promote llama_wrapper, multi_lora_manager, pro... ║
    • f0f3ecebde  2026-04-11  feat(llama_cpp): v2.1.0 — streaming, batch inference, Plu... ║
    • 7b80a66e02  2026-04-07  fix(llama_cpp): align LlamaCppPlugin with ILLMPlugin inte... ║
    • 01a86c4f10  2026-04-07  Changes before error encountered        ║
    • 1e348484ec  2026-04-07  feat(plugins): add stable_diffusion + llama_cpp plugins, ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "llm/llm_plugin_interface.h"
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <functional>
#include <nlohmann/json.hpp>

#ifdef THEMIS_LLM_ENABLED
#include "llm/llama_wrapper.h"
#endif

namespace themis {
namespace llamacpp {

using json = nlohmann::json;

/**
 * @brief Standalone llama_cpp plugin that wraps the existing LlamaWrapper
 *        and exposes it through the ILLMPlugin interface for dynamic loading.
 *
 * This plugin bridges the existing LlamaWrapper implementation with the
 * plugin system. It can be loaded dynamically via THEMIS_LLM_PLUGIN() and
 * registered in the PluginManager.
 *
 * Usage without model (test/stub mode):
 *   - initialize() returns true without loading any model
 *   - generate() returns an empty InferenceResponse
 *
 * Usage with model:
 *   - initialize() calls loadModel() with model_path from config
 *
 * Streaming:
 *   - Set InferenceRequest::stream_callback to receive tokens one-by-one.
 *   - Alternatively call generateStream(request, callback) directly.
 *
 * Batch inference:
 *   - generateBatch(requests) iterates over each request and calls generate().
 */
class LlamaCppPlugin : public llm::ILLMPlugin {
public:
    LlamaCppPlugin();
    ~LlamaCppPlugin() override;

    // ── ILLMPlugin ────────────────────────────────────────────────────────
    bool loadModel(const std::string& model_path, const json& config) override;
    void unloadModel() override;
    std::optional<llm::ModelInfo> getModelInfo() const override;
    bool isModelLoaded() const override { return model_loaded_; }

    bool loadLoRA(const std::string& lora_path, const std::string& lora_id,
                  float scale) override;
    bool unloadLoRA(const std::string& lora_id) override;
    std::vector<llm::LoRAInfo> listLoRAs() const override;

    llm::InferenceResponse generate(const llm::InferenceRequest& request) override;
    llm::InferenceResponse generateRAG(const llm::RAGContext& rag_context,
                                        const llm::InferenceRequest& request) override;
    std::vector<float> embed(const std::string& text) override;

    llm::LLMCapabilities getCapabilities() const override;
    json getMemoryStats() const override;
    json getPerformanceStats() const override;

    std::vector<uint8_t> exportLoRA(const std::string& lora_id) override;
    bool importLoRA(const std::string& lora_id,
                    const std::vector<uint8_t>& data) override;

    // ── Extended API (beyond ILLMPlugin) ──────────────────────────────────

    /**
     * @brief Streaming generation convenience wrapper.
     *
     * Calls generate() with the given request, injecting @p token_callback into
     * InferenceRequest::stream_callback.  When a real llama.cpp model is wired in,
     * each sampled token is forwarded to the callback before the full response is
     * returned.  In stub mode the callback receives the stub response as a single
     * "token" so that callers always get at least one callback invocation.
     *
     * @param request       Inference parameters (stream_callback field is overwritten).
     * @param token_callback Called once per token; must not throw.
     * @return Full InferenceResponse (same as generate()).
     */
    llm::InferenceResponse generateStream(
        llm::InferenceRequest request,
        std::function<void(const std::string& token)> token_callback);

    /**
     * @brief Batch inference over multiple requests.
     *
     * Iterates over @p requests and calls generate() for each one.
     * Order of responses mirrors order of requests.
     *
     * @param requests Vector of inference requests.
     * @return Vector of InferenceResponse, same size as @p requests.
     */
    std::vector<llm::InferenceResponse> generateBatch(
        const std::vector<llm::InferenceRequest>& requests);

    std::string getPluginVersion() const { return "2.1.0"; }
    std::string getModelId() const;

private:
    bool        model_loaded_ = false;
    std::string model_path_;
    std::string model_id_;
    size_t      context_length_ = 4096; ///< Model context window (tokens); set by loadModel()
    mutable std::mutex mutex_;

    uint64_t inference_count_   = 0;
    uint64_t error_count_       = 0;

    struct LoRAEntry {
        std::string id;
        std::string path;
        float       scale = 1.0f;
    };
    std::vector<LoRAEntry> loras_;

#ifdef THEMIS_LLM_ENABLED
    /// Real llama.cpp inference backend, created when a non-empty model path is
    /// provided to loadModel().  Null in stub/CI mode (empty model path).
    std::unique_ptr<llm::LlamaWrapper> wrapper_;
#endif
};

} // namespace llamacpp
} // namespace themis

// Export macro for dynamic loading
THEMIS_LLM_PLUGIN();
