/**
 * @file index_compression.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <functional>

namespace themis {
namespace index {

/// Byte-level compression algorithm applied on top of structural encodings.
enum class CompressionAlgorithm {
    NONE,    ///< No byte-level compression (structural encodings only)
    LZ4,     ///< LZ4 fast compression
    ZSTD,    ///< Zstandard — best ratio/speed balance
    SNAPPY,  ///< Snappy — fastest decompression
};

// ---------------------------------------------------------------------------
// BloomFilter
// ---------------------------------------------------------------------------

/// A simple in-memory Bloom filter to reduce false lookups on index data.
///
/// Uses k independent hash functions derived from two base hashes (double
/// hashing) to achieve a configurable false-positive rate.
class BloomFilter {
public:
    /// @param expected_elements  Estimated number of distinct elements.
    /// @param false_positive_rate  Target false-positive probability (0 < p < 1).
    explicit BloomFilter(size_t expected_elements,
                         double  false_positive_rate = 0.01);

    /// Insert a key into the filter.
    void insert(std::string_view key);

    /// Return true when the key *might* be present; false means definitely absent.
    bool mightContain(std::string_view key) const;

    /// Remove all entries (reset to empty).
    void clear();

    /// Approximate number of bits used.
    size_t bitCount() const { return bits_.size(); }

    /// Number of hash functions k.
    size_t hashCount() const { return k_; }

private:
    size_t              m_{0};   ///< Bit-array length
    size_t              k_{0};   ///< Number of hash functions
    std::vector<bool>   bits_;

    /// Two independent 64-bit hashes for double-hashing.
    static std::pair<uint64_t, uint64_t> hash2_(std::string_view key);
};

// ---------------------------------------------------------------------------
// DictionaryCodec
// ---------------------------------------------------------------------------

/// Dictionary encoding: maps frequent strings to compact integer codes.
///
/// The codec is built from a training corpus (a list of strings).  Strings
/// that appear at least min_frequency times are assigned codes starting at 0.
/// All other strings receive the sentinel code kMissCode.
class DictionaryCodec {
public:
    static constexpr uint32_t kMissCode = UINT32_MAX;

    struct Config {
        size_t max_dict_size    = 65536; ///< Maximum number of dictionary entries
        size_t min_frequency    = 2;     ///< Minimum occurrences to enter dictionary
    };

    explicit DictionaryCodec();
    explicit DictionaryCodec(const Config& cfg);

    /// Build (or rebuild) the dictionary from a corpus of strings.
    void train(const std::vector<std::string>& corpus);

    /// Encode a string to its dictionary code, or kMissCode when not present.
    uint32_t encode(std::string_view value) const;

    /// Decode a code back to the original string.
    /// Returns empty string for kMissCode or unknown codes.
    std::string decode(uint32_t code) const;

    /// True if the dictionary contains at least one entry.
    bool empty() const { return id_to_string_.empty(); }

    /// Number of entries in the dictionary.
    size_t size() const { return id_to_string_.size(); }

    /// Access all dictionary entries (for serialisation / inspection).
    const std::vector<std::string>& entries() const { return id_to_string_; }

private:
    Config                              cfg_;
    std::unordered_map<std::string, uint32_t> string_to_id_;
    std::vector<std::string>            id_to_string_;
};

// ---------------------------------------------------------------------------
// PrefixCompressor
// ---------------------------------------------------------------------------

/// Prefix compression: given a sorted list of keys that share a common prefix,
/// store the prefix once and keep only the unique suffixes.
///
/// Example:
///   Input:  ["idx:users:country:USA:pk1",
///             "idx:users:country:USA:pk2",
///             "idx:users:country:USA:pk3"]
///   Result: prefix = "idx:users:country:USA:", suffixes = ["pk1","pk2","pk3"]
struct PrefixBlock {
    std::string              prefix;
    std::vector<std::string> suffixes;

    /// Reconstruct the original keys.
    std::vector<std::string> decompress() const;

    /// Estimated byte saving vs. storing full keys.
    size_t savedBytes() const;
};

/** @brief Prefix compressor component. */
class PrefixCompressor {
public:
    /// Compress a sorted range of keys.  Adjacent keys that share a common
    /// prefix of at least min_prefix_len bytes are grouped into the same block.
    /// @param min_prefix_len  Minimum shared prefix length to trigger grouping.
    static std::vector<PrefixBlock> compress(
        const std::vector<std::string>& sorted_keys,
        size_t min_prefix_len = 4);

