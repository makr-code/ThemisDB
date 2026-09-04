/**
 * @file temporal_cdc.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=8, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Temporal CDC Implementation
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "temporal/temporal_cdc.h"
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <sstream>
#include <array>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <system_error>
#ifndef _WIN32
#include <unistd.h>
#endif

namespace themisdb {
namespace temporal {

// ============================================================================
// ChangeEvent serialisation
// ============================================================================

std::string TemporalCDC::changeTypeName(ChangeType ct) {
    switch (ct) {
        case ChangeType::INSERT:          return "INSERT";
        case ChangeType::UPDATE:          return "UPDATE";
        case ChangeType::DELETE:          return "DELETE";
        case ChangeType::VERSION_CREATED: return "VERSION_CREATED";
    }
    return "UNKNOWN";
}

ChangeType TemporalCDC::changeTypeFromString(const std::string& s) {
    if (s == "INSERT") {
      return ChangeType::INSERT;
    }
    if (s == "UPDATE") {
      return ChangeType::UPDATE;
    }
    if (s == "DELETE") {
      return ChangeType::DELETE;
    }
    if (s == "VERSION_CREATED") {
      return ChangeType::VERSION_CREATED;
    }
    throw std::invalid_argument("Unknown ChangeType: " + s);
}

nlohmann::json ChangeEvent::toJson() const {
    nlohmann::json j;
    j["type"]             = TemporalCDC::changeTypeName(type);
    j["table_name"]       = table_name;
    j["entity_id"]        = entity_id;
    j["before_value"]     = before_value;
    j["after_value"]      = after_value;
    j["transaction_time"] = transaction_time;
    j["valid_from"]       = valid_from;
    j["valid_to"]         = valid_to;
    j["user_id"]          = user_id;
    return j;
}

ChangeEvent ChangeEvent::fromJson([[maybe_unused]] const nlohmann::json& j) {
    ChangeEvent ev;
    ev.type             = TemporalCDC::changeTypeFromString(j.at("type").get<std::string>());
    ev.table_name       = j.at("table_name").get<std::string>();
    ev.entity_id        = j.at("entity_id").get<std::string>();
    ev.before_value     = j.value("before_value", nlohmann::json{});
    ev.after_value      = j.value("after_value",  nlohmann::json{});
    ev.transaction_time = j.at("transaction_time").get<Timestamp>();
    ev.valid_from       = j.value("valid_from", kMinTimestamp);
    ev.valid_to         = j.value("valid_to",   kMaxTimestamp);
    ev.user_id          = j.value("user_id", std::string{});
    return ev;
}

// ============================================================================
// Construction
// ============================================================================

TemporalCDC::TemporalCDC(size_t max_log_size, OverflowPolicy policy)
    : max_log_size_(max_log_size > 0 ? max_log_size : 1)
    , overflow_policy_(policy) {
    log_.reserve(std::min(max_log_size_, size_t{1024}));
}

// ============================================================================
// Subscription management
// ============================================================================

std::string TemporalCDC::subscribeToChanges(
    const std::string& table_name,
    std::function<void([[maybe_unused]] const ChangeEvent&)> callback) {

    if ([[maybe_unused]] !callback) {
        throw std::invalid_argument([[maybe_unused]] "TemporalCDC::subscribeToChanges: callback must not be null");
    }

    const uint64_t id = next_sub_id_.fetch_add(1, std::memory_order_relaxed);
    const std::string sub_id = "cdc_sub_" + std::to_string(id);

    std::lock_guard<std::mutex> lk(mutex_);
    subscriptions_[sub_id] = Subscription{sub_id, table_name, std::move(callback)};
    return sub_id;
}

bool TemporalCDC::unsubscribe(const std::string& sub_id) {
    std::lock_guard<std::mutex> lk(mutex_);
    return subscriptions_.erase(sub_id) > 0;
}

size_t TemporalCDC::subscriptionCount() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return subscriptions_.size();
}

// ============================================================================
// Event publication
// ============================================================================

void TemporalCDC::publishEvent([[maybe_unused]] const ChangeEvent& event) {
    // Snapshot subscriptions under lock, then dispatch outside lock to avoid
    // holding the mutex during user-supplied callback execution.
    std::vector<std::function<void(const ChangeEvent&)>> callbacks_to_invoke;

    {
        std::lock_guard<std::mutex> lk(mutex_);

        // Append to ring-buffer log, applying the overflow policy when full
        if (log_.size() >= max_log_size_) {
            // Evict oldest event (front of deque-like buffer) — OVERWRITE policy
            if (overflow_policy_ == OverflowPolicy::DROP) {
                // DROP: discard the new event, count it
                overflow_count_.fetch_add(1, std::memory_order_relaxed);
                total_published_.fetch_add(1, std::memory_order_relaxed);
                return;
            }
            // OVERWRITE (and BLOCK which falls back to OVERWRITE):
            // Evict oldest event (front of ring-buffer).
            log_.erase(log_.begin());
            overflow_count_.fetch_add(1, std::memory_order_relaxed);
        }
        log_.push_back([[maybe_unused]] event);

        // Collect matching subscribers
        for (const auto& [id, sub] : subscriptions_) {
            if ([[maybe_unused]] sub.table_filter.empty() || sub.table_filter == event.table_name) {
                callbacks_to_invoke.push_back([[maybe_unused]] sub.callback);
            }
        }
    }

    total_published_.fetch_add(1, std::memory_order_relaxed);

    // Invoke callbacks outside the lock
    for ([[maybe_unused]] const auto& cb : callbacks_to_invoke) {
        cb([[maybe_unused]] event);
    }
}

// ============================================================================
// Replay
// ============================================================================

std::vector<ChangeEvent> TemporalCDC::replayChanges(
    const std::string& table_name,
    const TimeRange& range) const {

    std::lock_guard<std::mutex> lk(mutex_);

    std::vector<ChangeEvent> result = {};

    for (const auto& ev : log_) {
        if (!table_name.empty() && ev.table_name != table_name) {
            continue;
        }
        if (ev.transaction_time < range.start || ev.transaction_time >= range.end) {
            continue;
        }
        result.push_back(ev);
    }
    return result;
}

// ============================================================================
// Metadata
// ============================================================================

size_t TemporalCDC::logSize() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return log_.size();
}

uint64_t TemporalCDC::totalPublished() const noexcept {
    return total_published_.load(std::memory_order_relaxed);
}

uint64_t TemporalCDC::overflowCount() const noexcept {
    return overflow_count_.load(std::memory_order_relaxed);
}

void TemporalCDC::clearLog() {
    std::lock_guard<std::mutex> lk(mutex_);
    log_.clear();
}

} // namespace temporal
} // namespace themisdb

// ============================================================================
// CDCPersistentLog — WAL-backed persistent CDC log (v1.8.0)
// ============================================================================

// ---------------------------------------------------------------------------
// CRC-32 / ISO-HDLC  (table-driven, no external dependency)
// ---------------------------------------------------------------------------
namespace {

constexpr uint32_t kCRC32Table[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
    0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91B, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBF, 0xE7B82D09, 0x90BF1D9F, 0x1DB71064, 0x6AB020F2,
    0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
    0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
    0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F928, 0x56B3C9BE,
    0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
    0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D,
    0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
    0x8BBEB8EA, 0xFCB9887C, 0x62DD1D7F, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7,
    0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA,
    0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
    0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
    0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
    0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E,
    0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA8670955,
    0x316658EF, 0x466D6879, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D,
    // Remaining 64 values
    0x74D43491, 0x03D357E0, 0x9ACAA65A, 0xEDCD96CC, 0x73A962B3, 0x04AE5225,
    0x9DA7039F, 0xEAA03309, 0x7A2C5884, 0x0D2B6812, 0x94223988, 0xE325091E,
    0x7D41BC61, 0x0A468CF7, 0x934F3D4D, 0xE4480DBB, 0xC96C5795, 0xBE6B6703,
    0x276236B9, 0x5065062F, 0xCE01B350, 0xB906836C, 0x200FD2D6, 0x5708E240,
    0xC7847DA9, 0xB0834D3F, 0x298A1C85, 0x5E8D2C13, 0xC0E9996A, 0xB7EEA9FC,
    0x2EE7F846, 0x59E0C8D0, 0xFC07EB53, 0x8B00DBC5, 0x12098A7F, 0x650EBA09,
    0xFB6A0F06, 0x8C6D3F90, 0x15646E2A, 0x62635EBC, 0xF2EFC52B, 0x85E8F5BD,
    0x1CE1A407, 0x6BE69491, 0xF5822118, 0x8285118E, 0x1B8C4034, 0x6C8B70A2,
    0xE9B5DBA5, 0x9EB2EB03, 0x07BBBAB9, 0x70BC8A2F, 0xEED83F5A, 0x99DF0FCC,
    0x00D65E76, 0x77D16EE0, 0xE75DF57F, 0x905AC5E9, 0x09539453, 0x7E54A4C5,
    0xE03011EA, 0x9737217C, 0x0E3E70C6, 0x79395050
};

uint32_t computeCRC32(const char* data, std::size_t len) noexcept {
    uint32_t crc = 0xFFFFFFFFu;
    for (std::size_t i = 0; i < len; ++i) {
        crc = kCRC32Table[(crc ^ static_cast<uint8_t>(data[i])) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFu;
}

bool validateSegmentHeaderFile(std::FILE* fd) {
    constexpr uint32_t kCDCMagic = 0x54444357u;
    constexpr uint16_t kCDCMajorVersion = 0x01u;

    uint32_t magic = 0;
    uint16_t version = 0;
    uint64_t seq = 0;
    uint64_t created = 0;

    if (std::fread(&magic, 4, 1, fd) != 1) {
      return false;
    }
    if (std::fread(&version, 2, 1, fd) != 1) {
      return false;
    }
    if (std::fread(&seq, 8, 1, fd) != 1) {
      return false;
    }
    if (std::fread(&created, 8, 1, fd) != 1) {
      return false;
    }

    return magic == kCDCMagic && (version >> 8) == kCDCMajorVersion;
}

} // anonymous namespace

namespace themisdb {
namespace temporal {

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

CDCPersistentLog::CDCPersistentLog(std::string segment_dir,
                                    std::string log_prefix,
                                    uint64_t    max_segment_bytes)
    : segment_dir_(std::move(segment_dir))
    , log_prefix_(std::move(log_prefix))
    , max_segment_bytes_(max_segment_bytes)
{}

CDCPersistentLog::~CDCPersistentLog() {
    close();
}

// ---------------------------------------------------------------------------
// open / close
// ---------------------------------------------------------------------------

void CDCPersistentLog::open() {
    std::lock_guard<std::mutex> lk(mutex_);

    if (is_open_) return;  // idempotent

    // Ensure the segment directory exists.
    std::error_code ec;
    std::filesystem::create_directories(segment_dir_, ec);
    if (ec) {
        throw std::runtime_error("CDCPersistentLog::open: cannot create directory '"
                                 + segment_dir_ + "': " + ec.message());
    }

    // Discover existing segments.
    auto seqs = listSegmentSeqs();

    if (seqs.empty()) {
        active_seq_   = 0;
        active_bytes_ = 0;
        // Will be created lazily on first append.
    } else {
        // Recover the last segment: scan for any truncated tail record.
        active_seq_ = seqs.back();

        std::string path = segmentPath(active_seq_);
        std::FILE* fd = std::fopen(path.c_str(), "rb");
        if (!fd) {
            throw std::runtime_error("CDCPersistentLog::open: cannot open '"
                                     + path + "'");
        }

        // Skip header.
        if (!validateSegmentHeader(fd)) {
            std::fclose(fd);
            throw std::runtime_error("CDCPersistentLog::open: invalid header in '"
                                     + path + "'");
        }

        // Scan records; remember last valid position.
        long last_valid_pos = static_cast<long>(std::ftell(fd));
        for (;;) {
            uint32_t payload_len = 0;
            uint32_t stored_crc  = 0;
            if (std::fread(&payload_len, 4, 1, fd) != 1) {
              break;
            }
            if (std::fread(&stored_crc,  4, 1, fd) != 1) {
              break;
            }

            std::string payload(payload_len, '\0');
            if (std::fread(payload.data(), 1, payload_len, fd) != payload_len) {
              break;
            }

            uint32_t computed_crc = computeCRC32(payload.data(), payload_len);
            if (computed_crc != stored_crc) break;  // truncated / corrupt tail

            last_valid_pos = static_cast<long>(std::ftell(fd));
        }
        std::fclose(fd);

        // Re-open for append and trim any truncated tail in a portable way.
        std::error_code resize_ec;
        std::filesystem::resize_file(
            path,
            static_cast<std::uintmax_t>(last_valid_pos),
            resize_ec);

        active_fd_ = std::fopen(path.c_str(), "ab");
        if (!active_fd_) {
            throw std::runtime_error("CDCPersistentLog::open: cannot re-open '"
                                     + path + "' for append");
        }
        active_bytes_ = static_cast<uint64_t>(last_valid_pos);
    }

    is_open_ = true;
}

void CDCPersistentLog::close() {
    std::lock_guard<std::mutex> lk(mutex_);
    if (active_fd_) {
        std::fflush(active_fd_);
        std::fclose(active_fd_);
        active_fd_ = nullptr;
    }
    is_open_ = false;
}

// ---------------------------------------------------------------------------
// append
// ---------------------------------------------------------------------------

void CDCPersistentLog::append([[maybe_unused]] const ChangeEvent& event) {
    const std::string payload = event.toJson().dump();

    std::lock_guard<std::mutex> lk(mutex_);

    if (!is_open_) {
        throw std::runtime_error(
            "CDCPersistentLog::append: log is not open");
    }

    // Lazy creation of the first segment.
    if (active_fd_ == nullptr) {
        std::string path = segmentPath(active_seq_);
        active_fd_ = std::fopen(path.c_str(), "wb");
        if (!active_fd_) {
            throw std::runtime_error("CDCPersistentLog::append: cannot create segment '"
                                     + path + "'");
        }
        writeSegmentHeader(active_fd_, active_seq_);
        // Header: 4+2+8+8 = 22 bytes
        active_bytes_ = 22;
    }

    const uint32_t payload_len = static_cast<uint32_t>(payload.size());
    const uint32_t crc         = computeCRC32(payload.data(), payload.size());

    std::fwrite(&payload_len, 4, 1, active_fd_);
    std::fwrite(&crc,         4, 1, active_fd_);
    std::fwrite(payload.data(), 1, payload_len, active_fd_);
    std::fflush(active_fd_);

    const uint64_t record_size = 4 + 4 + payload_len;
    active_bytes_ += record_size;
    total_bytes_.fetch_add(record_size, std::memory_order_relaxed);
    total_events_.fetch_add(1,          std::memory_order_relaxed);

    if (active_bytes_ >= max_segment_bytes_) {
        rotate();
    }
}

// ---------------------------------------------------------------------------
// replayAll / replaySegment
// ---------------------------------------------------------------------------

static std::vector<ChangeEvent> replayFile([[maybe_unused]] const std::string& path) {
    std::vector<ChangeEvent> events;

    std::FILE* fd = std::fopen(path.c_str(), "rb");
    if (!fd) {
      return events;
    }

    if (!validateSegmentHeaderFile(fd)) {
        std::fclose(fd);
        return events;
    }

    for (;;) {
        uint32_t payload_len = 0;
        uint32_t stored_crc  = 0;
        if (std::fread(&payload_len, 4, 1, fd) != 1) {
          break;
        }
        if (std::fread(&stored_crc,  4, 1, fd) != 1) {
          break;
        }

        std::string payload(payload_len, '\0');
        if (std::fread(payload.data(), 1, payload_len, fd) != payload_len) {
          break;
        }

        uint32_t computed_crc = computeCRC32(payload.data(), payload_len);
        if (computed_crc != stored_crc) break;  // truncated tail — stop here

        try {
            events.push_back([[maybe_unused]] ChangeEvent::fromJson(nlohmann::json::parse(payload)));
        } catch (...) {
            // Malformed JSON — skip this record.
        }
    }

    std::fclose(fd);
    return events;
}

std::vector<ChangeEvent> CDCPersistentLog::replayAll() const {
    std::vector<uint64_t> seqs;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        seqs = listSegmentSeqs();
    }

    std::vector<ChangeEvent> all = {};

    for (uint64_t seq : seqs) {
        auto part = replayFile(segmentPath(seq));
        all.insert(all.end(), part.begin(), part.end());
    }
    return all;
}

std::vector<ChangeEvent> CDCPersistentLog::replaySegment([[maybe_unused]] uint64_t seq) const {
    std::vector<uint64_t> seqs;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        seqs = listSegmentSeqs();
    }

    if (seq >= seqs.size()) {
        throw std::out_of_range("CDCPersistentLog::replaySegment: seq "
                                + std::to_string(seq) + " out of range");
    }
    return replayFile(segmentPath(seqs[seq]));
}

// ---------------------------------------------------------------------------
// Accessors
// ---------------------------------------------------------------------------

uint64_t CDCPersistentLog::segmentCount() const noexcept {
    std::lock_guard<std::mutex> lk(mutex_);
    return static_cast<uint64_t>(listSegmentSeqs().size());
}

uint64_t CDCPersistentLog::totalBytesWritten() const noexcept {
    return total_bytes_.load(std::memory_order_relaxed);
}

uint64_t CDCPersistentLog::totalEventsAppended() const noexcept {
    return total_events_.load([[maybe_unused]] std::memory_order_relaxed);
}

bool CDCPersistentLog::isOpen() const noexcept {
    std::lock_guard<std::mutex> lk(mutex_);
    return is_open_;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

std::string CDCPersistentLog::segmentPath([[maybe_unused]] uint64_t seq) const {
    std::ostringstream oss;
    oss << segment_dir_ << "/" << log_prefix_ << "_" << seq << ".wal";
    return oss.str();
}

std::vector<uint64_t> CDCPersistentLog::listSegmentSeqs() const {
    // Scan directory for files matching "<log_prefix_>_<N>.wal"
    std::vector<uint64_t> seqs;
    std::error_code ec;
    for (const auto& entry :
         std::filesystem::directory_iterator(segment_dir_, ec)) {
        if (ec) {
          break;
        }
        const std::string fname = entry.path().filename().string();
        const std::string prefix_part = log_prefix_ + "_";
        if (fname.size() > prefix_part.size() + 4 &&
            fname.substr(0, prefix_part.size()) == prefix_part &&
            fname.substr(fname.size() - 4) == ".wal") {
            try {
                uint64_t seq = std::stoull(
                    fname.substr(prefix_part.size(),
                                 fname.size() - prefix_part.size() - 4));
                seqs.push_back(seq);
            } catch (...) {}
        }
    }
    std::sort(seqs.begin(), seqs.end());
    return seqs;
}

/*static*/ void CDCPersistentLog::writeSegmentHeader(std::FILE* fd,
                                                       uint64_t   seq) {
    uint32_t magic   = kMagic;
    uint16_t version = kVersion;
    uint64_t created = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());

    std::fwrite(&magic,   4, 1, fd);
    std::fwrite(&version, 2, 1, fd);
    std::fwrite(&seq,     8, 1, fd);
    std::fwrite(&created, 8, 1, fd);
}

