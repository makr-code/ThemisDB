/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            temporal_cdc.cpp                                   ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-04-15 18:10:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     204                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 48fbf5b222  2026-03-21  Update search, temporal, and build artifacts ║
    • c5ff147e9f  2026-03-20  Changes before error encountered        ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * ThemisDB Temporal CDC Implementation
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "temporal/temporal_cdc.h"
#include <algorithm>
#include <stdexcept>

namespace themisdb {
namespace temporal {

// ============================================================================
// ChangeEvent serialisation
// ============================================================================

std::string TemporalCDC::changeTypeName(ChangeType ct) {
    switch (ct) {
        case ChangeType::INSERT:          return "INSERT";
        case ChangeType::UPDATE:          return "UPDATE";
        case ChangeType::DELETE:          return "DELETE";
        case ChangeType::VERSION_CREATED: return "VERSION_CREATED";
    }
    return "UNKNOWN";
}

ChangeType TemporalCDC::changeTypeFromString(const std::string& s) {
    if (s == "INSERT")          return ChangeType::INSERT;
    if (s == "UPDATE")          return ChangeType::UPDATE;
    if (s == "DELETE")          return ChangeType::DELETE;
    if (s == "VERSION_CREATED") return ChangeType::VERSION_CREATED;
    throw std::invalid_argument("Unknown ChangeType: " + s);
}

nlohmann::json ChangeEvent::toJson() const {
    nlohmann::json j;
    j["type"]             = TemporalCDC::changeTypeName(type);
    j["table_name"]       = table_name;
    j["entity_id"]        = entity_id;
    j["before_value"]     = before_value;
    j["after_value"]      = after_value;
    j["transaction_time"] = transaction_time;
    j["valid_from"]       = valid_from;
    j["valid_to"]         = valid_to;
    j["user_id"]          = user_id;
    return j;
}

ChangeEvent ChangeEvent::fromJson(const nlohmann::json& j) {
    ChangeEvent ev;
    ev.type             = TemporalCDC::changeTypeFromString(j.at("type").get<std::string>());
    ev.table_name       = j.at("table_name").get<std::string>();
    ev.entity_id        = j.at("entity_id").get<std::string>();
    ev.before_value     = j.value("before_value", nlohmann::json{});
    ev.after_value      = j.value("after_value",  nlohmann::json{});
    ev.transaction_time = j.at("transaction_time").get<Timestamp>();
    ev.valid_from       = j.value("valid_from", kMinTimestamp);
    ev.valid_to         = j.value("valid_to",   kMaxTimestamp);
    ev.user_id          = j.value("user_id", std::string{});
    return ev;
}

// ============================================================================
// Construction
// ============================================================================

TemporalCDC::TemporalCDC(size_t max_log_size)
    : max_log_size_(max_log_size > 0 ? max_log_size : 1) {
    log_.reserve(std::min(max_log_size_, size_t{1024}));
}

// ============================================================================
// Subscription management
// ============================================================================

std::string TemporalCDC::subscribeToChanges(
    const std::string& table_name,
    std::function<void(const ChangeEvent&)> callback) {

    if (!callback) {
        throw std::invalid_argument("TemporalCDC::subscribeToChanges: callback must not be null");
    }

    const uint64_t id = next_sub_id_.fetch_add(1, std::memory_order_relaxed);
    const std::string sub_id = "cdc_sub_" + std::to_string(id);

    std::lock_guard<std::mutex> lk(mutex_);
    subscriptions_[sub_id] = Subscription{sub_id, table_name, std::move(callback)};
    return sub_id;
}

bool TemporalCDC::unsubscribe(const std::string& sub_id) {
    std::lock_guard<std::mutex> lk(mutex_);
    return subscriptions_.erase(sub_id) > 0;
}

size_t TemporalCDC::subscriptionCount() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return subscriptions_.size();
}

// ============================================================================
// Event publication
// ============================================================================

void TemporalCDC::publishEvent(const ChangeEvent& event) {
    // Snapshot subscriptions under lock, then dispatch outside lock to avoid
    // holding the mutex during user-supplied callback execution.
    std::vector<std::function<void(const ChangeEvent&)>> callbacks_to_invoke;

    {
        std::lock_guard<std::mutex> lk(mutex_);

        // Append to ring-buffer log
        if (log_.size() >= max_log_size_) {
            // Evict oldest event (front of deque-like buffer)
            log_.erase(log_.begin());
        }
        log_.push_back(event);

        // Collect matching subscribers
        for (const auto& [id, sub] : subscriptions_) {
            if (sub.table_filter.empty() || sub.table_filter == event.table_name) {
                callbacks_to_invoke.push_back(sub.callback);
            }
        }
    }

    total_published_.fetch_add(1, std::memory_order_relaxed);

    // Invoke callbacks outside the lock
    for (const auto& cb : callbacks_to_invoke) {
        cb(event);
    }
}

// ============================================================================
// Replay
// ============================================================================

std::vector<ChangeEvent> TemporalCDC::replayChanges(
    const std::string& table_name,
    const TimeRange& range) const {

    std::lock_guard<std::mutex> lk(mutex_);

    std::vector<ChangeEvent> result;
    for (const auto& ev : log_) {
        if (!table_name.empty() && ev.table_name != table_name) {
            continue;
        }
        if (ev.transaction_time < range.start || ev.transaction_time >= range.end) {
            continue;
        }
        result.push_back(ev);
    }
    return result;
}

// ============================================================================
// Metadata
// ============================================================================

size_t TemporalCDC::logSize() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return log_.size();
}

uint64_t TemporalCDC::totalPublished() const noexcept {
    return total_published_.load(std::memory_order_relaxed);
}

void TemporalCDC::clearLog() {
    std::lock_guard<std::mutex> lk(mutex_);
    log_.clear();
}

} // namespace temporal
} // namespace themisdb
