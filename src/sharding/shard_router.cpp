/**
 * @file shard_router.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=23, H=31, M=12, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "sharding/shard_router.h"
#include "sharding/urn.h"
#include "sharding/prometheus_metrics.h"
#include "utils/tracing.h"
#include "utils/logger.h"
#include <algorithm>
#include <regex>
#include <chrono>
#include <limits>
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

namespace {

// W5-Sharding: Atomic version counter for monotonic merged version tokens
// Ensures version tokens across shard boundaries are monotonically increasing
// Format: [48-bit timestamp (microseconds) | 16-bit counter]
static std::atomic<uint16_t> g_merge_version_counter{0};

uint64_t makeMergeVersionToken() {
    // Get current timestamp in microseconds (48 bits)
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    
    // Increment counter (16 bits) - wraps around at 65536
    uint16_t counter = g_merge_version_counter.fetch_add(1, std::memory_order_relaxed);
    
    // Combine: upper 48 bits = timestamp, lower 16 bits = counter
    // This gives us monotonic ordering within a process and temporal ordering across shards
    return (static_cast<uint64_t>(micros) << 16) | counter;
}

uint64_t extractVersionToken(const nlohmann::json& payload) {
    if (payload.is_object()) {
        for (const char* key : {"mergeVersion", "version_token", "versionToken", "version"}) {
            auto it = payload.find(key);
            if (it != payload.end() && it->is_number_unsigned()) {
                return it->get<uint64_t>();
            }
            if (it != payload.end() && it->is_number_integer()) {
                const auto value = it->get<int64_t>();
                if (value >= 0) {
                    return static_cast<uint64_t>(value);
                }
            }
        }

        uint64_t nested_version = 0;
        for (const auto& [_, value] : payload.items()) {
            nested_version = std::max(nested_version, extractVersionToken(value));
        }
        return nested_version;
    }

    if (payload.is_array()) {
        uint64_t nested_version = 0;
        for (const auto& item : payload) {
            nested_version = std::max(nested_version, extractVersionToken(item));
        }
        return nested_version;
    }

    return 0;
}

uint64_t resolveShardResultVersion(const ShardResult& result) {
    return std::max(result.version_token, extractVersionToken(result.data));
}

uint64_t makeStrictMergeVersionToken(uint64_t observed_max_version) {
    const uint64_t candidate = makeMergeVersionToken();
    if (observed_max_version == std::numeric_limits<uint64_t>::max()) {
        return observed_max_version;
    }
    return std::max(candidate, observed_max_version + 1);
}

} // namespace

/** @brief Parse URL query string (`?a=b&c=d`) into key/value map. */
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

/** @brief Extract URN suffix from `/api/v1/data/<urn>` style path. */
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

/**
 * @brief Construct shard router facade and optional transaction coordinator.
 * @param resolver Shard mapping resolver.
 * @param executor Remote execution adapter.
 * @param config Routing configuration.
 * @param metrics Optional metrics collector.
 * @param truetime Optional TrueTime source enabling distributed transactions.
 */
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

/** @brief Replace TrueTime source and (re)create distributed txn coordinator if available. */
void ShardRouter::setTrueTime(std::shared_ptr<TrueTime> truetime) {
    truetime_ = truetime;
    if (truetime_) {
        DistributedTransactionCoordinator::Config txn_config;
        txn_coordinator_ = std::make_shared<DistributedTransactionCoordinator>(truetime_, txn_config);
    }
}

/** @brief Return current distributed transaction coordinator instance. */
std::shared_ptr<DistributedTransactionCoordinator> ShardRouter::getTransactionCoordinator() {
    return txn_coordinator_;
}

/**
 * @brief Route point read request by URN and return document payload on success.
 *
 * This method routes a GET request for a specific entity identified by its URN to the appropriate shard.
 * It supports snapshot reads when a timestamp is provided, enabling MVCC (Multi-Version Concurrency Control).
 *
 * @param urn The unique resource identifier (URN) of the entity to retrieve.
 * @param snapshot_timestamp Optional timestamp for reading from a specific point-in-time snapshot.
 *                           If provided, the read will be consistent with that snapshot.
 * @return std::optional<nlohmann::json> The retrieved document payload if successful, or std::nullopt on failure.
 *
 * @note This method is part of the single-shard routing strategy.
 * @note W2-S07: Read consistency model
 *       - Default: Read from primary shard (eventual consistency)
 *       - With snapshot_timestamp: Read from specified snapshot (MVCC)
 *       - No quorum checking: Primary shard is source of truth for consistency
 */
