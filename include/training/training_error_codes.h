/**
 * @file training_error_codes.h
 * @brief Training Module Error Taxonomy and Codes (Phase 3)
 *
 * Defines comprehensive error codes for all training module components,
 * organized hierarchically by operation class and fault mode.
 *
 * Error Code Layout:
 * - 0x8000-0x80FF: Checkpoint management errors
 * - 0x8100-0x81FF: Training execution errors
 * - 0x8200-0x82FF: Adapter merge errors
 * - 0x8300-0x83FF: Knowledge graph enrichment errors
 * - 0x8400-0x84FF: Auto-labeling errors
 * - 0x8500-0x85FF: Adapter serving errors
 * - 0x8600-0x86FF: Provenance tracking errors
 * - 0x8700-0x87FF: Data/Dataset validation errors
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

#include <cstdint>
#include <string>
#include <unordered_map>

namespace themis {
namespace training {

/**
 * @brief Comprehensive error code enumeration for training module.
 *
 * All errors follow a hierarchical pattern: 0x8XYZ where:
 * - X: component (0=checkpoint, 1=training, 2=merge, 3=enrichment, 4=labeling, 5=serving, 6=provenance, 7=dataset)
 * - Y: error category (0=denial/invalid, 1=resource, 2=io, 3=timeout, 4=corruption, 5=incompatibility, 6=state)
 * - Z: specific error within category
 *
 * Error Classification:
 * - DENIAL: Operation cannot proceed due to invalid input or constraint violation
 * - RESOURCE: Memory, disk, GPU, or other resource exhaustion
 * - IO: File system, network, or I/O operation failure
 * - TIMEOUT: Operation exceeded time limit
 * - CORRUPTION: Data corruption or integrity violation detected
 * - INCOMPATIBILITY: Version, size, or format mismatch
 * - STATE: Invalid state transition or precondition not met
 */
enum class TrainingErrorCode : uint32_t {
    // ========================================================================
    // Sentinel
    // ========================================================================
    SUCCESS = 0x0000,

    // ========================================================================
    // 0x8000-0x80FF: CHECKPOINT MANAGEMENT ERRORS
    // ========================================================================
    
    // DENIAL (0x80X0)
    /// Checkpoint directory is empty or invalid
    CHECKPOINT_DIR_INVALID = 0x8000,
    /// Checkpoint path contains path traversal or unsafe characters
    CHECKPOINT_PATH_UNSAFE = 0x8001,
    /// Checkpoint manifest is corrupted or unparseable
    CHECKPOINT_MANIFEST_INVALID = 0x8002,
    /// Requested checkpoint entry not found in manifest
    CHECKPOINT_NOT_FOUND = 0x8003,
    /// Checkpoint size is below minimum threshold
    CHECKPOINT_SIZE_INVALID = 0x8004,
    /// Checkpoint metadata contains invalid values
    CHECKPOINT_METADATA_INVALID = 0x8005,

    // RESOURCE (0x80X1)
    /// Insufficient disk space for checkpoint save
    CHECKPOINT_DISK_SPACE_EXHAUSTED = 0x8011,
    /// Memory allocation failed during checkpoint processing
    CHECKPOINT_MEMORY_EXHAUSTED = 0x8012,
    /// Checkpoint file handle limit exceeded
    CHECKPOINT_FD_LIMIT_EXCEEDED = 0x8013,

    // IO (0x80X2)
    /// Failed to read checkpoint file
    CHECKPOINT_READ_FAILED = 0x8022,
    /// Failed to write checkpoint file
    CHECKPOINT_WRITE_FAILED = 0x8023,
    /// Failed to rename/move checkpoint file
    CHECKPOINT_RENAME_FAILED = 0x8024,
    /// Failed to delete checkpoint file
    CHECKPOINT_DELETE_FAILED = 0x8025,
    /// Checkpoint manifest read/write failed
    CHECKPOINT_MANIFEST_IO_FAILED = 0x8026,

    // TIMEOUT (0x80X3)
    /// Checkpoint SHA-256 validation timeout
    CHECKPOINT_VALIDATION_TIMEOUT = 0x8033,
    /// Checkpoint I/O operation exceeded timeout
    CHECKPOINT_IO_TIMEOUT = 0x8034,

