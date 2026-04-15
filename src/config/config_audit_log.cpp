/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            config_audit_log.cpp                               ║
  Version:         0.0.14                                             ║
  Last Modified:   2026-04-15 18:07:35                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     82                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8aa77a0ee1  2026-03-16  fix(config): atomic enabled_ flag + concurrency stress te... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "config/config_audit_log.h"

namespace themis {
namespace config {

void ConfigAuditLog::enable() {
    enabled_.store(true, std::memory_order_relaxed);
}

void ConfigAuditLog::disable() {
    enabled_.store(false, std::memory_order_relaxed);
}

bool ConfigAuditLog::isEnabled() const {
    return enabled_.load(std::memory_order_relaxed);
}

void ConfigAuditLog::setMaxEntries(std::size_t max) {
    std::lock_guard<std::mutex> lock(mutex_);
    max_entries_ = (max >= 1) ? max : 1;
    while (entries_.size() > max_entries_) {
        entries_.pop_front();
    }
}

std::size_t ConfigAuditLog::maxEntries() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return max_entries_;
}

void ConfigAuditLog::record(AuditEntry entry) {
    // Fast path: single atomic load avoids mutex acquisition when disabled.
    if (!enabled_.load(std::memory_order_relaxed)) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.push_back(std::move(entry));
    while (entries_.size() > max_entries_) {
        entries_.pop_front();
    }
}

std::vector<AuditEntry> ConfigAuditLog::getEntries() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return {entries_.begin(), entries_.end()};
}

std::size_t ConfigAuditLog::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_.size();
}

void ConfigAuditLog::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.clear();
}

} // namespace config
} // namespace themis
