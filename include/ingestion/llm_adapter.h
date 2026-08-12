/**
 * @file llm_adapter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "ingestion/deontic_extractor.h"
#include "ingestion/inference_backend.h"
#include <string>
#include <memory>
#include <functional>

namespace themis {
namespace ingestion {

// ============================================================================
// LLM adapter configuration
// ============================================================================

/**
 * @brief Configuration for the LLM-based deontic extraction adapter.
 *
 * Controls which LLM model and (optional) LoRA adapter are used for semantic
 * extraction of deontic modalities from German legal texts.
 *
 * In Phase 1, the adapter falls back to regex-based extraction regardless of
 * these settings.  In Phase 2, when `THEMIS_ENABLE_LLM` is `ON` and a valid
 * `model_path` is supplied, the adapter will call the llama.cpp inference
 * engine through ThemisDB's existing LLM integration layer.
 */
struct LlmAdapterConfig {
    std::string model_path;     ///< Path to GGUF model file (e.g. mistral-7b.Q4_K_M.gguf)
    std::string adapter_path;   ///< Path to LoRA adapter weights (may be empty)
    double      temperature = 0.1;  ///< Sampling temperature (lower = more deterministic)
    int         context_size = 4096; ///< Context window size in tokens
    bool        use_gpu = false;    ///< Attempt GPU offloading via llama.cpp (requires CUDA/Metal)
    int         gpu_layers = 0;     ///< Number of layers to offload to GPU (0 = CPU only)

    LlmAdapterConfig() = default;
    LlmAdapterConfig(std::string model, std::string adapter = "",
                     double temp = 0.1)
        : model_path(std::move(model)), adapter_path(std::move(adapter)),
          temperature(temp) {}

    /// Returns true when a model path has been set.
    bool hasModel() const { return !model_path.empty(); }

    /// Returns true when a LoRA adapter path has been set.
    bool hasAdapter() const { return !adapter_path.empty(); }
};

// ============================================================================
// LegalLlmAdapter
// ============================================================================

/**
 * @brief LLM integration adapter for the legal ingestion pipeline.
 *
 * Bridges the `DeonticExtractor` (regex-based) with an injected
 * `ITextGenerationBackend`.  The adapter has **no knowledge** of any
 * concrete AI/LLM framework; it only calls the abstract backend interface.
 * This satisfies the Dependency-Inversion Principle (DIP) and keeps the
 * ingestion module free of any `llm/` includes.
 *
 * ## SoC contract
 * - `ingestion` module owns: `ITextGenerationBackend`, `LegalLlmAdapter`,
 *   `DeonticExtractor`, `LlmAdapterConfig`.
 * - `llm` module owns:  `LlmIngestionBridge` (a concrete backend that
 *   wraps `LLMPluginManager`).
 * - Wiring code (main / server bootstrap) creates an `LlmIngestionBridge`
 *   and injects it into `LegalLlmAdapter`.
 *
 * ## Fallback behaviour
 * When the injected backend returns `isAvailable() == false`, or when no
 * backend is provided (default: `NullTextGenerationBackend`), `buildExtractorFn()`
 * returns an empty function so `DeonticExtractor` keeps using its built-in
 * regex implementation.
 *
 * ## Usage
 * @code
 * // Regex fallback (no LLM):
 * LegalLlmAdapter adapter;  // uses NullTextGenerationBackend
 *
 * // With a real LLM backend (injected externally):
 * auto bridge = std::make_shared<LlmIngestionBridge>(...);
 * LegalLlmAdapter adapter(bridge);
 * adapter.setConfig({.model_path = "...", .temperature = 0.1});
 *
 * DeonticExtractor extractor = adapter.buildExtractor(0.75);
 * auto result = extractor.extract(text);
 * @endcode
 */
class LegalLlmAdapter {
public:
    /**
     * @brief Default constructor — uses `NullTextGenerationBackend`.
     *        `isLlmAvailable()` returns `false`; regex fallback is active.
     */
    LegalLlmAdapter();

