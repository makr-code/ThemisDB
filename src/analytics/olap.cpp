/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            olap.cpp                                           ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:57:58                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   80.0/100                                       ║
    • Total Lines:     1359                                           ║
    • Open Issues:     TODOs: 0, Stubs: 3                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 02a0d7f03  2026-02-21  feat(analytics): implement Phase 2 streaming & incrementa... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "analytics/olap.h"
#include "themis/gpu/query_accelerator.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <numeric>
#include <chrono>
#include <sstream>
#include <unordered_set>
#include <map>
#include <limits>
#include <string>
#include <string_view>
#include <variant>
#include <unordered_map>
#include <vector>
#include <spdlog/spdlog.h>

#ifdef ARROW_ENABLED
#include <arrow/api.h>
#include <arrow/io/api.h>
#include <parquet/arrow/writer.h>
#endif

#if defined(_WIN32)
// Windows build stub: minimal no-op implementations to unblock compilation
// NOTE: Full OLAP functionality is not available on Windows platform
namespace themis {
namespace analytics {

class OLAPEngine::Impl {};

OLAPEngine::OLAPEngine() : impl_(nullptr) {
    spdlog::warn("OLAPEngine: Using Windows stub implementation - full OLAP functionality not available");
}
OLAPEngine::OLAPEngine(const Config&) : impl_(nullptr) {
    spdlog::warn("OLAPEngine: Using Windows stub implementation - GPU acceleration not available on Windows platform");
}
OLAPEngine::~OLAPEngine() = default;

OLAPResult OLAPEngine::execute(const OLAPQuery&) { 
    spdlog::error("OLAPEngine::execute() not supported on Windows platform");
    return {}; 
}
OLAPResult OLAPEngine::executeSimpleGroupBy(const OLAPQuery&) { 
    spdlog::error("OLAPEngine::executeSimpleGroupBy() not supported on Windows platform");
    return {}; 
}
OLAPResult OLAPEngine::executeCubeQuery(const OLAPQuery&) { 
    spdlog::error("OLAPEngine::executeCubeQuery() not supported on Windows platform");
    return {}; 
}
OLAPResult OLAPEngine::executeRollupQuery(const OLAPQuery&) { 
    spdlog::error("OLAPEngine::executeRollupQuery() not supported on Windows platform");
    return {}; 
}
OLAPResult OLAPEngine::executeGroupingSetsQuery(const OLAPQuery&) { 
    spdlog::error("OLAPEngine::executeGroupingSetsQuery() not supported on Windows platform");
    return {}; 
}
double OLAPEngine::computeAggregate(const std::vector<double>&, Measure::Function, double) { 
    spdlog::error("OLAPEngine::computeAggregate() not supported on Windows platform");
    return 0.0; 
}
OLAPEngine::QueryPlan OLAPEngine::explain(const OLAPQuery&) { 
    spdlog::error("OLAPEngine::explain() not supported on Windows platform");
    return {}; 
}
void OLAPEngine::collectStatistics(std::string_view) {
    spdlog::error("OLAPEngine::collectStatistics() not supported on Windows platform");
}
bool OLAPEngine::exportToParquet(const OLAPResult&, const std::string&, const std::string&) { 
    spdlog::error("OLAPEngine::exportToParquet() not supported on Windows platform");
    return false; 
}
bool OLAPEngine::exportCollectionToParquet(std::string_view, const std::string&, const std::vector<Filter>&, const std::string&) { 
    spdlog::error("OLAPEngine::exportCollectionToParquet() not supported on Windows platform");
    return false; 
}

class ColumnarStore::Impl {
public:
    std::unordered_map<std::string, std::string> columns;
    size_t rows = 0;
};

ColumnarStore::ColumnarStore() : impl_(std::make_unique<Impl>()) {}
ColumnarStore::~ColumnarStore() = default;
void ColumnarStore::createColumn(std::string_view name, std::string_view type) { impl_->columns[std::string(name)] = std::string(type); }
void ColumnarStore::dropColumn(std::string_view name) { impl_->columns.erase(std::string(name)); }
bool ColumnarStore::hasColumn(std::string_view name) const { return impl_->columns.count(std::string(name)) > 0; }
void ColumnarStore::appendRows(const std::vector<std::unordered_map<std::string, std::variant<std::nullptr_t, bool, int64_t, double, std::string>>>& rows) { impl_->rows += rows.size(); }
void ColumnarStore::clear() { impl_->rows = 0; }
size_t ColumnarStore::rowCount() const { return impl_->rows; }
double ColumnarStore::sum(std::string_view) const { return 0.0; }
double ColumnarStore::avg(std::string_view) const { return 0.0; }
double ColumnarStore::min(std::string_view) const { return 0.0; }
double ColumnarStore::max(std::string_view) const { return 0.0; }
int64_t ColumnarStore::count(std::string_view) const { return static_cast<int64_t>(impl_->rows); }
int64_t ColumnarStore::countDistinct(std::string_view) const { return 0; }
double ColumnarStore::sumWhere(std::string_view, const std::vector<bool>&) const { return 0.0; }
ColumnarStore::ColumnStats ColumnarStore::getColumnStats(std::string_view column) const { ColumnStats stats; stats.name = std::string(column); stats.row_count = static_cast<int64_t>(impl_->rows); return stats; }

class MaterializedView::Impl {
public:
    std::vector<std::unordered_map<std::string, std::variant<std::nullptr_t, bool, int64_t, double, std::string>>> rows;
};

MaterializedView::MaterializedView(const Definition& def) : definition_(def), impl_(std::make_unique<Impl>()) {}
MaterializedView::~MaterializedView() = default;
void MaterializedView::refresh() {}
void MaterializedView::incrementalRefresh(const std::vector<std::unordered_map<std::string, std::variant<std::nullptr_t, bool, int64_t, double, std::string>>>& changes) { impl_->rows.insert(impl_->rows.end(), changes.begin(), changes.end()); }
OLAPResult MaterializedView::query(const std::vector<Filter>&, const std::vector<Sort>&, std::optional<int64_t>) { return {}; }
std::chrono::system_clock::time_point MaterializedView::lastRefreshTime() const { return std::chrono::system_clock::now(); }
int64_t MaterializedView::rowCount() const { return static_cast<int64_t>(impl_->rows.size()); }
bool MaterializedView::isStale() const { return false; }

} // namespace analytics
} // namespace themis

#else

namespace themis {
namespace analytics {

// ============================================================================
// OLAPEngine Implementation
// ============================================================================

class OLAPEngine::Impl {
public:
    // In-memory data for testing (would connect to storage in production)
    std::unordered_map<std::string, std::vector<std::unordered_map<std::string, std::variant<std::nullptr_t, bool, int64_t, double, std::string>>>> collections;

