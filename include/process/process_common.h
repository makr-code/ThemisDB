/**
 * @file process_common.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: process_common.h | Version: 0.0.1
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/*
 * Shared utilities for process module implementations
 */
#pragma once

#include <chrono>
#include <cstdint>

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
