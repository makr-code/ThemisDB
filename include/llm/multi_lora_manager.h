/**
 * @file multi_lora_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 96/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Multi-GPU placement strategy for LoRA adapters (v1.4.0)
 */
enum class MultiGPUStrategy {
    NONE = 0,           // Single GPU (default)
    ROUND_ROBIN = 1,    // Distribute LoRAs evenly across GPUs
    DATA_PARALLEL = 2,  // Replicate adapter on all GPUs
    MODEL_PARALLEL = 3  // Split large adapter across GPUs
};

/**
 * @brief GPU placement configuration for a LoRA adapter (v1.4.0)
 */
enum class GPUPlacement {
    SINGLE_GPU = 0,     // LoRA on single GPU
    MULTI_GPU = 1       // LoRA spans multiple GPUs
};

/**
 * @brief Multi-GPU configuration for LoRA adapters (v1.4.0)
 */
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

/**
 * @brief Quantization mode for LoRA adapters
 */
enum class QuantizationMode {
    NONE = 0,    // No quantization (FP32/FP16)
    INT8 = 1,    // 8-bit integer quantization (4× compression)
    INT4 = 2     // 4-bit integer quantization (8× compression)
};

/**
 * @brief LoRA quantization configuration
 */
struct LoRAQuantizationConfig {
    bool enabled = false;
    QuantizationMode mode = QuantizationMode::INT8;
    
    // Calibration parameters
    int calibration_samples = 100;       // Number of samples for scale calibration
    
    // Quantization strategy
    bool per_channel = true;             // Per-channel vs per-tensor scaling
    int group_size = 128;                // For INT4 grouping (0 = per-channel)
};

/**
 * @brief Quantization statistics for a LoRA adapter
 */
struct QuantizationStats {
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

/**
 * @brief Fusion strategy for combining multiple LoRA adapters
 */
enum class FusionStrategy {
    STATIC = 0,        // Fixed weights, cached permanently
    DYNAMIC = 1,       // Runtime adjustable weights
    SCHEDULED = 2      // Time-varying weights (A/B testing, smooth transitions)
};

/**
 * @brief Scheduling strategy for SCHEDULED fusion mode
 */
enum class SchedulingStrategy {
    LINEAR = 0,        // Linear interpolation between weights
    EXPONENTIAL = 1,   // Exponential decay/growth between weights
    STEP_WISE = 2,     // Step-wise discrete transitions
    CUSTOM = 3         // User-defined custom schedule function
};

/**
 * @brief Alpha scheduling function for dynamic fusion
 * 
 * Allows runtime computation of blend weights based on various factors:
 * - Time-based: gradual transition between adapters
 * - Performance-based: A/B testing with feedback
 * - Context-based: different weights per request type
 */
struct AlphaSchedule {
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

/**
 * @brief Configuration for LoRA fusion operation
 */
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

/**
 * @brief Fusion cache entry metadata
 */
struct FusionCacheEntry {
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

/**
 * @brief Fusion performance metrics
 */
struct FusionMetrics {
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

/**
 * @brief LoRA adapter slot
 * 
 * Represents a loaded LoRA adapter with its metadata and handle.
 */
struct LoRASlot {
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

/**
 * @brief Multi-LoRA Manager (vLLM-inspired)
 * 
 * Manages multiple LoRA adapters for a single base model.
 * Supports efficient switching between adapters and even batched
 * inference with different adapters per request.
 * 
 * Example workflow:
 * 1. Load base model (mistral-7b)
 * 2. Load multiple LoRAs: legal-qa, medical-diagnosis, code-assistant
 * 3. Request 1: Use legal-qa adapter
 * 4. Request 2: Use medical-diagnosis adapter
 * 5. Both can be in the same inference batch (if backend supports it)
 * 
 * LOCK HIERARCHY (always acquire in this order to prevent deadlocks):
 * 1. adapter_state_lock_ → Top-level shared state (read-write for queries)
 *    └─ adapter_cache_lock_ → Adapter cache modifications (exclusive)
 *       └─ metrics_lock_ → Telemetry updates (exclusive)
 *
 * Memory Ordering:
 * - eviction_thread_running_: std::memory_order_acquire/release
 * - eviction_thread_done_: std::memory_order_acquire/release
 * 
 * Thread Safety:
 * - All public methods are thread-safe
 * - Shared state (loras_, gpu_vram_usage_, fusion_cache_) protected by locks
 * - No circular lock dependencies enforced by documentation
 */
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
        /// When true and security_validator is set, a metadata-validation
        /// failure causes loadLoRA() to reject the adapter hard.
        /// When false the failure is logged as a warning and loading continues.
        bool enforce_security_validation = true;
    };
    
