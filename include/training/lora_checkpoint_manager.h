/*
 * ThemisDB | File: lora_checkpoint_manager.h | Version: 0.0.13
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <cstddef>
#include <ctime>

namespace themis {
namespace training {

/**
 * @brief Metadata stored in the checkpoint manifest alongside adapter weights.
 */
struct CheckpointManifestEntry {
    std::string checkpoint_path;   ///< Absolute path to the checkpoint file
    std::string sha256;            ///< SHA-256 hex digest of the checkpoint file
    std::string base_model_hash;   ///< SHA-256 of the base model used for training
    std::string adapter_version;   ///< Version tag (e.g., "legal_v1.2")
    size_t      epoch       = 0;   ///< Training epoch when checkpoint was saved
    size_t      step        = 0;   ///< Training step within the epoch
    double      loss        = 0.0; ///< Training loss at checkpoint
    double      accuracy    = 0.0; ///< Validation accuracy at checkpoint
    std::time_t saved_at    = 0;   ///< Unix timestamp of save

    CheckpointManifestEntry() = default;
};

/**
 * @brief Configuration for the LoRA checkpoint manager.
 */
struct CheckpointManagerConfig {
    std::string checkpoint_dir;              ///< Directory to store checkpoints
    size_t      max_checkpoints     = 3;     ///< Rolling window size (oldest pruned)
    bool        validate_on_load    = true;  ///< Validate SHA-256 on resume()
    bool        auto_rollback       = true;  ///< Fall back to previous on corruption
    std::string manifest_filename   = "checkpoint_manifest.json";

    CheckpointManagerConfig() = default;
};

/**
 * @brief Atomic, integrity-validated LoRA adapter checkpoint manager.
 *
 * Writes checkpoints atomically (write → .tmp → SHA-256 → rename) and keeps a
 * rolling window of the last `max_checkpoints` (default 3) entries. On
 * `resume()`, the SHA-256 of the requested checkpoint is verified; if corrupt
 * the manager automatically falls back to the previous valid entry and emits a
 * WARN-level log.
 *
 * Checkpoint metadata (epoch, step, loss, sha256, base_model_hash) is stored in
 * `checkpoint_manifest.json` inside `checkpoint_dir`.
 *
 * Example usage:
 * @code
 * CheckpointManagerConfig cfg;
 * cfg.checkpoint_dir   = "/var/lib/themis/checkpoints/legal_v1";
 * cfg.max_checkpoints  = 3;
 *
 * LoRACheckpointManager mgr(cfg);
 * mgr.save("weights.bin", {.epoch=1, .step=500, .loss=0.42, .adapter_version="legal_v1.1"});
 *
 * auto entry = mgr.resume();
 * if (entry) {
 *     // load entry->checkpoint_path
 * }
 * @endcode
 */
class LoRACheckpointManager {
public:
    /**
     * @brief Construct the checkpoint manager.
     * @param config Configuration for directory, window size, and validation.
     * @throws std::invalid_argument if checkpoint_dir is empty.
     */
    explicit LoRACheckpointManager(const CheckpointManagerConfig& config);

    ~LoRACheckpointManager();

    // Non-copyable, movable
    LoRACheckpointManager(const LoRACheckpointManager&)            = delete;
    LoRACheckpointManager& operator=(const LoRACheckpointManager&) = delete;
    LoRACheckpointManager(LoRACheckpointManager&&)                 = default;
    LoRACheckpointManager& operator=(LoRACheckpointManager&&)      = default;

    /**
     * @brief Atomically save a checkpoint with integrity metadata.
     *
     * Reads @p source_path, copies it to `checkpoint_dir/<filename>.tmp`,
     * computes its SHA-256, renames to the final path, then updates the
     * manifest. The oldest entry is pruned once the rolling window overflows.
     *
     * @param source_path Path to the adapter weights file to checkpoint.
     * @param meta        Metadata to store in the manifest (sha256 is computed
     *                    automatically and must not be pre-filled).
     * @return Manifest entry with the final path and computed SHA-256.
     * @throws std::runtime_error on I/O failure.
     */
    CheckpointManifestEntry save(const std::string& source_path,
                                 CheckpointManifestEntry meta);

    /**
     * @brief Validate and return the latest valid checkpoint entry.
     *
     * If `validate_on_load` is true the SHA-256 of the latest checkpoint is
     * verified. On mismatch, if `auto_rollback` is true the previous entry is
     * tried and a WARN log is emitted. Returns std::nullopt if no valid
     * checkpoint exists.
     *
     * @return Latest valid manifest entry, or std::nullopt.
     */
    std::optional<CheckpointManifestEntry> resume() const;

    /**
     * @brief Return all manifest entries, newest first.
     */
    std::vector<CheckpointManifestEntry> listCheckpoints() const;

    /**
     * @brief Validate the integrity of a specific checkpoint file.
     * @param entry Manifest entry to validate.
     * @return true if the file exists and its SHA-256 matches the stored digest.
     */
    bool validate(const CheckpointManifestEntry& entry) const;

    /**
     * @brief Delete all checkpoints and clear the manifest.
     */
    void clearAll();

    /**
     * @brief Return the path to the manifest JSON file.
     */
    std::string manifestPath() const;

    /**
     * @brief Persist a calibration result as `calibration_manifest.json`
     *        in the checkpoint directory.
     *
     * The calibration manifest is written alongside adapter weights so that
     * `ConfidenceCalibrator` thresholds are always co-located with the
     * checkpoint they were derived from.
     *
     * @param json_content Serialised calibration result (key=value or JSON string).
     * @throws std::runtime_error on I/O failure.
     */
    void saveCalibrationJson(const std::string& json_content);

    /**
     * @brief Load the calibration manifest from the checkpoint directory.
     * @return Contents of `calibration_manifest.json`, or empty string if not present.
     */
    std::string loadCalibrationJson() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace training
} // namespace themis
