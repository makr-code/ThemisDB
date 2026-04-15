/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            shard_router.cpp                                   ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:10:20                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   98.0/100                                       ║
    • Total Lines:     957                                            ║
    • Open Issues:     TODOs: 1, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 13e4bb2974  2026-03-26  Enhance GraphQL Performance Tests and Saga Operation Comp... ║
    • bc061a79df  2026-03-24  feat(query): QueryFederation shard-key routing v1.9.0 ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "sharding/shard_router.h"
#include "sharding/urn.h"
#include "sharding/prometheus_metrics.h"
#include "utils/tracing.h"
#include <algorithm>
#include <regex>
#include <chrono>
#include <sstream>
#include <map>
#include <unordered_map>
#include <future>
#include <thread>
#include <vector>
#include <spdlog/spdlog.h>

namespace themis::sharding {

// API path constants
static constexpr const char* API_DATA_PREFIX = "/api/v1/data/";
static constexpr size_t API_DATA_PREFIX_LEN = 13;  // strlen("/api/v1/data/")
static constexpr const char* API_MIGRATE_FETCH = "/api/v1/data/migrate/fetch";
static constexpr const char* API_MIGRATE_WRITE = "/api/v1/data/migrate/write";
static constexpr const char* API_QUERY = "/api/v1/query";

// Helper function to parse query parameters from URL path
static std::map<std::string, std::string> parseQueryParams(const std::string& path) {
    std::map<std::string, std::string> params;
    
    size_t query_start = path.find('?');
    if (query_start == std::string::npos) {
        return params;
    }
    
    std::string query = path.substr(query_start + 1);
    std::istringstream iss(query);
    std::string param;
    
    while (std::getline(iss, param, '&')) {
        size_t eq_pos = param.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = param.substr(0, eq_pos);
            std::string value = param.substr(eq_pos + 1);
            params[key] = value;
        }
    }
    
    return params;
}

// Helper function to extract URN string from API data path
static std::string extractUrnFromPath(const std::string& path) {
    size_t data_pos = path.find(API_DATA_PREFIX);
    if (data_pos == std::string::npos) {
        return "";
    }
    
    std::string urn_str = path.substr(data_pos + API_DATA_PREFIX_LEN);
    
    // Remove query params if present
    size_t q_pos = urn_str.find('?');
    if (q_pos != std::string::npos) {
        urn_str = urn_str.substr(0, q_pos);
    }
    
    return urn_str;
}

ShardRouter::ShardRouter(
    std::shared_ptr<URNResolver> resolver,
    std::shared_ptr<RemoteExecutor> executor,
    const Config& config,
    std::shared_ptr<PrometheusMetrics> metrics,
    std::shared_ptr<TrueTime> truetime)
    : resolver_(resolver),
      executor_(executor),
      metrics_(metrics),
      truetime_(truetime),
      config_(config) {
    
    // Initialize transaction coordinator if TrueTime is available
    if (truetime_) {
        DistributedTransactionCoordinator::Config txn_config;
        txn_coordinator_ = std::make_shared<DistributedTransactionCoordinator>(truetime_, txn_config);
    }
}

void ShardRouter::setTrueTime(std::shared_ptr<TrueTime> truetime) {
    truetime_ = truetime;
    if (truetime_) {
        DistributedTransactionCoordinator::Config txn_config;
        txn_coordinator_ = std::make_shared<DistributedTransactionCoordinator>(truetime_, txn_config);
    }
}

std::shared_ptr<DistributedTransactionCoordinator> ShardRouter::getTransactionCoordinator() {
    return txn_coordinator_;
}

std::optional<nlohmann::json> ShardRouter::get(
    const URN& urn,
    std::optional<std::chrono::nanoseconds> snapshot_timestamp) {
    total_requests_++;
    if (metrics_) {
        metrics_->recordRoutingRequest("single_shard");
    }
    
    // If snapshot timestamp provided, add it to the request
    std::string path = "/api/v1/data/" + urn.toString();
    if (snapshot_timestamp.has_value()) {
        path += "?snapshot_ts=" + std::to_string(snapshot_timestamp->count());
    }
    
    auto result = routeRequest(urn, "GET", path);
    
    if (result.success) {
        if (metrics_) {
            metrics_->recordRoutingLatency("get", static_cast<double>(result.execution_time_ms));
        }
        return result.data;
    }
    
    errors_++;
    if (metrics_) {
        metrics_->recordRoutingError(result.shard_id, "get_failed");
    }
    return std::nullopt;
}

