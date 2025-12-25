#include "performance/wisckey.h"
#include <stdexcept>
#include <cstring>

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
    std::lock_guard<std::mutex> lock(mutex_);
    
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
    std::lock_guard<std::mutex> lock(mutex_);
    
    log_file_->seekg(addr.offset);
    std::string value(addr.size, '\0');
    log_file_->read(&value[0], addr.size);
    
    if (log_file_->gcount() != addr.size) {
        return std::nullopt;
    }
    
    return value;
}

void ValueLog::sync() {
    std::lock_guard<std::mutex> lock(mutex_);
    log_file_->flush();
}

void ValueLog::compact(const std::vector<ValueAddress>& live_addresses) {
    // TODO: Implement garbage collection
    // This would copy live values to new log and update addresses
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
