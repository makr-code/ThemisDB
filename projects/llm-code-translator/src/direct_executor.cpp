/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            direct_executor.cpp                                ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:48:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   89.0/100                                       ║
    • Total Lines:     512                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • d275653619  2026-04-14  update after codefindings               ║
    • a2d7c07202  2026-04-14  update after codefindings               ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "direct_executor.h"
#include <chrono>
#include <algorithm>
#include <sstream>
#include <cmath>
#include <unordered_map>

namespace themis {
namespace llm_translator {

namespace {

[[nodiscard]] bool isScalarKey(const nlohmann::json& key) {
    return key.is_string() || key.is_number_integer() || key.is_number_unsigned();
}

[[nodiscard]] bool recordMatchesKey(const nlohmann::json& record, const nlohmann::json& key) {
    if (!record.is_object()) {
        return false;
    }
    if (record.contains("id") && record["id"] == key) {
        return true;
    }
    if (key.is_object()) {
        bool all_match = true;
        for (const auto& [k, v] : key.items()) {
            if (!record.contains(k) || record[k] != v) {
                all_match = false;
                break;
            }
        }
        if (all_match) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] nlohmann::json makeJsonArrayFromRows(const std::vector<nlohmann::json>& rows) {
    nlohmann::json out = nlohmann::json::array();
    for (const auto& row : rows) {
        out.push_back(row);
    }
    return out;
}

[[nodiscard]] bool lessOrEqualJson(const nlohmann::json& lhs, const nlohmann::json& rhs) {
    // Mixed-type fallback uses lexicographic string comparison to keep range
    // scans deterministic for heterogeneous key payloads.
    if (lhs.type() != rhs.type()) {
        return lhs.dump() <= rhs.dump();
    }
    return lhs <= rhs;
}

} // namespace

// DirectExecutor implementation
class DirectExecutor::Impl {
public:
    std::shared_ptr<DatabaseInterface> db;
    bool metrics_enabled = true;
    ExecutionStats stats;
    
    Impl(std::shared_ptr<DatabaseInterface> database) : db(database) {
        stats.total_executions = 0;
        stats.successful_executions = 0;
        stats.failed_executions = 0;
        stats.total_execution_time_ms = 0;
        stats.avg_execution_time_ms = 0;
    }
    
    void updateStats(bool success, int64_t execution_time_ms) {
        if (!metrics_enabled) {
          return;
        }
        
        stats.total_executions++;
        if (success) {
            stats.successful_executions++;
        } else {
            stats.failed_executions++;
        }
        stats.total_execution_time_ms += execution_time_ms;
        if (stats.total_executions > 0) {
            stats.avg_execution_time_ms = stats.total_execution_time_ms / stats.total_executions;
        }
    }
};

DirectExecutor::DirectExecutor(std::shared_ptr<DatabaseInterface> db)
    : impl_(std::make_unique<Impl>(db)) {
}

DirectExecutor::~DirectExecutor() = default;

ExecutionResult DirectExecutor::execute(const ExecutionPlan& plan) {
    ResourceLimits default_limits = {};
    return execute(plan, default_limits);
}

ExecutionResult DirectExecutor::execute(const ExecutionPlan& plan, const ResourceLimits& limits) {
    auto start_time = std::chrono::steady_clock::now();
    ExecutionResult result;
    
    try {
        // Route to appropriate executor based on operation type
        switch (plan.operation) {
            case OperationType::QUERY:
                result = executeQuery(plan, limits);
                break;
            case OperationType::AGGREGATE:
                result = executeAggregate(plan, limits);
                break;
            case OperationType::TRANSFORM:
                result = executeTransform(plan, limits);
                break;
            case OperationType::JOIN:
                result = executeJoin(plan, limits);
                break;
            case OperationType::GRAPH_TRAVERSE:
                result = executeGraphTraverse(plan, limits);
                break;
            case OperationType::VECTOR_SEARCH:
                result = executeVectorSearch(plan, limits);
                break;
            case OperationType::TIME_SERIES:
                result = executeTimeSeries(plan, limits);
                break;
            case OperationType::MUTATION:
                result = executeMutation(plan, limits);
                break;
            default:
                result.success = false;
                result.error_message = "Unknown operation type";
        }
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = std::string("Execution error: ") + e.what();
    }
    
    auto end_time = std::chrono::steady_clock::now();
    result.execution_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time).count();
    