std::optional<nlohmann::json> ShardRouter::get(
    const URN& urn,
    std::optional<std::chrono::nanoseconds> snapshot_timestamp) {
    total_requests_++;
    if (metrics_) {
        metrics_->recordRoutingRequest("single_shard");
    }
    // - Default: Read from primary shard (eventual consistency)
    // - With snapshot_timestamp: Read from specified snapshot (MVCC)
    // - No quorum checking: Primary shard is source of truth for consistency
    
    // If snapshot timestamp provided, add it to the request
    std::string path = "/api/v1/data/" + urn.toString();
    if (snapshot_timestamp.has_value()) {
        path += "?snapshot_ts=" + std::to_string(snapshot_timestamp->count());
    }
    
    auto result = routeRequest(urn, "GET", path);
    
    if (result.success) {
        if (metrics_) {
            metrics_->recordRoutingLatency("get", static_cast<double>(result.execution_time_ms));
            metrics_->recordCrossShardRequest(result.shard_id, "route", "success");
        }
        return result.data;
    }
    
    errors_++;
    if (metrics_) {
        metrics_->recordRoutingError(result.shard_id, "get_failed");
        metrics_->recordCrossShardRequest(result.shard_id, "route", "error");
    }
    return std::nullopt;
}

/**
 * @brief Route point write request by URN to owning shard.
 *
 * This method routes a PUT request for a specific entity identified by its URN to the appropriate shard.
 * The write is forwarded to the primary shard of the entity's partition.
 *
 * @param urn The unique resource identifier (URN) of the entity to update.
 * @param data The JSON payload containing the new data for the entity.
 * @return bool True if the write was successful, false otherwise.
 *
 * @note This method is part of the single-shard routing strategy.
 * @note W2-S07: Write consistency model
 *       - Primary shard: Write forwarded to primary after hashing
 *       - Replication: Async replication to replicas (not part of this call)
 *       - Durability: Write-through to primary's WAL
 *       - Atomicity: Single shard write is atomic; multi-shard requires 2PC
 */
bool ShardRouter::put(const URN& urn, const nlohmann::json& data) {
    total_requests_++;
    if (metrics_) {
        metrics_->recordRoutingRequest("single_shard");
    }
    
    // W2-S07: Write consistency model
    // - Primary shard: Write forwarded to primary after hashing
    // - Replication: Async replication to replicas (not part of this call)
    // - Durability: Write-through to primary's WAL
    // - Atomicity: Single shard write is atomic; multi-shard requires 2PC
    
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

/**
 * @brief Route point delete request by URN to owning shard.
 *
 * This method routes a DELETE request for a specific entity identified by its URN to the appropriate shard.
 * The deletion is forwarded to the primary shard of the entity's partition.
 *
 * @param urn The unique resource identifier (URN) of the entity to delete.
 * @return bool True if the deletion was successful, false otherwise.
 *
 * @note This method is part of the single-shard routing strategy.
 */
bool ShardRouter::del(const URN& urn) {
    total_requests_++;
    
    auto result = routeRequest(urn, "DELETE", "/api/v1/data/" + urn.toString());
    
    if (!result.success) {
        errors_++;
    }
    
    return result.success;
}

/**
 * @brief Execute query using selected routing strategy.
 *
 * This method routes a query to the appropriate shards based on its content.
 * It determines the routing strategy (single-shard, scatter-gather, namespace-local, or cross-shard join)
 * and executes the query accordingly.
 *
 * @param query Query text (e.g., AQL query string).
 * @return nlohmann::json JSON payload merged from one or more shard responses.
 *
 * @note The routing strategy is determined by analyzing the query content:
 *       - SINGLE_SHARD: Queries containing URN identifiers
 *       - SCATTER_GATHER: Full table scans or queries without specific shard hints
 *       - NAMESPACE_LOCAL: Queries scoped to a specific namespace
 *       - CROSS_SHARD_JOIN: Queries with JOIN operations across shards
 */
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

/** @brief Classify query into single-shard, scatter, namespace-local, or join strategy. */
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

/**
 * @brief Execute scatter-gather request across current healthy shard set.
 *
 * This method sends a query to all shards in the cluster and merges the results.
 * It is used for queries that span multiple shards, such as full table scans or namespace-local queries.
 *
 * @param query Query text sent to each selected shard.
 * @return std::vector<ShardResult> Per-shard execution records including failures/timeouts.
 *
 * @note This method implements the scatter-gather routing strategy.
 * @note The number of concurrent requests is limited by the configuration parameter `max_concurrent_shards`.
 * @note Results from each shard are merged into a single response using the `mergeResults` method.
 */
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
                        if (!executor_) {
                            result.success = false;
                            result.error_msg = "remote_executor_not_configured";
                        } else {
                            remote_count++;
                            auto exec_result = executor_->executeQuery(shard, query);
                            result.success = exec_result.success;
                            result.data = exec_result.data;
                            result.error_msg = exec_result.error;
                            result.execution_time_ms = exec_result.execution_time_ms;
                        }
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
        // Phase C observability gate: per-shard cross-shard request counters
        for (const auto& r : results) {
            metrics_->recordCrossShardRequest(r.shard_id, "scatter_gather",
                r.success ? "success" : "error");
        }
    }
    
    return results;
}

