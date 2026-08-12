/**
 * @file distributed_training_coordinator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <optional>
#include <chrono>
#include <nlohmann/json.hpp>
#include "llm/byzantine_detector.h"
#include "sharding/shard_router.h"
#include "sharding/shard_topology.h"

// Forward declarations
class ShardRouter;
class ShardTopology;
// Minimal training configuration used for coordinator initialization
struct TrainingConfig {
    virtual ~TrainingConfig() = default;
    int epochs = 1;
    int total_steps = 0;
    float learning_rate = 0.0f;
    int batch_size = 0;
};
class OptimizerState;
class TrainingMetrics;

// Byzantine types are defined in byzantine_detector.h

namespace themis {
namespace llm {

using json = nlohmann::json;

// Map local coordinator types to sharding infrastructure
using ShardRouter = themis::sharding::ShardRouter;
using ShardTopology = themis::sharding::ShardTopology;

// ============================================================================
// Gradient Synchronization Strategies
// ============================================================================

enum class SyncStrategy {
    ALL_REDUCE,          // All shards exchange gradients, average them
    PARAMETER_SERVER,    // Central shard aggregates gradients
    RING_ALL_REDUCE,     // Ring topology for communication efficiency
    HIERARCHICAL,        // Multi-level aggregation (regional → global)
    ASYNC_SGD            // Asynchronous updates (eventual consistency)
};

enum class GradientCompressionType {
    NONE,                // No compression
    QUANTIZATION_8BIT,   // 8-bit quantization
    QUANTIZATION_4BIT,   // 4-bit quantization (aggressive)
    SPARSE_TOPK,         // Send only top-K% largest gradients
    ERROR_FEEDBACK       // Compress with error feedback correction
};

// ============================================================================
// Distributed Training Configuration
// ============================================================================

struct DistributedTrainingConfig {
    virtual ~DistributedTrainingConfig() = default;
    SyncStrategy sync_strategy = SyncStrategy::ALL_REDUCE;
    GradientCompressionType compression = GradientCompressionType::NONE;
    
    std::string coordinator_shard;              // Shard orchestrating the training
    std::vector<std::string> participant_shards; // All shards participating
    
    int gradient_accumulation_steps = 1;        // Micro-batches before sync
    int sync_frequency = 1;                     // Sync every N steps
    float gradient_clip_norm = 1.0f;            // Clip before transmission
    
    bool use_mixed_precision = false;           // FP16 gradient transmission
    bool sparse_gradients = false;              // Only send non-zero gradients
    float sparse_threshold = 1e-6f;             // Sparsity threshold
    
    int max_retry_attempts = 3;                 // Network failure retry
    int timeout_seconds = 300;                  // Communication timeout
    
    // Fault tolerance
    bool enable_checkpointing = true;
    int checkpoint_frequency = 100;             // Steps between checkpoints
    std::string checkpoint_path;
    
    // Byzantine fault detection
    bool enable_byzantine_detection = false;
    ByzantineDetectionMethod detection_method = ByzantineDetectionMethod::MEDIAN;
    float detection_threshold = 3.0f;           // For median-based (MAD multiplier)
    int max_byzantine_shards = 1;               // For Krum/Bulyan (f parameter)
    ByzantineAction byzantine_action = ByzantineAction::EXCLUDE;

    // Training schedule — used for ETA estimation
    int total_steps = 0;                        // 0 = unknown / open-ended

    json toJSON() const;
    static DistributedTrainingConfig fromJSON(const json& j);
};

// ============================================================================
// Gradient Tensor (Distributed Communication Unit)
// ============================================================================

struct GradientTensor {
    virtual ~GradientTensor() = default;
    std::string layer_name;                     // "lora_layer_q_proj_A"
    std::vector<float> data;                    // Gradient values
    std::vector<int> shape;                     // Tensor dimensions
    
    // Metadata
    std::string source_shard;
    int64_t timestamp_ms = 0;
    int step_number = 0;
    
    // Compression info
    GradientCompressionType compression_type = GradientCompressionType::NONE;
    std::optional<std::vector<uint8_t>> compressed_data;
    
    size_t uncompressed_size() const { return data.size() * sizeof(float); }
    size_t compressed_size() const;
    
    void compress(GradientCompressionType type);
    void decompress();
    
    json toJSON() const;
    static GradientTensor fromJSON(const json& j);
};

// ============================================================================
// Gradient Exchange Message
// ============================================================================

struct GradientExchangeMessage {
    virtual ~GradientExchangeMessage() = default;
    std::string message_id;                     // Unique message ID
    std::string source_shard;
    std::string destination_shard;
    
    std::vector<GradientTensor> gradients;
    
    // All-reduce metadata
    int iteration_number = 0;
    int total_participants = 0;
    std::vector<std::string> participants_seen; // Ring all-reduce tracking
    
    // Timing
    int64_t sent_timestamp_ms = 0;
    int64_t received_timestamp_ms = 0;
    
    // Loss metrics from this shard
    std::optional<float> local_loss;
    std::optional<float> local_accuracy;
    int samples_in_batch = 0;
    
    json toJSON() const;
    static GradientExchangeMessage fromJSON(const json& j);
};

// ============================================================================
// Shard Training State (Per-Shard Status)
// ============================================================================

struct ShardTrainingState {
    virtual ~ShardTrainingState() = default;
    std::string shard_id;
    
    // Training progress
    int current_epoch = 0;
    int current_step = 0;
    int total_steps = 0;
    
    // Metrics
    float current_loss = 0.0f;
    float avg_grad_norm = 0.0f;
    int samples_processed = 0;
    
    // Health
    bool is_active = true;
    bool is_synchronized = true;
    int64_t last_heartbeat_ms = 0;
    int consecutive_failures = 0;
    
    // Resource usage
    float gpu_utilization = 0.0f;
    float memory_usage_gb = 0.0f;
    
    json toJSON() const;
    static ShardTrainingState fromJSON(const json& j);
};

// ============================================================================
// Distributed Training Statistics
// ============================================================================

struct DistributedTrainingStats {
    virtual ~DistributedTrainingStats() = default;
    int total_steps_completed = 0;
    int total_gradient_syncs = 0;
    
    // Communication metrics
    size_t total_bytes_sent = 0;
    size_t total_bytes_received = 0;
    float avg_sync_time_ms = 0.0f;
    float max_sync_time_ms = 0.0f;
    
    // Compression efficiency
    float compression_ratio = 1.0f;             // compressed_size / original_size
    float bandwidth_saved_gb = 0.0f;
    
    // Fault tolerance
    int total_retries = 0;
    int shard_failures = 0;
    int successful_recoveries = 0;
    
    // Speedup
    float effective_speedup = 1.0f;             // vs single shard
    float communication_overhead_pct = 0.0f;    // % time spent on network
    
    // Byzantine detection metrics
    int byzantine_detections = 0;
    int byzantine_shards_excluded = 0;
    std::map<std::string, int> per_shard_detection_count;
    float avg_anomaly_score = 0.0f;
    std::vector<float> gradient_norm_history;
    
    json toJSON() const;
};

// ============================================================================
// Gradient Aggregator (All-Reduce Implementation)
// ============================================================================

/** @brief Gradient Aggregator (All-Reduce Implementation). */
class GradientAggregator {
public:
    virtual ~GradientAggregator() = default;
    
