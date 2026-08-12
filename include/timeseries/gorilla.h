/**
 * @file gorilla.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

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
 * Sources:
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

/** @brief Bit writer component. */
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

// ── BitReader ─────────────────────────────────────────────────────────────
// All hot-path methods are defined inline here so that callers in different
// translation units (gorilla.cpp, gorilla_simd.cpp) can benefit from
// full inlining and per-call-site optimization.
/** @brief full inlining and per-call-site optimization. */
class BitReader {
public:
    // Primary constructor: raw pointer + size (all hot-path state is set here).
    BitReader(const uint8_t* data, size_t size) noexcept
        : ptr_(data), size_(size) {
        if (size_ > 0) cur_ = ptr_[0];
    }

    // Convenience constructor: delegates to the pointer+size form.
    explicit BitReader(const std::vector<uint8_t>& data) noexcept
        : BitReader(data.data(), data.size()) {}

    inline bool readBit() noexcept {
        if (idx_ >= size_) return false;
        bool bit = ((cur_ >> bitpos_) & 1U) != 0;
        if (++bitpos_ == 8) {
            ++idx_;
            bitpos_ = 0;
            if (idx_ < size_) cur_ = ptr_[idx_];
        }
        return bit;
    }

    inline uint64_t readBits(int bits) noexcept {
        if (bits <= 0) return 0;

        uint64_t v = 0;
        int out_bit = 0;

        // Drain remaining bits from the current partial byte first.
        if (bitpos_ != 0) {
            int avail = 8 - bitpos_;
            int take  = (bits < avail) ? bits : avail;
            uint8_t mask = static_cast<uint8_t>((1u << take) - 1u);
            v = static_cast<uint64_t>((cur_ >> bitpos_) & mask);
            out_bit  = take;
            bits    -= take;
            bitpos_ += take;
            if (bitpos_ == 8) {
                ++idx_;
                bitpos_ = 0;
                if (idx_ < size_) cur_ = ptr_[idx_];
            }
            if (bits == 0) return v;
        }

        // Byte-aligned: read full bytes in a tight loop.
        while (bits >= 8 && idx_ < size_) {
            v |= static_cast<uint64_t>(ptr_[idx_]) << out_bit;
            out_bit += 8;
            bits    -= 8;
            ++idx_;
        }
        if (idx_ < size_) cur_ = ptr_[idx_];

        // Read any remaining sub-byte bits from the current byte.
        if (bits > 0 && idx_ < size_) {
            uint8_t mask = static_cast<uint8_t>((1u << bits) - 1u);
            v |= static_cast<uint64_t>(cur_ & mask) << out_bit;
            bitpos_ = bits;
        }

        return v;
    }

    inline uint64_t readVarUInt() noexcept {
        // LEB128 unsigned; caller ensures byte alignment when required.
        uint64_t result = 0;
        int shift = 0;
        while (true) {
            if (idx_ >= size_) return result;
            uint8_t byte;
            if (bitpos_ == 0) {
                // Fast path: byte-aligned — read directly without going through readBit().
                byte = ptr_[idx_++];
                if (idx_ < size_) cur_ = ptr_[idx_];
            } else {
                byte = static_cast<uint8_t>(readBits(8));
            }
            result |= static_cast<uint64_t>(byte & 0x7F) << shift;
            if ((byte & 0x80) == 0) break;
            shift += 7;
        }
        return result;
    }

    inline int64_t readZigZag64() noexcept {
        uint64_t zz = readVarUInt();
        int64_t v = static_cast<int64_t>(zz >> 1);
        if (zz & 1ULL) v = ~v;
        return v;
    }

    inline bool eof() const noexcept { return idx_ >= size_; }

    inline void alignToByte() noexcept {
        if (bitpos_ != 0) {
            // Skip remaining bits in the current byte (encoder-written padding zeros).
            // Jumping directly to the next byte boundary is O(1) vs. O(bitpos_) with readBit().
            ++idx_;
            bitpos_ = 0;
            if (idx_ < size_) cur_ = ptr_[idx_];
        }
    }

private:
    const uint8_t* ptr_  {nullptr};
    size_t         size_ {0};
    size_t         idx_  {0};
    int            bitpos_ {0};
    uint8_t        cur_  {0};
};

/** @brief Gorilla encoder component. */
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

/** @brief Gorilla decoder component. */
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