    // CORRUPTION (0x80X4)
    /// Checkpoint SHA-256 mismatch detected
    CHECKPOINT_SHA256_MISMATCH = 0x8044,
    /// Checkpoint file is truncated or incomplete
    CHECKPOINT_TRUNCATED = 0x8045,
    /// Checkpoint magic number or header invalid
    CHECKPOINT_INVALID_FORMAT = 0x8046,

    // INCOMPATIBILITY (0x80X5)
    /// Base model hash mismatch; checkpoint from different model
    CHECKPOINT_MODEL_MISMATCH = 0x8055,
    /// Checkpoint adapter version incompatible
    CHECKPOINT_VERSION_INCOMPATIBLE = 0x8056,
    /// Checkpoint format version unsupported
    CHECKPOINT_FORMAT_VERSION_UNSUPPORTED = 0x8057,

    // STATE (0x80X6)
    /// Attempted resume but no valid checkpoint exists
    CHECKPOINT_NO_VALID_CHECKPOINT = 0x8066,
    /// Checkpoint recovery exhausted all fallback entries
    CHECKPOINT_RECOVERY_FAILED = 0x8067,

    // ========================================================================
    // 0x8100-0x81FF: TRAINING EXECUTION ERRORS
    // ========================================================================

    // DENIAL (0x81X0)
    /// Training configuration is invalid or incomplete
    TRAINING_CONFIG_INVALID = 0x8100,
    /// Training dataset is empty
    TRAINING_DATASET_EMPTY = 0x8101,
    /// Training dataset contains invalid samples
    TRAINING_DATASET_CORRUPTED = 0x8102,
    /// Model configuration is invalid
    TRAINING_MODEL_CONFIG_INVALID = 0x8103,
    /// Learning rate is out of valid range
    TRAINING_LEARNING_RATE_INVALID = 0x8104,
    /// Batch size exceeds model or memory limits
    TRAINING_BATCH_SIZE_INVALID = 0x8105,
    /// Training mode is unsupported
    TRAINING_MODE_UNSUPPORTED = 0x8106,

    // RESOURCE (0x81X1)
    /// GPU memory exhausted during training
    TRAINING_GPU_MEMORY_EXHAUSTED = 0x8111,
    /// System memory exhausted during training
    TRAINING_SYSTEM_MEMORY_EXHAUSTED = 0x8112,
    /// GPU not available or not initialized
    TRAINING_GPU_UNAVAILABLE = 0x8113,
    /// Maximum number of training threads/workers exceeded
    TRAINING_MAX_WORKERS_EXCEEDED = 0x8114,

    // IO (0x81X2)
    /// Failed to read training data batch
    TRAINING_DATA_READ_FAILED = 0x8122,
    /// Failed to write training metrics/logs
    TRAINING_METRICS_WRITE_FAILED = 0x8123,
    /// Training checkpoint save failed
    TRAINING_CHECKPOINT_SAVE_FAILED = 0x8124,

    // TIMEOUT (0x81X3)
    /// Training step exceeded timeout
    TRAINING_STEP_TIMEOUT = 0x8133,
    /// Entire training epoch exceeded timeout
    TRAINING_EPOCH_TIMEOUT = 0x8134,
    /// Data loading for batch exceeded timeout
    TRAINING_DATA_LOAD_TIMEOUT = 0x8135,

    // CORRUPTION (0x81X4)
    /// Detected NaN in training loss
    TRAINING_LOSS_NAN = 0x8144,
    /// Detected Inf in training loss
    TRAINING_LOSS_INF = 0x8145,
    /// Training metrics contain invalid values
    TRAINING_METRICS_INVALID = 0x8146,

    // INCOMPATIBILITY (0x81X5)
    /// Adapter rank incompatible with model
    TRAINING_ADAPTER_RANK_INCOMPATIBLE = 0x8155,
    /// Base model version incompatible with trainer
    TRAINING_MODEL_VERSION_INCOMPATIBLE = 0x8156,
    /// Quantization type not supported by trainer
    TRAINING_QUANTIZATION_UNSUPPORTED = 0x8157,

    // STATE (0x81X6)
    /// Training already in progress
    TRAINING_ALREADY_RUNNING = 0x8166,
    /// Training cancelled by user request
    TRAINING_CANCELLED = 0x8167,
    /// Training not initialized before step attempt
    TRAINING_NOT_INITIALIZED = 0x8168,

    // ========================================================================
    // 0x8200-0x82FF: ADAPTER MERGE ERRORS
    // ========================================================================

