/**
 * @file quorum_log.cpp
 * @brief WAL-style quorum log implementation for durable failover consensus.
 */

#include "failover/quorum_log.h"

#include <array>
#include <chrono>
#include <fstream>
#include <sstream>

#include "spdlog/spdlog.h"

namespace themis {
namespace failover {

// ---------------------------------------------------------------------------
// CRC32 — software implementation using the standard Ethernet polynomial.
// No external library required.
// ---------------------------------------------------------------------------
namespace {

constexpr uint32_t kCrc32Polynomial = 0xEDB88320u;

const std::array<uint32_t, 256>& crc32Table() noexcept {
    static const auto table = []() noexcept {
        std::array<uint32_t, 256> t{};
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int k = 0; k < 8; ++k) {
                c = (c & 1) ? (kCrc32Polynomial ^ (c >> 1)) : (c >> 1);
            }
            t[i] = c;
        }
        return t;
    }();
    return table;
}

uint32_t crc32Update(uint32_t crc, const void* data, std::size_t len) noexcept {
    const auto* p = static_cast<const uint8_t*>(data);
    const auto& table = crc32Table();
    crc ^= 0xFFFFFFFFu;
    for (std::size_t i = 0; i < len; ++i) {
        crc = table[(crc ^ p[i]) & 0xFFu] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFu;
}

} // namespace

// ---------------------------------------------------------------------------
// QuorumLog implementation
// ---------------------------------------------------------------------------

QuorumLog::QuorumLog(std::filesystem::path log_path)
    : log_path_(std::move(log_path)) {}

uint32_t QuorumLog::computeCrc32(uint64_t epoch, const std::string& node_id,
                                  const std::string& decision, int64_t ts_ms) noexcept {
    uint32_t crc = 0;
    crc = crc32Update(crc, &epoch, sizeof(epoch));
    crc = crc32Update(crc, node_id.data(),static_cast<int>(node_id.size()));
    crc = crc32Update(crc, decision.data(),static_cast<int>(decision.size()));
    crc = crc32Update(crc, &ts_ms, sizeof(ts_ms));
    return crc;
}

bool QuorumLog::append(uint64_t epoch, const std::string& node_id,
                        const std::string& decision) {
    const int64_t ts_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::system_clock::now().time_since_epoch())
                              .count();
    const uint32_t crc = computeCrc32(epoch, node_id, decision, ts_ms);

    // Validate no pipe characters in fields (would break parsing)
    if (node_id.find('|') != std::string::npos ||
        decision.find('|') != std::string::npos) {
        spdlog::error("QuorumLog::append: pipe character in node_id or decision — rejected");
        return false;
    }

    std::ofstream ofs(log_path_, std::ios::app | std::ios::out);
    if (!ofs) {
        spdlog::error("QuorumLog::append: cannot open log file '{}'", log_path_.string());
        return false;
    }

    ofs << epoch << '|' << node_id << '|' << decision << '|'
        << ts_ms << '|' << crc << '\n';

    if (!ofs) {
        spdlog::error("QuorumLog::append: write error on log file '{}'", log_path_.string());
        return false;
    }

    ofs.flush();
    return ofs.good();
}

QuorumState QuorumLog::recover() const {
    QuorumState state;

    std::ifstream ifs(log_path_);
    if (!ifs) {
        // File absent or unreadable — return empty state
        return state;
    }

    std::string line = {};
    uint64_t line_no = 0;
    while (std::getline(ifs, line)) {
        ++line_no;
        if (line.empty()) {
            continue;
        }

        // Parse: epoch|node_id|decision|timestamp_ms|crc32
        std::array<std::string, 5> fields;
        std::istringstream ss(line);
        bool ok = true;
        for (int i = 0; i < 5; ++i) {
            if (!std::getline(ss, fields[i], '|')) {
                ok = false;
                break;
            }
        }
        if (!ok) {
            spdlog::warn("QuorumLog::recover: skipping malformed entry at line {}", line_no);
            continue;
        }

        uint64_t epoch{};
        int64_t ts_ms{};
        uint32_t stored_crc{};
        try {
            epoch = std::stoull(fields[0]);
            ts_ms = std::stoll(fields[3]);
            stored_crc = static_cast<uint32_t>(std::stoul(fields[4]));
        } catch (const std::exception& e) {
            spdlog::warn("QuorumLog::recover: skipping unparseable entry at line {}: {}",
                         line_no, e.what());
            continue;
        }

        const std::string& node_id  = fields[1];
        const std::string& decision = fields[2];

        const uint32_t expected_crc = computeCrc32(epoch, node_id, decision, ts_ms);
        if (stored_crc != expected_crc) {
            spdlog::warn("QuorumLog::recover: CRC mismatch at line {} (stored={:#010x}, expected={:#010x}) — skipping",
                         line_no, stored_crc, expected_crc);
            continue;
        }

        // Valid entry — update running state
        state.last_epoch         = epoch;
        state.last_promoted_node = node_id;
        state.last_decision      = decision;
        state.valid              = true;
    }

    return state;
}

} // namespace failover
} // namespace themis