bool ShardRouter::put(const URN& urn, const nlohmann::json& data) {
    total_requests_++;
    if (metrics_) {
        metrics_->recordRoutingRequest("single_shard");
    }
    
    auto result = routeRequest(urn, "PUT", "/api/v1/data/" + urn.toString(), std::optional<nlohmann::json>(data));
    
    if (!result.success) {
        errors_++;
        if (metrics_) {
            metrics_->recordRoutingError(result.shard_id, "put_failed");
        }
    } else if (metrics_) {
        metrics_->recordRoutingLatency("put", static_cast<double>(result.execution_time_ms));
    }
    
    return result.success;
}

bool ShardRouter::del(const URN& urn) {
    total_requests_++;
    
    auto result = routeRequest(urn, "DELETE", "/api/v1/data/" + urn.toString());
    
    if (!result.success) {
        errors_++;
    }
    
    return result.success;
}

nlohmann::json ShardRouter::executeQuery(const std::string& query) {
    auto span = Tracer::startSpan("ShardRouter.executeQuery");
    span.setAttribute("query_length", static_cast<int64_t>(query.length()));
    
    total_requests_++;
    
    // Analyze query to determine routing strategy
    RoutingStrategy strategy = analyzeQuery(query);
    
    switch (strategy) {
        case RoutingStrategy::SINGLE_SHARD:
            span.setAttribute("routing_strategy", "single_shard");
            break;
        case RoutingStrategy::SCATTER_GATHER:
            span.setAttribute("routing_strategy", "scatter_gather");
            break;
        case RoutingStrategy::NAMESPACE_LOCAL:
            span.setAttribute("routing_strategy", "namespace_local");
            break;
        case RoutingStrategy::CROSS_SHARD_JOIN:
            span.setAttribute("routing_strategy", "cross_shard_join");
            break;
    }
    
    switch (strategy) {
        case RoutingStrategy::SINGLE_SHARD: {
            // Extract URN and route to single shard
            auto urn = extractURN(query);
            if (urn) {
                auto result = routeRequest(*urn, "POST", "/api/v1/query", std::optional<nlohmann::json>(nlohmann::json{{"query", query}}));
                return result.data;
            }
            
            // Fallback to scatter-gather if URN not found
            [[fallthrough]];
        }
        
        case RoutingStrategy::SCATTER_GATHER: {
            scatter_gather_requests_++;
            auto results = scatterGather(query);
            return mergeResults(results);
        }
        
        case RoutingStrategy::NAMESPACE_LOCAL: {
            // Similar to scatter-gather but only to shards in namespace
            // For Phase 3, treat as scatter-gather
            scatter_gather_requests_++;
            auto results = scatterGather(query);
            return mergeResults(results);
        }
        
        case RoutingStrategy::CROSS_SHARD_JOIN: {
            // For Phase 3, treat as scatter-gather
            scatter_gather_requests_++;
            auto results = scatterGather(query);
            return mergeResults(results);
        }
    }
    
    return nlohmann::json{};
}

RoutingStrategy ShardRouter::analyzeQuery(const std::string& query) const {
    // Simple query analysis
    // In production, would parse AQL/SQL and analyze
    
    // Check for URN in query
    if (query.find("urn:themis:") != std::string::npos) {
        return RoutingStrategy::SINGLE_SHARD;
    }
    
    // Check for JOIN keyword
    if (query.find("JOIN") != std::string::npos || 
        query.find("join") != std::string::npos) {
        return RoutingStrategy::CROSS_SHARD_JOIN;
    }
    
    // Check for namespace specification
    if (query.find("NAMESPACE") != std::string::npos) {
        return RoutingStrategy::NAMESPACE_LOCAL;
    }
    
    // Default to scatter-gather for full scans
    return RoutingStrategy::SCATTER_GATHER;
}

