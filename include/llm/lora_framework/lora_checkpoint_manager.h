/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            lora_checkpoint_manager.h                          ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-04-15 18:45:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     227                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 67965456c8  2026-03-22  Add constructors with default config for various classes ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

/**
 * @file lora_checkpoint_manager.h
 * @brief LoRA adapter checkpoint lifecycle management.
 *
 * Provides durable, versioned checkpointing of LoRA adapter weights
 * during and after fine-tuning, enabling:
 *
 * - Mid-training resume after process failure or preemption.
 * - Best-checkpoint selection based on validation-loss tracking.
 * - Automatic rotation to bound on-disk storage.
 * - Atomic save/load with SHA-256 integrity verification.
 *
 * ## Storage Layout
 * ```
 * <root>/
 *   <adapter_id>/
 *     checkpoint-<step>.bin      – adapter weights (safetensors)
 *     checkpoint-<step>.meta.json – training metadata
 *     best.json                  – symlink/record to best checkpoint
 * ```
 *
 * ## Thread Safety
 * All public methods are thread-safe.  Concurrent saves from multiple
 * trainers sharing the same manager are serialised with a mutex.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lora_config.h"
#include "lora_storage_service.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace llm {
namespace lora {

// ─────────────────────────────────────────────────────────────────────────────
// Data types
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Snapshot of training state attached to a checkpoint.
 */
struct CheckpointMeta {
    std::string adapter_id;          ///< Unique adapter identifier
    uint64_t    step        = 0;     ///< Global training step at checkpoint
    uint32_t    epoch       = 0;     ///< Epoch at checkpoint
    float       train_loss  = 0.0f;  ///< Training loss
    float       val_loss    = std::numeric_limits<float>::infinity(); ///< Validation loss (lower=better)
    double      elapsed_s   = 0.0;   ///< Wall-clock training time in seconds
    std::string created_at;          ///< ISO-8601 UTC timestamp
    std::string weights_sha256;      ///< SHA-256 hex digest of the weight blob

    json toJSON() const;
    static CheckpointMeta fromJSON(const json& j);
};

/**
 * @brief Reference to a stored checkpoint.
 */
struct CheckpointRef {
    std::string path;          ///< Absolute path to weight file
    CheckpointMeta meta;       ///< Associated metadata
    bool is_best = false;      ///< Whether this is the current best checkpoint
};

// ─────────────────────────────────────────────────────────────────────────────
// LoRACheckpointManager
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Manages LoRA adapter checkpoints throughout the training lifecycle.
 *
 * Usage:
 * ```cpp
 * LoRACheckpointManager mgr(config);
 * mgr.save("my_adapter", weights, meta);          // Save mid-epoch
 * auto ref = mgr.loadBest("my_adapter");          // Resume from best
 * ```
 */
class LoRACheckpointManager {
public:
    /**
     * @brief Configuration for the checkpoint manager.
     */
    struct Config {
        std::string root_dir     = "checkpoints";  ///< Base directory for checkpoint storage
        size_t      keep_last    = 5;              ///< How many recent checkpoints to retain (0 = keep all)
        bool        keep_best    = true;           ///< Always retain the checkpoint with lowest val_loss
        bool        verify_hash  = true;           ///< Verify SHA-256 on load
        bool        enable_compression = true;     ///< Compress weight blobs (zstd)
        std::chrono::seconds auto_save_interval{0}; ///< 0 = disabled; >0 = background auto-save period
    };

    LoRACheckpointManager();
    explicit LoRACheckpointManager(Config config);
    ~LoRACheckpointManager();

    // Non-copyable
    LoRACheckpointManager(const LoRACheckpointManager&)            = delete;
    LoRACheckpointManager& operator=(const LoRACheckpointManager&) = delete;

    // ── Save / Load ───────────────────────────────────────────────────────────

    /**
     * @brief Atomically save adapter weights and metadata as a new checkpoint.
     *
     * The weight blob is written to a temporary file first, then renamed to
     * avoid partial writes.  Old checkpoints are pruned after a successful save
     * according to the @p keep_last policy.
     *
     * @param adapter_id Logical adapter name.
     * @param weights    Weight data (raw safetensors blob).
     * @param meta       Training metadata snapshot.
     * @return Path of the saved checkpoint file on success.
     */
    std::string save(const std::string&         adapter_id,
                     const std::vector<uint8_t>& weights,
                     CheckpointMeta              meta);

    /**
     * @brief Load the latest checkpoint for the given adapter.
     *
     * @return Checkpoint reference including weights path and metadata,
     *         or std::nullopt when no checkpoint exists.
     */
    std::optional<CheckpointRef> loadLatest(const std::string& adapter_id) const;

    /**
     * @brief Load the best (lowest val_loss) checkpoint for the given adapter.
     *
     * @return Checkpoint reference, or std::nullopt when no checkpoint exists.
     */
    std::optional<CheckpointRef> loadBest(const std::string& adapter_id) const;

    /**
     * @brief Load a specific checkpoint by training step.
     */
    std::optional<CheckpointRef> loadByStep(const std::string& adapter_id,
                                             uint64_t           step) const;

    /**
     * @brief Read the raw weight bytes from a checkpoint reference.
     *
     * Verifies SHA-256 if Config::verify_hash is set.
     * @return Decompressed weight blob.
     * @throws std::runtime_error on hash mismatch or I/O error.
     */
    std::vector<uint8_t> readWeights(const CheckpointRef& ref) const;

    // ── Listing / Management ─────────────────────────────────────────────────

    /**
     * @brief List all stored checkpoints for an adapter, newest first.
     */
    std::vector<CheckpointRef> listCheckpoints(const std::string& adapter_id) const;

    /**
     * @brief Delete a specific checkpoint by step.
     * @return true if the checkpoint was found and deleted.
     */
    bool deleteCheckpoint(const std::string& adapter_id, uint64_t step);

    /**
     * @brief Delete all checkpoints for an adapter.
     */
    void deleteAll(const std::string& adapter_id);

    /**
     * @brief Prune checkpoints to satisfy Config::keep_last and Config::keep_best.
     */
    void prune(const std::string& adapter_id);

    const Config& config() const noexcept { return config_; }

private:
    Config      config_;
    mutable std::mutex mutex_;

    std::string adapterDir(const std::string& adapter_id) const;
    std::string weightPath(const std::string& adapter_id, uint64_t step) const;
    std::string metaPath(const std::string& adapter_id, uint64_t step) const;

    void writeMeta(const std::string& path, const CheckpointMeta& meta) const;
    CheckpointMeta readMeta(const std::string& path) const;
    void updateBestRecord(const std::string& adapter_id,
                          const CheckpointMeta& meta) const;
    std::optional<CheckpointMeta> readBestMeta(const std::string& adapter_id) const;
};

} // namespace lora
} // namespace llm
} // namespace themis
