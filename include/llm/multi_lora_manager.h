/**
 * @file multi_lora_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 96/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "llm/llm_plugin_interface.h"
#include "llm/lora_security_validator.h"

#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <atomic>
#include <condition_variable>

#include <nlohmann/json.hpp>

struct llama_context;

namespace themis {
namespace llm {

using json = nlohmann::json;

enum class MultiGPUStrategy {
    NONE = 0,           // Single GPU (default)
    ROUND_ROBIN = 1,    // Distribute LoRAs evenly across GPUs
    DATA_PARALLEL = 2,  // Replicate adapter on all GPUs
    MODEL_PARALLEL = 3  // Split large adapter across GPUs
};

enum class GPUPlacement {
    SINGLE_GPU = 0,     // LoRA on single GPU
    MULTI_GPU = 1       // LoRA spans multiple GPUs
};

struct MultiGPUConfig {
    bool enabled = false;
    std::vector<int> devices;                    // GPU device IDs to use (e.g., {0, 1, 2, 3})
    MultiGPUStrategy strategy = MultiGPUStrategy::ROUND_ROBIN;
    bool enable_peer_transfer = false;           // GPUDirect P2P
    // FIND-015: Use named constant for default max VRAM per GPU
    static constexpr size_t DEFAULT_MAX_VRAM_PER_GPU_MB = 24 * 1024;  // 24GB default
    size_t max_vram_per_gpu_mb = DEFAULT_MAX_VRAM_PER_GPU_MB;         // Max VRAM per GPU
    
    // Load balancing
    bool enable_load_balancing = true;
    float load_balance_threshold = 0.8f;         // Rebalance when GPU usage > 80%
    
    // Fault tolerance
    bool enable_fault_tolerance = true;
    int health_check_interval_sec = 30;
};

enum class QuantizationMode {
    NONE = 0,    // No quantization (FP32/FP16)
    INT8 = 1,    // 8-bit integer quantization (4× compression)
    INT4 = 2     // 4-bit integer quantization (8× compression)
};

struct LoRAQuantizationConfig {
    bool enabled = false;
    QuantizationMode mode = QuantizationMode::INT8;
    
    // Calibration parameters
    int calibration_samples = 100;       // Number of samples for scale calibration
    
    // Quantization strategy
    bool per_channel = true;             // Per-channel vs per-tensor scaling
    int group_size = 128;                // For INT4 grouping (0 = per-channel)
};

struct QuantizationStats {
    /**
     * @brief Quantization Stats.
     * @return Return value.
     */
    virtual ~QuantizationStats() = default;
    std::string lora_id;
    QuantizationMode mode = QuantizationMode::NONE;
    
    size_t original_bytes = 0;           // Original FP32 size
    size_t quantized_bytes = 0;          // Quantized size
    float compression_ratio = 1.0f;      // original_bytes / quantized_bytes
    
    float quantization_time_ms = 0.0f;   // Time to quantize
    float calibration_time_ms = 0.0f;    // Time for calibration
    
    // Per-channel statistics
    size_t num_channels = 0;
    float min_scale = 0.0f;              // Minimum scale factor
    float max_scale = 0.0f;              // Maximum scale factor
    float avg_scale = 0.0f;              // Average scale factor
};

enum class FusionStrategy {
    STATIC = 0,        // Fixed weights, cached permanently
    DYNAMIC = 1,       // Runtime adjustable weights
    SCHEDULED = 2      // Time-varying weights (A/B testing, smooth transitions)
};

enum class SchedulingStrategy {
    LINEAR = 0,        // Linear interpolation between weights
    EXPONENTIAL = 1,   // Exponential decay/growth between weights
    STEP_WISE = 2,     // Step-wise discrete transitions
    CUSTOM = 3         // User-defined custom schedule function
};

struct AlphaSchedule {
    /**
     * @brief Alpha Schedule.
     * @return Return value.
     */
    virtual ~AlphaSchedule() = default;
    std::string schedule_id;
    FusionStrategy strategy = FusionStrategy::STATIC;
    SchedulingStrategy scheduling_strategy = SchedulingStrategy::LINEAR;
    
    // Static weights (for STATIC strategy)
    std::vector<float> static_weights;
    