    /**
     * @brief Construct with an externally owned backend (injection).
     *
     * @param backend  Shared pointer to any `ITextGenerationBackend`.
     *                 Must not be null; pass a `NullTextGenerationBackend`
     *                 explicitly when no LLM is desired.
     */
    explicit LegalLlmAdapter(std::shared_ptr<ITextGenerationBackend> backend);

    ~LegalLlmAdapter();

    // Non-copyable (owns inference context)
    LegalLlmAdapter(const LegalLlmAdapter&) = delete;
    LegalLlmAdapter& operator=(const LegalLlmAdapter&) = delete;
    LegalLlmAdapter(LegalLlmAdapter&&) noexcept;
    LegalLlmAdapter& operator=(LegalLlmAdapter&&) noexcept;

    /**
     * @brief Set the LLM configuration.
     *
     * Must be called before `buildExtractorFn()`.  Calling this after the
     * extractor function has been built does not affect already-created
     * extractor functions; call `buildExtractorFn()` again to pick up the
     * new configuration.
     *
     * @param config  LLM model and adapter configuration
     */
    void setConfig(const LlmAdapterConfig& config);

    /**
     * @brief Return the current LLM configuration.
     */
    const LlmAdapterConfig& getConfig() const;

    /**
     * @brief Check whether a real LLM backend is available.
     *
        * When a `model_path` is configured, this validates that the referenced
        * model file is accessible for reading. Without a configured model path,
        * the check delegates to `ITextGenerationBackend::isAvailable()` on the
        * injected backend.
     *
        * @return true if a configured model file is readable or the injected
        *         backend reports that it is ready for inference
     */
    bool isLlmAvailable() const;

    /**
     * @brief Build a `DeonticExtractor::ExtractorFn` for the configured model.
     *
     * Phase 1: returns an empty `ExtractorFn{}` when no LLM is available.
     * Callers should check `if (fn)` before passing to `setExtractorFn()`.
     *
     * Phase 2: returns a callable that sends the text to the llama.cpp
     * inference engine with a structured German legal extraction prompt and
     * parses the JSON response into a `DeonticExtraction`.
     *
    * Throws when a model path is explicitly configured but the file cannot
    * be opened. This keeps misconfigured deployments fail-closed instead of
    * silently degrading to regex extraction.
    *
     * The returned function captures the model configuration by value so it
     * remains valid independently of the `LegalLlmAdapter` instance lifetime.
     *
     * @return ExtractorFn (may be empty in Phase 1 / when no model is set)
    * @throws std::runtime_error if `config_.model_path` is set but the model
    *         file is not readable
     */
    DeonticExtractor::ExtractorFn buildExtractorFn() const;

    /**
     * @brief Build a `DeonticExtractor` pre-configured with this adapter.
     *
     * Convenience factory that creates a `DeonticExtractor` and injects
     * the LLM extractor function (when available) in one step.
     *
     * @param confidence_threshold Confidence threshold for the extractor
     * @return Configured DeonticExtractor
     */
    DeonticExtractor buildExtractor(double confidence_threshold = 0.75) const;

private:
    LlmAdapterConfig config_;
    std::shared_ptr<ITextGenerationBackend> backend_; ///< injected backend (never null)

    /**
     * @brief Build the extraction prompt for the given text.
     *
     * Returns a structured prompt suitable for instruction-tuned models.
     * The prompt is in German to leverage legal-domain knowledge.
     */
    static std::string buildPrompt(const std::string& text);

    /**
     * @brief Parse a JSON response from the LLM into a DeonticExtraction.
     *
     * Expected JSON format:
     * @code
     * {
     *   "deontic_category": "obligation",
     *   "confidence": 0.92,
     *   "entities": [
     *     {"type": "person_role", "value": "Betreiber"},
     *     {"type": "law_reference", "value": "§ 4 Abs. 1"}
     *   ],
     *   "obligations": [
     *     {"actor": "Betreiber", "action": "Genehmigung einholen", "condition": ""}
     *   ]
     * }
     * @endcode
     *
     * @param llm_response Raw LLM output string
     * @return Populated DeonticExtraction
     */
    static DeonticExtraction parseLlmResponse(const std::string& llm_response);
};

} // namespace ingestion
} // namespace themis
