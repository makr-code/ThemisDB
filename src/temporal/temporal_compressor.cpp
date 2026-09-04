/**
 * @file temporal_compressor.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=0, M=7, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Temporal Data Compressor Implementation
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "temporal/temporal_compressor.h"
#include <lz4.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <unordered_map>

namespace themisdb {
namespace temporal {

// ============================================================================
// Base64 helpers (RFC 4648 – no line wrapping)
// ============================================================================

static constexpr const char kBase64Chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string TemporalCompressor::base64Encode(const std::string& input) {
    std::string out = {};
    out.reserve(((static_cast<int>(input.size()) + 2) / 3) * 4);
    const auto* data = reinterpret_cast<const unsigned char*>(input.data());
    size_t i = 0;
    for (; i + 2 < input.size(); i += 3) {
        uint32_t v = (uint32_t(data[i]) << 16) | (uint32_t(data[i+1]) << 8) | data[i+2];
        out += kBase64Chars[(v >> 18) & 0x3F];
        out += kBase64Chars[(v >> 12) & 0x3F];
        out += kBase64Chars[(v >>  6) & 0x3F];
        out += kBase64Chars[(v      ) & 0x3F];
    }
    if (i + 1 == input.size()) {
        uint32_t v = uint32_t(data[i]) << 16;
        out += kBase64Chars[(v >> 18) & 0x3F];
        out += kBase64Chars[(v >> 12) & 0x3F];
        out += '=';
        out += '=';
    } else if (i + 2 == input.size()) {
        uint32_t v = (uint32_t(data[i]) << 16) | (uint32_t(data[i+1]) << 8);
        out += kBase64Chars[(v >> 18) & 0x3F];
        out += kBase64Chars[(v >> 12) & 0x3F];
        out += kBase64Chars[(v >>  6) & 0x3F];
        out += '=';
    }
    return out;
}

static int base64CharValue(char c) {
    if (c >= 'A' && c <= 'Z') {
      return c - 'A';
    }
    if (c >= 'a' && c <= 'z') {
      return c - 'a' + 26;
    }
    if (c >= '0' && c <= '9') {
      return c - '0' + 52;
    }
    if (c == '+') {
      return 62;
    }
    if (c == '/') {
      return 63;
    }
    return -1;
}

std::string TemporalCompressor::base64Decode(const std::string& input) {
    std::string out = {};
    if (input.empty() || input.size() % 4 != 0) {
      return out;
    }
    out.reserve((input.size() / 4) * 3);
    for (size_t i = 0; i < input.size(); i += 4) {
        int a = base64CharValue(input[i]);
        int b = base64CharValue(input[i+1]);
        int c = input[i+2] == '=' ? 0 : base64CharValue(input[i+2]);
        int d = input[i+3] == '=' ? 0 : base64CharValue(input[i+3]);
        if (a < 0 || b < 0) {
          break;
        }
        uint32_t v = (uint32_t(a) << 18) | (uint32_t(b) << 12) | (uint32_t(c) << 6) | uint32_t(d);
        out += char((v >> 16) & 0xFF);
        if (input[i+2] != '=') {
          out += char((v >>  8) & 0xFF);
        }
        if (input[i+3] != '=') {
          out += char((v      ) & 0xFF);
        }
    }
    return out;
}

// ============================================================================
// Run-length encoder (simple byte-level LZ surrogate used for ZSTD mode)
//
// Format: sequences of repeated bytes → <repeat_marker><count><byte>
//         Other bytes are passed through verbatim.
// ============================================================================

static constexpr unsigned char kRlRepeatMarker = 0x01;
static constexpr unsigned char kRlEscapeByte   = 0x02;

std::string TemporalCompressor::rlEncode(const std::string& input) {
    if (input.empty()) return {};
    std::string out = {};
    out.reserve(input.size());
    size_t i = 0;
    while (static_cast<size_t>(i) < input.size()) {
        unsigned char cur = static_cast<unsigned char>(input[i]);
        size_t run = 1;
        while (i + run < input.size() &&
               static_cast<unsigned char>(input[i + run]) == cur &&
               run < 255) {
            ++run;
        }
        if (run >= 4 || cur == kRlRepeatMarker || cur == kRlEscapeByte) {
            out += static_cast<char>(kRlRepeatMarker);
            out += static_cast<char>(static_cast<unsigned char>(run));
            out += static_cast<char>(cur);
        } else {
            // Escape literal repeat-marker or escape bytes
            for (size_t j = 0; j < run; ++j) {
                out += static_cast<char>(cur);
            }
        }
        i += run;
    }
    return out;
}

std::string TemporalCompressor::rlDecode(const std::string& input) {
    std::string out = {};
    out.reserve(input.size() * 2);
    size_t i = 0;
    while (static_cast<size_t>(i) < input.size()) {
        unsigned char byte = static_cast<unsigned char>(input[i]);
        if (byte == kRlRepeatMarker && i + 2 < input.size()) {
            unsigned char count = static_cast<unsigned char>(input[i+1]);
            char ch = input[i+2];
            for (unsigned char k = 0; k < count; ++k) {
              out += ch;
            }
            i += 3;
        } else {
            out += static_cast<char>(byte);
            ++i;
        }
    }
    return out;
}

// ============================================================================
// Algorithm: ZSTD (simulated via RL-encode + base64)
// ============================================================================

nlohmann::json TemporalCompressor::applyZstd(const nlohmann::json& doc, int /*level*/) {
    const std::string raw = doc.dump();
    const std::string encoded = base64Encode(rlEncode(raw));
    return nlohmann::json{
        {"__compressed", "zstd"},
        {"__data",       encoded},
        {"__original_size",static_cast<int>(raw.size())}
    };
}

