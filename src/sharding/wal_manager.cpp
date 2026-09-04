/**
 * @file wal_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=2, M=7, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "sharding/wal_manager.h"
#include <filesystem>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <stdexcept>

namespace themis::sharding {

// ============================================================================
// LSN Implementation
// ============================================================================

/**
 * @brief Parse LSN from "segment/offset" textual form.
 * @param str Input LSN text.
 * @return Parsed LSN.
 * @throws std::invalid_argument on malformed input.
 */
LSN LSN::fromString(const std::string& str) {
    size_t pos = str.find('/');
    if (pos == std::string::npos) {
        throw std::invalid_argument("Invalid LSN format: " + str);
    }
    
    uint64_t segment = std::stoull(str.substr(0, pos));
    uint64_t offset = std::stoull(str.substr(pos + 1));
    
    return LSN(segment, offset);
}

// ============================================================================
// WALEntry Implementation
// ============================================================================

/** @brief Serialize WAL entry into compact binary format. */
std::vector<uint8_t> WALEntry::serialize() const {
    std::vector<uint8_t> result;
    
    // Format: [type:1][timestamp:8][lsn_segment:8][lsn_offset:8][tx_id_len:4][tx_id][data_len:4][data]
    
    result.push_back(static_cast<uint8_t>(type));
    
    // Timestamp (8 bytes, big-endian)
    for (int i = 7; i >= 0; --i) {
        result.push_back((timestamp >> (i * 8)) & 0xFF);
    }
    
    // LSN segment (8 bytes)
    for (int i = 7; i >= 0; --i) {
        result.push_back((lsn.segment >> (i * 8)) & 0xFF);
    }
    
    // LSN offset (8 bytes)
    for (int i = 7; i >= 0; --i) {
        result.push_back((lsn.offset >> (i * 8)) & 0xFF);
    }
    
    // Transaction ID length (4 bytes)
    uint32_t tx_id_len = static_cast<uint32_t>(transaction_id.size());
    for (int i = 3; i >= 0; --i) {
        result.push_back((tx_id_len >> (i * 8)) & 0xFF);
    }
    
    // Transaction ID
    result.insert(result.end(), transaction_id.begin(), transaction_id.end());
    
    // Data (JSON)
    std::string data_str = data.dump();
    uint32_t data_len = static_cast<uint32_t>(data_str.size());
    
    // Data length (4 bytes)
    for (int i = 3; i >= 0; --i) {
        result.push_back((data_len >> (i * 8)) & 0xFF);
    }
    
    // Data
    result.insert(result.end(), data_str.begin(), data_str.end());
    
    return result;
}

/**
 * @brief Deserialize WAL entry from binary bytes.
 * @param bytes Serialized entry bytes.
 * @return Parsed WAL entry.
 * @throws std::runtime_error on truncated/corrupt payload.
 */
WALEntry WALEntry::deserialize(const std::vector<uint8_t>& bytes) {
    if (static_cast<int>(bytes.size()) < 29) {  // Minimum size
        throw std::runtime_error("WAL entry too small");
    }
    
    WALEntry entry;
    size_t pos = 0;
    
    // Type
    entry.type = static_cast<WALEntryType>(bytes[pos++]);
    
    // Timestamp
    entry.timestamp = 0;
    for (int i = 0; i < 8; ++i) {
        entry.timestamp = (entry.timestamp << 8) | bytes[pos++];
    }
    
    // LSN segment
    entry.lsn.segment = 0;
    for (int i = 0; i < 8; ++i) {
        entry.lsn.segment = (entry.lsn.segment << 8) | bytes[pos++];
    }
    
    // LSN offset
    entry.lsn.offset = 0;
    for (int i = 0; i < 8; ++i) {
        entry.lsn.offset = (entry.lsn.offset << 8) | bytes[pos++];
    }
    
    // Transaction ID length
    uint32_t tx_id_len = 0;
    for (int i = 0; i < 4; ++i) {
        tx_id_len = (tx_id_len << 8) | bytes[pos++];
    }
    
    // Transaction ID
    if (pos + tx_id_len > static_cast<int>(bytes.size())) {
        throw std::runtime_error("WAL entry truncated: transaction_id overruns buffer");
    }
    entry.transaction_id = std::string(bytes.begin() + pos, bytes.begin() + pos + tx_id_len);
    pos += tx_id_len;
    
    // Data length
    if (pos + 4 > static_cast<int>(bytes.size())) {
        throw std::runtime_error("WAL entry truncated: missing data_len field");
    }
    uint32_t data_len = 0;
    for (int i = 0; i < 4; ++i) {
        data_len = (data_len << 8) | bytes[pos++];
    }
    
    // Data
    if (pos + data_len > static_cast<int>(bytes.size())) {
        throw std::runtime_error("WAL entry truncated: data overruns buffer");
    }
    std::string data_str(bytes.begin() + pos, bytes.begin() + pos + data_len);
    entry.data = nlohmann::json::parse(data_str);
    
    return entry;
}

