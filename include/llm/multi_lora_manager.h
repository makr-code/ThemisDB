#pragma once

#include "llm/llm_plugin_interface.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <optional>
#include <thread>
#include <atomic>
#include <condition_variable>

/**
 * @file multi_lora_manager.h
 * @brief vLLM-inspired multi-LoRA management for ThemisDB
 * 
 * This component implements efficient multi-LoRA adapter management similar to vLLM:
 * - Multiple LoRA adapters can be loaded simultaneously
 * - Efficient adapter switching during inference (minimal overhead)
 * - Batched inference with different LoRAs for different requests
 * - Memory-efficient adapter storage and composition
 * 
 * Key features from vLLM:
 * - Multi-LoRA inference: Different requests can use different adapters
 * - Adapter batching: Multiple LoRAs in a single batch
 * - Dynamic loading/unloading: Adapters load/unload on demand
 * - Memory pooling: Efficient VRAM usage for multiple adapters
 */

namespace themis {
namespace llm {

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
    size_t max_vram_per_gpu_mb = 24 * 1024;     // Max VRAM per GPU (default: 24GB)
    
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
 * @brief LoRA adapter slot
 * 
 * Represents a loaded LoRA adapter with its metadata and handle.
 */
struct LoRASlot {
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
 */
class MultiLoRAManager {
public:
    struct Config {
        // Memory limits
        size_t max_lora_vram_mb = 2048;  // 2 GB for all LoRAs
        size_t max_lora_slots = 16;      // Max concurrent LoRAs
        
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
    };
    
    explicit MultiLoRAManager(const Config& config);
    ~MultiLoRAManager();
    
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
     * @param context_handle Model context to apply to
     * @return true if applied successfully
     */
    bool applyLoRA(const std::string& lora_id, void* context_handle);
    
    /**
     * @brief Remove LoRA from model context
     * 
     * Deactivates a LoRA adapter.
     */
    bool removeLoRA(const std::string& lora_id, void* context_handle);
    
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
        void* model_context
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
     * @brief List all loaded LoRAs
     */
    std::vector<LoRAInfo> listLoRAs() const;

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
    mutable std::mutex mutex_;
    
    // Statistics
    size_t total_vram_bytes_ = 0;
    size_t cache_hits_ = 0;
    size_t cache_misses_ = 0;
    size_t evictions_ = 0;
    size_t switches_ = 0;                // LoRA switch count
    
    // Multi-GPU state (v1.4.0)
    std::unordered_map<int, size_t> gpu_vram_usage_;  // Per-GPU VRAM tracking
    int next_round_robin_gpu_ = 0;                     // Round-robin counter
    
    // Background eviction thread
    std::unique_ptr<std::thread> eviction_thread_;
    std::atomic<bool> eviction_thread_running_{false};
    std::condition_variable eviction_cv_;
    
    // Internal helpers
    LoRASlot* loadLoRAInternal(
        const std::string& lora_id,
        const std::string& lora_path,
        const std::string& base_model_id,
        float scale,
        bool quantize = false,
        GPUPlacement placement = GPUPlacement::SINGLE_GPU
    );
    
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
    
    bool hasCapacity(size_t vram_bytes) const;
    void updateMemoryUsage();
};

} // namespace llm
} // namespace themis
