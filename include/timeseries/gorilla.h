/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            gorilla.h                                          ║
  Version:         0.0.35                                             ║
  Last Modified:   2026-03-16 04:11:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     121                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#ifndef THEMIS_GORILLA_H
#define THEMIS_GORILLA_H

#include <cstdint>
#include <vector>
#include <utility>
#include <optional>

namespace themis {

/**
 * Gorilla-style time-series compression for (timestamp_ms, double)
 * 
 * Implements:
 *  - Timestamps: delta-of-delta encoding with ZigZag + varint
 *  - Values: XOR of IEEE-754 double bit patterns with leading/trailing zero optimization
 * 
 * @sources
 * - Algorithm: Gorilla Time Series Compression
 * - Paper: Pelkonen, T., Franklin, S., et al. (2015)
 *          "Gorilla: A Fast, Scalable, In-Memory Time Series Database"
 *          Proceedings of the VLDB Endowment, Vol. 8, No. 12
 * - Company: Facebook (Meta)
 * - URL: http://www.vldb.org/pvldb/vol8/p1816-teller.pdf
 * - Implementation: Custom implementation for ThemisDB based on algorithm description
 */

// ── Gorilla chunk header constants ──────────────────────────────────────────
// New chunks (v1+) are prefixed with a 3-byte header to allow decoders to
// validate the data before attempting to parse the bit-stream.  Legacy chunks
// (written before v1) have no header; both GorillaDecoder and GorillaSIMDDecoder
// detect the header by checking the magic bytes and fall back to the headerless
// decode path when the magic is absent.
static constexpr uint8_t kGorillaMagic0       = 0x47;  // 'G'
static constexpr uint8_t kGorillaMagic1       = 0x4F;  // 'O'
static constexpr uint8_t kGorillaCurrentVersion = 0x01; // format version 1

class BitWriter {
public:
    void writeBit(bool bit);
    void writeBits(uint64_t value, int bits);
    void writeVarUInt(uint64_t value);
    void writeZigZag64(int64_t value);
    void alignToByte();
    std::vector<uint8_t> finish();

private:
    std::vector<uint8_t> buf_;
    uint8_t cur_ {0};
    int bitpos_ {0}; // 0..7
};

class BitReader {
public:
    explicit BitReader(const std::vector<uint8_t>& data);
    bool readBit();
    uint64_t readBits(int bits);
    uint64_t readVarUInt();
    int64_t readZigZag64();
    bool eof() const;
    void alignToByte();

private:
    const std::vector<uint8_t>& buf_;
    size_t idx_ {0};
    int bitpos_ {8};
    uint8_t cur_ {0};
};

class GorillaEncoder {
public:
    void add(int64_t timestamp_ms, double value);
    std::vector<uint8_t> finish();

private:
    bool first_ {true};
    int64_t prev_ts_ {0};
    int64_t prev_dt_ {0};
    uint64_t prev_vbits_ {0};
    int prev_leading_ {64};
    int prev_trailing_ {64};
    BitWriter bw_;
};

class GorillaDecoder {
public:
    explicit GorillaDecoder(const std::vector<uint8_t>& data);
    std::optional<std::pair<int64_t,double>> next();

    /// Returns true if a decode error was encountered (truncated/corrupt data)
    bool hasError() const { return error_; }
    /// Returns total number of successfully decoded points
    size_t decodedCount() const { return decoded_count_; }

private:
    bool first_ {true};
    int64_t prev_ts_ {0};
    int64_t prev_dt_ {0};
    uint64_t prev_vbits_ {0};
    int prev_leading_ {64};
    int prev_trailing_ {64};
    // error_ and decoded_count_ are declared BEFORE data_ and br_ so that
    // gorilla_strip_header() (called during data_ initialisation) can safely
    // write to error_ via the reference it receives.
    bool error_ {false};
    size_t decoded_count_ {0};
    std::vector<uint8_t> data_; // owned, header-stripped payload (declared after error_)
    BitReader br_;

    // Strips the 3-byte chunk header if present; sets error_out=true on
    // unsupported version.  Returns the payload (header-stripped or original).
    static std::vector<uint8_t> gorilla_strip_header(
            const std::vector<uint8_t>& data, bool& error_out);
};

} // namespace themis

#endif // THEMIS_GORILLA_H
