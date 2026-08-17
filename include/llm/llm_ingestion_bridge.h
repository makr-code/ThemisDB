/**
 * @file llm_ingestion_bridge.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.2
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "ingestion/inference_backend.h"
#include "llm/llm_plugin_interface.h"
#include "llm/llm_plugin_manager.h"
#include <string>
#include <memory>

namespace themis {
namespace llm {

// ============================================================================
// LlmIngestionBridge
// ============================================================================

/**
 * @brief Concrete `ITextGenerationBackend` that delegates to `LLMPluginManager`.
 *
 * This is the **only** class that bridges the ingestion module and the LLM
 * module.  It lives in `llm/` so the `ingestion/` module never needs to
 * include any `llm/` headers.
 *
 * ## Dependency flow (SoC / DIP compliant)
 * @code
 *   ingestion/  →  ITextGenerationBackend   (abstract, in ingestion/)
 *                         ↑ implemented by
 *   llm/        →  LlmIngestionBridge       (concrete, in llm/)
 *                         ↓ calls
 *               →  LLMPluginManager::instance().generate()
 * @endcode
 *
 * ## Wiring (main.cpp / server bootstrap)
 * @code
 * #include "llm/llm_ingestion_bridge.h"
 * #include "ingestion/llm_adapter.h"
 *
 * auto bridge  = std::make_shared<LlmIngestionBridge>();
 * // Optional: override the model/adapter via LlmAdapterConfig in LegalLlmAdapter.
 * ingestion::LegalLlmAdapter adapter(bridge);
 * adapter.setConfig({ .model_path = "/models/mistral-7b.Q4_K_M.gguf",
 *                     .adapter_path = "/adapters/legal-lora.gguf",
 *                     .temperature  = 0.1 });
 * @endcode
 *
 * Thread-Safety: `generate()` delegates to `LLMPluginManager::generate()`.
 * The `LLMPluginManager` is thread-safe (shared mutex + atomic queue depth).
 */
class LlmIngestionBridge : public ingestion::ITextGenerationBackend {
public:
    LlmIngestionBridge() = default;
    ~LlmIngestionBridge() override = default;

    // Non-copyable (singleton backend)
    LlmIngestionBridge(const LlmIngestionBridge&) = delete;
    LlmIngestionBridge& operator=(const LlmIngestionBridge&) = delete;
    LlmIngestionBridge(LlmIngestionBridge&&)            noexcept noexcept = default;
    LlmIngestionBridge& operator=(LlmIngestionBridge&&) noexcept noexcept = default;

    /**
     * @brief Generate text via `LLMPluginManager::instance().generate()`.
     *
     * Assembles an `InferenceRequest` from the supplied parameters and
     * returns the generated text from `InferenceResponse::text`.
     *
     * On any exception from the plugin manager, returns an empty string
     * so the ingestion pipeline degrades gracefully.
     *
     * @param prompt       Full prompt string.
     * @param max_tokens   Token budget.
     * @param temperature  Sampling temperature.
     * @param lora_adapter Optional LoRA adapter identifier / path.
     * @return Generated text, or empty string on error.
     */
    std::string generate(const std::string& prompt,
                         int                max_tokens   = 512,
                         double             temperature  = 0.1,
                         const std::string& lora_adapter = "") override;

    /**
     * @brief Returns true when `LLMPluginManager` has at least one plugin loaded.
     */
    bool isAvailable() const override;

    /**
     * @brief Returns a human-readable description of the active model.
     */
    std::string description() const override;
};

} // namespace llm
} // namespace themis
