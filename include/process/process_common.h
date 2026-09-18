/**
 * @file process_common.h
 * @brief Process module common utilities and comprehensive error taxonomy (Phase 3).
 * @version 0.0.2
 */

#pragma once

#include <chrono>
#include <cstddef>
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

// Bounded Resource Constraints (Phase 2-3 Hardening)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Maximum nesting depth for process model structures (e.g., sub-processes).
 *
 * Prevents stack exhaustion during recursive model traversal and serialization.
 * Typical administrative processes have depth < 10.
 */
constexpr int32_t kMaxModelNestingDepth = 100;

/**
 * @brief Maximum number of elements (nodes, edges) in a single process model.
 *
 * Prevents unbounded memory consumption during model import and storage.
 * Typical administrative processes have < 500 elements.
 */
constexpr int32_t kMaxModelElements = 10000;

/**
 * @brief Maximum size (bytes) of XML/JSON input for a single model import.
 *
 * Prevents denial-of-service via large malformed inputs.
 * Aligned with existing serializer limits (10 MiB).
 */
constexpr size_t kMaxModelInputBytes = 10u * 1024u * 1024u;

/**
 * @brief Maximum size (bytes) for process retrieval context.
 *
 * Prevents unbounded growth of LLM prompt context.
 * Typical retrieval context is < 100 KiB.
 */
constexpr size_t kMaxRetrievalContextBytes = 1u * 1024u * 1024u;

/**
 * @brief Maximum depth for process retrieval (e.g., follow-parent depth in sub-process trees).
 *
 * Prevents infinite loops in linked process hierarchies.
 */
constexpr int32_t kMaxRetrievalDepth = 50;

/**
 * @brief Maximum timeout (milliseconds) for long-running process operations.
 *
 * Serialization, validation, and retrieval operations must complete within this window.
 * Conservative default for administrative process operations.
 */
constexpr int64_t kMaxOperationTimeoutMs = 30000;  // 30 seconds

// ─────────────────────────────────────────────────────────────────────────────
// Validation Helpers (Phase 3)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Validates that input size does not exceed the maximum model input size.
 *
 * @param input_size Size of the input in bytes.
 * @return true if the input size is within limits.
 */
inline bool isInputSizeValid(size_t input_size) {
    return input_size <= kMaxModelInputBytes;
}

/**
 * @brief Validates that nesting depth is within the safe limit.
 *
 * @param depth Current nesting depth.
 * @return true if depth is within limits.
 */
inline bool isNestingDepthValid(int32_t depth) {
    return depth <= kMaxModelNestingDepth;
}

/**
 * @brief Validates that element count is within the safe limit.
 *
 * @param element_count Number of elements in the model.
 * @return true if element count is within limits.
 */
inline bool isElementCountValid(int32_t element_count) {
    return element_count <= kMaxModelElements;
}

/**
 * @brief Validates that context size does not exceed retrieval limits.
 *
 * @param context_size Current context size in bytes.
 * @return true if context size is within limits.
 */
inline bool isContextSizeValid(size_t context_size) {
    return context_size <= kMaxRetrievalContextBytes;
}

/**
 * @brief Validates that traversal depth is within the safe retrieval limit.
 *
 * @param depth Current traversal depth.
 * @return true if depth is within limits.
 */
inline bool isRetrievalDepthValid(int32_t depth) {
    return depth <= kMaxRetrievalDepth;
}

/**
 * @brief Check if an operation has exceeded the timeout window.
 *
 * @param start_time_ms Time when the operation started (from nowMs()).
 * @return true if the operation has exceeded the timeout.
 */
inline bool hasOperationTimedOut(int64_t start_time_ms) {
    int64_t elapsed = nowMs() - start_time_ms;
    return elapsed >= kMaxOperationTimeoutMs;
}

} // namespace themis::process
