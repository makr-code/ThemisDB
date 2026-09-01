/**
 * @file llama_wrapper.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.9.0-beta
 * @note Maturity: PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "llm/llm_plugin_interface.h"
#include "llm/model_loader.h"
#include "llm/multi_lora_manager.h"
#include "llm/llm_prefix_cache.h"
#include "llm/continuous_batch_scheduler.h"
#include "llm/paged_kv_cache.h"
#include "llm/grafana_metrics.h"
#include "llm/llm_response_cache.h"
#include "llm/grammar.h"
#include "llm/grammar_cache.h"
#include "llm/llamacpp_inference_engine.h"  // provides LLMOutputValidator
#ifdef THEMIS_ENABLE_VISION
#include "llm/vision_encoder.h"
#endif
#include <mutex>
#include <unordered_map>
#include <memory>

#ifdef ERROR
#undef ERROR
#endif

// Forward declarations for ThemisDB storage classes
namespace themis {
namespace llm {
    class LLMModelStorage;
}
namespace storage {
    class BlobStorageManager;
}
namespace security {
    class FieldEncryption;
}
}

// Forward declarations for llama.cpp types
struct llama_model;
struct llama_context;
typedef int32_t llama_token;

namespace themis {
namespace llm {

/**
 * @brief RoPE scaling method enumeration
 */
enum class RopeScalingMethod {
    LINEAR,   // Linear scaling - simple, works for 2-4x
    NTK,      // NTK-Aware scaling - better quality than linear
    YARN,     // YaRN scaling - best quality for high factors (8x+)
    DYNAMIC   // Dynamic scaling - adapts to input length
};

/**
 * @brief Chat role enumeration for type-safe message roles
 */
enum class ChatRole {
    System,     // System message (instructions, persona)
    User,       // User message (query, input)
    Assistant   // Assistant message (response, output)
};

/**
 * @brief Chat message structure for multi-turn conversations
 */
struct ChatMessage {
    std::string role;      // "system", "user", "assistant" (kept as string for compatibility)
    std::string content;   // Message content
    
    // Helper constructor for enum-based creation
    ChatMessage(ChatRole r, const std::string& c) 
        : content(c) {
        switch (r) {
            case ChatRole::System: role = "system"; break;
            case ChatRole::User: role = "user"; break;
            case ChatRole::Assistant: role = "assistant"; break;
        }
    }
    
    // Default constructor for string-based creation (backwards compatibility)
    ChatMessage(const std::string& r, const std::string& c)
        : role(r), content(c) {}
    
    ChatMessage() = default;
};

/**
 * @brief Chat template format options
 */
enum class ChatFormat {
    ChatML,      // ChatML format: <|im_start|>role\ncontent<|im_end|>
    Llama2,      // Llama-2 format: [INST] content [/INST]
    Vicuna,      // Vicuna format: USER: content\nASSISTANT:
    Alpaca       // Alpaca format: ### Instruction:\ncontent\n### Response:
};

/**
 * @brief LlamaWrapper state machine states
 * 
 * Explicit state tracking prevents silent stub responses and enables
 * proper error handling in production RAG pipelines.
 */
enum class WrapperState {
    UNINITIALIZED,   // Constructor called, not yet loading
    LOADING,         // Async model load in progress
    READY,           // Model loaded, context created, ready for inference
    ERROR_STATE,     // Unrecoverable error (e.g., model load failed) - renamed from ERROR to avoid Windows macro conflict
    UNAVAILABLE      // Temporary unavailability (e.g., OOM, evicted)
};

/**
 * @brief State transition record for debugging and observability
 */
struct StateTransition {
    WrapperState from_state;
    WrapperState to_state;
    std::string reason;      // Why transition happened
    std::chrono::system_clock::time_point timestamp;
    
    StateTransition(WrapperState from, WrapperState to, const std::string& r)
        : from_state(from), to_state(to), reason(r),
          timestamp(std::chrono::system_clock::now()) {}
};

/**
 * @brief llama.cpp plugin implementation
 * 
 * This is a reference implementation showing how to create
 * an LLM plugin for ThemisDB. It wraps llama.cpp functionality
 * into the ILLMPlugin interface.
 * 
 * Uses LazyModelLoader (Ollama-style) and MultiLoRAManager (vLLM-style)
 * for efficient resource management.
 * 
 * Note: Actual llama.cpp integration will be done in v1.3.0.
 * This provides the plugin structure and API design.
 */
