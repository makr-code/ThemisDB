/*
 * ThemisDB | File: llm_plugin_interface.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 94/100
 * Gap Summary: total=7; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include "plugins/plugin_interface.h"
#include "llm/json_schema_converter.h"
#include "llm/context_window_budget.h"
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <optional>
#include <functional>
#include <nlohmann/json.hpp>

/**
 * @file llm_plugin_interface.h
 * @brief Plugin interface for LLM backends in ThemisDB
 * 
 * This interface enables plugin-based LLM integration for v1.3.0, supporting:
 * - Multiple LLM backends (llama.cpp, vLLM, etc.)
 * - LoRA adapter management
 * - Distributed reasoning capabilities
 * - Zero-copy memory access where possible
 * 
 * Based on AI_ECOSYSTEM_SHARDING_ARCHITECTURE.md design document.
 */

namespace themis {
namespace llm {

using json = nlohmann::json;

/**
 * @brief LLM-specific capabilities
 */
struct LLMCapabilities {
    // Model capabilities
    bool supports_instruct = false;      // Instruction-tuned models
    bool supports_chat = false;          // Chat/conversation models
    bool supports_completion = false;    // Text completion
    
    // Advanced features
    bool supports_lora = false;          // LoRA adapter support
    bool supports_quantization = false;  // Model quantization (Q4, Q8, etc.)
    bool supports_streaming = false;     // Token streaming
    bool supports_batching = false;      // Batch inference
    
    // Hardware acceleration
    bool gpu_accelerated = false;
    bool supports_cuda = false;
    bool supports_rocm = false;
    bool supports_metal = false;
    bool supports_vulkan = false;
    
    // Memory management
    bool supports_unified_memory = false;  // CUDA unified memory
    bool supports_zero_copy = false;       // Zero-copy from vector DB
    
    // Distributed features (for sharding architecture)
    bool supports_model_sharding = false;  // Model parallelism
    bool supports_pipeline_parallel = false;
    bool supports_tensor_parallel = false;

    // Multi-modal capabilities
    bool supports_multimodal = false;      // Image + text input (vision-language models)

    // Embedding / RAG / function-call support
    bool supports_embeddings = false;      // Text embedding (dense vector output)
    bool supports_rag = false;             // Retrieval-Augmented Generation
    bool supports_function_call = false;   // Structured function / tool calling

    // Plugin metadata
    std::string plugin_version;            // Semantic version of this plugin
};

/**
 * @brief Model information
 */
struct ModelInfo {
    std::string name;              // e.g., "mistral-7b-instruct"
    std::string path;              // Path to model file
    std::string format;            // e.g., "gguf", "safetensors"
    std::string architecture;      // e.g., "llama", "mistral", "gpt"
    std::string model_id;          // Logical id
    bool is_loaded = false;        // Load state
    size_t size_bytes = 0;
    std::string quantization;      // e.g., q4_0
    std::string loaded_at;         // Timestamp string
    
    size_t parameter_count = 0;    // Model size (e.g., 7B, 13B)
    size_t context_length = 0;     // Max context tokens
    size_t vocab_size = 0;
    
    size_t vram_required_mb = 0;   // Estimated VRAM usage
    size_t ram_required_mb = 0;    // RAM for CPU offload
    
    json metadata;                 // Additional model metadata
};

/**
 * @brief LoRA adapter information
 */
struct LoRAInfo {
    std::string id;                // Unique identifier
    std::string name;              // Human-readable name
    std::string path;              // Path to LoRA weights
    std::string base_model;        // Compatible base model
    std::string lora_id;           // Alias
    std::string adapter_id;        // Test-facing adapter id (alias of id)
    std::string base_model_id;     // Test-facing base model id (alias of base_model)
    bool is_loaded = false;
    
    size_t size_bytes = 0;
    float scale = 1.0f;            // LoRA scaling factor
    
    json metadata;                 // Domain, version, etc.
};

/**
 * @brief Inference request parameters
 */
struct InferenceRequest {
    std::string prompt;
    std::string model_id = "default";
    std::string request_id;      // Optional request identifier for tracing