/** @brief Return serialized byte size of this WAL entry. */
size_t WALEntry::size() const {
    return 1 + 8 + 8 + 8 + 4 + static_cast<int>(transaction_id.size()) + 4 + data.dump().size();
}

// ============================================================================
// WALManager Implementation
// ============================================================================

/**
 * @brief Construct WAL manager and recover on-disk segment state.
 * @param config WAL configuration.
 */
WALManager::WALManager(const WALManagerConfig& config)
    : config_(config), current_lsn_(0, 0), oldest_lsn_(0, 0) {
    
    // Create WAL directory if it doesn't exist
    namespace fs = std::filesystem;
    if (!fs::exists(config_.wal_directory)) {
        fs::create_directories(config_.wal_directory);
    }
    
    // Load existing segments
    loadExistingSegments();
    
    // Open current segment
    openSegment(current_lsn_.segment);
}

/** @brief Flush buffered writes and close active segment. */
WALManager::~WALManager() {
    std::lock_guard<std::mutex> lock(mutex_);
    flush();
    closeSegment();
}

/**
 * @brief Append entry to WAL and return assigned LSN.
 * @param entry Entry payload.
 * @return Assigned LSN.
 */
LSN WALManager::append(const WALEntry& entry) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Assign LSN
    WALEntry entry_copy = entry;
    entry_copy.lsn = current_lsn_;
    entry_copy.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    // Serialize entry
    auto bytes = entry_copy.serialize();
    
    // Check if we need to rotate segment
    if (current_lsn_.offset + bytes.size() > config_.segment_size) {
        flush();
        rotateSegment();
    }
    
    // Write to buffer
    write_buffer_.insert(write_buffer_.end(), bytes.begin(), bytes.end());
    
    // Update LSN
    LSN result_lsn = current_lsn_;
    current_lsn_.offset += bytes.size();
    
    // Update statistics
    total_entries_++;
    total_bytes_ += bytes.size();
    
    // Flush if configured or buffer full
    if (config_.sync_on_write || static_cast<int>(write_buffer_.size()) >= config_.write_buffer_size) {
        flush();
    }
    
    return result_lsn;
}

/** @brief Read single WAL entry by exact LSN. */
std::optional<WALEntry> WALManager::read(const LSN& lsn) {
    auto entries = readRange(lsn, LSN(lsn.segment, lsn.offset + 1));
    if (entries.empty()) {
        return std::nullopt;
    }
    return entries[0];
}

/**
 * @brief Read WAL entries in [start_lsn, end_lsn) range.
 * @param start_lsn Inclusive start LSN.
 * @param end_lsn Optional exclusive end LSN.
 * @return Ordered WAL entries in requested range.
 */