    /// Reconstruct all keys from a list of compressed blocks.
    static std::vector<std::string> decompress(
        const std::vector<PrefixBlock>& blocks);
};

// ---------------------------------------------------------------------------
// DeltaEncoder
// ---------------------------------------------------------------------------

/// Delta encoding for sorted integer-like key suffixes.
///
/// Instead of storing each value verbatim, only the difference to the
/// previous value is stored.  This is most effective when keys are densely
/// sequential (e.g., auto-increment primary keys).
///
/// @note  The empty-sequence sentinel is `{base=0, deltas=[]}`.  A single-
///        element sequence `{0}` produces the same block representation and
///        therefore cannot be round-tripped.  Callers should use positive
///        integer primary keys (the standard database convention) to avoid
///        this ambiguity.
struct DeltaBlock {
    int64_t              base{0};   ///< First value in the sequence
    std::vector<int64_t> deltas;    ///< Differences between consecutive values

    /// Reconstruct the original sequence of integers.
    std::vector<int64_t> decompress() const;
};

/** @brief Delta encoder component. */
class DeltaEncoder {
public:
    /// Encode a sorted list of integers.
    static DeltaBlock encode(const std::vector<int64_t>& sorted_values);

    /// Decode a DeltaBlock back to the original sorted sequence.
    static std::vector<int64_t> decode(const DeltaBlock& block);
};

// ---------------------------------------------------------------------------
// RunLengthEncoder
// ---------------------------------------------------------------------------

/// Run-Length Encoding (RLE) for repeated string values.
///
/// Consecutive equal values are stored as (value, count) pairs.
struct RunLengthBlock {
    struct Run {
        std::string value;
        uint32_t    count{1};
    };
    std::vector<Run> runs;

    /// Reconstruct the original value sequence.
    std::vector<std::string> decompress() const;
};

/** @brief Run length encoder component. */
class RunLengthEncoder {
public:
    /// Encode a sequence of strings with repeated values.
    static RunLengthBlock encode(const std::vector<std::string>& values);

    /// Decode a RunLengthBlock back to the original sequence.
    static std::vector<std::string> decode(const RunLengthBlock& block);

    /// Return the compression ratio: decoded_size / encoded_size.
    /// Values > 1 indicate savings; 1 = no savings.
    static double compressionRatio(const std::vector<std::string>& values);
};

// ---------------------------------------------------------------------------
// IndexCompressionCodec
// ---------------------------------------------------------------------------

/// High-level codec that combines all compression techniques.
///
/// Typical usage:
/// @code
///   IndexCompressionCodec::Config cfg;
///   cfg.enable_prefix_compression  = true;
///   cfg.enable_delta_encoding      = true;
///   cfg.enable_rle                 = true;
///   cfg.enable_dict_encoding       = true;
///   cfg.enable_bloom_filter        = true;
///   cfg.algorithm                  = CompressionAlgorithm::ZSTD;
///   cfg.compression_level          = 3;
///
///   IndexCompressionCodec codec(cfg);
///   codec.trainDictionary(sample_values);
///
///   auto blocks = codec.compressKeys(sorted_index_keys);
///   auto keys   = codec.decompressKeys(blocks);
/// @endcode
class IndexCompressionCodec {
public:
    struct Config {
        bool enable_prefix_compression = false; ///< Prefix-compression on key batches
        bool enable_delta_encoding     = false; ///< Delta-encoding for integer suffixes
        bool enable_rle                = false; ///< RLE for repeated values
        bool enable_dict_encoding      = false; ///< Dictionary encoding for field values
        bool enable_bloom_filter       = false; ///< Bloom filter to skip absent lookups

