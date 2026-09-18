/**
 * @file llama_wrapper.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.9.0-beta
 * @note Maturity: PRODUCTION-READY
 * @note Score: 100/100
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

enum class RopeScalingMethod {
    LINEAR,   // Linear scaling - simple, works for 2-4x
    NTK,      // NTK-Aware scaling - better quality than linear
    YARN,     // YaRN scaling - best quality for high factors (8x+)
    DYNAMIC   // Dynamic scaling - adapts to input length
};

enum class ChatRole {
    System,     // System message (instructions, persona)
    User,       // User message (query, input)
    Assistant   // Assistant message (response, output)
};

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

enum class ChatFormat {
    ChatML,      // ChatML format: <|im_start|>role\ncontent<|im_end|>
    Llama2,      // Llama-2 format: [INST] content [/INST]
    Vicuna,      // Vicuna format: USER: content\nASSISTANT:
    Alpaca       // Alpaca format: ### Instruction:\ncontent\n### Response:
};

enum class WrapperState {
    UNINITIALIZED,   // Constructor called, not yet loading
    LOADING,         // Async model load in progress
    READY,           // Model loaded, context created, ready for inference
    ERROR_STATE,     // Unrecoverable error (e.g., model load failed) - renamed from ERROR to avoid Windows macro conflict
    UNAVAILABLE      // Temporary unavailability (e.g., OOM, evicted)
};

struct StateTransition {
    WrapperState from_state;
    WrapperState to_state;
    std::string reason;      // Why transition happened
    std::chrono::system_clock::time_point timestamp;
    
    StateTransition(WrapperState from, WrapperState to, const std::string& r)
        : from_state(from), to_state(to), reason(r),
          timestamp(std::chrono::system_clock::now()) {}
};

class LlamaWrapper : public ILLMPlugin {
public:
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
    
    /**
     * @brief Llama Wrapper.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit LlamaWrapper(const Config& config);
    ~LlamaWrapper() override;
    
    // Prevent copying
    LlamaWrapper(const LlamaWrapper&) = delete;
    LlamaWrapper& operator=(const LlamaWrapper&) = delete;
    
    /**
     * @brief Set metrics collector (optional)
     * @param[in,out] collector Input/output parameter.
     * @details Implements setMetricsCollector without additional internal calls.
     */
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
    
    bool loadModel(
        const std::string& model_path,
        const json& config = {}
    ) override;
    
    bool loadModelFromThemisDB(
        const std::string& model_id,
        std::shared_ptr<LLMModelStorage> storage,
        std::shared_ptr<storage::BlobStorageManager> blob_manager,
        std::shared_ptr<::themis::security::FieldEncryption> encryption = nullptr,
        const json& config = {}
    );
    
    static size_t cleanupTempModels(int days_old = 7);
    
    void unloadModel() override;
    
    static bool verifyModelIntegrity(
        const std::string& file_path,
        const std::string& expected_checksum,
        const std::string& checksum_type = "sha256"
    );
    
    /**
     * @brief Calculate Model Checksum.
     * @param[in] file_path Path to the file.
     * @return Return value.
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

    [[nodiscard]] DraftTokensResult generateDraftTokens(
        const InferenceRequest& request,
        size_t k,
        size_t vocab_size_hint
    ) override;

    [[nodiscard]] std::vector<int> tokenizeForBridge(
        const std::string& text,
        bool add_bos = true
    );

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
     * @brief Generate Vision.
     * @param[in] vision_request Input parameter.
     * @return Return value.
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
    
    /**
     * @brief ═══════════════════════════════════════════════════════════ State Management (Production Readiness) ═══════════════════════════════════════════════════════════
     * @return Return value.
     */
    
    WrapperState state() const;
    
    /**
     * @brief State String.
     * @return Return value.
     */
    std::string stateString() const;
    
    /**
     * @brief State History.
     * @return Return value.
     */
    std::vector<StateTransition> stateHistory() const;
    
    /**
     * @brief Clear State History.
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
    
    /**
     * @brief ═══════════════════════════════════════════════════════════ Cache Management (Optional Features) ═══════════════════════════════════════════════════════════
     * @return Return value.
     */
    
    std::optional<PrefixCacheStatistics> getPrefixCacheStats() const;
    
    /**
     * @brief Clear Prefix Cache.
     */
    void clearPrefixCache();
    
    struct SpeculativeDecodingStats {
        size_t total_speculations = 0;
        size_t total_accepted = 0;
        size_t total_rejected = 0;
        double avg_acceptance_rate = 0.0;
        double avg_speedup = 0.0;
    };
    
    /**
     * @brief Get Speculative Stats.
     * @return Return value.
     */
    std::optional<SpeculativeDecodingStats> getSpeculativeStats() const;
    
    /**
     * @brief Start Batch Mode.
     */
    void startBatchMode();
    
    /**
     * @brief Stop Batch Mode.
     */
    void stopBatchMode();
    
    /**
     * @brief Is Batch Mode Active.
     * @return True when the operation succeeds.
     */
    bool isBatchModeActive() const;
    
    std::string submitBatchRequest(
        const InferenceRequest& request,
        ContinuousBatchScheduler::RequestPriority priority = ContinuousBatchScheduler::RequestPriority::NORMAL,
        std::function<void(const InferenceResponse&)> callback = nullptr
    );
    
    /**
     * @brief Get Batch Scheduler Stats.
     * @return Return value.
     */
    std::optional<ContinuousBatchScheduler::Stats> getBatchSchedulerStats() const;
    
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
    /**
     * @brief Validate Config.
     * @param[in] config Input parameter.
     */
    void validateConfig(const Config& config);
    
    /**
     * @brief Format Prompt For RAG.
     * @param[in] rag_context Input parameter.
     * @param[in] request Input parameter.
     * @return Return value.
     */
    std::string formatPromptForRAG(
        const RAGContext& rag_context,
        const InferenceRequest& request
    );
    
    /**
     * @brief Update Statistics.
     * @param[in] response Input parameter.
     */
    void updateStatistics(const InferenceResponse& response);
    
    /**
     * @brief Extract Model Id.
     * @param[in] model_path Path to the model.
     * @return Return value.
     */
    std::string extractModelId(const std::string& model_path);
    
    /**
     * @brief State machine helpers (Production Readiness)
     * @param[in] new_state Input parameter.
     * @param[in] reason Input parameter.
     */
    void transitionToState(WrapperState new_state, const std::string& reason);
    /**
     * @brief State To String.
     * @param[in] state Input parameter.
     * @return Return value.
     */
    static std::string stateToString(WrapperState state);
    
    /**
     * @brief Grammar-related helpers (Phase 3.
     * @details 2)
     */
    void initializeBuiltinGrammars();
    /**
     * @brief Get Or Create Grammar.
     * @param[in] request Input parameter.
     * @return Return value.
     */
    std::shared_ptr<Grammar> getOrCreateGrammar(const InferenceRequest& request);
    /**
     * @brief Load Grammar File.
     * @param[in] grammar_name Name of the grammar.
     * @return Return value.
     */
    std::string loadGrammarFile(const std::string& grammar_name);
    
    // Speculative Decoding helpers
    /**
     * @brief Load Draft Model.
     * @param[in] draft_path Path to the draft.
     * @return True when the operation succeeds.
     */
    bool loadDraftModel(const std::string& draft_path);
    /**
     * @brief Unload Draft Model.
     */
    void unloadDraftModel();
    /**
     * @brief Generate Speculative.
     * @param[in] request Input parameter.
     * @return Return value.
     */
    InferenceResponse generateSpeculative(const InferenceRequest& request);
    /**
     * @brief Generate Regular.
     * @param[in] request Input parameter.
     * @return Return value.
     */
    InferenceResponse generateRegular(const InferenceRequest& request);
    /**
     * @brief Get Probability.
     * @param[in,out] logits Input/output parameter.
     * @param[in] token Input parameter.
     * @param[in] n_vocab Input parameter.
     * @return Return value.
     */
    float getProbability(float* logits, llama_token token, int32_t n_vocab);
    /**
     * @brief Synchronize Draft To Target.
     * @param[in] accepted_tokens Input parameter.
     */
    void synchronizeDraftToTarget(const std::vector<llama_token>& accepted_tokens);
    
    // Vision support helpers
