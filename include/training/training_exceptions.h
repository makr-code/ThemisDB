/**
 * @file training_exceptions.h
 * @brief Training Module Exception Classes (Phase 3)
 *
 * Defines exception hierarchy for training module components with structured
 * error information, error codes, and recovery semantics.
 *
 * @version 1.0.0
 * @date 2026-08-07
 * @author ThemisDB Training Module Hardening
 *
 * @since v2.4.0 (Phase 3: Error Handling & Edge Cases)
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

#include "training/training_error_codes.h"

#include <stdexcept>
#include <string>
#include <memory>

namespace themis {
namespace training {

/**
 * @brief Base exception class for all training module failures.
 *
 * Provides structured error information including error code, message,
 * recovery flag, and diagnostic context. All public APIs of the training
 * module throw exceptions derived from this base class.
 *
 * @invariant All training exceptions are derived from TrainingException
 * @invariant error_code() always returns a valid TrainingErrorCode
 * @invariant is_recoverable() accurately reflects whether error is transient
 */
class TrainingException : public std::runtime_error {
public:
    /**
     * @brief Construct a training exception with full diagnostic information.
     *
     * @param message Human-readable error message (actionable for operators).
     * @param code Error code identifying the specific failure mode.
     * @param recoverable Whether the operation can be retried or recovered.
     * @param context Optional diagnostic context (operation name, input state, etc).
     */
    explicit TrainingException(const std::string& message,
                               TrainingErrorCode code = TrainingErrorCode::SUCCESS,
                               bool recoverable = false,
                               const std::string& context = "")
        : std::runtime_error(message),
          code_(code),
          recoverable_(recoverable),
          context_(context) {
    }

    virtual ~TrainingException() = default;

    /**
     * @brief Get the structured error code.
     */
    TrainingErrorCode error_code() const noexcept { return code_; }

    /**
     * @brief Get numeric error code value.
     */
    uint32_t error_code_value() const noexcept {
        return static_cast<uint32_t>(code_);
    }

    /**
     * @brief Check if the error is transient/recoverable.
     *
     * Recoverable errors typically indicate resource exhaustion, timeouts,
     * or temporary unavailability that may succeed on retry.
     * Non-recoverable errors indicate invalid input or state that requires
     * operator intervention.
     */
    bool is_recoverable() const noexcept { return recoverable_; }

    /**
     * @brief Get diagnostic context.
     *
     * Context string provides operation name, input state, or other information
     * useful for debugging and production troubleshooting.
     */
    const std::string& context() const noexcept { return context_; }

    /**
     * @brief Get formatted diagnostic message.
     *
     * Format: "[{ErrorCodeName}] {message} ({context})"
     * Useful for structured logging and operator alerts.
     */
    std::string diagnostic_message() const {
        std::string msg = "[" + trainingErrorCodeToString(code_) + "] " + what();
        if (!context_.empty()) {
            msg += " (" + context_ + ")";
        }
        return msg;
    }

protected:
    TrainingErrorCode code_;
    bool recoverable_;
    std::string context_;
};

/**
 * @brief Exception for checkpoint management failures.
 *
 * Raised when checkpoint save, load, validation, or recovery fails.
 * Examples: corrupted checkpoint, missing manifest, SHA-256 mismatch,
 * I/O errors, disk full, etc.
 */
class CheckpointException : public TrainingException {
public:
    explicit CheckpointException(const std::string& message,
                                 TrainingErrorCode code = TrainingErrorCode::CHECKPOINT_DIR_INVALID,
                                 bool recoverable = false,
                                 const std::string& context = "")
        : TrainingException(message, code, recoverable, context) {
    }
};

/**
 * @brief Exception for training execution failures.
 *
 * Raised during training step, epoch, or initialization failure.
 * Examples: invalid config, empty dataset, NaN loss, GPU OOM,
 * cancellation, timeout, etc.
 */
class TrainingFailureException : public TrainingException {
public:
    explicit TrainingFailureException(const std::string& message,
                                      TrainingErrorCode code = TrainingErrorCode::TRAINING_CONFIG_INVALID,
                                      bool recoverable = false,
                                      const std::string& context = "")
        : TrainingException(message, code, recoverable, context) {
    }
};

/**
 * @brief Exception for adapter merge failures.
 *
 * Raised when merging multiple adapters fails.
 * Examples: dimension mismatch, incompatible configs, NaN during merge,
 * merge conflicts, etc.
 */
class MergeException : public TrainingException {
public:
    explicit MergeException(const std::string& message,
                           TrainingErrorCode code = TrainingErrorCode::MERGE_NO_ADAPTERS,
                           bool recoverable = false,
                           const std::string& context = "")
        : TrainingException(message, code, recoverable, context) {
    }
};

/**
 * @brief Exception for knowledge graph enrichment failures.
 *
 * Raised when enriching training data with knowledge graph lookups fails.
 * Examples: graph not initialized, query timeout, cache miss, invalid result, etc.
 */
class EnrichmentException : public TrainingException {
public:
    explicit EnrichmentException(const std::string& message,
                                TrainingErrorCode code = TrainingErrorCode::ENRICHMENT_GRAPH_NOT_INITIALIZED,
                                bool recoverable = true,
                                const std::string& context = "")
        : TrainingException(message, code, recoverable, context) {
    }
};

/**
 * @brief Exception for auto-labeling failures.
 *
 * Raised when automatic labeling of training samples fails.
 * Examples: sample has no content, LLM unavailable, label timeout,
 * invalid label format, etc.
 */
class LabelingException : public TrainingException {
public:
    explicit LabelingException(const std::string& message,
                              TrainingErrorCode code = TrainingErrorCode::LABELING_NO_CONTENT,
                              bool recoverable = false,
                              const std::string& context = "")
        : TrainingException(message, code, recoverable, context) {
    }
};

/**
 * @brief Exception for adapter serving failures.
 *
 * Raised when registering, loading, or serving adapters fails.
 * Examples: adapter not found, config invalid, load failed, hotswap timeout, etc.
 */
class ServingException : public TrainingException {
public:
    explicit ServingException(const std::string& message,
                             TrainingErrorCode code = TrainingErrorCode::SERVING_ADAPTER_NOT_FOUND,
                             bool recoverable = false,
                             const std::string& context = "")
        : TrainingException(message, code, recoverable, context) {
    }
};

/**
 * @brief Exception for provenance tracking failures.
 *
 * Raised when recording or retrieving provenance data fails.
 * Examples: storage exhausted, query failed, data corrupted, etc.
 */
class ProvenanceException : public TrainingException {
public:
    explicit ProvenanceException(const std::string& message,
                               TrainingErrorCode code = TrainingErrorCode::PROVENANCE_RECORD_INVALID,
                               bool recoverable = false,
                               const std::string& context = "")
        : TrainingException(message, code, recoverable, context) {
    }
};

/**
 * @brief Exception for dataset validation failures.
 *
 * Raised when dataset is empty, invalid, corrupted, or cannot be loaded.
 * Examples: file not found, format unsupported, sample invalid, etc.
 */
class DatasetException : public TrainingException {
public:
    explicit DatasetException(const std::string& message,
                             TrainingErrorCode code = TrainingErrorCode::DATASET_PATH_INVALID,
                             bool recoverable = false,
                             const std::string& context = "")
        : TrainingException(message, code, recoverable, context) {
    }
};

}  // namespace training
}  // namespace themis
