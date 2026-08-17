// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file ts_edge_case_handler.h
 * @brief Deterministic edge-case handler for remote-write and encrypted chunk
 *        scenarios in the ThemisDB timeseries module.
 *
 * Provides TsEdgeCaseHandler, which enforces deterministic fail-safe behavior
 * across:
 *
 *   - Remote-write validation errors (invalid endpoint, auth failure, timeout)
 *   - Encrypted chunk edge cases (null key, rotation during write, truncated chunk)
 *   - Buffer pressure edge cases (full out-of-order buffer, forced re-sort trigger)
 *   - Retention policy boundary conditions (exact boundary eviction, concurrent expiry)
 *
 * ## Result Convention
 *
 * All operations return `std::optional<TimeseriesErrorCode>`:
 *   - `std::nullopt` — success
 *   - non-nullopt — the specific error code describing the failure
 *
 * ## Design Principles
 *
 * 1. **Fail-Safe**: all error paths return structured error codes rather than
 *    propagating exceptions to callers.
 * 2. **Bounded Behavior**: all operations complete within documented time bounds
 *    or emit a TIMEOUT incident.
 * 3. **Isolation**: encryption key lifecycle events do not interrupt in-flight
 *    ingest or query operations.
 * 4. **Thread-Safety**: all public methods are safe for concurrent invocation.
 *
 * @see include/timeseries/timeseries_api_contract.h  — base error taxonomy
 * @see src/timeseries/ROADMAP.md — Phase 2/3 Q4 2026 items
 */

#pragma once

#include "timeseries/timeseries_api_contract.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace themis {
namespace timeseries {

/**
 * @brief Result type for TsEdgeCaseHandler operations.
 *
 * Convention: nullopt = success; non-nullopt = error code.
 */
using TsEdgeCaseResult = std::optional<TimeseriesErrorCode>;

// ============================================================================
// § 1  TsEdgeCaseHandler
// ============================================================================

/**
 * @brief Deterministic edge-case handler for timeseries remote-write and
 *        encrypted chunk operations.
 *
 * Thread safety: all public methods are thread-safe via atomic counters.
 * The incident callback must itself be thread-safe if called concurrently.
 */
class TsEdgeCaseHandler {
public:
    /// @brief Incident callback type.
    using IncidentCallback = std::function<void(std::string_view id,
                                                std::string_view desc)>;

    /**
     * @brief Construct the handler with an optional incident callback.
     * @param on_incident Callback invoked on incident emission (may be null).
     */
    explicit TsEdgeCaseHandler(IncidentCallback on_incident = nullptr) noexcept;

    // Non-copyable; movable.
    TsEdgeCaseHandler(const TsEdgeCaseHandler&) = delete;
    TsEdgeCaseHandler& operator=(const TsEdgeCaseHandler&) = delete;
    TsEdgeCaseHandler(TsEdgeCaseHandler&&) noexcept = default;
    TsEdgeCaseHandler& operator=(TsEdgeCaseHandler&&) noexcept = default;

    // -------------------------------------------------------------------------
    // § 1.1  Remote-Write Validation
    // -------------------------------------------------------------------------

    /**
     * @brief Validate a remote-write endpoint URL.
     *
     * Checks:
     *   - Non-empty URL
     *   - URL starts with http:// or https://
     *   - URL does not exceed kMaxEndpointLength (2048)
     *   - URL does not contain control characters
     *
     * @param endpoint The remote-write endpoint URL to validate.
     * @return nullopt on success; REMOTE_WRITE_VALIDATION_ERROR on failure.
     */
    [[nodiscard]] TsEdgeCaseResult validateRemoteWriteEndpoint(
        std::string_view endpoint) const noexcept;

    /**
     * @brief Handle a remote-write timeout event.
     *
     * Records the timeout incident and returns REMOTE_WRITE_RETRIES_EXHAUSTED.
     *
     * @param endpoint   The endpoint that timed out.
     * @param timeout_ms Timeout duration in milliseconds.
     * @return REMOTE_WRITE_RETRIES_EXHAUSTED always; incident emitted.
     */
    [[nodiscard]] TsEdgeCaseResult handleRemoteWriteTimeout(
        std::string_view endpoint, uint32_t timeout_ms) noexcept;

    // -------------------------------------------------------------------------
    // § 1.2  Encrypted Chunk Edge Cases
    // -------------------------------------------------------------------------

    /**
     * @brief Validate an encryption key before a chunk write operation.
     *
     * Returns ENCRYPTION_STATE_INVALID if:
     *   - key_bytes is null or empty
     *   - key_len is not 16, 24, or 32 (AES-128/192/256)
     *
     * @param key_bytes  Pointer to the raw key material.
     * @param key_len    Length of the key in bytes.
     * @return nullopt on success; ENCRYPTION_STATE_INVALID on failure.
     */
    [[nodiscard]] TsEdgeCaseResult validateEncryptionKey(
        const uint8_t* key_bytes, std::size_t key_len) const noexcept;

    /**
     * @brief Handle an encryption key rotation event during an active chunk write.
     *
     * Implements isolation semantics: in-flight write uses old key; new key
     * is queued for subsequent writes.  Returns ENCRYPTION_KEY_NOT_FOUND if
     * the new key fails validation.
     *
     * @param new_key_bytes Pointer to new key material.
     * @param new_key_len   Length of new key in bytes.
     * @return nullopt when rotation is queued; ENCRYPTION_KEY_NOT_FOUND on invalid key.
     */
    [[nodiscard]] TsEdgeCaseResult handleKeyRotationDuringWrite(
        const uint8_t* new_key_bytes, std::size_t new_key_len) noexcept;

    // -------------------------------------------------------------------------
    // § 1.3  Buffer Pressure
    // -------------------------------------------------------------------------

    /**
     * @brief Returns true when the out-of-order buffer has reached capacity.
     *
     * A forced re-sort flush must be triggered when this returns true.
     *
     * @param current_buffer_size Current out-of-order buffer element count.
     * @return true when flush is required.
     */
    [[nodiscard]] bool isOutOfOrderFlushRequired(
        std::size_t current_buffer_size) const noexcept;

    // -------------------------------------------------------------------------
    // § 1.4  Diagnostic Counters
    // -------------------------------------------------------------------------

    /// @brief Number of remote-write timeout incidents recorded.
    [[nodiscard]] uint64_t remoteWriteTimeoutCount() const noexcept;

    /// @brief Number of encryption key rotation events handled.
    [[nodiscard]] uint64_t keyRotationCount() const noexcept;

    /// @brief Number of validation failures recorded.
    [[nodiscard]] uint64_t validationFailureCount() const noexcept;

private:
    IncidentCallback      on_incident_;
    std::atomic<uint64_t> remote_write_timeout_count_{0};
    std::atomic<uint64_t> key_rotation_count_{0};
    std::atomic<uint64_t> validation_failure_count_{0};

    static constexpr std::size_t kMaxEndpointLength = 2048;

    void emitIncident(std::string_view id, std::string_view desc) const noexcept;
};

} // namespace timeseries
} // namespace themis
