/*
 * ThemisDB | File: prometheus_remote_write.h | Version: 0.0.15
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