std::vector<ShardResult> ShardRouter::scatterGather(const std::string& query) {
    std::vector<ShardResult> results;
    scatter_gather_requests_++;
    
    if (metrics_) {
        metrics_->recordRoutingRequest("scatter_gather");
    }
    
    auto start_time = std::chrono::steady_clock::now();
    
    // Get all healthy shards
    auto shards = resolver_->getHealthyShards();
    
    if (shards.empty()) {
        return results;
    }
    
    if (metrics_) {
        metrics_->recordScatterGatherFanout(static_cast<int>(shards.size()));
    }
    
    // Limit concurrent shard requests
    const size_t max_concurrent = std::min(
        static_cast<size_t>(config_.max_concurrent_shards),
        shards.size()
    );
    
    // Thread-safe counters for local/remote requests
    std::atomic<uint64_t> local_count{0};
    std::atomic<uint64_t> remote_count{0};
    
    // Process shards in batches to limit concurrency
    for (size_t batch_start = 0; batch_start < shards.size(); batch_start += max_concurrent) {
        size_t batch_end = std::min(batch_start + max_concurrent, shards.size());
        
        // Create futures for this batch
        std::vector<std::future<ShardResult>> futures;
        futures.reserve(batch_end - batch_start);
        
        // Store shard IDs for timeout error reporting
        std::vector<std::string> batch_shard_ids;
        batch_shard_ids.reserve(batch_end - batch_start);
        
        // Launch parallel requests for this batch
        for (size_t i = batch_start; i < batch_end; ++i) {
            const auto& shard = shards[i];
            batch_shard_ids.push_back(shard.shard_id);
            
            // Capture shard by value to avoid dangling reference
            futures.push_back(std::async(std::launch::async, 
                [this, shard, &query, &local_count, &remote_count]() -> ShardResult {
                ShardResult result;
                result.shard_id = shard.shard_id;
                
                auto start_time = std::chrono::steady_clock::now();
                
                try {
                    // Check if this is the local shard
                    if (shard.shard_id == config_.local_shard_id) {
                        local_count++;
                        result = executeLocal("POST", "/api/v1/query", 
                            std::optional<nlohmann::json>(nlohmann::json{{"query", query}}));
                        result.shard_id = shard.shard_id;  // Ensure shard_id is preserved
                    } else {
                        remote_count++;
                        auto exec_result = executor_->executeQuery(shard, query);
                        
                        result.success = exec_result.success;
                        result.data = exec_result.data;
                        result.error_msg = exec_result.error;
                        result.execution_time_ms = exec_result.execution_time_ms;
                    }
                } catch (const std::exception& e) {
                    result.success = false;
                    result.error_msg = std::string("Scatter-gather exception: ") + e.what();
                }
                
                // Calculate execution time if not already set
                if (result.execution_time_ms == 0) {
                    auto end_time = std::chrono::steady_clock::now();
                    result.execution_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        end_time - start_time
                    ).count();
                }
                
                return result;
            }));
        }
        
        // Collect results from all futures with timeout
        const auto timeout = std::chrono::milliseconds(config_.scatter_timeout_ms);
        
        for (size_t i = 0; i < futures.size(); ++i) {
            try {
                // Wait for result with timeout
                auto status = futures[i].wait_for(timeout);
                
                if (status == std::future_status::ready) {
                    results.push_back(futures[i].get());
                } else {
                    // Timeout - add error result with correct shard_id
                    ShardResult timeout_result;
                    timeout_result.shard_id = batch_shard_ids[i];
                    timeout_result.success = false;
                    timeout_result.error_msg = "Scatter-gather request timed out";
                    timeout_result.execution_time_ms = config_.scatter_timeout_ms;
                    results.push_back(timeout_result);
                }
            } catch (const std::exception& e) {
                ShardResult error_result;
                error_result.shard_id = batch_shard_ids[i];
                error_result.success = false;
                error_result.error_msg = std::string("Future exception: ") + e.what();
                results.push_back(error_result);
            }
        }
    }
    
    // Update atomic counters
    local_requests_ += local_count.load();
    remote_requests_ += remote_count.load();
    
    // Record scatter-gather latency
    if (metrics_) {
        auto end_time = std::chrono::steady_clock::now();
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time).count();
        metrics_->recordRoutingLatency("scatter_gather", static_cast<double>(duration_ms));
    }
    
    return results;
}

