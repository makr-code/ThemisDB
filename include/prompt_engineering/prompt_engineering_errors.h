/**
 * @file prompt_engineering_errors.h
 * @brief Error taxonomy for prompt engineering module failure modes (Phase 1 contract).
 * @version 1.0.0
 * @note Maturity: 🟢 DESIGN/CONTRACT
 * @note Status: Phase 1 frozen interface (Q3 2026)
 *
 * This file defines the canonical error codes and failure classes for all prompt engineering
 * operations. All error paths in the module must map to one of these categories.
 *
 * Error taxonomy:
 * - Template errors (9500-9549): invalid/missing template state
 * - Injection errors (9550-9599): context/parameter injection failures
 * - Version errors (9600-9649): version control and history conflicts
 * - Optimization errors (9650-9699): optimizer divergence/non-convergence
 * - Evaluator errors (9700-9749): evaluation inconsistency/corruption
 * - Rewrite errors (9750-9799): rewrite rule application and safety violations
 * - Concurrency errors (9800-9849): concurrent mutation races
 * - Configuration errors (9850-9899): invalid YAML/config state
 *
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include <string>
#include <cstdint>

namespace themis {
namespace prompt_engineering {

/**
 * @brief Error taxonomy for prompt engineering module.
 *
 * All error codes are stable within major release line.
 * New error codes MUST be approved in code review and documented in this header.
 */
enum class PromptEngineeringErrorCode : std::uint32_t {
    // Template errors (9500-9549)
    TEMPLATE_INVALID_ID = 9500,           ///< Template ID missing or malformed
    TEMPLATE_NOT_FOUND = 9501,            ///< Template ID not in registry
    TEMPLATE_ALREADY_EXISTS = 9502,       ///< Template ID already registered
    TEMPLATE_VALIDATION_FAILED = 9503,    ///< Template structure validation failed (missing placeholders, cycles)
    TEMPLATE_OVERSIZED = 9504,            ///< Template exceeds max byte/token limits
    TEMPLATE_PLACEHOLDER_MISSING = 9505,  ///< Required placeholder not found in template
    TEMPLATE_PLACEHOLDER_RECURSIVE = 9506, ///< Recursive placeholder reference detected
    TEMPLATE_INJECTION_MISMATCH = 9507,   ///< Injected context doesn't match placeholder signature
    TEMPLATE_MALFORMED_JSON = 9508,       ///< Template JSON parse error
    TEMPLATE_UNSUPPORTED_MIME_TYPE = 9509, ///< Unsupported MIME type in multi-modal context

    // Injection errors (9550-9599)
    INJECTION_CONTEXT_MISSING = 9550,      ///< Required context key not provided
    INJECTION_CONTEXT_TYPE_MISMATCH = 9551, ///< Context value type doesn't match placeholder expectation
    INJECTION_SUBSTITUTION_FAILED = 9552,  ///< Template variable substitution failed
    INJECTION_ENCODING_ERROR = 9553,       ///< Text encoding error during injection
    INJECTION_SIZE_EXCEEDED = 9554,        ///< Injected context exceeds size bounds
    INJECTION_SEMANTIC_MISMATCH = 9555,    ///< Semantic constraint violation (e.g. enum value out of range)
    INJECTION_CIRCULAR_REFERENCE = 9556,   ///< Circular dependency in context injection

    // Version errors (9600-9649)
    VERSION_CONFLICT = 9600,               ///< Conflicting edits to same version
    VERSION_NOT_FOUND = 9601,              ///< Requested version ID doesn't exist
    VERSION_COMMIT_FAILED = 9602,          ///< Commit to version history failed
    VERSION_HISTORY_CORRUPTED = 9603,      ///< Version history integrity violation
    VERSION_MERGE_CONFLICT = 9604,         ///< Automatic merge of concurrent edits failed
    VERSION_DRIFT_DETECTED = 9605,         ///< Version branch has diverged beyond threshold
    VERSION_ROLLBACK_FAILED = 9606,        ///< Unable to rollback to target version
    VERSION_CLEANUP_FAILED = 9607,         ///< Garbage collection of old versions failed