    // Target weights (for SCHEDULED strategy transitions)
    std::vector<float> target_weights;
    
    // Dynamic scheduling parameters
    std::chrono::system_clock::time_point start_time;
    std::chrono::seconds transition_duration{0};  // For smooth transitions
    
    // Exponential scheduling parameters
    float exponential_base = 2.0f;  // Controls curve steepness (higher = faster/slower transition)
    bool exponential_decay = true;  // true for decay, false for growth
    
    // Step-wise scheduling parameters
    std::vector<double> step_times;         // Time points (seconds) for step transitions
    std::vector<std::vector<float>> step_weights;  // Weight vectors at each step
    
    // Function pointer for custom scheduling logic
    // Returns weights vector based on current time offset (seconds since start)
    using ScheduleFunc = std::function<std::vector<float>(double time_offset)>;
    ScheduleFunc schedule_func;
    
    // A/B testing parameters (for backward compatibility)
    float a_weight = 0.5f;  // Weight for adapter A in A/B test
    float b_weight = 0.5f;  // Weight for adapter B in A/B test
    
    // Performance tracking for adaptive scheduling
    size_t total_requests = 0;
    std::vector<double> performance_scores;  // Per-adapter performance
};

struct FusionConfig {
    FusionStrategy strategy = FusionStrategy::STATIC;
    std::vector<std::string> source_lora_ids;
    std::vector<float> weights;
    
    // Caching behavior
    bool enable_cache = true;
    std::chrono::seconds cache_ttl{3600};  // 1 hour default
    
    // Compatibility checks
    bool enforce_quantization_match = true;
    bool enforce_gpu_placement_match = false;
    bool enforce_rank_match = false;
    
    // Alpha scheduling (for DYNAMIC/SCHEDULED strategies)
    AlphaSchedule alpha_schedule;
};

struct FusionCacheEntry {
    /**
     * @brief Fusion Cache Entry.
     * @return Return value.
     */
    virtual ~FusionCacheEntry() = default;
    std::string fusion_id;
    std::vector<std::string> source_lora_ids;
    std::vector<float> weights;
    
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point last_used;
    size_t use_count = 0;
    
    FusionStrategy strategy = FusionStrategy::STATIC;
    
    // Performance metrics
    double avg_inference_time_ms = 0.0;
    size_t inference_count = 0;
};

struct FusionMetrics {
    /**
     * @brief Fusion Metrics.
     * @return Return value.
     */
    virtual ~FusionMetrics() = default;
    std::string fusion_id;
    FusionStrategy strategy;
    
    size_t total_fusions = 0;
    size_t cache_hits = 0;
    size_t cache_misses = 0;
    size_t invalidations = 0;
    
    double avg_fusion_time_ms = 0.0;
    double avg_inference_time_ms = 0.0;
    
    // Per-strategy breakdown
    std::map<FusionStrategy, size_t> fusions_by_strategy;
    std::map<FusionStrategy, double> avg_time_by_strategy;
};

struct LoRASlot {
    /**
     * @brief Lo RASlot.
     * @return Return value.
     */
    virtual ~LoRASlot() = default;
    std::string lora_id;
    std::string path;
    std::string base_model_id;
    
    void* adapter_handle = nullptr;     // Opaque LoRA handle
    float scale = 1.0f;
    
    size_t vram_bytes = 0;
    size_t rank = 0;                    // LoRA rank (r)
    size_t alpha = 0;                   // LoRA alpha
    
    std::chrono::system_clock::time_point loaded_at;
    std::chrono::system_clock::time_point last_used;
    size_t use_count = 0;
    
    bool is_active = false;             // Currently applied to model
    bool keep_loaded = false;           // Pin in memory
    
    // Quantization support (v1.4.0)
    bool is_quantized = false;
    QuantizationMode quantization_mode = QuantizationMode::NONE;
    size_t original_vram_bytes = 0;     // Original size before quantization
    std::vector<float> scale_factors;   // Per-channel scale factors
    std::vector<uint8_t> quantized_weights;  // Quantized weight data
    
    // Multi-GPU support (v1.4.0)
    GPUPlacement gpu_placement = GPUPlacement::SINGLE_GPU;
    std::vector<int> assigned_gpus;     // GPU device IDs where this LoRA is loaded
    int primary_gpu = 0;                 // Primary GPU for single-GPU or coordinator for multi-GPU
    
