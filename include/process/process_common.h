/**
 * @file process_common.h
 * @brief Process module common utilities and comprehensive error taxonomy (Phase 3).
 * @version 0.0.2
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>

namespace themis::process {

/**
 * @brief Get current wall-clock time in milliseconds since Unix epoch.
 *
 * @return Current time in milliseconds
 */
inline int64_t nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

// ─────────────────────────────────────────────────────────────────────────────
// Comprehensive Process Module Error Taxonomy (Phase 3)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Unified error codes for all process module operations.
 *
 * Error codes are partitioned by category with reserved ranges:
 * - 7600–7609: Import/Deserialization errors
 * - 7610–7619: Lifecycle/Model management errors
 * - 7620–7629: Retrieval/RAG errors
 * - 7630–7639: Linking/Resolution errors
 * - 7640–7649: Validation/Compliance errors
 * - 7650–7699: System/Resource errors
 *
 * @note All error codes are actionable for operators and include enough context
 *       for troubleshooting production incidents. There are NO silent failures.
 */
enum class ProcessErrorCode : int32_t {
    // IMPORT/DESERIALIZATION (7600-7609)
    EMPTY_INPUT = 7600,
    INPUT_TOO_LARGE = 7601,
    MALFORMED_INPUT = 7602,
    MISSING_REQUIRED_ELEMENT = 7603,
    UNSUPPORTED_ELEMENT = 7604,
    BROKEN_REFERENCE = 7605,
    FILE_READ_ERROR = 7606,
    SEMANTIC_VIOLATION = 7607,
    ENCODING_ERROR = 7608,
    RESOURCE_EXHAUSTION_IMPORT = 7609,
    
    // LIFECYCLE/MODEL MANAGEMENT (7610-7619)
    MODEL_NOT_FOUND = 7610,
    MODEL_ALREADY_EXISTS = 7611,
    INVALID_MODEL_STATE = 7612,
    MODEL_PERSISTENCE_FAILED = 7613,
    NORMALIZATION_FAILED = 7614,
    SNAPSHOT_FAILED = 7615,
    EXPORT_FAILED = 7616,
    CONCURRENT_MODIFICATION = 7617,
    CONSISTENCY_ERROR = 7618,
    RESOURCE_EXHAUSTION_LIFECYCLE = 7619,
    
    // RETRIEVAL/RAG (7620-7629)
    RETRIEVAL_MODEL_NOT_FOUND = 7620,
    GRAPH_TRAVERSAL_FAILED = 7621,
    SUBGRAPH_EXTRACTION_FAILED = 7622,
    CONTEXT_ASSEMBLY_FAILED = 7623,
    SIMILARITY_COMPUTATION_FAILED = 7624,
    COMMUNITY_DETECTION_FAILED = 7625,
    EMPTY_RETRIEVAL_RESULT = 7626,
    RETRIEVAL_QUOTA_EXCEEDED = 7627,
    QUERY_ROUTING_FAILED = 7628,
    RESOURCE_EXHAUSTION_RETRIEVAL = 7629,
    
    // LINKING/RESOLUTION (7630-7639)
    LINK_TARGET_NOT_FOUND = 7630,
    LINK_ALREADY_EXISTS = 7631,
    INVALID_LINK_STATE = 7632,
    LINK_CREATION_FAILED = 7633,
    CIRCULAR_DEPENDENCY = 7634,
    LINK_VALIDATION_FAILED = 7635,
    LINK_RESOLUTION_FAILED = 7636,
    LINK_MODIFICATION_FAILED = 7637,
    LINK_DEPTH_EXCEEDED = 7638,
    RESOURCE_EXHAUSTION_LINKING = 7639,
    
    // VALIDATION/COMPLIANCE (7640-7649)
    DMN_EVALUATION_FAILED = 7640,
    CONFORMANCE_CHECK_FAILED = 7641,
    OCEL_VALIDATION_FAILED = 7642,
    SLA_VALIDATION_FAILED = 7643,
    COMPLIANCE_RULE_VIOLATION = 7644,
    CONSTRAINT_VIOLATION = 7645,
    INVALID_STATE_TRANSITION = 7646,
    VALIDATION_MODEL_NOT_FOUND = 7647,
    RESOURCE_EXHAUSTION_VALIDATION = 7649,
    
    // SYSTEM/RESOURCE ERRORS (7650-7699)
    DATABASE_ERROR = 7650,
    MEMORY_ALLOCATION_FAILED = 7651,
    OPERATION_TIMEOUT = 7652,
    CONCURRENCY_LIMIT_EXCEEDED = 7653,
    IO_ERROR = 7654,
    INVALID_CONFIGURATION = 7655,
    INTERNAL_ERROR = 7699,
};

/**
 * @brief Convert ProcessErrorCode to a human-readable string.
 *
 * @param code Error code to convert.
 * @return Short error name (e.g., "MALFORMED_INPUT").
 */
std::string errorCodeToString(ProcessErrorCode code) noexcept;

/**
 * @brief Get the error category for an error code.
 *
 * @param code Error code.
 * @return Category name (e.g., "IMPORT", "RETRIEVAL", "SYSTEM").
 */
std::string errorCodeCategory(ProcessErrorCode code) noexcept;

/**
 * @brief Diagnostic message helper for structured error reporting.
 *
 * Constructs an actionable diagnostic message suitable for operator triage.
 *
 * @param code Error code.
 * @param context Brief operational context (e.g., "import BPMN from file").
 * @param detail Additional detail (e.g., "line 42: unexpected token").
 * @return Formatted diagnostic message.
 */
std::string formatDiagnostic(
    ProcessErrorCode code,
    std::string_view context,
    std::string_view detail = ""
) noexcept;

} // namespace themis::process