nlohmann::json TemporalCompressor::decompressZstd(const nlohmann::json& doc) {
    const std::string encoded = doc.at("__data").get<std::string>();
    const std::string raw = rlDecode(base64Decode(encoded));
    return nlohmann::json::parse(raw);
}

// ============================================================================
// Algorithm: DELTA (JSON field-level patch)
//
// A delta payload looks like:
//   { "__compressed": "delta",
//     "__base_ref":   "<key>@<sys_start_ms>",
//     "__patch":      { <field>: <new_value>, ... },
//     "__removed":    [<field>, ...] }
//
// Fields absent from __patch and __removed are unchanged from the base.
// ============================================================================

nlohmann::json TemporalCompressor::applyDelta(const nlohmann::json& base,
                                               const nlohmann::json& current,
                                               const std::string& base_ref) {
    nlohmann::json patch = nlohmann::json::object();
    nlohmann::json removed = nlohmann::json::array();

    // Fields in current that differ from base
    for (auto& [field, val] : current.items()) {
        if (!base.contains(field) || base.at(field) != val) {
            patch[field] = val;
        }
    }
    // Fields in base that are absent from current
    for (auto& [field, _] : base.items()) {
        if (!current.contains(field)) {
            removed.push_back(field);
        }
    }

    return nlohmann::json{
        {"__compressed", "delta"},
        {"__base_ref",   base_ref},
        {"__patch",      patch},
        {"__removed",    removed}
    };
}

// ============================================================================
// Algorithm: GORILLA (XOR-delta for numeric fields)
//
// For each numeric field across a version chain, we store:
//   { "__compressed": "gorilla",
//     "__field":      "<name>",
//     "__timestamps": [t0, t1-t0, t2-t1, ...],   // delta-encoded
//     "__values":     [v0_bits, xor1, xor2, ...]   // XOR-delta as hex strings
//   }
//
// Non-numeric fields fall back to the raw value.
// ============================================================================

nlohmann::json TemporalCompressor::applyGorilla(
    const std::string& field_name,
    const std::vector<std::pair<Timestamp, double>>& series) {

    if (series.empty()) {
      return nlohmann::json::object();
    }

    // Delta-encode timestamps
    nlohmann::json ts_arr = nlohmann::json::array();
    ts_arr.push_back(series[0].first);
    for (size_t i = 1; i < series.size(); ++i)
        ts_arr.push_back(series[i].first - series[static_cast<int>(i - 1)].first);

    // XOR-delta encode doubles (store as uint64 bit patterns)
    nlohmann::json val_arr = nlohmann::json::array();
    uint64_t prev_bits = 0;
    for (size_t i = 0; i < series.size(); ++i) {
        uint64_t bits = 0;
        static_assert(sizeof(double) == sizeof(uint64_t), "");
        std::memcpy(&bits, &series[i].second, sizeof(bits));
        val_arr.push_back(bits ^ prev_bits);
        prev_bits = bits;
    }

    return nlohmann::json{
        {"__compressed", "gorilla"},
        {"__field",      field_name},
        {"__timestamps", ts_arr},
        {"__values",     val_arr}
    };
}

