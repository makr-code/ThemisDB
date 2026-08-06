/**
 * @file process_common.cpp
 * @brief Process module common utilities and error diagnostics (Phase 3).
 * @version 0.0.1
 *
 * Implements the comprehensive error taxonomy and diagnostic infrastructure
 * for standardized fail-safe behavior and unified diagnostics.
 */

#include "process/process_common.h"
#include <unordered_map>
#include <sstream>

namespace themis::process {

// ─────────────────────────────────────────────────────────────────────────────
// Error Code to String Mapping
// ─────────────────────────────────────────────────────────────────────────────

std::string errorCodeToString(ProcessErrorCode code) noexcept {
    static const std::unordered_map<int32_t, std::string> code_map = {
        // Import/Deserialization
        {7600, "EMPTY_INPUT"},
        {7601, "INPUT_TOO_LARGE"},
        {7602, "MALFORMED_INPUT"},
        {7603, "MISSING_REQUIRED_ELEMENT"},
        {7604, "UNSUPPORTED_ELEMENT"},
        {7605, "BROKEN_REFERENCE"},
        {7606, "FILE_READ_ERROR"},
        {7607, "SEMANTIC_VIOLATION"},
        {7608, "ENCODING_ERROR"},
        {7609, "RESOURCE_EXHAUSTION_IMPORT"},
        
        // Lifecycle/Model Management
        {7610, "MODEL_NOT_FOUND"},
        {7611, "MODEL_ALREADY_EXISTS"},
        {7612, "INVALID_MODEL_STATE"},
        {7613, "MODEL_PERSISTENCE_FAILED"},
        {7614, "NORMALIZATION_FAILED"},
        {7615, "SNAPSHOT_FAILED"},
        {7616, "EXPORT_FAILED"},
        {7617, "CONCURRENT_MODIFICATION"},
        {7618, "CONSISTENCY_ERROR"},
        {7619, "RESOURCE_EXHAUSTION_LIFECYCLE"},
        
        // Retrieval/RAG
        {7620, "RETRIEVAL_MODEL_NOT_FOUND"},
        {7621, "GRAPH_TRAVERSAL_FAILED"},
        {7622, "SUBGRAPH_EXTRACTION_FAILED"},
        {7623, "CONTEXT_ASSEMBLY_FAILED"},
        {7624, "SIMILARITY_COMPUTATION_FAILED"},
        {7625, "COMMUNITY_DETECTION_FAILED"},
        {7626, "EMPTY_RETRIEVAL_RESULT"},
        {7627, "RETRIEVAL_QUOTA_EXCEEDED"},
        {7628, "QUERY_ROUTING_FAILED"},
        {7629, "RESOURCE_EXHAUSTION_RETRIEVAL"},
        
        // Linking/Resolution
        {7630, "LINK_TARGET_NOT_FOUND"},
        {7631, "LINK_ALREADY_EXISTS"},
        {7632, "INVALID_LINK_STATE"},
        {7633, "LINK_CREATION_FAILED"},
        {7634, "CIRCULAR_DEPENDENCY"},
        {7635, "LINK_VALIDATION_FAILED"},
        {7636, "LINK_RESOLUTION_FAILED"},
        {7637, "LINK_MODIFICATION_FAILED"},
        {7638, "LINK_DEPTH_EXCEEDED"},
        {7639, "RESOURCE_EXHAUSTION_LINKING"},
        
        // Validation/Compliance
        {7640, "DMN_EVALUATION_FAILED"},
        {7641, "CONFORMANCE_CHECK_FAILED"},
        {7642, "OCEL_VALIDATION_FAILED"},
        {7643, "SLA_VALIDATION_FAILED"},
        {7644, "COMPLIANCE_RULE_VIOLATION"},
        {7645, "CONSTRAINT_VIOLATION"},
        {7646, "INVALID_STATE_TRANSITION"},
        {7647, "VALIDATION_MODEL_NOT_FOUND"},
        {7649, "RESOURCE_EXHAUSTION_VALIDATION"},
        
        // System/Resource
        {7650, "DATABASE_ERROR"},
        {7651, "MEMORY_ALLOCATION_FAILED"},
        {7652, "OPERATION_TIMEOUT"},
        {7653, "CONCURRENCY_LIMIT_EXCEEDED"},
        {7654, "IO_ERROR"},
        {7655, "INVALID_CONFIGURATION"},
        {7699, "INTERNAL_ERROR"},
    };
    
    auto it = code_map.find(static_cast<int32_t>(code));
    if (it != code_map.end()) {
        return it->second;
    }
    return "UNKNOWN_ERROR";
}

// ─────────────────────────────────────────────────────────────────────────────
// Error Category Mapping
// ─────────────────────────────────────────────────────────────────────────────

std::string errorCodeCategory(ProcessErrorCode code) noexcept {
    const int32_t c = static_cast<int32_t>(code);
    
    if (c >= 7600 && c <= 7609) {
        return "IMPORT";
    } else if (c >= 7610 && c <= 7619) {
        return "LIFECYCLE";
    } else if (c >= 7620 && c <= 7629) {
        return "RETRIEVAL";
    } else if (c >= 7630 && c <= 7639) {
        return "LINKING";
    } else if (c >= 7640 && c <= 7649) {
        return "VALIDATION";
    } else if (c >= 7650 && c <= 7699) {
        return "SYSTEM";
    }
    
    return "UNKNOWN";
}

// ─────────────────────────────────────────────────────────────────────────────
// Diagnostic Message Formatting
// ─────────────────────────────────────────────────────────────────────────────

std::string formatDiagnostic(
    ProcessErrorCode code,
    std::string_view context,
    std::string_view detail
) noexcept {
    std::ostringstream oss;
    
    const std::string category = errorCodeCategory(code);
    const std::string error_name = errorCodeToString(code);
    
    oss << "[" << category << "/" << error_name << "]";
    
    if (!context.empty()) {
        oss << " Failed to " << context;
    }
    
    if (!detail.empty()) {
        oss << ": " << detail;
    }
    
    return oss.str();
}

} // namespace themis::process