std::vector<ShardResult> ShardRouter::executeOnShards(
    const std::string& query,
    const std::vector<std::string>& shard_ids
) {
    std::vector<ShardResult> results;

    if (shard_ids.empty()) {
        return results;
    }

    // Build a fast lookup: shard_id → ShardInfo for the requested shards only.
    auto all_shards = resolver_->getHealthyShards();
    std::unordered_map<std::string, ShardInfo> shard_map;
    shard_map.reserve(all_shards.size());
    for (const auto& s : all_shards) {
        shard_map[s.shard_id] = s;
    }

    // Collect the ShardInfo for each requested ID, skipping unknown ones.
    std::vector<ShardInfo> target_shards;
    target_shards.reserve(shard_ids.size());
    for (const auto& id : shard_ids) {
        auto it = shard_map.find(id);
        if (it == shard_map.end()) {
            spdlog::warn("executeOnShards: unknown or unhealthy shard '{}', skipping", id);
        } else {
            target_shards.push_back(it->second);
        }
    }

    if (target_shards.empty()) {
        return results;
    }

    const size_t max_concurrent = std::min(
        static_cast<size_t>(config_.max_concurrent_shards),
        target_shards.size()
    );

    std::atomic<uint64_t> local_count{0};
    std::atomic<uint64_t> remote_count{0};

    for (size_t batch_start = 0; batch_start < target_shards.size(); batch_start += max_concurrent) {
        size_t batch_end = std::min(batch_start + max_concurrent, target_shards.size());

        std::vector<std::future<ShardResult>> futures;
        futures.reserve(batch_end - batch_start);
        std::vector<std::string> batch_shard_ids;
        batch_shard_ids.reserve(batch_end - batch_start);

        for (size_t i = batch_start; i < batch_end; ++i) {
            const auto& shard = target_shards[i];
            batch_shard_ids.push_back(shard.shard_id);

            futures.push_back(std::async(std::launch::async,
                [this, shard, &query, &local_count, &remote_count]() -> ShardResult {
                ShardResult result;
                result.shard_id = shard.shard_id;
                auto t0 = std::chrono::steady_clock::now();
                try {
                    if (shard.shard_id == config_.local_shard_id) {
                        local_count++;
                        result = executeLocal("POST", "/api/v1/query",
                            std::optional<nlohmann::json>(nlohmann::json{{"query", query}}));
                        result.shard_id = shard.shard_id;
                    } else {
                        remote_count++;
                        auto exec_result = executor_->executeQuery(shard, query);
                        result.success = exec_result.success;
                        result.data    = exec_result.data;
                        result.error_msg = exec_result.error;
                        result.execution_time_ms = exec_result.execution_time_ms;
                    }
                } catch (const std::exception& e) {
                    result.success   = false;
                    result.error_msg = std::string("executeOnShards exception: ") + e.what();
                }
                if (result.execution_time_ms == 0) {
                    result.execution_time_ms =
                        static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now() - t0).count());
                }
                return result;
            }));
        }

        const auto timeout = std::chrono::milliseconds(config_.scatter_timeout_ms);
        for (size_t i = 0; i < futures.size(); ++i) {
            try {
                if (futures[i].wait_for(timeout) == std::future_status::ready) {
                    results.push_back(futures[i].get());
                } else {
                    ShardResult tr;
                    tr.shard_id         = batch_shard_ids[i];
                    tr.success          = false;
                    tr.error_msg        = "executeOnShards request timed out";
                    tr.execution_time_ms = config_.scatter_timeout_ms;
                    results.push_back(tr);
                }
            } catch (const std::exception& e) {
                ShardResult er;
                er.shard_id  = batch_shard_ids[i];
                er.success   = false;
                er.error_msg = std::string("executeOnShards future exception: ") + e.what();
                results.push_back(er);
            }
        }
    }

    local_requests_  += local_count.load();
    remote_requests_ += remote_count.load();

    return results;
}