    // OpenTelemetry distributed tracing context (W3C Trace Context format).
    // Populated by the caller when the request originates from a traced parent
    // span.  If non-empty, the inference engine propagates these values into
    // the response so downstream systems can correlate spans.
    // Format: lowercase hex — trace_id is 32 hex chars (128-bit),
    //                         span_id  is 16 hex chars (64-bit).
    std::string trace_id;        ///< W3C traceparent trace-id (128-bit hex, 32 chars)
    std::string span_id;         ///< W3C traceparent parent-id (64-bit hex, 16 chars)
    
    // Generation parameters
    int max_tokens = 512;
    float temperature = 0.7f;
    float top_p = 0.9f;
    int top_k = 40;
    float repetition_penalty = 1.1f;
    
    // Optional system prompt
    std::optional<std::string> system_prompt;
    
    // LoRA adapter to use (if any)
    std::optional<std::string> lora_adapter_id;
    
    // Grammar-constrained generation (Phase 3.2)
    std::optional<std::string> grammar_type;      // Built-in grammar: "json", "xml", "csv", "react_agent"
    std::optional<std::string> grammar_ebnf;      // Custom EBNF grammar text

    // JSON schema binding for structured output (Issue #1922)
    // When set, the output is constrained to produce valid JSON matching this schema.
    // Converted to grammar_ebnf via JsonSchemaConverter::schemaToEbnf() before inference.
    std::optional<json> json_schema;

    // Tool / function calling (Issue #1922)
    // When non-empty, the model is constrained to produce a tool call JSON:
    //   {"name": "<tool>", "arguments": {<args>}}
    // The tool call is parsed from response.text and stored in response.tool_calls.
    std::vector<ToolDefinition> tools;

    // Streaming callback
    std::function<void(const std::string& token)> stream_callback;
    
    // Stop sequences
    std::vector<std::string> stop_sequences;

    // Multi-modal image inputs (vision-language models).
    // When non-empty the inference engine encodes each image and injects the
    // resulting embeddings into the LLM context alongside the text prompt.
    // Requires the loaded model to have vision support (LLMCapabilities::supports_multimodal).
    std::vector<std::string> image_paths;   ///< Paths to image files (JPEG, PNG, …)
    
    // Metadata for tracking
    json metadata;
    
    // F2-3: Tenant identifier used for cache isolation.  Without this, two
    // tenants with identical prompts share a single cache entry, leaking
    // inference results across tenant boundaries.
    std::string tenant_id;
};

/**
 * @brief Inference response
 */
struct InferenceResponse {
    std::string request_id;      // Mirrors request id if provided
    std::string text;              // Generated text
    std::string model_id;          // Model identifier used
    bool cache_hit = false;        // Whether response came from cache

    // OpenTelemetry trace context propagated from the originating request.
    // The inference engine copies trace_id/span_id from InferenceRequest so
    // callers and observability pipelines can correlate the response to its
    // parent trace without accessing the original request object.
    std::string trace_id;        ///< W3C traceparent trace-id (echoed from request)
    std::string span_id;         ///< W3C traceparent parent-id (echoed from request)
    
    // Statistics
    int tokens_generated = 0;
    int tokens_prompt = 0;
    float inference_time_ms = 0.0f;
    float tokens_per_second = 0.0f;
    int64_t latency_ms = 0;        // Wall-clock latency in milliseconds
    
    // Model information
    std::string model_used;
    std::optional<std::string> lora_used;

    // Tool calls parsed from model output (Issue #1922).
    // Populated when InferenceRequest::tools is non-empty and the model
    // produces a valid tool call JSON object.
    std::vector<ToolCall> tool_calls;
    
    // Quality metrics (if available)
    std::optional<float> perplexity;
    std::vector<float> logprobs;   // Log probabilities per token
    
    json metadata;

