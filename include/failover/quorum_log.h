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

struct QuorumEntry {
    uint64_t epoch{0};
    std::string node_id;   ///< Node that was promoted/voted on
    std::string decision;  ///< "PROMOTE", "REJECT", "QUORUM_REACHED"
    int64_t timestamp_ms{0};  ///< Unix epoch milliseconds
    uint32_t crc32{0};     ///< CRC32 of epoch+node_id+decision+timestamp_ms
};

struct QuorumState {
    uint64_t last_epoch{0};
    std::string last_promoted_node;
    std::string last_decision;
    bool valid{false};  ///< false if log was empty or all entries were corrupt
};

class QuorumLog {
public:
    /**
     * @brief Quorum Log.
     * @param[in] log_path Path to the log.
     * @return Return value.
     */
    explicit QuorumLog(std::filesystem::path log_path);
    ~QuorumLog() = default;

    QuorumLog(const QuorumLog&) = delete;
    QuorumLog& operator=(const QuorumLog&) = delete;

    /**
     * @brief Append.
     * @param[in] epoch Input parameter.
     * @param[in] node_id Identifier of the node.
     * @param[in] decision Input parameter.
     * @return True when the operation succeeds.
     */
    bool append(uint64_t epoch, const std::string& node_id, const std::string& decision);

    /**
     * @brief Recover.
     * @return Return value.
     */
    QuorumState recover() const;

    const std::filesystem::path& path() const noexcept { return log_path_; }

private:
    /**
     * @brief Compute Crc32.
     * @param[in] epoch Input parameter.
     * @param[in] node_id Identifier of the node.
     * @param[in] decision Input parameter.
     * @param[in] ts_ms Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    static uint32_t computeCrc32(uint64_t epoch, const std::string& node_id,
                                 const std::string& decision, int64_t ts_ms) noexcept;

    std::filesystem::path log_path_;
};

} // namespace failover
} // namespace themis