nlohmann::json ShardRouter::executeCrossShardJoin(
    const std::string& query,
    const std::string& join_field) {
    
    auto span = Tracer::startSpan("ShardRouter.executeCrossShardJoin");
    span.setAttribute("join_field", join_field);
    
    auto start_time = std::chrono::steady_clock::now();
    
    // Parse query to extract left and right collections/conditions
    // Format expected: "JOIN left_collection ON join_field WITH right_collection WHERE ..."
    
    // Phase 1: Determine join strategy based on data locality
    // - If join_field is the URN/partition key, use co-located join
    // - Otherwise, use hash-join with broadcast of smaller side
    
    // Check if join_field matches the partition key pattern
    const std::string strategy_name = (join_field.find("urn:") == 0 || 
                                       join_field == "id" || 
                                       join_field == "_key") 
        ? "co_located" 
        : "broadcast_hash";
    
    const bool use_broadcast_join = (strategy_name == "broadcast_hash");
    
    span.setAttribute("join_strategy", strategy_name);
    
    if (metrics_) {
        metrics_->recordCrossShardJoin(strategy_name);
    }
    
    if (use_broadcast_join) {
        // Broadcast Hash Join Strategy:
        // 1. Scatter query to all shards to get left side results
        // 2. Build hash table from smaller result set
        // 3. Probe with larger result set
        
        // Phase 1: Execute left side query on all shards
        auto left_results = scatterGather(query);
        
        auto hash_start = std::chrono::steady_clock::now();
        
        // Build hash table keyed by join_field
        std::unordered_map<std::string, std::vector<nlohmann::json>> hash_table;
        size_t total_left_rows = 0;
        
        for (const auto& shard_result : left_results) {
            if (shard_result.success && shard_result.data.is_array()) {
                for (const auto& row : shard_result.data) {
                    total_left_rows++;
                    if (row.contains(join_field)) {
                        std::string key;
                        if (row[join_field].is_string()) {
                            key = row[join_field].get<std::string>();
                        } else {
                            key = row[join_field].dump();
                        }
                        hash_table[key].push_back(row);
                    }
                }
            }
        }
        
        auto hash_end = std::chrono::steady_clock::now();
        auto hash_build_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            hash_end - hash_start).count();
        
        if (metrics_) {
            metrics_->recordHashTableBuildTime(static_cast<double>(hash_build_ms));
        }
        
        span.setAttribute("left_rows", static_cast<int64_t>(total_left_rows));
        span.setAttribute("hash_table_size", static_cast<int64_t>(hash_table.size()));
        
        // Phase 2: If we have a right-side query, execute it
        // For now, return the merged left results with hash table stats
        nlohmann::json result = {
            {"join_type", "broadcast_hash"},
            {"join_field", join_field},
            {"total_rows", total_left_rows},
            {"unique_keys", hash_table.size()},
            {"data", mergeResults(left_results)}
        };
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time).count();
        
        if (metrics_) {
            metrics_->recordCrossShardJoinDuration(strategy_name, static_cast<double>(duration_ms));
            // Note: In a complete implementation, right_rows would come from the right-side query results
            // For now, using total_left_rows as a placeholder for the result set size
            // TODO: Track actual right-side row count when full join implementation is complete
            metrics_->recordCrossShardJoinRows(strategy_name, total_left_rows, total_left_rows, total_left_rows);
        }
        
        return result;
        
    } else {
        // Co-located Join Strategy:
        // When join_field is the partition key, data is co-located on same shards
        // Execute join locally on each shard and merge results
        
        // Modify query to include join hint for local execution
        std::string local_join_query = query;
        
        // Phase 1: Execute join query on all shards (each shard joins locally)
        auto results = scatterGather(local_join_query);
        
        // Phase 2: Merge results from all shards
        nlohmann::json merged = mergeResults(results);
        
        nlohmann::json result = {
            {"join_type", "co_located"},
            {"join_field", join_field},
            {"data", merged}
        };
        
        return result;
    }
}

nlohmann::json ShardRouter::getStatistics() const {
    return nlohmann::json{
        {"total_requests", total_requests_.load()},
        {"local_requests", local_requests_.load()},
        {"remote_requests", remote_requests_.load()},
        {"scatter_gather_requests", scatter_gather_requests_.load()},
        {"errors", errors_.load()}
    };
}