    // Aggregate gradients from multiple shards
    virtual std::vector<GradientTensor> aggregate(
        const std::vector<std::vector<GradientTensor>>& shard_gradients
    ) = 0;
    
    // Get aggregation strategy name
    virtual std::string getStrategy() const = 0;
};

// All-Reduce: Average gradients from all shards
/** @brief All-Reduce: Average gradients from all shards. */
class AllReduceAggregator : public GradientAggregator {
public:
    ~AllReduceAggregator() override = default;

    std::vector<GradientTensor> aggregate(
        const std::vector<std::vector<GradientTensor>>& shard_gradients
    ) override;
    
    std::string getStrategy() const override { return "ALL_REDUCE"; }
};

// Parameter Server: Weighted average (data-proportional)
/** @brief Parameter Server: Weighted average (data-proportional). */
class ParameterServerAggregator : public GradientAggregator {
public:
    ParameterServerAggregator(const std::map<std::string, float>& shard_weights)
        : shard_weights_(shard_weights) {}
    ~ParameterServerAggregator() override = default;
    
    std::vector<GradientTensor> aggregate(
        const std::vector<std::vector<GradientTensor>>& shard_gradients
    ) override;
    
    std::string getStrategy() const override { return "PARAMETER_SERVER"; }
    
private:
    std::map<std::string, float> shard_weights_;  // Shard ID -> weight
};

// Ring All-Reduce: Communication-efficient ring pattern
/** @brief Ring All-Reduce: Communication-efficient ring pattern. */
class RingAllReduceAggregator : public GradientAggregator {
public:
    ~RingAllReduceAggregator() override = default;

    std::vector<GradientTensor> aggregate(
        const std::vector<std::vector<GradientTensor>>& shard_gradients
    ) override;
    
    std::string getStrategy() const override { return "RING_ALL_REDUCE"; }
    
    void setRingTopology(const std::vector<std::string>& ring_order);
    
private:
    std::vector<std::string> ring_order_;
};

// ============================================================================
// Distributed Training Coordinator (Main Orchestrator)
// ============================================================================

/** @brief Distributed Training Coordinator (Main Orchestrator). */
class DistributedTrainingCoordinator {
public:
    DistributedTrainingCoordinator(
        std::shared_ptr<ShardRouter> shard_router,
        std::shared_ptr<ShardTopology> shard_topology,
        const DistributedTrainingConfig& config
    );
    
    ~DistributedTrainingCoordinator();
    
