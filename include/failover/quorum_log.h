#pragma once
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace failover {

/// @brief A single quorum decision entry persisted to the WAL.
struct QuorumEntry {
    uint64_t epoch{0};
    std::string node_id;   ///< Node that was promoted/voted on
    std::string decision;  ///< "PROMOTE", "REJECT", "QUORUM_REACHED"
    int64_t timestamp_ms{0};  ///< Unix epoch milliseconds
    uint32_t crc32{0};     ///< CRC32 of epoch+node_id+decision+timestamp_ms
};

/// @brief Recovered quorum state from the log.
struct QuorumState {
    uint64_t last_epoch{0};
    std::string last_promoted_node;
    std::string last_decision;
    bool valid{false};  ///< false if log was empty or all entries were corrupt
};

/**
 * @brief WAL-style quorum log for durable failover consensus.
 *
 * Appends quorum decisions as fixed-format text entries with CRC32 integrity.
 * On recovery, reads all valid entries and returns the last known quorum state.
 *
 * Fail-closed: if the log file cannot be opened for write, append() returns false
 * and the caller must block promotion (QUORUM_UNAVAILABLE).
 *
 * @thread_safety Not thread-safe; external synchronization required (failover_mutex_).
 */
class QuorumLog {
public:
    /// @param log_path Path to the quorum log file (created if absent).
    explicit QuorumLog(std::filesystem::path log_path);
    ~QuorumLog() = default;

    QuorumLog(const QuorumLog&) = delete;
    QuorumLog& operator=(const QuorumLog&) = delete;

    /// @brief Appends a quorum decision to the log.
    /// @returns true on success; false if the file cannot be opened or written.
    bool append(uint64_t epoch, const std::string& node_id, const std::string& decision);

    /// @brief Reads all valid entries and returns the last known quorum state.
    /// @details Entries with invalid CRC32 are skipped with a warning.
    QuorumState recover() const;

    /// @brief Returns the log file path.
    const std::filesystem::path& path() const noexcept { return log_path_; }

private:
    static uint32_t computeCrc32(uint64_t epoch, const std::string& node_id,
                                 const std::string& decision, int64_t ts_ms) noexcept;

    std::filesystem::path log_path_;
};

} // namespace failover
} // namespace themis