    // DENIAL (0x82X0)
    /// No adapters provided for merge
    MERGE_NO_ADAPTERS = 0x8200,
    /// Merge weights do not sum to expected value
    MERGE_WEIGHTS_INVALID = 0x8201,
    /// Adapter dimensions incompatible for merge
    MERGE_ADAPTER_DIMENSION_MISMATCH = 0x8202,
    /// Merge strategy not supported
    MERGE_STRATEGY_UNSUPPORTED = 0x8203,
    /// Adapter configuration incompatible for merge
    MERGE_ADAPTER_CONFIG_INCOMPATIBLE = 0x8204,

    // RESOURCE (0x82X1)
    /// GPU memory exhausted during merge
    MERGE_GPU_MEMORY_EXHAUSTED = 0x8211,
    /// System memory exhausted during merge
    MERGE_SYSTEM_MEMORY_EXHAUSTED = 0x8212,

    // IO (0x82X2)
    /// Failed to read adapter for merge
    MERGE_ADAPTER_READ_FAILED = 0x8222,
    /// Failed to write merged adapter
    MERGE_OUTPUT_WRITE_FAILED = 0x8223,

    // TIMEOUT (0x82X3)
    /// Merge operation exceeded timeout
    MERGE_TIMEOUT = 0x8233,

    // CORRUPTION (0x82X4)
    /// Detected NaN during merge computation
    MERGE_COMPUTATION_NAN = 0x8244,
    /// Detected Inf during merge computation
    MERGE_COMPUTATION_INF = 0x8245,
    /// Merge result validation failed
    MERGE_RESULT_INVALID = 0x8246,

    // INCOMPATIBILITY (0x82X5)
    /// Adapter base model mismatch
    MERGE_BASE_MODEL_MISMATCH = 0x8255,
    /// Adapter version mismatch
    MERGE_ADAPTER_VERSION_MISMATCH = 0x8256,

    // STATE (0x82X6)
    /// Merge conflicts detected; rollback required
    MERGE_CONFLICTS_DETECTED = 0x8266,
    /// Merge in inconsistent state
    MERGE_STATE_INCONSISTENT = 0x8267,

    // ========================================================================
    // 0x8300-0x83FF: KNOWLEDGE GRAPH ENRICHMENT ERRORS
    // ========================================================================

    // DENIAL (0x83X0)
    /// Training data has no enrichable content
    ENRICHMENT_NO_ENRICHABLE_CONTENT = 0x8300,
    /// Knowledge graph not initialized
    ENRICHMENT_GRAPH_NOT_INITIALIZED = 0x8301,
    /// Graph query is malformed
    ENRICHMENT_GRAPH_QUERY_INVALID = 0x8302,
    /// Enrichment configuration is invalid
    ENRICHMENT_CONFIG_INVALID = 0x8303,

    // RESOURCE (0x83X1)
    /// Knowledge graph cache memory exhausted
    ENRICHMENT_CACHE_MEMORY_EXHAUSTED = 0x8311,
    /// Graph query result set too large
    ENRICHMENT_RESULT_SET_TOO_LARGE = 0x8312,

    // IO (0x83X2)
    /// Failed to query knowledge graph
    ENRICHMENT_GRAPH_QUERY_FAILED = 0x8322,
    /// Failed to load enrichment cache
    ENRICHMENT_CACHE_LOAD_FAILED = 0x8323,

    // TIMEOUT (0x83X3)
    /// Graph enrichment query timeout
    ENRICHMENT_QUERY_TIMEOUT = 0x8333,
    /// Cache loading exceeded timeout
    ENRICHMENT_CACHE_LOAD_TIMEOUT = 0x8334,

    // CORRUPTION (0x83X4)
    /// Enrichment cache is corrupted
    ENRICHMENT_CACHE_CORRUPTED = 0x8344,
    /// Enriched data contains invalid values
    ENRICHMENT_RESULT_INVALID = 0x8345,

    // STATE (0x83X6)
    /// Enrichment cache miss; fallback applied
    ENRICHMENT_CACHE_MISS = 0x8366,
    /// Graph connection lost during enrichment
    ENRICHMENT_GRAPH_DISCONNECTED = 0x8367,

    // ========================================================================
    // 0x8400-0x84FF: AUTO-LABELING ERRORS
    // ========================================================================

