/**
 * @file training_error_diagnostics.h
 * @brief Training Module Error Diagnostics and Structured Logging (Phase 3)
 *
 * Provides utilities for capturing and formatting diagnostic information
 * for training module failures, enabling production troubleshooting and
 * structured error reporting.
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

#include <string>
#include <sstream>
#include <chrono>
#include <ctime>
#include <vector>

namespace themis {
namespace training {

/**
 * @brief Captures diagnostic context for structured error reporting.
 *
 * Used throughout the training module to capture operation-specific context
 * that aids in post-mortem analysis and production troubleshooting.
 *
 * Example usage:
 * @code
 * TrainingDiagnostics diag;
 * diag.operation("checkpoint_save")
 *     .input("path", checkpoint_path)
 *     .input("size_bytes", file_size)
 *     .add_note("attempted 3 retries on ENOSPC");
 * throw CheckpointException(
 *     "Failed to save checkpoint: disk full",
 *     TrainingErrorCode::CHECKPOINT_DISK_SPACE_EXHAUSTED,
 *     false,  // not recoverable without operator action
 *     diag.to_string()
 * );
 * @endcode
 */
class TrainingDiagnostics {
public:
    TrainingDiagnostics() {
        timestamp_ = std::time(nullptr);
    }

    /**
     * @brief Set the operation name (e.g., "checkpoint_save", "training_step").
     */
    TrainingDiagnostics& operation(const std::string& op) {
        operation_ = op;
        return *this;
    }

    /**
     * @brief Add an input parameter or state variable.
     *
     * @param key Parameter name.
     * @param value Parameter value (as string).
     */
    TrainingDiagnostics& input(const std::string& key, const std::string& value) {
        inputs_.push_back({key, value});
        return *this;
    }

    /**
     * @brief Add a numeric input parameter.
     */
    TrainingDiagnostics& input(const std::string& key, int64_t value) {
        return input(key, std::to_string(value));
    }

    /**
     * @brief Add a numeric input parameter.
     */
    TrainingDiagnostics& input(const std::string& key, double value) {
        std::ostringstream oss = {};
        oss << value;
        return input(key, oss.str());
    }

    /**
     * @brief Add a boolean input parameter.
     */
    TrainingDiagnostics& input(const std::string& key, bool value) {
        return input(key, value ? "true" : "false");
    }

    /**
     * @brief Add a diagnostic note or observation.
     *
     * @param note Human-readable note describing the failure or observed state.
     */
    TrainingDiagnostics& add_note(const std::string& note) {
        notes_.push_back(note);
        return *this;
    }

    /**
     * @brief Set error code.
     */
    TrainingDiagnostics& error_code(TrainingErrorCode code) {
        error_code_ = code;
        return *this;
    }

    /**
     * @brief Set whether error is recoverable.
     */
    TrainingDiagnostics& recoverable(bool is_recoverable) {
        recoverable_ = is_recoverable;
        return *this;
    }

    /**
     * @brief Get the timestamp as ISO8601 string.
     */
    std::string timestamp_string() const {
        char buf[32];
        std::tm* tm_info = std::localtime(&timestamp_);
        std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", tm_info);
        return std::string(buf);
    }

    /**
     * @brief Convert diagnostics to formatted string.
     *
     * Format:
     * ```
     * [TIMESTAMP] operation="{operation}" error_code="{code}"
     * Inputs: {key1}={value1}, {key2}={value2}, ...
     * Notes: {note1}; {note2}; ...
     * ```
     */
    std::string to_string() const {
        std::ostringstream oss = {};

        oss << "[" << timestamp_string() << "] ";
        
        if (!operation_.empty()) {
            oss << "operation=\"" << operation_ << "\" ";
        }
        
        if (error_code_ != TrainingErrorCode::SUCCESS) {
            oss << "error_code=" << trainingErrorCodeToString(error_code_) << " ";
        }

        oss << "recoverable=" << (recoverable_ ? "true" : "false");

        if (!inputs_.empty()) {
            oss << "\nInputs: ";
            for (size_t i = 0; i < inputs_.size(); ++i) {
                if (i > 0) {
                  oss << ", ";
                }
                oss << inputs_[i].first << "=" << inputs_[i].second;
            }
        }

        if (!notes_.empty()) {
            oss << "\nNotes: ";
            for (size_t i = 0; i < notes_.size(); ++i) {
                if (i > 0) {
                  oss << "; ";
                }
                oss << notes_[i];
            }
        }

        return oss.str();
    }

private:
    std::time_t timestamp_;
    std::string operation_;
    std::vector<std::pair<std::string, std::string>> inputs_;
    std::vector<std::string> notes_;
    TrainingErrorCode error_code_ = TrainingErrorCode::SUCCESS;
    bool recoverable_ = false;
};

/**
 * @brief Helper for structured error logging at various log levels.
 *
 * Used by implementation to emit consistent diagnostic output to logs.
 * Logs should include the error code, message, and context for
 * production troubleshooting.
 *
 * Example usage:
 * @code
 * THEMIS_LOG_ERROR() << TrainingErrorLogger(
 *     TrainingErrorCode::CHECKPOINT_SHA256_MISMATCH,
 *     "Checkpoint validation failed: SHA-256 mismatch",
 *     "checkpoint_path=/var/lib/themis/ckpt_legal_v1.bin, expected_sha=abc..., got=def..."
 * );
 * @endcode
 */
class TrainingErrorLogger {
public:
    TrainingErrorLogger(TrainingErrorCode code,
                       const std::string& message,
                       const std::string& context = "")
        : code_(code), message_(message), context_(context) {
    }

    /**
     * @brief Convert to loggable string.
     */
    std::string to_log_string() const {
        std::string result = "[" + trainingErrorCodeToString(code_) + "] " + message_;
        if (!context_.empty()) {
            result += " | " + context_;
        }
        return result;
    }

    /**
     * @brief Stream insertion operator for easy logging.
     */
    friend std::ostream& operator<<(std::ostream& os,
                                    const TrainingErrorLogger& logger) {
        os << logger.to_log_string();
        return os;
    }

private:
    TrainingErrorCode code_;
    std::string message_;
    std::string context_;
};

}  // namespace training
}  // namespace themis