    explicit MultiLoRAManager(const Config& config);
    ~MultiLoRAManager();
    
    /**
     * @brief Move constructor for resource transfer
     * 
     * Transfers ownership of internal resources (threads, GPU state) from other
     * to this object. The source object is left in a valid empty state.
     * 
     * @param other Source object to move from (left in valid empty state)
     * @note Marked noexcept: move operations don't throw
     * @cwe CWE-457 (Uninitialized Variable) prevention: ensures moved-from state is valid
     */
    MultiLoRAManager(MultiLoRAManager&& other) noexcept;
    
    /**
     * @brief Move assignment operator for resource transfer
     * 
     * Transfers ownership of internal resources from other to this object.
     * Cleans up existing resources before transfer. Self-assignment safe.
     * 
     * @param other Source object to move from
     * @return Reference to this object
     * @note Marked noexcept: move operations don't throw
     * @cwe CWE-415 (Double Free) prevention: proper cleanup before reassignment
     * @cwe CWE-672 (Use After Free) prevention: source left in valid state
     */
    MultiLoRAManager& operator=(MultiLoRAManager&& other) noexcept;
    
    // Delete copy constructor and assignment (Rule of Five)
    // Resources are not copyable due to unique_ptr and thread ownership
    MultiLoRAManager(const MultiLoRAManager&) = delete;
    MultiLoRAManager& operator=(const MultiLoRAManager&) = delete;
    
    /**
     * @brief Set quantization configuration
     * 
     * Configures quantization parameters for subsequently loaded LoRAs.
     * Does not affect already-loaded LoRAs.
     * 
     * @param config Quantization configuration
     */
    void setQuantizationConfig(const LoRAQuantizationConfig& config);
    
    /**
     * @brief Get quantization configuration
     * 
     * @return Current quantization configuration
     */
    LoRAQuantizationConfig getQuantizationConfig() const;
    
    /**
     * @brief Load a LoRA adapter (lazy loading)
     * 
     * If adapter is already loaded, returns immediately.
     * Otherwise, loads it on-demand.
     * 
     * Thread-safe.
     * 
     * @param lora_id Unique LoRA identifier
     * @param lora_path Path to LoRA weights file
     * @param base_model_id Compatible base model
     * @param scale LoRA scaling factor (default: 1.0)
     * @return true if loaded successfully
     */
    bool loadLoRA(
        const std::string& lora_id,
        const std::string& lora_path,
        const std::string& base_model_id,
        float scale = 1.0f
    );
    
    /**
     * @brief Load a LoRA adapter with optional quantization
     * 
     * Loads a LoRA adapter and optionally applies quantization.
     * 
     * @param lora_id Unique LoRA identifier
     * @param lora_path Path to LoRA weights file
     * @param base_model_id Compatible base model
     * @param quantize Whether to apply quantization (uses current config)
     * @param scale LoRA scaling factor (default: 1.0)
     * @return true if loaded successfully
     */
    bool loadLoRA(
        const std::string& lora_id,
        const std::string& lora_path,
        const std::string& base_model_id,
        bool quantize,
        float scale = 1.0f
    );
    
    /**
     * @brief Load a LoRA adapter with multi-GPU placement (v1.4.0)
     * 
     * Loads a LoRA adapter with explicit GPU placement control.
     * 
     * @param lora_id Unique LoRA identifier
     * @param lora_path Path to LoRA weights file
     * @param base_model_id Compatible base model
     * @param quantize Whether to apply quantization
     * @param placement GPU placement strategy (SINGLE_GPU or MULTI_GPU)
     * @param scale LoRA scaling factor (default: 1.0)
     * @return true if loaded successfully
     */
    bool loadLoRA(
        const std::string& lora_id,
        const std::string& lora_path,
        const std::string& base_model_id,
        bool quantize,
        GPUPlacement placement,
        float scale = 1.0f
    );
    
