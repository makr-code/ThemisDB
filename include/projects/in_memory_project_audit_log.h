/**
 * @file in_memory_project_audit_log.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "projects/project_audit_log.h"

#include <vector>
#include <mutex>
#include <algorithm>

namespace themis {
namespace projects {

/**
 * @brief In-memory implementation of IProjectAuditLog.
 *
 * Thread-safe append-only log intended for:
 *   - unit/integration testing without storage dependencies
 *   - in-process audit trail (bounded to `max_entries`)
 *
 * Production deployments should use a persistent sink (e.g. RocksDB or
 * an append-only event store).  This implementation is a production-quality
 * in-process sink: bounded capacity, thread-safe, and compliant with
 * IProjectAuditLog.
 *
 * Capacity and eviction
 * ─────────────────────
 * When the log reaches `max_entries`, the oldest 10 % of entries are
 * dropped to keep memory bounded.  Default capacity is 100 000 entries.
 */
class InMemoryProjectAuditLog : public IProjectAuditLog {
public:
    static constexpr size_t kDefaultMaxEntries = 100'000;

    explicit InMemoryProjectAuditLog(
        size_t max_entries = kDefaultMaxEntries);

    ~InMemoryProjectAuditLog() override = default;

    InMemoryProjectAuditLog(const InMemoryProjectAuditLog&)            = delete;
    InMemoryProjectAuditLog& operator=(const InMemoryProjectAuditLog&) = delete;

    // ── IProjectAuditLog ──────────────────────────────────────────────────

    /**
     * @brief Append an audit entry to the log.  Thread-safe.
     */
    void record(const ProjectAuditEntry& entry) override;

    /**
     * @brief Query log entries matching the given options.
     *
     * Applies filters in this order:
     *   1. project_id match (required)
     *   2. action_filter (optional)
     *   3. actor_id_filter (optional, empty = all actors)
     *   4. time window [start_time, end_time)
     * Result is sorted by timestamp descending (opts.sort_direction == "desc")
     * or ascending (any other value).  `limit` and `offset` are applied last.
     */
    [[nodiscard]] std::vector<ProjectAuditEntry> query(
        const AuditQueryOptions& opts) const override;

    /**
     * @brief Count entries matching the given options (ignores limit/offset).
     */
    [[nodiscard]] size_t count(const AuditQueryOptions& opts) const override;

    /**
     * @brief Remove all entries for `project_id` that are older than
     *        `before`.  Returns the number of entries removed.
     */
    bool purge(const std::string& project_id,
               std::chrono::system_clock::time_point before) override;

    // ── Observability ─────────────────────────────────────────────────────

    /**
     * @brief Total number of entries currently stored.
     */
    [[nodiscard]] size_t size() const;

    /**
     * @brief Remove all entries.  Intended for test teardown.
     */
    void clear();

private:
    mutable std::mutex mutex_;
    std::vector<ProjectAuditEntry> entries_;
    size_t max_entries_;

    /// Apply all filters in opts; returns matching entries unsorted.
    [[nodiscard]] std::vector<ProjectAuditEntry> applyFilters(
        const AuditQueryOptions& opts) const;
};

} // namespace projects
} // namespace themis