    // DENIAL (0x84X0)
    /// Sample has no content to label
    LABELING_NO_CONTENT = 0x8400,
    /// Sample domain type is not supported
    LABELING_DOMAIN_UNSUPPORTED = 0x8401,
    /// Labeler configuration is invalid
    LABELING_CONFIG_INVALID = 0x8402,
    /// Label class set is empty
    LABELING_CLASS_SET_EMPTY = 0x8403,
    /// Sample content is too short or malformed
    LABELING_CONTENT_INVALID = 0x8404,

    // RESOURCE (0x84X1)
    /// LLM model not available for labeling
    LABELING_LLM_UNAVAILABLE = 0x8411,
    /// Label generation queue full
    LABELING_QUEUE_FULL = 0x8412,

    // IO (0x84X2)
    /// Failed to read sample for labeling
    LABELING_SAMPLE_READ_FAILED = 0x8422,
    /// Failed to write generated labels
    LABELING_OUTPUT_WRITE_FAILED = 0x8423,

    // TIMEOUT (0x84X3)
    /// Label generation exceeded timeout
    LABELING_TIMEOUT = 0x8433,

    // CORRUPTION (0x84X4)
    /// Generated label is malformed
    LABELING_RESULT_INVALID = 0x8444,
    /// Label confidence score is invalid
    LABELING_CONFIDENCE_INVALID = 0x8445,

    // STATE (0x84X6)
    /// Labeling already in progress
    LABELING_ALREADY_RUNNING = 0x8466,
    /// Labeler not initialized
    LABELING_NOT_INITIALIZED = 0x8467,

    // ========================================================================
    // 0x8500-0x85FF: ADAPTER SERVING ERRORS
    // ========================================================================

    // DENIAL (0x85X0)
    /// Adapter not found in serving registry
    SERVING_ADAPTER_NOT_FOUND = 0x8500,
    /// Serving port or endpoint is invalid
    SERVING_ENDPOINT_INVALID = 0x8501,
    /// Server configuration is invalid
    SERVING_CONFIG_INVALID = 0x8502,
    /// Adapter is incompatible with serving runtime
    SERVING_ADAPTER_INCOMPATIBLE = 0x8503,

    // RESOURCE (0x85X1)
    /// Server memory pool exhausted
    SERVING_MEMORY_EXHAUSTED = 0x8511,
    /// Maximum concurrent requests exceeded
    SERVING_MAX_CONNECTIONS_EXCEEDED = 0x8512,
    /// GPU memory full for adapter serving
    SERVING_GPU_MEMORY_EXHAUSTED = 0x8513,

    // IO (0x85X2)
    /// Failed to load adapter for serving
    SERVING_ADAPTER_LOAD_FAILED = 0x8522,
    /// Failed to unload adapter
    SERVING_ADAPTER_UNLOAD_FAILED = 0x8523,

    // TIMEOUT (0x85X3)
    /// Request processing exceeded timeout
    SERVING_REQUEST_TIMEOUT = 0x8533,
    /// Hot-swap operation exceeded timeout
    SERVING_HOTSWAP_TIMEOUT = 0x8534,

    // CORRUPTION (0x85X4)
    /// Loaded adapter is corrupted or invalid
    SERVING_ADAPTER_CORRUPTED = 0x8544,

    // STATE (0x85X6)
    /// Server not running or not initialized
    SERVING_NOT_INITIALIZED = 0x8566,
    /// Adapter already loaded
    SERVING_ADAPTER_ALREADY_LOADED = 0x8567,
    /// Hot-swap in progress; operation blocked
    SERVING_HOTSWAP_IN_PROGRESS = 0x8568,

    // ========================================================================
    // 0x8600-0x86FF: PROVENANCE TRACKING ERRORS
    // ========================================================================

    // DENIAL (0x86X0)
    /// Provenance record is invalid or incomplete
    PROVENANCE_RECORD_INVALID = 0x8600,
    /// Provenance data source not available
    PROVENANCE_SOURCE_UNAVAILABLE = 0x8601,
    /// Provenance query is malformed
    PROVENANCE_QUERY_INVALID = 0x8602,

    // RESOURCE (0x86X1)
    /// Provenance storage exhausted
    PROVENANCE_STORAGE_EXHAUSTED = 0x8611,
    /// Provenance record buffer full
    PROVENANCE_BUFFER_FULL = 0x8612,

    // IO (0x86X2)
    /// Failed to write provenance record
    PROVENANCE_WRITE_FAILED = 0x8622,
    /// Failed to read provenance data
    PROVENANCE_READ_FAILED = 0x8623,

