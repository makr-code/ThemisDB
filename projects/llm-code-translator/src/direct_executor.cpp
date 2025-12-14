#include "direct_executor.h"
#include <chrono>
#include <algorithm>
#include <sstream>
#include <cmath>

namespace llm_code_translator {

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
        if (!metrics_enabled) return;
        
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
    ResourceLimits default_limits;
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
    
    // Apply sorting
    if (!plan.sorting.empty()) {
        data = applySorting(data, plan.sorting);
    }
    
    // Apply pagination
    int limit = plan.parameters.value("limit", -1);
    int offset = plan.parameters.value("offset", 0);
    if (limit > 0 || offset > 0) {
        data = applyPagination(data, limit, offset);
    }
    
    result.success = true;
    result.result_data = data;
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
    
    // Group by fields
    std::vector<std::string> group_by = plan.parameters.value("groupBy", std::vector<std::string>());
    nlohmann::json aggregations = plan.parameters.value("aggregations", nlohmann::json::array());
    
    // Simplified aggregation logic (real implementation would be more sophisticated)
    if (group_by.empty()) {
        // No grouping - aggregate all rows
        nlohmann::json agg_result;
        for (const auto& agg : aggregations) {
            std::string function = agg.value("function", "");
            std::string field = agg.value("field", "");
            std::string as = agg.value("as", field + "_" + function);
            
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
        result.result_data = nlohmann::json::array({agg_result});
    } else {
        // With grouping - simplified version
        result.result_data = nlohmann::json::array();
    }
    
    result.success = true;
    result.rows_affected = result.result_data.size();
    
    return result;
}

ExecutionResult DirectExecutor::executeTransform(const ExecutionPlan& plan, const ResourceLimits& limits) {
    ExecutionResult result;
    
    nlohmann::json data = impl_->db->scan(plan.datasource);
    
    // Apply filters
    if (!plan.filters.empty()) {
        data = applyFilters(data, plan.filters);
    }
    
    // Apply transformations (simplified)
    nlohmann::json transformations = plan.parameters.value("transformations", nlohmann::json::array());
    
    result.success = true;
    result.result_data = data;
    result.rows_affected = data.is_array() ? data.size() : 1;
    
    return result;
}

ExecutionResult DirectExecutor::executeJoin(const ExecutionPlan& plan, const ResourceLimits& limits) {
    ExecutionResult result;
    result.success = false;
    result.error_message = "JOIN operation not yet implemented";
    return result;
}

ExecutionResult DirectExecutor::executeGraphTraverse(const ExecutionPlan& plan, const ResourceLimits& limits) {
    ExecutionResult result;
    
    nlohmann::json start_node = plan.parameters.value("start_node", nlohmann::json());
    std::string traversal_type = plan.parameters.value("traversal_type", "BFS");
    int max_depth = plan.parameters.value("max_depth", 10);
    
    nlohmann::json graph_data = impl_->db->graphTraverse(plan.datasource, start_node, traversal_type, max_depth);
    
    result.success = true;
    result.result_data = graph_data;
    result.rows_affected = graph_data.is_array() ? graph_data.size() : 1;
    
    return result;
}

ExecutionResult DirectExecutor::executeVectorSearch(const ExecutionPlan& plan, const ResourceLimits& limits) {
    ExecutionResult result;
    
    std::vector<float> query_vector = plan.parameters.value("query_vector", std::vector<float>());
    int top_k = plan.parameters.value("top_k", 10);
    std::string distance_metric = plan.parameters.value("distance_metric", "cosine");
    
    nlohmann::json search_results = impl_->db->vectorSearch(plan.datasource, query_vector, top_k, distance_metric);
    
    result.success = true;
    result.result_data = search_results;
    result.rows_affected = search_results.is_array() ? search_results.size() : 1;
    
    return result;
}

ExecutionResult DirectExecutor::executeTimeSeries(const ExecutionPlan& plan, const ResourceLimits& limits) {
    ExecutionResult result;
    
    std::string start_time = plan.parameters.value("start_time", "");
    std::string end_time = plan.parameters.value("end_time", "");
    std::string aggregation = plan.parameters.value("aggregation", "none");
    
    nlohmann::json ts_data = impl_->db->timeSeriesQuery(plan.datasource, start_time, end_time, aggregation);
    
    result.success = true;
    result.result_data = ts_data;
    result.rows_affected = ts_data.is_array() ? ts_data.size() : 1;
    
    return result;
}

ExecutionResult DirectExecutor::executeMutation(const ExecutionPlan& plan, const ResourceLimits& limits) {
    ExecutionResult result;
    result.success = false;
    result.error_message = "MUTATION operation not yet implemented";
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
            
            if (filter.op == "==") {
                if (value != filter.value) matches = false;
            } else if (filter.op == "!=") {
                if (value == filter.value) matches = false;
            } else if (filter.op == ">") {
                if (value.is_number() && filter.value.is_number()) {
                    if (value.get<double>() <= filter.value.get<double>()) matches = false;
                }
            } else if (filter.op == ">=") {
                if (value.is_number() && filter.value.is_number()) {
                    if (value.get<double>() < filter.value.get<double>()) matches = false;
                }
            } else if (filter.op == "<") {
                if (value.is_number() && filter.value.is_number()) {
                    if (value.get<double>() >= filter.value.get<double>()) matches = false;
                }
            } else if (filter.op == "<=") {
                if (value.is_number() && filter.value.is_number()) {
                    if (value.get<double>() > filter.value.get<double>()) matches = false;
                }
            }
            
            if (!matches) break;
        }
        
