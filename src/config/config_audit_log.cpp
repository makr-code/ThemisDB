/*
 * ThemisDB | File: config_audit_log.cpp | Version: 0.0.15
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=6, H=12, M=2, L=0
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
