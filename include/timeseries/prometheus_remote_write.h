/**
 * @file prometheus_remote_write.h
 * @brief Phase 2 hardening: Validation error handling with bounded retry behavior.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Status: Phase 2 — Core Implementation Complete
 * 
 * ## Overview
 * 
 * PrometheusRemoteWrite implements parsing and validation of Prometheus remote-write protocol
 * requests with explicit error handling for all failure modes.
 * 
 * ## Key Features
 * 
 * - **Protobuf Wire-Format Parsing**: Deterministic decoder with bounds checking
 * - **Validation Error Handling**: Explicit errors for malformed requests (non-retryable)
 * - **Bounded Retry Support**: Max retry configuration prevents resource exhaustion
 * - **Error Taxonomy**: Distinct error codes for transport, format, and policy failures
 * - **Format Preservation**: Lossless decoding of Prometheus metric names and labels
 * 
 * ## Error Handling
 * 
 * Error codes returned in Result<T> when present:
 * - **REMOTE_WRITE_INVALID_FORMAT**: Malformed protobuf, truncated data, or invalid wire format
 * - **REMOTE_WRITE_ENDPOINT_UNAVAILABLE**: Network error or endpoint timeout (retryable)
 * - **REMOTE_WRITE_POLICY_VIOLATION**: Quota exceeded or unsupported metric type
 * 
 * ## Parsing Guarantees
 * 
 * 1. **Deterministic Decoding**: Same wire-format input → same parsed output
 * 2. **Bounds Checking**: No buffer overruns; truncated data returns INVALID_FORMAT
 * 3. **Varint Overflow Guard**: Varint overflow (≥64 bits) detected and rejected
 * 4. **Empty Handling**: Empty metric names and zero samples handled explicitly
 * 
 * ## Thread Safety
 * 
 * - All methods are stateless and thread-safe
 * - Parsing functions can be called concurrently from multiple threads
 * 
 * ## Integration Expectations
 * 
 * Callers should implement retry logic with exponential backoff for ENDPOINT_UNAVAILABLE errors:
 * - Max retries: configurable (suggest 3-5)
 * - Backoff: exponential (e.g., 100ms → 200ms → 400ms)
 * - Circuit breaker: After N consecutive failures, mark endpoint unavailable
 * 
 * @see include/timeseries/timeseries_api_contract.h
 * @see src/timeseries/ROADMAP.md — Phase 2 items
 */


#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "utils/expected.h"

namespace themis {
namespace timeseries {

/// A decoded Prometheus label (name=value pair).
struct PromLabel {
    std::string name;
    std::string value = {};
};

/// A decoded Prometheus sample (double value + millisecond timestamp).
struct PromSample {
    double   value        = 0.0;
    int64_t  timestamp_ms = 0;
};

/// A decoded Prometheus TimeSeries (labels + samples).
struct PromTimeSeries {
    std::vector<PromLabel>  labels;
    std::vector<PromSample> samples;

    /// Returns the value of the `__name__` label, or an empty string when absent.
    std::string metricName() const;

    /// Returns the value of the label with the given name, or an empty string.
    std::string labelValue(const std::string& name) const;
};

/// Decoded Prometheus WriteRequest.
struct PromWriteRequest {
    std::vector<PromTimeSeries> timeseries;

    /**
     * Decode a raw (uncompressed) protobuf-encoded WriteRequest.
     *
     * @param data  Pointer to the beginning of the serialised bytes.
     * @param size  Number of bytes.
        * @throws      Returns an error when the payload is malformed, including
        *              invalid/truncated protobuf wire start for the first
        *              timeseries field.
     * @return      Decoded request or an error string.
     */
    static Result<PromWriteRequest> decode(const uint8_t* data, size_t size);

    /**
     * Decode a snappy-compressed protobuf-encoded WriteRequest.
     *
     * Prometheus remote-write clients always compress with snappy before
     * sending; this is the primary entry-point used by the HTTP handler.
     *
     * @param data  Pointer to the beginning of the snappy-compressed bytes.
     * @param size  Number of bytes.
     * @return      Decoded request or an error string.
     */
    static Result<PromWriteRequest> decodeSnappy(const uint8_t* data, size_t size);
};

} // namespace timeseries
} // namespace themis