    // ========================================================================
    // Training Orchestration
    // ========================================================================
    
    // Initialize distributed training session
    bool initialize(const std::string& adapter_id, const TrainingConfig& training_config);
    
    // Execute one distributed training step
    struct StepResult {
        bool success = false;
        int step_number = 0;
        std::vector<GradientTensor> aggregated_gradients;
        float sync_time_ms = 0.0f;
        float total_time_ms = 0.0f;
        std::map<std::string, ShardTrainingState> shard_states;
        std::optional<float> aggregated_loss;
        std::optional<float> aggregated_accuracy;
        std::map<std::string, float> per_shard_loss;  // For monitoring
    };
    StepResult executeStep();
    
    // Finalize training and collect final adapters
    bool finalize();
    
    // Stop training (graceful shutdown)
    void stop();
    
    // ========================================================================
    // Gradient Synchronization
    // ========================================================================
    
    // Collect gradients from all participant shards
    std::map<std::string, std::vector<GradientTensor>> collectGradients(int step_number);
    
    // Aggregate collected gradients
    std::vector<GradientTensor> aggregateGradients(
        const std::map<std::string, std::vector<GradientTensor>>& shard_gradients
    );
    
    // Aggregate loss values from all shards
    std::optional<float> aggregateLoss(
        const std::map<std::string, std::vector<GradientTensor>>& shard_gradients
    );
    
    // Weighted average based on samples processed
    float computeWeightedLoss(
        const std::vector<std::pair<float, int>>& shard_losses_and_counts
    );
    
    // Broadcast aggregated gradients to all shards
    bool broadcastGradients(const std::vector<GradientTensor>& gradients, int step_number);
    
    // ========================================================================
    // Fault Tolerance & Health Monitoring
    // ========================================================================
    
    // Check shard health (heartbeat)
    std::map<std::string, ShardTrainingState> checkShardHealth();
    
    // Handle shard failure (remove from training, redistribute work)
    bool handleShardFailure(const std::string& failed_shard);
    
    // Save distributed checkpoint
    bool saveCheckpoint(int step_number);
    
    // Resume from checkpoint
    bool resumeFromCheckpoint(const std::string& checkpoint_path);
    
    // ========================================================================
    // Monitoring & Statistics
    // ========================================================================
    
    DistributedTrainingStats getStatistics() const;
    
    std::map<std::string, ShardTrainingState> getShardStates() const;
    
    float estimateRemainingTime() const;  // Minutes
    
    // Progress callback
    using ProgressCallback = std::function<void(int step, const StepResult& result)>;
    void setProgressCallback(ProgressCallback callback);
    
    // ========================================================================
    // Configuration
    // ========================================================================
    
    DistributedTrainingConfig getConfig() const { return config_; }
    
    void updateConfig(const DistributedTrainingConfig& config);
    
private:
    // Dependencies
    std::shared_ptr<ShardRouter> shard_router_;
    std::shared_ptr<ShardTopology> shard_topology_;
    
    // Configuration
    DistributedTrainingConfig config_;
    
    // State
    std::string adapter_id_;
    bool is_initialized_ = false;
    bool is_running_ = false;
    int current_step_ = 0;
    
    // Shard management
    std::map<std::string, ShardTrainingState> shard_states_;
    std::vector<std::string> active_shards_;
    
    // Gradient aggregator
    std::unique_ptr<GradientAggregator> aggregator_;
    
    // Byzantine detector
    std::unique_ptr<ByzantineDetector> byzantine_detector_;
    
    // Statistics
    DistributedTrainingStats stats_;
    std::chrono::steady_clock::time_point start_time_;
    
    // Callback
    ProgressCallback progress_callback_;
    
    // Helper methods
    void initializeAggregator();
    void initializeByzantineDetector();
    bool validateShardParticipation();
    void updateStatistics(const StepResult& result);
    std::vector<GradientTensor> compressGradients(const std::vector<GradientTensor>& gradients);
    std::vector<GradientTensor> decompressGradients(const std::vector<GradientTensor>& gradients);
    void clipAnomalousGradients(
        std::map<std::string, std::vector<GradientTensor>>& shard_gradients,
        const DetectionResult& detection_result
    );
};

// ============================================================================
// Factory for Creating Coordinators
// ============================================================================

/** @brief Factory for Creating Coordinators. */
class DistributedTrainingCoordinatorFactory {
public:
    static std::unique_ptr<DistributedTrainingCoordinator> create(
        std::shared_ptr<ShardRouter> shard_router,
        std::shared_ptr<ShardTopology> shard_topology,
        const DistributedTrainingConfig& config
    );
    
    // Create with automatic shard discovery
    static std::unique_ptr<DistributedTrainingCoordinator> createWithAutoDiscovery(
        std::shared_ptr<ShardRouter> shard_router,
        SyncStrategy strategy = SyncStrategy::ALL_REDUCE
    );
};

} // namespace llm
} // namespace themis

