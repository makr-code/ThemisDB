/**
 * @file in_memory_project_audit_log.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "projects/in_memory_project_audit_log.h"

#include <algorithm>
#include <chrono>

namespace themis {
namespace projects {

// ─────────────────────────────────────────────────────────────────────────────

InMemoryProjectAuditLog::InMemoryProjectAuditLog(size_t max_entries)
    : max_entries_(max_entries)
{
    entries_.reserve(std::min(max_entries, size_t{4096}));
}

// ── record ────────────────────────────────────────────────────────────────────

void InMemoryProjectAuditLog::record(const ProjectAuditEntry& entry)
{
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.push_back(entry);

    // Bounded eviction: drop oldest 10 % when capacity exceeded
    if (entries_.size() > max_entries_) {
        const size_t evict = std::max(size_t{1}, max_entries_ / 10);
        entries_.erase(entries_.begin(),
                       entries_.begin() + static_cast<std::ptrdiff_t>(evict));
    }
}

// ── applyFilters ──────────────────────────────────────────────────────────────

std::vector<ProjectAuditEntry> InMemoryProjectAuditLog::applyFilters(
    const AuditQueryOptions& opts) const
{
    // Caller must hold mutex_.
    std::vector<ProjectAuditEntry> result = {};

    result.reserve(entries_.size() / 4 + 1);

    const auto zero = std::chrono::system_clock::time_point{};

    for (const auto& e : entries_) {
        if (e.project_id != opts.project_id)
            continue;
        if (opts.action_filter.has_value() && e.action != *opts.action_filter)
            continue;
        if (!opts.actor_id_filter.empty() && e.actor_id != opts.actor_id_filter)
            continue;
        if (opts.start_time != zero && e.timestamp < opts.start_time)
            continue;
        if (opts.end_time   != zero && e.timestamp >= opts.end_time)
            continue;
        result.push_back(e);
    }
    return result;
}

// ── query ─────────────────────────────────────────────────────────────────────

std::vector<ProjectAuditEntry> InMemoryProjectAuditLog::query(
    const AuditQueryOptions& opts) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto result = applyFilters(opts);

    // Sort
    const bool desc = (opts.sort_direction == "desc");
    std::sort(result.begin(), result.end(),
        [desc](const ProjectAuditEntry& a, const ProjectAuditEntry& b) {
            return desc ? (a.timestamp > b.timestamp)
                        : (a.timestamp < b.timestamp);
        });

    // offset + limit
    if (opts.offset >= result.size())
        return {};
    result.erase(result.begin(),
                 result.begin() + static_cast<std::ptrdiff_t>(opts.offset));
    if (opts.limit > 0 && result.size() > opts.limit)
        result.resize(opts.limit);

    return result;
}

// ── count ─────────────────────────────────────────────────────────────────────

size_t InMemoryProjectAuditLog::count(const AuditQueryOptions& opts) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return applyFilters(opts).size();
}

// ── purge ─────────────────────────────────────────────────────────────────────

bool InMemoryProjectAuditLog::purge(
    const std::string& project_id,
    std::chrono::system_clock::time_point before)
{
    std::lock_guard<std::mutex> lock(mutex_);
    const size_t before_size = entries_.size();
    entries_.erase(
        std::remove_if(entries_.begin(), entries_.end(),
            [&]([[maybe_unused]] const ProjectAuditEntry& e) {
                return e.project_id == project_id && e.timestamp < before;
            }),
        entries_.end());
    return entries_.size() < before_size;
}

// ── size / clear ──────────────────────────────────────────────────────────────

size_t InMemoryProjectAuditLog::size() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_.size();
}

void InMemoryProjectAuditLog::clear()
{
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.clear();
}

} // namespace projects
} // namespace themis
