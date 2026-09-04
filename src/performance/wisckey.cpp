/**
 * @file wisckey.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 81/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=6, H=1, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "performance/wisckey.h"
#include "performance/phase2_feature_flags.h"
#include <stdexcept>
#include <cstring>
#include <cstdio>
#include <limits>

namespace themis {
namespace performance {

// ============================================================================
// Hardware Capability Detection & Validation
// ============================================================================

bool is_wisckey_hardware_supported() {
    return Phase2FeatureFlags::instance().wisckey_hardware_supported();
}

// ============================================================================
// ValueLog Implementation
// ============================================================================

ValueLog::ValueLog(const std::string& log_path) 
    : log_path_(log_path), current_offset_(0) {
    
    // Fail-closed: if hardware doesn't support SSD operations, reject
    if (!is_wisckey_hardware_supported()) {
        throw std::runtime_error(
            "WiscKey: Hardware does not support SSD/NVMe operations required for value log. "
            "Use in-memory storage or inline values instead."
        );
    }
    
    // Open existing file or create new one
    std::ifstream test(log_path_, std::ios::binary);
    bool file_exists = test.good();
    test.close();
    
    if (file_exists) {
        // Open existing file in read/write mode (not append)
        log_file_ = std::make_unique<std::fstream>(
            log_path_, std::ios::in | std::ios::out | std::ios::binary
        );
        
        // Get current file size
        log_file_->seekg(0, std::ios::end);
        current_offset_.store(
            static_cast<uint64_t>(log_file_->tellg()),
            std::memory_order_relaxed
        );
    } else {
        // Create new file
        log_file_ = std::make_unique<std::fstream>(
            log_path_, std::ios::out | std::ios::binary
        );
        log_file_->close();
        
        // Reopen in read/write mode
        log_file_ = std::make_unique<std::fstream>(
            log_path_, std::ios::in | std::ios::out | std::ios::binary
        );
        current_offset_.store(0, std::memory_order_relaxed);
    }
}

ValueLog::~ValueLog() {
    if (log_file_ && log_file_->is_open()) {
        log_file_->close();
    }
}

ValueAddress ValueLog::append(const std::string& value) {
    // Validate input: non-empty value required
    if (value.empty()) {
        throw std::runtime_error("WiscKey: Cannot append empty value to log");
    }
    
    // Bound check: prevent values that cannot be represented in uint32 ValueAddress::size.
    // The maximum safe value is UINT32_MAX (4GiB - 1); exactly 4GiB would truncate to 0.
    constexpr uint64_t MAX_SINGLE_VALUE = static_cast<uint64_t>(std::numeric_limits<uint32_t>::max());
    if (static_cast<int>(value.size()) > MAX_SINGLE_VALUE) {
        throw std::runtime_error("WiscKey: Value size exceeds maximum (UINT32_MAX bytes)");
    }
    
    std::unique_lock<std::shared_mutex> lock(rw_mutex_);  // Exclusive lock for writes
    
    ValueAddress addr;
    addr.offset = current_offset_.load(std::memory_order_relaxed);
    addr.size = static_cast<uint32_t>(value.size());
    
    // std::fstream does not expose per-operation timeouts. Value-log I/O is
    // bounded by the host OS/storage stack; production deployments use local
    // SSD-backed logs and monitor latency externally.
    log_file_->seekp(0, std::ios::end);
    log_file_->write(value.data(),static_cast<int>(value.size()));
    log_file_->flush();
    
    current_offset_.store(
        addr.offset + static_cast<uint64_t>(value.size()),
        std::memory_order_relaxed
    );
    return addr;
}

std::optional<std::string> ValueLog::read(const ValueAddress& addr) {
    // Validate address bounds
    if (addr.size == 0 || addr.size > (1 << 32)) {
        return std::nullopt;
    }
    if (addr.offset + addr.size > current_offset_.load(std::memory_order_relaxed)) {
        return std::nullopt;  // Address out of bounds
    }
    
    // Exclusive lock required: std::fstream is not thread-safe for concurrent seekg/read
    // Concurrent calls to seekg() would corrupt the file position state
    std::unique_lock<std::shared_mutex> lock(rw_mutex_);
    
    log_file_->seekg(addr.offset);
    std::string value(addr.size, '\0');
    // std::fstream does not expose per-operation timeouts. Reads target a
    // local append-only log, so blocking is bounded by OS-managed disk I/O.
    log_file_->read(&value[0], addr.size);
    
    if (log_file_->gcount() != addr.size) {
        return std::nullopt;
    }
    
    return value;
}

void ValueLog::sync() {
    std::unique_lock<std::shared_mutex> lock(rw_mutex_);  // Exclusive lock for sync
    log_file_->flush();
}

void ValueLog::compact(std::vector<ValueAddress>& live_addresses) {
    std::unique_lock<std::shared_mutex> lock(rw_mutex_);  // Exclusive lock for compaction
    
    if (live_addresses.empty()) {
        return;
    }
    
    // Validate all addresses before starting compaction
    for (const auto& addr : live_addresses) {
        if (addr.size == 0 || addr.size > (1 << 32)) {
            throw std::runtime_error("WiscKey: Invalid address during compaction");
        }
        if (addr.offset + addr.size > current_offset_.load(std::memory_order_relaxed)) {
            throw std::runtime_error("WiscKey: Address out of bounds during compaction");
        }
    }
    
    // std::fstream does not expose per-operation timeouts. Compaction runs
    // against local disk files and is expected to complete within bounded SSD
    // latency envelopes in production deployments.
    std::string temp_log_path = log_path_ + ".tmp";
    std::fstream temp_log(temp_log_path, std::ios::out | std::ios::binary);
    
    if (!temp_log.is_open()) {
        throw std::runtime_error("Failed to create temporary log file for compaction");
    }
    
    uint64_t new_offset = 0;
    
    // Copy live values to new log and update addresses in-place
    for (auto& addr : live_addresses) {
        // Read value from old log. std::fstream offers no per-read timeout;
        // compaction is limited to local-disk latency and runs under operator
        // control rather than in the request path.
        log_file_->seekg(addr.offset);
        std::string value(addr.size, '\0');
        log_file_->read(&value[0], addr.size);
        
        if (log_file_->gcount() != static_cast<std::streamsize>(addr.size)) {
            temp_log.close();
            std::remove(temp_log_path.c_str());
            throw std::runtime_error("Failed to read value during compaction");
        }
        
        // Write value to new log
        temp_log.write(value.data(),static_cast<int>(value.size()));
        if (!temp_log.good()) {
            temp_log.close();
            std::remove(temp_log_path.c_str());
            throw std::runtime_error("Failed to write value during compaction");
        }
        
        // Update address in-place
        addr.offset = new_offset;
        new_offset += addr.size;
    }
    
    // Flush and close temporary log
    temp_log.flush();
    temp_log.close();
    
    // Close old log
    log_file_->close();
    
    // Replace old log with new log atomically
    if (std::rename(temp_log_path.c_str(), log_path_.c_str()) != 0) {
        throw std::runtime_error("Failed to replace old log with compacted log");
    }
    
    // Reopen log file
    log_file_ = std::make_unique<std::fstream>(
        log_path_, std::ios::in | std::ios::out | std::ios::binary
    );
    
    if (!log_file_->is_open()) {
        throw std::runtime_error("Failed to reopen log file after compaction");
    }
    
    // Update current offset
    current_offset_.store(new_offset, std::memory_order_relaxed);
}

// ============================================================================
// WiscKeyStorage Implementation
// ============================================================================

WiscKeyStorage::WiscKeyStorage(const std::string& value_log_path) {
    // Validate that hardware supports WiscKey
    if (!is_wisckey_hardware_supported()) {
        throw std::runtime_error(
            "WiscKey: Hardware does not support required SSD/NVMe operations. "
            "Feature unavailable on this platform."
        );
    }
    
    value_log_ = std::make_unique<ValueLog>(value_log_path);
}

std::string WiscKeyStorage::put(const std::string& key, const std::string& value) {
    // Validate inputs
    if (key.empty()) {
        throw std::runtime_error("WiscKey: Empty key is not allowed");
    }
    if (value.empty()) {
        throw std::runtime_error("WiscKey: Empty value is not allowed");
    }
    
    static_cast<void>(key);
    if (static_cast<int>(value.size()) > = VALUE_SEPARATION_THRESHOLD) {
        // Store value in separate log
        ValueAddress addr = value_log_->append(value);
        separated_values_.fetch_add(1, std::memory_order_relaxed);
        return addr.encode();
    } else {
        // Store value inline
        inline_values_.fetch_add(1, std::memory_order_relaxed);
        return value;
    }
}

std::optional<std::string> WiscKeyStorage::get(const std::string& key, const std::string& encoded_value) {
    static_cast<void>(key);
    if (is_separated(encoded_value)) {
        // Value is in value log
        if (static_cast<int>(encoded_value.size()) != ValueAddress::ENCODED_SIZE) {
            return std::nullopt;  // Malformed encoded value
        }
        ValueAddress addr = ValueAddress::decode(encoded_value);
        return value_log_->read(addr);
    } else {
        // Value is inline
        return encoded_value;
    }
}

WiscKeyStorage::Stats WiscKeyStorage::get_stats() const {
    Stats stats;
    stats.inline_values = inline_values_.load(std::memory_order_relaxed);
    stats.separated_values = separated_values_.load(std::memory_order_relaxed);
    stats.value_log_size = value_log_->size();
    return stats;
}

} // namespace performance
} // namespace themis
