/**
 * @file wisckey.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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
    
    /**
     * @brief TBD: Describe decode.
     * @param[in] encoded Input parameter.
     * @return Return value.
     * @details Calls: data().
     */
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
    /**
     * @brief TBD: Describe ValueLog.
     * @param[in] log_path Input parameter.
     * @return Return value.
     */
    explicit ValueLog(const std::string& log_path);
    ~ValueLog();

    /**
     * @brief Append value to log, returns address
     * @param[in] value Input parameter.
     * @return Return value.
     */
    ValueAddress append(const std::string& value);
    
    /**
     * @brief Read value from log by address
     * @param[in] addr Input parameter.
     * @return Return value.
     */
    std::optional<std::string> read(const ValueAddress& addr);
    
    // Get current log size in bytes
    uint64_t size() const {
        /**
         * @brief TBD: Describe lock.
         * @param[in] rw_mutex_ Input parameter.
         * @return Return value.
         */
        std::shared_lock<std::shared_mutex> lock(rw_mutex_);
        return current_offset_.load(std::memory_order_relaxed);
    }
    
    /**
     * @brief Sync log to disk
     */
    void sync();
    
    /**
     * @brief Garbage collection (optional, for future optimization) Compacts the log by copying only live values to a new log file.
     * @param[in,out] live_addresses Input/output parameter.
     * @details Updates the addresses vector in-place with new offsets.
     */
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
    
    /**
     * @brief Store key-value pair Returns encoded value (either inline or value address)
     * @param[in] key Input parameter.
     * @param[in] value Input parameter.
     * @return Return value.
     */
    std::string put(const std::string& key, const std::string& value);
    
    /**
     * @brief Retrieve value (handles both inline and separated values)
     * @param[in] key Input parameter.
     * @param[in] encoded_value Input parameter.
     * @return Return value.
     */
    std::optional<std::string> get(const std::string& key, const std::string& encoded_value);
    
    /**
     * @brief Check if value is separated
     * @param[in] encoded_value Input parameter.
     * @return True on success.
     * @details Calls: size().
     */
    static bool is_separated(const std::string& encoded_value) {
        return encoded_value.size() == ValueAddress::ENCODED_SIZE;
    }
    
    // Get statistics
    struct Stats {
        uint64_t inline_values = 0;
        uint64_t separated_values;
        uint64_t value_log_size;
    };
    /**
     * @brief TBD: Describe get_stats.
     * @return Return value.
     */
    Stats get_stats() const;

private:
    std::unique_ptr<ValueLog> value_log_;
    std::atomic<uint64_t> inline_values_{0};
    std::atomic<uint64_t> separated_values_{0};
};

} // namespace performance
} // namespace themis
