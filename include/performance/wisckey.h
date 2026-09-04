/**
 * @file wisckey.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// WiscKey: Separation of Keys and Values for LSM Trees
// Paper: "WiscKey: Separating Keys from Values in SSD-conscious Storage" (FAST'16)
// Authors: Lanyue Lu et al., University of Wisconsin-Madison
//
// Key idea: Store large values (>1KB) in separate value log to reduce write amplification
// Expected gain: +40-60% write throughput for large values
// Reference: https://www.usenix.org/system/files/conference/fast16/fast16-papers-lu.pdf

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <mutex>
#include <shared_mutex>
#include <memory>
#include <fstream>
#include <optional>
#include <atomic>

namespace themis {
namespace performance {

/// Value address in the value log
struct ValueAddress {
    static constexpr size_t ENCODED_SIZE = 12;  // 8 bytes offset + 4 bytes size
    
    uint64_t offset;      // Offset in value log file
    uint32_t size;        // Size of value in bytes
    
    // Encode as 12-byte blob for storage in LSM tree
    // Note: Uses little-endian byte order
    std::string encode() const {
        std::string result(ENCODED_SIZE, '\0');
        *reinterpret_cast<uint64_t*>(result.data()) = offset;
        *reinterpret_cast<uint32_t*>(result.data() + 8) = size;
        return result;
    }
    
    static ValueAddress decode(const std::string& encoded) {
        ValueAddress addr;
        addr.offset = *reinterpret_cast<const uint64_t*>(encoded.data());
        addr.size = *reinterpret_cast<const uint32_t*>(encoded.data() + 8);
        return addr;
    }
};

/// Append-only value log for storing large values
class ValueLog {
public:
    explicit ValueLog(const std::string& log_path);
    ~ValueLog();

    // Append value to log, returns address
    ValueAddress append(const std::string& value);
    
    // Read value from log by address
    std::optional<std::string> read(const ValueAddress& addr);
    
    // Get current log size in bytes
    uint64_t size() const {
        std::shared_lock<std::shared_mutex> lock(rw_mutex_);
        return current_offset_.load(std::memory_order_relaxed);
    }
    
    // Sync log to disk
    void sync();
    
    // Garbage collection (optional, for future optimization)
    // Compacts the log by copying only live values to a new log file.
    // Updates the addresses vector in-place with new offsets.
    void compact(std::vector<ValueAddress>& live_addresses);

private:
    std::string log_path_;
    std::unique_ptr<std::fstream> log_file_;
    std::atomic<uint64_t> current_offset_;
    mutable std::shared_mutex rw_mutex_;  // Reader-writer lock for concurrent reads
};

/// WiscKey storage engine wrapper
/// Decides whether to store value inline or in value log
class WiscKeyStorage {
public:
    // Threshold for value separation (1KB as per paper)
    static constexpr size_t VALUE_SEPARATION_THRESHOLD = 1024;
    
    WiscKeyStorage(const std::string& value_log_path);
    
    // Store key-value pair
    // Returns encoded value (either inline or value address)
    std::string put(const std::string& key, const std::string& value);
    
    // Retrieve value (handles both inline and separated values)
    std::optional<std::string> get(const std::string& key, const std::string& encoded_value);
    
    // Check if value is separated
    static bool is_separated(const std::string& encoded_value) {
        return encoded_value.size() == ValueAddress::ENCODED_SIZE;
    }
    
    // Get statistics
    struct Stats {
        uint64_t inline_values = 0;
        uint64_t separated_values;
        uint64_t value_log_size;
    };
    Stats get_stats() const;

private:
    std::unique_ptr<ValueLog> value_log_;
    std::atomic<uint64_t> inline_values_{0};
    std::atomic<uint64_t> separated_values_{0};
};

} // namespace performance
} // namespace themis