    // Security and audit (v1.5.0)
    std::string tenant_id;               // Tenant identifier for isolation
    bool is_replicated = false;          // True if replicated across multiple nodes/GPUs for HA
};

class MultiLoRAManager {
public:
    struct Config {
        // Memory limits
        size_t max_lora_vram_mb = 2048;  // 2 GB for all LoRAs
        size_t max_lora_slots = 16;      // Max concurrent LoRAs
        
        // Trusted directory: LoRA files must reside under this path (F1-1/F1-2 fix).
        // Defaults to an empty string which disables the check (legacy behaviour);
        // production deployments must set this to the managed LoRA storage directory.
        std::string lora_base_dir;

        // Cache policy
        std::chrono::seconds lora_ttl{1800};  // 30 min TTL
        bool enable_lazy_load = true;
        
        // Batching (vLLM-style)
        bool enable_multi_lora_batch = false;  // Multiple LoRAs in one batch
        size_t max_loras_per_batch = 4;
        
        // Adapter fusion
        bool enable_adapter_fusion = false;    // Merge multiple LoRAs
        
        // Quantization (v1.4.0)
        LoRAQuantizationConfig quantization;
        
        // Multi-GPU support (v1.4.0)
        MultiGPUConfig multi_gpu;

        // Security validation (v1.20.0): when set, loadLoRAInternal() calls
        // validateMetadata() before any GGUF parse.  Optional: null disables
        // validation (legacy / test deployments).
        std::shared_ptr<LoRASecurityValidator> security_validator;
        bool enforce_security_validation = true;
    };
    
    /**
     * @brief Multi Lo RAManager.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit MultiLoRAManager(const Config& config);
    ~MultiLoRAManager();
    
    MultiLoRAManager(MultiLoRAManager&& other) noexcept;
    
    MultiLoRAManager& operator=(MultiLoRAManager&& other) noexcept;
    
    // Delete copy constructor and assignment (Rule of Five)
    // Resources are not copyable due to unique_ptr and thread ownership
    MultiLoRAManager(const MultiLoRAManager&) = delete;
    MultiLoRAManager& operator=(const MultiLoRAManager&) = delete;
    
    /**
     * @brief Set Quantization Config.
     * @param[in] config Input parameter.
     */
    void setQuantizationConfig(const LoRAQuantizationConfig& config);
    
    /**
     * @brief Get Quantization Config.
     * @return Return value.
     */
    LoRAQuantizationConfig getQuantizationConfig() const;
    
    bool loadLoRA(
        const std::string& lora_id,
        const std::string& lora_path,
        const std::string& base_model_id,
        float scale = 1.0f
    );
    
    bool loadLoRA(
        const std::string& lora_id,
        const std::string& lora_path,
        const std::string& base_model_id,
        bool quantize,
        float scale = 1.0f
    );
    
    bool loadLoRA(
        const std::string& lora_id,
        const std::string& lora_path,
        const std::string& base_model_id,
        bool quantize,
        GPUPlacement placement,
        float scale = 1.0f
    );
    
    /**
     * @brief Initialize Lo RAWith Model.
     * @param[in] lora_id Identifier of the lora.
     * @param[in,out] model Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool initializeLoRAWithModel(const std::string& lora_id, void* model);
    
    bool unloadLoRA(const std::string& lora_id, bool force = false);
    
    /**
     * @brief Get Lo RA.
     * @param[in] lora_id Identifier of the lora.
     * @return Pointer to the result.
     */
    LoRASlot* getLoRA(const std::string& lora_id);
    
    using ApplyAdapterFn = std::function<bool(const LoRASlot& slot)>;
    using RemoveAdapterFn = std::function<bool(const LoRASlot& slot)>;

    /**
     * @brief Set Apply Adapter Fn.
     * @param[in] fn Input parameter.
     */
    void setApplyAdapterFn(ApplyAdapterFn fn);
    /**
     * @brief Set Remove Adapter Fn.
     * @param[in] fn Input parameter.
     */
    void setRemoveAdapterFn(RemoveAdapterFn fn);

