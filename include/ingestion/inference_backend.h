/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            inference_backend.h                                ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-04-15 18:45:18                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     145                                            ║
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
#include <vector>
#include <functional>
#include <mutex>

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
    [[nodiscard]] virtual std::string generate(const std::string& prompt,
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
    [[nodiscard]] virtual bool isAvailable() const = 0;

    /**
     * @brief Human-readable description of the backend for logging.
     *
     * Example: "llama.cpp/mistral-7b-Q4_K_M + legal-lora-v2"
     */
    [[nodiscard]] virtual std::string description() const = 0;
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
    using GenerateFn = std::function<std::string(const std::string& prompt,
                                                 int max_tokens,
                                                 double temperature,
                                                 const std::string& lora_adapter)>;
    using AvailabilityFn = std::function<bool()>;

    static void setGenerateFn(GenerateFn fn) {
        std::lock_guard<std::mutex> lk(generateFnMutex());
        generateFnStorage() = std::move(fn);
    }
    static void setAvailabilityFn(AvailabilityFn fn) {
        std::lock_guard<std::mutex> lk(availabilityFnMutex());
        availabilityFnStorage() = std::move(fn);
    }

    std::string generate(const std::string& prompt,
                         int   max_tokens,
                         double temperature,
                         const std::string& lora_adapter) override {
        GenerateFn fn;
        {
            std::lock_guard<std::mutex> lk(generateFnMutex());
            fn = generateFnStorage();
        }
        if (fn) {
            try {
                return fn(prompt, max_tokens, temperature, lora_adapter);
            } catch (...) {
                return {};
            }
        }
        return {};
    }

    bool        isAvailable() const override {
        AvailabilityFn fn;
        {
            std::lock_guard<std::mutex> lk(availabilityFnMutex());
            fn = availabilityFnStorage();
        }
        if (fn) {
            try {
                return fn();
            } catch (...) {
                return false;
            }
        }
        return false;
    }
    std::string description() const override { return "NullTextGenerationBackend (no-op)"; }

private:
    static std::mutex& generateFnMutex() {
        static std::mutex m;
        return m;
    }
    static GenerateFn& generateFnStorage() {
        static GenerateFn fn;
        return fn;
    }
    static std::mutex& availabilityFnMutex() {
        static std::mutex m;
        return m;
    }
    static AvailabilityFn& availabilityFnStorage() {
        static AvailabilityFn fn;
        return fn;
    }
};

// ============================================================================
// IEmbeddingBackend — pure abstraction for dense text embedding (v1.4.0)
// ============================================================================

/**
 * @brief Produces a dense embedding vector from a text snippet.
 *
 * Implementations include:
 *  - ONNX-CLIP sentence encoder (when THEMIS_ENABLE_ONNX_CLIP is ON)
 *  - multilingual-E5 via llama.cpp embeddings API
 *  - NullEmbeddingBackend (zero-vector stub, isAvailable() == false)
 *
 * Thread safety: implementations MUST be thread-safe.
 */
class IEmbeddingBackend {
public:
    virtual ~IEmbeddingBackend() = default;

    /**
     * @brief Compute a dense embedding for @p text.
     *
     * @param text  Input text snippet (UTF-8).
     * @return      Dense embedding vector of `dimensions()` floats.
     *              Returns an empty vector on error; callers should
     *              check `isAvailable()` before calling.
     */
    [[nodiscard]] virtual std::vector<float> embed(const std::string& text) = 0;

    /**
     * @brief Number of dimensions in the embedding vector.
     */
    [[nodiscard]] virtual int dimensions() const = 0;

    /**
     * @brief Return true when the backend is ready for inference.
     */
    [[nodiscard]] virtual bool isAvailable() const = 0;

    /**
     * @brief Human-readable backend description for logging.
     *
     * Example: "multilingual-E5-base ONNX (768-d)"
     */
    [[nodiscard]] virtual std::string description() const = 0;
};

// ============================================================================
// NullEmbeddingBackend — zero-vector stub for testing / disabled configs
// ============================================================================

// STUB/SIMULATION NOTE:
// Purpose: Safe default when no real embedding model is configured; keeps
//   ChunkEmbedStep compilable without a live ONNX/llama model.
// Activation: Default in ChunkEmbedStep when no IEmbeddingBackend is injected.
// Production Delta: Returns zero-filled vector; isAvailable() returns false.
// Removal Plan: Not removed — remains the no-config default.
//   Inject a real backend (ONNX-CLIP, multilingual-E5) for production use.

/**
 * @brief Returns zero-filled embedding vectors of configurable dimensionality.
 */
class NullEmbeddingBackend : public IEmbeddingBackend {
public:
    using EmbedFn = std::function<std::vector<float>(const std::string& text, int dims)>;
    using AvailabilityFn = std::function<bool()>;

    static void setEmbedFn(EmbedFn fn) {
        std::lock_guard<std::mutex> lk(embedFnMutex());
        embedFnStorage() = std::move(fn);
    }
    static void setAvailabilityFn(AvailabilityFn fn) {
        std::lock_guard<std::mutex> lk(availabilityFnMutex());
        availabilityFnStorage() = std::move(fn);
    }

    /// Construct with embedding dimensionality (default: 768).
    explicit NullEmbeddingBackend(int dims = 768) : dims_(dims) {}

    std::vector<float> embed(const std::string& text) override {
        EmbedFn fn;
        {
            std::lock_guard<std::mutex> lk(embedFnMutex());
            fn = embedFnStorage();
        }
        if (fn) {
            try {
                return fn(text, dims_);
            } catch (...) {
                return std::vector<float>(static_cast<std::size_t>(dims_), 0.0f);
            }
        }
        return std::vector<float>(static_cast<std::size_t>(dims_), 0.0f);
    }

    int  dimensions()  const override { return dims_; }
    bool isAvailable() const override {
        AvailabilityFn fn;
        {
            std::lock_guard<std::mutex> lk(availabilityFnMutex());
            fn = availabilityFnStorage();
        }
        if (fn) {
            try {
                return fn();
            } catch (...) {
                return false;
            }
        }
        return false;
    }
    std::string description() const override {
        return "NullEmbeddingBackend (zero-vector, dims=" + std::to_string(dims_) + ")";
    }

private:
    static std::mutex& embedFnMutex() {
        static std::mutex m;
        return m;
    }
    static EmbedFn& embedFnStorage() {
        static EmbedFn fn;
        return fn;
    }
    static std::mutex& availabilityFnMutex() {
        static std::mutex m;
        return m;
    }
    static AvailabilityFn& availabilityFnStorage() {
        static AvailabilityFn fn;
        return fn;
    }

    int dims_;
};

} // namespace ingestion
} // namespace themis
