/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            wisckey.cpp                                        ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:18:18                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   88.0/100                                       ║
    • Total Lines:     210                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "performance/wisckey.h"
#include <stdexcept>
#include <cstring>
#include <cstdio>

namespace themis {
namespace performance {

ValueLog::ValueLog(const std::string& log_path) 
    : log_path_(log_path), current_offset_(0) {
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
        current_offset_ = log_file_->tellg();
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
        current_offset_ = 0;
    }
}

ValueLog::~ValueLog() {
    if (log_file_ && log_file_->is_open()) {
        log_file_->close();
    }
}

ValueAddress ValueLog::append(const std::string& value) {
    std::unique_lock<std::shared_mutex> lock(rw_mutex_);  // Exclusive lock for writes
    
    ValueAddress addr;
    addr.offset = current_offset_;
    addr.size = static_cast<uint32_t>(value.size());
    
    // Seek to end and write
    log_file_->seekp(0, std::ios::end);
    log_file_->write(value.data(), value.size());
    log_file_->flush();
    
    current_offset_ += value.size();
    return addr;
}

std::optional<std::string> ValueLog::read(const ValueAddress& addr) {
    // Exclusive lock required: std::fstream is not thread-safe for concurrent seekg/read
    // Concurrent calls to seekg() would corrupt the file position state
    std::unique_lock<std::shared_mutex> lock(rw_mutex_);
    
    log_file_->seekg(addr.offset);
    std::string value(addr.size, '\0');
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
    
    // Create temporary new log file
    std::string temp_log_path = log_path_ + ".tmp";
    std::fstream temp_log(temp_log_path, std::ios::out | std::ios::binary);
    
    if (!temp_log.is_open()) {
        throw std::runtime_error("Failed to create temporary log file for compaction");
    }
    
    uint64_t new_offset = 0;
    
    // Copy live values to new log and update addresses in-place
    for (auto& addr : live_addresses) {
        // Read value from old log
        log_file_->seekg(addr.offset);
        std::string value(addr.size, '\0');
        log_file_->read(&value[0], addr.size);
        
        if (log_file_->gcount() != static_cast<std::streamsize>(addr.size)) {
            temp_log.close();
            std::remove(temp_log_path.c_str());
            throw std::runtime_error("Failed to read value during compaction");
        }
        
        // Write value to new log
        temp_log.write(value.data(), value.size());
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
    current_offset_ = new_offset;
}

WiscKeyStorage::WiscKeyStorage(const std::string& value_log_path) {
    value_log_ = std::make_unique<ValueLog>(value_log_path);
}

std::string WiscKeyStorage::put(const std::string& key, const std::string& value) {
    if (value.size() >= VALUE_SEPARATION_THRESHOLD) {
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
    if (is_separated(encoded_value)) {
        // Value is in value log
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
