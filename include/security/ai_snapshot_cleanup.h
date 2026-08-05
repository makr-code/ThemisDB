/**
 * @file ai_snapshot_cleanup.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: ai_snapshot_cleanup.h | Version: 0.0.1
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// AI Safety Layer — ASL-11: Snapshot Cleanup / Retention-Policy
// Deletes pre-operation AI snapshots older than retention_days or when
// total size exceeds max_total_gb.
//
// Thread-safe: runCleanup() can be called from any thread.
// Docs: docs/de/security/ai_safety/AI_SAFETY_ARCHITECTURE.md
// Roadmap: src/security/ROADMAP.md § Phase 3 (ASL-11)

#pragma once
#include <cstdint>
#include <ctime>
#include <string>
#include <vector>

namespace themis {
namespace security {

/// Metadata about a single pre-operation snapshot directory.
struct AiSnapshotInfo {
    std::string  path;        ///< Absolute path to the snapshot directory
    std::uint64_t size_bytes; ///< Recursively summed size in bytes
    std::time_t  created_at;  ///< Unix timestamp (last_write_time)
};

/**
 * @brief ASL-11: Snapshot retention-policy enforcer.
 *
 * Scans `config.snapshot_dir` for AI pre-operation snapshot directories
 * (those whose name contains "_pre_op" or starts with "op-") and removes
 * entries that exceed the configured age or total-size limits.
 *
 * ### Thread safety
 * `runCleanup()` and `listSnapshots()` are stateless after construction;
 * safe for concurrent invocation.
 */
class AiSnapshotCleanupJob {
public:
    struct Config {
        std::string   snapshot_dir   = "/var/themis/ai-snapshots"; ///< Root snapshot directory
        int           retention_days = 7;                          ///< Max age before deletion
        std::uint64_t max_total_gb   = 100;                        ///< Max total size in GiB
    };

    explicit AiSnapshotCleanupJob(Config cfg);
    AiSnapshotCleanupJob();

    /**
     * @brief Run cleanup: remove expired and oversized snapshots.
     * @return Number of snapshot directories deleted.
     */
    [[nodiscard]] int runCleanup();

    /**
     * @brief List all AI snapshots in the directory, sorted ascending by creation time.
     */
    [[nodiscard]] std::vector<AiSnapshotInfo> listSnapshots() const;

    [[nodiscard]] const Config& config() const noexcept { return config_; }

private:
    Config config_;

    /// Return total size of all snapshots in bytes.
    [[nodiscard]] static std::uint64_t totalSizeBytes(
        const std::vector<AiSnapshotInfo>& snaps) noexcept;

    /// Recursively delete a directory; returns true on success.
    static bool removeDirectory(const std::string& path) noexcept;

    /// Return true if @p snap is older than @p retention_days.
    [[nodiscard]] static bool isExpired(const AiSnapshotInfo& snap,
                                        int retention_days) noexcept;
};

} // namespace security
} // namespace themis