    /**
     * @brief Initialize LoRA adapter with llama.cpp model handle
     * 
     * This method actually loads the LoRA adapter weights using llama.cpp's
     * llama_lora_adapter_init() API. Must be called with a valid model handle
     * before the LoRA can be applied to contexts.
     * 
     * @param lora_id LoRA identifier (must already exist in loras_ map)
     * @param model llama_model handle for loading the adapter
     * @return true if initialization successful
     */
    bool initializeLoRAWithModel(const std::string& lora_id, void* model);
    
    /**
     * @brief Unload a LoRA adapter
     * 
     * @param lora_id LoRA to unload
     * @param force If true, unload even if pinned
     */
    bool unloadLoRA(const std::string& lora_id, bool force = false);
    
    /**
     * @brief Get LoRA slot (if loaded)
     * 
     * Returns pointer to loaded LoRA slot, or nullptr if not loaded.
     * Updates last_used timestamp.
     */
    LoRASlot* getLoRA(const std::string& lora_id);
    
    /**
     * @brief Apply LoRA to model context
     * 
     * Activates a specific LoRA adapter for the next inference.
     * Multiple LoRAs can be active simultaneously if backend supports it.
     * 
     * @param lora_id LoRA to activate
     * @param context Model context to apply to (llama_context*)
     * @return true if applied successfully
     */
    /// Bridge callback for applying a LoRA adapter when no llama_context is available.
    using ApplyAdapterFn = std::function<bool(const LoRASlot& slot)>;
    /// Bridge callback for removing a LoRA adapter when no llama_context is available.
    using RemoveAdapterFn = std::function<bool(const LoRASlot& slot)>;

    void setApplyAdapterFn(ApplyAdapterFn fn);
    void setRemoveAdapterFn(RemoveAdapterFn fn);

    bool applyLoRA(const std::string& lora_id, llama_context* context);
    
    /**
     * @brief Remove LoRA from model context
     * 
     * Deactivates a LoRA adapter.
     */
    bool removeLoRA(const std::string& lora_id, llama_context* context);
    
    /**
     * @brief Batch inference with multiple LoRAs (vLLM-style)
     * 
     * Processes multiple inference requests, each with its own LoRA adapter,
     * in a single batch for efficiency.
     * 
     * Requires backend support for multi-LoRA batching.
     * 
     * @param requests Vector of inference requests with LoRA IDs
     * @return Vector of responses (same order as requests)
     */
    std::vector<InferenceResponse> batchInferenceMultiLoRA(
        const std::vector<std::pair<InferenceRequest, std::string>>& requests,
        llama_context* model_context
    );
    
    /**
     * @brief Fuse multiple LoRAs into a single adapter
     * 
     * Combines multiple LoRA adapters into one for efficiency.
     * Useful when always using the same combination of adapters.
     * 
     * @param lora_ids LoRAs to fuse
     * @param fused_id New identifier for fused adapter
     * @param weights Weights for each LoRA (default: equal)
     * @return true if fusion successful
     */
    bool fuseLoRAs(
        const std::vector<std::string>& lora_ids,
        const std::string& fused_id,
        const std::vector<float>& weights = {}
    );
    
    /**
     * @brief Fuse multiple LoRAs with advanced configuration
     * 
     * Extended fusion API with dynamic composition, alpha scheduling,
     * and fine-grained control over caching and compatibility checks.
     * 
     * @param fused_id New identifier for fused adapter
     * @param config Fusion configuration including strategy and scheduling
     * @return true if fusion successful
     */
    bool fuseLoRAsAdvanced(
        const std::string& fused_id,
        const FusionConfig& config
    );
    
    /**
     * @brief Update weights for a dynamically fused adapter
     * 
     * Allows runtime adjustment of blend weights for DYNAMIC fusion strategy.
     * The fused adapter must have been created with FusionStrategy::DYNAMIC.
     * 
     * @param fusion_id ID of the fused adapter
     * @param new_weights New blend weights (must match number of source LoRAs)
     * @return true if weights updated successfully
     */
    bool updateFusionWeights(
        const std::string& fusion_id,
        const std::vector<float>& new_weights
    );
    