    // TIMEOUT (0x86X3)
    /// Provenance query exceeded timeout
    PROVENANCE_QUERY_TIMEOUT = 0x8633,

    // CORRUPTION (0x86X4)
    /// Provenance data is corrupted
    PROVENANCE_DATA_CORRUPTED = 0x8644,
    /// Provenance chain integrity violated
    PROVENANCE_CHAIN_BROKEN = 0x8645,

    // STATE (0x86X6)
    /// Provenance tracking not initialized
    PROVENANCE_NOT_INITIALIZED = 0x8666,

    // ========================================================================
    // 0x8700-0x87FF: DATASET VALIDATION ERRORS
    // ========================================================================

    // DENIAL (0x87X0)
    /// Dataset path is invalid or empty
    DATASET_PATH_INVALID = 0x8700,
    /// Dataset format is not supported
    DATASET_FORMAT_UNSUPPORTED = 0x8701,
    /// Dataset contains no samples
    DATASET_EMPTY = 0x8702,
    /// Dataset sample validation failed
    DATASET_SAMPLE_INVALID = 0x8703,
    /// Dataset splits are misconfigured
    DATASET_SPLITS_INVALID = 0x8704,

    // RESOURCE (0x87X1)
    /// Dataset loading exhausted available memory
    DATASET_MEMORY_EXHAUSTED = 0x8711,
    /// Dataset too large for available storage
    DATASET_STORAGE_EXHAUSTED = 0x8712,

    // IO (0x87X2)
    /// Failed to read dataset file
    DATASET_READ_FAILED = 0x8722,
    /// Failed to parse dataset format
    DATASET_PARSE_FAILED = 0x8723,

    // TIMEOUT (0x87X3)
    /// Dataset loading exceeded timeout
    DATASET_LOAD_TIMEOUT = 0x8733,

    // CORRUPTION (0x87X4)
    /// Dataset file is corrupted
    DATASET_FILE_CORRUPTED = 0x8744,
    /// Dataset manifest contains invalid entries
    DATASET_MANIFEST_CORRUPTED = 0x8745,

    // STATE (0x87X6)
    /// Dataset iterator exhausted
    DATASET_ITERATOR_EXHAUSTED = 0x8766,
};

/**
 * @brief Convert TrainingErrorCode to human-readable string.
 *
 * @param code Error code to convert.
 * @return Descriptive string for the error code.
 */