    // Optimization errors (9650-9699)
    OPTIMIZATION_DIVERGED = 9650,          ///< Optimizer diverged instead of converging
    OPTIMIZATION_NON_CONVERGENT = 9651,    ///< Optimizer failed to converge within iteration budget
    OPTIMIZATION_INVALID_PARAMETERS = 9652, ///< Optimizer config invalid or incompatible
    OPTIMIZATION_FEEDBACK_CORRUPT = 9653,  ///< Feedback signal corrupted or inconsistent
    OPTIMIZATION_TIMEOUT = 9654,           ///< Optimization exceeded time budget
    OPTIMIZATION_OUT_OF_MEMORY = 9655,     ///< Insufficient memory for optimization state
    OPTIMIZATION_INITIALIZATION_FAILED = 9656, ///< Optimizer failed to initialize
    OPTIMIZATION_EARLY_TERMINATION = 9657, ///< Optimizer stopped due to safety/policy constraint

    // Evaluator errors (9700-9749)
    EVALUATION_INCONSISTENT = 9700,        ///< Multiple evaluations of same prompt diverge
    EVALUATION_SCORE_INVALID = 9701,       ///< Evaluation score out of valid range
    EVALUATION_TIMEOUT = 9702,             ///< Evaluation exceeded time budget
    EVALUATION_MODEL_UNAVAILABLE = 9703,   ///< Evaluation model not loaded or unavailable
    EVALUATION_CORRUPTED_STATE = 9704,     ///< Evaluator internal state corrupted
    EVALUATION_MEMORY_ERROR = 9705,        ///< Insufficient memory for evaluation
    EVALUATION_CONSTRAINT_VIOLATION = 9706, ///< Evaluation result violates domain constraint
    EVALUATION_SAMPLING_FAILED = 9707,     ///< Monte-Carlo/sampling evaluation failed

    // Rewrite errors (9750-9799) – Phase 2 reserved
    REWRITE_RULE_NOT_FOUND = 9750,        ///< Rewrite rule ID not registered
    REWRITE_RULE_INVALID = 9751,          ///< Rewrite rule definition malformed
    REWRITE_PHASE_VIOLATION = 9752,       ///< Attempted execution out of phase order
    REWRITE_MAX_STEPS_EXCEEDED = 9753,    ///< Rewrite exceeded max step bound
    REWRITE_DOCUMENT_INVALID = 9754,      ///< Document state invalid for rewrite
    REWRITE_UNSAFE_TRANSFORMATION = 9755, ///< Rewrite produced unsafe/blocked content
    REWRITE_TRACE_FAILED = 9756,          ///< Rewrite trace generation failed
    REWRITE_YAML_PARSE_ERROR = 9757,      ///< YAML rule config parse error
    REWRITE_REGEX_PATHOLOGICAL = 9758,    ///< Regex pattern triggers pathological behavior

    // Concurrency errors (9800-9849)
    CONCURRENCY_RACE_DETECTED = 9800,      ///< Concurrent mutation race condition
    CONCURRENCY_DEADLOCK = 9801,           ///< Deadlock detected in concurrent access
    CONCURRENCY_STARVATION = 9802,         ///< Thread starvation under contention
    CONCURRENCY_ORDERING_VIOLATION = 9803, ///< Concurrent ops completed in invalid order
    CONCURRENCY_STATE_INCONSISTENT = 9804, ///< State machine inconsistency from concurrent access

    // Configuration errors (9850-9899)
    CONFIG_INVALID_YAML = 9850,            ///< YAML configuration parse error
    CONFIG_MISSING_REQUIRED_FIELD = 9851,  ///< Required config field not provided
    CONFIG_INVALID_VALUE = 9852,           ///< Config value out of valid range
    CONFIG_INCOMPATIBLE_OPTIONS = 9853,    ///< Mutually incompatible config options
    CONFIG_FILE_NOT_FOUND = 9854,          ///< Configuration file not found
    CONFIG_PERMISSION_DENIED = 9855,       ///< Insufficient permissions to read config
    CONFIG_SCHEMA_MISMATCH = 9856,         ///< Config doesn't match expected schema version
};