std::vector<WALEntry> WALManager::readRange(const LSN& start_lsn, 
                                            const std::optional<LSN>& end_lsn) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Make in-memory buffered writes visible to readers.
    flush();
    
    std::vector<WALEntry> result;
    
    // Determine which segments to read
    uint64_t start_segment = start_lsn.segment;
    uint64_t end_segment = end_lsn.has_value() ? end_lsn->segment : current_lsn_.segment;
    
    for (uint64_t seg = start_segment; seg <= end_segment; ++seg) {
        std::string seg_path = getSegmentPath(seg);
        
        std::ifstream file(seg_path, std::ios::binary);
        if (!file.is_open()) {
            continue;  // Segment doesn't exist
        }
        
        // Read entire segment
        file.seekg(0, std::ios::end);
        size_t file_size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        std::vector<uint8_t> buffer(file_size);
        file.read(reinterpret_cast<char*>(buffer.data()), file_size);
        
        // Parse entries
        size_t pos = 0;
        while (static_cast<size_t>(pos) <static_cast<int>(buffer.size())) {
            // Find entry size (need to peek at header)
            if (pos + 29 > static_cast<int>(buffer.size())) {
              break;
            }
            
            std::vector<uint8_t> entry_bytes(buffer.begin() + pos, buffer.end());
            
            try {
                WALEntry entry = WALEntry::deserialize(entry_bytes);
                
                // Check if within range
                if (entry.lsn < start_lsn) {
                    pos += entry.size();
                    continue;
                }
                
                if (end_lsn.has_value() && entry.lsn >= *end_lsn) {
                    break;
                }
                
                result.push_back(entry);
                pos += entry.size();
                
            } catch (...) {
                break;  // Corrupted entry or end of valid data
            }
        }
    }
    
    return result;
}

/** @brief Return current append position. */
LSN WALManager::getCurrentLSN() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_lsn_;
}

/** @brief Return oldest retained LSN. */
LSN WALManager::getOldestLSN() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return oldest_lsn_;
}

/** @brief Flush buffered WAL bytes to active segment stream. */
void WALManager::flush() {
    // Must be called with mutex held
    if (write_buffer_.empty()) {
        return;
    }
    
    if (!current_segment_ || !current_segment_->is_open()) {
        return;
    }
    
    // Write buffer to file
    current_segment_->write(reinterpret_cast<const char*>(write_buffer_.data()), 
                           write_buffer_.size());
    
    // Always flush the C++ stream buffer so same-process readers can observe
    // recently appended entries. sync_on_write controls fsync-level durability.
    current_segment_->flush();
    if (config_.sync_on_write) {
        // Note: fsync() would be called here for true durability
    }
    
    write_buffer_.clear();
}

/** @brief Append checkpoint marker and return its LSN. */
LSN WALManager::checkpoint() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Create checkpoint entry
    WALEntry checkpoint_entry;
    checkpoint_entry.type = WALEntryType::CHECKPOINT;
    checkpoint_entry.data = {
        {"checkpoint_lsn", current_lsn_.toString()}
    };
    
    return append(checkpoint_entry);
}

/**
 * @brief Truncate WAL retention before target LSN (segment-granularity).
 * @param lsn Lower retention bound.
 */
void WALManager::truncate(const LSN& lsn) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Remove segments older than lsn.segment
    namespace fs = std::filesystem;
    
    for (uint64_t seg = oldest_lsn_.segment; seg < lsn.segment; ++seg) {
        std::string seg_path = getSegmentPath(seg);
        if (fs::exists(seg_path)) {
            fs::remove(seg_path);
        }
    }
    
    oldest_lsn_ = lsn;
}

/** @brief Return WAL statistics snapshot. */
WALManager::Statistics WALManager::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    Statistics stats;
    stats.total_entries = total_entries_;
    stats.total_bytes = total_bytes_;
    stats.current_lsn = current_lsn_;
    stats.oldest_lsn = oldest_lsn_;
    
    // Count segments
    namespace fs = std::filesystem;
    for (const auto& entry : fs::directory_iterator(config_.wal_directory)) {
        if (entry.path().extension() == ".wal") {
            stats.segments++;
        }
    }
    
    return stats;
}

