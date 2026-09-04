/**
 * @file graph_error_taxonomy.cpp
 * @brief Graph Module Error Taxonomy Implementation
 *
 * Implements error code classification, description mapping, and diagnostics
 * helpers for the graph module error taxonomy (Phase 1).
 *
 * @version 1.0.0
 * @date 2026-08-01
 */

#include "graph/graph_error_taxonomy.h"
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <iomanip>

namespace themis {
namespace graph {

// ─────────────────────────────────────────────────────────────────────────────
// Error Description Lookup Table
// ─────────────────────────────────────────────────────────────────────────────

[[nodiscard]] std::string_view
getErrorDescription(GraphErrorCode code) noexcept {
    // Static lookup table for all Phase 1 error codes
    static const std::unordered_map<uint32_t, std::string_view> descriptions{
        // Query Optimizer (GRAPH01)
        {0x01010001, "Query optimizer: null or invalid query AST (precondition)"},
        {0x01010002, "Query optimizer: query pattern not supported by current optimizer"},
        {0x01010003, "Query optimizer: graph statistics unavailable or stale"},
        {0x01010004, "Query optimizer: cost model calculation overflow (data error)"},
        {0x01020001, "Query optimizer: GPU acceleration unavailable, CPU fallback required"},
        {0x01020002, "Query optimizer: distributed planner offline, local fallback required"},
        {0x01030001, "Query optimizer: constraint set detected as unsatisfiable"},

        // Traversal & Execution (GRAPH02)
        {0x02010001, "Traversal: source or target vertex not found in graph"},
        {0x02010002, "Traversal: invalid edge type or property filter specification"},
        {0x02010003, "Traversal: frontier exceeded maximum size limit (resource exhaustion)"},
        {0x02010004, "Traversal: path depth exceeds configured maximum (resource limit)"},
        {0x02010005, "Traversal: timeout elapsed during traversal execution"},
        {0x02010006, "Traversal: parallel traversal thread creation failed"},
        {0x02020001, "Traversal: GPU memory exhausted, CPU fallback required"},
        {0x02020002, "Traversal: GPU kernel launch failed, CPU fallback required"},
        {0x02020003, "Traversal: distributed shard unavailable, retry or use cache"},
        {0x02030001, "Traversal: constraint cannot be satisfied during traversal"},

        // Semantic & Reasoning (GRAPH03)
        {0x03010001, "Reasoning: invalid inference rule syntax (parsing error)"},
        {0x03010002, "Reasoning: ontology not loaded or corrupted"},
        {0x03010003, "Reasoning: rule variable binding failed (no matching facts)"},
        {0x03030001, "Reasoning: inferred fact contradicts known base fact (conflict)"},
        {0x03030002, "Reasoning: cyclic rule dependency detected (infinite recursion risk)"},

        // Tensor Utilities (GRAPH04)
        {0x04010001, "Tensor utility: invalid tensor dimensions or shape mismatch"},
        {0x04010002, "Tensor utility: fingerprint computation failed (NaN or infinity)"},
        {0x04010003, "Tensor utility: graph deduplication threshold out of valid range"},
        {0x04020001, "Tensor utility: GPU tensor operation unavailable, CPU fallback required"},

        // Distributed Graph (GRAPH05)
        {0x05010001, "Distributed: shard configuration invalid or incomplete"},
        {0x05010002, "Distributed: cross-shard merge operation failed (incompatible results)"},
        {0x05010003, "Distributed: vertex not hashed to any shard (internal error)"},
        {0x05020001, "Distributed: shard peer offline, operation deferred"},
        {0x05020002, "Distributed: RPC timeout on cross-shard communication"},

        // Resource & Planning (GRAPH06)
        {0x06010001, "Resource: plan cache full, LRU eviction required"},
        {0x06010002, "Resource: resource pool exhausted (bounded allocation exceeded)"},
        {0x06010003, "Resource: load balancing decision failed"},
        {0x06020001, "Resource: cache miss on frequently-accessed plan"},

        // Generic / Infrastructure (GRAPH07)
        {0x07010001, "Generic: unknown or unclassified error"},
        {0x07010002, "Generic: feature not implemented (stub/TODO code path)"},
        {0x07010003, "Generic: internal consistency check failed (invariant violation)"},
        {0x07020001, "Generic: temporary system resource unavailable (transient)"},
    };

    const auto c = static_cast<uint32_t>(code);
    auto it = descriptions.find(c);
    if (it != descriptions.end()) {
        return it->second;
    }
    return "Unknown error code (not in Phase 1 taxonomy)";
}

// ─────────────────────────────────────────────────────────────────────────────
// Error Code Formatting
// ─────────────────────────────────────────────────────────────────────────────

[[nodiscard]] std::string
getErrorCodeHex(GraphErrorCode code) noexcept {
    std::ostringstream oss = {};
    oss << "0x" << std::hex << std::setw(8) << std::setfill('0')
        << static_cast<uint32_t>(code);
    return oss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// Error Context Diagnostics
// ─────────────────────────────────────────────────────────────────────────────

[[nodiscard]] std::string
ErrorContext::getRecoveryRecommendation() const noexcept {
    const auto category = getErrorCategory(code);
    
    if (category == ErrorCategory::DENIAL) {
        return "DENIAL: Fix preconditions and retry. "
               "Verify input data, constraints, and resource availability.";
    } else if (category == ErrorCategory::FALLBACK) {
        return "FALLBACK: Operation will retry on CPU. "
               "Check GPU/distributed resource availability or disable acceleration.";
    } else if (category == ErrorCategory::REASONING_CONFLICT) {
        return "CONFLICT: Knowledge graph inference contradiction detected. "
               "Operator intervention required; review conflicting facts and rules.";
    }
    
    return "UNKNOWN: Unclassified error category.";
}

[[nodiscard]] std::string
ErrorContext::formatDiagnostic() const noexcept {
    std::ostringstream oss;
    oss << "[GRAPH ERROR]\n"
        << "  Code: " << getErrorCodeHex(code) << "\n"
        << "  Category: ";
    
    switch (getErrorCategory(code)) {
        case ErrorCategory::DENIAL:
            oss << "DENIAL";
            break;
        case ErrorCategory::FALLBACK:
            oss << "FALLBACK";
            break;
        case ErrorCategory::REASONING_CONFLICT:
            oss << "REASONING_CONFLICT";
            break;
    }
    
    oss << "\n"
        << "  Component: " << component << "\n"
        << "  Operation: " << operation << "\n"
        << "  Description: " << getErrorDescription(code) << "\n";
    
    if (!context.empty()) {
        oss << "  Context: " << context << "\n";
    }
    
    oss << "  Recovery: " << getRecoveryRecommendation() << "\n";
    
    if (!recovery_hint.empty()) {
        oss << "  Hint: " << recovery_hint << "\n";
    }
    
    return oss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// Error Code Validation
// ─────────────────────────────────────────────────────────────────────────────

[[nodiscard]] bool
isValidErrorCode(GraphErrorCode code) noexcept {
    if (code == GraphErrorCode::SUCCESS) {
      return true;
    }
    
    const auto c = static_cast<uint32_t>(code);
    
    // Extract component (xx = first two hex digits after the initial 0x)
    const auto component = (c >> 24) & 0xFF;
    if (component < 0x01 || component > 0x07) {
        return false; // Invalid component range
    }
    
    // Known codes lookup (exhaustive list from Phase 1 taxonomy)
    static const std::unordered_set<uint32_t> valid_codes{
        // Query Optimizer (GRAPH01)
        0x01010001, 0x01010002, 0x01010003, 0x01010004,
        0x01020001, 0x01020002,
        0x01030001,

        // Traversal & Execution (GRAPH02)
        0x02010001, 0x02010002, 0x02010003, 0x02010004, 0x02010005, 0x02010006,
        0x02020001, 0x02020002, 0x02020003,
        0x02030001,

        // Semantic & Reasoning (GRAPH03)
        0x03010001, 0x03010002, 0x03010003,
        0x03030001, 0x03030002,

        // Tensor Utilities (GRAPH04)
        0x04010001, 0x04010002, 0x04010003,
        0x04020001,

        // Distributed Graph (GRAPH05)
        0x05010001, 0x05010002, 0x05010003,
        0x05020001, 0x05020002,

        // Resource & Planning (GRAPH06)
        0x06010001, 0x06010002, 0x06010003,
        0x06020001,

        // Generic / Infrastructure (GRAPH07)
        0x07010001, 0x07010002, 0x07010003,
        0x07020001,
    };
    
    return valid_codes.count(c) > 0;
}

} // namespace graph
} // namespace themis