        CompressionAlgorithm algorithm       = CompressionAlgorithm::NONE;
        int                  compression_level = 3; ///< Algorithm-specific level

        size_t bloom_expected_elements    = 100'000;
        double bloom_false_positive_rate  = 0.01;

        DictionaryCodec::Config dict_config;
    };

    /// Compression statistics for monitoring.
    struct Stats {
        uint64_t keys_compressed{0};     ///< Total keys processed by compressKeys
        uint64_t keys_decompressed{0};   ///< Total keys restored by decompressKeys
        uint64_t bloom_inserts{0};       ///< Total bloom-filter inserts
        uint64_t bloom_lookups{0};       ///< Total bloom mightContain queries
        uint64_t bloom_hits{0};          ///< Lookups that returned true (might be FP)
        uint64_t bloom_rejections{0};    ///< Lookups that returned false (definite miss)
        uint64_t dict_encodes{0};        ///< Dictionary encode calls
        uint64_t dict_hits{0};           ///< Successful dictionary encodes
        uint64_t rle_runs_saved{0};      ///< Runs saved by RLE (total count - runs)
        uint64_t prefix_bytes_saved{0};  ///< Bytes saved by prefix compression
    };

    explicit IndexCompressionCodec();
    explicit IndexCompressionCodec(const Config& cfg);

    // -- Configuration -------------------------------------------------------

    const Config& config() const { return cfg_; }

    // -- Dictionary ----------------------------------------------------------

    /// Train the dictionary codec from a sample of index field values.
    void trainDictionary(const std::vector<std::string>& sample_values);

    bool dictionaryReady() const { return !dict_codec_.empty(); }

    /// Encode a value through the dictionary (returns kMissCode on miss).
    uint32_t encodeValue(std::string_view value) const;

    /// Decode a dictionary code back to its string.
    std::string decodeValue(uint32_t code) const;

    // -- Bloom filter --------------------------------------------------------

    /// Register a key in the Bloom filter.
    void bloomInsert(std::string_view key);

    /// Check whether a key might exist (false == definitely absent).
    bool bloomMightContain(std::string_view key) const;

    /// Reset the Bloom filter (e.g. after a full index rebuild).
    void resetBloom();

    // -- Key compression -----------------------------------------------------

    /// Compress a sorted list of index keys into prefix-compressed blocks.
    /// Also updates Bloom filter and stats.
    std::vector<PrefixBlock> compressKeys(
        const std::vector<std::string>& sorted_keys);

    /// Reconstruct the original key list from compressed blocks.
    std::vector<std::string> decompressKeys(
        const std::vector<PrefixBlock>& blocks);

    // -- Value RLE -----------------------------------------------------------

    /// Compress a list of index values using RLE.
    RunLengthBlock compressValues(const std::vector<std::string>& values);

    /// Decompress an RLE block.
    std::vector<std::string> decompressValues(const RunLengthBlock& block);

    // -- Delta encoding ------------------------------------------------------

    /// Encode sorted integer primary-key suffixes.
    DeltaBlock encodePKs(const std::vector<int64_t>& sorted_pks);

    /// Decode a DeltaBlock back to PK integers.
    std::vector<int64_t> decodePKs(const DeltaBlock& block);

    // -- Statistics ----------------------------------------------------------

    const Stats& stats() const { return stats_; }
    void         resetStats()  { stats_ = {}; }

private:
    Config            cfg_;
    BloomFilter       bloom_;
    DictionaryCodec   dict_codec_;
    mutable Stats     stats_;
};

} // namespace index
} // namespace themis
