/**
 * @file training_state_checkpoint.h
 * @brief Serializable training state snapshots for reproducibility
 * @version 0.0.1
 * @note Maturity: 🟡 BETA (Phase 1 foundation)
 * @author makr
 * 
 * Captures complete training state at checkpoints for:
 * - Recovery from interruptions
 * - Bit-exact reproducibility
 * - Distributed training synchronization
 * 
 * @since 2026-07-01 (EPIC: LoRA/AdaLoRA Training Pipeline, Phase 1)
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace themis {
namespace training {

using json = nlohmann::json;

/**
 * @brief Training state checkpoint
 * 
 * Comprehensive snapshot of training state including:
 * - Epoch and step counters
 * - Loss history
 * - RNG state (for reproducibility)
 * - Optimizer state (momentum, etc.)
 * - Dataset position (for resumption)
 * - Provenance snapshot
 * 
 * Used for recovery, distributed sync, and reproducibility validation.
 */
struct TrainingStateCheckpoint {
    // Training position
    size_t epoch = 0;                             ///< Current epoch number
    size_t global_step = 0;                       ///< Global training step count
    size_t steps_in_epoch = 0;                    ///< Steps completed in current epoch
    
    // Loss history
    std::vector<double> step_losses;              ///< Loss at each training step
    double current_loss = 0.0;                    ///< Most recent loss
    double best_loss = std::numeric_limits<double>::max(); ///< Best loss seen
    
    // RNG state for reproducibility
    std::vector<uint8_t> rng_state;              ///< Serialized RNG state
    std::string rng_provider_name;                ///< Name of RNG provider
    
    // Optimizer state
    std::vector<uint8_t> optimizer_state;        ///< Serialized optimizer parameters
    std::string optimizer_name;                   ///< Name of optimizer
    
    // Dataset position
    std::string dataset_source;                   ///< Dataset identifier
    size_t dataset_position = 0;                  ///< Samples processed
    uint64_t dataset_seed = 0;                    ///< Dataset shuffle seed
    
    // Hyperparameters snapshot
    json hyperparameters;                         ///< Training hyperparameters at checkpoint time
    
    // Provenance
    std::string training_run_id;                  ///< Training run identifier
    std::string training_config_hash;             ///< SHA-256 of config
    std::string base_model_hash;                  ///< SHA-256 of base model
    
    // Timestamps
    std::chrono::system_clock::time_point checkpoint_time;
    double elapsed_seconds = 0.0;                 ///< Wall-clock training time
    
    // Hardware info
    std::string hardware_platform;                ///< cuda/hip/metal/cpu
    int num_devices = 1;                          ///< Number of devices used
    
    TrainingStateCheckpoint() = default;
    
    /**
     * @brief Convert to JSON
     */
    json toJSON() const;
    
    /**
     * @brief Load from JSON
     */
    static TrainingStateCheckpoint fromJSON(const json& j);
    
    /**
     * @brief Compute checksum for integrity verification
     */
    std::string computeChecksum() const;
    
    /**
     * @brief Verify checkpoint integrity
     */
    bool verifyChecksum(const std::string& expected_checksum) const;
};

/**
 * @brief Checkpoint manager for training persistence
 * 
 * Handles saving, loading, and managing training state checkpoints
 * with atomic writes and integrity verification.
 */
class TrainingCheckpointManager {
public:
    /**
     * @brief Construct checkpoint manager
     * 
     * @param checkpoint_dir Directory for checkpoint files
     * @param training_run_id Unique identifier for this training run
     */
    explicit TrainingCheckpointManager(
        const std::string& checkpoint_dir,
        const std::string& training_run_id
    );
    
    ~TrainingCheckpointManager();
    
    /**
     * @brief Save checkpoint to disk
     * 
     * Uses atomic write (write-then-rename) to ensure no partial data on crash.
     * 
     * @param checkpoint The checkpoint to save
     * @return Path where checkpoint was saved
     * @throws std::runtime_error if write fails
     */
    std::string saveCheckpoint(const TrainingStateCheckpoint& checkpoint);
    
    /**
     * @brief Load most recent checkpoint
     * 
     * @return Latest checkpoint, or empty checkpoint if none found
     * @throws std::runtime_error if file corrupted
     */
    TrainingStateCheckpoint loadLatestCheckpoint();
    
    /**
     * @brief Load checkpoint by epoch number
     * 
     * @param epoch Epoch number to load
     * @return Checkpoint for specified epoch
     * @throws std::runtime_error if not found or corrupted
     */
    TrainingStateCheckpoint loadCheckpoint(size_t epoch);
    
    /**
     * @brief Get list of available checkpoints
     * 
     * @return Vector of (epoch, path) pairs in chronological order
     */
    std::vector<std::pair<size_t, std::string>> listCheckpoints() const;
    
    /**
     * @brief Delete old checkpoints, keeping only last N
     * 
     * @param keep_last Number of checkpoints to retain
     * @return Number of checkpoints deleted
     */
    size_t pruneOldCheckpoints(size_t keep_last = 3);
    
private:
    std::string checkpoint_dir_;
    std::string training_run_id_;
    
    std::string getCheckpointPath(size_t epoch) const;
};

} // namespace training
} // namespace themis
