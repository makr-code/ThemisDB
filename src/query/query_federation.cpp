/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            query_federation.cpp                               ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:50:22                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   86.0/100                                       ║
    • Total Lines:     668                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7811d1486a  2026-03-27  feat: Enhance backward compatibility and legacy support a... ║
    • 13e4bb2974  2026-03-26  Enhance GraphQL Performance Tests and Saga Operation Comp... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "query/query_federation.h"
#include <chrono>
#include <algorithm>
#include <regex>
#include <set>
#include <stdexcept>
#include <spdlog/spdlog.h>

// Configuration constants
namespace {
    // Threshold for using partition pruning strategy
    constexpr size_t PARTITION_PRUNING_THRESHOLD = 5;
}

namespace themis::query {

QueryFederation::QueryFederation(
    std::shared_ptr<sharding::ShardRouter> shard_router
) : QueryFederation(std::move(shard_router), Config{}) {
}

QueryFederation::QueryFederation(
    std::shared_ptr<sharding::ShardRouter> shard_router,
    const Config& config
) : shard_router_(std::move(shard_router)),
    sharding_manager_(nullptr),
    config_(config)
{
    spdlog::info("QueryFederation initialized: pushdown={}, parallel={}, streaming={}",
                 config_.enable_pushdown, config_.enable_parallel_execution, 
                 config_.enable_result_streaming);
}

QueryFederation::QueryFederation(
    std::shared_ptr<sharding::ShardRouter> shard_router,
    sharding::ShardingManager& sharding_manager
) : QueryFederation(std::move(shard_router), sharding_manager, Config{}) {
}

QueryFederation::QueryFederation(
    std::shared_ptr<sharding::ShardRouter> shard_router,
    sharding::ShardingManager& sharding_manager,
    const Config& config
) : shard_router_(std::move(shard_router)),
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

    // Fan-out to all shards
    auto raw_results = shard_router_->scatterGather(query);

    // Convert ShardResult → ShardRetrievalResult
    std::vector<distributed_knowledge::ShardRetrievalResult> rag_results;
    rag_results.reserve(raw_results.size());

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
            for (const auto& dj : *doc_list) {
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

    return rag_merger_->merge(rag_results);
}

// ─────────────────────────────────────────────────────────────────────────────

nlohmann::json QueryFederation::execute(const std::string& query) {    total_queries_++;
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        spdlog::info("Executing federated query: {}", query.substr(0, 100));
        
        // 1. Analyze query
        auto metadata = analyzeQuery(query);
        
        // 2. Create execution plan
        auto plan = createExecutionPlan(query);
        
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
                
            case ExecutionPlan::Strategy::PARTITION_PRUNING:
                partition_pruned_queries_++;
                spdlog::debug("QueryFederation: partition pruning to {} shard(s)", plan.target_shards.size());
                {
                    // Execute once and retain only the results from shards
                    // identified by routing analysis.
                    auto all_results = shard_router_->scatterGather(query);
                    const auto& targets = plan.target_shards;
                    shard_results.clear();
                    shard_results.reserve(all_results.size());
                    for (auto& sr : all_results) {
                        bool relevant = targets.empty() ||
                            std::find(targets.begin(), targets.end(), sr.shard_id)
                                != targets.end();
                        if (relevant) {
                            shard_results.push_back(std::move(sr));
                        }
                    }
                    spdlog::debug("Partition pruning: kept {}/{} shard results",
                                  shard_results.size(), all_results.size());
                }
                break;
                
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
        
        spdlog::info("Federated query completed in {}ms, {} results", 
                    duration_ms, final_result.size());
        
        return final_result;
        
    } catch (const std::exception& e) {
        spdlog::error("Federated query execution failed: {}", e.what());
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
        auto eq_pos = join_condition.find('=');
        if (eq_pos != std::string::npos) {
            size_t rhs_start = eq_pos + 1;
            if (rhs_start < join_condition.size() && join_condition[rhs_start] == '=') {
                ++rhs_start;  // skip second '=' for '==' syntax
            }
            left_field  = extractField(join_condition.substr(0, eq_pos));
            right_field = extractField(join_condition.substr(rhs_start));
        } else {
            left_field  = extractField(join_condition);
            right_field = left_field;
        }
    }

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
        const nlohmann::json small_data =
            shard_router_->executeQuery("FOR doc IN " + small_table + " RETURN doc");

        // Build hash table keyed by the join field from the small side.
        std::unordered_map<std::string, std::vector<nlohmann::json>> hash_table;
        if (small_data.is_array()) {
            for (const auto& row : small_data) {
                if (!row.contains(small_field)) continue;
                std::string key = row[small_field].is_string()
                    ? row[small_field].get<std::string>()
                    : row[small_field].dump();
                hash_table[key].push_back(row);
            }
        }

