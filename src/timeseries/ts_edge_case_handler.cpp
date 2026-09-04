// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file ts_edge_case_handler.cpp
 * @brief Implementation of TsEdgeCaseHandler.
 *
 * @see include/timeseries/ts_edge_case_handler.h
 * @see src/timeseries/ROADMAP.md — Phase 2/3 Q4 2026 items
 */

#include "timeseries/ts_edge_case_handler.h"

#include <algorithm>

namespace themis {
namespace timeseries {

// ============================================================================
// § 1  Construction
// ============================================================================

TsEdgeCaseHandler::TsEdgeCaseHandler(IncidentCallback on_incident) noexcept
    : on_incident_(std::move(on_incident)) {
}

// ============================================================================
// § 2  Remote-Write Validation
// ============================================================================

TsEdgeCaseResult TsEdgeCaseHandler::validateRemoteWriteEndpoint(
        std::string_view endpoint) const noexcept {

    if (endpoint.empty()) {
        emitIncident("TS-ECH-RW-EMPTY", "Remote-write endpoint URL is empty");
        validation_failure_count_.fetch_add(1, std::memory_order_relaxed);
        return TimeseriesErrorCode::REMOTE_WRITE_VALIDATION_ERROR;
    }
    if (static_cast<int>(endpoint.size()) > kMaxEndpointLength) {
        emitIncident("TS-ECH-RW-TOOLONG", "Remote-write endpoint URL exceeds maximum length");
        validation_failure_count_.fetch_add(1, std::memory_order_relaxed);
        return TimeseriesErrorCode::REMOTE_WRITE_VALIDATION_ERROR;
    }
    const bool http  = endpoint.size() >= 7 && endpoint.substr(0, 7) == "http://";
    const bool https = endpoint.size() >= 8 && endpoint.substr(0, 8) == "https://";
    if (!http && !https) {
        emitIncident("TS-ECH-RW-PROTOCOL",
                     "Remote-write endpoint must use http:// or https://");
        validation_failure_count_.fetch_add(1, std::memory_order_relaxed);
        return TimeseriesErrorCode::REMOTE_WRITE_VALIDATION_ERROR;
    }
    for (unsigned char c : endpoint) {
        if (c < 0x20 || c == 0x7F) {
            emitIncident("TS-ECH-RW-CONTROL",
                         "Remote-write endpoint URL contains control characters");
            validation_failure_count_.fetch_add(1, std::memory_order_relaxed);
            return TimeseriesErrorCode::REMOTE_WRITE_VALIDATION_ERROR;
        }
    }
    return std::nullopt; // success
}

TsEdgeCaseResult TsEdgeCaseHandler::handleRemoteWriteTimeout(
        std::string_view endpoint, uint32_t timeout_ms) noexcept {
    remote_write_timeout_count_.fetch_add(1, std::memory_order_relaxed);
    std::string desc = "Remote-write timeout after ";
    desc += std::to_string(timeout_ms);
    desc += " ms for endpoint: ";
    desc += std::string(endpoint.substr(0, std::min(endpoint.size(),
                                                     static_cast<std::size_t>(80))));
    emitIncident("TS-ECH-RW-TIMEOUT", desc);
    return TimeseriesErrorCode::REMOTE_WRITE_RETRIES_EXHAUSTED;
}

// ============================================================================
// § 3  Encrypted Chunk Edge Cases
// ============================================================================

TsEdgeCaseResult TsEdgeCaseHandler::validateEncryptionKey(
        const uint8_t* key_bytes, std::size_t key_len) const noexcept {
    if (!key_bytes || key_len == 0) {
        emitIncident("TS-ECH-ENC-NULL", "Encryption key is null or empty");
        validation_failure_count_.fetch_add(1, std::memory_order_relaxed);
        return TimeseriesErrorCode::ENCRYPTION_STATE_INVALID;
    }
    if (key_len != 16 && key_len != 24 && key_len != 32) {
        emitIncident("TS-ECH-ENC-KEYLEN", "Encryption key length must be 16, 24, or 32 bytes");
        validation_failure_count_.fetch_add(1, std::memory_order_relaxed);
        return TimeseriesErrorCode::ENCRYPTION_STATE_INVALID;
    }
    return std::nullopt; // success
}

TsEdgeCaseResult TsEdgeCaseHandler::handleKeyRotationDuringWrite(
        const uint8_t* new_key_bytes, std::size_t new_key_len) noexcept {
    auto ec = validateEncryptionKey(new_key_bytes, new_key_len);
    if (ec.has_value()) {
        emitIncident("TS-ECH-ENC-ROTATION-INVALID",
                     "Key rotation rejected: new key is invalid");
        return TimeseriesErrorCode::ENCRYPTION_KEY_NOT_FOUND;
    }
    key_rotation_count_.fetch_add(1, std::memory_order_relaxed);
    emitIncident("TS-ECH-ENC-ROTATION-QUEUED",
                 "Encryption key rotation queued; active write completes with old key");
    return std::nullopt; // success
}

// ============================================================================
// § 4  Buffer Pressure
// ============================================================================

bool TsEdgeCaseHandler::isOutOfOrderFlushRequired(
        std::size_t current_buffer_size) const noexcept {
    return current_buffer_size >= kMaxOutOfOrderBuffer;
}

// ============================================================================
// § 5  Diagnostic Counters
// ============================================================================

uint64_t TsEdgeCaseHandler::remoteWriteTimeoutCount() const noexcept {
    return remote_write_timeout_count_.load(std::memory_order_relaxed);
}

uint64_t TsEdgeCaseHandler::keyRotationCount() const noexcept {
    return key_rotation_count_.load(std::memory_order_relaxed);
}

uint64_t TsEdgeCaseHandler::validationFailureCount() const noexcept {
    return validation_failure_count_.load(std::memory_order_relaxed);
}

// ============================================================================
// § 6  Internal Helpers
// ============================================================================

void TsEdgeCaseHandler::emitIncident(
        std::string_view id, std::string_view desc) const noexcept {
    if (on_incident_) {
        try { on_incident_(id, desc); } catch (...) {}
    }
}

} // namespace timeseries
} // namespace themis
