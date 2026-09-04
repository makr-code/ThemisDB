/**
 * @file tensor_edge_case_handler.cpp
 * @brief Implementation of deterministic edge case handling for tensor module.
 *
 * @version 1.0.0
 * @date 2026-09-01
 */

#include "tensor/tensor_edge_case_handler.h"
#include "utils/error_registry.h"

#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace themis {
namespace tensor {

// ============================================================================
// EdgeCaseResult Factory Methods
// ============================================================================

EdgeCaseResult EdgeCaseResult::Ok(const std::string& message) noexcept {
    EdgeCaseResult result;
    result.success = true;
    result.error_code = 0;
    result.error_message = message.empty() ? "Operation succeeded" : message;
    result.recovery_action = "none";
    result.is_recoverable = false;
    return result;
}

EdgeCaseResult EdgeCaseResult::Error(int code, const std::string& message,
                                      const std::string& recovery_action,
                                      bool recoverable) noexcept {
    EdgeCaseResult result;
    result.success = false;
    result.error_code = code;
    result.error_message = message;
    result.recovery_action = recovery_action;
    result.is_recoverable = recoverable;
    return result;
}

// ============================================================================
// TensorEdgeCaseHandler Implementation
// ============================================================================

TensorEdgeCaseHandler::TensorEdgeCaseHandler() noexcept
    : detailed_diagnostics_(false) {}

// ─────────────────────────────────────────────────────────────────────────
// TEDGE-01: Invalid Adapter Reference
// ─────────────────────────────────────────────────────────────────────────

EdgeCaseResult TensorEdgeCaseHandler::handleInvalidAdapterReference(
    const std::string& adapter_key,
    const std::string& context_op) noexcept {
    stats_.invalid_adapter_refs++;

    EdgeCaseResult result = EdgeCaseResult::Error(
        static_cast<int>(errors::ErrorCode::ERR_TENSOR_ADAPTER_NOT_FOUND),
        fmt::format(
            "Adapter '{}' not found in graph (context: {})",
            adapter_key, context_op),
        "fail-closed",
        false  // Not recoverable — adapter must exist
    );

    emitDiagnostic("TEDGE-01: Invalid Adapter Reference", result);
    return result;
}

// ─────────────────────────────────────────────────────────────────────────
// TEDGE-02: Out-of-Bounds Index Access
// ─────────────────────────────────────────────────────────────────────────

EdgeCaseResult TensorEdgeCaseHandler::handleOutOfBoundsIndex(
    std::size_t requested_k,
    std::size_t actual_size) noexcept {
    stats_.out_of_bounds_accesses++;

    EdgeCaseResult result = EdgeCaseResult::Error(
        static_cast<int>(errors::ErrorCode::ERR_TENSOR_INDEX_LOOKUP_FAILED),
        fmt::format(
            "Requested k={} but graph has only {} adapters",
            requested_k, actual_size),
        "degrade",
        true  // Recoverable — return min(k, size) results
    );

    emitDiagnostic("TEDGE-02: Out-of-Bounds Index", result);
    return result;
}

// ─────────────────────────────────────────────────────────────────────────
// TEDGE-03: Stale/Invalid Fingerprint
// ─────────────────────────────────────────────────────────────────────────

EdgeCaseResult TensorEdgeCaseHandler::handleStaleFingerprint(
    const std::string& adapter_key,
    float norm_value) noexcept {
    stats_.stale_fingerprints++;

    std::string norm_desc = "unknown";
    if (std::isnan(norm_value)) {
        norm_desc = "NaN";
    } else if (std::isinf(norm_value)) {
        norm_desc = fmt::format("Inf (sign: {})", norm_value > 0 ? "+" : "-");
    } else if (norm_value <= 0.0f) {
        norm_desc = fmt::format("negative/zero (value: {})", norm_value);
    }

    EdgeCaseResult result = EdgeCaseResult::Error(
        static_cast<int>(errors::ErrorCode::ERR_TENSOR_GRAPH_INVALID_SELF_IP),
        fmt::format(
            "Adapter '{}' has invalid fingerprint norm: {}",
            adapter_key, norm_desc),
        "fail-closed",
        false  // Not recoverable — requires recomputation
    );

    emitDiagnostic("TEDGE-03: Stale Fingerprint", result);
    return result;
}

// ─────────────────────────────────────────────────────────────────────────
// TEDGE-04: Self-Similarity Computation Failure
// ─────────────────────────────────────────────────────────────────────────

EdgeCaseResult TensorEdgeCaseHandler::handleSelfSimilarityFailure(
    const std::string& adapter_key,
    float computed_score) noexcept {
    stats_.self_similarity_failures++;

    std::string score_desc = "unknown";
    if (std::isnan(computed_score)) {
        score_desc = "NaN";
    } else if (std::isinf(computed_score)) {
        score_desc = fmt::format("Inf (sign: {})", computed_score > 0 ? "+" : "-");
    } else if (std::abs(computed_score - 1.0f) > 0.01f) {
        score_desc = fmt::format("diverged from 1.0 (value: {:.6f})", computed_score);
    }

    EdgeCaseResult result = EdgeCaseResult::Error(
        static_cast<int>(errors::ErrorCode::ERR_TENSOR_GRAPH_INVALID_SELF_IP),
        fmt::format(
            "Self-similarity for adapter '{}' is invalid: {}",
            adapter_key, score_desc),
        "retry",
        true  // Recoverable — transient computation error
    );

    emitDiagnostic("TEDGE-04: Self-Similarity Failure", result);
    return result;
}

// ─────────────────────────────────────────────────────────────────────────
// TEDGE-05: Null/Empty Train Comparison
// ─────────────────────────────────────────────────────────────────────────

EdgeCaseResult TensorEdgeCaseHandler::handleNullTrainComparison(
    const std::string& key_a,
    const std::string& key_b,
    const std::string& null_train_key) noexcept {
    stats_.null_train_comparisons++;

    std::string null_info = null_train_key.empty()
        ? fmt::format("one or both")
        : fmt::format("'{}' (or both)", null_train_key);

    EdgeCaseResult result = EdgeCaseResult::Error(
        static_cast<int>(errors::ErrorCode::ERR_TENSOR_GRAPH_OTHER_TRAIN_NOT_FOUND),
        fmt::format(
            "Cannot compare adapters '{}' and '{}': {} has null/empty train",
            key_a, key_b, null_info),
        "fail-closed",
        false  // Not recoverable — trains must exist
    );

    emitDiagnostic("TEDGE-05: Null Train Comparison", result);
    return result;
}

// ─────────────────────────────────────────────────────────────────────────
// TEDGE-06: Concurrent Modification Detected
// ─────────────────────────────────────────────────────────────────────────

EdgeCaseResult TensorEdgeCaseHandler::handleConcurrentModification(
    const std::string& operation,
    const std::string& affected_key) noexcept {
    stats_.concurrent_modifications++;

    std::string affected_info = affected_key.empty()
        ? "(unknown adapter)"
        : fmt::format("('{}' modified)", affected_key);

    EdgeCaseResult result = EdgeCaseResult::Error(
        static_cast<int>(errors::ErrorCode::ERR_TENSOR_CONCURRENT_MODIFICATION),
        fmt::format(
            "Concurrent modification detected during '{}' {}",
            operation, affected_info),
        "retry",
        true  // Recoverable — retry with full lock
    );

    emitDiagnostic("TEDGE-06: Concurrent Modification", result);
    return result;
}

// ─────────────────────────────────────────────────────────────────────────
// TEDGE-07: Bridge Routing Failure
// ─────────────────────────────────────────────────────────────────────────

EdgeCaseResult TensorEdgeCaseHandler::handleBridgeRoutingFailure(
    double load_level,
    std::size_t queue_depth) noexcept {
    stats_.bridge_routing_failures++;

    EdgeCaseResult result = EdgeCaseResult::Error(
        static_cast<int>(errors::ErrorCode::ERR_TENSOR_INDEX_ROUTING_FAILED),
        fmt::format(
            "Bridge routing failed: system load {:.2f}%, routing queue depth {}",
            load_level * 100.0, queue_depth),
        "retry",
        true  // Recoverable — transient load issue
    );

    emitDiagnostic("TEDGE-07: Bridge Routing Failure", result);
    return result;
}

// ─────────────────────────────────────────────────────────────────────────
// TEDGE-08: Adapter Communication Failure
// ─────────────────────────────────────────────────────────────────────────

EdgeCaseResult TensorEdgeCaseHandler::handleAdapterCommunicationFailure(
    const std::string& adapter_key,
    uint32_t timeout_ms) noexcept {
    stats_.adapter_comm_failures++;

    EdgeCaseResult result = EdgeCaseResult::Error(
        static_cast<int>(errors::ErrorCode::ERR_TENSOR_ADAPTER_COMMUNICATION_ERROR),
        fmt::format(
            "Communication with adapter '{}' failed (timeout: {}ms)",
            adapter_key, timeout_ms),
        "retry",
        true  // Recoverable — transient network issue
    );

    emitDiagnostic("TEDGE-08: Adapter Communication Failure", result);
    return result;
}

// ─────────────────────────────────────────────────────────────────────────
// TEDGE-09: Invalid Decomposition Result
// ─────────────────────────────────────────────────────────────────────────

EdgeCaseResult TensorEdgeCaseHandler::handleInvalidDecompositionResult(
    const std::string& chunk_id,
    const std::string& error_detail) noexcept {
    stats_.invalid_decompositions++;

    EdgeCaseResult result = EdgeCaseResult::Error(
        static_cast<int>(errors::ErrorCode::ERR_TENSOR_FINGERPRINT_COMPUTATION_FAILED),
        fmt::format(
            "Invalid decomposition result for chunk '{}': {}",
            chunk_id, error_detail),
        "retry",
        true  // Recoverable — retry with different parameters
    );

    emitDiagnostic("TEDGE-09: Invalid Decomposition Result", result);
    return result;
}

// ─────────────────────────────────────────────────────────────────────────
// TEDGE-10: Kappa-Gate Threshold Violation
// ─────────────────────────────────────────────────────────────────────────

EdgeCaseResult TensorEdgeCaseHandler::handleKappaGateViolation(
    std::size_t embedding_dim,
    double estimated_kappa,
    double min_kappa) noexcept {
    stats_.kappa_gate_violations++;

    EdgeCaseResult result = EdgeCaseResult::Error(
        static_cast<int>(errors::ErrorCode::ERR_TENSOR_FINGERPRINT_COMPUTATION_FAILED),
        fmt::format(
            "Kappa-gate violation for dim={}: estimated κ={:.3f} < min_κ={:.3f}",
            embedding_dim, estimated_kappa, min_kappa),
        "degrade",
        true  // Recoverable — can degrade to non-compressed path
    );

    emitDiagnostic("TEDGE-10: Kappa-Gate Violation", result);
    return result;
}

// ─────────────────────────────────────────────────────────────────────────
// TEDGE-11: Export Serialization Failure
// ─────────────────────────────────────────────────────────────────────────

EdgeCaseResult TensorEdgeCaseHandler::handleExportSerializationFailure(
    const std::string& export_path,
    const std::string& error_detail) noexcept {
    stats_.export_serialization_failures++;

    EdgeCaseResult result = EdgeCaseResult::Error(
        static_cast<int>(errors::ErrorCode::ERR_TENSOR_PERSISTENCE_FAILED),
        fmt::format(
            "Failed to export graph to '{}': {}",
            export_path, error_detail),
        "retry",
        true  // Recoverable — transient I/O issue or permissions
    );

    emitDiagnostic("TEDGE-11: Export Serialization Failure", result);
    return result;
}

// ─────────────────────────────────────────────────────────────────────────
// TEDGE-12: Replay Deserialization Failure
// ─────────────────────────────────────────────────────────────────────────

EdgeCaseResult TensorEdgeCaseHandler::handleReplayDeserialization(
    const std::string& adapter_key,
    const std::string& corruption_type) noexcept {
    stats_.replay_deserialization_failures++;

    EdgeCaseResult result = EdgeCaseResult::Error(
        static_cast<int>(errors::ErrorCode::ERR_TENSOR_PERSISTENCE_FAILED),
        fmt::format(
            "Failed to deserialize adapter '{}' during replay: {}",
            adapter_key, corruption_type),
        "degrade",
        true  // Recoverable — skip corrupted entry or rebuild
    );

    emitDiagnostic("TEDGE-12: Replay Deserialization Failure", result);
    return result;
}

// ─────────────────────────────────────────────────────────────────────────
// TEDGE-13: Partial Graph Loss Recovery
// ─────────────────────────────────────────────────────────────────────────

EdgeCaseResult TensorEdgeCaseHandler::handlePartialGraphLossRecovery(
    std::size_t total_entries,
    std::size_t recovered_entries) noexcept {
    stats_.partial_graph_losses++;

    double recovery_pct = total_entries > 0
        ? (100.0 * static_cast<double>(recovered_entries) / total_entries)
        : 0.0;

    EdgeCaseResult result = EdgeCaseResult::Error(
        static_cast<int>(errors::ErrorCode::ERR_TENSOR_PERSISTENCE_FAILED),
        fmt::format(
            "Partial graph loss: recovered {} of {} entries ({:.1f}%)",
            recovered_entries, total_entries, recovery_pct),
        "degrade",
        true  // Recoverable — use partial graph or full rebuild
    );

    emitDiagnostic("TEDGE-13: Partial Graph Loss Recovery", result);
    return result;
}

// ─────────────────────────────────────────────────────────────────────────
// TEDGE-14: Out-of-Memory During Computation
// ─────────────────────────────────────────────────────────────────────────

EdgeCaseResult TensorEdgeCaseHandler::handleOutOfMemoryDuringComputation(
    std::size_t requested_bytes,
    const std::string& operation) noexcept {
    stats_.oom_during_computation++;

    double size_mb = static_cast<double>(requested_bytes) / (1024.0 * 1024.0);

    EdgeCaseResult result = EdgeCaseResult::Error(
        static_cast<int>(errors::ErrorCode::ERR_TENSOR_FINGERPRINT_COMPUTATION_FAILED),
        fmt::format(
            "Out-of-memory during {}: could not allocate {:.2f} MB",
            operation, size_mb),
        "fail-closed",
        false  // Not recoverable without freeing memory
    );

    emitDiagnostic("TEDGE-14: Out-of-Memory During Computation", result);
    return result;
}

// ─────────────────────────────────────────────────────────────────────────
// TEDGE-15: Concurrent Memory Exhaustion
// ─────────────────────────────────────────────────────────────────────────

EdgeCaseResult TensorEdgeCaseHandler::handleConcurrentMemoryExhaustion(
    std::size_t current_memory_bytes,
    std::size_t max_memory_bytes,
    std::size_t concurrent_threads) noexcept {
    stats_.concurrent_memory_exhaustion++;

    double current_mb = static_cast<double>(current_memory_bytes) / (1024.0 * 1024.0);
    double max_mb = static_cast<double>(max_memory_bytes) / (1024.0 * 1024.0);
    double usage_pct = max_memory_bytes > 0
        ? (100.0 * current_memory_bytes / max_memory_bytes)
        : 0.0;

    EdgeCaseResult result = EdgeCaseResult::Error(
        static_cast<int>(errors::ErrorCode::ERR_TENSOR_LOCK_ACQUISITION_FAILED),
        fmt::format(
            "Concurrent memory exhaustion: {:.2f} MB / {:.2f} MB ({:.1f}%) "
            "with {} concurrent threads",
            current_mb, max_mb, usage_pct, concurrent_threads),
        "retry",
        true  // Recoverable — throttle or cleanup
    );

    emitDiagnostic("TEDGE-15: Concurrent Memory Exhaustion", result);
    return result;
}

// ─────────────────────────────────────────────────────────────────────────
// Diagnostics & Statistics
// ─────────────────────────────────────────────────────────────────────────

void TensorEdgeCaseHandler::setDetailedDiagnostics([[maybe_unused]] bool enable) noexcept {
    detailed_diagnostics_ = enable;
}

TensorEdgeCaseHandler::EdgeCaseStats TensorEdgeCaseHandler::getStats() const noexcept {
    return stats_;
}

void TensorEdgeCaseHandler::resetStats() noexcept {
    stats_ = EdgeCaseStats();
}

// ─────────────────────────────────────────────────────────────────────────
// Private Helpers
// ─────────────────────────────────────────────────────────────────────────

void TensorEdgeCaseHandler::emitDiagnostic(
    const std::string& scenario,
    const EdgeCaseResult& result) const noexcept {
    if (!detailed_diagnostics_) {
        return;
    }

    auto logger = spdlog::get("themis");
    if (!logger) {
        return;
    }

    // Format timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    std::string timestamp = oss.str();

    // Build diagnostic message
    std::string diag = fmt::format(
        "[TENSOR_EDGE_CASE] {} | Code: {} | Message: {} | Recovery: {} | Recoverable: {} | Time: {}",
        scenario,
        result.error_code,
        result.error_message,
        result.recovery_action,
        result.is_recoverable ? "yes" : "no",
        timestamp
    );

    if (result.success) {
        logger->info(diag);
    } else if (result.is_recoverable) {
        logger->warn(diag);
    } else {
        logger->error(diag);
    }
}

std::string TensorEdgeCaseHandler::formatErrorMessage(
    const std::string& base_message,
    const std::string& context) const noexcept {
    if (context.empty()) {
        return base_message;
    }
    return fmt::format("{} [context: {}]", base_message, context);
}

bool TensorEdgeCaseHandler::isRecoverable([[maybe_unused]] int error_code) const noexcept {
    // Most tensor errors are potentially recoverable via retry or fallback
    // Only persist failures (9560+) are truly non-recoverable
    return error_code < static_cast<int>(errors::ErrorCode::ERR_TENSOR_RECOVERY_FAILED);
}

}  // namespace tensor
}  // namespace themis