/**
 * @brief Execute query on explicit subset of shard ids.
 * @param query Query text.
 * @param shard_ids Target shard identifiers.
 * @return Per-shard execution records for targeted subset.
 */
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
        // W2-S07: Use safe map access with at() instead of find() + iterator
        try {
            target_shards.push_back(shard_map.at(id));
        } catch (const std::out_of_range&) {
            spdlog::warn("executeOnShards: unknown or unhealthy shard '{}', skipping", id);
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
                        if (!executor_) {
                            result.success = false;
                            result.error_msg = "remote_executor_not_configured";
                        } else {
                            remote_count++;
                            auto exec_result = executor_->executeQuery(shard, query);
                            result.success = exec_result.success;
                            result.data    = exec_result.data;
                            result.error_msg = exec_result.error;
                            result.execution_time_ms = exec_result.execution_time_ms;
                        }
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

/**
 * @brief Execute cross-shard join operation.
 *
 * This method performs a join operation across multiple shards.
 * It supports two strategies: co-located join (when the join field is the partition key) and broadcast hash join.
 *
 * @param query Query string containing the join operation.
 * @param join_field The field used for joining records across shards.
 * @return nlohmann::json Joined results with monotonic mergeVersion/version_token metadata so
 *         callers can detect stale merged snapshots across shards.
 *
 * @note This method implements the cross-shard join routing strategy.
 * @note The join strategy is determined by analyzing the join_field:
 *       - If the field matches the partition key pattern, a co-located join is performed.
 *       - Otherwise, a broadcast hash join is used.
 */
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

        // Phase 2: Parse right-side collection from the query string and probe
        // the hash table with right-side documents.
        //
        // Expected query format:
        //   "JOIN <left_coll> ON <field> WITH <right_coll> [WHERE ...]"
        // A plain collection name is also accepted as a fallback.
        std::string right_collection;
        {
            const std::string with_kw = " WITH ";  // uppercase per convention
            const std::string WITH_KW = " with ";  // case-insensitive fallback
            auto pos = query.find(with_kw);
            if (pos == std::string::npos) pos = query.find(WITH_KW);
            if (pos != std::string::npos) {
                pos += with_kw.size();
                // Collection name ends at whitespace, ';', or end-of-string.
                auto end = query.find_first_of(" \t\r\n;", pos);
                right_collection = (end == std::string::npos)
                    ? query.substr(pos)
                    : query.substr(pos, end - pos);
            }
        }

        size_t total_right_rows  = 0;
        size_t total_matched_rows = 0;
        uint64_t observed_version = 0;
        for (const auto& shard_result : left_results) {
            observed_version = std::max(observed_version, resolveShardResultVersion(shard_result));
        }
        uint64_t merge_version = makeStrictMergeVersionToken(observed_version);
        nlohmann::json joined_rows = nlohmann::json::array();

        if (!right_collection.empty()) {
            // Fetch right-side documents from all shards.
            const std::string right_query =
                "FOR doc IN " + right_collection + " RETURN doc";
            auto right_results = scatterGather(right_query);
            for (const auto& shard_result : right_results) {
                observed_version = std::max(observed_version, resolveShardResultVersion(shard_result));
                merge_version = makeStrictMergeVersionToken(observed_version);
                if (!shard_result.success || !shard_result.data.is_array()) continue;
                for (const auto& right_row : shard_result.data) {
                    total_right_rows++;
                    if (!right_row.contains(join_field)) continue;
                    std::string key = right_row[join_field].is_string()
                        ? right_row[join_field].get<std::string>()
                        : right_row[join_field].dump();
                    auto it = hash_table.find(key);
                    if (it == hash_table.end()) continue;
                    // Emit one merged row per matching left-side entry.
                    for (const auto& left_row : it->second) {
                        nlohmann::json merged = nlohmann::json::object();
                        for (const auto& [k, v] : left_row.items()) {
                            merged["left_" + k] = v;
                        }
                        for (const auto& [k, v] : right_row.items()) {
                            const std::string rk = "right_" + k;
                            if (!merged.contains(rk)) merged[rk] = v;
                        }
                        const uint64_t row_merge_version = std::max(
                            merge_version,
                            std::max(extractVersionToken(left_row), extractVersionToken(right_row)));
                        merged["mergeVersion"] = row_merge_version;
                        merged["version_token"] = row_merge_version;
                        joined_rows.push_back(std::move(merged));
                        ++total_matched_rows;
                    }
                }
            }
        }

        nlohmann::json result = {
            {"join_type",    "broadcast_hash"},
            {"join_field",   join_field},
            {"left_rows",    total_left_rows},
            {"right_rows",   total_right_rows},
            {"matched_rows", total_matched_rows},
            {"mergeVersion", merge_version},
            {"version_token", merge_version},
            {"data",         std::move(joined_rows)}
        };
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time).count();
        
        if (metrics_) {
            metrics_->recordCrossShardJoinDuration(strategy_name, static_cast<double>(duration_ms));
            metrics_->recordCrossShardJoinRows(strategy_name, total_left_rows,
                                               total_right_rows, total_matched_rows);
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
        const uint64_t merge_version = merged.value("mergeVersion", makeMergeVersionToken());
        
        nlohmann::json result = {
            {"join_type", "co_located"},
            {"join_field", join_field},
            {"mergeVersion", merge_version},
            {"version_token", merge_version},
            {"data", merged}
        };
        
        return result;
    }
}