class LlamaWrapper : public ILLMPlugin {
public:
    /**
     * @brief Configuration for llama.cpp backend
     */
    struct Config {
        // GPU settings
        int n_gpu_layers = 32;        // Number of layers to offload to GPU
        bool use_mmap = true;         // Memory-map model file
        bool use_mlock = false;       // Lock memory (prevent swapping)
        
        // Context settings
        int n_ctx = 4096;             // Context length
        int n_batch = 512;            // Batch size for prompt processing
        int n_threads = 8;            // CPU threads (for layers not on GPU)
        
        // Memory management
        size_t max_vram_mb = 14336;   // Max VRAM to use (14GB default)
        bool unified_memory = false;  // Use CUDA unified memory
        
        // Performance optimizations (llama.cpp features)
        bool use_flash_attn = true;   // Flash Attention for 15-25% speedup
        bool use_kv_cache_reuse = true; // KV-Cache Reuse for 10-20x first-token speedup
        bool enable_embeddings = false; // Enable embeddings extraction mode
        
        // Speculative Decoding (Phase 2.1)
        bool use_speculative_decoding = false; // 2-3x inference speedup
        std::string draft_model_path;          // Path to draft model
        int draft_n_gpu_layers = 16;           // GPU layers for draft model
        int speculative_tokens = 5;            // Number of tokens to speculate
        float acceptance_threshold = 0.8f;     // Probability threshold for acceptance
        bool enable_draft_kv_cache = true;     // KV cache for draft model
        
        // Continuous Batching (Phase 2.2)
        bool use_continuous_batching = false;  // 8x throughput improvement
        size_t max_batch_size = 32;            // Max sequences in batch
        size_t max_concurrent_requests = 128;   // Max pending requests
        size_t max_tokens_per_batch = 8192;    // Total token budget per batch
        std::string scheduler_policy = "priority"; // fifo, priority, sjf
        bool enable_preemption = true;         // Allow request preemption
        bool enable_chunked_prefill = true;    // Chunk large prefills
        size_t prefill_chunk_size = 512;       // Tokens per prefill chunk
        
        // Lazy loading (Ollama-style)
        LazyModelLoader::Config lazy_loader_config;
        
        // Multi-LoRA (vLLM-style)
        MultiLoRAManager::Config multi_lora_config;
        
        // KV-Cache Reuse (Prefix Caching)
        LLMPrefixCache::Config prefix_cache_config;
        // Response cache (optional) — disabled by default to avoid
        // unconditional RocksDB initialisation during startup.
        // Enable explicitly when a persistent response cache is desired.
        bool enable_response_cache = false;
        LLMResponseCache::Config response_cache_config;
        
        // Grammar-Constrained Generation (Phase 3.2)
        struct GrammarConfig {
            bool enabled = false;
            std::string default_grammar = "json";      // Default built-in grammar
            std::string custom_grammars_path = "/grammars/";  // Path to custom grammar files
            bool cache_grammars = true;                 // Enable grammar caching
            size_t max_cached_grammars = 100;          // Max grammars to cache
        };
        GrammarConfig grammar_config;
        // RoPE Scaling (Phase 3.1) - Extended Context Window
        struct RopeScalingConfig {
            bool enabled = false;
            RopeScalingMethod method = RopeScalingMethod::YARN;
            int max_context = 32768;        // Target context length (8x increase)
            int original_context = 4096;    // Model's trained context length
            
            // YaRN-specific parameters (used when method == YARN)
            float yarn_ext_factor = 1.0f;
            float yarn_attn_factor = 1.0f;
            float yarn_beta_fast = 32.0f;
            float yarn_beta_slow = 1.0f;
        } rope_scaling;
        // Vision Support (Multi-Modal LLM)
        bool enable_vision = false;           // Enable vision/multi-modal support
        std::string clip_model_path;          // Path to CLIP vision encoder model
        int vision_threads = 4;               // Threads for image encoding
        bool preload_vision = true;           // Keep vision encoder in memory
        
        // Output Validation (Production Readiness)
        bool enable_output_validation = true;  // Enable output validation
        int min_output_length = 1;             // Minimum response length (chars)
        int max_output_length = 100000;        // Maximum response length
        bool require_utf8 = true;              // Enforce UTF-8 encoding
        double min_coherence = 0.3;            // Minimum coherence score (0-1)
        