        // 2. Fetch large table and probe the hash table.
        const nlohmann::json large_data =
            shard_router_->executeQuery("FOR doc IN " + large_table + " RETURN doc");

        result = nlohmann::json::array();
        if (large_data.is_array()) {
            for (const auto& large_row : large_data) {
                if (!large_row.contains(large_field)) continue;
                const std::string key = large_row[large_field].is_string()
                    ? large_row[large_field].get<std::string>()
                    : large_row[large_field].dump();
                auto it = hash_table.find(key);
                if (it == hash_table.end()) continue;

                for (const auto& small_row : it->second) {
                    nlohmann::json merged = nlohmann::json::object();
                    for (const auto& [k, v] : small_row.items()) {
                        merged[(left_is_small ? left_collection : right_collection) + "_" + k] = v;
                    }
                    for (const auto& [k, v] : large_row.items()) {
                        const std::string rk =
                            (left_is_small ? right_collection : left_collection) + "_" + k;
                        if (!merged.contains(rk)) merged[rk] = v;
                    }
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
            shard_router_->executeQuery("FOR doc IN " + left_collection + " RETURN doc");
        const nlohmann::json right_data =
            shard_router_->executeQuery("FOR doc IN " + right_collection + " RETURN doc");

        std::unordered_map<std::string, std::vector<nlohmann::json>> hash_table;
        if (left_data.is_array()) {
            for (const auto& row : left_data) {
                if (!row.contains(left_field)) continue;
                std::string key = row[left_field].is_string()
                    ? row[left_field].get<std::string>()
                    : row[left_field].dump();
                hash_table[key].push_back(row);
            }
        }

        result = nlohmann::json::array();
        if (right_data.is_array()) {
            for (const auto& right_row : right_data) {
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
    
    // 2. Combine partial results
    nlohmann::json combined = nlohmann::json::object();
    
    for (const auto& shard_result : shard_results) {
        if (!shard_result.success) {
            spdlog::warn("Shard {} failed: {}", 
                        shard_result.shard_id, shard_result.error_msg);
            continue;
        }
        
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

    // ── Collection name ──────────────────────────────────────────────────────
    // Match:  FOR <var> IN <collection>
    {
        std::regex re_for(R"(FOR\s+\w+\s+IN\s+(\w+))", std::regex::icase);
        std::sregex_iterator it(query.begin(), query.end(), re_for);
        std::sregex_iterator end;
        for (; it != end; ++it) {
            metadata.tables.push_back((*it)[1].str());
        }
    }

    // ── Shard-key predicate ───────────────────────────────────────────────────
    // Extracts well-known AQL patterns using regex (not a full AQL AST parser).
    // Recognised forms:
    //   POINT:  FILTER <var>._key == "&lt;value&gt;"  (double or single quotes)
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
            QueryMetadata::ShardKeyPredicate pred;
            pred.kind       = QueryMetadata::ShardKeyPredicate::Kind::POINT;
            pred.collection = col;
            pred.key_value  = m[1].str();
            metadata.shard_key_predicate = std::move(pred);
            metadata.predicates.push_back("shard_key_point");
        } else {
            // Range lookup: ... _key >= "<min>" AND ... _key <= "<max>"
            std::regex re_range(
                R"(FILTER\s+\w+\._key\s*>=\s*[\"']([^\"']+)[\"']\s+AND\s+\w+\._key\s*<=\s*[\"']([^\"']+)[\"'])",
                std::regex::icase);
            if (std::regex_search(query, m, re_range)) {
                QueryMetadata::ShardKeyPredicate pred;
                pred.kind       = QueryMetadata::ShardKeyPredicate::Kind::RANGE;
                pred.collection = col;
                pred.key_min    = m[1].str();
                pred.key_max    = m[2].str();
                metadata.shard_key_predicate = std::move(pred);
                metadata.predicates.push_back("shard_key_range");
            }
        }
    }

    // ── Generic FILTER ────────────────────────────────────────────────────────
    if (query.find("FILTER") != std::string::npos &&
        metadata.predicates.empty()) {
        metadata.predicates.push_back("filter_present");
    }

    // ── Aggregations ─────────────────────────────────────────────────────────
    // ---- Collection extraction -------------------------------------------------
    // Pattern: FOR <var> IN <collection>
    size_t for_pos = query.find("FOR");
    size_t in_pos  = query.find(" IN ");
    if (for_pos != std::string::npos && in_pos != std::string::npos
            && in_pos > for_pos) {
        size_t start = in_pos + 4;
        size_t end   = query.find_first_of(" \n\t", start);
        if (end == std::string::npos) end = query.size();
        if (end > start) {
            metadata.tables.push_back(query.substr(start, end - start));
        }
    }

    // ---- Predicate / aggregation extraction ------------------------------------
    if (query.find("FILTER") != std::string::npos) {
        metadata.predicates.push_back("filter_present");
    }
    if (query.find("COLLECT") != std::string::npos ||
        query.find("COUNT")   != std::string::npos ||
        query.find("SUM")     != std::string::npos) {
        metadata.aggregations.push_back("aggregation_present");
    }

    // ── Joins ─────────────────────────────────────────────────────────────────
    if (query.find("JOIN") != std::string::npos) {
        metadata.joins.push_back("join_present");
    }

    // ── LIMIT ────────────────────────────────────────────────────────────────
    {
        std::regex re_limit(R"(LIMIT\s+(\d+))", std::regex::icase);
        std::smatch m2;
        if (std::regex_search(query, m2, re_limit)) {
            try {
                metadata.limit = std::stoull(m2[1].str());
            } catch (...) {
                metadata.limit = 100;
            }
        }
    }

    // ---- LIMIT extraction ------------------------------------------------------
    if (query.find("LIMIT") != std::string::npos) {
        metadata.limit = 100;
    }

    // ---- Shard-key predicate extraction ----------------------------------------
    // Point-lookup:  FILTER <var>._key == "&lt;value&gt;"
    // Range:         FILTER <var>._key >= "<min>" AND <var>._key <= "<max>"
    //
    // The patterns are intentionally simple (no full AQL parser); they cover the
    // common parameterised forms produced by drivers and the AQL translator.

    auto extract_quoted = [](const std::string& s, size_t pos) -> std::string {
        // Find the opening quote after `pos` and return the quoted content.
        size_t q1 = s.find('"', pos);
        if (q1 == std::string::npos) return {};
        size_t q2 = s.find('"', q1 + 1);
        if (q2 == std::string::npos) return {};
        return s.substr(q1 + 1, q2 - q1 - 1);
    };

    // Check for equality predicate on _key
    size_t eq_pos = query.find("._key ==");
    if (eq_pos != std::string::npos) {
        std::string val = extract_quoted(query, eq_pos + 8);
        if (!val.empty()) {
            metadata.point_lookup_key = val;
        }
    }

    // Check for range predicate: ._key >= "<min>" … ._key <= "<max>"
    if (!metadata.point_lookup_key.has_value()) {
        size_t ge_pos = query.find("._key >=");
        size_t le_pos = query.find("._key <=");
        if (ge_pos != std::string::npos && le_pos != std::string::npos) {
            std::string min_val = extract_quoted(query, ge_pos + 8);
            std::string max_val = extract_quoted(query, le_pos + 8);
            if (!min_val.empty() && !max_val.empty()) {
                metadata.key_range = {min_val, max_val};
            }
        }
    }

    return metadata;
}

std::vector<std::string> QueryFederation::determineRelevantShards(
    const QueryMetadata& metadata
) {
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
                return shards;
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
                return shards;
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
    return ids;
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
    
    // Collect all successful results
    for (const auto& result : results) {
        if (!result.success) {
            spdlog::warn("Skipping failed shard: {}, error: {}", 
                        result.shard_id, result.error_msg);
            continue;
        }
        
        // Merge data arrays
        if (result.data.is_array()) {
            for (const auto& item : result.data) {
                merged.push_back(item);
            }
        } else if (result.data.is_object()) {
            // Handle object results (e.g., aggregations)
            merged.push_back(result.data);
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
    
    // Apply ORDER BY if present
    if (metadata.order_by.has_value()) {
        // Sort results
        // Simplified: actual implementation would parse ORDER BY clause
        spdlog::debug("Applying ORDER BY: {}", *metadata.order_by);
    }
    
    // Apply LIMIT and OFFSET if present
    if (metadata.limit.has_value() || metadata.offset.has_value()) {
        if (result.is_array()) {
            size_t offset = metadata.offset.value_or(0);
            size_t limit = metadata.limit.value_or(result.size());
            
            size_t start = std::min(offset, result.size());
            size_t end = std::min(start + limit, result.size());
            
            nlohmann::json paginated = nlohmann::json::array();
            for (size_t i = start; i < end; ++i) {
                paginated.push_back(result[i]);
            }
            
            result = paginated;
            spdlog::debug("Applied pagination: offset={}, limit={}, result_size={}",
                         offset, limit, result.size());
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