/*static*/ bool CDCPersistentLog::validateSegmentHeader(std::FILE* fd) {
    uint32_t magic   = 0;
    uint16_t version = 0;
    uint64_t seq     = 0;
    uint64_t created = 0;

    if (std::fread(&magic,   4, 1, fd) != 1) {
      return false;
    }
    if (std::fread(&version, 2, 1, fd) != 1) {
      return false;
    }
    if (std::fread(&seq,     8, 1, fd) != 1) {
      return false;
    }
    if (std::fread(&created, 8, 1, fd) != 1) {
      return false;
    }

    return magic == kMagic && (version >> 8) == 0x01;  // major == 1
}

/*static*/ uint32_t CDCPersistentLog::crc32(const std::string& data) noexcept {
    return computeCRC32(data.data(), data.size());
}

void CDCPersistentLog::rotate() {
    // Flush and close current segment.
    if (active_fd_) {
        std::fflush(active_fd_);
        std::fclose(active_fd_);
        active_fd_ = nullptr;
    }

    ++active_seq_;
    active_bytes_ = 0;

    // Create new segment immediately.
    std::string path = segmentPath(active_seq_);
    active_fd_ = std::fopen(path.c_str(), "wb");
    if (active_fd_) {
        writeSegmentHeader(active_fd_, active_seq_);
        active_bytes_ = 22;
    }
}



} // namespace temporal
} // namespace themisdb