    /**
     * @brief Set alpha schedule for scheduled fusion
     * 
     * Configures time-varying weights for SCHEDULED fusion strategy.
     * Used for A/B testing, gradual transitions, and adaptive blending.
     * 
     * @param fusion_id ID of the fused adapter
     * @param schedule Alpha scheduling configuration
     * @return true if schedule set successfully
     */
    bool setAlphaSchedule(
        const std::string& fusion_id,
        const AlphaSchedule& schedule
    );
    
    /**
     * @brief Get current fusion weights (resolves scheduled weights)
     * 
     * Returns the current effective weights for a fused adapter,
     * accounting for any active alpha scheduling.
     * 
     * @param fusion_id ID of the fused adapter
     * @return Current weights, or empty vector if fusion not found
     */
    std::vector<float> getCurrentFusionWeights(const std::string& fusion_id) const;
    
    /**
     * @brief Invalidate fusion cache entry
     * 
     * Forces re-computation of a fused adapter on next use.
     * Useful when source LoRAs have been modified or reloaded.
     * 
     * @param fusion_id ID of the fused adapter to invalidate
     * @return true if cache entry was invalidated
     */
    bool invalidateFusionCache(const std::string& fusion_id);
    
    /**
     * @brief Clear all fusion cache entries
     * 
     * Removes all cached fused adapters. Source LoRAs remain loaded.
     * 
     * @return Number of cache entries cleared
     */
    size_t clearFusionCache();
    
    /**
     * @brief Get fusion cache statistics
     * 
     * Returns metrics about fusion caching and performance.
     * 
     * @return Fusion metrics including cache hit rate and timing
     */
    FusionMetrics getFusionMetrics() const;
    
    /**
     * @brief List all cached fusion entries
     * 
     * @return Vector of fusion cache entry metadata
     */
    std::vector<FusionCacheEntry> listFusionCache() const;
    
    /**
     * @brief Check compatibility of LoRAs for fusion
     * 
     * Validates that a set of LoRAs can be safely fused together
     * based on quantization mode, base model, rank, and GPU placement.
     * 
     * @param lora_ids LoRAs to check for compatibility
     * @param config Fusion configuration with compatibility requirements
     * @return true if all LoRAs are compatible for fusion
     */
    bool checkFusionCompatibility(
        const std::vector<std::string>& lora_ids,
        const FusionConfig& config
    ) const;
    
    /**
     * @brief Pin a LoRA in memory (prevent eviction)
     */
    void pinLoRA(const std::string& lora_id);
    
    /**
     * @brief Unpin a LoRA (allow eviction)
     */
    void unpinLoRA(const std::string& lora_id);
    
    /**
     * @brief Check if LoRA is loaded
     */
    bool isLoRALoaded(const std::string& lora_id) const;
    
    /**
     * @brief Get quantization statistics for a LoRA adapter
     * 
     * Returns statistics about quantization for a specific LoRA,
     * including compression ratio and memory savings.
     * 
     * @param lora_id LoRA identifier
     * @return Quantization statistics, or nullopt if LoRA not loaded or not quantized
     */
    std::optional<QuantizationStats> getQuantizationStats(const std::string& lora_id) const;
    
    /**
     * @brief Get multi-GPU configuration (v1.4.0)
     * 
     * @return Current multi-GPU configuration
     */
    MultiGPUConfig getMultiGPUConfig() const;
    
    /**
     * @brief Set multi-GPU configuration (v1.4.0)
     * 
     * Updates multi-GPU configuration. Affects subsequently loaded LoRAs.
     * 
     * @param config Multi-GPU configuration
     */
    void setMultiGPUConfig(const MultiGPUConfig& config);
    
    /**
     * @brief Get GPU placement for a LoRA adapter (v1.4.0)
     * 
     * @param lora_id LoRA identifier
     * @return GPU device IDs where the LoRA is placed, empty if not loaded
     */
    std::vector<int> getLoRAGPUPlacement(const std::string& lora_id) const;
    
