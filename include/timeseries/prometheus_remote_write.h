/*
 * ThemisDB - Hybrid Database System
 * File: prometheus_remote_write.h
 *
 * Prometheus remote-write endpoint compatibility.
 *
 * Implements decoding of the Prometheus remote-write 1.0 wire format:
 *   - Snappy-compressed Protocol Buffer payload
 *   - WriteRequest { repeated TimeSeries timeseries = 1; }
 *   - TimeSeries   { repeated Label labels = 1; repeated Sample samples = 2; }
 *   - Label        { string name = 1; string value = 2; }
 *   - Sample       { double value = 1; int64 timestamp = 2; }
 *
 * Reference: https://prometheus.io/docs/concepts/remote_write_spec/
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
    std::string value;
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
