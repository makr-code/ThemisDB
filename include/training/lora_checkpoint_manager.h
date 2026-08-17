/**
 * @file lora_checkpoint_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
#include "training/training_error_codes.h"
#include "training/training_exceptions.h"

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
 *
 * Phase 2 hardening: Added timeout support for I/O operations, explicit recovery
 * configuration, and diagnostics control.
 */
struct CheckpointManagerConfig {
    std::string checkpoint_dir;              ///< Directory to store checkpoints
    size_t      max_checkpoints     = 3;     ///< Rolling window size (oldest pruned)
    bool        validate_on_load    = true;  ///< Validate SHA-256 on resume()
    bool        auto_rollback       = true;  ///< Fall back to previous on corruption
    std::string manifest_filename   = "checkpoint_manifest.json";
    
    // Phase 2: timeout and recovery hardening
    int         io_timeout_ms       = 300000; ///< Timeout for I/O ops (SHA-256, file copy)
    bool        cleanup_partial     = true;   ///< Remove partial/corrupted checkpoints
    size_t      min_checkpoint_size = 1024;   ///< Minimum valid checkpoint size (bytes)

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
 * ## Phase 3: Error Handling and Edge Cases
 *
 * All public methods throw CheckpointException with structured error codes and
 * diagnostics for production troubleshooting:
 *
 * - **Constructor**: throws CheckpointException if checkpoint_dir is empty
 *   or invalid (code: CHECKPOINT_DIR_INVALID)
 *
 * - **save()**: throws CheckpointException on:
 *   - File read failure (CHECKPOINT_READ_FAILED)
 *   - Disk full (CHECKPOINT_DISK_SPACE_EXHAUSTED)
 *   - File write failure (CHECKPOINT_WRITE_FAILED)
 *   - Rename/move failure (CHECKPOINT_RENAME_FAILED)
 *   - I/O timeout (CHECKPOINT_IO_TIMEOUT)
 *
 * - **resume()**: returns std::nullopt if no valid checkpoint found;
 *   logs recovered checkpoint path on success or auto-rollback
 *
 * - **resumeWithDiagnostics()**: includes detailed recovery information
 *   including which checkpoints were attempted and why they failed
 *
 * - **validate()**: returns false if file missing or SHA-256 mismatch
 *
 * - **cleanupPartialCheckpoints()**: logs which files were removed
 *
 * - **auditCheckpoints()**: reports validity status for each entry
 *   and returns count of valid checkpoints
 *
 * Edge cases handled:
 * - Empty checkpoint directory: gracefully handled (resume returns nullopt)
 * - Corrupted manifest: malformed entries silently dropped, valid entries retained
 * - Partially-written checkpoints: detected by size check, cleaned up
 * - Disk full during save: detected early, error thrown with recoverable=true
 * - SHA-256 validation timeout: error thrown with code CHECKPOINT_VALIDATION_TIMEOUT
 * - All checkpoints corrupted: auto-rollback exhausted, explicit error with recovery options
 *
 * Example usage:
 * @code
 * CheckpointManagerConfig cfg;
 * cfg.checkpoint_dir   = "/var/lib/themis/checkpoints/legal_v1";
 * cfg.max_checkpoints  = 3;
 *
 * try {
 *     LoRACheckpointManager mgr(cfg);  // throws if dir invalid
 *     mgr.save("weights.bin", {.epoch=1, .step=500, .loss=0.42, .adapter_version="legal_v1.1"});
 *
 *     auto entry = mgr.resume();  // nullopt if no valid checkpoint
 *     if (entry) {
 *         // load entry->checkpoint_path
 *     }
 * } catch (const CheckpointException& e) {
 *     // Handle checkpoint failure with error code and diagnostics
 *     log_error << e.diagnostic_message();
 * }
 * @endcode
 */
class LoRACheckpointManager {
public:
    /**
     * @brief Construct the checkpoint manager.
     * @param config Configuration for directory, window size, and validation.
     * @throws CheckpointException if checkpoint_dir is empty or path is unsafe
     *         (error code: CHECKPOINT_DIR_INVALID or CHECKPOINT_PATH_UNSAFE)
     */
    explicit LoRACheckpointManager(const CheckpointManagerConfig& config);

    ~LoRACheckpointManager();

    // Non-copyable, movable
    LoRACheckpointManager(const LoRACheckpointManager&)            = delete;
    LoRACheckpointManager& operator=(const LoRACheckpointManager&) = delete;
    LoRACheckpointManager(LoRACheckpointManager&&)                 noexcept = default;
    LoRACheckpointManager& operator=(LoRACheckpointManager&&)      noexcept = default;

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
     * @brief Phase 2: Recover a checkpoint with detailed diagnostics.
     *
     * Attempts to recover a checkpoint from the manifest, returning full
     * diagnostic information about the recovery process (which checkpoints were
     * attempted, why they failed, etc.). This is useful for understanding
     * checkpoint corruption or rollback behavior.
     *
     * @param[out] diagnostics Optional string to receive detailed recovery info
     * @return Latest valid manifest entry, or std::nullopt if recovery fails
     */
    std::optional<CheckpointManifestEntry> resumeWithDiagnostics(
        std::string* diagnostics = nullptr) const;

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

    /**
     * @brief Phase 2: Clean up partial/corrupted checkpoints in the directory.
     *
     * Removes checkpoint files that are not in the manifest, or checkpoint files
     * that fail validation and are marked for cleanup in the config.
     * This helps recover disk space and maintain a clean checkpoint directory.
     *
     * @return Number of files cleaned up
     */
    size_t cleanupPartialCheckpoints();

    /**
     * @brief Phase 2: Verify all checkpoints in the manifest and report status.
     *
     * Performs a full audit of the checkpoint directory: validates each entry,
     * reports which are valid/corrupt, and optionally removes corrupt entries.
     *
     * @param[out] diagnostics Optional string to receive audit results
     * @return Number of valid checkpoints found
     */
    size_t auditCheckpoints(std::string* diagnostics = nullptr);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace training
} // namespace themis