inline std::string trainingErrorCodeToString(TrainingErrorCode code) {
    static const std::unordered_map<uint32_t, std::string> kErrorMap{
        // Checkpoint errors
        {0x8000, "CHECKPOINT_DIR_INVALID"},
        {0x8001, "CHECKPOINT_PATH_UNSAFE"},
        {0x8002, "CHECKPOINT_MANIFEST_INVALID"},
        {0x8003, "CHECKPOINT_NOT_FOUND"},
        {0x8004, "CHECKPOINT_SIZE_INVALID"},
        {0x8005, "CHECKPOINT_METADATA_INVALID"},
        {0x8011, "CHECKPOINT_DISK_SPACE_EXHAUSTED"},
        {0x8012, "CHECKPOINT_MEMORY_EXHAUSTED"},
        {0x8013, "CHECKPOINT_FD_LIMIT_EXCEEDED"},
        {0x8022, "CHECKPOINT_READ_FAILED"},
        {0x8023, "CHECKPOINT_WRITE_FAILED"},
        {0x8024, "CHECKPOINT_RENAME_FAILED"},
        {0x8025, "CHECKPOINT_DELETE_FAILED"},
        {0x8026, "CHECKPOINT_MANIFEST_IO_FAILED"},
        {0x8033, "CHECKPOINT_VALIDATION_TIMEOUT"},
        {0x8034, "CHECKPOINT_IO_TIMEOUT"},
        {0x8044, "CHECKPOINT_SHA256_MISMATCH"},
        {0x8045, "CHECKPOINT_TRUNCATED"},
        {0x8046, "CHECKPOINT_INVALID_FORMAT"},
        {0x8055, "CHECKPOINT_MODEL_MISMATCH"},
        {0x8056, "CHECKPOINT_VERSION_INCOMPATIBLE"},
        {0x8057, "CHECKPOINT_FORMAT_VERSION_UNSUPPORTED"},
        {0x8066, "CHECKPOINT_NO_VALID_CHECKPOINT"},
        {0x8067, "CHECKPOINT_RECOVERY_FAILED"},

        // Training errors
        {0x8100, "TRAINING_CONFIG_INVALID"},
        {0x8101, "TRAINING_DATASET_EMPTY"},
        {0x8102, "TRAINING_DATASET_CORRUPTED"},
        {0x8103, "TRAINING_MODEL_CONFIG_INVALID"},
        {0x8104, "TRAINING_LEARNING_RATE_INVALID"},
        {0x8105, "TRAINING_BATCH_SIZE_INVALID"},
        {0x8106, "TRAINING_MODE_UNSUPPORTED"},
        {0x8111, "TRAINING_GPU_MEMORY_EXHAUSTED"},
        {0x8112, "TRAINING_SYSTEM_MEMORY_EXHAUSTED"},
        {0x8113, "TRAINING_GPU_UNAVAILABLE"},
        {0x8114, "TRAINING_MAX_WORKERS_EXCEEDED"},
        {0x8122, "TRAINING_DATA_READ_FAILED"},
        {0x8123, "TRAINING_METRICS_WRITE_FAILED"},
        {0x8124, "TRAINING_CHECKPOINT_SAVE_FAILED"},
        {0x8133, "TRAINING_STEP_TIMEOUT"},
        {0x8134, "TRAINING_EPOCH_TIMEOUT"},
        {0x8135, "TRAINING_DATA_LOAD_TIMEOUT"},
        {0x8144, "TRAINING_LOSS_NAN"},
        {0x8145, "TRAINING_LOSS_INF"},
        {0x8146, "TRAINING_METRICS_INVALID"},
        {0x8155, "TRAINING_ADAPTER_RANK_INCOMPATIBLE"},
        {0x8156, "TRAINING_MODEL_VERSION_INCOMPATIBLE"},
        {0x8157, "TRAINING_QUANTIZATION_UNSUPPORTED"},
        {0x8166, "TRAINING_ALREADY_RUNNING"},
        {0x8167, "TRAINING_CANCELLED"},
        {0x8168, "TRAINING_NOT_INITIALIZED"},

        // Merge errors
        {0x8200, "MERGE_NO_ADAPTERS"},
        {0x8201, "MERGE_WEIGHTS_INVALID"},
        {0x8202, "MERGE_ADAPTER_DIMENSION_MISMATCH"},
        {0x8203, "MERGE_STRATEGY_UNSUPPORTED"},
        {0x8204, "MERGE_ADAPTER_CONFIG_INCOMPATIBLE"},
        {0x8211, "MERGE_GPU_MEMORY_EXHAUSTED"},
        {0x8212, "MERGE_SYSTEM_MEMORY_EXHAUSTED"},
        {0x8222, "MERGE_ADAPTER_READ_FAILED"},
        {0x8223, "MERGE_OUTPUT_WRITE_FAILED"},
        {0x8233, "MERGE_TIMEOUT"},
        {0x8244, "MERGE_COMPUTATION_NAN"},
        {0x8245, "MERGE_COMPUTATION_INF"},
        {0x8246, "MERGE_RESULT_INVALID"},
        {0x8255, "MERGE_BASE_MODEL_MISMATCH"},
        {0x8256, "MERGE_ADAPTER_VERSION_MISMATCH"},
        {0x8266, "MERGE_CONFLICTS_DETECTED"},
        {0x8267, "MERGE_STATE_INCONSISTENT"},

        // Enrichment errors
        {0x8300, "ENRICHMENT_NO_ENRICHABLE_CONTENT"},
        {0x8301, "ENRICHMENT_GRAPH_NOT_INITIALIZED"},
        {0x8302, "ENRICHMENT_GRAPH_QUERY_INVALID"},
        {0x8303, "ENRICHMENT_CONFIG_INVALID"},
        {0x8311, "ENRICHMENT_CACHE_MEMORY_EXHAUSTED"},
        {0x8312, "ENRICHMENT_RESULT_SET_TOO_LARGE"},
        {0x8322, "ENRICHMENT_GRAPH_QUERY_FAILED"},
        {0x8323, "ENRICHMENT_CACHE_LOAD_FAILED"},
        {0x8333, "ENRICHMENT_QUERY_TIMEOUT"},
        {0x8334, "ENRICHMENT_CACHE_LOAD_TIMEOUT"},
        {0x8344, "ENRICHMENT_CACHE_CORRUPTED"},
        {0x8345, "ENRICHMENT_RESULT_INVALID"},
        {0x8366, "ENRICHMENT_CACHE_MISS"},
        {0x8367, "ENRICHMENT_GRAPH_DISCONNECTED"},

        // Labeling errors
        {0x8400, "LABELING_NO_CONTENT"},
        {0x8401, "LABELING_DOMAIN_UNSUPPORTED"},
        {0x8402, "LABELING_CONFIG_INVALID"},
        {0x8403, "LABELING_CLASS_SET_EMPTY"},
        {0x8404, "LABELING_CONTENT_INVALID"},
        {0x8411, "LABELING_LLM_UNAVAILABLE"},
        {0x8412, "LABELING_QUEUE_FULL"},
        {0x8422, "LABELING_SAMPLE_READ_FAILED"},
        {0x8423, "LABELING_OUTPUT_WRITE_FAILED"},
        {0x8433, "LABELING_TIMEOUT"},
        {0x8444, "LABELING_RESULT_INVALID"},
        {0x8445, "LABELING_CONFIDENCE_INVALID"},
        {0x8466, "LABELING_ALREADY_RUNNING"},
        {0x8467, "LABELING_NOT_INITIALIZED"},

        // Serving errors
        {0x8500, "SERVING_ADAPTER_NOT_FOUND"},
        {0x8501, "SERVING_ENDPOINT_INVALID"},
        {0x8502, "SERVING_CONFIG_INVALID"},
        {0x8503, "SERVING_ADAPTER_INCOMPATIBLE"},
        {0x8511, "SERVING_MEMORY_EXHAUSTED"},
        {0x8512, "SERVING_MAX_CONNECTIONS_EXCEEDED"},
        {0x8513, "SERVING_GPU_MEMORY_EXHAUSTED"},
        {0x8522, "SERVING_ADAPTER_LOAD_FAILED"},
        {0x8523, "SERVING_ADAPTER_UNLOAD_FAILED"},
        {0x8533, "SERVING_REQUEST_TIMEOUT"},
        {0x8534, "SERVING_HOTSWAP_TIMEOUT"},
        {0x8544, "SERVING_ADAPTER_CORRUPTED"},
        {0x8566, "SERVING_NOT_INITIALIZED"},
        {0x8567, "SERVING_ADAPTER_ALREADY_LOADED"},
        {0x8568, "SERVING_HOTSWAP_IN_PROGRESS"},

        // Provenance errors
        {0x8600, "PROVENANCE_RECORD_INVALID"},
        {0x8601, "PROVENANCE_SOURCE_UNAVAILABLE"},
        {0x8602, "PROVENANCE_QUERY_INVALID"},
        {0x8611, "PROVENANCE_STORAGE_EXHAUSTED"},
        {0x8612, "PROVENANCE_BUFFER_FULL"},
        {0x8622, "PROVENANCE_WRITE_FAILED"},
        {0x8623, "PROVENANCE_READ_FAILED"},
        {0x8633, "PROVENANCE_QUERY_TIMEOUT"},
        {0x8644, "PROVENANCE_DATA_CORRUPTED"},
        {0x8645, "PROVENANCE_CHAIN_BROKEN"},
        {0x8666, "PROVENANCE_NOT_INITIALIZED"},

        // Dataset errors
        {0x8700, "DATASET_PATH_INVALID"},
        {0x8701, "DATASET_FORMAT_UNSUPPORTED"},
        {0x8702, "DATASET_EMPTY"},
        {0x8703, "DATASET_SAMPLE_INVALID"},
        {0x8704, "DATASET_SPLITS_INVALID"},
        {0x8711, "DATASET_MEMORY_EXHAUSTED"},
        {0x8712, "DATASET_STORAGE_EXHAUSTED"},
        {0x8722, "DATASET_READ_FAILED"},
        {0x8723, "DATASET_PARSE_FAILED"},
        {0x8733, "DATASET_LOAD_TIMEOUT"},
        {0x8744, "DATASET_FILE_CORRUPTED"},
        {0x8745, "DATASET_MANIFEST_CORRUPTED"},
        {0x8766, "DATASET_ITERATOR_EXHAUSTED"},
    };

    auto it = kErrorMap.find(static_cast<uint32_t>(code));
    if (it != kErrorMap.end()) {
        return it->second;
    }
    return "UNKNOWN_TRAINING_ERROR_0x" + std::to_string(static_cast<uint32_t>(code));
}

}  // namespace training
}  // namespace themis