// ============================================================================
// Algorithm: DICTIONARY
//
// Builds a per-field value dictionary across the version batch.
// Each string value is replaced with its integer index.
// ============================================================================

nlohmann::json TemporalCompressor::applyDictionary(
    const nlohmann::json& doc,
    std::unordered_map<std::string, std::unordered_map<std::string, int>>& dicts) {

    nlohmann::json encoded = nlohmann::json::object();
    for (auto& [field, val] : doc.items()) {
        if (val.is_string()) {
            auto& dict = dicts[field];
            const std::string& s = val.get<std::string>();
            auto it = dict.find(s);
            if (it == dict.end()) {
                int idx = static_cast<int>(dict.size());
                dict[s] = idx;
                it = dict.find(s);
            }
            encoded[field] = it->second;
        } else {
            encoded[field] = val;
        }
    }
    return nlohmann::json{
        {"__compressed", "dictionary"},
        {"__doc",        encoded}
    };
}

// ============================================================================
// ============================================================================
// Algorithm: LZ4 — high-throughput block compression
// ============================================================================
//
// Payload format stored in the history table:
//   { "__compressed": "lz4",
//     "__original_size": <int>,      // original JSON string byte count
//     "__data": "<base64-encoded LZ4 compressed block>"
//   }

nlohmann::json TemporalCompressor::applyLz4(const nlohmann::json& doc) {
    const std::string src = doc.dump();
    const int src_size    = static_cast<int>(src.size());

    // LZ4_compressBound gives the worst-case output size.
    const int max_dst = LZ4_compressBound(src_size);
    std::string dst(static_cast<size_t>(max_dst), '\0');

    const int compressed_size = LZ4_compress_default(
        src.data(),
        dst.data(),
        src_size,
        max_dst);

    if (compressed_size <= 0) {
        // Compression failed — return doc unchanged so data is not lost.
        return doc;
    }

    dst.resize(static_cast<size_t>(compressed_size));

    return nlohmann::json{
        {"__compressed",    "lz4"},
        {"__original_size", src_size},
        {"__data",          base64Encode(dst)}
    };
}

nlohmann::json TemporalCompressor::decompressLz4(const nlohmann::json& doc) {
    if (!doc.contains("__original_size") || !doc.contains("__data")) {
        return doc;
    }

    const int original_size = doc["__original_size"].get<int>();
    if (original_size <= 0) {
        return doc;
    }

    const std::string encoded     = doc["__data"].get<std::string>();
    const std::string compressed  = base64Decode(encoded);

    std::string decompressed(static_cast<size_t>(original_size), '\0');

    const int decompressed_size = LZ4_decompress_safe(
        compressed.data(),
        decompressed.data(),
        static_cast<int>(compressed.size()),
        original_size);

    if (decompressed_size < 0 || decompressed_size != original_size) {
        return doc;  // Decompression error — return marker document.
    }

    try {
        return nlohmann::json::parse(decompressed);
    } catch (...) {
        return doc;
    }
}

// algorithmName
// ============================================================================

std::string TemporalCompressor::algorithmName(CompressionAlgorithm algo) {
    switch (algo) {
        case CompressionAlgorithm::DELTA:      return "DELTA";
        case CompressionAlgorithm::ZSTD:       return "ZSTD";
        case CompressionAlgorithm::GORILLA:    return "GORILLA";
        case CompressionAlgorithm::DICTIONARY: return "DICTIONARY";
        case CompressionAlgorithm::LZ4:        return "LZ4";
    }
    return "UNKNOWN";
}

// ============================================================================
// decompress (dispatcher)
// ============================================================================

nlohmann::json TemporalCompressor::decompress(const nlohmann::json& doc) {
    if (!doc.is_object() || !doc.contains("__compressed")) {
        return doc;  // Not a compressed payload
    }
    const std::string tag = doc["__compressed"].get<std::string>();

    if (tag == "zstd") {
        return decompressZstd(doc);
    }
    if (tag == "lz4") {
        return decompressLz4(doc);
    }
    if (tag == "delta") {
        // Cannot decompress delta without the base; return as-is.
        // Full chain decompression requires access to the table history.
        return doc;
    }
    if (tag == "gorilla" || tag == "dictionary") {
        // Structured decompression requires field-level context;
        // return marker document so callers can detect compressed state.
        return doc;
    }
    return doc;
}

