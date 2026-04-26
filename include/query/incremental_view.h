/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            incremental_view.h                                 ║
  Version:         0.1.0                                              ║
  Last Modified:   2026-07-01 00:00:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🟡 Interface Header (Target: Q3 2026)                       ║
╚═════════════════════════════════════════════════════════════════════╝
 */
#pragma once
// Incremental view maintenance for materialized views
#include <string>
#include <vector>
#include <chrono>

namespace themis { namespace query {

struct ViewDefinition {
    std::string view_id;
    std::string name;
    std::string query;
    bool is_materialized = true;
    std::chrono::milliseconds refresh_interval{0};
    std::vector<std::string> source_collections;
};

enum class ViewRefreshMode {
    IMMEDIATE,
    DEFERRED,
    ON_DEMAND,
};

struct ViewRefreshStats {
    std::chrono::system_clock::time_point last_refresh;
    double last_refresh_ms = 0.0;
    size_t rows_updated = 0;
    size_t rows_deleted = 0;
    size_t rows_inserted = 0;
    bool is_stale = false;
};

class IIncrementalViewMaintainer {
public:
    virtual ~IIncrementalViewMaintainer() = default;
    virtual bool createView(const ViewDefinition& def) = 0;
    virtual bool dropView(const std::string& view_id) = 0;
    virtual ViewRefreshStats refreshView(const std::string& view_id) = 0;
    virtual ViewRefreshStats getStats(const std::string& view_id) const = 0;
    virtual bool setRefreshMode(const std::string& view_id, ViewRefreshMode mode) = 0;
    virtual std::vector<ViewDefinition> listViews() const = 0;
    virtual bool isStale(const std::string& view_id) const = 0;
};

}} // namespace themis::query
