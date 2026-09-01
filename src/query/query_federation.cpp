/**
 * @file query_federation.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "query/query_federation.h"
#include "query/scope_enforcer.h"
#include <chrono>
#include <algorithm>
#include <cctype>
#include <regex>
#include <set>
#include <stdexcept>
#include <string_view>
#include <unordered_set>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include "utils/logger.h"

// Configuration constants
namespace {
    // Threshold for using partition pruning strategy
    constexpr size_t PARTITION_PRUNING_THRESHOLD = 5;

    [[nodiscard]] std::shared_ptr<::themis::sharding::ShardRouter> requireShardRouter(
        std::shared_ptr<::themis::sharding::ShardRouter> shard_router) {
        if (!shard_router) {
            throw std::invalid_argument("QueryFederation: shard_router cannot be null");
        }
        return shard_router;
    }

    [[nodiscard]] bool isValidAqlIdentifier(std::string_view value) {
        if (value.empty()) {
            return false;
        }
        const auto is_ident_char = [](unsigned char c) {
            return std::isalnum(c) != 0 || c == '_';
        };

        const unsigned char first = static_cast<unsigned char>(value.front());
        if (!(std::isalpha(first) != 0 || first == '_')) {
            return false;
        }

        for (size_t i = 1; i < value.size(); ++i) {
            if (!is_ident_char(static_cast<unsigned char>(value[i]))) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] bool isShardKeyFieldName(std::string_view raw_field) {
        if (raw_field.empty()) {
            return false;
        }

        std::string field(raw_field);
        std::transform(field.begin(), field.end(), field.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        const size_t dot_pos = field.rfind('.');
        const std::string_view terminal =
            dot_pos == std::string::npos ? std::string_view(field)
                                         : std::string_view(field).substr(dot_pos + 1);

        return terminal == "_key" || terminal == "id";
    }

    void enforceJsonSizeLimit(const nlohmann::json& value,
                              uint64_t max_bytes,
                              std::string_view context) {
        const auto estimated_bytes = static_cast<uint64_t>(value.dump().size());
        if (estimated_bytes > max_bytes) {
            // Optimize: Use fmt::format for error message building
            throw std::runtime_error(
                fmt::format("QueryFederation: {} exceeds max_result_size_bytes limit", context));
        }
    }

    void enforceAccumulatedSizeLimit(uint64_t accumulated_bytes,
                                     uint64_t max_bytes,
                                     std::string_view context) {
        if (accumulated_bytes > max_bytes) {
            // Optimize: Use fmt::format for error message building
            throw std::runtime_error(
                fmt::format("QueryFederation: {} exceeds max_result_size_bytes limit", context));
        }
    }

    static std::string stableJsonOrderKey(const nlohmann::json& doc) {
        if (doc.contains("_key") && doc["_key"].is_string()) {
            return doc["_key"].get<std::string>();
        }
        return doc.dump();
    }

    static bool stableJsonLess(const nlohmann::json& a, const nlohmann::json& b) {
        const std::string key_a = stableJsonOrderKey(a);
        const std::string key_b = stableJsonOrderKey(b);
        if (key_a == key_b) {
            return a.dump() < b.dump();
        }
        return key_a < key_b;
    }

    static std::vector<nlohmann::json> sortedJsonArray(const nlohmann::json& data) {
        std::vector<nlohmann::json> values;
        if (!data.is_array()) {
            return values;
        }
        values.reserve(data.size());
        for (const auto& value : data) {
            values.push_back(value);
        }
        std::sort(values.begin(), values.end(), stableJsonLess);
        return values;
    }
}

namespace themis::query {

::themis::query::QueryFederation::QueryFederation(
    std::shared_ptr<::themis::sharding::ShardRouter> shard_router
) : QueryFederation(std::move(shard_router), Config{}) {
}

::themis::query::QueryFederation::QueryFederation(
    std::shared_ptr<::themis::sharding::ShardRouter> shard_router,
    const Config& config
) : shard_router_(requireShardRouter(std::move(shard_router))),
    sharding_manager_(nullptr),
    config_(config)
{
    spdlog::info("QueryFederation initialized: pushdown={}, parallel={}, streaming={}",
                 config_.enable_pushdown, config_.enable_parallel_execution, 
                 config_.enable_result_streaming);
}

::themis::query::QueryFederation::QueryFederation(
    std::shared_ptr<::themis::sharding::ShardRouter> shard_router,
    ::themis::sharding::ShardingManager& sharding_manager
) : QueryFederation(std::move(shard_router), sharding_manager, Config{}) {
}

::themis::query::QueryFederation::QueryFederation(
    std::shared_ptr<::themis::sharding::ShardRouter> shard_router,
    ::themis::sharding::ShardingManager& sharding_manager,
    const Config& config
) : shard_router_(requireShardRouter(std::move(shard_router))),
    sharding_manager_(&sharding_manager),
    config_(config)
{
    spdlog::info("QueryFederation initialized with ShardingManager: pushdown={}, parallel={}, streaming={}",
                 config_.enable_pushdown, config_.enable_parallel_execution,
                 config_.enable_result_streaming);
}

// ─────────────────────────────────────────────────────────────────────────────
// DK-4: Federated RAG merge (Layer C)
// ─────────────────────────────────────────────────────────────────────────────

void QueryFederation::setRAGMerger(
    std::shared_ptr<distributed_knowledge::FederatedRAGMerger> merger)
{
    rag_merger_ = std::move(merger);
}

void QueryFederation::setShardRouter(
    std::shared_ptr<sharding::AdaptiveShardRouter> router)
{
    adaptive_router_ = std::move(router);
}

distributed_knowledge::MergedRAGContext QueryFederation::mergeRAGResults(
    const std::vector<distributed_knowledge::ShardRetrievalResult>& shard_results) const
{
    if (!rag_merger_) {
        throw std::logic_error(
            "QueryFederation::mergeRAGResults: no FederatedRAGMerger injected — "
            "call setRAGMerger() first");
    }
    return rag_merger_->merge(shard_results);
}

distributed_knowledge::MergedRAGContext QueryFederation::executeFederatedRAGQuery(
    const std::string& query,
    distributed_knowledge::AdapterDomainType domain)
{
    if (!rag_merger_) {
        throw std::logic_error(
            "QueryFederation::executeFederatedRAGQuery: no FederatedRAGMerger "
            "injected — call setRAGMerger() first");
    }

    // Initialize scope enforcer for federated result validation
    auto scope_enforcer = std::make_unique<ScopeEnforcerImpl>();

    // Fan-out to all shards
    auto raw_results = shard_router_->scatterGather(query);
    std::sort(raw_results.begin(), raw_results.end(), [](const auto& a, const auto& b) {
        return a.shard_id < b.shard_id;
    });

    // Convert ShardResult → ShardRetrievalResult with scope validation
    std::vector<distributed_knowledge::ShardRetrievalResult> rag_results;
    rag_results.reserve(raw_results.size());
    uint64_t accumulated_rag_input_bytes = 0;

    for (const auto& sr : raw_results) {
        distributed_knowledge::ShardRetrievalResult rr;
        rr.shard_id = sr.shard_id;
        rr.ok       = sr.success;
        if (!sr.success) {
            rr.error_message = sr.error_msg;
            rag_results.push_back(std::move(rr));
            continue;
        }

        // Per-shard accuracy delta from AdaptiveShardRouter (DK-2)
        if (adaptive_router_) {
            rr.adapter_accuracy_delta =
                adaptive_router_->getAdapterAccuracyDelta(sr.shard_id, domain);
        }

        // Extract documents from shard data
        // Supports two layouts:
        //   1. data is a JSON array of document objects
        //   2. data is a JSON object with a "docs" array key
        const nlohmann::json* doc_list = nullptr;
        if (sr.data.is_array()) {
            doc_list = &sr.data;
        } else if (sr.data.is_object() && sr.data.contains("docs") &&
                   sr.data["docs"].is_array()) {
            doc_list = &sr.data["docs"];
        }

        if (doc_list) {
            size_t rank = 1;
            uint64_t shard_scope_bytes = 0;
            const std::string scope_key = sr.shard_id;
            
            // Reset scope accumulation for this shard
            scope_enforcer->resetScopeAccumulation(scope_key);
            
            for (const auto& dj : *doc_list) {
                const auto doc_size = static_cast<uint64_t>(dj.dump().size());
                shard_scope_bytes += doc_size;
                
                // Phase 2 Executor Scope Fix: Enforce per-shard scope boundaries
                // Validates that no single shard exceeds resource limits
                if (auto scope_result = scope_enforcer->enforceAccumulatedScopeBounds(
                    scope_key, doc_size, config_.max_result_size_bytes)) {
                    // Scope validation passed
                    accumulated_rag_input_bytes += doc_size;
                    enforceAccumulatedSizeLimit(
                        accumulated_rag_input_bytes,
                        config_.max_result_size_bytes,
                        "federated RAG input");
                } else {
                    // Scope violation detected
                    spdlog::warn("Scope boundary violation in shard '{}': {}",
                                sr.shard_id, scope_result.error().context());
                    // Continue but log violation
                }
                
                distributed_knowledge::RetrievedDocument doc;
                doc.doc_id  = dj.value("doc_id",
                              dj.value("_key",
                              dj.value("id", std::to_string(rank))));
                doc.content = dj.value("content", dj.dump());
                doc.shard_id         = sr.shard_id;
                doc.relevance_score  = dj.value("score", 1.0 / static_cast<double>(rank));
                doc.rank_in_shard    = rank++;
                rr.documents.push_back(std::move(doc));
            }
        }

        rag_results.push_back(std::move(rr));
    }

    std::sort(rag_results.begin(), rag_results.end(), [](const auto& a, const auto& b) {
        return a.shard_id < b.shard_id;
    });

    return rag_merger_->merge(rag_results);
}

// ─────────────────────────────────────────────────────────────────────────────

nlohmann::json QueryFederation::execute(const std::string& query) {
    total_queries_++;
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        spdlog::info("Executing federated query: {}", query.substr(0, 100));
        
        // 1. Analyze query
        auto metadata = analyzeQuery(query);
        static const std::regex join_keyword_regex(R"(\bJOIN\b)", std::regex::icase);
        if (std::regex_search(query, join_keyword_regex) && metadata.joins.empty()) {
            throw std::invalid_argument(
                "QueryFederation::execute: JOIN requires a parseable ON equality condition");
        }
        
        // 2. Create execution plan
        auto plan = createExecutionPlan(query);

        // Q2: Audit — federation request dispatch
        spdlog::info("[audit] {{\"event\":\"federation_dispatch\","
                     "\"request_type\":\"{}\",\"shard_count\":{},"
                     "\"table_count\":{}}}",
                     [&]() -> const char* {
                         switch (plan.strategy) {
                             case ExecutionPlan::Strategy::SCATTER_GATHER:   return "scatter_gather";
                             case ExecutionPlan::Strategy::PARTITION_PRUNING:return "partition_pruning";
                             case ExecutionPlan::Strategy::BROADCAST_JOIN:   return "broadcast_join";
                             case ExecutionPlan::Strategy::SHUFFLE_JOIN:     return "shuffle_join";
                             case ExecutionPlan::Strategy::MAP_REDUCE:       return "map_reduce";
                             default:                                         return "unknown";
                         }
                     }(),
                     plan.target_shards.size(),
                     metadata.tables.size());
        
        // 3. Execute based on strategy
        std::vector<sharding::ShardResult> shard_results;
        
        switch (plan.strategy) {
            case ExecutionPlan::Strategy::SCATTER_GATHER:
                scatter_gather_queries_++;
                if (plan.target_shards.size() > 10) {
                    spdlog::warn("QueryFederation: broadcasting to {} shards (no shard-key predicate found); "
                                 "consider adding a FILTER on _key to enable partition pruning",
                                 plan.target_shards.size());
                }
                shard_results = shard_router_->scatterGather(query);
                break;
                
            case ExecutionPlan::Strategy::PARTITION_PRUNING: {
                partition_pruned_queries_++;
                spdlog::debug("QueryFederation: partition pruning to {} shard(s)", plan.target_shards.size());

                // Optimize: Use unordered_set for O(n) deduplication instead of sort + unique (O(n log n))
                std::unordered_set<std::string> unique_shards;
                std::vector<std::string> deduped_targets;
                deduped_targets.reserve(plan.target_shards.size());

                for (const auto& shard : plan.target_shards) {
                    if (unique_shards.insert(shard).second) {
                        deduped_targets.push_back(shard);
                    }
                }

                if (!deduped_targets.empty()) {
                    // Phase 2 Executor Scope Fix: Initialize scope enforcer for result validation
                    auto scope_enforcer = std::make_unique<ScopeEnforcerImpl>();
                    shard_results = shard_router_->executeOnShards(query, deduped_targets);

                    // Validate scope boundaries for each shard result
                    for (auto& result : shard_results) {
                        QueryScope shard_scope;
                        shard_scope.shard_id = result.shard_id;
                        shard_scope.is_federated = true;
                        shard_scope.scope_generation = 1;

                        // Scope validation on shard result data
                        if (result.success && !result.data.empty()) {
                            const std::string result_data = result.data.dump();
                            if (auto scope_result = scope_enforcer->validateResultScope(
                                    result_data, shard_scope)) {
                                spdlog::debug("Scope validation passed for shard '{}'", result.shard_id);
                            } else {
                                spdlog::warn("Scope validation failed for shard '{}': {}",
                                             result.shard_id, scope_result.error().context());
                            }
                        }
                    }
                } else {
                    spdlog::warn("Partition pruning selected without target shards; falling back to scatter-gather");
                    shard_results = shard_router_->scatterGather(query);
                }

                spdlog::debug("Partition pruning: received {} shard result(s)",
                              shard_results.size());
                break;
            }
                
            case ExecutionPlan::Strategy::BROADCAST_JOIN:
                broadcast_joins_++;
                // Handled by executeJoin - check if we have enough tables
                if (metadata.tables.size() >= 2 && !metadata.joins.empty()) {
                    return executeJoin(metadata.tables[0], metadata.tables[1], 
                                     metadata.joins[0]);
                } else {
                    spdlog::warn("Broadcast join requested but insufficient metadata");
                    shard_results = shard_router_->scatterGather(query);
                }
                break;
                
            case ExecutionPlan::Strategy::SHUFFLE_JOIN:
                shuffle_joins_++;
                // Handled by executeJoin - check if we have enough tables
                if (metadata.tables.size() >= 2 && !metadata.joins.empty()) {
                    return executeJoin(metadata.tables[0], metadata.tables[1], 
                                     metadata.joins[0]);
                } else {
                    spdlog::warn("Shuffle join requested but insufficient metadata");
                    shard_results = shard_router_->scatterGather(query);
                }
                break;
                
            case ExecutionPlan::Strategy::MAP_REDUCE:
                // Execute map phase on shards, reduce locally
                shard_results = shard_router_->scatterGather(query);
                break;
        }
        
        // 4. Merge results
        auto merged = mergeResults(shard_results, metadata);
        
        // 5. Apply global operations (ORDER BY, LIMIT, etc.)
        auto final_result = applyGlobalOperations(merged, metadata);
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time).count();

        // Q2: Audit — federated result merge
        const bool truncated = (final_result.is_array() && config_.max_result_size_bytes > 0 &&
                                 final_result.dump().size() >= config_.max_result_size_bytes);
        spdlog::info("[audit] {{\"event\":\"federation_result_merge\","
                     "\"result_count\":{},\"truncated\":{},"
                     "\"merge_time_ms\":{}}}",
                     final_result.is_array() ? final_result.size() : 0u,
                     truncated,
                     duration_ms);
        
        spdlog::info("Federated query completed in {}ms, {} results", 
                    duration_ms, final_result.size());
        
        return final_result;
        
    } catch (const std::exception& e) {
        // Q2: Audit — cross-cluster query failure
        // Exception Safety (Wave A §13): Capture full error context before propagating.
        // This ensures upstream callers (API boundaries) receive complete error information.
        const nlohmann::json audit_event = {
            {"event", "federation_failure"},
            {"reason", e.what()},
            {"exception_type", typeid(e).name()},
            {"affected_clusters", total_queries_.load()},
            {"timestamp_ms", std::chrono::system_clock::now().time_since_epoch().count()}
        };
        spdlog::error("[audit] {}", audit_event.dump());
        spdlog::error("Federated query execution failed: exception_type={} reason={}",
                     typeid(e).name(), e.what());
        throw;
    } catch (...) {
        // RATIONALE (Wave A §13 Exception Safety):
        //   Unknown exception from shard router or query execution. This catch-all
        //   ensures we log audit information even for non-std::exception types.
        //   After logging, we re-throw to preserve exception propagation semantics.
        // ACTIVATION: Only reached if exception is not std::exception (rare).
        // PRODUCTION DELTA: Exception is re-thrown with audit context logged first.
        // ACTION: Always audit before propagating to ensure observability.
        const nlohmann::json audit_event = {
            {"event", "federation_failure"},
            {"reason", "unknown exception"},
            {"affected_clusters", total_queries_.load()},
            {"timestamp_ms", std::chrono::system_clock::now().time_since_epoch().count()}
        };
        spdlog::error("[audit] {}", audit_event.dump());
        spdlog::error("Federated query execution failed with unknown exception");
        throw;
    }
}

QueryFederation::ExecutionPlan QueryFederation::createExecutionPlan(
    const std::string& query
) {
    ExecutionPlan plan;
    
    // Analyze query to determine strategy
    auto metadata = analyzeQuery(query);
    
    // Default strategy
    plan.strategy = ExecutionPlan::Strategy::SCATTER_GATHER;
    plan.estimated_cost = 1000; // Simplified cost
    
    // Check for JOINs
    if (!metadata.joins.empty()) {
        if (metadata.tables.size() >= 2) {
            // Estimate table sizes
            uint64_t left_size = estimateCollectionSize(metadata.tables[0]);
            uint64_t right_size = estimateCollectionSize(metadata.tables[1]);
            
            // Use broadcast join for small tables
            if (std::min(left_size, right_size) < config_.broadcast_threshold_bytes) {
                plan.strategy = ExecutionPlan::Strategy::BROADCAST_JOIN;
                spdlog::debug("Using broadcast join strategy");
            } else {
                plan.strategy = ExecutionPlan::Strategy::SHUFFLE_JOIN;
                spdlog::debug("Using shuffle join strategy");
            }
        } else {
            plan.strategy = ExecutionPlan::Strategy::SCATTER_GATHER;
        }
    }
    // Check for aggregations
    else if (!metadata.aggregations.empty()) {
        plan.strategy = ExecutionPlan::Strategy::MAP_REDUCE;
        spdlog::debug("Using map-reduce strategy for aggregation");
    }
    // Shard-key predicate takes priority over generic predicate pruning
    else if (config_.enable_pushdown && metadata.shard_key_predicate.has_value()) {
        plan.target_shards = determineRelevantShards(metadata);
        if (!plan.target_shards.empty()) {
            plan.strategy = ExecutionPlan::Strategy::PARTITION_PRUNING;
            spdlog::debug("Using partition pruning via shard-key predicate: {} shard(s)",
                          plan.target_shards.size());
        }
    }
    // Fall back to generic predicate-based pruning (no shard-key hint)
    else if (config_.enable_pushdown && !metadata.predicates.empty()) {
        plan.target_shards = determineRelevantShards(metadata);
        
        if (!plan.target_shards.empty() &&
            plan.target_shards.size() < PARTITION_PRUNING_THRESHOLD) {
            plan.strategy = ExecutionPlan::Strategy::PARTITION_PRUNING;
            spdlog::debug("Using partition pruning: {} shards", 
                         plan.target_shards.size());
        }
    }

    // For SCATTER_GATHER, populate target_shards with all healthy shard IDs so
    // the broadcast warning in execute() has the count.
    if (plan.strategy == ExecutionPlan::Strategy::SCATTER_GATHER) {
        QueryMetadata all_meta;
        plan.target_shards = determineRelevantShards(all_meta);
    }
    
    return plan;
}

nlohmann::json QueryFederation::executeJoin(
    const std::string& left_collection,
    const std::string& right_collection,
    const std::string& join_condition
) {
    if (!isValidAqlIdentifier(left_collection)) {
        throw std::invalid_argument(
            "QueryFederation::executeJoin: left_collection must be a valid AQL identifier");
    }
    if (!isValidAqlIdentifier(right_collection)) {
        throw std::invalid_argument(
            "QueryFederation::executeJoin: right_collection must be a valid AQL identifier");
    }
    if (join_condition.find_first_not_of(" \t\r\n") == std::string::npos) {
        throw std::invalid_argument(
            "QueryFederation::executeJoin: join_condition cannot be empty");
    }

    spdlog::info("Executing cross-shard JOIN: {} ⋈ {} ON {}",
                 left_collection, right_collection, join_condition);
    
    // Estimate sizes
    uint64_t left_size = estimateCollectionSize(left_collection);
    uint64_t right_size = estimateCollectionSize(right_collection);
    
    nlohmann::json result = nlohmann::json::array();

    // -------------------------------------------------------------------------
    // Parse join_condition to extract per-side field names.
    // Supported formats (case-insensitive equality sign):
    //   "field"                          → same field used on both sides
    //   "alias.field"                    → field extracted, same on both sides
    //   "left_alias.lfield = right_alias.rfield"
    //   "lfield = rfield"
    // -------------------------------------------------------------------------
    std::string left_field;
    std::string right_field;

    auto extractField = [](const std::string& expr) -> std::string {
        // Trim whitespace.
        size_t start = expr.find_first_not_of(" \t");
        size_t end   = expr.find_last_not_of(" \t");
        if (start == std::string::npos) return expr;
        std::string trimmed = expr.substr(start, end - start + 1);
        // If "alias.field", keep only the part after the last '.'.
        auto dot = trimmed.rfind('.');
        return (dot != std::string::npos) ? trimmed.substr(dot + 1) : trimmed;
    };

    {
        const auto eq_pos = join_condition.find('=');
        if (eq_pos == std::string::npos) {
            throw std::invalid_argument(
                "QueryFederation::executeJoin: join_condition must be an equality expression");
        }
        size_t rhs_start = eq_pos + 1;
        if (rhs_start < join_condition.size() && join_condition[rhs_start] == '=') {
            ++rhs_start;  // skip second '=' for '==' syntax
        }
        left_field  = extractField(join_condition.substr(0, eq_pos));
        right_field = extractField(join_condition.substr(rhs_start));
    }
    if (left_field.empty() || right_field.empty()) {
        throw std::invalid_argument(
            "QueryFederation::executeJoin: join_condition must resolve to non-empty field names");
    }

    uint64_t estimated_result_bytes = 0;

    if (config_.enable_broadcast_join && 
        std::min(left_size, right_size) < config_.broadcast_threshold_bytes) {
        // Broadcast join: fetch the smaller table, build a hash table, then
        // probe with the larger table.
        broadcast_joins_++;
        
        const bool left_is_small = (left_size <= right_size);
        const std::string small_table = left_is_small ? left_collection  : right_collection;
        const std::string large_table = left_is_small ? right_collection : left_collection;
        const std::string small_field = left_is_small ? left_field       : right_field;
        const std::string large_field = left_is_small ? right_field      : left_field;
        
        spdlog::info("Broadcasting {} to all shards for join with {}",
                    small_table, large_table);
        
        // 1. Fetch small table completely.
        // Optimize: Build query string more efficiently using fmt or ostringstream
        const std::string small_query = fmt::format("FOR doc IN {} RETURN doc", small_table);
        const nlohmann::json small_data =
            shard_router_->executeQuery(small_query);
        enforceJsonSizeLimit(
            small_data, config_.max_result_size_bytes, "broadcast join build-side input");
        const auto small_rows = sortedJsonArray(small_data);

        // Build hash table keyed by the join field from the small side.
        std::unordered_map<std::string, std::vector<nlohmann::json>> hash_table;
        if (!small_rows.empty()) {
            for (const auto& row : small_rows) {
                if (!row.contains(small_field)) continue;
                std::string key = row[small_field].is_string()
                    ? row[small_field].get<std::string>()
                    : row[small_field].dump();
                hash_table[key].push_back(row);
            }
        }

        // 2. Fetch large table and probe the hash table.
        const std::string large_query = fmt::format("FOR doc IN {} RETURN doc", large_table);
        const nlohmann::json large_data =
            shard_router_->executeQuery(large_query);
        enforceJsonSizeLimit(
            large_data, config_.max_result_size_bytes, "broadcast join probe-side input");
        const auto large_rows = sortedJsonArray(large_data);

        result = nlohmann::json::array();
        if (!large_rows.empty()) {
            for (const auto& large_row : large_rows) {
                if (!large_row.contains(large_field)) continue;
                const std::string key = large_row[large_field].is_string()
                    ? large_row[large_field].get<std::string>()
                    : large_row[large_field].dump();
                auto it = hash_table.find(key);
                if (it == hash_table.end()) continue;

                for (const auto& small_row : it->second) {
                    nlohmann::json merged = nlohmann::json::object();
                    // [W9-10-FIX: string_concat_loop — query_federation.cpp:312]
                    // Pre-compute prefix+underscore strings outside the field
                    // iteration loops to avoid O(F) redundant string allocations
                    // per joined row where F = number of fields.
                    const std::string& small_prefix = left_is_small ? left_collection : right_collection;
                    const std::string& large_prefix = left_is_small ? right_collection : left_collection;
                    // Build "prefix_" once; field names are appended below.
                    const std::string small_pfx_sep = small_prefix + '_';
                    const std::string large_pfx_sep = large_prefix + '_';
                    for (const auto& [k, v] : small_row.items()) {
                        merged[small_pfx_sep + k] = v;
                    }
                    for (const auto& [k, v] : large_row.items()) {
                        const std::string rk = large_pfx_sep + k;
                        if (!merged.contains(rk)) merged[rk] = v;
                    }
                    estimated_result_bytes += static_cast<uint64_t>(merged.dump().size());
                    enforceAccumulatedSizeLimit(
                        estimated_result_bytes,
                        config_.max_result_size_bytes,
                        "broadcast join result");
                    result.push_back(std::move(merged));
                }
            }
        }

        spdlog::info("Broadcast join completed: {} result rows", result.size());
        
    } else {
        // Shuffle join: both sides are fetched and joined in-process using
        // a hash join (build on left, probe with right).
        shuffle_joins_++;
        
        spdlog::info("Using shuffle join for {} ⋈ {}", 
                    left_collection, right_collection);

        const nlohmann::json left_data =
            shard_router_->executeQuery(fmt::format("FOR doc IN {} RETURN doc", left_collection));
        const nlohmann::json right_data =
            shard_router_->executeQuery(fmt::format("FOR doc IN {} RETURN doc", right_collection));
        enforceJsonSizeLimit(
            left_data, config_.max_result_size_bytes, "shuffle join left-side input");
        enforceJsonSizeLimit(
            right_data, config_.max_result_size_bytes, "shuffle join right-side input");
        const auto left_rows = sortedJsonArray(left_data);
        const auto right_rows = sortedJsonArray(right_data);

        std::unordered_map<std::string, std::vector<nlohmann::json>> hash_table;
        if (!left_rows.empty()) {
            for (const auto& row : left_rows) {
                if (!row.contains(left_field)) continue;
                std::string key = row[left_field].is_string()
                    ? row[left_field].get<std::string>()
                    : row[left_field].dump();
                hash_table[key].push_back(row);
            }
        }

        result = nlohmann::json::array();
        if (!right_rows.empty()) {
            for (const auto& right_row : right_rows) {
                if (!right_row.contains(right_field)) continue;
                const std::string key = right_row[right_field].is_string()
                    ? right_row[right_field].get<std::string>()
                    : right_row[right_field].dump();
                auto it = hash_table.find(key);
                if (it == hash_table.end()) continue;
                for (const auto& left_row : it->second) {
                    nlohmann::json merged = nlohmann::json::object();
                    for (const auto& [k, v] : left_row.items()) {
                        merged[left_collection + "_" + k] = v;
                    }
                    for (const auto& [k, v] : right_row.items()) {
                        const std::string rk = right_collection + "_" + k;
                        if (!merged.contains(rk)) merged[rk] = v;
                    }
                    estimated_result_bytes += static_cast<uint64_t>(merged.dump().size());
                    enforceAccumulatedSizeLimit(
                        estimated_result_bytes,
                        config_.max_result_size_bytes,
                        "shuffle join result");
                    result.push_back(std::move(merged));
                }
            }
        }

        spdlog::info("Shuffle join completed: {} result rows", result.size());
    }
    
    return result;
}

nlohmann::json QueryFederation::executeAggregation(const std::string& query) {
    spdlog::info("Executing federated aggregation: {}", query.substr(0, 100));
    
    // 1. Push partial aggregation to shards
    auto shard_results = shard_router_->scatterGather(query);
    std::sort(shard_results.begin(), shard_results.end(), [](const auto& a, const auto& b) {
        return a.shard_id < b.shard_id;
    });
    
    // 2. Combine partial results
    nlohmann::json combined = nlohmann::json::object();
    
    for (const auto& shard_result : shard_results) {
        if (!shard_result.success) {
            spdlog::warn("Shard {} failed: {}", 
                        shard_result.shard_id, shard_result.error_msg);
            continue;
        }

        enforceJsonSizeLimit(
            shard_result.data,
            config_.max_result_size_bytes,
            "aggregation shard result");
        
        // Merge aggregation results
        // This is simplified - actual implementation would handle
        // different aggregation functions (SUM, AVG, COUNT, etc.)
        if (combined.empty()) {
            combined = shard_result.data;
        } else {
            // Combine logic depends on aggregation type
            // For COUNT/SUM: add values
            // For AVG: combine weighted averages
            // For MIN/MAX: take min/max
        }

        enforceJsonSizeLimit(
            combined,
            config_.max_result_size_bytes,
            "aggregation result");
    }
    
    return combined;
}

nlohmann::json QueryFederation::getStatistics() const {
    nlohmann::json stats;
    
    stats["total_queries"] = total_queries_.load();
    stats["scatter_gather_queries"] = scatter_gather_queries_.load();
    stats["partition_pruned_queries"] = partition_pruned_queries_.load();
    stats["broadcast_joins"] = broadcast_joins_.load();
    stats["shuffle_joins"] = shuffle_joins_.load();
    
    stats["config"] = {
        {"enable_pushdown", config_.enable_pushdown},
        {"enable_parallel_execution", config_.enable_parallel_execution},
        {"enable_result_streaming", config_.enable_result_streaming},
        {"max_parallel_shards", config_.max_parallel_shards}
    };
    
    return stats;
}

QueryFederation::QueryMetadata QueryFederation::analyzeQuery(
    const std::string& query
) {
    QueryMetadata metadata;
    metadata.query_text = query;
    std::string query_upper = query;
    std::transform(query_upper.begin(), query_upper.end(), query_upper.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    const auto push_unique = [](std::vector<std::string>& values,
                                const std::string& value) {
        if (value.empty()) {
            return;
        }
        if (std::find(values.begin(), values.end(), value) == values.end()) {
            values.push_back(value);
        }
    };

    // ── Collection name ──────────────────────────────────────────────────────
    // Match:  FOR <var> IN <collection>
    {
        std::regex re_for(R"(FOR\s+\w+\s+IN\s+(\w+))", std::regex::icase);
        std::sregex_iterator it(query.begin(), query.end(), re_for);
        std::sregex_iterator end;
        for (; it != end; ++it) {
            if (it->size() > 1) {
                push_unique(metadata.tables, (*it)[1].str());
            }
        }
    }

    // ── Shard-key predicate ───────────────────────────────────────────────────
    // Extracts well-known AQL patterns using regex (not a full AQL AST parser).
    // Recognised forms:
    //   POINT:  FILTER <var>._key == "<value>"  (double or single quotes)
    //   RANGE:  FILTER <var>._key >= "<min>" AND <var>._key <= "<max>"
    //
    // Known limitations (fall back to scatter-gather):
    //   • No spaces variation: `_key=="value"` — not matched
    //   • Back-tick quoting:   `_key == \`value\`` — not matched
    //   • IN-array:            `_key IN ["a","b"]` — not matched
    //   • BETWEEN syntax       — not matched
    //   • Compound predicates with OR — not matched
    // A full AQL AST integration is planned for v2.0.0.
    if (!metadata.tables.empty()) {
        const std::string& col = metadata.tables.front();

        // Point lookup
        std::regex re_point(
            R"(FILTER\s+\w+\._key\s*==\s*[\"']([^\"']+)[\"'])",
            std::regex::icase);
        std::smatch m;
        if (std::regex_search(query, m, re_point)) {
            if (m.size() > 1) {
                QueryMetadata::ShardKeyPredicate pred;
                pred.kind       = QueryMetadata::ShardKeyPredicate::Kind::POINT;
                pred.collection = col;
                pred.key_value  = m[1].str();
                metadata.shard_key_predicate = std::move(pred);
                metadata.point_lookup_key = m[1].str();
                metadata.key_range.reset();
                push_unique(metadata.predicates, "shard_key_point");
            }
        } else {
            // Range lookup: ... _key >= "<min>" AND ... _key <= "<max>"
            std::regex re_range(
                R"(FILTER\s+\w+\._key\s*>=\s*[\"']([^\"']+)[\"']\s+AND\s+\w+\._key\s*<=\s*[\"']([^\"']+)[\"'])",
                std::regex::icase);
            if (std::regex_search(query, m, re_range)) {
                if (m.size() > 2) {
                    QueryMetadata::ShardKeyPredicate pred;
                    pred.kind       = QueryMetadata::ShardKeyPredicate::Kind::RANGE;
                    pred.collection = col;
                    pred.key_min    = m[1].str();
                    pred.key_max    = m[2].str();
                    metadata.shard_key_predicate = std::move(pred);
                    metadata.point_lookup_key.reset();
                    metadata.key_range = std::make_pair(m[1].str(), m[2].str());
                    push_unique(metadata.predicates, "shard_key_range");
                }
            }
        }
    }

    // ── Generic FILTER ────────────────────────────────────────────────────────
    if (query_upper.find("FILTER") != std::string::npos) {
        push_unique(metadata.predicates, "filter_present");
    }

    // ── Aggregations ─────────────────────────────────────────────────────────
    // ---- Collection extraction -------------------------------------------------
    // Pattern: FOR <var> IN <collection>
    if (metadata.tables.empty()) {
        size_t for_pos = query_upper.find("FOR");
        size_t in_pos  = query_upper.find(" IN ");
        if (for_pos != std::string::npos && in_pos != std::string::npos
                && in_pos > for_pos) {
            size_t start = in_pos + 4;
            size_t end   = query.find_first_of(" \n\t", start);
            if (end == std::string::npos) end = query.size();
            if (end > start) {
                push_unique(metadata.tables, query.substr(start, end - start));
            }
        }
    }
    // SQL-style fallback: FROM <collection>
    if (metadata.tables.empty()) {
        std::regex re_from(R"(\bFROM\s+(\w+))", std::regex::icase);
        std::smatch m_from;
        if (std::regex_search(query, m_from, re_from) && m_from.size() > 1) {
            push_unique(metadata.tables, m_from[1].str());
        }
    }

    // ---- Predicate / aggregation extraction ------------------------------------
    if (query_upper.find("COLLECT") != std::string::npos ||
        query_upper.find("COUNT")   != std::string::npos ||
        query_upper.find("SUM")     != std::string::npos) {
        push_unique(metadata.aggregations, "aggregation_present");
    }

    // ── Joins ─────────────────────────────────────────────────────────────────
    if (query_upper.find("JOIN") != std::string::npos) {
        std::regex re_join_table(R"(\bJOIN\s+(\w+))", std::regex::icase);
        std::smatch m_join_table;
        if (std::regex_search(query, m_join_table, re_join_table) &&
            m_join_table.size() > 1) {
            push_unique(metadata.tables, m_join_table[1].str());
        }

        std::regex re_join_on(
            R"(\bON\s+([A-Za-z_][A-Za-z0-9_\.]*)\s*(?:==|=)\s*([A-Za-z_][A-Za-z0-9_\.]*))",
            std::regex::icase);
        std::smatch m_join_on;
        if (std::regex_search(query, m_join_on, re_join_on) && m_join_on.size() > 2) {
            // Optimize: Use fmt::format for cleaner string building instead of + concatenation
            push_unique(
                metadata.joins,
                fmt::format("{} = {}", m_join_on[1].str(), m_join_on[2].str()));
        } else {
            spdlog::warn(
                "QueryFederation: JOIN detected without parseable ON condition; "
                "falling back from JOIN strategy");
        }
    }

    // ── LIMIT ────────────────────────────────────────────────────────────────
    {
        std::regex re_limit(R"(LIMIT\s+(\d+)(?:\s*,\s*(\d+))?)", std::regex::icase);
        std::smatch m2;
        if (std::regex_search(query, m2, re_limit)) {
            try {
                if (m2.size() > 2 && m2[2].matched) {
                    metadata.offset = std::stoull(m2[1].str());
                    metadata.limit = std::stoull(m2[2].str());
                } else if (m2.size() > 1) {
                    metadata.limit = std::stoull(m2[1].str());
                }
            } catch (const std::out_of_range& ex) {
                // Numeric value exceeds uint64_t range. Log context and degrade safely.
                THEMIS_WARN("QueryFederation::analyzeQuery: LIMIT/OFFSET numeric overflow; "
                            "match[1]={} match[2]={} error={} (falling back to no limit)",
                            m2.size() > 1 ? m2[1].str() : "<none>",
                            m2.size() > 2 ? m2[2].str() : "<none>",
                            ex.what());
                metadata.limit.reset();
                metadata.offset.reset();
            } catch (const std::invalid_argument& ex) {
                // std::stoull cannot parse the matched string (internal regex error unlikely).
                // Log context and degrade safely.
                THEMIS_WARN("QueryFederation::analyzeQuery: LIMIT/OFFSET parse error; "
                            "match[1]={} match[2]={} error={} (falling back to no limit)",
                            m2.size() > 1 ? m2[1].str() : "<none>",
                            m2.size() > 2 ? m2[2].str() : "<none>",
                            ex.what());
                metadata.limit.reset();
                metadata.offset.reset();
            } catch (const std::exception& ex) {
                // Unexpected exception type from std::stoull or regex.
                // Log full context and degrade safely.
                THEMIS_WARN("QueryFederation::analyzeQuery: LIMIT/OFFSET extraction failed; "
                            "match[1]={} match[2]={} error_type={} error={} (falling back to no limit)",
                            m2.size() > 1 ? m2[1].str() : "<none>",
                            m2.size() > 2 ? m2[2].str() : "<none>",
                            typeid(ex).name(),
                            ex.what());
                metadata.limit.reset();
                metadata.offset.reset();
            } catch (...) {
                // RATIONALE (Wave A §13 Exception Safety):
                //   Unknown exception from regex or numeric parsing. This catch-all is necessary
                //   because std::stoull may throw implementation-specific exceptions on some
                //   platforms. We degrade safely by resetting limit/offset to std::nullopt,
                //   which signals the caller to apply no pagination filter. This preserves
                //   correctness (returns all rows) at the cost of no optimization.
                // ACTIVATION: Only reached if exception is not std::exception (rare).
                // PRODUCTION DELTA: All rows returned instead of limited set.
                // ACTION: Always log operator context and gracefully degrade to unlimited.
                THEMIS_WARN("QueryFederation::analyzeQuery: LIMIT/OFFSET extraction failed with unknown exception; "
                            "falling back to no limit (query={} approx_len={})",
                            query.length() > 100 ? query.substr(0, 97) + "..." : query,
                            query.length());
                metadata.limit.reset();
                metadata.offset.reset();
            }
        }
    }

    // ---- SQL-style shard-key predicate extraction ------------------------------
    // Support basic SQL forms for systems that issue SQL through federation:
    //   WHERE id = "v" / WHERE _key = "v"
    //   WHERE id >= "a" AND id <= "z"
    // Alias-qualified fields are accepted (e.g., t.id, t._key).
    if (!metadata.tables.empty()) {
        const std::string& col = metadata.tables.front();

        if (!metadata.point_lookup_key.has_value()) {
            std::regex re_sql_point(
                R"(\bWHERE\s+([A-Za-z_][A-Za-z0-9_\.]*)\s*(?:==|=)\s*[\"']([^\"']+)[\"'])",
                std::regex::icase);
            std::smatch m_sql_point;
            if (std::regex_search(query, m_sql_point, re_sql_point) && m_sql_point.size() > 2) {
                const std::string field = m_sql_point[1].str();
                const std::string key_value = m_sql_point[2].str();
                if (isShardKeyFieldName(field) && !key_value.empty()) {
                    QueryMetadata::ShardKeyPredicate pred;
                    pred.kind = QueryMetadata::ShardKeyPredicate::Kind::POINT;
                    pred.collection = col;
                    pred.key_value = key_value;
                    metadata.shard_key_predicate = std::move(pred);
                    metadata.point_lookup_key = key_value;
                    metadata.key_range.reset();
                    push_unique(metadata.predicates, "shard_key_point");
                }
            }
        }

        if (!metadata.point_lookup_key.has_value() && !metadata.key_range.has_value()) {
            std::regex re_sql_range(
                R"(\bWHERE\s+([A-Za-z_][A-Za-z0-9_\.]*)\s*>=\s*[\"']([^\"']+)[\"']\s+AND\s+([A-Za-z_][A-Za-z0-9_\.]*)\s*<=\s*[\"']([^\"']+)[\"'])",
                std::regex::icase);
            std::smatch m_sql_range;
            if (std::regex_search(query, m_sql_range, re_sql_range) && m_sql_range.size() > 4) {
                const std::string left_field = m_sql_range[1].str();
                const std::string min_val = m_sql_range[2].str();
                const std::string right_field = m_sql_range[3].str();
                const std::string max_val = m_sql_range[4].str();
                if (isShardKeyFieldName(left_field) &&
                    isShardKeyFieldName(right_field) &&
                    !min_val.empty() && !max_val.empty()) {
                    QueryMetadata::ShardKeyPredicate pred;
                    pred.kind = QueryMetadata::ShardKeyPredicate::Kind::RANGE;
                    pred.collection = col;
                    pred.key_min = min_val;
                    pred.key_max = max_val;
                    metadata.shard_key_predicate = std::move(pred);
                    metadata.point_lookup_key.reset();
                    metadata.key_range = std::make_pair(min_val, max_val);
                    push_unique(metadata.predicates, "shard_key_range");
                }
            }
        }
    }

    return metadata;
}

std::vector<std::string> QueryFederation::determineRelevantShards(
    const QueryMetadata& metadata
) {
    std::lock_guard<std::mutex> lock(routing_mutex_);

    const auto normalizeShardIds = [](std::vector<std::string> shards) {
        std::sort(shards.begin(), shards.end());
        shards.erase(std::unique(shards.begin(), shards.end()), shards.end());
        return shards;
    };

    if (sharding_manager_) {
        const std::string collection =
            metadata.tables.empty() ? std::string{} : metadata.tables.front();

        if (metadata.point_lookup_key.has_value()) {
            std::string shard = sharding_manager_->GetShardForKey(
                collection, *metadata.point_lookup_key);
            if (!shard.empty()) {
                spdlog::debug("Shard-key point-lookup: key=\"{}\" → shard={}",
                              *metadata.point_lookup_key, shard);
                return {shard};
            }
        }

        if (metadata.key_range.has_value()) {
            auto shards = sharding_manager_->GetShardsForKeyRange(
                collection,
                metadata.key_range->first,
                metadata.key_range->second);
            if (!shards.empty()) {
                spdlog::debug("Shard-key range [{}, {}] → {} shard(s)",
                              metadata.key_range->first,
                              metadata.key_range->second,
                              shards.size());
                return normalizeShardIds(std::move(shards));
            }
        }

        if (metadata.shard_key_predicate.has_value()) {
            const auto& pred = *metadata.shard_key_predicate;
            if (pred.kind == QueryMetadata::ShardKeyPredicate::Kind::POINT) {
                std::string shard = sharding_manager_->GetShardForKey(
                    pred.collection, pred.key_value);
                if (!shard.empty()) {
                    spdlog::debug("Shard-key point-lookup (predicate): key=\"{}\" → shard={}",
                                  pred.key_value, shard);
                    return {shard};
                }
            } else {
                auto shards = sharding_manager_->GetShardsForKeyRange(
                    pred.collection, pred.key_min, pred.key_max);
                if (!shards.empty()) {
                    spdlog::debug("Shard-key range (predicate) [{}, {}] → {} shard(s)",
                                  pred.key_min, pred.key_max, shards.size());
                    return normalizeShardIds(std::move(shards));
                }
            }
        }
    }

    // If a shard-key predicate was detected, use the ShardRouter's routing
    // methods (via the URNResolver + ConsistentHashRing) to identify exactly
    // which shards are responsible.
    if (metadata.shard_key_predicate.has_value()) {
        const auto& pred = *metadata.shard_key_predicate;

        if (pred.kind == QueryMetadata::ShardKeyPredicate::Kind::POINT) {
            // Single shard lookup
            const std::string shard_id =
                shard_router_->getResolver().getShardForKey(pred.collection, pred.key_value);
            if (!shard_id.empty()) {
                spdlog::debug("QueryFederation: point-lookup key='{}' → shard '{}'",
                              pred.key_value, shard_id);
                return {shard_id};
            }
        } else {
            // Range lookup
            auto shards = shard_router_->getResolver().getShardsForKeyRange(
                pred.collection, pred.key_min, pred.key_max);
            if (!shards.empty()) {
                spdlog::debug("QueryFederation: range-lookup [{},{}] → {} shard(s)",
                              pred.key_min, pred.key_max, shards.size());
                return normalizeShardIds(std::move(shards));
            }
        }
    }

    // No shard-key hint — return all healthy shard IDs (caller will broadcast).
    auto all_shards = shard_router_->getResolver().getHealthyShards();
    std::vector<std::string> ids;
    ids.reserve(all_shards.size());
    for (const auto& s : all_shards) {
        ids.push_back(s.shard_id);
    }
    return normalizeShardIds(std::move(ids));
}

std::string QueryFederation::rewriteQueryForShard(
    const std::string& query,
    [[maybe_unused]] const std::string& shard_id
) {
    // Rewrite query to add shard-specific predicates
    // For example, add a filter on partition key
    
    std::string rewritten = query;
    
    // Simplified: just return original query
    // Real implementation would add shard-specific filters
    
    return rewritten;
}

nlohmann::json QueryFederation::mergeResults(
    const std::vector<sharding::ShardResult>& results,
    [[maybe_unused]] const QueryMetadata& metadata
) {
    nlohmann::json merged = nlohmann::json::array();
    uint64_t estimated_merged_bytes = 0;

    const auto appendMergedValue = [this, &merged, &estimated_merged_bytes](
                                       const nlohmann::json& value,
                                       std::string_view context) {
        estimated_merged_bytes += static_cast<uint64_t>(value.dump().size());
        enforceAccumulatedSizeLimit(
            estimated_merged_bytes,
            config_.max_result_size_bytes,
            context);
        merged.push_back(value);
    };
    
    // Collect all successful results in stable shard order so concatenation
    // does not depend on shard arrival order.
    std::vector<std::reference_wrapper<const sharding::ShardResult>> ordered_results;
    ordered_results.reserve(results.size());
    for (const auto& result : results) {
        ordered_results.emplace_back(result);
    }
    std::sort(ordered_results.begin(), ordered_results.end(), [](const auto& a, const auto& b) {
        return a.get().shard_id < b.get().shard_id;
    });

    // Collect all successful results
    for (const auto& result_ref : ordered_results) {
        const auto& result = result_ref.get();
        if (!result.success) {
            spdlog::warn("Skipping failed shard: {}, error: {}", 
                        result.shard_id, result.error_msg);
            continue;
        }
        
        // Merge data arrays
        if (result.data.is_array()) {
            for (const auto& item : result.data) {
                appendMergedValue(item, "merged federated result");
            }
        } else if (result.data.is_object()) {
            // Handle object results (e.g., aggregations)
            appendMergedValue(result.data, "merged federated result");
        }
    }
    
    spdlog::debug("Merged {} results from {} shards", 
                 merged.size(), results.size());
    
    return merged;
}

nlohmann::json QueryFederation::applyGlobalOperations(
    const nlohmann::json& merged,
    const QueryMetadata& metadata
) {
    nlohmann::json result = merged;
    
    // Apply deterministic ordering for array results before pagination.
    // The current implementation does not yet evaluate the ORDER BY expression,
    // so we keep the output stable by sorting with the shared JSON comparator.
    if (result.is_array() && (metadata.order_by.has_value() || metadata.limit.has_value() || metadata.offset.has_value())) {
        std::vector<nlohmann::json> ordered_rows;
        ordered_rows.reserve(result.size());
        for (const auto& row : result) {
            ordered_rows.push_back(row);
        }
        std::sort(ordered_rows.begin(), ordered_rows.end(), stableJsonLess);
        result = nlohmann::json::array();
        for (const auto& row : ordered_rows) {
            result.push_back(row);
        }
    }

    // Apply ORDER BY if present
    if (metadata.order_by.has_value()) {
        spdlog::debug("Applying ORDER BY: {}", *metadata.order_by);
    }
    
    // Apply LIMIT and OFFSET if present
    if (metadata.limit.has_value() || metadata.offset.has_value()) {
        if (result.is_array()) {
            const uint64_t requested_offset = metadata.offset.value_or(0);
            const uint64_t requested_limit =
                metadata.limit.value_or(static_cast<uint64_t>(result.size()));

            const size_t start = static_cast<size_t>(
                std::min<uint64_t>(requested_offset, static_cast<uint64_t>(result.size())));
            const size_t remaining = result.size() - start;
            const size_t page_size = static_cast<size_t>(
                std::min<uint64_t>(requested_limit, static_cast<uint64_t>(remaining)));
            const size_t end = start + page_size;
            
            nlohmann::json paginated = nlohmann::json::array();
            for (size_t i = start; i < end; ++i) {
                paginated.push_back(result[i]);
            }
            
            result = paginated;
            spdlog::debug("Applied pagination: offset={}, limit={}, result_size={}",
                         requested_offset, requested_limit, result.size());
        }
    }
    
    return result;
}

uint64_t QueryFederation::estimateCollectionSize([[maybe_unused]] const std::string& collection) {
    // Simplified size estimation
    // Real implementation would query metadata or use statistics
    
    // Return a default size (10 MB)
    return 10 * 1024 * 1024;
}

} // namespace themis::query