        // Timeouts / Backpressure (Q1 production-readiness)
        // Maximum wall-clock time (milliseconds) allowed for a single inference
        // request from submission to last token.  0 means unlimited (default).
        // Requests that exceed this limit are cancelled and the caller receives
        // an error response.
        uint32_t request_timeout_ms = 0;

        // Model integrity verification (anti-poisoning)
        // When non-empty, loadModel() verifies the model file's SHA-256 digest
        // against this value before proceeding with the load.  An empty string
        // disables the check (a warning is emitted in that case).
        std::string expected_model_sha256;
        
        // Require model integrity verification by default (security hardening)
        // When true, loadModel() will fail if no checksum is provided.
        // Set to false to allow loading models without integrity verification
        // (not recommended for production).
        bool require_model_integrity = true;
    };
    
    explicit LlamaWrapper(const Config& config);
    ~LlamaWrapper() override;
    
    // Prevent copying
    LlamaWrapper(const LlamaWrapper&) = delete;
    LlamaWrapper& operator=(const LlamaWrapper&) = delete;
    
    // Set metrics collector (optional)
    void setMetricsCollector(monitoring::LLMMetricsCollector* collector) {
        metrics_collector_ = collector;
        
        // Also set on response cache if enabled
        if (response_cache_) {
            response_cache_->setMetricsCollector(collector);
        }
    }
    
    // ═══════════════════════════════════════════════════════════
    // Model Management
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Load a model file and initialize lazy runtime state for inference.
     * @param model_path Path to the model file on disk.
     * @param config Optional load configuration. Supports SHA-256 integrity
     *        hints via `expected_checksum`, `model_checksum`, or `checksum`.
     *        When no expected hash is supplied, loading continues but emits a
     *        security warning instead of enforcing a hard failure.
     * @return true when the model loads successfully; false on I/O, integrity,
     *         or backend initialization failures.
     */
    bool loadModel(
        const std::string& model_path,
        const json& config = {}
    ) override;
    
    /**
     * @brief Load model from ThemisDB storage
     * 
     * Loads a model that was previously stored in ThemisDB's blob storage.
     * This enables native model storage in the database without requiring
     * filesystem access.
     * 
     * Process:
     * 1. Retrieve model metadata from LLMModelStorage
     * 2. Download model blob from BlobStorageManager
     * 3. Handle decryption if encryption is enabled
     * 4. Write to temporary file (with cleanup)
     * 5. Load model using standard loadModel() flow
     * 
     * @param model_id Unique model identifier stored in ThemisDB
     * @param storage LLMModelStorage instance for metadata retrieval
     * @param blob_manager BlobStorageManager for blob download
     * @param encryption Optional encryption service for decryption
     * @param config Optional loading configuration
     * @return true if model loaded successfully, false otherwise
     * 
     * @throws std::runtime_error if model not found in storage
     * @throws std::runtime_error if blob retrieval fails
     * @throws std::runtime_error if decryption fails (when encryption enabled)
     */
    bool loadModelFromThemisDB(
        const std::string& model_id,
        std::shared_ptr<LLMModelStorage> storage,
        std::shared_ptr<storage::BlobStorageManager> blob_manager,
        std::shared_ptr<::themis::security::FieldEncryption> encryption = nullptr,
        const json& config = {}
    );
    
    /**
     * @brief Clean up old temporary model files
     * 
     * Removes cached model files from /tmp/themisdb_models/ that are older
     * than the specified number of days. This helps prevent disk space issues
     * from accumulated cached models.
     * 
     * @param days_old Remove files older than this many days (default: 7)
     * @return Number of files removed
     */
    static size_t cleanupTempModels(int days_old = 7);
    
    void unloadModel() override;
    
    /**
     * @brief Verify model file integrity using checksum
     * 
     * Verifies that a model file matches the expected checksum to detect
     * corruption or tampering. Supports SHA256 and MD5 (legacy/deprecated).
     * 
     * @param file_path Path to the model file to verify
     * @param expected_checksum Expected checksum value
     * @param checksum_type Checksum algorithm ("sha256" or "md5")
     * @return true if checksum matches, false if mismatch or verification fails
     */
    static bool verifyModelIntegrity(
        const std::string& file_path,
        const std::string& expected_checksum,
        const std::string& checksum_type = "sha256"
    );
    