    /**
     * @brief Get per-GPU memory statistics (v1.4.0)
     * 
     * @return Map of GPU ID to VRAM usage in bytes
     */
    std::unordered_map<int, size_t> getPerGPUMemoryUsage() const;
    
    /**
     * @brief Balance LoRA load across GPUs (v1.4.0)
     * 
     * Redistributes LoRAs across GPUs for better load balancing.
     * Only effective when multi-GPU is enabled with ROUND_ROBIN strategy.
     * 
     * @return Number of LoRAs moved
     */
    size_t balanceGPULoad();
    
    /**
     * @brief Get usage heatmap for all LoRAs (v1.5.0)
     * 
     * Returns a heatmap showing access patterns and usage statistics
     * for resource-aware eviction decisions.
     * 
     * @return Map of LoRA ID to usage metrics (access count, last used, etc.)
     */
    json getUsageHeatmap() const;
    
    /**
     * @brief Resource-aware eviction based on GPU VRAM pressure (v1.5.0)
     * 
     * Evicts LoRAs based on GPU-specific resource constraints, usage patterns,
     * and priority. More intelligent than simple LRU eviction.
     * 
     * @param gpu_id GPU device to free memory on (-1 for global)
     * @param target_vram_mb Target VRAM to free
     * @return Amount of VRAM freed (MB)
     */
    size_t evictResourceAware(int gpu_id = -1, size_t target_vram_mb = 0);
    
    /**
     * @brief Get scheduling recommendations for LoRA placement (v1.5.0)
     * 
     * Provides intelligent placement recommendations based on available
     * slots, VRAM, expected latency, and current GPU loads.
     * 
     * @param lora_vram_bytes Expected VRAM usage of LoRA
     * @param priority Priority level (0-10)
     * @return Recommended GPU ID and placement metrics
     */
    json getSchedulingRecommendation(size_t lora_vram_bytes, int priority = 5) const;
    
    /**
     * @brief Migrate LoRA adapter to another GPU (v1.5.0)
     * 
     * Performs warm migration of a LoRA adapter from current GPU to target GPU
     * with minimal service interruption.
     * 
     * @param lora_id LoRA to migrate
     * @param target_gpu Target GPU device ID
     * @return true if migration successful
     */
    bool migrateLoRAToGPU(const std::string& lora_id, int target_gpu);
    
    /**
     * @brief Check GPU health and trigger auto-migration on failure (v1.5.0)
     * 
     * Monitors GPU health and automatically migrates adapters from
     * unhealthy GPUs to healthy ones.
     * 
     * @return Number of adapters migrated due to GPU failures
     */
    size_t checkGPUHealthAndMigrate();
    
    /**
     * @brief List all loaded LoRAs
     */
    std::vector<LoRAInfo> listLoRAs() const;
    
    /**
     * @brief Set tenant ID for a LoRA adapter (v1.5.0 - Security)
     * 
     * Associates a LoRA adapter with a specific tenant for
     * GPU memory isolation and audit logging.
     * 
     * @param lora_id LoRA identifier
     * @param tenant_id Tenant identifier
     */
    void setLoRATenant(const std::string& lora_id, const std::string& tenant_id);
    
    /**
     * @brief Get audit log for GPU transfer events (v1.5.0 - Security)
     * 
     * Returns audit log of all GPU transfer events including LoRA
     * migrations, load/unload operations with timestamps and tenant info.
     * 
     * @param limit Maximum number of recent events to return (0 = all)
     * @return JSON array of audit events
     */
    json getGPUTransferAuditLog(size_t limit = 100) const;

    /**
     * @brief List loaded LoRAs filtered by base model id
     */
    std::vector<LoRAInfo> listLoRAs(const std::string& base_model_id) const;

    /**
     * @brief Get LoRA info by id
     */
    std::optional<LoRAInfo> getLoRAInfo(const std::string& lora_id) const;
    
    /**
     * @brief Evict least recently used LoRA(s)
     * 
     * @param target_vram_mb Target VRAM to free
     * @return Amount of VRAM freed (MB)
     */
    size_t evictLRU(size_t target_vram_mb = 0);
    
