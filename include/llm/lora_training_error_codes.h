/**
 * @file lora_training_error_codes.h
 * @brief Error codes for LoRA training operations [7200-7299]
 * @version 0.1.0
 * @date 2026-08-17
 * 
 * This header defines specific error codes for LoRA training service operations,
 * enabling precise error handling, logging, and diagnostics.
 */

#pragma once

#include <cstdint>
#include <string>
#include <stdexcept>

namespace themis {
namespace llm {
namespace lora {

/**
 * @brief Error codes for LoRA training operations [7200-7299]
 * 
 * Codes are organized by category:
 * - 7200-7209: Initialization errors
 * - 7210-7219: Training loop errors
 * - 7220-7229: Checkpoint errors
 * - 7230-7239: Gradient/optimizer errors
 * - 7240-7249: Distributed training errors
 * - 7250-7259: Validation errors
 * - 7260-7269: Resource errors
 * - 7270-7299: General errors
 */
enum class LoRATrainingErrorCode : uint16_t {
    // ===== Initialization Errors (7200-7209) =====
    INIT_MODEL_PATH_EMPTY = 7200,           ///< Model path is empty or null
    INIT_MODEL_NOT_FOUND = 7201,            ///< Model file does not exist
    INIT_MODEL_FILE_READ_FAILED = 7202,     ///< Failed to read model file
    INIT_GGUF_FORMAT_INVALID = 7203,        ///< GGUF format is invalid or corrupted
    INIT_GGUF_HEADER_READ_FAILED = 7204,    ///< Failed to read GGUF header
    INIT_WEIGHTS_LOAD_FAILED = 7205,        ///< Failed to load model weights
    INIT_DEVICE_UNAVAILABLE = 7206,         ///< GPU/device not available, fallback to CPU failed
    INIT_MEMORY_ALLOCATION_FAILED = 7207,   ///< Memory allocation during init failed
    INIT_TRAINING_STATE_FAILED = 7208,      ///< Failed to initialize training state
    INIT_VERIFICATION_FAILED = 7209,        ///< Model verification (forward+backward) failed

    // ===== Training Loop Errors (7210-7219) =====
    TRAIN_ALREADY_IN_PROGRESS = 7210,       ///< Training already running for adapter
    TRAIN_INVALID_DATA = 7211,              ///< Invalid training data provided
    TRAIN_EMPTY_DATASET = 7212,             ///< Training dataset is empty
    TRAIN_FORWARD_PASS_FAILED = 7213,       ///< Forward pass computation failed
    TRAIN_LOSS_NAN_OR_INF = 7214,           ///< Loss became NaN or Inf
    TRAIN_DEVICE_MEMORY_EXCEEDED = 7215,    ///< GPU memory exceeded during training
    TRAIN_OUT_OF_MEMORY = 7216,             ///< Out of memory during training
    TRAIN_INTERRUPTED = 7217,               ///< Training interrupted by user
    TRAIN_TIMEOUT = 7218,                   ///< Training exceeded timeout
    TRAIN_HYPERPARAMS_INVALID = 7219,       ///< Invalid hyperparameters

    // ===== Checkpoint Errors (7220-7229) =====
    CKPT_SAVE_FAILED = 7220,                ///< Failed to save checkpoint
    CKPT_ATOMIC_WRITE_FAILED = 7221,        ///< Atomic checkpoint write failed
    CKPT_LOAD_FAILED = 7222,                ///< Failed to load checkpoint
    CKPT_VERSION_MISMATCH = 7223,           ///< Checkpoint version mismatch
    CKPT_CHECKSUM_INVALID = 7224,           ///< Checkpoint checksum verification failed
    CKPT_CORRUPTED = 7225,                  ///< Checkpoint file is corrupted
    CKPT_DIR_CREATE_FAILED = 7226,          ///< Failed to create checkpoint directory
    CKPT_FILE_WRITE_FAILED = 7227,          ///< Failed to write checkpoint file
    CKPT_METADATA_INVALID = 7228,           ///< Checkpoint metadata is invalid
    CKPT_NO_VALID_CHECKPOINT = 7229,        ///< No valid checkpoint found for recovery