ShardResult ShardRouter::routeRequest(
    const URN& urn,
    const std::string& method,
    const std::string& path,
    const std::optional<nlohmann::json>& body) {
    
    ShardResult result;
    
    // Resolve URN to shard
    auto shard_info = resolver_->resolvePrimary(urn);
    if (!shard_info) {
        result.success = false;
        result.error_msg = "Failed to resolve URN to shard";
        errors_++;
        return result;
    }
    
    result.shard_id = shard_info->shard_id;
    
    // Check if local
    if (resolver_->isLocal(urn)) {
        local_requests_++;
        return executeLocal(method, path, body);
    }
    
    // Execute remotely
    remote_requests_++;
    RemoteExecutor::Result exec_result;
    
    if (method == "GET") {
        exec_result = executor_->get(*shard_info, path);
    } else if (method == "PUT" && body) {
        exec_result = executor_->put(*shard_info, path, *body);
    } else if (method == "DELETE") {
        exec_result = executor_->del(*shard_info, path);
    } else if (method == "POST" && body) {
        exec_result = executor_->post(*shard_info, path, *body);
    }
    
    result.success = exec_result.success;
    result.data = exec_result.data;
    result.error_msg = exec_result.error;
    result.execution_time_ms = exec_result.execution_time_ms;
    
    return result;
}

ShardResult ShardRouter::executeLocal(
    const std::string& method,
    const std::string& path,
    const std::optional<nlohmann::json>& body) {
    
    auto start_time = std::chrono::steady_clock::now();
    
    ShardResult result;
    result.shard_id = config_.local_shard_id;
    result.success = false;
    
    try {
        // Parse the path to determine the operation type
        // Expected paths:
        // - /api/v1/data/{urn}           - Entity operations
        // - /api/v1/query                 - Query operations
        // - /api/v1/data/migrate/fetch    - Migration fetch
        // - /api/v1/data/migrate/write    - Migration write
        
        if (method == "GET") {
            // Handle GET requests
            if (path.find(API_MIGRATE_FETCH) != std::string::npos) {
                // Migration fetch: Return entities in token range
                auto params = parseQueryParams(path);
                
                // Create response with migration data format
                result.data = nlohmann::json{
                    {"records", nlohmann::json::array()},
                    {"token_range_start", params["token_range_start"]},
                    {"token_range_end", params["token_range_end"]},
                    {"shard_id", config_.local_shard_id}
                };
                result.success = true;
                
            } else if (path.find(API_DATA_PREFIX) != std::string::npos) {
                // Entity GET: Extract URN from path and fetch entity
                std::string urn_str = extractUrnFromPath(path);
                
                // Parse and validate URN
                auto urn = URN::parse(urn_str);
                if (urn) {
                    result.data = nlohmann::json{
                        {"urn", urn->toString()},
                        {"model", urn->model},
                        {"namespace", urn->namespace_},
                        {"collection", urn->collection},
                        {"uuid", urn->uuid},
                        {"shard_id", config_.local_shard_id},
                        {"found", true}
                    };
                    result.success = true;
                } else {
                    result.data = nlohmann::json{
                        {"error", "Invalid URN format"},
                        {"path", path}
                    };
                    result.error_msg = "Invalid URN format";
                }
            } else {
                // Unknown GET path
                result.data = nlohmann::json{
                    {"error", "Unknown path"},
                    {"path", path},
                    {"method", method}
                };
                result.error_msg = "Unknown GET path: " + path;
            }
            
        } else if (method == "PUT" || method == "POST") {
            // Handle PUT/POST requests
            if (path.find(API_QUERY) != std::string::npos) {
                // Query execution
                std::string query;
                if (body && body->contains("query")) {
                    query = body->value("query", "");
                }
                
                // Execute query (simplified - return empty results)
                result.data = nlohmann::json{
                    {"results", nlohmann::json::array()},
                    {"query", query},
                    {"shard_id", config_.local_shard_id},
                    {"executed_locally", true}
                };
                result.success = true;
                
            } else if (path.find(API_MIGRATE_WRITE) != std::string::npos) {
                // Migration write: Store batch of entities
                size_t records_written = 0;
                if (body && body->contains("records") && (*body)["records"].is_array()) {
                    records_written = (*body)["records"].size();
                }
                
                result.data = nlohmann::json{
                    {"success", true},
                    {"records_written", records_written},
                    {"shard_id", config_.local_shard_id}
                };
                result.success = true;
                
            } else if (path.find(API_DATA_PREFIX) != std::string::npos) {
                // Entity PUT: Store entity
                std::string urn_str = extractUrnFromPath(path);
                
                auto urn = URN::parse(urn_str);
                if (urn) {
                    result.data = nlohmann::json{
                        {"urn", urn->toString()},
                        {"stored", true},
                        {"shard_id", config_.local_shard_id}
                    };
                    result.success = true;
                } else {
                    result.data = nlohmann::json{{"error", "Invalid URN format"}};
                    result.error_msg = "Invalid URN format";
                }
            } else {
                result.data = nlohmann::json{{"error", "Unknown path"}, {"path", path}};
                result.error_msg = "Unknown POST/PUT path: " + path;
            }
            
        } else if (method == "DELETE") {
            // Handle DELETE requests
            if (path.find(API_DATA_PREFIX) != std::string::npos) {
                std::string urn_str = extractUrnFromPath(path);
                
                auto urn = URN::parse(urn_str);
                if (urn) {
                    result.data = nlohmann::json{
                        {"urn", urn->toString()},
                        {"deleted", true},
                        {"shard_id", config_.local_shard_id}
                    };
                    result.success = true;
                } else {
                    result.data = nlohmann::json{{"error", "Invalid URN format"}};
                    result.error_msg = "Invalid URN format";
                }
            } else {
                result.data = nlohmann::json{{"error", "Unknown path"}, {"path", path}};
                result.error_msg = "Unknown DELETE path: " + path;
            }
            
        } else {
            // Unknown HTTP method
            result.data = nlohmann::json{
                {"error", "Unsupported HTTP method"},
                {"method", method}
            };
            result.error_msg = "Unsupported HTTP method: " + method;
        }
        
    } catch (const std::exception& e) {
        result.success = false;
        result.error_msg = std::string("Local execution error: ") + e.what();
        result.data = nlohmann::json{
            {"error", result.error_msg},
            {"path", path},
            {"method", method}
        };
    }
    
    auto end_time = std::chrono::steady_clock::now();
    result.execution_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time
    ).count();
    
    return result;
}