    /**
     * @brief Calculate SHA256 checksum of a model file
     * 
     * Computes the SHA256 hash of a model file for integrity verification
     * and storage in metadata.
     * 
     * @param file_path Path to the model file
     * @return SHA256 hash as hex string, or empty string on error
     */
    static std::string calculateModelChecksum(const std::string& file_path);
    
    std::optional<ModelInfo> getModelInfo() const override;
    
    bool isModelLoaded() const override;
    
    // ═══════════════════════════════════════════════════════════
    // LoRA Management
    // ═══════════════════════════════════════════════════════════
    
    bool loadLoRA(
        const std::string& lora_id,
        const std::string& lora_path,
        float scale = 1.0f
    ) override;
    
    bool unloadLoRA(const std::string& lora_id) override;
    
    std::vector<LoRAInfo> listLoRAs() const override;
    
    // ═══════════════════════════════════════════════════════════
    // Inference
    // ═══════════════════════════════════════════════════════════
    
    InferenceResponse generate(const InferenceRequest& request) override;

    /**
     * @brief Generate draft token IDs and raw logits for speculative decoding.
     *
     * Evaluates the request prompt, then iteratively samples @p k draft tokens
     * from the current model while capturing the raw pre-sampling logits row for
     * each step. The returned rows are compatible with
     * SpeculativeDecoder::verify().
     *
     * @param request Inference request; only prompt and sampling parameters are used.
     * @param k Number of draft tokens to generate.
     * @param vocab_size_hint Optional expected vocabulary size; if it differs from
     *        the loaded model vocabulary, the model vocabulary is used.
     * @return Draft token IDs and aligned raw logits.
     *
     * @throws std::runtime_error if no model/context is loaded or llama_decode fails.
     * @throws std::invalid_argument if @p k is zero.
     */
    [[nodiscard]] DraftTokensResult generateDraftTokens(
        const InferenceRequest& request,
        size_t k,
        size_t vocab_size_hint
    ) override;

    /**
     * @brief Tokenize arbitrary text with the loaded llama.cpp vocabulary.
     *
     * Intended as a production bridge for speculative-draft/runtime wiring
     * when the caller needs real token IDs for externally supplied text.
     *
     * @param text Text to tokenize.
     * @param add_bos Whether to prepend the BOS token when supported.
     * @return Token IDs in llama vocabulary space.
     *
     * @throws std::runtime_error if no model is loaded or tokenization fails.
     */
    [[nodiscard]] std::vector<int> tokenizeForBridge(
        const std::string& text,
        bool add_bos = true
    );

    /**
     * @brief Compute exact target-model logits for a speculative draft token sequence.
     *
     * Evaluates the request prompt once, captures the next-token logits for the
     * current prefix, then incrementally feeds each supplied draft token back
     * through the live llama.cpp context and captures the resulting next-token
     * logits after every step. The returned matrix therefore has exactly
     * `draft_token_ids.size() + 1` rows and is directly compatible with
     * `SpeculativeDecoder::verify()`.
     *
     * @param request Inference request whose prompt forms the verification prefix.
     * @param draft_token_ids Draft token IDs to validate in target-vocabulary space.
     * @return `(K+1) x vocab_size` raw target-logit matrix.
     *
     * @throws std::runtime_error if no live model/context is available or prompt/token evaluation fails.
     * @throws std::invalid_argument if any supplied token is outside the loaded vocabulary range.
     */
    [[nodiscard]] std::vector<std::vector<float>> computeTargetLogitsForTokens(
        const InferenceRequest& request,
        const std::vector<int>& draft_token_ids
    );
    
    InferenceResponse generateRAG(
        const RAGContext& rag_context,
        const InferenceRequest& request
    ) override;
    
#ifdef THEMIS_ENABLE_VISION
    /**
     * @brief Generate response with vision support (multi-modal)
     */
    VisionResponse generateVision(const VisionRequest& vision_request);
#endif
    
    std::vector<float> embed(const std::string& text) override;
    
    // ═══════════════════════════════════════════════════════════
    // Capabilities
    // ═══════════════════════════════════════════════════════════
    
    LLMCapabilities getCapabilities() const override;
    
    json getMemoryStats() const override;
    
    json getPerformanceStats() const override;
    
