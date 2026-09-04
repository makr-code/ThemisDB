/**
 * @file workload_replay.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "index/workload_replay.h"
#include "utils/logger.h"
#include <utility>

namespace themis {

// ============================================================================
// WorkloadEvent
// ============================================================================

json WorkloadEvent::toJSON() const {
    return {
        {"table_name",   table_name},
        {"column_name",  column_name},
        {"access_type",  (access_type == IndexRecommender::AccessType::FILTER) ? "filter" : "sort"},
        {"selectivity",  selectivity},
    };
}

WorkloadEvent WorkloadEvent::fromJSON(const json& j) {
    WorkloadEvent e;
    e.table_name  = j.at("table_name").get<std::string>();
    e.column_name = j.at("column_name").get<std::string>();
    const std::string at = j.at("access_type").get<std::string>();
    e.access_type = (at == "filter") ? IndexRecommender::AccessType::FILTER
                                     : IndexRecommender::AccessType::SORT;
    e.selectivity = j.at("selectivity").get<double>();
    return e;
}

// ============================================================================
// WorkloadCapture – public API
// ============================================================================

WorkloadCapture::WorkloadCapture(WorkloadCapture&& other) noexcept {
    std::lock_guard<std::mutex> lock(other.mutex_);
    events_ = std::move(other.events_);
    total_queries_ = other.total_queries_;
    other.total_queries_ = 0;
}

WorkloadCapture& WorkloadCapture::operator=(WorkloadCapture&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    std::scoped_lock lock(mutex_, other.mutex_);
    events_ = std::move(other.events_);
    total_queries_ = other.total_queries_;
    other.total_queries_ = 0;
    return *this;
}

void WorkloadCapture::recordEvent(const WorkloadEvent& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    events_.push_back(event);
}

void WorkloadCapture::recordEvent(std::string_view table_name,
                                  std::string_view column_name,
                                  IndexRecommender::AccessType access_type,
                                  double selectivity) {
    WorkloadEvent e;
    e.table_name  = std::string(table_name);
    e.column_name = std::string(column_name);
    e.access_type = access_type;
    e.selectivity = selectivity;
    recordEvent(e);
}

void WorkloadCapture::recordQuery() {
    std::lock_guard<std::mutex> lock(mutex_);
    ++total_queries_;
}

size_t WorkloadCapture::eventCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int>(events_.size());
}

uint64_t WorkloadCapture::totalQueries() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return total_queries_;
}

std::vector<WorkloadEvent> WorkloadCapture::events() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return events_;
}

void WorkloadCapture::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    events_.clear();
    total_queries_ = 0;
}

json WorkloadCapture::toJSON() const {
    std::lock_guard<std::mutex> lock(mutex_);
    json arr = json::array();
    for (const auto& e : events_) {
        arr.push_back(e.toJSON());
    }
    return {
        {"total_queries", total_queries_},
        {"events",        arr},
    };
}

WorkloadCapture WorkloadCapture::fromJSON(const json& j) {
    WorkloadCapture capture;
    capture.total_queries_ = j.at("total_queries").get<uint64_t>();
    const auto& eventsArr = j.at("events");
    capture.events_.reserve(eventsArr.size());
    for (const auto& ej : eventsArr) {
        capture.events_.push_back(WorkloadEvent::fromJSON(ej));
    }
    return capture;
}

// ============================================================================
// WorkloadReplayer
// ============================================================================

void WorkloadReplayer::feed(const WorkloadCapture& capture, IndexRecommender& rec) {
    const uint64_t total_queries = capture.totalQueries();
    for (uint64_t i = 0; i < total_queries; ++i) {
        rec.recordQuery();
    }
    for (const auto& e : capture.events()) {
        rec.recordAccess(e.table_name, e.column_name, e.access_type, e.selectivity);
    }
    THEMIS_DEBUG("WorkloadReplayer: fed {} events and {} queries into IndexRecommender",
                  capture.eventCount(), total_queries);
}

std::vector<IndexRecommendation> WorkloadReplayer::replay(
    const WorkloadCapture& capture,
    std::string_view table_name,
    const std::vector<std::string>& existing_indexes) const
{
    IndexRecommender rec;
    feed(capture, rec);
    return rec.recommend(table_name, existing_indexes);
}

std::map<std::string, std::vector<IndexRecommendation>> WorkloadReplayer::replayAll(
    const WorkloadCapture& capture,
    const std::map<std::string, std::vector<std::string>>& existing_indexes) const
{
    IndexRecommender rec;
    feed(capture, rec);
    return rec.recommendAll(existing_indexes);
}

} // namespace themis
