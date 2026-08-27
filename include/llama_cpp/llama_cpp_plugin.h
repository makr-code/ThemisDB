/**
 * @file llama_cpp_plugin.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=16; TODO=1, Stub=14, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

    LlamaCppPlugin(LlamaCppPlugin&&) noexcept = default;
    LlamaCppPlugin& operator=(LlamaCppPlugin&&) noexcept = default;

    // ── ILLMPlugin ────────────────────────────────────────────────────────
    bool loadModel(const std::string& model_path, const json& config) override;
    void unloadModel() override;
    std::optional<llm::ModelInfo> getModelInfo() const override;
    bool isModelLoaded() const override { return model_loaded_; }

    /// @copydoc llm::ILLMPlugin::loadLoRA
    bool loadLoRA(const std::string& lora_id, const std::string& lora_path,
                  float scale) override;
    /// @copydoc llm::ILLMPlugin::unloadLoRA
    bool unloadLoRA(const std::string& lora_id) override;
    /// @copydoc llm::ILLMPlugin::listLoRAs
    std::vector<llm::LoRAInfo> listLoRAs() const override;

    llm::InferenceResponse generate(const llm::InferenceRequest& request) override;
    llm::InferenceResponse generateRAG(const llm::RAGContext& rag_context,
                                        const llm::InferenceRequest& request) override;
    std::vector<float> embed(const std::string& text) override;
    
    /**
     * @brief Generate K draft tokens with per-token logit distributions.
     *
     * Phase 2 Implementation: Real draft-logit pipeline using llama_get_logits()
     * from the underlying llama.cpp context. Returns actual logit distributions
     * for speculative decoding verification.
     *
     * When @p k is 0 the function returns an empty result immediately without
     * acquiring any significant resources.  @p vocab_size_hint values exceeding
     * 65 536 are capped to bound memory allocation in stub/fallback mode.
     *
     * @param request        Inference request (prompt + generation parameters).
     * @param k              Number of draft tokens to produce (0 is valid).
     * @param vocab_size_hint Expected vocabulary size; 32000 used as fallback
     *                       and capped in stub/fallback mode to bound memory usage.
     * @return DraftTokensResult with k tokens and k logit rows (empty when k==0).
     */
    llm::ILLMPlugin::DraftTokensResult generateDraftTokens(
        const llm::InferenceRequest& request,
        size_t                       k,
        size_t                       vocab_size_hint) override;

    llm::LLMCapabilities getCapabilities() const override;
    json getMemoryStats() const override;
    json getPerformanceStats() const override;

    std::vector<uint8_t> exportLoRA(const std::string& lora_id) override;
    bool importLoRA(const std::string& lora_id,
                    const std::vector<uint8_t>& data) override;

    // ── Extended API (beyond ILLMPlugin) ──────────────────────────────────

    /**
     * @brief Inject a custom embedding backend for stub/test environments.
     *
     * When set, `embed()` delegates to @p fn instead of returning the
     * 384-dimensional zero-vector stub.  Enables semantic-search tests and
     * RAG pipelines to operate without a compiled llama.cpp model.
     *
     * @param fn  Callable `std::vector<float>(const std::string& text)`.
     *            Must not throw; returning an empty vector causes `embed()`
     *            to fall back to the zero-vector stub.
     *
     * Stub #200 injection API — resolves the zero-vector fallback when
     * `wrapper_` is nullptr.
     */
    using EmbedFn = std::function<std::vector<float>(const std::string& text)>;
    void setEmbedFn(EmbedFn fn);

    /**
     * @brief Inject a custom generation backend for stub/test environments.
     *
     * When set, `generate()` delegates to @p fn before using the built-in
     * fail-closed stub path. This allows non-llama.cpp builds to forward text
     * generation into an external backend while preserving the existing error
     * response when no callback is configured.
     */
    using GenerateFn = std::function<llm::InferenceResponse(const llm::InferenceRequest&)>;
    void setGenerateFn(GenerateFn fn);

    /**
     * @brief Set an optional inference policy gate.
     *
     * When set, generate() and generateRAG() invoke this function before
     * dispatching to the inference backend.  The gate receives the request and
     * an output parameter for a human-readable denial reason.  Returning
     * @c false causes the call to return success=false with the denial reason
     * as error_message.  Returning @c true allows inference to proceed.
     *
     * This is the recommended integration point for governance policy engines
     * (e.g. PolicyEngine::checkInferencePermission()) without introducing a
     * compile-time dependency on the governance library.
     *
     * Thread-safe: stored under mutex_.
     *
     * @param fn  Callable matching PolicyFn; nullptr disables the gate (default).
     */
    using PolicyFn = std::function<bool(const llm::InferenceRequest&, std::string& denial_reason)>;
    void setPolicyFn(PolicyFn fn);

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

    std::atomic<uint64_t> inference_count_{0};
    std::atomic<uint64_t> error_count_{0};
    /// Streaming retry counter — incremented by Sub-Agent D retry logic.
    std::atomic<uint64_t> stream_retry_count_{0};

    struct LoRAEntry {
        std::string id;
        std::string path;
        float       scale = 1.0f;
    };
    std::vector<LoRAEntry> loras_;

/// Injected embedding backend (Stub #200 injection API).
    EmbedFn embed_fn_;
    GenerateFn generate_fn_;
    /// Optional inference policy gate (nullptr = no check).
    PolicyFn policy_fn_;

    /// @brief Compute a hex digest of the file at @p path.
    /// Uses FNV-64 as a CI-safe placeholder; swap for SHA-256 (OpenSSL EVP)
    /// in production deployments where libcrypto is available.
    /// Returns empty string on I/O error.
    static std::string computeFileDigest(const std::string& path);

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
