/**
 * @file inference_backend.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.2
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=13; TODO=1, Stub=8, Unimpl=0, Mock=1, Sim=3, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

// ============================================================================
// ITensorDecompositionBackend — pure abstraction for Tensor-Train decomposition
// ============================================================================

/**
 * @brief Abstract interface for Tensor-Train (TT) decomposition used by the
 *        ingestion pipeline.
 *
 * ## Motivation (SoC / DIP)
 *
 * The ingestion module must not depend on any concrete tensor-storage backend.
 * Instead it depends only on this lightweight interface.  The binding to the
 * real `TensorTrainDecomposer` (stored in `storage/`) is performed in the
 * `tensor` module through `TensorIngestionBridge` and injected at construction
 * time — the ingestion engine never sees a single `storage/` or `tensor/` header.
 *
 * ## Dependency graph (SoC / DIP compliant)
 * @code
 *   ingestion  →  ITensorDecompositionBackend  (this file)
 *                         ↑ implemented by
 *   tensor/    →  TensorIngestionBridge : ITensorDecompositionBackend
 *                         ↓ calls
 *               →  TensorTrainDecomposer::decompose()
 * @endcode
 *
 * ## Returned record format
 *
 * `decompose()` returns a `TensorCoreRecord` (defined in extraction_context.h)
 * that wraps the flattened TT-core bytes produced by `TTTrain::serialize()`.
 * The record also carries provenance metadata from `ExtractionContext::manifest`
 * so that regulated-industry consumers (FITKO, eJustice) can trace every
 * core back to its source document.
 *
 * ## κ-gate (avoid wasteful decomposition)
 *
 * `shouldDecompose()` performs a cheap pilot check (Frobenius norm + pilot
 * SVD on a small random sample) and returns `true` only when the data is
 * expected to achieve a compression ratio κ ≥ `min_kappa`.  The default
 * `min_kappa = 1.3` is the boundary identified in
 * `research/HNSW_FAISS_TT_BOUNDARY_ANALYSIS.md`.
 *
 * Thread safety: all methods MUST be thread-safe.
 */

/// @brief Opaque byte blob for one TT-train serialised by TTTrain::serialize().
using SerializedTTTrain = std::vector<uint8_t>;

/**
 * @brief One TT-decomposed record produced by `builtin.chunk_tt_decompose`.
 *
 * Kept as a forward-declared-friendly struct so `ExtractionContext` does not
 * need to include any tensor headers.
 */
struct TensorCoreRecord {
    std::string  chunk_id;        ///< Matches VectorRecord::chunk_id (file_id:seq)
    std::string  source_file_id;  ///< SHA-256 of the originating file
    std::size_t  order{0};        ///< TT order (number of cores / modes)
    std::size_t  max_rank{0};     ///< Maximum bond dimension achieved
    double       compression_ratio{0.0}; ///< dense_elements / tt_parameters
    double       achieved_eps{0.0};      ///< ‖T-T_approx‖_F / ‖T‖_F
    SerializedTTTrain serialized_train;  ///< Raw bytes from TTTrain::serialize()
    std::unordered_map<std::string, std::string> metadata;
    ///< Provenance: "source_file", "page", "section_ref", "tenant_id", etc.
};

/** @brief I tensor decomposition backend implementation. */
class ITensorDecompositionBackend {
public:
    virtual ~ITensorDecompositionBackend() = default;

    /**
     * @brief Decompose a dense embedding vector into TT-format.
     *
     * @param embedding    Dense float32 embedding from a prior `chunk_embed` step.
     * @param chunk_id     Identifier linking result to the originating chunk.
     * @param source_file_id  SHA-256 digest of the originating file.
     * @param epsilon      Relative reconstruction error tolerance ε ∈ (0, 1].
     *                     Passed directly to `TensorTrainDecomposer`.
     * @param max_rank     Hard bond-dimension cap (0 = no cap).
     * @return Populated `TensorCoreRecord`.  On error, returns a record with
     *         empty `serialized_train` so callers can degrade gracefully.
     */
    [[nodiscard]] virtual TensorCoreRecord decompose(
        const std::vector<float>& embedding,
        const std::string&        chunk_id,
        const std::string&        source_file_id,
        double                    epsilon  = 0.01,
        std::size_t               max_rank = 0) = 0;

    /**
     * @brief κ-gate: estimate whether decomposition is worthwhile.
     *
     * Returns `true` when the pilot compressibility estimate κ ≥ `min_kappa`.
     * Implementations are permitted to use a randomised approximation.
     *
     * @param embedding  The dense vector to probe.
     * @param min_kappa  Minimum compression ratio to justify decomposition.
     *                   Default: 1.3 (boundary from TT boundary analysis).
     */
    [[nodiscard]] virtual bool shouldDecompose(
        const std::vector<float>& embedding,
        double min_kappa = 1.3) const = 0;

    /**
     * @brief Return true when the backend is configured and operational.
     */
    [[nodiscard]] virtual bool isAvailable() const = 0;

    /**
     * @brief Human-readable description of the backend for logging.
     *
     * Example: "TensorIngestionBridge → TensorTrainDecomposer (ε=0.01)"
     */
    [[nodiscard]] virtual std::string description() const = 0;
};

// ============================================================================
// NullTensorDecompositionBackend — always-unavailable stub
// ============================================================================

// STUB/SIMULATION NOTE:
// Purpose: Safe default when no real tensor decomposer is configured; keeps
//   ChunkTtDecomposeStep compilable without a live TensorTrainDecomposer.
// Activation: Default in ChunkTtDecomposeStep when no
//   ITensorDecompositionBackend is injected.
// Production Delta: isAvailable() returns false; decompose() returns an empty
//   TensorCoreRecord; shouldDecompose() always returns false.
// Removal Plan: Not removed — remains the no-config default.
//   Inject a real TensorIngestionBridge for production use.

/**
 * @brief No-op backend that signals unavailability.
 *
 * Used as the default when no real tensor decomposer is configured.
 */
class NullTensorDecompositionBackend : public ITensorDecompositionBackend {
public:
    TensorCoreRecord decompose(
        const std::vector<float>& /*embedding*/,
        const std::string&        chunk_id,
        const std::string&        source_file_id,
        double   /*epsilon*/,
        std::size_t /*max_rank*/) override
    {
        TensorCoreRecord rec;
        rec.chunk_id       = chunk_id;
        rec.source_file_id = source_file_id;
        return rec;
    }

    bool shouldDecompose(const std::vector<float>& /*embedding*/,
                         double /*min_kappa*/) const override {
        return false;
    }

    bool        isAvailable() const override { return false; }
    std::string description() const override {
        return "NullTensorDecompositionBackend (no-op)";
    }
};

} // namespace ingestion
} // namespace themis
