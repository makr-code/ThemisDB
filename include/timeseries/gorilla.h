/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            gorilla.h                                          ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-02-21 11:48:30                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   98.0/100                                       ║
    • Total Lines:     125                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 5f378814d  2026-02-21  TimeSeries Module – Production Readiness Roadmap (All 7 P... ║
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
    BitReader br_;
    bool error_ {false};
    size_t decoded_count_ {0};
};

} // namespace themis

#endif // THEMIS_GORILLA_H