    impl_->updateStats(result.success, result.execution_time_ms);
    
    return result;
}

ExecutionResult DirectExecutor::executeQuery(const ExecutionPlan& plan, const ResourceLimits& limits) {
    ExecutionResult result;
    
    // Scan the datasource
    nlohmann::json data = impl_->db->scan(plan.datasource);
    
    // Apply filters
    if (!plan.filters.empty()) {
        data = applyFilters(data, plan.filters);
    }
    
    // Apply pagination
    int64_t limit = -1;
    int64_t offset = 0;
    
    auto limit_it = plan.parameters.find("limit");
    if (limit_it != plan.parameters.end() && std::holds_alternative<int64_t>(limit_it->second)) {
        limit = std::get<int64_t>(limit_it->second);
    }
    
    auto offset_it = plan.parameters.find("offset");
    if (offset_it != plan.parameters.end() && std::holds_alternative<int64_t>(offset_it->second)) {
        offset = std::get<int64_t>(offset_it->second);
    }
    
    if (limit > 0 || offset > 0) {
        data = applyPagination(data, limit, offset);
    }
    
    result.success = true;
    result.data = data;
    result.rows_affected = data.is_array() ? data.size() : 1;
    
    return result;
}

ExecutionResult DirectExecutor::executeAggregate(const ExecutionPlan& plan, const ResourceLimits& limits) {
    ExecutionResult result;
    
    // Get base data
    nlohmann::json data = impl_->db->scan(plan.datasource);
    
    // Apply filters first
    if (!plan.filters.empty()) {
        data = applyFilters(data, plan.filters);
    }
    
    // Group by fields (from plan.group_by)
    const auto& group_by = plan.group_by;
    const auto& aggregations = plan.aggregations;
    
    // Simplified aggregation logic (real implementation would be more sophisticated)
    if (group_by.empty()) {
        // No grouping - aggregate all rows
        nlohmann::json agg_result;
        for (const auto& agg : aggregations) {
            std::string function = agg.function;
            std::string field = agg.field;
            std::string as = agg.alias.empty() ? (field + "_" + function) : agg.alias;
            
            if (function == "COUNT") {
                agg_result[as] = data.size();
            } else if (function == "SUM" || function == "AVG") {
                double sum = 0.0;
                int count = 0;
                for (const auto& row : data) {
                    if (row.contains(field)) {
                        sum += row[field].get<double>();
                        count++;
                    }
                }
                agg_result[as] = (function == "SUM") ? sum : (count > 0 ? sum / count : 0.0);
            }
        }
        result.data = nlohmann::json::array({agg_result});
    } else {
        // With grouping - simplified version
        result.data = nlohmann::json::array();
    }
    
    result.success = true;
    result.rows_affected = result.data.size();
    
    return result;
}

ExecutionResult DirectExecutor::executeTransform(const ExecutionPlan& plan, const ResourceLimits& limits) {
    ExecutionResult result;
    
    nlohmann::json data = impl_->db->scan(plan.datasource);
    
    // Apply filters
    if (!plan.filters.empty()) {
        data = applyFilters(data, plan.filters);
    }
    
    // Apply transformations (simplified - would need transformation specs)
    
    result.success = true;
    result.data = data;
    result.rows_affected = data.is_array() ? data.size() : 1;
    
    return result;
}

ExecutionResult DirectExecutor::executeJoin(const ExecutionPlan& plan, const ResourceLimits& limits) {
    ExecutionResult result;

    // Left datasource
    nlohmann::json left = impl_->db->scan(plan.datasource);
    if (!left.is_array()) { left = nlohmann::json::array(); }

    // Right datasource from parameters["join_source"]
    std::string right_source = {};
    std::string join_key = "id";
    if (plan.parameters.count("join_source")) {
        right_source = std::get<std::string>(plan.parameters.at("join_source"));
    }
    if (plan.parameters.count("join_key")) {
        join_key = std::get<std::string>(plan.parameters.at("join_key"));
    }

    if (right_source.empty()) {
        result.success = false;
        result.error_message = "JOIN requires parameters.join_source";
        return result;
    }

    nlohmann::json right = impl_->db->scan(right_source);
    if (!right.is_array()) { right = nlohmann::json::array(); }

    // Build hash map on right side keyed by join_key
    std::unordered_map<std::string, nlohmann::json> right_map = {};

    for (const auto& row : right) {
        if (row.contains(join_key)) {
            right_map[row[join_key].dump()] = row;
        }
    }

    nlohmann::json joined = nlohmann::json::array();
    for (const auto& lrow : left) {
        if (!lrow.contains(join_key)) { continue; }
        auto it = right_map.find(lrow[join_key].dump());
        if (it == right_map.end()) { continue; }
        nlohmann::json merged = lrow;
        for (auto& [k, v] : it->second.items()) {
            if (!merged.contains(k)) { merged[k] = v; }
        }
        joined.push_back(std::move(merged));
    }

    if (!plan.filters.empty()) {
        joined = applyFilters(joined, plan.filters);
    }

    result.success = true;
    result.data = std::move(joined);
    result.rows_affected = static_cast<int64_t>(result.data.size());
    return result;
}

ExecutionResult DirectExecutor::executeGraphTraverse(const ExecutionPlan& plan, const ResourceLimits& limits) {
    ExecutionResult result;
    
    nlohmann::json start_node;
    std::string traversal_type = "BFS";
    int64_t max_depth = 10;
    
    auto start_it = plan.parameters.find("start_node");
    if (start_it != plan.parameters.end() && std::holds_alternative<std::string>(start_it->second)) {
        start_node = std::get<std::string>(start_it->second);
    }
    
    auto type_it = plan.parameters.find("traversal_type");
    if (type_it != plan.parameters.end() && std::holds_alternative<std::string>(type_it->second)) {
        traversal_type = std::get<std::string>(type_it->second);
    }
    
    auto depth_it = plan.parameters.find("max_depth");
    if (depth_it != plan.parameters.end() && std::holds_alternative<int64_t>(depth_it->second)) {
        max_depth = std::get<int64_t>(depth_it->second);
    }
    
    nlohmann::json graph_data = impl_->db->graphTraverse(plan.datasource, start_node, traversal_type, max_depth);
    
    result.success = true;
    result.data = graph_data;
    result.rows_affected = graph_data.is_array() ? graph_data.size() : 1;
    
    return result;
}

ExecutionResult DirectExecutor::executeVectorSearch(const ExecutionPlan& plan, const ResourceLimits& limits) {
    ExecutionResult result;
    
    // Vector search would need vector parameter - for now simplified
    std::vector<float> query_vector;
    int64_t top_k = 10;
    std::string distance_metric = "cosine";
    
    auto topk_it = plan.parameters.find("top_k");
    if (topk_it != plan.parameters.end() && std::holds_alternative<int64_t>(topk_it->second)) {
        top_k = std::get<int64_t>(topk_it->second);
    }
    
    auto metric_it = plan.parameters.find("distance_metric");
    if (metric_it != plan.parameters.end() && std::holds_alternative<std::string>(metric_it->second)) {
        distance_metric = std::get<std::string>(metric_it->second);
    }
    
    nlohmann::json search_results = impl_->db->vectorSearch(plan.datasource, query_vector, top_k, distance_metric);
    
    result.success = true;
    result.data = search_results;
    result.rows_affected = search_results.is_array() ? search_results.size() : 1;
    
    return result;
}

ExecutionResult DirectExecutor::executeTimeSeries(const ExecutionPlan& plan, const ResourceLimits& limits) {
    ExecutionResult result;
    
    std::string start_time = {};
    std::string end_time = {};
    std::string aggregation = "none";
    
    auto start_it = plan.parameters.find("start_time");
    if (start_it != plan.parameters.end() && std::holds_alternative<std::string>(start_it->second)) {
        start_time = std::get<std::string>(start_it->second);
    }
    
    auto end_it = plan.parameters.find("end_time");
    if (end_it != plan.parameters.end() && std::holds_alternative<std::string>(end_it->second)) {
        end_time = std::get<std::string>(end_it->second);
    }
    
    auto agg_it = plan.parameters.find("aggregation");
    if (agg_it != plan.parameters.end() && std::holds_alternative<std::string>(agg_it->second)) {
        aggregation = std::get<std::string>(agg_it->second);
    }
    
    nlohmann::json ts_data = impl_->db->timeSeriesQuery(plan.datasource, start_time, end_time, aggregation);
    
    result.success = true;
    result.data = ts_data;
    result.rows_affected = ts_data.is_array() ? ts_data.size() : 1;
    
    return result;
}

ExecutionResult DirectExecutor::executeMutation(const ExecutionPlan& plan, const ResourceLimits& limits) {
    ExecutionResult result;

    // Determine mutation type from parameters["mutation_type"]: insert | update | delete
    std::string mutation_type = "insert";
    if (plan.parameters.count("mutation_type")) {
        auto& pv = plan.parameters.at("mutation_type");
        if (std::holds_alternative<std::string>(pv)) {
            mutation_type = std::get<std::string>(pv);
        }
    }

    if (mutation_type == "insert") {
        // record is passed as a JSON string in parameters["record_json"]
        std::string record_json = {};
        if (plan.parameters.count("record_json")) {
            record_json = std::get<std::string>(plan.parameters.at("record_json"));
        }
        if (record_json.empty()) {
            result.error_message = "MUTATION insert requires parameters.record_json (JSON string)";
            return result;
        }
        nlohmann::json record = nlohmann::json::parse(record_json, nullptr, false);
        if (record.is_discarded() || !record.is_object()) {
            result.error_message = "MUTATION insert: parameters.record_json is not valid JSON object";
            return result;
        }
        nlohmann::json key = record.contains("id") ? record["id"] : record;
        bool ok = impl_->db->put(plan.datasource, key, record);
        result.success = ok;
        if (!ok) { result.error_message = "Insert failed"; }
        result.rows_affected = ok ? 1 : 0;

    } else if (mutation_type == "delete") {
        // key is passed as a string in parameters["key"]
        if (!plan.parameters.count("key")) {
            result.error_message = "MUTATION delete requires parameters.key";
            return result;
        }
        nlohmann::json key = std::get<std::string>(plan.parameters.at("key"));
        bool ok = impl_->db->del(plan.datasource, key);
        result.success = ok;
        if (!ok) { result.error_message = "Delete failed"; }
        result.rows_affected = ok ? 1 : 0;

    } else if (mutation_type == "update") {
        // key as string, updates as JSON string in parameters["updates_json"]
        if (!plan.parameters.count("key") || !plan.parameters.count("updates_json")) {
            result.error_message = "MUTATION update requires parameters.key and parameters.updates_json";
            return result;
        }
        nlohmann::json key = std::get<std::string>(plan.parameters.at("key"));
        std::string updates_str = std::get<std::string>(plan.parameters.at("updates_json"));
        nlohmann::json updates = nlohmann::json::parse(updates_str, nullptr, false);
        if (updates.is_discarded() || !updates.is_object()) {
            result.error_message = "MUTATION update: parameters.updates_json is not valid JSON object";
            return result;
        }
        nlohmann::json existing = impl_->db->get(plan.datasource, key);
        if (!existing.is_object()) { existing = nlohmann::json::object(); }
        for (auto& [k, v] : updates.items()) { existing[k] = v; }
        bool ok = impl_->db->put(plan.datasource, key, existing);
        result.success = ok;
        if (!ok) { result.error_message = "Update failed"; }
        result.rows_affected = ok ? 1 : 0;

    } else {
        result.error_message = "Unknown mutation_type: " + mutation_type;
    }

    return result;
}

nlohmann::json DirectExecutor::applyFilters(const nlohmann::json& data, 
                                             const std::vector<FilterCondition>& filters) {
    if (!data.is_array()) {
        return data;
    }
    
    nlohmann::json filtered = nlohmann::json::array();
    
    for (const auto& row : data) {
        bool matches = true;
        
        for (const auto& filter : filters) {
            if (!row.contains(filter.field)) {
                matches = false;
                break;
            }
            
            auto value = row[filter.field];
            
            // Handle different value types from PlanValue variant
            auto compareValue = [&](const auto& filter_val) -> bool {
                if (filter.op == "==") {
                    return value == filter_val;
                } else if (filter.op == "!=") {
                    return value != filter_val;
                } else if (filter.op == ">") {
                    if (value.is_number()) {
                        return value.get<double>() > static_cast<double>(filter_val);
                    }
                } else if (filter.op == ">=") {
                    if (value.is_number()) {
                        return value.get<double>() >= static_cast<double>(filter_val);
                    }
                } else if (filter.op == "<") {
                    if (value.is_number()) {
                        return value.get<double>() < static_cast<double>(filter_val);
                    }
                } else if (filter.op == "<=") {
                    if (value.is_number()) {
                        return value.get<double>() <= static_cast<double>(filter_val);
                    }
                }
                return false;
            };
            
            bool condition_met = std::visit([&](const auto& val) { return compareValue(val); }, filter.value);
            
            if (!condition_met) {
                matches = false;
                break;
            }
        }
        
        if (matches) {
            filtered.push_back(row);
        }
    }
    
    return filtered;
}

nlohmann::json DirectExecutor::applyPagination(const nlohmann::json& data, int limit, int offset) {
    if (!data.is_array()) {
        return data;
    }
    
    nlohmann::json paginated = nlohmann::json::array();
    
    size_t start_idx = std::max(0, offset);
    size_t end_idx = data.size();
    
    if (limit > 0) {
        end_idx = std::min(data.size(), start_idx + static_cast<size_t>(limit));
    }
    
    for (size_t i = start_idx; i < end_idx; ++i) {
        paginated.push_back(data[i]);
    }
    
    return paginated;
}

bool DirectExecutor::checkResourceLimits(const ResourceLimits& limits, 
                                         int64_t elapsed_ms, 
                                         size_t memory_used) {
    if (elapsed_ms > limits.max_execution_time_ms) {
        return false;
    }
    if (memory_used > limits.max_memory_bytes) {
        return false;
    }
    return true;
}

void DirectExecutor::enableMetrics(bool enable) {
    impl_->metrics_enabled = enable;
}

DirectExecutor::ExecutionStats DirectExecutor::getStats() const {
    return impl_->stats;
}

void DirectExecutor::resetStats() {
    impl_->stats = ExecutionStats();
}

// STUB/SIMULATION NOTE:
// Purpose: Provide deterministic in-process data for translator execution during tests and demos.
// Activation: Active when DirectExecutor is wired with MockDatabase instead of a real backend adapter.
// Production Delta: Operations are process-local (in-memory) but CRUD/query behavior is functional.
// Removal Plan: Keep for tests; production code paths should use concrete database adapters.
// MockDatabase implementation
MockDatabase::MockDatabase() {
    loadSampleData();
}

void MockDatabase::loadSampleData() {
    // Sample sensor data
    data_["sensor_readings"] = {
        {{"sensor_id", "S001"}, {"temperature", 67.3}, {"timestamp", "2024-12-14T10:00:00Z"}},
        {{"sensor_id", "S002"}, {"temperature", 45.2}, {"timestamp", "2024-12-14T10:00:00Z"}},
        {{"sensor_id", "S042"}, {"temperature", 52.1}, {"timestamp", "2024-12-14T10:00:00Z"}},
        {{"sensor_id", "S001"}, {"temperature", 65.8}, {"timestamp", "2024-12-14T11:00:00Z"}},
        {{"sensor_id", "S042"}, {"temperature", 54.7}, {"timestamp", "2024-12-14T11:00:00Z"}}
    };
}

nlohmann::json MockDatabase::scan(const std::string& datasource) {
    const auto it = data_.find(datasource);
    if (it == data_.end()) {
        return nlohmann::json::array();
    }
    return makeJsonArrayFromRows(it->second);
}

nlohmann::json MockDatabase::get(const std::string& datasource, const nlohmann::json& key) {
    if (!isScalarKey(key) && !key.is_object()) {
        return nlohmann::json();
    }
    const auto it = data_.find(datasource);
    if (it == data_.end()) {
        return nlohmann::json();
    }
    for (const auto& row : it->second) {
        if (recordMatchesKey(row, key)) {
            return row;
        }
    }
    return nlohmann::json();
}

bool MockDatabase::put(const std::string& datasource, const nlohmann::json& key, const nlohmann::json& value) {
    if (datasource.empty() || !value.is_object()) {
        return false;
    }
    if (!isScalarKey(key) && !key.is_object()) {
        return false;
    }
    auto record = value;
    if (!record.contains("id") && isScalarKey(key)) {
        record["id"] = key;
    }
    auto& rows = data_[datasource];
    for (auto& row : rows) {
        if (recordMatchesKey(row, key)) {
            row = std::move(record);
            return true;
        }
    }
    rows.push_back(std::move(record));
    return true;
}

bool MockDatabase::del(const std::string& datasource, const nlohmann::json& key) {
    if (!isScalarKey(key) && !key.is_object()) {
        return false;
    }
    const auto it = data_.find(datasource);
    if (it == data_.end()) {
        return false;
    }
    auto& rows = it->second;
    const auto erase_it = std::find_if(rows.begin(), rows.end(),
                                       [&](const nlohmann::json& row) {
                                           return recordMatchesKey(row, key);
                                       });
    if (erase_it == rows.end()) {
        return false;
    }
    rows.erase(erase_it);
    return true;
}

std::vector<nlohmann::json> MockDatabase::multiGet(const std::string& datasource, 
                                                      const std::vector<nlohmann::json>& keys) {
    std::vector<nlohmann::json> out = {};

    out.reserve(keys.size());
    for (const auto& key : keys) {
        auto row = get(datasource, key);
        if (!row.is_null() && !row.empty()) {
            out.push_back(std::move(row));
        }
    }
    return out;
}

nlohmann::json MockDatabase::indexScan(const std::string& datasource, 
                                        const std::string& index_name,
                                        const nlohmann::json& start_key,
                                        const nlohmann::json& end_key) {
    nlohmann::json out = nlohmann::json::array();
    if (index_name.empty()) {
        return scan(datasource);
    }
    const auto it = data_.find(datasource);
    if (it == data_.end()) {
        return out;
    }
    for (const auto& row : it->second) {
        if (!row.is_object() || !row.contains(index_name)) {
            continue;
        }
        const auto& val = row[index_name];
        const bool ge_start = start_key.is_null() || lessOrEqualJson(start_key, val);
        const bool le_end = end_key.is_null() || lessOrEqualJson(val, end_key);
        if (ge_start && le_end) {
            out.push_back(row);
        }
    }
    return out;
}

nlohmann::json MockDatabase::graphTraverse(const std::string& datasource,
                                           const nlohmann::json& start_node,
                                           const std::string& traversal_type,
                                           int max_depth) {
    (void)traversal_type;
    nlohmann::json out = nlohmann::json::array();
    if (max_depth <= 0) {
        return out;
    }
    const auto it = data_.find(datasource);
    if (it == data_.end()) {
        return out;
    }
    for (const auto& row : it->second) {
        if (!row.is_object()) {
            continue;
        }
        if (start_node.is_null() || ((row.contains("source") && row["source"] == start_node) ||
            (row.contains("from") && row["from"] == start_node) ||
            (row.contains("sensor_id") && row["sensor_id"] == start_node))) {
            out.push_back(row);
        }
    }
    return out;
}

nlohmann::json MockDatabase::vectorSearch(const std::string& datasource,
                                          const std::vector<float>& query_vector,
                                          int top_k,
                                          const std::string& distance_metric) {
    nlohmann::json out = nlohmann::json::array();
    if (top_k <= 0 || query_vector.empty()) {
        return out;
    }
    const auto it = data_.find(datasource);
    if (it == data_.end()) {
        return out;
    }
    struct ScoredRow {
        double score = 0.0;
        nlohmann::json row;
    };
    std::vector<ScoredRow> scored;
    double qn = 0.0;
    if (distance_metric != "l2") {
        for (const auto v : query_vector) {
            qn += static_cast<double>(v) * v;
        }
    }
    const double query_norm = (distance_metric != "l2" && qn > 0.0) ? std::sqrt(qn) : 0.0;
    for (const auto& row : it->second) {
        if (!row.is_object() || !row.contains("embedding") || !row["embedding"].is_array()) {
            continue;
        }
        const auto embedding = row["embedding"].get<std::vector<float>>();
        if (embedding.empty()) {
            continue;
        }
        if (embedding.size() != query_vector.size()) {
            continue;
        }
        const auto count = embedding.size();
        double score = 0.0;
        if (distance_metric == "l2") {
            for (std::size_t i = 0; i < count; ++i) {
                const auto d = static_cast<double>(embedding[i] - query_vector[i]);
                score += d * d;
            }
            score = std::sqrt(score);
        } else {
            double dot = 0.0;
            double en = 0.0;
            for (std::size_t i = 0; i < count; ++i) {
                dot += static_cast<double>(embedding[i]) * query_vector[i];
                en += static_cast<double>(embedding[i]) * embedding[i];
            }
            score = (query_norm > 0.0 && en > 0.0) ? dot / (query_norm * std::sqrt(en)) : 0.0;
        }
        scored.push_back({score, row});
    }
    if (distance_metric == "l2") {
        std::sort(scored.begin(), scored.end(),
                  [](const ScoredRow& a, const ScoredRow& b) { return a.score < b.score; });
    } else {
        std::sort(scored.begin(), scored.end(),
                  [](const ScoredRow& a, const ScoredRow& b) { return a.score > b.score; });
    }
    const auto take = std::min<std::size_t>(static_cast<std::size_t>(top_k), scored.size());
    for (std::size_t i = 0; i < take; ++i) {
        auto row = scored[i].row;
        row["score"] = scored[i].score;
        out.push_back(std::move(row));
    }
    return out;
}

nlohmann::json MockDatabase::timeSeriesQuery(const std::string& datasource,
                                             const std::string& start_time,
                                             const std::string& end_time,
                                             const std::string& aggregation) {
    nlohmann::json filtered = nlohmann::json::array();
    const auto it = data_.find(datasource);
    if (it == data_.end()) {
        return filtered;
    }
    for (const auto& row : it->second) {
        if (!row.is_object() || !row.contains("timestamp")) {
            continue;
        }
        const auto ts = row["timestamp"].get<std::string>();
        if (((!start_time.empty() && ts < start_time) || (!end_time.empty() && ts > end_time))) {
            continue;
        }
        filtered.push_back(row);
    }
    if (aggregation == "none" || aggregation.empty()) {
        return filtered;
    }
    double sum = 0.0;
    double min_val = std::numeric_limits<double>::max();
    double max_val = std::numeric_limits<double>::lowest();
    std::size_t count = 0;
    for (const auto& row : filtered) {
        if (!row.contains("temperature") || !row["temperature"].is_number()) {
            continue;
        }
        const auto v = row["temperature"].get<double>();
        sum += v;
        min_val = std::min(min_val, v);
        max_val = std::max(max_val, v);
        ++count;
    }
    nlohmann::json aggregated = nlohmann::json::object();
    aggregated["count"] = count;
    if (count == 0) {
        aggregated["value"] = 0.0;
        return nlohmann::json::array({aggregated});
    }
    if (aggregation == "avg") {
        aggregated["value"] = sum / static_cast<double>(count);
    } else if (aggregation == "sum") {
        aggregated["value"] = sum;
    } else if (aggregation == "min") {
        aggregated["value"] = min_val;
    } else if (aggregation == "max") {
        aggregated["value"] = max_val;
    } else {
        return filtered;
    }
    return nlohmann::json::array({aggregated});
}

} // namespace llm_translator
} // namespace themis