/**
 * @brief Return aggregate routing counters collected by this router instance.
 * @return JSON object with totals for all requests, local/remote dispatches,
 *         scatter-gather operations, and observed errors.
 */
nlohmann::json ShardRouter::getStatistics() const {
    return nlohmann::json{
        {"total_requests", total_requests_.load()},
        {"local_requests", local_requests_.load()},
        {"remote_requests", remote_requests_.load()},
        {"scatter_gather_requests", scatter_gather_requests_.load()},
        {"errors", errors_.load()}
    };
}

/**
 * @brief Resolve the owning shard for a URN and dispatch the request.
 * @param urn Entity identifier used for primary-shard resolution.
 * @param method HTTP-style operation verb. Empty values are rejected fail-closed.
 * @param path Relative API path to execute on the target shard. Empty values are rejected fail-closed.
 * @param body Optional JSON payload for PUT/POST style requests.
 * @return Result envelope containing the target shard id, payload, and any failure detail.
 * @note Local shards are executed via @ref executeLocal while remote shards require a configured executor.
 */
ShardResult ShardRouter::routeRequest(
    const URN& urn,
    const std::string& method,
    const std::string& path,
    const std::optional<nlohmann::json>& body) {
    
    ShardResult result;

    // Fail-closed guards: reject empty method and path
    if (method.empty()) {
        result.success = false;
        result.error_msg = "method is empty";
        errors_++;
        spdlog::error("ShardRouter::routeRequest: method is empty");
        return result;
    }
    
    if (path.empty()) {
        result.success = false;
        result.error_msg = "path is empty";
        errors_++;
        spdlog::error("ShardRouter::routeRequest: path is empty");
        return result;
    }
    
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
    
    // Execute remotely — fail-closed when executor is not configured.
    if (!executor_) {
        result.success = false;
        result.error_msg = "remote_executor_not_configured";
        errors_++;
        THEMIS_ERROR("ShardRouter[{}]: routeRequest rejected — remote_executor_not_configured (shard={})",
                     config_.local_shard_id, result.shard_id);
        return result;
    }

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

/**
 * @brief Execute a routed request against the local shard simulation facade.
 * @param method HTTP-style operation verb. Empty values are rejected fail-closed.
 * @param path Relative API path describing the local operation to emulate.
 * @param body Optional JSON payload used for query or write-style requests.
 * @return Result envelope containing locally synthesized payload data or an error description.
 * @note Unknown methods, invalid URNs, and unsupported paths are converted into structured JSON errors.
 */
ShardResult ShardRouter::executeLocal(
    const std::string& method,
    const std::string& path,
    const std::optional<nlohmann::json>& body) {
    
    ShardResult result;
    result.shard_id = config_.local_shard_id;
    result.success = false;

    // Fail-closed guards: reject empty method and path
    if (method.empty()) {
        result.error_msg = "method is empty";
        errors_++;
        spdlog::error("ShardRouter::executeLocal: method is empty");
        return result;
    }
    
    if (path.empty()) {
        result.error_msg = "path is empty";
        errors_++;
        spdlog::error("ShardRouter::executeLocal: path is empty");
        return result;
    }
    
    auto start_time = std::chrono::steady_clock::now();
    
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

/**
 * @brief Merge per-shard responses into one logical query result.
 * @param results Individual shard results produced by scatter-gather or subset execution.
 * @return JSON object containing merged result rows, per-shard errors, counters,
 *         and monotonic merge version metadata.
 * @note Successful array payloads are concatenated; non-array payloads are appended as single records.
 */
nlohmann::json ShardRouter::mergeResults(const std::vector<ShardResult>& results) {
    // W2-S07: Merge strategy for distributed query results
    // - Conflict resolution: Last-write-wins based on shard response order
    // - Consistency model: Read committed (eventual) from multiple shards
    // - Duplicate handling: Client responsible for deduplication on keys
    // - Ordering: Results ordered by shard response arrival, not key
    // W5-Sharding: Ensure monotonic version metadata propagation across shard boundaries
    
    nlohmann::json merged;
    merged["results"] = nlohmann::json::array();
    merged["errors"] = nlohmann::json::array();
    merged["shard_count"] = results.size();
    
    size_t success_count = 0;
    uint64_t observed_version = 0;
    
    // W5-Sharding: Scan all shard versions and use maximum to ensure forward consistency
    for (const auto& result : results) {
        const uint64_t shard_version = resolveShardResultVersion(result);
        // Ensure merge_version is strictly greater than any shard version
        // This prevents stale read detection across shard boundaries
        observed_version = std::max(observed_version, shard_version);
    }
    
    const uint64_t merge_version = makeStrictMergeVersionToken(observed_version);
    
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
    merged["mergeVersion"] = merge_version;
    merged["version_token"] = merge_version;
    
    return merged;
}

/**
 * @brief Apply offset/limit slicing to an already merged result set.
 * @param merged JSON payload that may contain a `results` array.
 * @param offset Zero-based start index within the merged `results` array.
 * @param limit Maximum number of rows to return.
 * @return Copy of @p merged with paginated `results` and pagination metadata when applicable.
 */
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

/**
 * @brief Extract the first URN literal embedded in a query string.
 * @param query Query text to inspect.
 * @return Parsed URN when a matching literal is found and can be parsed; otherwise `std::nullopt`.
 */
std::optional<URN> ShardRouter::extractURN(const std::string& query) const {
    // Simple regex to find URN in query
    std::regex urn_pattern(R"(urn:themis:[^:]+:[^:]+:[^:]+:[a-f0-9-]+)");
    std::smatch match;
    
    if (std::regex_search(query, match, urn_pattern)) {
        return URN::parse(match[0].str());
    }
    
    return std::nullopt;
}

/**
 * @brief Extract an explicit namespace selector from a query string.
 * @param query Query text to inspect.
 * @return Namespace token following the `NAMESPACE` keyword, or `std::nullopt` when absent.
 */
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