// ============================================================================
// compressHistory — primary entry point
// ============================================================================

CompressionStats TemporalCompressor::compressHistory(
    SystemVersionedTable& table,
    const TimeRange& range,
    const CompressionConfig& config) {

    std::lock_guard<std::mutex> lk(mutex_);

    const auto wall_start = std::chrono::steady_clock::now();

    CompressionStats stats;

    // Grace window: skip versions younger than delay_before_compression
    Timestamp cutoff_start = kMinTimestamp;
    if (!config.compress_immediately && config.delay_before_compression.count() > 0) {
        cutoff_start = now() -
            static_cast<Timestamp>(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    config.delay_before_compression).count());
    }

    const std::vector<std::string> keys = table.getAllKeys();

    // DICTIONARY algorithm needs a pass to build the global dictionary first
    std::unordered_map<std::string, std::unordered_map<std::string, int>> global_dicts;

    for (const auto& key : keys) {
        const auto versions = table.getHistoryInRange(key, range);

        // Filter to closed (historical) versions only; skip current versions
        // and versions newer than the grace window.
        std::vector<const VersionedDocument*> candidates = {};

        for (const auto& v : versions) {
            if (v.isCurrent()) {
              continue;
            }
            ++stats.versions_processed;
            if (!config.compress_immediately && v.sys_time.start >= cutoff_start) {
                ++stats.versions_skipped;
                continue;
            }
            candidates.push_back(&v);
        }

        if (candidates.empty()) {
          continue;
        }

        // Sort candidates by sys_time.start ascending for DELTA/GORILLA
        std::vector<VersionedDocument> sorted_versions = {};

        for (const auto* vp : candidates) {
          sorted_versions.push_back(*vp);
        }
        std::sort(sorted_versions.begin(), sorted_versions.end(),
                  [](const VersionedDocument& a, const VersionedDocument& b) {
                      return a.sys_time.start < b.sys_time.start;
                  });

        switch (config.algorithm) {
        // ── DELTA ──────────────────────────────────────────────────────────
        case CompressionAlgorithm::DELTA: {
            for (size_t i = 1; i < sorted_versions.size(); ++i) {
                const auto& base    = sorted_versions[static_cast<int>(i - 1)];
                const auto& current = sorted_versions[i];

                const std::string original_str = current.data.dump();
                stats.original_size_bytes += original_str.size();

                const std::string base_ref =
                    key + "@" + std::to_string(base.sys_time.start);
                nlohmann::json compressed =
                    applyDelta(base.data, current.data, base_ref);

                const std::string compressed_str = compressed.dump();
                stats.compressed_size_bytes += compressed_str.size();

                bool ok = table.replaceHistoricalPayload(
                    key, current.sys_time.start, compressed);
                if (ok) {
                    ++stats.versions_compressed;
                } else {
                    stats.errors.emplace_back(
                        key, "replaceHistoricalPayload failed for start=" +
                                 std::to_string(current.sys_time.start));
                }
            }
            // First version of the key is the baseline — count it as-is
            if (!sorted_versions.empty()) {
                const std::string s = sorted_versions[0].data.dump();
                stats.original_size_bytes   += s.size();
                stats.compressed_size_bytes += s.size();
            }
            break;
        }

        // ── ZSTD ───────────────────────────────────────────────────────────
        case CompressionAlgorithm::ZSTD: {
            for (const auto& v : sorted_versions) {
                const std::string original_str = v.data.dump();
                stats.original_size_bytes += original_str.size();

                nlohmann::json compressed = applyZstd(v.data, config.compression_level);
                const std::string compressed_str = compressed.dump();
                stats.compressed_size_bytes += compressed_str.size();

                bool ok = table.replaceHistoricalPayload(
                    key, v.sys_time.start, compressed);
                if (ok) {
                    ++stats.versions_compressed;
                } else {
                    stats.errors.emplace_back(
                        key, "replaceHistoricalPayload failed for start=" +
                                 std::to_string(v.sys_time.start));
                }
            }
            break;
        }

        // ── GORILLA ────────────────────────────────────────────────────────
        case CompressionAlgorithm::GORILLA: {
            if (sorted_versions.empty()) {
              break;
            }

            // Collect all numeric field names from first version
            std::vector<std::string> numeric_fields = {};

            for (auto& [f, val] : sorted_versions[0].data.items()) {
                if (val.is_number()) {
                  numeric_fields.push_back(f);
                }
            }

            // Build per-field time series
            std::unordered_map<std::string,
                std::vector<std::pair<Timestamp, double>>> series_map;
            for (const auto& f : numeric_fields) {
                auto& s = series_map[f];
                for (const auto& v : sorted_versions) {
                    if (v.data.contains(f) && v.data[f].is_number())
                        s.emplace_back(v.sys_time.start,
                                       v.data[f].get<double>());
                }
            }

            // Encode each version: numeric fields → Gorilla ref, others verbatim
            for (size_t i = 0; i < sorted_versions.size(); ++i) {
                const auto& v = sorted_versions[i];
                const std::string orig = v.data.dump();
                stats.original_size_bytes += orig.size();

                nlohmann::json compressed_doc = nlohmann::json::object();
                for (auto& [f, val] : v.data.items()) {
                    if (val.is_number()) {
                        // Reference into the compressed Gorilla series by index
                        compressed_doc[f] = nlohmann::json{
                            {"__gorilla_ref", f},
                            {"__index",       static_cast<int>(i)}
                        };
                    } else {
                        compressed_doc[f] = val;
                    }
                }
                // Attach Gorilla series on first version only
                if (i == 0) {
                    for (const auto& f : numeric_fields) {
                        compressed_doc["__gorilla_series_" + f] =
                            applyGorilla(f, series_map[f]);
                    }
                }
                compressed_doc["__compressed"] = "gorilla";

                const std::string comp_str = compressed_doc.dump();
                stats.compressed_size_bytes += comp_str.size();

                bool ok = table.replaceHistoricalPayload(
                    key, v.sys_time.start, compressed_doc);
                if (ok) {
                    ++stats.versions_compressed;
                } else {
                    stats.errors.emplace_back(
                        key, "replaceHistoricalPayload failed for start=" +
                                 std::to_string(v.sys_time.start));
                }
            }
            break;
        }

        // ── DICTIONARY ─────────────────────────────────────────────────────
        case CompressionAlgorithm::DICTIONARY: {
            for (const auto& v : sorted_versions) {
                const std::string orig = v.data.dump();
                stats.original_size_bytes += orig.size();

                nlohmann::json compressed =
                    applyDictionary(v.data, global_dicts);
                const std::string comp_str = compressed.dump();
                stats.compressed_size_bytes += comp_str.size();

                bool ok = table.replaceHistoricalPayload(
                    key, v.sys_time.start, compressed);
                if (ok) {
                    ++stats.versions_compressed;
                } else {
                    stats.errors.emplace_back(
                        key, "replaceHistoricalPayload failed for start=" +
                                 std::to_string(v.sys_time.start));
                }
            }
            break;
        }

        // ── LZ4 ────────────────────────────────────────────────────────────
        case CompressionAlgorithm::LZ4: {
            for (const auto& v : sorted_versions) {
                const std::string orig = v.data.dump();
                stats.original_size_bytes += orig.size();

                nlohmann::json compressed = applyLz4(v.data);
                const std::string comp_str = compressed.dump();
                stats.compressed_size_bytes += comp_str.size();

                bool ok = table.replaceHistoricalPayload(
                    key, v.sys_time.start, compressed);
                if (ok) {
                    ++stats.versions_compressed;
                } else {
                    stats.errors.emplace_back(
                        key, "replaceHistoricalPayload failed for start=" +
                                 std::to_string(v.sys_time.start));
                }
            }
            break;
        }

        } // switch
    } // for each key

    // Compute ratio
    if (stats.compressed_size_bytes > 0 && stats.original_size_bytes > 0) {
        stats.compression_ratio =
            static_cast<double>(stats.original_size_bytes) /
            static_cast<double>(stats.compressed_size_bytes);
    }

    const auto wall_end = std::chrono::steady_clock::now();
    stats.compression_time =
        std::chrono::duration_cast<std::chrono::milliseconds>(wall_end - wall_start);

    return stats;
}

} // namespace temporal
} // namespace themisdb