    // ═══════════════════════════════════════════════════════════
    // State Management (Production Readiness)
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Get current wrapper state
     * @return Current state of the wrapper
     */
    WrapperState state() const;
    
    /**
     * @brief Get state as human-readable string
     */
    std::string stateString() const;
    
    /**
     * @brief Get state transition history for debugging
     * @return Vector of state transitions
     */
    std::vector<StateTransition> stateHistory() const;
    
    /**
     * @brief Clear state history (for memory management)
     */
    void clearStateHistory();
    
    // ═══════════════════════════════════════════════════════════
    // Distributed Features
    // ═══════════════════════════════════════════════════════════
    
    std::vector<uint8_t> exportLoRA(const std::string& lora_id) override;
    
    bool importLoRA(
        const std::string& lora_id,
        const std::vector<uint8_t>& data
    ) override;

    // Non-ILLMPlugin convenience method for tests
    std::string getName() const { return "llamacpp"; }
    
    // ═══════════════════════════════════════════════════════════
    // Cache Management (Optional Features)
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Get prefix cache statistics
     * @return Cache statistics (hits, misses, hit rate) or nullopt if cache disabled
     */
    std::optional<PrefixCacheStatistics> getPrefixCacheStats() const;
    
    /**
     * @brief Clear prefix cache
     */
    void clearPrefixCache();
    
    /**
     * @brief Get speculative decoding statistics
     * @return Speculative decoding stats or nullopt if disabled
     */
    struct SpeculativeDecodingStats {
        size_t total_speculations = 0;
        size_t total_accepted = 0;
        size_t total_rejected = 0;
        double avg_acceptance_rate = 0.0;
        double avg_speedup = 0.0;
    };
    
    std::optional<SpeculativeDecodingStats> getSpeculativeStats() const;
    
    /**
     * @brief Start continuous batching mode
     * Initializes the batch scheduler for high-throughput scenarios
     */
    void startBatchMode();
    
    /**
     * @brief Stop continuous batching mode
     */
    void stopBatchMode();
    
    /**
     * @brief Check if batch mode is active
     */
    bool isBatchModeActive() const;
    
    /**
     * @brief Submit async request to batch scheduler
     * @param request Inference request
     * @param priority Request priority
     * @param callback Callback for response (optional)
     * @return Request ID for tracking
     */
    std::string submitBatchRequest(
        const InferenceRequest& request,
        ContinuousBatchScheduler::RequestPriority priority = ContinuousBatchScheduler::RequestPriority::NORMAL,
        std::function<void(const InferenceResponse&)> callback = nullptr
    );
    
    /**
     * @brief Get batch scheduler statistics
     * @return Scheduler stats or nullopt if batch mode disabled
     */
    std::optional<ContinuousBatchScheduler::Stats> getBatchSchedulerStats() const;
    
    /**
     * @brief Format chat messages according to template
     */
    std::string formatChatMessages(
        const std::vector<ChatMessage>& messages,
        ChatFormat format = ChatFormat::ChatML
    );
    
private:
    Config config_;
    
    // Ollama-style lazy model loader
    std::unique_ptr<LazyModelLoader> model_loader_;
    
    // vLLM-style multi-LoRA manager
    std::unique_ptr<MultiLoRAManager> lora_manager_;
    
    // Active LoRA adapter tracking (for auto-rebinding after context switches)
    std::string active_lora_adapter_;  // Currently applied adapter ID
    void* last_context_ptr_ = nullptr;  // Last context where adapter was applied
    
    // KV-Cache Reuse (Prefix Caching)
    std::unique_ptr<LLMPrefixCache> prefix_cache_;
    
    // Speculative Decoding (Phase 2.1)
    llama_model* draft_model_ = nullptr;
    llama_context* draft_context_ = nullptr;
    std::string draft_model_id_;
    SpeculativeDecodingStats speculative_stats_;
    
    // Continuous Batching (Phase 2.2)
    std::unique_ptr<ContinuousBatchScheduler> batch_scheduler_;
    std::unique_ptr<PagedKVCache> paged_kv_cache_;
    bool batch_mode_active_ = false;
    // Response cache for frequent queries
    std::unique_ptr<LLMResponseCache> response_cache_;
    