    /**
     * @brief Apply Lo RA.
     * @param[in] lora_id Identifier of the lora.
     * @param[in,out] context Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool applyLoRA(const std::string& lora_id, llama_context* context);
    
    /**
     * @brief Remove Lo RA.
     * @param[in] lora_id Identifier of the lora.
     * @param[in,out] context Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool removeLoRA(const std::string& lora_id, llama_context* context);
    
    std::vector<InferenceResponse> batchInferenceMultiLoRA(
        const std::vector<std::pair<InferenceRequest, std::string>>& requests,
        llama_context* model_context
    );
    
    bool fuseLoRAs(
        const std::vector<std::string>& lora_ids,
        const std::string& fused_id,
        const std::vector<float>& weights = {}
    );
    
    /**
     * @brief Fuse Lo RAs Advanced.
     * @param[in] fused_id Identifier of the fused.
     * @param[in] config Input parameter.
     * @return True when the operation succeeds.
     */
    bool fuseLoRAsAdvanced(
        const std::string& fused_id,
        const FusionConfig& config
    );
    
    /**
     * @brief Update Fusion Weights.
     * @param[in] fusion_id Identifier of the fusion.
     * @param[in] new_weights Input parameter.
     * @return True when the operation succeeds.
     */
    bool updateFusionWeights(
        const std::string& fusion_id,
        const std::vector<float>& new_weights
    );
    
    /**
     * @brief Set Alpha Schedule.
     * @param[in] fusion_id Identifier of the fusion.
     * @param[in] schedule Input parameter.
     * @return True when the operation succeeds.
     */
    bool setAlphaSchedule(
        const std::string& fusion_id,
        const AlphaSchedule& schedule
    );
    
    /**
     * @brief Get Current Fusion Weights.
     * @param[in] fusion_id Identifier of the fusion.
     * @return Return value.
     */
    std::vector<float> getCurrentFusionWeights(const std::string& fusion_id) const;
    
    /**
     * @brief Invalidate Fusion Cache.
     * @param[in] fusion_id Identifier of the fusion.
     * @return True when the operation succeeds.
     */
    bool invalidateFusionCache(const std::string& fusion_id);
    
    /**
     * @brief Clear Fusion Cache.
     * @return Return value.
     */
    size_t clearFusionCache();
    
    /**
     * @brief Get Fusion Metrics.
     * @return Return value.
     */
    FusionMetrics getFusionMetrics() const;
    
    /**
     * @brief List Fusion Cache.
     * @return Return value.
     */
    std::vector<FusionCacheEntry> listFusionCache() const;
    
    /**
     * @brief Check Fusion Compatibility.
     * @param[in] lora_ids Input parameter.
     * @param[in] config Input parameter.
     * @return True when the operation succeeds.
     */
    bool checkFusionCompatibility(
        const std::vector<std::string>& lora_ids,
        const FusionConfig& config
    ) const;
    
    /**
     * @brief Pin Lo RA.
     * @param[in] lora_id Identifier of the lora.
     */
    void pinLoRA(const std::string& lora_id);
    
    /**
     * @brief Unpin Lo RA.
     * @param[in] lora_id Identifier of the lora.
     */
    void unpinLoRA(const std::string& lora_id);
    
    /**
     * @brief Is Lo RALoaded.
     * @param[in] lora_id Identifier of the lora.
     * @return True when the operation succeeds.
     */
    bool isLoRALoaded(const std::string& lora_id) const;
    
    /**
     * @brief Get Quantization Stats.
     * @param[in] lora_id Identifier of the lora.
     * @return Return value.
     */
    std::optional<QuantizationStats> getQuantizationStats(const std::string& lora_id) const;
    
    /**
     * @brief Get Multi GPUConfig.
     * @return Return value.
     */
    MultiGPUConfig getMultiGPUConfig() const;
    
    /**
     * @brief Set Multi GPUConfig.
     * @param[in] config Input parameter.
     */
    void setMultiGPUConfig(const MultiGPUConfig& config);
    
    /**
     * @brief Get Lo RAGPUPlacement.
     * @param[in] lora_id Identifier of the lora.
     * @return Return value.
     */
    std::vector<int> getLoRAGPUPlacement(const std::string& lora_id) const;
    
