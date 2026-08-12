/**
 * @file gorilla.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "timeseries/gorilla.h"
#include <cstring>
#include <limits>
#if defined(_MSC_VER)
#include <intrin.h>
#endif

namespace themis {

static inline uint64_t dbl_to_bits(double v) {
    uint64_t b;
    std::memcpy(&b, &v, sizeof(b));
    return b;
}

static inline double bits_to_dbl(uint64_t b) {
    double v;
    std::memcpy(&v, &b, sizeof(v));
    return v;
}

static inline int clz64(uint64_t x) {
    if (x == 0) return 64;
#if defined(_MSC_VER)
    unsigned long idx;
    _BitScanReverse64(&idx, x);
    return 63 - static_cast<int>(idx);
#else
    return __builtin_clzll(x);
#endif
}

static inline int ctz64(uint64_t x) {
    if (x == 0) return 64;
#if defined(_MSC_VER)
    unsigned long idx;
    _BitScanForward64(&idx, x);
    return static_cast<int>(idx);
#else
    return __builtin_ctzll(x);
#endif
}

// ------- BitWriter -------
void BitWriter::writeBit(bool bit) {
    cur_ |= (static_cast<uint8_t>(bit) & 1) << bitpos_;
    bitpos_++;
    if (bitpos_ == 8) {
        buf_.push_back(cur_);
        cur_ = 0;
        bitpos_ = 0;
    }
}

void BitWriter::writeBits(uint64_t value, int bits) {
    for (int i = 0; i < bits; ++i) {
        writeBit((value >> i) & 1ULL);
    }
}

void BitWriter::writeVarUInt(uint64_t v) {
    // LEB128 unsigned
    while (v >= 0x80) {
        buf_.push_back(static_cast<uint8_t>(v & 0x7FUL) | 0x80U);
        v >>= 7;
    }
    buf_.push_back(static_cast<uint8_t>(v & 0x7FUL));
}

void BitWriter::writeZigZag64(int64_t value) {
    uint64_t zz = (static_cast<uint64_t>(value) << 1) ^ static_cast<uint64_t>(value >> 63);
    writeVarUInt(zz);
}

void BitWriter::alignToByte() {
    if (bitpos_ != 0) {
        buf_.push_back(cur_);
        cur_ = 0;
        bitpos_ = 0;
    }
}

std::vector<uint8_t> BitWriter::finish() {
    if (bitpos_ != 0) {
        buf_.push_back(cur_);
        cur_ = 0;
        bitpos_ = 0;
    }
    return buf_;
}

// ------- GorillaEncoder -------
void GorillaEncoder::add(int64_t timestamp_ms, double value) {
    if (first_) {
        // Write first timestamp and value in full
        bw_.writeZigZag64(timestamp_ms); // allow negative for completeness
        bw_.writeBits(dbl_to_bits(value), 64);
        prev_ts_ = timestamp_ms;
        prev_dt_ = 0;
        prev_vbits_ = dbl_to_bits(value);
        prev_leading_ = 64;
        prev_trailing_ = 64;
        first_ = false;
        return;
    }

    // Ensure we start new point with varint aligned to byte boundary
    bw_.alignToByte();
    // Timestamp: delta-of-delta with ZigZag+varint (byte-aligned)
    int64_t dt = timestamp_ms - prev_ts_;
    int64_t dod = dt - prev_dt_;
    bw_.writeZigZag64(dod);
    prev_ts_ = timestamp_ms;
    prev_dt_ = dt;

    // Value: XOR encoding
    uint64_t vbits = dbl_to_bits(value);
    uint64_t xorv = vbits ^ prev_vbits_;
    
    if (xorv == 0) {
        // Write control bit 0
        bw_.writeBit(false);
        // Keep previous value the same explicitly
        prev_vbits_ = vbits;
    } else {
        bw_.writeBit(true); // different
        // New header each time in this simplified variant: use actual leading/trailing zeros
        int leading = clz64(xorv);
        int trailing = ctz64(xorv);
        // Handle edge case: if all 64 bits are significant, we need special encoding
        // Since we can only encode 0-63 in 6 bits, we use sig=0 to mean 64
        int significant = 64 - leading - trailing;
        if (significant > 64) significant = 64;  // safety clamp
        
        // Write leading (6 bits), significant length (6 bits: 0 means 64, 1-63 literal), then value bits
        bw_.writeBits(static_cast<uint64_t>(leading), 6);
        bw_.writeBits(static_cast<uint64_t>(significant & 63), 6);  // 64 becomes 0
        bw_.writeBits((xorv >> trailing), significant);
        prev_leading_ = leading;
        prev_trailing_ = trailing;
        prev_vbits_ = vbits;
    }
    // After finishing value bits for this point, keep bitstream as-is; next call will align before varint
}

std::vector<uint8_t> GorillaEncoder::finish() {
    auto payload = bw_.finish();
    std::vector<uint8_t> result;
    result.reserve(3 + payload.size());
    result.push_back(kGorillaMagic0);
    result.push_back(kGorillaMagic1);
    result.push_back(kGorillaCurrentVersion);
    result.insert(result.end(), payload.begin(), payload.end());
    return result;
}

// ------- GorillaDecoder -------
GorillaDecoder::GorillaDecoder(const std::vector<uint8_t>& data)
    : data_(gorilla_strip_header(data, error_))
    , br_(data_) {}

// Strip the 3-byte chunk header if present and return the payload slice.
// Sets error=true when magic bytes are present but the version is unsupported.
// Legacy chunks (no header) are returned unchanged.
/* static */ std::vector<uint8_t> GorillaDecoder::gorilla_strip_header(
        const std::vector<uint8_t>& data, bool& error_out) {
    if (data.size() >= 3 &&
            data[0] == kGorillaMagic0 &&
            data[1] == kGorillaMagic1) {
        if (data[2] != kGorillaCurrentVersion) {
            error_out = true;
            return {};
        }
        return std::vector<uint8_t>(data.begin() + 3, data.end());
    }
    // Legacy format: no header — return as-is
    return data;
}