    // Grammar-Constrained Generation (Phase 3.2)
    std::unique_ptr<GrammarCache> grammar_cache_;
    std::unordered_map<std::string, std::string> builtin_grammars_;  // name -> ebnf text
    // Vision Support (Multi-Modal)
#ifdef THEMIS_ENABLE_VISION
    std::unique_ptr<VisionEncoder> vision_encoder_;
#endif
    bool vision_enabled_ = false;
    
    // Current active model
    std::string current_model_id_;
    std::string current_model_path_;
    std::string configured_model_id_;
    std::string configured_model_path_;
    
    // Statistics
    struct Stats {
        size_t total_inferences = 0;
        size_t total_tokens_generated = 0;
        double total_inference_time_ms = 0.0;
    };
    Stats stats_;
    
    // State machine (Production Readiness)
    WrapperState current_state_ = WrapperState::UNINITIALIZED;
    std::vector<StateTransition> state_history_;
    static constexpr size_t MAX_STATE_HISTORY = 100;  // Limit memory usage
    
    // Output validation (Production Readiness)
    std::unique_ptr<LLMOutputValidator> output_validator_;
    
    // Metrics collection (optional)
    monitoring::LLMMetricsCollector* metrics_collector_ = nullptr;
    
    // Thread safety
    mutable std::mutex mutex_;
    
    // Helper methods
    void validateConfig(const Config& config);
    
    std::string formatPromptForRAG(
        const RAGContext& rag_context,
        const InferenceRequest& request
    );
    
    void updateStatistics(const InferenceResponse& response);
    
    std::string extractModelId(const std::string& model_path);
    
    // State machine helpers (Production Readiness)
    void transitionToState(WrapperState new_state, const std::string& reason);
    static std::string stateToString(WrapperState state);
    
    // Grammar-related helpers (Phase 3.2)
    void initializeBuiltinGrammars();
    std::shared_ptr<Grammar> getOrCreateGrammar(const InferenceRequest& request);
    std::string loadGrammarFile(const std::string& grammar_name);
    
    // Speculative Decoding helpers
    bool loadDraftModel(const std::string& draft_path);
    void unloadDraftModel();
    InferenceResponse generateSpeculative(const InferenceRequest& request);
    InferenceResponse generateRegular(const InferenceRequest& request);
    float getProbability(float* logits, llama_token token, int32_t n_vocab);
    void synchronizeDraftToTarget(const std::vector<llama_token>& accepted_tokens);
    
    // Vision support helpers
#ifdef THEMIS_ENABLE_VISION
    bool initializeVisionEncoder();
    void shutdownVisionEncoder();
    std::string buildVisionPrompt(const VisionRequest& request);
#endif
    
    // Internal llama.cpp helper functions
    std::vector<llama_token> tokenizeInternal(
        llama_model* model,
        const std::string& text,
        bool add_bos
    );
    
    std::string detokenizeInternal(
        llama_context* ctx,
        const std::vector<llama_token>& tokens
    );
    
    llama_token sampleTokenInternal(
        llama_context* ctx,
        llama_model* model,
        float* logits,
        int32_t n_vocab,
        float temperature,
        float top_p,
        llama_grammar* grammar = nullptr
    );
    
    // Chat formatting helpers (implementation details)
    std::string formatChatML(const std::vector<ChatMessage>& messages);
    std::string formatLlama2(const std::vector<ChatMessage>& messages);
    std::string formatVicuna(const std::vector<ChatMessage>& messages);
    std::string formatAlpaca(const std::vector<ChatMessage>& messages);
    
public:
    // ═══════════════════════════════════════════════════════════
    // Output Formatting Helpers (MCP, SSE, AQL)
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Format response as JSON for MCP protocol
     * Converts InferenceResponse to MCP-compatible JSON format
     */
    static json formatAsMCPResponse(const InferenceResponse& response);
    
    /**
     * @brief Format response as SSE (Server-Sent Events) data
     * Returns SSE-formatted string: "data: {...}\n\n"
     */
    static std::string formatAsSSE(const InferenceResponse& response);
    
    /**
     * @brief Format response as JSON with embedded markdown
     * Useful for rich text responses with code blocks
     */
    static json formatAsJsonMarkdown(const InferenceResponse& response);
    
    /**
     * @brief Format streaming token as SSE event
     * For real-time token streaming via Server-Sent Events
     */
    static std::string formatStreamTokenAsSSE(const std::string& token, const std::string& request_id = "");
};

} // namespace llm
} // namespace themis