    std::unordered_map<int, size_t> getPerGPUMemoryUsage() const;
    
    /**
     * @brief Balance GPULoad.
     * @return Return value.
     */
    size_t balanceGPULoad();
    
    /**
     * @brief Get Usage Heatmap.
     * @return Return value.
     */
    json getUsageHeatmap() const;
    
    size_t evictResourceAware(int gpu_id = -1, size_t target_vram_mb = 0);
    
    json getSchedulingRecommendation(size_t lora_vram_bytes, int priority = 5) const;
    
    /**
     * @brief Migrate Lo RATo GPU.
     * @param[in] lora_id Identifier of the lora.
     * @param[in] target_gpu Input parameter.
     * @return True when the operation succeeds.
     */
    bool migrateLoRAToGPU(const std::string& lora_id, int target_gpu);
    
    /**
     * @brief Check GPUHealth And Migrate.
     * @return Return value.
     */
    size_t checkGPUHealthAndMigrate();
    
    /**
     * @brief List Lo RAs.
     * @return Return value.
     */
    std::vector<LoRAInfo> listLoRAs() const;
    
    /**
     * @brief Set Lo RATenant.
     * @param[in] lora_id Identifier of the lora.
     * @param[in] tenant_id Identifier of the tenant.
     */
    void setLoRATenant(const std::string& lora_id, const std::string& tenant_id);
    
    json getGPUTransferAuditLog(size_t limit = 100) const;

    /**
     * @brief List Lo RAs.
     * @param[in] base_model_id Identifier of the base model.
     * @return Return value.
     */
    std::vector<LoRAInfo> listLoRAs(const std::string& base_model_id) const;

    /**
     * @brief Get Lo RAInfo.
     * @param[in] lora_id Identifier of the lora.
     * @return Return value.
     */
    std::optional<LoRAInfo> getLoRAInfo(const std::string& lora_id) const;
    
    size_t evictLRU(size_t target_vram_mb = 0);
    
    /**
     * @brief Evict Expired.
     * @return Return value.
     */
    size_t evictExpired();
    
    /**
     * @brief Get Memory Stats.
     * @return Return value.
     */
    json getMemoryStats() const;
    
    /**
     * @brief Get Cache Stats.
     * @return Return value.
     */
    json getCacheStats() const;

    // Compact typed statistics API for tests
    struct Stats {
        size_t total_loras_loaded = 0;
        size_t cache_hits = 0;
        size_t cache_misses = 0;
        size_t evictions = 0;
        size_t switches = 0;
    };

    /**
     * @brief Return access control statistics.
     * @return Access control statistics.
     */
    Stats getStatistics() const;

    // Backward-compat: legacy tests expect getStats()
    Stats getStats() const { return getStatistics(); }
    
    /**
     * @brief Export Lo RA.
     * @param[in] lora_id Identifier of the lora.
     * @return Return value.
     */
    std::vector<uint8_t> exportLoRA(const std::string& lora_id);
    
    /**
     * @brief Import Lo RA.
     * @param[in] lora_id Identifier of the lora.
     * @param[in] data Input parameter.
     * @param[in] base_model_id Identifier of the base model.
     * @return True when the operation succeeds.
     */
    bool importLoRA(
        const std::string& lora_id,
        const std::vector<uint8_t>& data,
        const std::string& base_model_id
    );
    
private:
    Config config_;
    
    std::unordered_map<std::string, std::unique_ptr<LoRASlot>> loras_;
    
    // LOCK HIERARCHY ENFORCEMENT (§3.1):
    // ┌─ adapter_state_lock_ : std::shared_mutex
    // │  └─ adapter_cache_lock_ : std::mutex  (for modifications only)
    // │     └─ metrics_lock_ : std::mutex
    // └─ eviction_cv_ : std::condition_variable (paired with adapter_cache_lock_)
    
    mutable std::shared_mutex adapter_state_lock_;
    
    mutable std::mutex adapter_cache_lock_;

    mutable std::mutex mutex_;
    
    mutable std::mutex metrics_lock_;
    
    std::condition_variable eviction_cv_;
    
    // Statistics (protected by metrics_lock_)
    size_t total_vram_bytes_ = 0;
    size_t cache_hits_ = 0;
    size_t cache_misses_ = 0;
    size_t evictions_ = 0;
    size_t switches_ = 0;                // LoRA switch count
    