    // ===== Gradient & Optimizer Errors (7230-7239) =====
    GRAD_BACKWARD_PASS_FAILED = 7230,       ///< Backward pass computation failed
    GRAD_NAN_OR_INF = 7231,                 ///< Gradient contains NaN or Inf values
    GRAD_ACCUMULATION_FAILED = 7232,        ///< Gradient accumulation failed
    GRAD_CLIPPING_FAILED = 7233,            ///< Gradient clipping computation failed
    OPT_UPDATE_FAILED = 7234,               ///< Optimizer update step failed
    OPT_STATE_INVALID = 7235,               ///< Optimizer state is invalid
    OPT_LR_SCHEDULE_FAILED = 7236,          ///< Learning rate schedule computation failed
    GRAD_SYNC_TIMEOUT = 7237,               ///< Gradient synchronization timeout (distributed)
    GRAD_SYNC_FAILED = 7238,                ///< Gradient synchronization failed (distributed)
    GRAD_PRECISION_LOST = 7239,             ///< Gradient precision loss detected

    // ===== Distributed Training Errors (7240-7249) =====
    DIST_COORDINATOR_INIT_FAILED = 7240,    ///< Failed to initialize coordinator
    DIST_COORDINATOR_UNAVAILABLE = 7241,    ///< Coordinator not available (standalone mode fallback)
    DIST_ALLREDUCE_FAILED = 7242,           ///< AllReduce operation failed
    DIST_SHARD_COMMUNICATION_FAILED = 7243, ///< Failed to communicate with shard
    DIST_INCOMPATIBLE_TOPOLOGY = 7244,      ///< Shard topology incompatible with training
    DIST_WORKER_FAILURE = 7245,             ///< Remote worker failure during training
    DIST_MODE_DETECTION_FAILED = 7246,      ///< Failed to detect standalone/distributed mode
    DIST_MIGRATION_FAILED = 7247,           ///< Failed to migrate standalone→distributed
    DIST_SYNC_BARRIER_FAILED = 7248,        ///< Synchronization barrier failed
    DIST_CONFIG_INVALID = 7249,             ///< Distributed configuration is invalid

    // ===== Validation Errors (7250-7259) =====
    VAL_EMPTY_DATASET = 7250,               ///< Validation dataset is empty
    VAL_FORWARD_PASS_FAILED = 7251,         ///< Forward pass failed during validation
    VAL_METRIC_COMPUTATION_FAILED = 7252,   ///< Failed to compute validation metrics
    VAL_METRIC_NAN_OR_INF = 7253,           ///< Validation metric is NaN or Inf
    VAL_INVALID_PREDICTIONS = 7254,         ///< Invalid model predictions during validation
    VAL_SHAPE_MISMATCH = 7255,              ///< Prediction/target shape mismatch
    VAL_TIMEOUT = 7256,                     ///< Validation exceeded timeout
    VAL_DEVICE_FAILURE = 7257,              ///< Device failure during validation
    VAL_METRIC_RANGE_INVALID = 7258,        ///< Metric value outside valid range
    VAL_INSUFFICIENT_SAMPLES = 7259,        ///< Insufficient samples for statistical significance

    // ===== Resource Errors (7260-7269) =====
    RES_GPU_MEMORY_INSUFFICIENT = 7260,     ///< Insufficient GPU memory
    RES_CPU_MEMORY_INSUFFICIENT = 7261,     ///< Insufficient CPU memory
    RES_DEVICE_INITIALIZATION_FAILED = 7262,///< Device initialization failed
    RES_DEVICE_RESET_FAILED = 7263,         ///< Device reset failed
    RES_KERNEL_LAUNCH_FAILED = 7264,        ///< GPU kernel launch failed
    RES_DISK_SPACE_EXCEEDED = 7265,         ///< Insufficient disk space for checkpoint
    RES_TIMEOUT_WAITING_FOR_DEVICE = 7266,  ///< Timeout waiting for device availability
    RES_INVALID_DEVICE_ID = 7267,           ///< Invalid GPU device ID
    RES_DEVICE_SYNC_FAILED = 7268,          ///< Device synchronization failed
    RES_PROFILE_OVERHEAD = 7269,            ///< Resource profiling overhead too high