    // GPU acceleration
    OLAPEngine::Config config;
    std::unique_ptr<themis::gpu::GPUQueryAccelerator> gpu_accelerator;
};

OLAPEngine::OLAPEngine() : impl_(std::make_unique<Impl>()) {}

OLAPEngine::OLAPEngine(const Config& config) : impl_(std::make_unique<Impl>()) {
    impl_->config = config;
    if (config.enable_gpu) {
        themis::gpu::GPUQueryAccelerator::Config gpu_cfg;
        gpu_cfg.gpu_threshold_rows = config.gpu_threshold_rows;
        impl_->gpu_accelerator = std::make_unique<themis::gpu::GPUQueryAccelerator>(gpu_cfg);
        spdlog::info("OLAPEngine: GPU acceleration enabled (device {}, threshold {} rows)",
                     config.gpu_device_id, config.gpu_threshold_rows);
    }
}

OLAPEngine::~OLAPEngine() = default;

OLAPResult OLAPEngine::execute(const OLAPQuery& query) {
    auto start = std::chrono::high_resolution_clock::now();
    
    OLAPResult result;
    
    switch (query.grouping_mode) {
        case OLAPQuery::GroupingMode::Simple:
            result = executeSimpleGroupBy(query);
            break;
        case OLAPQuery::GroupingMode::Cube:
            result = executeCubeQuery(query);
            break;
        case OLAPQuery::GroupingMode::Rollup:
            result = executeRollupQuery(query);
            break;
        case OLAPQuery::GroupingMode::GroupingSets:
            result = executeGroupingSetsQuery(query);
            break;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    result.execution_time_ms = std::chrono::duration<double, std::milli>(end - start).count();
    
    return result;
}

OLAPResult OLAPEngine::executeSimpleGroupBy(const OLAPQuery& query) {
    OLAPResult result;
    
    // Build column list
    for (const auto& dim : query.dimensions) {
        result.columns.push_back(dim.name);
    }
    for (const auto& measure : query.measures) {
        result.columns.push_back(measure.name);
    }
    
    // Group data by dimensions
    std::map<std::vector<std::string>, std::vector<double>> groups;
    
    auto it = impl_->collections.find(query.collection);
    if (it == impl_->collections.end()) {
        return result;  // Empty result for non-existent collection
    }
    
    for (const auto& row : it->second) {
        // Build group key
        std::vector<std::string> groupKey;
        for (const auto& dim : query.dimensions) {
            auto fieldIt = row.find(dim.name);
            if (fieldIt != row.end()) {
                if (auto* s = std::get_if<std::string>(&fieldIt->second)) {
                    groupKey.push_back(*s);
                } else if (auto* i = std::get_if<int64_t>(&fieldIt->second)) {
                    groupKey.push_back(std::to_string(*i));
                } else if (auto* d = std::get_if<double>(&fieldIt->second)) {
                    groupKey.push_back(std::to_string(*d));
                } else {
                    groupKey.push_back("");
                }
            } else {
                groupKey.push_back("");
            }
        }
        
        // Collect measure values
        for (const auto& measure : query.measures) {
            auto fieldIt = row.find(measure.field);
            double val = 0.0;
            if (fieldIt != row.end()) {
                if (auto* d = std::get_if<double>(&fieldIt->second)) {
                    val = *d;
                } else if (auto* i = std::get_if<int64_t>(&fieldIt->second)) {
                    val = static_cast<double>(*i);
                }
            }
            groups[groupKey].push_back(val);
        }
    }
    
    // Compute aggregates for each group
    for (const auto& [groupKey, values] : groups) {
        OLAPResult::Row resultRow;
        
        // Add dimension values
        for (size_t i = 0; i < query.dimensions.size(); ++i) {
            resultRow.values[query.dimensions[i].name] = groupKey[i];
        }
        
        // Compute measure aggregates
        size_t valueIdx = 0;
        for (const auto& measure : query.measures) {
            std::vector<double> measureValues;
            for (size_t i = valueIdx; i < values.size(); i += query.measures.size()) {
                measureValues.push_back(values[i]);
            }
            
            double aggValue = computeAggregate(measureValues, measure.function, measure.percentile_value);
            resultRow.values[measure.name] = aggValue;
            ++valueIdx;
        }
        
        result.rows.push_back(std::move(resultRow));
    }
    
    result.total_rows = result.rows.size();
    
    // Apply sorting
    if (!query.sorts.empty()) {
        std::sort(result.rows.begin(), result.rows.end(),
            [&query](const OLAPResult::Row& a, const OLAPResult::Row& b) {
                for (const auto& sort : query.sorts) {
                    auto aIt = a.values.find(sort.field);
                    auto bIt = b.values.find(sort.field);
                    
                    if (aIt == a.values.end() || bIt == b.values.end()) continue;
                    
                    // Compare as doubles for numeric types
                    double aVal = 0, bVal = 0;
                    if (auto* d = std::get_if<double>(&aIt->second)) aVal = *d;
                    else if (auto* i = std::get_if<int64_t>(&aIt->second)) aVal = static_cast<double>(*i);
                    
                    if (auto* d = std::get_if<double>(&bIt->second)) bVal = *d;
                    else if (auto* i = std::get_if<int64_t>(&bIt->second)) bVal = static_cast<double>(*i);
                    
                    if (aVal != bVal) {
                        return sort.ascending ? (aVal < bVal) : (aVal > bVal);
                    }
                }
                return false;
            });
    }
    
    // Apply limit/offset
    if (query.offset && *query.offset > 0) {
        if (static_cast<size_t>(*query.offset) < result.rows.size()) {
            result.rows.erase(result.rows.begin(), result.rows.begin() + *query.offset);
        } else {
            result.rows.clear();
        }
    }
    
    if (query.limit && *query.limit > 0) {
        if (static_cast<size_t>(*query.limit) < result.rows.size()) {
            result.has_more = true;
            result.rows.resize(*query.limit);
        }
    }
    
    return result;
}

OLAPResult OLAPEngine::executeCubeQuery(const OLAPQuery& query) {
    OLAPResult result;
    
    // CUBE generates all possible grouping combinations
    // For n dimensions, this is 2^n grouping sets
    size_t numDimensions = query.dimensions.size();
    size_t numCombinations = 1ULL << numDimensions;
    
    // Build column list
    for (const auto& dim : query.dimensions) {
        result.columns.push_back(dim.name);
    }
    for (const auto& measure : query.measures) {
        result.columns.push_back(measure.name);
    }
    result.columns.push_back("_grouping_id");
    
    // Process each grouping combination
    for (size_t mask = 0; mask < numCombinations; ++mask) {
        OLAPQuery subQuery = query;
        subQuery.grouping_mode = OLAPQuery::GroupingMode::Simple;
        subQuery.dimensions.clear();
        
        for (size_t i = 0; i < numDimensions; ++i) {
            if (mask & (1ULL << i)) {
                subQuery.dimensions.push_back(query.dimensions[i]);
            }
        }
        
        auto subResult = executeSimpleGroupBy(subQuery);
        
        for (auto& row : subResult.rows) {
            // Add NULL for dimensions not in this grouping
            for (size_t i = 0; i < numDimensions; ++i) {
                if (!(mask & (1ULL << i))) {
                    row.values[query.dimensions[i].name] = nullptr;
                }
            }
            row.grouping_id = static_cast<int64_t>(mask);
            row.values["_grouping_id"] = static_cast<int64_t>(mask);
            result.rows.push_back(std::move(row));
        }
    }
    
    result.total_rows = result.rows.size();
    return result;
}

OLAPResult OLAPEngine::executeRollupQuery(const OLAPQuery& query) {
    OLAPResult result;
    
    // ROLLUP generates hierarchical groupings
    // For dimensions (A, B, C), generates: (A,B,C), (A,B), (A), ()
    size_t numDimensions = query.dimensions.size();
    
    // Build column list
    for (const auto& dim : query.dimensions) {
        result.columns.push_back(dim.name);
    }
    for (const auto& measure : query.measures) {
        result.columns.push_back(measure.name);
    }
    result.columns.push_back("_level");
    
    // Process each rollup level
    for (size_t level = 0; level <= numDimensions; ++level) {
        OLAPQuery subQuery = query;
        subQuery.grouping_mode = OLAPQuery::GroupingMode::Simple;
        subQuery.dimensions.clear();
        
        for (size_t i = 0; i < numDimensions - level; ++i) {
            subQuery.dimensions.push_back(query.dimensions[i]);
        }
        
        auto subResult = executeSimpleGroupBy(subQuery);
        
        for (auto& row : subResult.rows) {
            // Add NULL for dimensions at higher levels
            for (size_t i = numDimensions - level; i < numDimensions; ++i) {
                row.values[query.dimensions[i].name] = nullptr;
            }
            row.values["_level"] = static_cast<int64_t>(level);
            result.rows.push_back(std::move(row));
        }
    }
    
    result.total_rows = result.rows.size();
    return result;
}

OLAPResult OLAPEngine::executeGroupingSetsQuery(const OLAPQuery& query) {
    OLAPResult result;
    
    // Build column list
    for (const auto& dim : query.dimensions) {
        result.columns.push_back(dim.name);
    }
    for (const auto& measure : query.measures) {
        result.columns.push_back(measure.name);
    }
    
    // Process each grouping set
    for (const auto& groupingSet : query.grouping_sets) {
        OLAPQuery subQuery = query;
        subQuery.grouping_mode = OLAPQuery::GroupingMode::Simple;
        subQuery.dimensions.clear();
        
        std::unordered_set<std::string> setDimensions(groupingSet.dimensions.begin(), groupingSet.dimensions.end());
        
        for (const auto& dim : query.dimensions) {
            if (setDimensions.count(dim.name)) {
                subQuery.dimensions.push_back(dim);
            }
        }
        
        auto subResult = executeSimpleGroupBy(subQuery);
        
        for (auto& row : subResult.rows) {
            // Add NULL for dimensions not in this grouping set
            for (const auto& dim : query.dimensions) {
                if (!setDimensions.count(dim.name)) {
                    row.values[dim.name] = nullptr;
                }
            }
            result.rows.push_back(std::move(row));
        }
    }
    
    result.total_rows = result.rows.size();
    return result;
}

std::vector<CubeCell> OLAPEngine::executeCube(
    std::string_view collection,
    const std::vector<Dimension>& dimensions,
    const std::vector<Measure>& measures,
    const std::vector<Filter>& filters
) {
    OLAPQuery query;
    query.collection = std::string(collection);
    query.dimensions = dimensions;
    query.measures = measures;
    query.filters = filters;
    query.grouping_mode = OLAPQuery::GroupingMode::Cube;
    
    auto result = execute(query);
    
    std::vector<CubeCell> cells;
    for (const auto& row : result.rows) {
        CubeCell cell;
        cell.grouping_id = row.grouping_id;
        
        for (const auto& dim : dimensions) {
            auto it = row.values.find(dim.name);
            if (it != row.values.end()) {
                if (std::holds_alternative<std::nullptr_t>(it->second)) {
                    cell.dimensions[dim.name] = std::nullopt;
                } else if (auto* s = std::get_if<std::string>(&it->second)) {
                    cell.dimensions[dim.name] = *s;
                }
            }
        }
        
        for (const auto& measure : measures) {
            auto it = row.values.find(measure.name);
            if (it != row.values.end()) {
                if (auto* d = std::get_if<double>(&it->second)) {
                    cell.measures[measure.name] = *d;
                } else if (auto* i = std::get_if<int64_t>(&it->second)) {
                    cell.measures[measure.name] = static_cast<double>(*i);
                }
            }
        }
        
        cells.push_back(std::move(cell));
    }
    
    return cells;
}

std::vector<RollupRow> OLAPEngine::executeRollup(
    std::string_view collection,
    const std::vector<Dimension>& dimensions,
    const std::vector<Measure>& measures,
    const std::vector<Filter>& filters
) {
    OLAPQuery query;
    query.collection = std::string(collection);
    query.dimensions = dimensions;
    query.measures = measures;
    query.filters = filters;
    query.grouping_mode = OLAPQuery::GroupingMode::Rollup;
    
    auto result = execute(query);
    
    std::vector<RollupRow> rows;
    for (const auto& row : result.rows) {
        RollupRow rollupRow;
        
        auto levelIt = row.values.find("_level");
        if (levelIt != row.values.end()) {
            if (auto* i = std::get_if<int64_t>(&levelIt->second)) {
                rollupRow.level = static_cast<int>(*i);
            }
        }
        
        for (const auto& dim : dimensions) {
            auto it = row.values.find(dim.name);
            if (it != row.values.end()) {
                if (std::holds_alternative<std::nullptr_t>(it->second)) {
                    rollupRow.dimension_values.push_back(std::nullopt);
                } else if (auto* s = std::get_if<std::string>(&it->second)) {
                    rollupRow.dimension_values.push_back(*s);
                } else {
                    rollupRow.dimension_values.push_back(std::nullopt);
                }
            } else {
                rollupRow.dimension_values.push_back(std::nullopt);
            }
        }
        
        for (const auto& measure : measures) {
            auto it = row.values.find(measure.name);
            if (it != row.values.end()) {
                if (auto* d = std::get_if<double>(&it->second)) {
                    rollupRow.measures[measure.name] = *d;
                } else if (auto* i = std::get_if<int64_t>(&it->second)) {
                    rollupRow.measures[measure.name] = static_cast<double>(*i);
                }
            }
        }
        
        rows.push_back(std::move(rollupRow));
    }
    
    return rows;
}

std::vector<OLAPEngine::WindowResult> OLAPEngine::evaluateWindowFunctions(
    const std::vector<std::unordered_map<std::string, double>>& data,
    const std::vector<Measure>& measures,
    const OLAPQuery::WindowSpec& window
) {
    std::vector<WindowResult> results;
    
    for (const auto& measure : measures) {
        WindowResult result;
        result.function = Measure::functionName(measure.function);
        result.field = measure.field;
        result.values.resize(data.size());
        
        // Simple implementation without partitioning for now
        for (size_t i = 0; i < data.size(); ++i) {
            // Determine window bounds
            size_t start = 0;
            size_t end = data.size();
            
            if (window.rows_preceding) {
                start = (i > static_cast<size_t>(*window.rows_preceding)) ? 
                        (i - *window.rows_preceding) : 0;
            }
            if (window.rows_following) {
                end = std::min(i + *window.rows_following + 1, data.size());
            }
            
            // Collect window values
            std::vector<double> windowValues;
            for (size_t j = start; j < end; ++j) {
                auto it = data[j].find(measure.field);
                if (it != data[j].end()) {
                    windowValues.push_back(it->second);
                }
            }
            
            result.values[i] = computeAggregate(windowValues, measure.function, measure.percentile_value);
        }
        
        results.push_back(std::move(result));
    }
    
    return results;
}

OLAPEngine::QueryPlan OLAPEngine::explain(const OLAPQuery& query) {
    QueryPlan plan;
    
    plan.estimated_rows = 1000;  // Placeholder
    plan.estimated_cost = 1.0;
    
    // Check for index usage
    if (query.filters.empty()) {
        plan.optimization_notes.push_back("Full table scan required (no filters)");
    } else {
        plan.optimization_notes.push_back("Filter pushdown applied");
    }
    
    // Check grouping complexity
    if (query.grouping_mode == OLAPQuery::GroupingMode::Cube) {
        size_t combinations = 1ULL << query.dimensions.size();
        plan.optimization_notes.push_back(
            "CUBE will generate " + std::to_string(combinations) + " grouping combinations"
        );
        plan.estimated_cost *= combinations;
    } else if (query.grouping_mode == OLAPQuery::GroupingMode::Rollup) {
        plan.optimization_notes.push_back(
            "ROLLUP will generate " + std::to_string(query.dimensions.size() + 1) + " levels"
        );
    }
    
    // Parallel execution possibility
    if (plan.estimated_rows > 10000) {
        plan.parallel_execution = true;
        plan.optimization_notes.push_back("Parallel execution recommended");
    }

    // GPU acceleration note
    if (impl_->config.enable_gpu) {
        plan.optimization_notes.push_back(
            "GPU acceleration enabled (CUDA/ROCm, threshold " +
            std::to_string(impl_->config.gpu_threshold_rows) + " rows)"
        );
    }
    
    return plan;
}

void OLAPEngine::collectStatistics(std::string_view collection) {
    // Placeholder for statistics collection
    (void)collection;
}

double OLAPEngine::computeAggregate(
    const std::vector<double>& values,
    Measure::Function function,
    double percentile
) {
    if (values.empty()) return 0.0;

    // GPU-accelerated path for basic aggregations when GPU is enabled and
    // the value set is large enough to justify GPU dispatch overhead.
    if (impl_->gpu_accelerator && values.size() >= impl_->config.gpu_threshold_rows) {
        using AggFunc = themis::gpu::GPUQueryAccelerator::AggFunc;
        using Row = themis::gpu::GPUQueryAccelerator::Row;

        std::optional<AggFunc> gpu_func;
        switch (function) {
            case Measure::Function::Sum:   gpu_func = AggFunc::SUM;   break;
            case Measure::Function::Count: gpu_func = AggFunc::COUNT; break;
            case Measure::Function::Min:   gpu_func = AggFunc::MIN;   break;
            case Measure::Function::Max:   gpu_func = AggFunc::MAX;   break;
            case Measure::Function::Avg:   gpu_func = AggFunc::AVG;   break;
            default: break;
        }

        if (gpu_func) {
            std::vector<Row> gpu_rows;
            gpu_rows.reserve(values.size());
            for (size_t i = 0; i < values.size(); ++i) {
                Row row;
                row.id = static_cast<uint64_t>(i);
                row.data.resize(sizeof(double));
                std::memcpy(row.data.data(), &values[i], sizeof(double));
                gpu_rows.push_back(std::move(row));
            }

            auto value_fn = [](const Row& r) -> double {
                if (r.data.size() < sizeof(double)) return 0.0;
                double v;
                std::memcpy(&v, r.data.data(), sizeof(double));
                return v;
            };

            auto agg_result = impl_->gpu_accelerator->aggregate(gpu_rows, *gpu_func, value_fn);
            return agg_result.value;
        }
    }

    switch (function) {
        case Measure::Function::Count:
            return static_cast<double>(values.size());
            
        case Measure::Function::Sum:
            return std::accumulate(values.begin(), values.end(), 0.0);
            
        case Measure::Function::Avg:
            return std::accumulate(values.begin(), values.end(), 0.0) / values.size();
            
        case Measure::Function::Min:
            return *std::min_element(values.begin(), values.end());
            
        case Measure::Function::Max:
            return *std::max_element(values.begin(), values.end());
            
        case Measure::Function::StdDev: {
            double mean = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
            double variance = 0.0;
            for (double v : values) {
                variance += (v - mean) * (v - mean);
            }
            variance /= values.size();
            return std::sqrt(variance);
        }
            
        case Measure::Function::Variance: {
            double mean = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
            double variance = 0.0;
            for (double v : values) {
                variance += (v - mean) * (v - mean);
            }
            return variance / values.size();
        }
            
        case Measure::Function::Median: {
            std::vector<double> sorted = values;
            std::sort(sorted.begin(), sorted.end());
            size_t mid = sorted.size() / 2;
            if (sorted.size() % 2 == 0) {
                return (sorted[mid - 1] + sorted[mid]) / 2.0;
            }
            return sorted[mid];
        }
            
        case Measure::Function::Percentile: {
            std::vector<double> sorted = values;
            std::sort(sorted.begin(), sorted.end());
            double rank = percentile / 100.0 * (sorted.size() - 1);
            size_t lower = static_cast<size_t>(rank);
            size_t upper = lower + 1;
            if (upper >= sorted.size()) {
                return sorted.back();
            }
            double fraction = rank - lower;
            return sorted[lower] + fraction * (sorted[upper] - sorted[lower]);
        }
            
        case Measure::Function::CountDistinct: {
            std::unordered_set<double> unique(values.begin(), values.end());
            return static_cast<double>(unique.size());
        }
            
        case Measure::Function::First:
            return values.front();
            
        case Measure::Function::Last:
            return values.back();
    }
    
    return 0.0;
}

// ============================================================================
// ColumnarStore Implementation
// ============================================================================

class ColumnarStore::Impl {
public:
    struct Column {
        std::string name;
        std::string type;
        std::vector<std::variant<std::nullptr_t, bool, int64_t, double, std::string>> data;
    };
    
    std::unordered_map<std::string, Column> columns;
    size_t row_count = 0;
};

ColumnarStore::ColumnarStore() : impl_(std::make_unique<Impl>()) {}
ColumnarStore::~ColumnarStore() = default;

void ColumnarStore::createColumn(std::string_view name, std::string_view type) {
    Impl::Column col;
    col.name = std::string(name);
    col.type = std::string(type);
    impl_->columns[col.name] = std::move(col);
}

void ColumnarStore::dropColumn(std::string_view name) {
    impl_->columns.erase(std::string(name));
}

bool ColumnarStore::hasColumn(std::string_view name) const {
    return impl_->columns.find(std::string(name)) != impl_->columns.end();
}

void ColumnarStore::appendRows(
    const std::vector<std::unordered_map<std::string, std::variant<std::nullptr_t, bool, int64_t, double, std::string>>>& rows
) {
    for (const auto& row : rows) {
        for (auto& [name, col] : impl_->columns) {
            auto it = row.find(name);
            if (it != row.end()) {
                col.data.push_back(it->second);
            } else {
                col.data.push_back(nullptr);
            }
        }
        ++impl_->row_count;
    }
}

void ColumnarStore::clear() {
    for (auto& [name, col] : impl_->columns) {
        col.data.clear();
    }
    impl_->row_count = 0;
}

size_t ColumnarStore::rowCount() const {
    return impl_->row_count;
}

double ColumnarStore::sum(std::string_view column) const {
    auto it = impl_->columns.find(std::string(column));
    if (it == impl_->columns.end()) return 0.0;
    
    double result = 0.0;
    for (const auto& val : it->second.data) {
        if (auto* d = std::get_if<double>(&val)) {
            result += *d;
        } else if (auto* i = std::get_if<int64_t>(&val)) {
            result += static_cast<double>(*i);
        }
    }
    return result;
}

double ColumnarStore::avg(std::string_view column) const {
    auto it = impl_->columns.find(std::string(column));
    if (it == impl_->columns.end() || it->second.data.empty()) return 0.0;
    
    return sum(column) / it->second.data.size();
}

double ColumnarStore::min(std::string_view column) const {
    auto it = impl_->columns.find(std::string(column));
    if (it == impl_->columns.end() || it->second.data.empty()) return 0.0;
    
    double result = std::numeric_limits<double>::max();
    for (const auto& val : it->second.data) {
        double v = 0.0;
        if (auto* d = std::get_if<double>(&val)) {
            v = *d;
        } else if (auto* i = std::get_if<int64_t>(&val)) {
            v = static_cast<double>(*i);
        } else {
            continue;
        }
        result = std::min(result, v);
    }
    return result;
}

double ColumnarStore::max(std::string_view column) const {
    auto it = impl_->columns.find(std::string(column));
    if (it == impl_->columns.end() || it->second.data.empty()) return 0.0;
    
    double result = std::numeric_limits<double>::lowest();
    for (const auto& val : it->second.data) {
        double v = 0.0;
        if (auto* d = std::get_if<double>(&val)) {
            v = *d;
        } else if (auto* i = std::get_if<int64_t>(&val)) {
            v = static_cast<double>(*i);
        } else {
            continue;
        }
        result = std::max(result, v);
    }
    return result;
}

int64_t ColumnarStore::count(std::string_view column) const {
    auto it = impl_->columns.find(std::string(column));
    if (it == impl_->columns.end()) return 0;
    
    int64_t result = 0;
    for (const auto& val : it->second.data) {
        if (!std::holds_alternative<std::nullptr_t>(val)) {
            ++result;
        }
    }
    return result;
}

int64_t ColumnarStore::countDistinct(std::string_view column) const {
    auto it = impl_->columns.find(std::string(column));
    if (it == impl_->columns.end()) return 0;
    
    std::unordered_set<std::string> unique;
    for (const auto& val : it->second.data) {
        if (auto* s = std::get_if<std::string>(&val)) {
            unique.insert(*s);
        } else if (auto* d = std::get_if<double>(&val)) {
            unique.insert(std::to_string(*d));
        } else if (auto* i = std::get_if<int64_t>(&val)) {
            unique.insert(std::to_string(*i));
        }
    }
    return static_cast<int64_t>(unique.size());
}

double ColumnarStore::sumWhere(std::string_view column, const std::vector<bool>& mask) const {
    auto it = impl_->columns.find(std::string(column));
    if (it == impl_->columns.end()) return 0.0;
    
    double result = 0.0;
    size_t minSize = std::min(it->second.data.size(), mask.size());
    for (size_t i = 0; i < minSize; ++i) {
        if (mask[i]) {
            const auto& val = it->second.data[i];
            if (auto* d = std::get_if<double>(&val)) {
                result += *d;
            } else if (auto* n = std::get_if<int64_t>(&val)) {
                result += static_cast<double>(*n);
            }
        }
    }
    return result;
}

ColumnarStore::ColumnStats ColumnarStore::getColumnStats(std::string_view column) const {
    ColumnStats stats;
    stats.name = std::string(column);
    
    auto it = impl_->columns.find(std::string(column));
    if (it == impl_->columns.end()) return stats;
    
    stats.type = it->second.type;
    stats.row_count = it->second.data.size();
    
    std::unordered_set<std::string> unique;
    double sum = 0.0;
    int64_t nonNullCount = 0;
    
    for (const auto& val : it->second.data) {
        if (std::holds_alternative<std::nullptr_t>(val)) {
            ++stats.null_count;
            continue;
        }
        
        ++nonNullCount;
        
        double v = 0.0;
        if (auto* d = std::get_if<double>(&val)) {
            v = *d;
            unique.insert(std::to_string(*d));
        } else if (auto* i = std::get_if<int64_t>(&val)) {
            v = static_cast<double>(*i);
            unique.insert(std::to_string(*i));
        } else if (auto* s = std::get_if<std::string>(&val)) {
            unique.insert(*s);
            continue;  // Skip numeric stats for strings
        }
        
        sum += v;
        if (!stats.min_value || v < *stats.min_value) stats.min_value = v;
        if (!stats.max_value || v > *stats.max_value) stats.max_value = v;
    }
    
    stats.distinct_count = unique.size();
    if (nonNullCount > 0) {
        stats.avg_value = sum / nonNullCount;
    }
    
    return stats;
}

// ============================================================================
// MaterializedView Implementation
// ============================================================================

class MaterializedView::Impl {
public:
    OLAPResult cached_result;
    std::chrono::system_clock::time_point last_refresh;
    bool is_initialized = false;

    // Per-group incremental aggregate state for delta maintenance.
    // Key = '\0'-separated dimension values; Value = map of measure_name → AggState.
    using Row = std::unordered_map<std::string, std::variant<std::nullptr_t, bool, int64_t, double, std::string>>;

    struct AggState {
        int64_t count = 0;
        double  sum   = 0.0;
        std::multiset<double> sorted;   // for MIN/MAX
        double  wf_mean = 0.0;          // Welford mean
        double  wf_m2   = 0.0;          // Welford M2

        void add(double v, int sign) {
            if (sign > 0) {
                ++count; sum += v;
                sorted.insert(v);
                double delta = v - wf_mean;
                wf_mean += delta / static_cast<double>(count);
                wf_m2   += delta * (v - wf_mean);
            } else if (sign < 0 && count > 0) {
                --count; sum -= v;
                auto it = sorted.find(v);
                if (it != sorted.end()) sorted.erase(it);
                if (count > 0) {
                    double old_mean = wf_mean;
                    double new_mean = (wf_mean * static_cast<double>(count + 1) - v)
                                      / static_cast<double>(count);
                    wf_m2 -= (v - old_mean) * (v - new_mean);
                    if (wf_m2 < 0.0) wf_m2 = 0.0;
                    wf_mean = new_mean;
                } else {
                    wf_mean = 0.0; wf_m2 = 0.0;
                }
            }
        }
        double result(Measure::Function f, double pct = 0.0) const {
            if (count == 0) return 0.0;
            switch (f) {
                case Measure::Function::Count:    return static_cast<double>(count);
                case Measure::Function::Sum:      return sum;
                case Measure::Function::Avg:      return sum / static_cast<double>(count);
                case Measure::Function::Min:      return sorted.empty() ? 0.0 : *sorted.begin();
                case Measure::Function::Max:      return sorted.empty() ? 0.0 : *sorted.rbegin();
                case Measure::Function::StdDev:   return (count >= 2) ? std::sqrt(wf_m2 / static_cast<double>(count - 1)) : 0.0;
                case Measure::Function::Variance: return (count >= 2) ? wf_m2 / static_cast<double>(count - 1) : 0.0;
                default: return sum;
            }
        }
    };

    // group_key → (measure_name → AggState)
    std::map<std::string, std::unordered_map<std::string, AggState>> groups;

    static std::string makeGroupKey(const Row& row,
                                    const std::vector<Dimension>& dims) {
        std::string key;
        for (const auto& d : dims) {
            auto it = row.find(d.name);
            if (it != row.end()) {
                if (auto* s = std::get_if<std::string>(&it->second)) key += *s;
                else if (auto* i = std::get_if<int64_t>(&it->second)) key += std::to_string(*i);
                else if (auto* dv = std::get_if<double>(&it->second)) key += std::to_string(*dv);
            }
            key += '\0';
        }
        return key;
    }

    static double fieldToDouble(const std::variant<std::nullptr_t, bool, int64_t, double, std::string>& v) {
        if (auto* d = std::get_if<double>(&v))   return *d;
        if (auto* i = std::get_if<int64_t>(&v))  return static_cast<double>(*i);
        if (auto* b = std::get_if<bool>(&v))      return *b ? 1.0 : 0.0;
        return 0.0;
    }

    void applyDelta(const Row& row, int sign, const std::vector<Dimension>& dims,
                    const std::vector<Measure>& measures) {
        std::string gk = makeGroupKey(row, dims);
        auto& group = groups[gk];
        for (const auto& m : measures) {
            auto& state = group[m.name];
            double v = 0.0;
            auto it = row.find(m.field);
            if (it != row.end()) v = fieldToDouble(it->second);
            state.add(v, sign);
        }
        // Remove empty groups after deletion
        if (sign < 0) {
            auto git = groups.find(gk);
            if (git != groups.end()) {
                bool empty = true;
                for (const auto& [n, s] : git->second)
                    if (s.count > 0) { empty = false; break; }
                if (empty) groups.erase(git);
            }
        }
    }

    // Rebuild OLAPResult from current groups
    OLAPResult buildResult(const std::vector<Dimension>& dims,
                           const std::vector<Measure>& measures) const {
        OLAPResult r;
        for (const auto& d : dims)  r.columns.push_back(d.name);
        for (const auto& m : measures) r.columns.push_back(m.name);

        for (const auto& [gk, agg_map] : groups) {
            OLAPResult::Row row;
            // Decode group key
            std::istringstream iss(gk);
            std::string token;
            for (const auto& d : dims) {
                std::getline(iss, token, '\0');
                row.values[d.name] = token;
            }
            for (const auto& m : measures) {
                auto it = agg_map.find(m.name);
                if (it != agg_map.end()) {
                    row.values[m.name] = it->second.result(m.function, m.percentile_value);
                }
            }
            r.rows.push_back(std::move(row));
        }
        r.total_rows = static_cast<int64_t>(r.rows.size());
        return r;
    }
};

MaterializedView::MaterializedView(const Definition& def) 
    : definition_(def), impl_(std::make_unique<Impl>()) {}

MaterializedView::~MaterializedView() = default;

void MaterializedView::refresh() {
    OLAPEngine engine;
    
    OLAPQuery query;
    query.collection = definition_.source_collection;
    query.dimensions = definition_.dimensions;
    query.measures = definition_.measures;
    query.filters = definition_.base_filters;
    
    impl_->cached_result = engine.execute(query);
    impl_->last_refresh = std::chrono::system_clock::now();
    impl_->is_initialized = true;
}

void MaterializedView::incrementalRefresh(
    const std::vector<std::unordered_map<std::string, std::variant<std::nullptr_t, bool, int64_t, double, std::string>>>& changes
) {
    // Delta maintenance: apply each change row as an INSERT to the incremental
    // aggregate state, then rebuild the cached OLAPResult from current groups.
    // This avoids a full re-scan of the source collection.
    for (const auto& row : changes) {
        impl_->applyDelta(row, +1, definition_.dimensions, definition_.measures);
    }
    impl_->cached_result = impl_->buildResult(definition_.dimensions, definition_.measures);
    impl_->last_refresh  = std::chrono::system_clock::now();
    impl_->is_initialized = true;
}

OLAPResult MaterializedView::query(
    const std::vector<Filter>& filters,
    const std::vector<Sort>& sorts,
    std::optional<int64_t> limit
) {
    if (!impl_->is_initialized) {
        refresh();
    }
    
    // Apply additional filters to cached result
    OLAPResult result = impl_->cached_result;
    
    // Apply filters to cached result rows
    if (!filters.empty()) {
        using RowVal = std::variant<std::nullptr_t, bool, int64_t, double, std::string>;
        auto fieldStr = [](const RowVal& v) -> std::string {
            if (auto* s = std::get_if<std::string>(&v)) return *s;
            if (auto* i = std::get_if<int64_t>(&v))    return std::to_string(*i);
            if (auto* d = std::get_if<double>(&v))      return std::to_string(*d);
            if (auto* b = std::get_if<bool>(&v))        return *b ? "true" : "false";
            return "";
        };
        auto fieldDbl = [](const RowVal& v) -> double {
            if (auto* d = std::get_if<double>(&v))   return *d;
            if (auto* i = std::get_if<int64_t>(&v))  return static_cast<double>(*i);
            if (auto* b = std::get_if<bool>(&v))      return *b ? 1.0 : 0.0;
            return 0.0;
        };
        auto filterDbl = [](const Filter& f) -> double {
            if (auto* d = std::get_if<double>(&f.value))   return *d;
            if (auto* i = std::get_if<int64_t>(&f.value))  return static_cast<double>(*i);
            if (auto* b = std::get_if<bool>(&f.value))      return *b ? 1.0 : 0.0;
            return 0.0;
        };
        auto filterStr = [](const Filter& f) -> std::string {
            if (auto* s = std::get_if<std::string>(&f.value)) return *s;
            if (auto* i = std::get_if<int64_t>(&f.value))    return std::to_string(*i);
            if (auto* d = std::get_if<double>(&f.value))      return std::to_string(*d);
            if (auto* b = std::get_if<bool>(&f.value))        return *b ? "true" : "false";
            return "";
        };

        auto passesFilters = [&](const OLAPResult::Row& row) -> bool {
            for (const auto& f : filters) {
                auto it = row.values.find(f.field);
                bool is_null = (it == row.values.end()) ||
                               std::holds_alternative<std::nullptr_t>(it->second);

                if (f.op == Filter::Operator::IsNull)    { if (!is_null) return false; continue; }
                if (f.op == Filter::Operator::IsNotNull) { if (is_null)  return false; continue; }
                if (is_null) return false;

                const auto& fv = it->second;
                switch (f.op) {
                    case Filter::Operator::Eq:
                        if (fieldStr(fv) != filterStr(f)) return false;
                        break;
                    case Filter::Operator::Ne:
                        if (fieldStr(fv) == filterStr(f)) return false;
                        break;
                    case Filter::Operator::Lt:
                        if (!(fieldDbl(fv) < filterDbl(f)))  return false;
                        break;
                    case Filter::Operator::Le:
                        if (!(fieldDbl(fv) <= filterDbl(f))) return false;
                        break;
                    case Filter::Operator::Gt:
                        if (!(fieldDbl(fv) > filterDbl(f)))  return false;
                        break;
                    case Filter::Operator::Ge:
                        if (!(fieldDbl(fv) >= filterDbl(f))) return false;
                        break;
                    case Filter::Operator::In: {
                        if (auto* vec = std::get_if<std::vector<std::string>>(&f.value)) {
                            std::string fs = fieldStr(fv);
                            if (std::find(vec->begin(), vec->end(), fs) == vec->end())
                                return false;
                        }
                        break;
                    }
                    case Filter::Operator::NotIn: {
                        if (auto* vec = std::get_if<std::vector<std::string>>(&f.value)) {
                            std::string fs = fieldStr(fv);
                            if (std::find(vec->begin(), vec->end(), fs) != vec->end())
                                return false;
                        }
                        break;
                    }
                    case Filter::Operator::Contains: {
                        std::string fs = fieldStr(fv), fvs = filterStr(f);
                        if (fs.find(fvs) == std::string::npos) return false;
                        break;
                    }
                    case Filter::Operator::StartsWith: {
                        std::string fs = fieldStr(fv), fvs = filterStr(f);
                        if (fs.rfind(fvs, 0) != 0) return false;
                        break;
                    }
                    case Filter::Operator::EndsWith: {
                        std::string fs = fieldStr(fv), fvs = filterStr(f);
                        if (fs.size() < fvs.size() ||
                            fs.rfind(fvs) != fs.size() - fvs.size()) return false;
                        break;
                    }
                    case Filter::Operator::Between: {
                        double lo = filterDbl(f), hi = lo;
                        if (f.value2) {
                            if (auto* d = std::get_if<double>(&*f.value2))
                                hi = *d;
                            else if (auto* i = std::get_if<int64_t>(&*f.value2))
                                hi = static_cast<double>(*i);
                        }
                        double fd = fieldDbl(fv);
                        if (fd < lo || fd > hi) return false;
                        break;
                    }
                    default: break;
                }
            }
            return true;
        };

        result.rows.erase(
            std::remove_if(result.rows.begin(), result.rows.end(),
                [&passesFilters](const OLAPResult::Row& row) {
                    return !passesFilters(row);
                }),
            result.rows.end()
        );
        result.total_rows = static_cast<int64_t>(result.rows.size());
    }
    
    // Apply sorting
    if (!sorts.empty()) {
        std::sort(result.rows.begin(), result.rows.end(),
            [&sorts](const OLAPResult::Row& a, const OLAPResult::Row& b) {
                for (const auto& sort : sorts) {
                    auto aIt = a.values.find(sort.field);
                    auto bIt = b.values.find(sort.field);
                    
                    if (aIt == a.values.end() || bIt == b.values.end()) continue;
                    
                    double aVal = 0, bVal = 0;
                    if (auto* d = std::get_if<double>(&aIt->second)) aVal = *d;
                    if (auto* d = std::get_if<double>(&bIt->second)) bVal = *d;
                    
                    if (aVal != bVal) {
                        return sort.ascending ? (aVal < bVal) : (aVal > bVal);
                    }
                }
                return false;
            });
    }
    
    // Apply limit
    if (limit && *limit > 0 && static_cast<size_t>(*limit) < result.rows.size()) {
        result.has_more = true;
        result.rows.resize(*limit);
    }
    
    return result;
}

std::chrono::system_clock::time_point MaterializedView::lastRefreshTime() const {
    return impl_->last_refresh;
}

int64_t MaterializedView::rowCount() const {
    return impl_->cached_result.rows.size();
}

bool MaterializedView::isStale() const {
    if (!impl_->is_initialized) return true;
    
    if (definition_.refresh_mode == Definition::RefreshMode::Manual) {
        return false;  // Manual views are never "stale"
    }
    
    auto now = std::chrono::system_clock::now();
    auto age = std::chrono::duration_cast<std::chrono::seconds>(now - impl_->last_refresh);
    
    return age.count() > definition_.refresh_interval_seconds;
}

// ============================================================================
// v1.1.0: Parquet Export Implementation
// ============================================================================

#ifdef ARROW_ENABLED

bool OLAPEngine::exportToParquet(
    const OLAPResult& result,
    const std::string& path,
    const std::string& compression
) {
        // Build Arrow schema from result columns
        std::vector<std::shared_ptr<arrow::Field>> schema_fields;
        
        for (const auto& col_name : result.columns) {
            // Infer type from first non-null value
            arrow::Type::type arrow_type = arrow::Type::STRING;  // Default to string
            
            if (!result.rows.empty()) {
                for (const auto& row : result.rows) {
                    auto it = row.values.find(col_name);
                    if (it != row.values.end()) {
                        std::visit([&](const auto& val) {
                            using T = std::decay_t<decltype(val)>;
                            if constexpr (std::is_same_v<T, bool>) {
                                arrow_type = arrow::Type::BOOL;
                            } else if constexpr (std::is_same_v<T, int64_t>) {
                                arrow_type = arrow::Type::INT64;
                            } else if constexpr (std::is_same_v<T, double>) {
                                arrow_type = arrow::Type::DOUBLE;
                            } else if constexpr (std::is_same_v<T, std::string>) {
                                arrow_type = arrow::Type::STRING;
                            }
                        }, it->second);
                        break;
                    }
                }
            }
            
            std::shared_ptr<arrow::DataType> field_type;
            switch (arrow_type) {
                case arrow::Type::BOOL: field_type = arrow::boolean(); break;
                case arrow::Type::INT64: field_type = arrow::int64(); break;
                case arrow::Type::DOUBLE: field_type = arrow::float64(); break;
                default: field_type = arrow::utf8(); break;
            }
            
            schema_fields.push_back(arrow::field(col_name, field_type));
        }
        
        auto schema = std::make_shared<arrow::Schema>(schema_fields);
        
        // Build column arrays
        std::vector<std::shared_ptr<arrow::Array>> arrays;
        
        for (size_t col_idx = 0; col_idx < result.columns.size(); ++col_idx) {
            const auto& col_name = result.columns[col_idx];
            const auto& field_type = schema_fields[col_idx]->type();
            
            // Create array builder based on type
            std::shared_ptr<arrow::Array> array;
            
            if (field_type->id() == arrow::Type::BOOL) {
                arrow::BooleanBuilder builder;
                for (const auto& row : result.rows) {
                    auto it = row.values.find(col_name);
                    if (it != row.values.end() && std::holds_alternative<bool>(it->second)) {
                        auto st = builder.Append(std::get<bool>(it->second));
                        if (!st.ok()) return false;
                    } else {
                        auto st = builder.AppendNull();
                        if (!st.ok()) return false;
                    }
                }
                auto st = builder.Finish(&array);
                if (!st.ok()) return false;
            } else if (field_type->id() == arrow::Type::INT64) {
                arrow::Int64Builder builder;
                for (const auto& row : result.rows) {
                    auto it = row.values.find(col_name);
                    if (it != row.values.end() && std::holds_alternative<int64_t>(it->second)) {
                        auto st = builder.Append(std::get<int64_t>(it->second));
                        if (!st.ok()) return false;
                    } else {
                        auto st = builder.AppendNull();
                        if (!st.ok()) return false;
                    }
                }
                auto st = builder.Finish(&array);
                if (!st.ok()) return false;
            } else if (field_type->id() == arrow::Type::DOUBLE) {
                arrow::DoubleBuilder builder;
                for (const auto& row : result.rows) {
                    auto it = row.values.find(col_name);
                    if (it != row.values.end() && std::holds_alternative<double>(it->second)) {
                        auto st = builder.Append(std::get<double>(it->second));
                        if (!st.ok()) return false;
                    } else {
                        auto st = builder.AppendNull();
                        if (!st.ok()) return false;
                    }
                }
                auto st = builder.Finish(&array);
                if (!st.ok()) return false;
            } else {
                arrow::StringBuilder builder;
                for (const auto& row : result.rows) {
                    auto it = row.values.find(col_name);
                    if (it != row.values.end() && std::holds_alternative<std::string>(it->second)) {
                        auto st = builder.Append(std::get<std::string>(it->second));
                        if (!st.ok()) return false;
                    } else {
                        auto st = builder.AppendNull();
                        if (!st.ok()) return false;
                    }
                }
                auto st = builder.Finish(&array);
                if (!st.ok()) return false;
            }
            
            arrays.push_back(array);
        }
        
        // Create Arrow Table
        auto table = arrow::Table::Make(schema, arrays);
        
        // Write to Parquet
        std::shared_ptr<arrow::io::FileOutputStream> outfile;
        PARQUET_ASSIGN_OR_THROW(outfile, arrow::io::FileOutputStream::Open(path));
        
        // Set compression
        parquet::WriterProperties::Builder props_builder;
        if (compression == "snappy") {
            props_builder.compression(parquet::Compression::SNAPPY);
        } else if (compression == "gzip") {
            props_builder.compression(parquet::Compression::GZIP);
        } else if (compression == "zstd") {
            props_builder.compression(parquet::Compression::ZSTD);
        } else {
            props_builder.compression(parquet::Compression::UNCOMPRESSED);
        }
        
        auto props = props_builder.build();
        
        // Write table
        PARQUET_THROW_NOT_OK(
            parquet::arrow::WriteTable(*table, arrow::default_memory_pool(), outfile, 1024, props)
        );
        
        return true;
}

bool OLAPEngine::exportCollectionToParquet(
    std::string_view collection,
    const std::string& path,
    const std::vector<Filter>& filters,
    const std::string& compression
) {
    // Build simple query to export all data
    OLAPQuery query;
    query.collection = std::string(collection);
    query.filters = filters;
    query.grouping_mode = OLAPQuery::GroupingMode::Simple;
    
    // Execute query to get all rows
    auto result = execute(query);
    
    // Export to Parquet
    return exportToParquet(result, path, compression);
}
#else
// Arrow not available - stub implementations
bool OLAPEngine::exportToParquet(
    const OLAPResult&,
    const std::string&,
    const std::string&
) {
    return false;  // Arrow not compiled in
}

bool OLAPEngine::exportCollectionToParquet(
    std::string_view,
    const std::string&,
    const std::vector<Filter>&,
    const std::string&
) {
    return false;  // Arrow not compiled in
}
#endif // ARROW_ENABLED

} // namespace analytics
} // namespace themis

#endif // _WIN32