#ifdef THEMIS_ENABLE_VISION
    /**
     * @brief Initialize Vision Encoder.
     * @return True when the operation succeeds.
     */
    bool initializeVisionEncoder();
    /**
     * @brief Shutdown Vision Encoder.
     */
    void shutdownVisionEncoder();
    /**
     * @brief Build Vision Prompt.
     * @param[in] request Input parameter.
     * @return Return value.
     */
    std::string buildVisionPrompt(const VisionRequest& request);
#endif
    
    /**
     * @brief Internal llama.
     * @param[in,out] model Input/output parameter.
     * @param[in] text Input parameter.
     * @param[in] add_bos Input parameter.
     * @return Return value.
     * @details cpp helper functions
     */
    std::vector<llama_token> tokenizeInternal(
        llama_model* model,
        const std::string& text,
        bool add_bos
    );
    
    /**
     * @brief Detokenize Internal.
     * @param[in,out] ctx Input/output parameter.
     * @param[in] tokens Input parameter.
     * @return Return value.
     */
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
    
    /**
     * @brief Chat formatting helpers (implementation details)
     * @param[in] messages Input parameter.
     * @return Return value.
     */
    std::string formatChatML(const std::vector<ChatMessage>& messages);
    /**
     * @brief Format Llama2.
     * @param[in] messages Input parameter.
     * @return Return value.
     */
    std::string formatLlama2(const std::vector<ChatMessage>& messages);
    /**
     * @brief Format Vicuna.
     * @param[in] messages Input parameter.
     * @return Return value.
     */
    std::string formatVicuna(const std::vector<ChatMessage>& messages);
    /**
     * @brief Format Alpaca.
     * @param[in] messages Input parameter.
     * @return Return value.
     */
    std::string formatAlpaca(const std::vector<ChatMessage>& messages);
    
public:
    /**
     * @brief ═══════════════════════════════════════════════════════════ Output Formatting Helpers (MCP, SSE, AQL) ═══════════════════════════════════════════════════════════
     * @param[in] response Input parameter.
     * @return Return value.
     */
    
    static json formatAsMCPResponse(const InferenceResponse& response);
    
    /**
     * @brief Format As SSE.
     * @param[in] response Input parameter.
     * @return Return value.
     */
    static std::string formatAsSSE(const InferenceResponse& response);
    
    /**
     * @brief Format As Json Markdown.
     * @param[in] response Input parameter.
     * @return Return value.
     */
    static json formatAsJsonMarkdown(const InferenceResponse& response);
    
    static std::string formatStreamTokenAsSSE(const std::string& token, const std::string& request_id = "");
};

} // namespace llm
} // namespace themis
