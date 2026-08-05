/**
 * @file ai_snapshot_cleanup.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: ai_snapshot_cleanup.cpp | Version: 0.0.1 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 170
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=1, L=0
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// AI Safety Layer — ASL-11: Snapshot Cleanup Implementation
// Docs: docs/de/security/ai_safety/AI_SAFETY_ARCHITECTURE.md
// Roadmap: src/security/ROADMAP.md § Phase 3 (ASL-11)

#include "security/ai_snapshot_cleanup.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <numeric>
#include <system_error>

namespace fs = std::filesystem;

namespace themis {
namespace security {

// ============================================================================
// Construction
// ============================================================================

AiSnapshotCleanupJob::AiSnapshotCleanupJob(const Config& cfg)
    : config_(cfg) {}

// ============================================================================
// listSnapshots
// ============================================================================

std::vector<AiSnapshotInfo> AiSnapshotCleanupJob::listSnapshots() const {
    std::vector<AiSnapshotInfo> result;

    std::error_code ec;
    if (!fs::exists(config_.snapshot_dir, ec) || ec) {
        return result;
    }

    for (const auto& entry : fs::directory_iterator(config_.snapshot_dir, ec)) {
        if (ec) break;
        if (!entry.is_directory(ec) || ec) continue;

        const std::string name = entry.path().filename().string();

        // Only process directories that look like AI pre-op snapshots.
        const bool is_snapshot =
            name.find("_pre_op") != std::string::npos ||
            name.rfind("op-", 0) == 0;
        if (!is_snapshot) continue;

        // Recursively compute size.
        std::uint64_t size = 0;
        for (const auto& sub : fs::recursive_directory_iterator(entry.path(), ec)) {
            if (ec) { ec.clear(); continue; }
            if (sub.is_regular_file(ec) && !ec) {
                size += static_cast<std::uint64_t>(sub.file_size(ec));
                if (ec) ec.clear();
            }
        }

        // Convert last_write_time to time_t.
        std::time_t ts = 0;
        auto lwt = entry.last_write_time(ec);
        if (!ec) {
            // Convert fs::file_time_type → system_clock → time_t.
            // This uses the clock-offset formula to bridge the gap between the
            // filesystem clock epoch (implementation-defined) and the system clock
            // epoch (1970-01-01).  Equivalent to clock_cast in C++20.
            const auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                lwt - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
            ts = std::chrono::system_clock::to_time_t(sctp);
        }

        result.push_back({entry.path().string(), size, ts});
    }

    // Sort ascending by creation time (oldest first) for size-capping logic.
    std::sort(result.begin(), result.end(),
              [](const AiSnapshotInfo& a, const AiSnapshotInfo& b) {
                  return a.created_at < b.created_at;
              });

    return result;
}

// ============================================================================
// runCleanup
// ============================================================================

int AiSnapshotCleanupJob::runCleanup() {
    auto snaps = listSnapshots();
    int deleted = 0;

    // Phase 1: delete all snapshots that exceed the retention age.
    for (auto it = snaps.begin(); it != snaps.end(); ) {
        if (isExpired(*it, config_.retention_days)) {
            spdlog::info("AI Safety ASL-11: deleting expired snapshot '{}' (age > {} days)",
                         it->path, config_.retention_days);
            if (removeDirectory(it->path)) {
                ++deleted;
                it = snaps.erase(it);
            } else {
                ++it;
            }
        } else {
            ++it;
        }
    }

    // Phase 2: delete oldest snapshots while total size exceeds the cap.
    const std::uint64_t max_bytes =
        config_.max_total_gb * std::uint64_t{1024} * std::uint64_t{1024} * std::uint64_t{1024};

    while (!snaps.empty() && totalSizeBytes(snaps) > max_bytes) {
        const auto& oldest = snaps.front();
        spdlog::info("AI Safety ASL-11: deleting oversized snapshot '{}' (total > {} GiB)",
                     oldest.path, config_.max_total_gb);
        if (removeDirectory(oldest.path)) {
            ++deleted;
        }
        snaps.erase(snaps.begin());
    }

    return deleted;
}

// ============================================================================
// Helper implementations
// ============================================================================

std::uint64_t AiSnapshotCleanupJob::totalSizeBytes(
    const std::vector<AiSnapshotInfo>& snaps) noexcept
{
    std::uint64_t total = 0;
    for (const auto& s : snaps) {
        total += s.size_bytes;
    }
    return total;
}

bool AiSnapshotCleanupJob::removeDirectory(const std::string& path) noexcept {
    std::error_code ec;
    if (!fs::exists(path, ec) || ec) return false;
    fs::remove_all(path, ec);
    if (ec) {
        spdlog::warn("AI Safety ASL-11: failed to remove '{}': {}", path, ec.message());
        return false;
    }
    return true;
}

bool AiSnapshotCleanupJob::isExpired(const AiSnapshotInfo& snap,
                                     int retention_days) noexcept
{
    if (retention_days <= 0) return false;
    const auto now = std::time(nullptr);
    const auto threshold = static_cast<std::time_t>(retention_days) * 86400;
    return (now - snap.created_at) > threshold;
}

} // namespace security
} // namespace themis
