/*
 * ThemisDB - Hybrid Database System
 * File: prometheus_remote_write.cpp
 *
 * Prometheus remote-write 1.0 wire-format decoder.
 *
 * Wire-format reference:
 *   https://prometheus.io/docs/concepts/remote_write_spec/
 *
 * Protobuf wire types used here:
 *   0 – VARINT  (int32, int64, uint32, uint64, bool, enum)
 *   1 – I64     (fixed64, sfixed64, double)
 *   2 – LEN     (string, bytes, embedded messages, packed repeated)
 *
 * Field tags for each message:
 *
 *   WriteRequest  { timeseries: field 1, wire 2 → tag 0x0A }
 *   TimeSeries    { labels:     field 1, wire 2 → tag 0x0A
 *                   samples:    field 2, wire 2 → tag 0x12 }
 *   Label         { name:       field 1, wire 2 → tag 0x0A
 *                   value:      field 2, wire 2 → tag 0x12 }
 *   Sample        { value:      field 1, wire 1 → tag 0x09
 *                   timestamp:  field 2, wire 0 → tag 0x10 }
 */

#include "timeseries/prometheus_remote_write.h"
#include "utils/error_registry.h"
#include <cstring>
#include <cstdint>
#include <snappy.h>

namespace themis {
namespace timeseries {

// ─────────────────────────────────────────────
// Internal minimal protobuf wire-format decoder
// ─────────────────────────────────────────────

namespace proto {

/// Read a protobuf varint from [pos, end).  Advances *pos past the varint.
/// Returns false when the buffer is exhausted before the varint terminates.
static bool readVarint64(const uint8_t* buf, size_t& pos, size_t end, uint64_t& out) {
    out = 0;
    int shift = 0;
    while (pos < end) {
        uint8_t b = buf[pos++];
        out |= static_cast<uint64_t>(b & 0x7F) << shift;
        if ((b & 0x80) == 0) return true;
        shift += 7;
        if (shift >= 64) return false; // overflow guard
    }
    return false; // truncated
}

/// Read a length-delimited byte span.  Returns false on error.
static bool readLenDelim(const uint8_t* buf, size_t& pos, size_t end,
                         const uint8_t*& span_begin, size_t& span_len) {
    uint64_t len = 0;
    if (!readVarint64(buf, pos, end, len)) return false;
    if (pos + len > end) return false;
    span_begin = buf + pos;
    span_len   = static_cast<size_t>(len);
    pos += span_len;
    return true;
}

/// Skip a field of unknown type given the wire_type nibble.
static bool skipField(const uint8_t* buf, size_t& pos, size_t end, uint32_t wire_type) {
    switch (wire_type) {
        case 0: { // VARINT
            uint64_t ignored = 0;
            return readVarint64(buf, pos, end, ignored);
        }
        case 1: { // I64 – 8 bytes
            if (pos + 8 > end) return false;
            pos += 8;
            return true;
        }
        case 2: { // LEN
            const uint8_t* ignored = nullptr;
            size_t len = 0;
            return readLenDelim(buf, pos, end, ignored, len);
        }
        case 5: { // I32 – 4 bytes
            if (pos + 4 > end) return false;
            pos += 4;
            return true;
        }
        default:
            return false; // unknown wire type
    }
}

// ── Label ─────────────────────────────────────────────────────────────────────

static bool decodeLabel(const uint8_t* buf, size_t size, PromLabel& out) {
    size_t pos = 0;
    while (pos < size) {
        uint64_t tag_raw = 0;
        if (!readVarint64(buf, pos, size, tag_raw)) return false;
        uint32_t field_number = static_cast<uint32_t>(tag_raw >> 3);
        uint32_t wire_type    = static_cast<uint32_t>(tag_raw & 0x07);

        if (wire_type == 2) {
            const uint8_t* span = nullptr;
            size_t span_len = 0;
            if (!readLenDelim(buf, pos, size, span, span_len)) return false;

            if (field_number == 1) {
                out.name.assign(reinterpret_cast<const char*>(span), span_len);
            } else if (field_number == 2) {
                out.value.assign(reinterpret_cast<const char*>(span), span_len);
            }
            // else: unknown field – already consumed by readLenDelim
        } else {
            if (!skipField(buf, pos, size, wire_type)) return false;
        }
    }
    return true;
}

// ── Sample ────────────────────────────────────────────────────────────────────

static bool decodeSample(const uint8_t* buf, size_t size, PromSample& out) {
    size_t pos = 0;
    while (pos < size) {
        uint64_t tag_raw = 0;
        if (!readVarint64(buf, pos, size, tag_raw)) return false;
        uint32_t field_number = static_cast<uint32_t>(tag_raw >> 3);
        uint32_t wire_type    = static_cast<uint32_t>(tag_raw & 0x07);

        if (field_number == 1 && wire_type == 1) {
            // double value – I64 (8 bytes little-endian IEEE 754)
            if (pos + 8 > size) return false;
            static_assert(sizeof(double) == 8, "double must be 64-bit");
            std::memcpy(&out.value, buf + pos, 8);
            pos += 8;
        } else if (field_number == 2 && wire_type == 0) {
            // int64 timestamp
            uint64_t ts = 0;
            if (!readVarint64(buf, pos, size, ts)) return false;
            // Re-interpret as signed int64 (two's complement)
            out.timestamp_ms = static_cast<int64_t>(ts);
        } else {
            if (!skipField(buf, pos, size, wire_type)) return false;
        }
    }
    return true;
}

// ── TimeSeries ────────────────────────────────────────────────────────────────

static bool decodeTimeSeries(const uint8_t* buf, size_t size, PromTimeSeries& out) {
    size_t pos = 0;
    while (pos < size) {
        uint64_t tag_raw = 0;
        if (!readVarint64(buf, pos, size, tag_raw)) return false;
        uint32_t field_number = static_cast<uint32_t>(tag_raw >> 3);
        uint32_t wire_type    = static_cast<uint32_t>(tag_raw & 0x07);

        if (wire_type == 2) {
            const uint8_t* span = nullptr;
            size_t span_len = 0;
            if (!readLenDelim(buf, pos, size, span, span_len)) return false;

            if (field_number == 1) {
                PromLabel label;
                if (!decodeLabel(span, span_len, label)) return false;
                out.labels.push_back(std::move(label));
            } else if (field_number == 2) {
                PromSample sample;
                if (!decodeSample(span, span_len, sample)) return false;
                out.samples.push_back(std::move(sample));
            }
            // else: unknown field – already consumed
        } else {
            if (!skipField(buf, pos, size, wire_type)) return false;
        }
    }
    return true;
}

// ── WriteRequest ──────────────────────────────────────────────────────────────

static bool decodeWriteRequest(const uint8_t* buf, size_t size, PromWriteRequest& out) {
    size_t pos = 0;
    while (pos < size) {
        uint64_t tag_raw = 0;
        if (!readVarint64(buf, pos, size, tag_raw)) return false;
        uint32_t field_number = static_cast<uint32_t>(tag_raw >> 3);
        uint32_t wire_type    = static_cast<uint32_t>(tag_raw & 0x07);

        if (wire_type == 2) {
            const uint8_t* span = nullptr;
            size_t span_len = 0;
            if (!readLenDelim(buf, pos, size, span, span_len)) return false;

            if (field_number == 1) {
                PromTimeSeries ts;
                if (!decodeTimeSeries(span, span_len, ts)) return false;
                out.timeseries.push_back(std::move(ts));
            }
            // else: unknown field – already consumed
        } else {
            if (!skipField(buf, pos, size, wire_type)) return false;
        }
    }
    return true;
}

} // namespace proto

// ─────────────────────────────────────────────
// PromTimeSeries helpers
// ─────────────────────────────────────────────

std::string PromTimeSeries::metricName() const {
    return labelValue("__name__");
}

std::string PromTimeSeries::labelValue(const std::string& name) const {
    for (const auto& l : labels) {
        if (l.name == name) return l.value;
    }
    return {};
}

// ─────────────────────────────────────────────
// PromWriteRequest decode entry-points
// ─────────────────────────────────────────────

Result<PromWriteRequest> PromWriteRequest::decode(const uint8_t* data, size_t size) {
    PromWriteRequest req;
    if (!proto::decodeWriteRequest(data, size, req)) {
        return Err<PromWriteRequest>(errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
                                    "Failed to decode Prometheus WriteRequest protobuf");
    }
    return req;
}

Result<PromWriteRequest> PromWriteRequest::decodeSnappy(const uint8_t* data, size_t size) {
    // Step 1: determine uncompressed length
    size_t uncompressed_len = 0;
    if (!snappy::GetUncompressedLength(
            reinterpret_cast<const char*>(data), size, &uncompressed_len)) {
        return Err<PromWriteRequest>(errors::ErrorCode::ERR_COMPRESSION_INVALID_FORMAT,
                                    "Failed to determine snappy uncompressed length");
    }

    // Step 2: decompress
    std::string uncompressed;
    uncompressed.resize(uncompressed_len);
    if (!snappy::RawUncompress(
            reinterpret_cast<const char*>(data), size,
            &uncompressed[0])) {
        return Err<PromWriteRequest>(errors::ErrorCode::ERR_COMPRESSION_INVALID_FORMAT,
                                    "Snappy decompression failed");
    }

    // Step 3: decode protobuf
    return decode(reinterpret_cast<const uint8_t*>(uncompressed.data()),
                  uncompressed.size());
}

} // namespace timeseries
} // namespace themis
