/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            config_audit_log.cpp                               ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-03-16 04:14:10                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     84                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 9102ab739  2026-02-24  feat(config): implement config audit trail - log paths ac... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "config/config_audit_log.h"

namespace themis {
namespace config {

void ConfigAuditLog::enable() {
    std::lock_guard<std::mutex> lock(mutex_);
    enabled_ = true;
}

void ConfigAuditLog::disable() {
    std::lock_guard<std::mutex> lock(mutex_);
    enabled_ = false;
}

bool ConfigAuditLog::isEnabled() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return enabled_;
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
    std::lock_guard<std::mutex> lock(mutex_);
    if (!enabled_) {
        return;
    }
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