        if (matches) {
            filtered.push_back(row);
        }
    }
    
    return filtered;
}

nlohmann::json DirectExecutor::applySorting(const nlohmann::json& data, 
                                            const std::vector<SortOrder>& sorting) {
    if (!data.is_array() || sorting.empty()) {
        return data;
    }
    
    nlohmann::json sorted = data;
    
    // Simple sorting implementation (production would be more efficient)
    std::sort(sorted.begin(), sorted.end(), [&sorting](const nlohmann::json& a, const nlohmann::json& b) {
        for (const auto& sort : sorting) {
            if (!a.contains(sort.field) || !b.contains(sort.field)) {
                continue;
            }
            
            auto a_val = a[sort.field];
            auto b_val = b[sort.field];
            
            if (a_val != b_val) {
                bool less_than = false;
                if (a_val.is_number() && b_val.is_number()) {
                    less_than = a_val.get<double>() < b_val.get<double>();
                } else if (a_val.is_string() && b_val.is_string()) {
                    less_than = a_val.get<std::string>() < b_val.get<std::string>();
                }
                
                return sort.ascending ? less_than : !less_than;
            }
        }
        return false;
    });
    
    return sorted;
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

// MockDatabase implementation
MockDatabase::MockDatabase() {
    loadSampleData();
}

void MockDatabase::loadSampleData() {
    // Sample sensor data
    data_["sensor_readings"] = nlohmann::json::array({
        {{"sensor_id", "S001"}, {"temperature", 67.3}, {"timestamp", "2024-12-14T10:00:00Z"}},
        {{"sensor_id", "S002"}, {"temperature", 45.2}, {"timestamp", "2024-12-14T10:00:00Z"}},
        {{"sensor_id", "S042"}, {"temperature", 52.1}, {"timestamp", "2024-12-14T10:00:00Z"}},
        {{"sensor_id", "S001"}, {"temperature", 65.8}, {"timestamp", "2024-12-14T11:00:00Z"}},
        {{"sensor_id", "S042"}, {"temperature", 54.7}, {"timestamp", "2024-12-14T11:00:00Z"}}
    });
}

nlohmann::json MockDatabase::scan(const std::string& datasource) {
    if (data_.find(datasource) != data_.end()) {
        return data_[datasource];
    }
    return nlohmann::json::array();
}

nlohmann::json MockDatabase::get(const std::string& datasource, const nlohmann::json& key) {
    return nlohmann::json();
}

bool MockDatabase::put(const std::string& datasource, const nlohmann::json& key, const nlohmann::json& value) {
    return true;
}

bool MockDatabase::del(const std::string& datasource, const nlohmann::json& key) {
    return true;
}

std::vector<nlohmann::json> MockDatabase::multiGet(const std::string& datasource, 
                                                     const std::vector<nlohmann::json>& keys) {
    return std::vector<nlohmann::json>();
}

nlohmann::json MockDatabase::indexScan(const std::string& datasource, 
                                        const std::string& index_name,
                                        const nlohmann::json& start_key,
                                        const nlohmann::json& end_key) {
    return nlohmann::json::array();
}

nlohmann::json MockDatabase::graphTraverse(const std::string& datasource,
                                           const nlohmann::json& start_node,
                                           const std::string& traversal_type,
                                           int max_depth) {
    return nlohmann::json::array();
}

nlohmann::json MockDatabase::vectorSearch(const std::string& datasource,
                                          const std::vector<float>& query_vector,
                                          int top_k,
                                          const std::string& distance_metric) {
    return nlohmann::json::array();
}

nlohmann::json MockDatabase::timeSeriesQuery(const std::string& datasource,
                                             const std::string& start_time,
                                             const std::string& end_time,
                                             const std::string& aggregation) {
    return nlohmann::json::array();
}

} // namespace llm_code_translator