    // ===== General Errors (7270-7299) =====
    GENERAL_INVALID_ADAPTER_ID = 7270,      ///< Invalid adapter ID format
    GENERAL_INVALID_CONFIG = 7271,          ///< Invalid configuration provided
    GENERAL_SERIALIZATION_FAILED = 7272,    ///< Failed to serialize training state
    GENERAL_DESERIALIZATION_FAILED = 7273,  ///< Failed to deserialize training state
    GENERAL_FILE_IO_ERROR = 7274,           ///< General file I/O error
    GENERAL_JSON_PARSE_ERROR = 7275,        ///< Failed to parse JSON
    GENERAL_TENSOR_OPERATION_FAILED = 7276, ///< Tensor operation failed
    GENERAL_UNWIND_SAFETY_VIOLATED = 7277,  ///< Exception safety violation detected
    GENERAL_INTERNAL_STATE_INVALID = 7278,  ///< Internal state invariant violated
    GENERAL_UNIMPLEMENTED = 7279,           ///< Feature not implemented
    GENERAL_UNKNOWN_ERROR = 7299            ///< Unknown error
};

/**
 * @brief Exception class for LoRA training operations
 * 
 * Provides structured error information with error codes, stage tracking,
 * and recovery hints for better diagnostics and recovery.
 */
class LoRATrainingException : public std::runtime_error {
public:
    /**
     * @brief Construct exception with error code and message
     * 
     * @param code Error code from LoRATrainingErrorCode
     * @param message Detailed error message
     */
    LoRATrainingException(LoRATrainingErrorCode code, const std::string& message)
        : std::runtime_error(message)
        , error_code_(code)
        , adapter_id_("")
        , stage_("")
        , recovery_hint_("")
    {}

    /**
     * @brief Construct exception with full context
     * 
     * @param code Error code
     * @param message Error message
     * @param adapter_id Associated adapter ID
     * @param stage Training stage where error occurred
     * @param recovery_hint Remediation suggestion
     */
    LoRATrainingException(
        LoRATrainingErrorCode code,
        const std::string& message,
        const std::string& adapter_id,
        const std::string& stage,
        const std::string& recovery_hint
    )
        : std::runtime_error(message)
        , error_code_(code)
        , adapter_id_(adapter_id)
        , stage_(stage)
        , recovery_hint_(recovery_hint)
    {}

    /// Get the error code
    LoRATrainingErrorCode getErrorCode() const { return error_code_; }

    /// Get the adapter ID associated with error
    const std::string& getAdapterId() const { return adapter_id_; }

    /// Get the training stage where error occurred
    const std::string& getStage() const { return stage_; }

    /// Get the recovery hint
    const std::string& getRecoveryHint() const { return recovery_hint_; }