    // Status
    bool success = false;          // true when inference completed without error
    std::string error_message;     // Non-empty on failure
};

/**
 * @brief RAG (Retrieval-Augmented Generation) context
 *
 * max_context_tokens should be set to ModelInfo::context_length of the loaded
 * model.  The RAGContextAssembler uses this value together with
 * response_budget_tokens to compute the exact token budget available for the
 * retrieved chunks.  Setting it to 0 triggers the 4 096-token fallback.
 */
struct RAGContext {
    std::string query;             // User query
    std::string collection_name;
    int top_k = 0;
    
    // Retrieved documents/chunks
    struct Document {
        std::string content;
        std::string source;        // Document identifier
        float relevance_score = 0.0f;
        json metadata;
    };
    std::vector<Document> documents;
    
    // Context assembly parameters
    // NOTE: set from ModelInfo::context_length — do NOT hardcode.
    int max_context_tokens = 0;    // 0 → falls back to kDefaultContextWindowTokens
    std::string context_template;  // How to format context

    // Response-budget reservation (tokens kept for the model answer).
    // The actual reservation is max(response_budget_tokens, 20 % of the window).
    int response_budget_tokens = 512;
};

/**
 * @brief Base interface for LLM plugins
 * 
 * All LLM backend plugins must implement this interface.
 * Examples: LlamaWrapper, VLLMPlugin, OpenAIPlugin, etc.
 */
class ILLMPlugin {
public:
    virtual ~ILLMPlugin() = default;
    
    // ═══════════════════════════════════════════════════════════
    // Model Management
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Load a model
     * @param model_path Path to model file
     * @param config Model configuration (JSON)
     * @return true if loaded successfully
     */
    [[nodiscard]] virtual bool loadModel(
        const std::string& model_path,
        const json& config = {}
    ) = 0;
    
    /**
     * @brief Unload current model
     */
    virtual void unloadModel() = 0;
    
    /**
     * @brief Get current model information
     */
    [[nodiscard]] virtual std::optional<ModelInfo> getModelInfo() const = 0;
    
    /**
     * @brief Check if model is loaded
     */
    [[nodiscard]] virtual bool isModelLoaded() const = 0;
    
    // ═══════════════════════════════════════════════════════════
    // LoRA Management
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Load a LoRA adapter
     * @param lora_id Unique identifier
     * @param lora_path Path to LoRA weights
     * @param scale LoRA scaling factor
     * @return true if loaded successfully
     */
    [[nodiscard]] virtual bool loadLoRA(
        const std::string& lora_id,
        const std::string& lora_path,
        float scale = 1.0f
    ) = 0;
    
    /**
     * @brief Unload a LoRA adapter
     */
    [[nodiscard]] virtual bool unloadLoRA(const std::string& lora_id) = 0;
    
    /**
     * @brief List loaded LoRA adapters
     */
    [[nodiscard]] virtual std::vector<LoRAInfo> listLoRAs() const = 0;
    
    // ═══════════════════════════════════════════════════════════
    // Inference
    // ═══════════════════════════════════════════════════════════

    /**
     * @brief Result of a draft-token generation pass for speculative decoding.
     *
     * Returned by generateDraftTokens().  Each element of `logits[i]` is a
     * vocab_size-dimensional raw logit vector for draft position i, suitable
     * for passing directly to SpeculativeDecoder::verify().
     */
    struct DraftTokensResult {
        /// K draft token IDs (one per speculative step).
        std::vector<int> tokens;
        /// K × vocab_size raw logit rows (row i corresponds to tokens[i]).
        std::vector<std::vector<float>> logits;
        /// Vocabulary size used for the logit rows.
        size_t vocab_size = 0;
    };

    // ─────────────────────────────────────────────────────────────────────
    // STUB #261 bridge — callback injection for generateDraftTokens()
    // ─────────────────────────────────────────────────────────────────────

    /// Callback type that replaces the default heuristic implementation of
    /// generateDraftTokens() without requiring a full plugin subclass.
    using GenerateDraftTokensFn = std::function<
        DraftTokensResult(const InferenceRequest& /*request*/,
                          size_t                  /*k*/,
                          size_t                  /*vocab_size_hint*/)>;