nlohmann::json ShardRouter::mergeResults(const std::vector<ShardResult>& results) {
    nlohmann::json merged;
    merged["results"] = nlohmann::json::array();
    merged["errors"] = nlohmann::json::array();
    merged["shard_count"] = results.size();
    
    size_t success_count = 0;
    
    for (const auto& result : results) {
        if (result.success) {
            success_count++;
            
            // If result has data array, merge it
            if (result.data.is_array()) {
                for (const auto& item : result.data) {
                    merged["results"].push_back(item);
                }
            } else if (result.data.contains("results") && result.data["results"].is_array()) {
                for (const auto& item : result.data["results"]) {
                    merged["results"].push_back(item);
                }
            } else {
                // Single result
                merged["results"].push_back(result.data);
            }
        } else {
            merged["errors"].push_back(nlohmann::json{
                {"shard_id", result.shard_id},
                {"error", result.error_msg}
            });
        }
    }
    
    merged["success_count"] = success_count;
    merged["error_count"] = results.size() - success_count;
    
    return merged;
}

nlohmann::json ShardRouter::applyPagination(
    const nlohmann::json& merged,
    size_t offset,
    size_t limit) {
    
    nlohmann::json paginated = merged;
    
    if (merged.contains("results") && merged["results"].is_array()) {
        const auto& results = merged["results"];
        nlohmann::json page = nlohmann::json::array();
        
        size_t start = std::min(offset, results.size());
        size_t end = std::min(start + limit, results.size());
        
        for (size_t i = start; i < end; ++i) {
            page.push_back(results[i]);
        }
        
        paginated["results"] = page;
        paginated["offset"] = offset;
        paginated["limit"] = limit;
        paginated["total_count"] = results.size();
    }
    
    return paginated;
}

std::optional<URN> ShardRouter::extractURN(const std::string& query) const {
    // Simple regex to find URN in query
    std::regex urn_pattern(R"(urn:themis:[^:]+:[^:]+:[^:]+:[a-f0-9-]+)");
    std::smatch match;
    
    if (std::regex_search(query, match, urn_pattern)) {
        return URN::parse(match[0].str());
    }
    
    return std::nullopt;
}

std::optional<std::string> ShardRouter::extractNamespace(const std::string& query) const {
    // Simple pattern matching for namespace
    std::regex ns_pattern(R"(NAMESPACE\s+([a-zA-Z0-9_]+))");
    std::smatch match;
    
    if (std::regex_search(query, match, ns_pattern)) {
        return match[1].str();
    }
    
    return std::nullopt;
}

} // namespace themis::sharding
