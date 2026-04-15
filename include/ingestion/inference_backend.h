/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            inference_backend.h                                ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-04-15 18:03:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     123                                            ║
    • Open Issues:     TODOs: 0, Stubs: 2                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • db7df90e31  2026-04-15  feat(ingestion): Google Benchmarks QJ01–QJ11 + SoC/OOP do... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>

namespace themis {
namespace ingestion {

// ============================================================================
// ITextGenerationBackend — pure abstraction for text generation
// ============================================================================

/**
 * @brief Abstract interface for text generation used by the ingestion pipeline.
 *
 * ## Motivation (SoC / DIP)
 *
 * The ingestion module must not depend on any concrete AI / LLM backend.
 * Instead, it depends only on this lightweight interface.  The binding to
 * a real LLM (e.g. llama.cpp via `LLMPluginManager`) is performed in the
 * `llm` module through `LlmIngestionBridge` and injected at construction
 * time — the ingestion engine never sees a single `llm/` header.
 *
 * ## Dependency graph (after refactoring)
 * @code
 *   ingestion  →  ITextGenerationBackend  (this file)
 *                         ↑
 *   llm        →  LlmIngestionBridge : ITextGenerationBackend
 *                         ↑
 *   main / server  →  wires LlmIngestionBridge into LegalLlmAdapter
 * @endcode
 *
 * ## Usage
 * @code
 * // 1. In the llm module (llm_ingestion_bridge.h/.cpp):
 * class LlmIngestionBridge : public ingestion::ITextGenerationBackend { ... };
 *
 * // 2. In wiring code (main.cpp or server bootstrap):
 * auto bridge  = std::make_shared<LlmIngestionBridge>();
 * LegalLlmAdapter adapter(bridge);
 *
 * // 3. The ingestion pipeline uses LegalLlmAdapter without knowing about LLMs.
 * @endcode
 *
 * Thread-Safety: `generate()` MUST be thread-safe.
 */
class ITextGenerationBackend {
public:
    virtual ~ITextGenerationBackend() = default;

    /**
     * @brief Generate text from a prompt.
     *
     * @param prompt        The full prompt string (already assembled by caller).
     * @param max_tokens    Maximum number of tokens to generate.
     * @param temperature   Sampling temperature (lower = more deterministic).
     * @param lora_adapter  Optional LoRA adapter identifier / path.
     *                      Empty string means no adapter.
     * @return Generated text.  On unrecoverable error, implementations SHOULD
     *         return an empty string rather than throwing, so the ingestion
     *         pipeline can degrade gracefully.  Exceptions are caught and
     *         wrapped by `LegalLlmAdapter`.
     */
    virtual std::string generate(const std::string& prompt,
                                 int                max_tokens    = 512,
                                 double             temperature   = 0.1,
                                 const std::string& lora_adapter  = "") = 0;

    /**
     * @brief Check whether a real backend is available at this moment.
     *
     * Returns `true` when the underlying backend is configured and ready for
     * inference (e.g. model loaded, GPU memory available).
     * Returns `false` during warm-up or when the model has not been configured.
     */
    virtual bool isAvailable() const = 0;

    /**
     * @brief Human-readable description of the backend for logging.
     *
     * Example: "llama.cpp/mistral-7b-Q4_K_M + legal-lora-v2"
     */
    virtual std::string description() const = 0;
};

// ============================================================================
// NullTextGenerationBackend — always-unavailable fallback / stub
// ============================================================================

// STUB/SIMULATION NOTE:
// Purpose: Safe default when no real LLM plugin is configured; keeps all
//   dependants (LegalLlmAdapter, NerDeStep, LlmExtractStep, DeonticStep,
//   IngestionQualityJudge) compilable and runnable without a live LLM.
// Activation: Always instantiated as the default in LegalLlmAdapter,
//   NerDeStep, LlmExtractStep, DeonticStep, and IngestionManager when the
//   caller does not inject a concrete ITextGenerationBackend.
// Production Delta: isAvailable() returns false → all LLM-dependent paths
//   fall back to deterministic regex/rule-based extraction.
//   No actual text is generated; generate() returns an empty string.
// Removal Plan: Not removed — remains the compile-time / no-config default.
//   A real backend (e.g. LlmIngestionBridge) must be injected explicitly.

/**
 * @brief No-op backend that signals unavailability.
 *
 * Used as the default when no real LLM is configured.  `isAvailable()`
 * returns `false`, causing `LegalLlmAdapter::buildExtractorFn()` to return
 * an empty function so `DeonticExtractor` falls back to built-in regex.
 */
class NullTextGenerationBackend : public ITextGenerationBackend {
public:
    std::string generate(const std::string& /*prompt*/,
                         int   /*max_tokens*/,
                         double /*temperature*/,
                         const std::string& /*lora_adapter*/) override {
        return {};
    }

    bool        isAvailable() const override { return false; }
    std::string description() const override { return "NullTextGenerationBackend (no-op)"; }
};

} // namespace ingestion
} // namespace themis