    // Multi-GPU state (v1.4.0) (protected by adapter_state_lock_)
    std::unordered_map<int, size_t> gpu_vram_usage_;  // Per-GPU VRAM tracking
    int next_round_robin_gpu_ = 0;                     // Round-robin counter
    
    // Enhanced tracking for v1.5.0 (protected by adapter_state_lock_)
    std::unordered_map<std::string, std::string> lora_tenants_;  // LoRA -> Tenant mapping
    
    // Audit log structure (v1.5.0) (protected by metrics_lock_)
    struct AuditEvent {
        std::chrono::system_clock::time_point timestamp;
        std::string event_type;  // "load", "unload", "migrate", "evict"
        std::string lora_id;
        std::string tenant_id;
        int source_gpu = 0;
        int target_gpu = 0;
        size_t vram_bytes = 0;
        std::string details;
    };
    std::vector<AuditEvent> audit_log_;
    size_t max_audit_log_size_ = 1000;
    
    // GPU health tracking (v1.5.0) (protected by adapter_state_lock_)
    std::unordered_map<int, bool> gpu_health_status_;  // GPU ID -> healthy status
    std::unordered_map<int, std::chrono::system_clock::time_point> gpu_last_health_check_;
    
    void logGPUTransferEvent(const std::string& event_type, const std::string& lora_id,
                             int source_gpu, int target_gpu, size_t vram_bytes,
                             const std::string& details = "");
    
    /**
     * @brief Helper for access frequency calculation
     * @param[in] lora Input parameter.
     * @param[in] now Input parameter.
     * @return Return value.
     */
    double calculateAccessFrequency(const LoRASlot* lora, 
                                   const std::chrono::system_clock::time_point& now) const;
    // Fusion cache and metrics (v1.5.0) (protected by adapter_cache_lock_)
    std::unordered_map<std::string, FusionCacheEntry> fusion_cache_;
    std::unordered_map<std::string, FusionConfig> fusion_configs_;
    std::unordered_map<std::string, AlphaSchedule> fusion_schedules_;
    
    FusionMetrics fusion_metrics_;
    size_t total_fusions_ = 0;
    size_t fusion_cache_hits_ = 0;
    size_t fusion_cache_misses_ = 0;
    size_t fusion_invalidations_ = 0;
    
    // Background eviction thread
    std::unique_ptr<std::thread> eviction_thread_;
    
    std::atomic<bool> eviction_thread_running_{false};
    
    std::atomic<bool> eviction_thread_done_{true};
    
    ApplyAdapterFn apply_adapter_fn_;
    RemoveAdapterFn remove_adapter_fn_;
    
    // Internal helpers
    LoRASlot* loadLoRAInternal(
        const std::string& lora_id,
        const std::string& lora_path,
        const std::string& base_model_id,
        float scale,
        bool quantize = false,
        GPUPlacement placement = GPUPlacement::SINGLE_GPU
    );

    /**
     * @brief Is Lo RAPath Trusted.
     * @param[in] lora_path Path to the lora.
     * @return True when the operation succeeds.
     */
    bool isLoRAPathTrusted(const std::string& lora_path) const;
    
    // Background eviction worker
    /**
     * @brief Eviction Worker.
     */
    void evictionWorker();
    /**
     * @brief Start Eviction Thread.
     */
    void startEvictionThread();
    /**
     * @brief Stop Eviction Thread.
     */
    void stopEvictionThread();
    
    /**
     * @brief Multi-GPU helpers (v1.
     * @param[in] vram_bytes Input parameter.
     * @return Return value.
     * @details 4.0)
     */
    int selectGPUForLoRA(size_t vram_bytes);  // Select best GPU for new LoRA
    /**
     * @brief Load Lo RAOn GPU.
     * @param[in,out] lora Input/output parameter.
     * @param[in] gpu_id Identifier of the gpu.
     * @return True when the operation succeeds.
     */
    bool loadLoRAOnGPU(LoRASlot* lora, int gpu_id);  // Load LoRA on specific GPU
    /**
     * @brief Load Lo RAMulti GPU.
     * @param[in,out] lora Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool loadLoRAMultiGPU(LoRASlot* lora);  // Load LoRA across multiple GPUs
    /**
     * @brief Update GPUMemory Tracking.
     */
    void updateGPUMemoryTracking();  // Recalculate per-GPU memory usage
    /**
     * @brief Is GPUHealthy.
     * @param[in] gpu_id Identifier of the gpu.
     * @return True when the operation succeeds.
     */
    bool isGPUHealthy(int gpu_id) const;  // Check GPU health status
    /**
     * @brief Get Available GPUs.
     * @return Return value.
     */
    std::vector<int> getAvailableGPUs() const;  // Get list of available GPUs
    