    /**
     * @brief Evict LoRAs that exceeded their TTL
     * 
     * @return Number of LoRAs evicted
     */
    size_t evictExpired();
    
    /**
     * @brief Get memory usage statistics
     */
    json getMemoryStats() const;
    
    /**
     * @brief Get LoRA cache statistics
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

    Stats getStatistics() const;

    // Backward-compat: legacy tests expect getStats()
    Stats getStats() const { return getStatistics(); }
    
    /**
     * @brief Export LoRA for cross-shard transfer
     * 
     * Serializes a LoRA adapter for transfer to another shard.
     */
    std::vector<uint8_t> exportLoRA(const std::string& lora_id);
    
    /**
     * @brief Import LoRA from another shard
     * 
     * Deserializes and loads a LoRA adapter received from another shard.
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
    
    /// Read-write lock for adapter state queries (many readers, few writers)
    mutable std::shared_mutex adapter_state_lock_;
    
    /// Exclusive lock for adapter cache modifications (loading/unloading)
    mutable std::mutex adapter_cache_lock_;
    
    /// Exclusive lock for telemetry updates (statistics)
    mutable std::mutex metrics_lock_;
    
    /// Condition variable for eviction thread signaling (paired with adapter_cache_lock_)
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
    
    // Helper for access frequency calculation
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
    
    /// Atomic flag: eviction thread is running (memory_order_acquire/release)
    std::atomic<bool> eviction_thread_running_{false};
    
    /// @brief Set to true when the eviction thread has fully exited.
    /// Used by stopEvictionThread() to implement a timed join (W1-L01 no_timeout fix).
    /// Protected by memory_order_acquire/release for thread synchronization.
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
     * @brief Verify that @p lora_path is contained within the trusted
     *        @c config_.lora_base_dir directory (F1-1/F1-2 fix).
     *
     * Returns true when the check passes or when @c config_.lora_base_dir is
     * empty (legacy/unconfigured deployments).  Returns false when the path
     * escapes the base directory; callers must reject the request in that case.
     */
    bool isLoRAPathTrusted(const std::string& lora_path) const;
    
    // Background eviction worker
    void evictionWorker();
    void startEvictionThread();
    void stopEvictionThread();
    
    // Multi-GPU helpers (v1.4.0)
    int selectGPUForLoRA(size_t vram_bytes);  // Select best GPU for new LoRA
    bool loadLoRAOnGPU(LoRASlot* lora, int gpu_id);  // Load LoRA on specific GPU
    bool loadLoRAMultiGPU(LoRASlot* lora);  // Load LoRA across multiple GPUs
    void updateGPUMemoryTracking();  // Recalculate per-GPU memory usage
    bool isGPUHealthy(int gpu_id) const;  // Check GPU health status
    std::vector<int> getAvailableGPUs() const;  // Get list of available GPUs
    
    // Quantization helpers
    bool quantizeLoRA(LoRASlot* lora);
    void quantizeINT8(LoRASlot* lora, const std::vector<float>& weights);
    void quantizeINT4(LoRASlot* lora, const std::vector<float>& weights);
    void calibrateScales(const std::vector<float>& weights, std::vector<float>& scales);
    std::vector<float> simulateWeights(size_t count);  // For testing without real weights
    
    // Fusion helpers (v1.5.0)
    bool fuseLoRAsInternal(const std::string& fused_id, const FusionConfig& config);
    std::vector<float> computeScheduledWeights(const std::string& fusion_id) const;
    std::vector<float> computeLinearSchedule(const AlphaSchedule& schedule, double time_offset) const;
    std::vector<float> computeExponentialSchedule(const AlphaSchedule& schedule, double time_offset) const;
    std::vector<float> computeStepWiseSchedule(const AlphaSchedule& schedule, double time_offset) const;
    bool validateFusionCompatibility(
        const std::vector<LoRASlot*>& source_loras,
        const FusionConfig& config
    ) const;
    void updateFusionMetrics(const std::string& fusion_id, double fusion_time_ms);
    void updateInferenceMetrics(const std::string& fusion_id, double inference_time_ms);
    
    bool hasCapacity(size_t vram_bytes) const;
    void updateMemoryUsage();
};

} // namespace llm
} // namespace themis

