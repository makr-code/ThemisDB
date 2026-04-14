/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            llm_adapter.h                                      ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-04-14 11:25:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     231                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2bb85b14f2  2026-03-11  feat(ingestion): add llm_adapter.h/cpp + fix README gaps ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "ingestion/deontic_extractor.h"
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
 * Bridges the `DeonticExtractor` (Phase 1: regex) with a real LLM inference
 * engine (Phase 2: Mistral 7B + LoRA via llama.cpp).
 *
 * The adapter exposes a `buildExtractorFn()` method that returns a
 * `DeonticExtractor::ExtractorFn` suitable for passing to
 * `DeonticExtractor::setExtractorFn()`.  This cleanly decouples the
 * extraction interface from the inference backend.
 *
 * **Phase 1 behaviour (current):**
 * - When `THEMIS_ENABLE_LLM` is `OFF` at compile time, or when no model path
 *   is configured, `buildExtractorFn()` returns an empty `ExtractorFn` so
 *   that `DeonticExtractor` keeps using its built-in regex implementation.
 * - `isLlmAvailable()` returns `false`.
 *
 * **Phase 2 behaviour (planned – requires `THEMIS_ENABLE_LLM=ON`):**
 * - When a valid `LlmAdapterConfig::model_path` is set, `buildExtractorFn()`
 *   returns a function that calls the llama.cpp inference engine.
 * - The prompt template instructs the model to classify the deontic category
 *   and extract entities from the supplied German legal text.
 * - The LoRA adapter fine-tuned on BImSchG / StGB / DSGVO is loaded when
 *   `LlmAdapterConfig::adapter_path` is non-empty.
 *
 * Usage (Phase 1 / testing):
 * @code
 * LegalLlmAdapter adapter;
 * adapter.setConfig({.model_path = ""});  // no model → regex fallback
 *
 * DeonticExtractor extractor;
 * auto fn = adapter.buildExtractorFn();
 * if (fn) {
 *     extractor.setExtractorFn(std::move(fn));
 * }
 * auto result = extractor.extract(text);
 * @endcode
 *
 * Usage (Phase 2 with LLM):
 * @code
 * LegalLlmAdapter adapter;
 * adapter.setConfig({
 *     .model_path   = "/models/mistral-7b.Q4_K_M.gguf",
 *     .adapter_path = "/adapters/legal-lora.gguf",
 *     .temperature  = 0.1
 * });
 *
 * DeonticExtractor extractor;
 * extractor.setExtractorFn(adapter.buildExtractorFn());
 * @endcode
 */
class LegalLlmAdapter {
public:
    LegalLlmAdapter();
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
     * @brief Check whether a real LLM is available for inference.
     *
     * Returns `true` when:
     *  - `THEMIS_ENABLE_LLM` is `ON` at compile time, AND
     *  - A non-empty `model_path` has been configured, AND
     *  - The model file can be found on the filesystem.
     *
     * Returns `false` in Phase 1 (always) and in Phase 2 when the model
     * file is missing.
     *
     * @return true if LLM inference is available
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
     * The returned function captures the model configuration by value so it
     * remains valid independently of the `LegalLlmAdapter` instance lifetime.
     *
     * @return ExtractorFn (may be empty in Phase 1 / when no model is set)
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