    // Quantization helpers
    /**
     * @brief Quantize Lo RA.
     * @param[in,out] lora Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool quantizeLoRA(LoRASlot* lora);
    /**
     * @brief Quantize INT8.
     * @param[in,out] lora Input/output parameter.
     * @param[in] weights Input parameter.
     */
    void quantizeINT8(LoRASlot* lora, const std::vector<float>& weights);
    /**
     * @brief Quantize INT4.
     * @param[in,out] lora Input/output parameter.
     * @param[in] weights Input parameter.
     */
    void quantizeINT4(LoRASlot* lora, const std::vector<float>& weights);
    /**
     * @brief Calibrate Scales.
     * @param[in] weights Input parameter.
     * @param[in,out] scales Input/output parameter.
     */
    void calibrateScales(const std::vector<float>& weights, std::vector<float>& scales);
    /**
     * @brief Simulate Weights.
     * @param[in] count Input parameter.
     * @return Return value.
     */
    std::vector<float> simulateWeights(size_t count);  // For testing without real weights
    
    /**
     * @brief Fusion helpers (v1.
     * @param[in] fused_id Identifier of the fused.
     * @param[in] config Input parameter.
     * @return True when the operation succeeds.
     * @details 5.0)
     */
    bool fuseLoRAsInternal(const std::string& fused_id, const FusionConfig& config);
    /**
     * @brief Compute Scheduled Weights.
     * @param[in] fusion_id Identifier of the fusion.
     * @return Return value.
     */
    std::vector<float> computeScheduledWeights(const std::string& fusion_id) const;
    /**
     * @brief Compute Linear Schedule.
     * @param[in] schedule Input parameter.
     * @param[in] time_offset Input parameter.
     * @return Return value.
     */
    std::vector<float> computeLinearSchedule(const AlphaSchedule& schedule, double time_offset) const;
    /**
     * @brief Compute Exponential Schedule.
     * @param[in] schedule Input parameter.
     * @param[in] time_offset Input parameter.
     * @return Return value.
     */
    std::vector<float> computeExponentialSchedule(const AlphaSchedule& schedule, double time_offset) const;
    /**
     * @brief Compute Step Wise Schedule.
     * @param[in] schedule Input parameter.
     * @param[in] time_offset Input parameter.
     * @return Return value.
     */
    std::vector<float> computeStepWiseSchedule(const AlphaSchedule& schedule, double time_offset) const;
    /**
     * @brief Validate Fusion Compatibility.
     * @param[in] source_loras Input parameter.
     * @param[in] config Input parameter.
     * @return True when the operation succeeds.
     */
    bool validateFusionCompatibility(
        const std::vector<LoRASlot*>& source_loras,
        const FusionConfig& config
    ) const;
    /**
     * @brief Update Fusion Metrics.
     * @param[in] fusion_id Identifier of the fusion.
     * @param[in] fusion_time_ms Input parameter.
     */
    void updateFusionMetrics(const std::string& fusion_id, double fusion_time_ms);
    /**
     * @brief Update Inference Metrics.
     * @param[in] fusion_id Identifier of the fusion.
     * @param[in] inference_time_ms Input parameter.
     */
    void updateInferenceMetrics(const std::string& fusion_id, double inference_time_ms);
    
    /**
     * @brief Has Capacity.
     * @param[in] vram_bytes Input parameter.
     * @return True when the operation succeeds.
     */
    bool hasCapacity(size_t vram_bytes) const;
    /**
     * @brief Update Memory Usage.
     */
    void updateMemoryUsage();
};

} // namespace llm
} // namespace themis