    /// Get error code as string
    static std::string getErrorCodeString(LoRATrainingErrorCode code) {
        switch (code) {
            // Initialization
            case LoRATrainingErrorCode::INIT_MODEL_PATH_EMPTY: return "INIT_MODEL_PATH_EMPTY";
            case LoRATrainingErrorCode::INIT_MODEL_NOT_FOUND: return "INIT_MODEL_NOT_FOUND";
            case LoRATrainingErrorCode::INIT_MODEL_FILE_READ_FAILED: return "INIT_MODEL_FILE_READ_FAILED";
            case LoRATrainingErrorCode::INIT_GGUF_FORMAT_INVALID: return "INIT_GGUF_FORMAT_INVALID";
            case LoRATrainingErrorCode::INIT_GGUF_HEADER_READ_FAILED: return "INIT_GGUF_HEADER_READ_FAILED";
            case LoRATrainingErrorCode::INIT_WEIGHTS_LOAD_FAILED: return "INIT_WEIGHTS_LOAD_FAILED";
            case LoRATrainingErrorCode::INIT_DEVICE_UNAVAILABLE: return "INIT_DEVICE_UNAVAILABLE";
            case LoRATrainingErrorCode::INIT_MEMORY_ALLOCATION_FAILED: return "INIT_MEMORY_ALLOCATION_FAILED";
            case LoRATrainingErrorCode::INIT_TRAINING_STATE_FAILED: return "INIT_TRAINING_STATE_FAILED";
            case LoRATrainingErrorCode::INIT_VERIFICATION_FAILED: return "INIT_VERIFICATION_FAILED";

            // Training
            case LoRATrainingErrorCode::TRAIN_ALREADY_IN_PROGRESS: return "TRAIN_ALREADY_IN_PROGRESS";
            case LoRATrainingErrorCode::TRAIN_INVALID_DATA: return "TRAIN_INVALID_DATA";
            case LoRATrainingErrorCode::TRAIN_EMPTY_DATASET: return "TRAIN_EMPTY_DATASET";
            case LoRATrainingErrorCode::TRAIN_FORWARD_PASS_FAILED: return "TRAIN_FORWARD_PASS_FAILED";
            case LoRATrainingErrorCode::TRAIN_LOSS_NAN_OR_INF: return "TRAIN_LOSS_NAN_OR_INF";
            case LoRATrainingErrorCode::TRAIN_DEVICE_MEMORY_EXCEEDED: return "TRAIN_DEVICE_MEMORY_EXCEEDED";
            case LoRATrainingErrorCode::TRAIN_OUT_OF_MEMORY: return "TRAIN_OUT_OF_MEMORY";
            case LoRATrainingErrorCode::TRAIN_INTERRUPTED: return "TRAIN_INTERRUPTED";
            case LoRATrainingErrorCode::TRAIN_TIMEOUT: return "TRAIN_TIMEOUT";
            case LoRATrainingErrorCode::TRAIN_HYPERPARAMS_INVALID: return "TRAIN_HYPERPARAMS_INVALID";

            // Checkpoint
            case LoRATrainingErrorCode::CKPT_SAVE_FAILED: return "CKPT_SAVE_FAILED";
            case LoRATrainingErrorCode::CKPT_ATOMIC_WRITE_FAILED: return "CKPT_ATOMIC_WRITE_FAILED";
            case LoRATrainingErrorCode::CKPT_LOAD_FAILED: return "CKPT_LOAD_FAILED";
            case LoRATrainingErrorCode::CKPT_VERSION_MISMATCH: return "CKPT_VERSION_MISMATCH";
            case LoRATrainingErrorCode::CKPT_CHECKSUM_INVALID: return "CKPT_CHECKSUM_INVALID";
            case LoRATrainingErrorCode::CKPT_CORRUPTED: return "CKPT_CORRUPTED";
            case LoRATrainingErrorCode::CKPT_DIR_CREATE_FAILED: return "CKPT_DIR_CREATE_FAILED";
            case LoRATrainingErrorCode::CKPT_FILE_WRITE_FAILED: return "CKPT_FILE_WRITE_FAILED";
            case LoRATrainingErrorCode::CKPT_METADATA_INVALID: return "CKPT_METADATA_INVALID";
            case LoRATrainingErrorCode::CKPT_NO_VALID_CHECKPOINT: return "CKPT_NO_VALID_CHECKPOINT";

            // Gradients
            case LoRATrainingErrorCode::GRAD_BACKWARD_PASS_FAILED: return "GRAD_BACKWARD_PASS_FAILED";
            case LoRATrainingErrorCode::GRAD_NAN_OR_INF: return "GRAD_NAN_OR_INF";
            case LoRATrainingErrorCode::GRAD_ACCUMULATION_FAILED: return "GRAD_ACCUMULATION_FAILED";
            case LoRATrainingErrorCode::GRAD_CLIPPING_FAILED: return "GRAD_CLIPPING_FAILED";
            case LoRATrainingErrorCode::OPT_UPDATE_FAILED: return "OPT_UPDATE_FAILED";
            case LoRATrainingErrorCode::OPT_STATE_INVALID: return "OPT_STATE_INVALID";
            case LoRATrainingErrorCode::OPT_LR_SCHEDULE_FAILED: return "OPT_LR_SCHEDULE_FAILED";
            case LoRATrainingErrorCode::GRAD_SYNC_TIMEOUT: return "GRAD_SYNC_TIMEOUT";
            case LoRATrainingErrorCode::GRAD_SYNC_FAILED: return "GRAD_SYNC_FAILED";
            case LoRATrainingErrorCode::GRAD_PRECISION_LOST: return "GRAD_PRECISION_LOST";

            // Distributed
            case LoRATrainingErrorCode::DIST_COORDINATOR_INIT_FAILED: return "DIST_COORDINATOR_INIT_FAILED";
            case LoRATrainingErrorCode::DIST_COORDINATOR_UNAVAILABLE: return "DIST_COORDINATOR_UNAVAILABLE";
            case LoRATrainingErrorCode::DIST_ALLREDUCE_FAILED: return "DIST_ALLREDUCE_FAILED";
            case LoRATrainingErrorCode::DIST_SHARD_COMMUNICATION_FAILED: return "DIST_SHARD_COMMUNICATION_FAILED";
            case LoRATrainingErrorCode::DIST_INCOMPATIBLE_TOPOLOGY: return "DIST_INCOMPATIBLE_TOPOLOGY";
            case LoRATrainingErrorCode::DIST_WORKER_FAILURE: return "DIST_WORKER_FAILURE";
            case LoRATrainingErrorCode::DIST_MODE_DETECTION_FAILED: return "DIST_MODE_DETECTION_FAILED";
            case LoRATrainingErrorCode::DIST_MIGRATION_FAILED: return "DIST_MIGRATION_FAILED";
            case LoRATrainingErrorCode::DIST_SYNC_BARRIER_FAILED: return "DIST_SYNC_BARRIER_FAILED";
            case LoRATrainingErrorCode::DIST_CONFIG_INVALID: return "DIST_CONFIG_INVALID";

            // Validation
            case LoRATrainingErrorCode::VAL_EMPTY_DATASET: return "VAL_EMPTY_DATASET";
            case LoRATrainingErrorCode::VAL_FORWARD_PASS_FAILED: return "VAL_FORWARD_PASS_FAILED";
            case LoRATrainingErrorCode::VAL_METRIC_COMPUTATION_FAILED: return "VAL_METRIC_COMPUTATION_FAILED";
            case LoRATrainingErrorCode::VAL_METRIC_NAN_OR_INF: return "VAL_METRIC_NAN_OR_INF";
            case LoRATrainingErrorCode::VAL_INVALID_PREDICTIONS: return "VAL_INVALID_PREDICTIONS";
            case LoRATrainingErrorCode::VAL_SHAPE_MISMATCH: return "VAL_SHAPE_MISMATCH";
            case LoRATrainingErrorCode::VAL_TIMEOUT: return "VAL_TIMEOUT";
            case LoRATrainingErrorCode::VAL_DEVICE_FAILURE: return "VAL_DEVICE_FAILURE";
            case LoRATrainingErrorCode::VAL_METRIC_RANGE_INVALID: return "VAL_METRIC_RANGE_INVALID";
            case LoRATrainingErrorCode::VAL_INSUFFICIENT_SAMPLES: return "VAL_INSUFFICIENT_SAMPLES";

            // Resources
            case LoRATrainingErrorCode::RES_GPU_MEMORY_INSUFFICIENT: return "RES_GPU_MEMORY_INSUFFICIENT";
            case LoRATrainingErrorCode::RES_CPU_MEMORY_INSUFFICIENT: return "RES_CPU_MEMORY_INSUFFICIENT";
            case LoRATrainingErrorCode::RES_DEVICE_INITIALIZATION_FAILED: return "RES_DEVICE_INITIALIZATION_FAILED";
            case LoRATrainingErrorCode::RES_DEVICE_RESET_FAILED: return "RES_DEVICE_RESET_FAILED";
            case LoRATrainingErrorCode::RES_KERNEL_LAUNCH_FAILED: return "RES_KERNEL_LAUNCH_FAILED";
            case LoRATrainingErrorCode::RES_DISK_SPACE_EXCEEDED: return "RES_DISK_SPACE_EXCEEDED";
            case LoRATrainingErrorCode::RES_TIMEOUT_WAITING_FOR_DEVICE: return "RES_TIMEOUT_WAITING_FOR_DEVICE";
            case LoRATrainingErrorCode::RES_INVALID_DEVICE_ID: return "RES_INVALID_DEVICE_ID";
            case LoRATrainingErrorCode::RES_DEVICE_SYNC_FAILED: return "RES_DEVICE_SYNC_FAILED";
            case LoRATrainingErrorCode::RES_PROFILE_OVERHEAD: return "RES_PROFILE_OVERHEAD";

            // General
            case LoRATrainingErrorCode::GENERAL_INVALID_ADAPTER_ID: return "GENERAL_INVALID_ADAPTER_ID";
            case LoRATrainingErrorCode::GENERAL_INVALID_CONFIG: return "GENERAL_INVALID_CONFIG";
            case LoRATrainingErrorCode::GENERAL_SERIALIZATION_FAILED: return "GENERAL_SERIALIZATION_FAILED";
            case LoRATrainingErrorCode::GENERAL_DESERIALIZATION_FAILED: return "GENERAL_DESERIALIZATION_FAILED";
            case LoRATrainingErrorCode::GENERAL_FILE_IO_ERROR: return "GENERAL_FILE_IO_ERROR";
            case LoRATrainingErrorCode::GENERAL_JSON_PARSE_ERROR: return "GENERAL_JSON_PARSE_ERROR";
            case LoRATrainingErrorCode::GENERAL_TENSOR_OPERATION_FAILED: return "GENERAL_TENSOR_OPERATION_FAILED";
            case LoRATrainingErrorCode::GENERAL_UNWIND_SAFETY_VIOLATED: return "GENERAL_UNWIND_SAFETY_VIOLATED";
            case LoRATrainingErrorCode::GENERAL_INTERNAL_STATE_INVALID: return "GENERAL_INTERNAL_STATE_INVALID";
            case LoRATrainingErrorCode::GENERAL_UNIMPLEMENTED: return "GENERAL_UNIMPLEMENTED";
            case LoRATrainingErrorCode::GENERAL_UNKNOWN_ERROR: return "GENERAL_UNKNOWN_ERROR";

            default: return "UNKNOWN_ERROR";
        }
    }

    /// Get formatted error message with full context
    std::string getFormattedMessage() const {
        std::string msg = "LoRA Training Error [" + getErrorCodeString(error_code_) + "]: " + what();
        if (!adapter_id_.empty()) {
          msg += " (adapter=" + adapter_id_ + ")";
        }
        if (!stage_.empty()) {
          msg += " (stage=" + stage_ + ")";
        }
        if (!recovery_hint_.empty()) {
          msg += " -> " + recovery_hint_;
        }
        return msg;
    }

private:
    LoRATrainingErrorCode error_code_;
    std::string adapter_id_;
    std::string stage_;
    std::string recovery_hint_;
};

/**
 * @brief Helper function to check if value is finite (not NaN or Inf)
 */
inline bool isFiniteValue(float value) {
    return std::isfinite(value) && !std::isnan(value) && !std::isinf(value);
}

/**
 * @brief Helper function to validate gradient magnitude
 */
inline bool isValidGradientMagnitude(float grad_magnitude) {
    return isFiniteValue(grad_magnitude) && grad_magnitude >= 0.0f;
}

} // namespace lora
} // namespace llm
} // namespace themis