/**
 * @brief Convert error code to human-readable description.
 * @param code Error code from PromptEngineeringErrorCode
 * @return String description of the error
 */
inline std::string error_code_to_string(PromptEngineeringErrorCode code) {
    switch (code) {
        // Template errors
        case PromptEngineeringErrorCode::TEMPLATE_INVALID_ID:
            return "TEMPLATE_INVALID_ID: Template ID missing or malformed";
        case PromptEngineeringErrorCode::TEMPLATE_NOT_FOUND:
            return "TEMPLATE_NOT_FOUND: Template ID not in registry";
        case PromptEngineeringErrorCode::TEMPLATE_ALREADY_EXISTS:
            return "TEMPLATE_ALREADY_EXISTS: Template ID already registered";
        case PromptEngineeringErrorCode::TEMPLATE_VALIDATION_FAILED:
            return "TEMPLATE_VALIDATION_FAILED: Template structure validation failed";
        case PromptEngineeringErrorCode::TEMPLATE_OVERSIZED:
            return "TEMPLATE_OVERSIZED: Template exceeds max byte/token limits";
        case PromptEngineeringErrorCode::TEMPLATE_PLACEHOLDER_MISSING:
            return "TEMPLATE_PLACEHOLDER_MISSING: Required placeholder not found";
        case PromptEngineeringErrorCode::TEMPLATE_PLACEHOLDER_RECURSIVE:
            return "TEMPLATE_PLACEHOLDER_RECURSIVE: Recursive placeholder reference";
        case PromptEngineeringErrorCode::TEMPLATE_INJECTION_MISMATCH:
            return "TEMPLATE_INJECTION_MISMATCH: Injected context type mismatch";
        case PromptEngineeringErrorCode::TEMPLATE_MALFORMED_JSON:
            return "TEMPLATE_MALFORMED_JSON: Template JSON parse error";
        case PromptEngineeringErrorCode::TEMPLATE_UNSUPPORTED_MIME_TYPE:
            return "TEMPLATE_UNSUPPORTED_MIME_TYPE: Unsupported MIME type in multi-modal";

        // Injection errors
        case PromptEngineeringErrorCode::INJECTION_CONTEXT_MISSING:
            return "INJECTION_CONTEXT_MISSING: Required context key not provided";
        case PromptEngineeringErrorCode::INJECTION_CONTEXT_TYPE_MISMATCH:
            return "INJECTION_CONTEXT_TYPE_MISMATCH: Context value type mismatch";
        case PromptEngineeringErrorCode::INJECTION_SUBSTITUTION_FAILED:
            return "INJECTION_SUBSTITUTION_FAILED: Template variable substitution failed";
        case PromptEngineeringErrorCode::INJECTION_ENCODING_ERROR:
            return "INJECTION_ENCODING_ERROR: Text encoding error during injection";
        case PromptEngineeringErrorCode::INJECTION_SIZE_EXCEEDED:
            return "INJECTION_SIZE_EXCEEDED: Injected context exceeds size bounds";
        case PromptEngineeringErrorCode::INJECTION_SEMANTIC_MISMATCH:
            return "INJECTION_SEMANTIC_MISMATCH: Semantic constraint violation";
        case PromptEngineeringErrorCode::INJECTION_CIRCULAR_REFERENCE:
            return "INJECTION_CIRCULAR_REFERENCE: Circular dependency in context";

        // Version errors
        case PromptEngineeringErrorCode::VERSION_CONFLICT:
            return "VERSION_CONFLICT: Conflicting edits to same version";
        case PromptEngineeringErrorCode::VERSION_NOT_FOUND:
            return "VERSION_NOT_FOUND: Requested version ID doesn't exist";
        case PromptEngineeringErrorCode::VERSION_COMMIT_FAILED:
            return "VERSION_COMMIT_FAILED: Commit to version history failed";
        case PromptEngineeringErrorCode::VERSION_HISTORY_CORRUPTED:
            return "VERSION_HISTORY_CORRUPTED: Version history integrity violation";
        case PromptEngineeringErrorCode::VERSION_MERGE_CONFLICT:
            return "VERSION_MERGE_CONFLICT: Automatic merge of concurrent edits failed";
        case PromptEngineeringErrorCode::VERSION_DRIFT_DETECTED:
            return "VERSION_DRIFT_DETECTED: Version branch diverged beyond threshold";
        case PromptEngineeringErrorCode::VERSION_ROLLBACK_FAILED:
            return "VERSION_ROLLBACK_FAILED: Unable to rollback to target version";
        case PromptEngineeringErrorCode::VERSION_CLEANUP_FAILED:
            return "VERSION_CLEANUP_FAILED: Garbage collection of old versions failed";

        // Optimization errors
        case PromptEngineeringErrorCode::OPTIMIZATION_DIVERGED:
            return "OPTIMIZATION_DIVERGED: Optimizer diverged instead of converging";
        case PromptEngineeringErrorCode::OPTIMIZATION_NON_CONVERGENT:
            return "OPTIMIZATION_NON_CONVERGENT: Failed to converge within iteration budget";
        case PromptEngineeringErrorCode::OPTIMIZATION_INVALID_PARAMETERS:
            return "OPTIMIZATION_INVALID_PARAMETERS: Optimizer config invalid";
        case PromptEngineeringErrorCode::OPTIMIZATION_FEEDBACK_CORRUPT:
            return "OPTIMIZATION_FEEDBACK_CORRUPT: Feedback signal corrupted";
        case PromptEngineeringErrorCode::OPTIMIZATION_TIMEOUT:
            return "OPTIMIZATION_TIMEOUT: Optimization exceeded time budget";
        case PromptEngineeringErrorCode::OPTIMIZATION_OUT_OF_MEMORY:
            return "OPTIMIZATION_OUT_OF_MEMORY: Insufficient memory for state";
        case PromptEngineeringErrorCode::OPTIMIZATION_INITIALIZATION_FAILED:
            return "OPTIMIZATION_INITIALIZATION_FAILED: Optimizer failed to initialize";
        case PromptEngineeringErrorCode::OPTIMIZATION_EARLY_TERMINATION:
            return "OPTIMIZATION_EARLY_TERMINATION: Optimizer stopped by constraint";

        // Evaluator errors
        case PromptEngineeringErrorCode::EVALUATION_INCONSISTENT:
            return "EVALUATION_INCONSISTENT: Multiple evaluations diverge";
        case PromptEngineeringErrorCode::EVALUATION_SCORE_INVALID:
            return "EVALUATION_SCORE_INVALID: Evaluation score out of valid range";
        case PromptEngineeringErrorCode::EVALUATION_TIMEOUT:
            return "EVALUATION_TIMEOUT: Evaluation exceeded time budget";
        case PromptEngineeringErrorCode::EVALUATION_MODEL_UNAVAILABLE:
            return "EVALUATION_MODEL_UNAVAILABLE: Evaluation model not loaded";
        case PromptEngineeringErrorCode::EVALUATION_CORRUPTED_STATE:
            return "EVALUATION_CORRUPTED_STATE: Evaluator internal state corrupted";
        case PromptEngineeringErrorCode::EVALUATION_MEMORY_ERROR:
            return "EVALUATION_MEMORY_ERROR: Insufficient memory for evaluation";
        case PromptEngineeringErrorCode::EVALUATION_CONSTRAINT_VIOLATION:
            return "EVALUATION_CONSTRAINT_VIOLATION: Result violates domain constraint";
        case PromptEngineeringErrorCode::EVALUATION_SAMPLING_FAILED:
            return "EVALUATION_SAMPLING_FAILED: Monte-Carlo/sampling evaluation failed";

        // Rewrite errors
        case PromptEngineeringErrorCode::REWRITE_RULE_NOT_FOUND:
            return "REWRITE_RULE_NOT_FOUND: Rewrite rule ID not registered";
        case PromptEngineeringErrorCode::REWRITE_RULE_INVALID:
            return "REWRITE_RULE_INVALID: Rewrite rule definition malformed";
        case PromptEngineeringErrorCode::REWRITE_PHASE_VIOLATION:
            return "REWRITE_PHASE_VIOLATION: Execution out of phase order";
        case PromptEngineeringErrorCode::REWRITE_MAX_STEPS_EXCEEDED:
            return "REWRITE_MAX_STEPS_EXCEEDED: Exceeded max step bound";
        case PromptEngineeringErrorCode::REWRITE_DOCUMENT_INVALID:
            return "REWRITE_DOCUMENT_INVALID: Document state invalid for rewrite";
        case PromptEngineeringErrorCode::REWRITE_UNSAFE_TRANSFORMATION:
            return "REWRITE_UNSAFE_TRANSFORMATION: Rewrite produced unsafe content";
        case PromptEngineeringErrorCode::REWRITE_TRACE_FAILED:
            return "REWRITE_TRACE_FAILED: Rewrite trace generation failed";
        case PromptEngineeringErrorCode::REWRITE_YAML_PARSE_ERROR:
            return "REWRITE_YAML_PARSE_ERROR: YAML rule config parse error";
        case PromptEngineeringErrorCode::REWRITE_REGEX_PATHOLOGICAL:
            return "REWRITE_REGEX_PATHOLOGICAL: Regex pattern triggers pathological behavior";

        // Concurrency errors
        case PromptEngineeringErrorCode::CONCURRENCY_RACE_DETECTED:
            return "CONCURRENCY_RACE_DETECTED: Concurrent mutation race condition";
        case PromptEngineeringErrorCode::CONCURRENCY_DEADLOCK:
            return "CONCURRENCY_DEADLOCK: Deadlock detected in concurrent access";
        case PromptEngineeringErrorCode::CONCURRENCY_STARVATION:
            return "CONCURRENCY_STARVATION: Thread starvation under contention";
        case PromptEngineeringErrorCode::CONCURRENCY_ORDERING_VIOLATION:
            return "CONCURRENCY_ORDERING_VIOLATION: Concurrent ops in invalid order";
        case PromptEngineeringErrorCode::CONCURRENCY_STATE_INCONSISTENT:
            return "CONCURRENCY_STATE_INCONSISTENT: State machine inconsistency";

        // Configuration errors
        case PromptEngineeringErrorCode::CONFIG_INVALID_YAML:
            return "CONFIG_INVALID_YAML: YAML configuration parse error";
        case PromptEngineeringErrorCode::CONFIG_MISSING_REQUIRED_FIELD:
            return "CONFIG_MISSING_REQUIRED_FIELD: Required config field not provided";
        case PromptEngineeringErrorCode::CONFIG_INVALID_VALUE:
            return "CONFIG_INVALID_VALUE: Config value out of valid range";
        case PromptEngineeringErrorCode::CONFIG_INCOMPATIBLE_OPTIONS:
            return "CONFIG_INCOMPATIBLE_OPTIONS: Mutually incompatible config options";
        case PromptEngineeringErrorCode::CONFIG_FILE_NOT_FOUND:
            return "CONFIG_FILE_NOT_FOUND: Configuration file not found";
        case PromptEngineeringErrorCode::CONFIG_PERMISSION_DENIED:
            return "CONFIG_PERMISSION_DENIED: Insufficient permissions to read config";
        case PromptEngineeringErrorCode::CONFIG_SCHEMA_MISMATCH:
            return "CONFIG_SCHEMA_MISMATCH: Config doesn't match schema version";

        default:
            return "UNKNOWN_ERROR: Error code not recognized";
    }
}

/**
 * @brief Error context for detailed diagnostics and incident triage.
 *
 * All error paths MUST populate relevant fields to enable production incident investigation.
 */
struct PromptEngineeringErrorContext {
    PromptEngineeringErrorCode code;      ///< Primary error code
    std::string message;                  ///< Human-readable error message
    std::string template_id;              ///< Template ID involved (if applicable)
    std::string version_id;               ///< Version ID involved (if applicable)
    std::string operation;                ///< Operation that failed (create/get/inject/validate/etc)
    std::string remediation_hint;         ///< Suggested recovery action for operators
    uint32_t retry_count;                 ///< Number of retry attempts before failure
    bool is_retryable;                    ///< Whether operation can be safely retried
};

} // namespace prompt_engineering
} // namespace themis