std::optional<std::pair<int64_t,double>> GorillaDecoder::next() {
    if (error_) return std::nullopt;

    if (first_) {
        if (br_.eof()) return std::nullopt;
        // First timestamp varint is at byte boundary
        br_.alignToByte();
        if (br_.eof()) return std::nullopt;
        int64_t ts = br_.readZigZag64();
        if (br_.eof()) { error_ = true; return std::nullopt; }
        uint64_t vbits = br_.readBits(64);
        prev_ts_ = ts;
        prev_dt_ = 0;
        prev_vbits_ = vbits;
        prev_leading_ = 64;
        prev_trailing_ = 64;
        first_ = false;
        decoded_count_++;
        return std::make_pair(ts, bits_to_dbl(vbits));
    }

    // Subsequent varints are byte-aligned; align and check EOF before reading
    br_.alignToByte();
    if (br_.eof()) return std::nullopt;
    
    int64_t dod = br_.readZigZag64();
    int64_t dt = prev_dt_ + dod;
    int64_t ts = prev_ts_ + dt;
    prev_dt_ = dt;
    prev_ts_ = ts;

    if (br_.eof()) { error_ = true; return std::nullopt; }
    bool different = br_.readBit();
    
    uint64_t vbits;
    if (!different) {
        vbits = prev_vbits_;
    } else {
        if (br_.eof()) { error_ = true; return std::nullopt; }
        int leading = static_cast<int>(br_.readBits(6));
        if (br_.eof()) { error_ = true; return std::nullopt; }
        int significant = static_cast<int>(br_.readBits(6));
        if (significant == 0) significant = 64;  // 0 encodes 64
        // Validate leading + significant fits in 64 bits
        if (leading + significant > 64) { error_ = true; return std::nullopt; }
        if (br_.eof() && significant > 0) { error_ = true; return std::nullopt; }
        uint64_t payload = br_.readBits(significant);
        int trailing = 64 - leading - significant;
        uint64_t xorv = (payload << trailing);
        vbits = prev_vbits_ ^ xorv;
        prev_leading_ = leading;
        prev_trailing_ = trailing;
    }
    
    prev_vbits_ = vbits;
    decoded_count_++;
    return std::make_optional(std::make_pair(ts, bits_to_dbl(vbits)));
}

} // namespace themis