    /// Inject (or remove) a real generateDraftTokens() implementation into
    /// the default virtual method body.  Pass nullptr / empty fn to restore
    /// the built-in text-heuristic path.  Thread-safe with concurrent calls
    /// to generateDraftTokens().
    static void setDefaultGenerateDraftTokensFn(GenerateDraftTokensFn fn) {
        std::lock_guard<std::mutex> lk(s_draft_fn_mutex_);
        s_default_draft_fn_ = std::move(fn);
    }

    /**
     * @brief Generate K draft tokens with per-token logit distributions.
     *
     * Used by InferenceEngineEnhanced::trySpeculativeGeneration() to feed
     * real token IDs and logit distributions into SpeculativeDecoder::verify().
     *
     * When a fn has been injected via setDefaultGenerateDraftTokensFn() the
     * call is forwarded to that fn.  Otherwise the built-in heuristic applies:
     * generate() is called internally and the returned text is mapped to token
     * IDs via UTF-8 byte values modulo vocab_size; logit distributions are
     * peaked (+5 / −5) at the mapped IDs (STUB #261 — Q1 2027).
     *
     * @param request        Inference request (prompt + generation parameters).
     *                       max_tokens is overridden to k internally.
     * @param k              Number of draft tokens to produce.
     * @param vocab_size_hint Expected vocabulary size; 32 000 used as fallback.
     * @return DraftTokensResult with k tokens and k logit rows.
     */
    [[nodiscard]] virtual DraftTokensResult generateDraftTokens(
        const InferenceRequest& request,
        size_t                  k,
        size_t                  vocab_size_hint
    ) {
        // Check injected fn first (STUB #261 bridge).
        GenerateDraftTokensFn fn_copy;
        {
            std::lock_guard<std::mutex> lk(s_draft_fn_mutex_);
            fn_copy = s_default_draft_fn_;
        }
        if (fn_copy) {
            return fn_copy(request, k, vocab_size_hint);
        }

        // Built-in text-heuristic fallback.
        InferenceRequest draft_req = request;
        draft_req.max_tokens      = static_cast<int>(k);
        draft_req.stream_callback = nullptr;

        const auto resp = generate(draft_req);

        const size_t vocab         = (vocab_size_hint > 0) ? vocab_size_hint : 32000u;
        constexpr float kPeak      =  5.0f;
        constexpr float kBaseline  = -5.0f;

        DraftTokensResult result;
        result.vocab_size = vocab;
        result.tokens.reserve(k);
        result.logits.reserve(k);

        const std::string& text = resp.text;
        for (size_t i = 0; i < k; ++i) {
            const int token_id = (i < text.size())
                ? (static_cast<int>(static_cast<unsigned char>(text[i])) %
                   static_cast<int>(vocab))
                : 0;
            result.tokens.push_back(token_id);

            std::vector<float> row(vocab, kBaseline);
            row[static_cast<size_t>(token_id)] = kPeak;
            result.logits.push_back(std::move(row));
        }
        return result;
    }

private:
    // Inline static storage for the default generateDraftTokens() injection
    // (STUB #261 bridge).  Using inline static avoids a separate .cpp TU.
    // Placed in a private section between two public ones so that the injected
    // state cannot be accessed directly; access is exclusively through the
    // public static setter setDefaultGenerateDraftTokensFn().
    inline static std::mutex              s_draft_fn_mutex_;
    inline static GenerateDraftTokensFn   s_default_draft_fn_;

public:

    /**
     * @brief Generate text from prompt
     * @param request Inference parameters
     * @return Generated response
     */
    [[nodiscard]] virtual InferenceResponse generate(const InferenceRequest& request) = 0;
    
    /**
     * @brief RAG-enhanced generation
     * @param rag_context Retrieved documents and query
     * @param request Generation parameters
     * @return Generated response
     */
    [[nodiscard]] virtual InferenceResponse generateRAG(
        const RAGContext& rag_context,
        const InferenceRequest& request
    ) = 0;
    