/** @brief Open/create active WAL segment file for given segment number. */
void WALManager::openSegment([[maybe_unused]] uint64_t segment_number) {
    std::string seg_path = getSegmentPath(segment_number);
    
    current_segment_ = std::make_unique<std::fstream>(
        seg_path,
        std::ios::in | std::ios::out | std::ios::binary | std::ios::app
    );
    
    if (!current_segment_->is_open()) {
        throw std::runtime_error("Failed to open WAL segment: " + seg_path);
    }
}

/** @brief Close active WAL segment file when open. */
void WALManager::closeSegment() {
    if (current_segment_ && current_segment_->is_open()) {
        current_segment_->close();
    }
}

/** @brief Rotate to next segment and enforce segment retention policy. */
void WALManager::rotateSegment() {
    // Must be called with mutex held
    closeSegment();
    
    current_lsn_.segment++;
    current_lsn_.offset = 0;
    
    openSegment(current_lsn_.segment);
    cleanupOldSegments();
}

/** @brief Build filesystem path for WAL segment number. */
std::string WALManager::getSegmentPath([[maybe_unused]] uint64_t segment_number) const {
    std::ostringstream oss = {};
    oss << config_.wal_directory << "/wal_" 
        << std::setfill('0') << std::setw(16) << std::hex << segment_number 
        << ".wal";
    return oss.str();
}

/** @brief Scan WAL directory and recover oldest/current LSN boundaries. */
void WALManager::loadExistingSegments() {
    namespace fs = std::filesystem;
    
    if (!fs::exists(config_.wal_directory)) {
        return;
    }
    
    // Find all WAL segments
    std::vector<uint64_t> segments = {};

    for (const auto& entry : fs::directory_iterator(config_.wal_directory)) {
        if (entry.path().extension() == ".wal") {
            std::string filename = entry.path().stem().string();
            // Parse segment number from "wal_XXXXXXXXXXXXXXXX"
            if (filename.substr(0, 4) == "wal_") {
                uint64_t seg_num = std::stoull(filename.substr(4), nullptr, 16);
                segments.push_back(seg_num);
            }
        }
    }
    
    if (segments.empty()) {
        return;
    }
    
    // Sort segments
    std::sort(segments.begin(), segments.end());
    
    oldest_lsn_.segment = segments.front();
    oldest_lsn_.offset = 0;
    
    // Set current LSN to end of last segment
    uint64_t last_segment = segments.back();
    std::string last_seg_path = getSegmentPath(last_segment);
    
    std::ifstream file(last_seg_path, std::ios::binary | std::ios::ate);
    if (file.is_open()) {
        size_t file_size = file.tellg();
        current_lsn_.segment = last_segment;
        current_lsn_.offset = file_size;
    }
}

/** @brief Remove oldest segments exceeding configured max segment count. */
void WALManager::cleanupOldSegments() {
    // Keep only max_segments
    namespace fs = std::filesystem;
    
    std::vector<std::pair<uint64_t, std::string>> segments;
    for (const auto& entry : fs::directory_iterator(config_.wal_directory)) {
        if (entry.path().extension() == ".wal") {
            std::string filename = entry.path().stem().string();
            if (filename.substr(0, 4) == "wal_") {
                uint64_t seg_num = std::stoull(filename.substr(4), nullptr, 16);
                segments.push_back({seg_num, entry.path().string()});
            }
        }
    }
    
    if (static_cast<int>(segments.size()) <= config_.max_segments) {
        return;
    }
    
    // Sort by segment number
    std::sort(segments.begin(), segments.end());
    
    // Remove oldest segments
    size_t to_remove = static_cast<int>(segments.size()) - config_.max_segments;
    for (size_t i = 0; i < to_remove; ++i) {
        fs::remove(segments[i].second);
    }
    
    // Update oldest LSN
    if (to_remove > 0) {
        oldest_lsn_.segment = segments[to_remove].first;
        oldest_lsn_.offset = 0;
    }
}

} // namespace themis::sharding