    /**
     * @brief Embed text to vector
     * @param text Text to embed
     * @return Embedding vector (typically 768 or 1024 dimensions)
     */
    [[nodiscard]] virtual std::vector<float> embed(const std::string& text) = 0;
    
    // ═══════════════════════════════════════════════════════════
    // Capabilities
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Get plugin capabilities
     */
    [[nodiscard]] virtual LLMCapabilities getCapabilities() const = 0;
    
    /**
     * @brief Get memory usage statistics
     */
    [[nodiscard]] virtual json getMemoryStats() const = 0;
    
    /**
     * @brief Get performance statistics
     */
    [[nodiscard]] virtual json getPerformanceStats() const = 0;
    
    // ═══════════════════════════════════════════════════════════
    // Distributed Features (for sharding architecture)
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Export LoRA for transfer to another shard
     * @param lora_id LoRA identifier
     * @return Serialized LoRA weights
     */
    [[nodiscard]] virtual std::vector<uint8_t> exportLoRA(const std::string& lora_id) = 0;
    
    /**
     * @brief Import LoRA from another shard
     * @param lora_id LoRA identifier
     * @param data Serialized LoRA weights
     * @return true if imported successfully
     */
    [[nodiscard]] virtual bool importLoRA(
        const std::string& lora_id,
        const std::vector<uint8_t>& data
    ) = 0;
};

/**
 * @brief Wrapper to integrate ILLMPlugin with ThemisDB plugin system
 * 
 * This adapter class bridges ILLMPlugin to IThemisPlugin, allowing
 * LLM plugins to be managed by the unified PluginManager.
 */
class LLMPluginAdapter : public plugins::IThemisPlugin {
public:
    explicit LLMPluginAdapter(std::unique_ptr<ILLMPlugin> llm_plugin)
        : llm_plugin_(std::move(llm_plugin)) {}
    
    ~LLMPluginAdapter() override = default;

    // IThemisPlugin interface implementation
    const char* getName() const override { return "LLM Plugin"; }
    const char* getVersion() const override { return "1.0.0"; }
    plugins::PluginType getType() const override { 
        return plugins::PluginType::LLM_BACKEND;
    }
    
    plugins::PluginCapabilities getCapabilities() const override {
        auto llm_caps = llm_plugin_->getCapabilities();
        plugins::PluginCapabilities caps;
        caps.supports_streaming = llm_caps.supports_streaming;
        caps.supports_batching = llm_caps.supports_batching;
        caps.gpu_accelerated = llm_caps.gpu_accelerated;
        caps.thread_safe = true;
        return caps;
    }
    
    bool initialize(const char* config_json) override {
        try {
            json config = json::parse(config_json);
            if (config.contains("model_path")) {
                return llm_plugin_->loadModel(
                    config["model_path"].get<std::string>(),
                    config
                );
            }
            return true;
        } catch (...) {
            return false;
        }
    }
    
    void shutdown() override {
        llm_plugin_->unloadModel();
    }
    
    void* getInstance() override {
        return llm_plugin_.get();
    }
    
    // Direct access to LLM plugin
    ILLMPlugin* getLLMPlugin() { return llm_plugin_.get(); }
    const ILLMPlugin* getLLMPlugin() const { return llm_plugin_.get(); }
    
private:
    std::unique_ptr<ILLMPlugin> llm_plugin_;
};

} // namespace llm
} // namespace themis

/**
 * @brief Export macro for dynamic loading of LLM plugins.
 *
 * Add this macro once in the .cpp file of your LLM plugin implementation.
 */
#define THEMIS_LLM_PLUGIN()                                                        \
    extern "C" THEMIS_PLUGIN_EXPORT                                                \
        themis::llm::ILLMPlugin* themis_llm_create();                             \
    extern "C" THEMIS_PLUGIN_EXPORT                                                \
        void themis_llm_destroy(themis::llm::ILLMPlugin* p)

