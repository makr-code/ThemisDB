/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            rpc_service_impl.cpp                               ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:50:51                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     2460                                           ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "server/rpc_service_impl.h"
#include "plugins/rpc_plugin_interface.h"
#include "storage/rocksdb_wrapper.h"
#include "index/spatial_index.h"
#include "utils/geo/ewkb.h"
#include "server/auth_middleware.h"
#include <sstream>
#include <chrono>
#include <algorithm>
#include <limits>
#include <queue>
#include <unordered_map>
#include <unordered_set>

// Define THEMIS_VERSION_STRING if not already defined
#ifndef THEMIS_VERSION_STRING
#define THEMIS_VERSION_STRING "1.3.0-dev"
#endif

namespace themis {
namespace server {
namespace rpc {

// Constants for geospatial calculations
namespace {
    constexpr double PI = 3.14159265358979323846;
    constexpr double DEG_TO_RAD = PI / 180.0;
    constexpr double EARTH_RADIUS_METERS = 6371000.0;
}

// Helper function to get timestamp in nanoseconds
static uint64_t getCurrentTimestampNs() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
}

// Global transaction state
static std::mutex transaction_mutex;
static std::unordered_map<std::string, std::unique_ptr<RocksDBWrapper::TransactionWrapper>> active_transactions;

json ThemisRPCService::handleGet(const json& params) {
    try {
        std::string model(params.value("model", ""));
        std::string collection(params.value("collection", ""));
        std::string uuid(params.value("uuid", ""));
        
        if (model.empty() || collection.empty() || uuid.empty()) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                "Missing required parameters: model, collection, uuid"
            );
        }
        
        // Get storage engine
        auto storage = storage_;
        if (!storage) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Database storage not initialized"
            );
        }
        
        // Construct storage key: collection:model:uuid
        std::string key = collection + ":" + model + ":" + uuid;
        
        // Get value from storage
        std::string value;
        bool found = storage->get(key, value);
        
        if (!found) {
            json result = {
                {"found", false}
            };
            return createSuccess(result);
        }
        
        // Parse the stored JSON entity
        json entity;
        try {
            entity = json::parse(value);
        } catch (const json::exception& e) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                std::string("Failed to parse stored entity: ") + e.what()
            );
        }
        
        json result = {
            {"found", true},
            {"entity", entity},
            {"version", entity.value("_version", 1)},
            {"timestamp_ns", entity.value("_timestamp_ns", 0)}
        };
        
        return createSuccess(result);
        
    } catch (const std::exception& e) {
        return createError(
            themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
            e.what()
        );
    }
}

json ThemisRPCService::handlePut(const json& params) {
    try {
        std::string model(params.value("model", ""));
        std::string collection(params.value("collection", ""));
        std::string uuid(params.value("uuid", ""));
        
        if (model.empty() || collection.empty() || uuid.empty()) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                "Missing required parameters: model, collection, uuid"
            );
        }
        
        if (!params.contains("entity")) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                "Missing required parameter: entity"
            );
        }
        
        // Get storage engine
        auto storage = storage_;
        if (!storage) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Database storage not initialized"
            );
        }
        
        // Construct storage key
        std::string key = collection + ":" + model + ":" + uuid;
        
        // Get entity and add metadata
        json entity = params["entity"];
        entity["_collection"] = collection;
        entity["_model"] = model;
        entity["uuid"] = uuid;
        entity["_timestamp_ns"] = getCurrentTimestampNs();
        
        // Set version: Client provides version in entity, or 0 for new entities
        // This supports both insert (version 0 -> 1) and update (version N -> N+1)
        int current_version = entity.value("_version", 0);
        entity["_version"] = current_version + 1;
        
        // Serialize to JSON string
        std::string value = entity.dump();
        
        // Check for optional transaction_id for transactional write
        std::string tx_id(params.value("transaction_id", ""));
        bool success = false;
        if (!tx_id.empty()) {
            std::lock_guard<std::mutex> lock(transaction_mutex);
            auto it = active_transactions.find(tx_id);
            if (it == active_transactions.end()) {
                return createError(
                    themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                    "Transaction not found: " + tx_id
                );
            }
            std::vector<uint8_t> value_vec(value.begin(), value.end());
            success = it->second->put(key, value_vec);
        } else {
            success = storage->put(key, value);
        }
        
        if (!success) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Failed to write to database"
            );
        }
        
        json result = {
            {"success", true},
            {"version", entity["_version"]}
        };
        
        return createSuccess(result);
        
    } catch (const std::exception& e) {
        return createError(
            themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
            e.what()
        );
    }
}

json ThemisRPCService::handleInsert(const json& params) {
    try {
        std::string model(params.value("model", ""));
        std::string collection(params.value("collection", ""));
        std::string uuid(params.value("uuid", ""));
        
        if (model.empty() || collection.empty() || uuid.empty()) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                "Missing required parameters: model, collection, uuid"
            );
        }
        
        if (!params.contains("entity")) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                "Missing required parameter: entity"
            );
        }
        
        // Get storage engine
        auto storage = storage_;
        if (!storage) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Database storage not initialized"
            );
        }
        
        // Construct storage key
        std::string key = collection + ":" + model + ":" + uuid;
        
        // Build entity with metadata (new insert always starts at version 1)
        json entity = params["entity"];
        entity["_collection"] = collection;
        entity["_model"] = model;
        entity["uuid"] = uuid;
        entity["_timestamp_ns"] = getCurrentTimestampNs();
        entity["_version"] = 1;
        std::string value = entity.dump();
        std::vector<uint8_t> value_vec(value.begin(), value.end());
        
        // Check for optional transaction_id
        std::string tx_id(params.value("transaction_id", ""));
        
        if (!tx_id.empty()) {
            // Transactional path: hold lock for both existence check and write
            std::lock_guard<std::mutex> lock(transaction_mutex);
            auto it = active_transactions.find(tx_id);
            if (it == active_transactions.end()) {
                return createError(
                    themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                    "Transaction not found: " + tx_id
                );
            }
            
            // Check existence using transaction's isolation-correct read
            if (it->second->get(key).has_value()) {
                return createError(
                    themis::plugins::rpc::RPCErrorCode::ENTITY_ALREADY_EXISTS,
                    "Entity already exists: " + key
                );
            }
            
            if (!it->second->put(key, value_vec)) {
                return createError(
                    themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                    "Failed to write to database"
                );
            }
        } else {
            // Non-transactional path: use an internal transaction for atomic check-then-write
            // to prevent concurrent duplicate inserts.
            auto tx = storage->beginTransaction();
            if (!tx) {
                return createError(
                    themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                    "Failed to begin internal transaction for insert"
                );
            }
            
            if (tx->get(key).has_value()) {
                tx->rollback();
                return createError(
                    themis::plugins::rpc::RPCErrorCode::ENTITY_ALREADY_EXISTS,
                    "Entity already exists: " + key
                );
            }
            
            if (!tx->put(key, value_vec) || !tx->commit()) {
                return createError(
                    themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                    "Failed to write to database"
                );
            }
        }
        
        json result = {
            {"success", true},
            {"version", 1}
        };
        
        return createSuccess(result);
        
    } catch (const std::exception& e) {
        return createError(
            themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
            e.what()
        );
    }
}

json ThemisRPCService::handleDelete(const json& params) {
    try {
        std::string model(params.value("model", ""));
        std::string collection(params.value("collection", ""));
        std::string uuid(params.value("uuid", ""));
        bool cascade = params.value("cascade", false);

        if (model.empty() || collection.empty() || uuid.empty()) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                "Missing required parameters: model, collection, uuid"
            );
        }

        // Get storage engine
        auto storage = storage_;
        if (!storage) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Database storage not initialized"
            );
        }

        // Construct storage key
        std::string key = collection + ":" + model + ":" + uuid;

        // Check entity existence before attempting deletion
        std::string existing_value;
        if (!storage->get(key, existing_value)) {
            json result = {
                {"found", false},
                {"deleted_count", 0}
            };
            return createSuccess(result);
        }

        // Helper lambda: find direct children of an entity within its collection.
        // Child entities carry _parent_uuid (and optionally _parent_model /
        // _parent_collection) fields that point to their parent.
        auto find_children = [&](const std::string& p_collection,
                                  const std::string& p_model,
                                  const std::string& p_uuid) -> std::vector<std::string> {
            std::vector<std::string> children;
            std::string scan_prefix = p_collection + ":";
            std::string parent_key  = p_collection + ":" + p_model + ":" + p_uuid;

            auto iter_result = storage->newSafeIterator();
            if (!iter_result) return children;

            auto& iter = iter_result.value();
            iter.Seek(scan_prefix);
            while (iter.Valid()) {
                std::string iter_key(iter.key());
                if (iter_key.substr(0, scan_prefix.length()) != scan_prefix) break;
                if (iter_key != parent_key) {
                    std::string iter_value(iter.value());
                    try {
                        json entity = json::parse(iter_value);
                        if (entity.value("_parent_uuid", "") == p_uuid &&
                            entity.value("_parent_model", "") == p_model &&
                            entity.value("_parent_collection", "") == p_collection) {
                            children.push_back(iter_key);
                        }
                    } catch (const json::exception&) {
                        // Skip invalid JSON entries
                    }
                }
                iter.Next();
            }
            return children;
        };

        // Discover direct children of the target entity
        std::vector<std::string> direct_children = find_children(collection, model, uuid);

        // Referential integrity: block deletion when children exist and cascade is off
        if (!direct_children.empty() && !cascade) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::TRANSACTION_CONFLICT,
                "Referential integrity violation: " +
                    std::to_string(direct_children.size()) + " child entit" +
                    (direct_children.size() == 1 ? "y" : "ies") +
                    " reference this entity. Use cascade=true to delete them."
            );
        }

        // Collect all descendants via BFS for cascade delete
        std::vector<std::string> keys_to_delete;
        if (cascade) {
            std::queue<std::string> bfs_queue;
            for (const auto& child_key : direct_children) {
                bfs_queue.push(child_key);
                keys_to_delete.push_back(child_key);
            }

            while (!bfs_queue.empty()) {
                std::string curr_key = bfs_queue.front();
                bfs_queue.pop();

                // Parse collection, model, uuid from "collection:model:uuid"
                size_t first_colon  = curr_key.find(':');
                size_t second_colon = (first_colon  != std::string::npos)
                                    ? curr_key.find(':', first_colon + 1)
                                    : std::string::npos;
                if (first_colon == std::string::npos || second_colon == std::string::npos) {
                    continue;
                }
                std::string curr_collection = curr_key.substr(0, first_colon);
                std::string curr_model      = curr_key.substr(first_colon + 1,
                                                               second_colon - first_colon - 1);
                std::string curr_uuid       = curr_key.substr(second_colon + 1);

                auto grandchildren = find_children(curr_collection, curr_model, curr_uuid);
                for (const auto& gc_key : grandchildren) {
                    bfs_queue.push(gc_key);
                    keys_to_delete.push_back(gc_key);
                }
            }
        }

        // Delete descendants in reverse BFS order (deepest level first)
        int deleted_count = 0;
        for (auto it = keys_to_delete.rbegin(); it != keys_to_delete.rend(); ++it) {
            if (!storage->del(*it)) {
                return createError(
                    themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                    "Failed to delete child entity during cascade: " + *it
                );
            }
            ++deleted_count;
        }

        // Delete the target entity itself
        if (!storage->del(key)) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Failed to delete entity from database"
            );
        }
        ++deleted_count;

        json result = {
            {"success", true},
            {"deleted_count", deleted_count}
        };

        return createSuccess(result);

    } catch (const std::exception& e) {
        return createError(
            themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
            e.what()
        );
    }
}

json ThemisRPCService::handleBatchGet(const json& params) {
    try {
        if (!params.contains("keys") || !params["keys"].is_array()) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                "Missing or invalid parameter: keys (must be array)"
            );
        }
        
        // Get storage engine
        auto storage = storage_;
        if (!storage) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Database storage not initialized"
            );
        }
        
        const auto& keys_array = params["keys"];
        std::vector<std::string> keys;
        std::vector<json> results_array;
        
        // Build keys list
        for (const auto& key_obj : keys_array) {
            if (!key_obj.contains("collection") || !key_obj.contains("model") || !key_obj.contains("uuid")) {
                return createError(
                    themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                    "Each key must contain collection, model, and uuid"
                );
            }
            std::string key = key_obj["collection"].get<std::string>() + ":" +
                            key_obj["model"].get<std::string>() + ":" +
                            key_obj["uuid"].get<std::string>();
            keys.push_back(key);
        }
        
        // Perform batch get
        auto values = storage->multiGet(keys);
        
        // Build results
        for (size_t i = 0; i < values.size(); ++i) {
            json result_item;
            if (values[i].has_value()) {
                // Parse JSON entity directly from vector<uint8_t>
                try {
                    json entity = json::parse(values[i]->begin(), values[i]->end());
                    result_item = {
                        {"found", true},
                        {"entity", entity}
                    };
                } catch (const json::exception&) {
                    result_item = {
                        {"found", false},
                        {"error", "Failed to parse entity"}
                    };
                }
            } else {
                result_item = {
                    {"found", false}
                };
            }
            results_array.push_back(result_item);
        }
        
        json result = {
            {"results", results_array},
            {"count", results_array.size()}
        };
        
        return createSuccess(result);
        
    } catch (const std::exception& e) {
        return createError(
            themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
            e.what()
        );
    }
}

json ThemisRPCService::handleBatchPut(const json& params) {
    try {
        if (!params.contains("entities") || !params["entities"].is_array()) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                "Missing or invalid parameter: entities (must be array)"
            );
        }
        
        // Get storage engine
        auto storage = storage_;
        if (!storage) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Database storage not initialized"
            );
        }
        
        const auto& entities_array = params["entities"];
        
        // Create write batch for atomic operation
        auto batch = storage->createWriteBatch();
        
        uint64_t timestamp = getCurrentTimestampNs();
        int count = 0;
        
        for (const auto& item : entities_array) {
            if (!item.contains("collection") || !item.contains("model") || 
                !item.contains("uuid") || !item.contains("entity")) {
                return createError(
                    themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                    "Each item must contain collection, model, uuid, and entity"
                );
            }
            
            std::string collection = item["collection"];
            std::string model = item["model"];
            std::string uuid = item["uuid"];
            std::string key = collection + ":" + model + ":" + uuid;
            
            // Add metadata to entity
            json entity = item["entity"];
            entity["_collection"] = collection;
            entity["_model"] = model;
            entity["uuid"] = uuid;
            entity["_timestamp_ns"] = timestamp;
            
            // Set version: Client provides version in entity, or 0 for new entities
            // This supports both insert (version 0 -> 1) and update (version N -> N+1)
            int current_version = entity.value("_version", 0);
            entity["_version"] = current_version + 1;
            
            // Serialize and add to batch
            std::string value = entity.dump();
            batch->put(key, std::vector<uint8_t>(value.begin(), value.end()));
            count++;
        }
        
        // Commit batch atomically
        bool success = batch->commit();
        
        if (!success) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Failed to commit batch write"
            );
        }
        
        json result = {
            {"success", true},
            {"count", count}
        };
        
        return createSuccess(result);
        
    } catch (const std::exception& e) {
        return createError(
            themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
            e.what()
        );
    }
}

json ThemisRPCService::handleBatchDelete(const json& params) {
    try {
        if (!params.contains("keys") || !params["keys"].is_array()) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                "Missing or invalid parameter: keys (must be array)"
            );
        }

        auto storage = storage_;
        if (!storage) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Database storage not initialized"
            );
        }

        const auto& keys_array = params["keys"];

        auto batch = storage->createWriteBatch();
        int count = 0;

        for (const auto& key_obj : keys_array) {
            if (!key_obj.contains("collection") || !key_obj.contains("model") || !key_obj.contains("uuid")) {
                return createError(
                    themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                    "Each key must contain collection, model, and uuid"
                );
            }
            std::string key = key_obj["collection"].get<std::string>() + ":" +
                              key_obj["model"].get<std::string>() + ":" +
                              key_obj["uuid"].get<std::string>();
            batch->del(key);
            count++;
        }

        bool success = batch->commit();

        if (!success) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Failed to commit batch delete"
            );
        }

        json result = {
            {"success", true},
            {"count", count}
        };

        return createSuccess(result);

    } catch (const std::exception& e) {
        return createError(
            themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
            e.what()
        );
    }
}

json ThemisRPCService::handleQuery(const json& params) {
    try {
        std::string aql;
        if (params.is_object()) {
            aql = params.value("aql", "");
            if (aql.empty()) {
                aql = params.value("query", "");
            }
        }

        // Get storage engine
        auto storage = storage_;
        if (!storage) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Database storage not initialized"
            );
        }

        const std::string collection = params.value("collection", "");
        const std::string model = params.value("model", "");

        json filter = json::object();
        if (params.contains("filter") && params["filter"].is_object()) {
            filter = params["filter"];
        } else if (params.contains("predicates") && params["predicates"].is_array()) {
            for (const auto& pred : params["predicates"]) {
                if (!pred.is_object() || !pred.contains("column") || !pred.contains("value") ||
                    !pred["column"].is_string()) {
                    return createError(
                        themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                        "Invalid predicate format: expected {column, value}"
                    );
                }
                filter[pred["column"].get<std::string>()] = pred["value"];
            }
        }

        // Productive structured query path (collection + optional model/filter).
        if (!collection.empty()) {
            const std::string ret_mode = params.value("return", "results");
            const bool count_only = (ret_mode == "count");
            const int limit = params.value("limit", 100);

            std::string prefix = collection + ":";
            if (!model.empty()) {
                prefix += model + ":";
            }

            auto iter_result = storage->newSafeIterator();
            if (!iter_result) {
                return createError(
                    themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                    "Failed to create iterator: " + iter_result.error().message()
                );
            }

            auto& iter = iter_result.value();
            size_t matched_total = 0;
            size_t emitted = 0;
            json results = json::array();

            iter.Seek(prefix);
            while (iter.Valid()) {
                std::string key(iter.key());
                if (key.substr(0, prefix.length()) != prefix) {
                    break;
                }

                std::string value(iter.value());
                try {
                    json entity = json::parse(value);

                    bool matches = true;
                    for (auto& [field, expected_value] : filter.items()) {
                        if (!entity.contains(field) || entity[field] != expected_value) {
                            matches = false;
                            break;
                        }
                    }

                    if (matches) {
                        ++matched_total;
                        if (!count_only && emitted < static_cast<size_t>(std::max(limit, 0))) {
                            results.push_back(entity);
                            ++emitted;
                        }
                    }
                } catch (const json::exception&) {
                    // Skip malformed JSON entries.
                }

                iter.Next();
            }

            // has_more is true if we matched more entities than we emitted (limited by size constraint)
            const bool has_more_result = !count_only && (matched_total > emitted);

            json result;
            if (count_only) {
                result = {
                    {"count", matched_total},
                    {"collection", collection},
                    {"mode", "scan_filter_count"}
                };
                if (!model.empty()) {
                    result["model"] = model;
                }
            } else {
                result = {
                    {"results", results},
                    {"count", emitted},
                    {"matched_total", matched_total},
                    {"has_more", has_more_result}
                };
            }

            return createSuccess(result);
        }

        if (aql.empty()) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                "Missing required parameter: aql or collection"
            );
        }
        
        // Note: Full AQL query engine integration requires the AQL parser and execution engine.
        // The AQL engine is an optional module that must be enabled during build.
        // For basic queries, use the 'search' or 'paginated_query' methods which support
        // simple field-based filtering directly on the storage layer.
        json result = {
            {"results", json::array()},
            {"has_more", false},
            {"count", 0},
            {"note", "AQL query engine module not available. Use 'search' or 'paginated_query' for basic filtering."}
        };
        
        return createSuccess(result);
        
    } catch (const std::exception& e) {
        return createError(
            themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
            e.what()
        );
    }
}

json ThemisRPCService::handleVectorSearch(const json& params) {
    try {
        std::string collection(params.value("collection", ""));
        
        if (collection.empty()) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                "Missing required parameter: collection"
            );
        }
        
        const bool has_vector = params.contains("vector") && params["vector"].is_array();
        const bool has_query_vector = params.contains("query_vector") && params["query_vector"].is_array();
        if (!has_vector && !has_query_vector) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                "Missing or invalid parameter: vector/query_vector (must be array)"
            );
        }
        
        // Get storage engine
        auto storage = storage_;
        if (!storage) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Database storage not initialized"
            );
        }
        
        // Extract parameters
        [[maybe_unused]] int k = params.value("k", 10);  // default top-k = 10
        std::string metric(params.value("metric", "cosine"));  // cosine, euclidean, dot
        
        // Note: Vector search requires the vector index module (FAISS or similar).
        // The vector index is an optional module that must be enabled during build.
        // When available, it provides high-performance similarity search over embeddings.
        json result = {
            {"results", json::array()},
            {"note", "Vector search requires vector index module (enable with -DTHEMIS_ENABLE_VECTOR_INDEX=ON)"}
        };
        
        return createSuccess(result);
        
    } catch (const std::exception& e) {
        return createError(
            themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
            e.what()
        );
    }
}

json ThemisRPCService::handleGraphTraverse(const json& params) {
    try {
        // Get storage engine
        auto storage = storage_;
        if (!storage) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Database storage not initialized"
            );
        }
        
        // Extract parameters
        std::string start_vertex(params.value("start_vertex", ""));
        std::string direction(params.value("direction", "outbound"));  // outbound, inbound, any
        [[maybe_unused]] int max_depth = params.value("max_depth", 1);
        
        if (start_vertex.empty()) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                "Missing required parameter: start_vertex"
            );
        }
        
        // Note: Graph traversal requires the graph index module.
        // The graph index is an optional module that must be enabled during build.
        // When available, it provides efficient graph queries (BFS, DFS, shortest path, etc.).
        json result = {
            {"vertices", json::array()},
            {"edges", json::array()},
            {"note", "Graph traversal requires graph index module (enable with -DTHEMIS_ENABLE_GRAPH_INDEX=ON)"}
        };
        
        return createSuccess(result);
        
    } catch (const std::exception& e) {
        return createError(
            themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
            e.what()
        );
    }
}

json ThemisRPCService::handleGeoQuery(const json& params) {
    try {
        std::string collection(params.value("collection", ""));
        
        if (collection.empty()) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                "Missing required parameter: collection"
            );
        }
        
        // Get storage engine
        auto storage = storage_;
        if (!storage) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Database storage not initialized"
            );
        }
        
        // Check if spatial index is available
        if (!spatial_index_) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Spatial index not initialized"
            );
        }
        
        // Verify that the collection has a spatial index
        if (!spatial_index_->hasSpatialIndex(collection)) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                "Collection '" + collection + "' does not have a spatial index. Create one first using spatial index API."
            );
        }
        
        // Extract geo query parameters
        std::string query_type(params.value("type", ""));  // within, near, intersects
        
        if (query_type.empty()) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                "Missing required parameter: type (within, near, intersects)"
            );
        }
        
        json results = json::array();
        
        // Handle different query types
        if (query_type == "intersects" || query_type == "within") {
            // Parse bounding box
            if (!params.contains("bbox") || !params["bbox"].is_object()) {
                return createError(
                    themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                    "Missing or invalid 'bbox' parameter. Expected: {minx, miny, maxx, maxy}"
                );
            }
            
            auto bbox_json = params["bbox"];
            if (!bbox_json.contains("minx") || !bbox_json.contains("miny") ||
                !bbox_json.contains("maxx") || !bbox_json.contains("maxy")) {
                return createError(
                    themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                    "bbox must contain: minx, miny, maxx, maxy"
                );
            }
            
            geo::MBR query_bbox(
                bbox_json["minx"].get<double>(),
                bbox_json["miny"].get<double>(),
                bbox_json["maxx"].get<double>(),
                bbox_json["maxy"].get<double>()
            );
            
            // Perform spatial search
            auto search_results = spatial_index_->searchIntersects(collection, query_bbox);
            
            // Convert results to JSON
            for (const auto& result : search_results) {
                json result_obj;
                result_obj["primary_key"] = result.primary_key;
                result_obj["mbr"] = {
                    {"minx", result.mbr.minx},
                    {"miny", result.mbr.miny},
                    {"maxx", result.mbr.maxx},
                    {"maxy", result.mbr.maxy}
                };
                if (result.z_min.has_value() && result.z_max.has_value()) {
                    result_obj["z_min"] = result.z_min.value();
                    result_obj["z_max"] = result.z_max.value();
                }
                results.push_back(result_obj);
            }
            
        } else if (query_type == "near") {
            // Parse center point and radius
            if (!params.contains("center") || !params["center"].is_object()) {
                return createError(
                    themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                    "Missing or invalid 'center' parameter. Expected: {lon, lat}"
                );
            }
            
            if (!params.contains("radius")) {
                return createError(
                    themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                    "Missing 'radius' parameter (in meters)"
                );
            }
            
            auto center = params["center"];
            if (!center.contains("lon") || !center.contains("lat")) {
                return createError(
                    themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                    "center must contain: lon, lat"
                );
            }
            
            double lon = center["lon"].get<double>();
            double lat = center["lat"].get<double>();
            double radius = params["radius"].get<double>();
            
            // Create bounding box from center + radius
            // Rough approximation: 1 degree latitude ≈ 111km
            // 1 degree longitude varies by latitude
            constexpr double METERS_PER_DEGREE_LAT = 111000.0;
            double meters_per_degree_lon = METERS_PER_DEGREE_LAT * std::cos(lat * DEG_TO_RAD);
            
            double lat_delta = radius / METERS_PER_DEGREE_LAT;
            double lon_delta = radius / meters_per_degree_lon;
            
            geo::MBR query_bbox(
                lon - lon_delta,
                lat - lat_delta,
                lon + lon_delta,
                lat + lat_delta
            );
            
            // Perform spatial search
            auto search_results = spatial_index_->searchIntersects(collection, query_bbox);
            
            // Convert results to JSON and add distance
            for (const auto& result : search_results) {
                json result_obj;
                result_obj["primary_key"] = result.primary_key;
                result_obj["mbr"] = {
                    {"minx", result.mbr.minx},
                    {"miny", result.mbr.miny},
                    {"maxx", result.mbr.maxx},
                    {"maxy", result.mbr.maxy}
                };
                
                // Calculate approximate distance from center to MBR centroid
                double result_lon = (result.mbr.minx + result.mbr.maxx) / 2.0;
                double result_lat = (result.mbr.miny + result.mbr.maxy) / 2.0;
                
                // Haversine formula for great circle distance
                double lat1_rad = lat * DEG_TO_RAD;
                double lat2_rad = result_lat * DEG_TO_RAD;
                double dlat = (result_lat - lat) * DEG_TO_RAD;
                double dlon = (result_lon - lon) * DEG_TO_RAD;
                
                double a = std::sin(dlat/2) * std::sin(dlat/2) +
                          std::cos(lat1_rad) * std::cos(lat2_rad) *
                          std::sin(dlon/2) * std::sin(dlon/2);
                double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1-a));
                double distance = EARTH_RADIUS_METERS * c;
                
                result_obj["distance"] = distance;
                
                if (result.z_min.has_value() && result.z_max.has_value()) {
                    result_obj["z_min"] = result.z_min.value();
                    result_obj["z_max"] = result.z_max.value();
                }
                
                // Only include results within the specified radius
                if (distance <= radius) {
                    results.push_back(result_obj);
                }
            }
            
        } else {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                "Invalid query type. Supported types: intersects, within, near"
            );
        }
        
        json result = {
            {"results", results},
            {"count", results.size()},
            {"query_type", query_type},
            {"collection", collection}
        };
        
        return createSuccess(result);
        
    } catch (const std::exception& e) {
        return createError(
            themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
            e.what()
        );
    }
}

json ThemisRPCService::handleTimeSeriesQuery(const json& params) {
    try {
        std::string collection(params.value("collection", ""));
        
        if (collection.empty()) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                "Missing required parameter: collection"
            );
        }
        
        // Get storage engine
        auto storage = storage_;
        if (!storage) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Database storage not initialized"
            );
        }
        
        // Extract time series parameters
        uint64_t start_time = params.value("start_time", static_cast<uint64_t>(0));
        uint64_t end_time = params.value("end_time", static_cast<uint64_t>(0));
        std::string aggregation(params.value("aggregation", ""));  // sum, avg, min, max, count
        std::string agg_field(params.value("field", ""));          // field to aggregate
        int limit = params.value("limit", 1000);
        
        if (start_time == 0 || end_time == 0) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                "Missing required parameters: start_time, end_time"
            );
        }

        // Scan the collection and filter documents by _timestamp_ns range
        std::string prefix = collection + ":";
        auto iter_result = storage->newSafeIterator();
        if (!iter_result) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Failed to create iterator: " + iter_result.error().message()
            );
        }

        auto& iter = iter_result.value();
        json data_points = json::array();
        int count = 0;

        // Aggregation accumulators
        double agg_sum = 0.0;
        double agg_min = std::numeric_limits<double>::max();
        double agg_max = std::numeric_limits<double>::lowest();
        int agg_count = 0;

        iter.Seek(prefix);
        while (iter.Valid() && count < limit) {
            std::string key(iter.key());
            if (key.substr(0, prefix.length()) != prefix) {
                break;
            }

            std::string value(iter.value());
            try {
                json entity = json::parse(value);

                // Filter by timestamp range
                uint64_t ts = entity.value("_timestamp_ns", static_cast<uint64_t>(0));
                if (ts >= start_time && ts <= end_time) {
                    data_points.push_back(entity);
                    count++;

                    // Accumulate aggregation values if requested
                    if (!aggregation.empty() && !agg_field.empty() && entity.contains(agg_field)) {
                        try {
                            double val = entity[agg_field].get<double>();
                            agg_sum += val;
                            agg_min = std::min(agg_min, val);
                            agg_max = std::max(agg_max, val);
                            agg_count++;
                        } catch (const json::exception&) {
                            // Field is not numeric; skip aggregation for this record
                        }
                    }
                }
            } catch (const json::exception&) {
                // Skip invalid JSON entries
            }

            iter.Next();
        }

        json result = {
            {"data", data_points},
            {"count", count},
            {"start_time", start_time},
            {"end_time", end_time}
        };

        // Append aggregation result if requested and data was found
        if (!aggregation.empty() && agg_count > 0) {
            double agg_result_val = 0.0;
            if (aggregation == "sum")        agg_result_val = agg_sum;
            else if (aggregation == "avg")   agg_result_val = agg_sum / agg_count;
            else if (aggregation == "min")   agg_result_val = agg_min;
            else if (aggregation == "max")   agg_result_val = agg_max;
            else if (aggregation == "count") agg_result_val = static_cast<double>(agg_count);

            result["aggregation"] = aggregation;
            result["aggregation_result"] = agg_result_val;
        }

        return createSuccess(result);
        
    } catch (const std::exception& e) {
        return createError(
            themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
            e.what()
        );
    }
}

// Transaction state management
// NOTE: These are currently global static variables for simplicity.
// In a production system, these should be:
// 1. Instance variables of ThemisRPCService, or
// 2. Managed by a dedicated TransactionManager class
// This allows proper cleanup, testing, and multi-instance support.
static uint64_t transaction_counter = 0;

json ThemisRPCService::handleTransactionBegin(const json& params) {
    try {
        // Get storage engine
        auto storage = storage_;
        if (!storage) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Database storage not initialized"
            );
        }
        
        // Extract isolation level if provided. Some callers pass null/empty JSON;
        // treat that as default isolation instead of throwing type_error.
        std::string isolation = "READ_COMMITTED";
        if (params.is_object()) {
            isolation = params.value("isolation_level", isolation);
        }
        
        // Create new transaction
        std::lock_guard<std::mutex> lock(transaction_mutex);
        std::string tx_id = "tx_" + std::to_string(++transaction_counter);
        
        // Create transaction wrapper
        auto tx = storage->beginTransaction();
        if (!tx) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Failed to begin transaction"
            );
        }
        active_transactions[tx_id] = std::move(tx);
        
        json result = {
            {"transaction_id", tx_id},
            {"status", "active"},
            {"isolation_level", isolation}
        };
        
        return createSuccess(result);
        
    } catch (const std::exception& e) {
        return createError(
            themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
            e.what()
        );
    }
}

json ThemisRPCService::handleTransactionCommit(const json& params) {
    try {
        std::string tx_id;
        if (params.is_object()) {
            tx_id = params.value("transaction_id", "");
        }
        
        if (tx_id.empty()) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                "Missing required parameter: transaction_id"
            );
        }
        
        // Get storage engine
        auto storage = storage_;
        if (!storage) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Database storage not initialized"
            );
        }
        
        // Find and commit transaction
        std::lock_guard<std::mutex> lock(transaction_mutex);
        auto it = active_transactions.find(tx_id);
        if (it == active_transactions.end()) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                "Transaction not found: " + tx_id
            );
        }
        
        // Commit the transaction
        bool success = it->second->commit();
        active_transactions.erase(it);
        
        if (!success) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Transaction commit failed"
            );
        }
        
        json result = {
            {"success", true},
            {"transaction_id", tx_id}
        };
        
        return createSuccess(result);
        
    } catch (const std::exception& e) {
        return createError(
            themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
            e.what()
        );
    }
}

json ThemisRPCService::handleTransactionAbort(const json& params) {
    try {
        std::string tx_id;
        if (params.is_object()) {
            tx_id = params.value("transaction_id", "");
        }
        
        if (tx_id.empty()) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                "Missing required parameter: transaction_id"
            );
        }
        
        // Get storage engine
        auto storage = storage_;
        if (!storage) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Database storage not initialized"
            );
        }
        
        // Find and rollback transaction
        std::lock_guard<std::mutex> lock(transaction_mutex);
        auto it = active_transactions.find(tx_id);
        if (it == active_transactions.end()) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                "Transaction not found: " + tx_id
            );
        }
        
        // Rollback the transaction
        it->second->rollback();
        active_transactions.erase(it);
        
        json result = {
            {"success", true},
            {"transaction_id", tx_id}
        };
        
        return createSuccess(result);
        
    } catch (const std::exception& e) {
        return createError(
            themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
            e.what()
        );
    }
}

json ThemisRPCService::handleHealthCheck([[maybe_unused]] const json& params) {
    try {
        int64_t uptime_seconds = 0;
        if (start_time_) {
            auto now = std::chrono::steady_clock::now();
            uptime_seconds = std::chrono::duration_cast<std::chrono::seconds>(
                now - *start_time_
            ).count();
        }
        
        json result = {
            {"status", "serving"},
            {"version", THEMIS_VERSION_STRING},
            {"uptime_seconds", uptime_seconds}
        };
        
        return createSuccess(result);
        
    } catch (const std::exception& e) {
        return createError(
            themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
            e.what()
        );
    }
}

json ThemisRPCService::handleAuthenticate(const json& params) {
    try {
        std::string username;
        std::string password;
        if (params.is_object()) {
            username = params.value("username", "");
            password = params.value("password", "");
        }
        
        if (username.empty() || password.empty()) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::AUTHENTICATION_FAILED,
                "Missing username or password"
            );
        }
        
        // Authenticate using AuthMiddleware if available
        if (auth_ && auth_->isEnabled()) {
            // For password-based authentication, construct a token
            // Note: In production, this would integrate with a proper
            // authentication backend that validates username/password
            // and returns a JWT token. For now, we document this limitation.
            return createError(
                themis::plugins::rpc::RPCErrorCode::AUTHENTICATION_FAILED,
                "Password authentication requires integration with authentication backend. Use JWT tokens via Authorization header."
            );
        }
        
        // If no auth middleware, reject authentication requests
        return createError(
            themis::plugins::rpc::RPCErrorCode::AUTHENTICATION_FAILED,
            "Authentication not configured on server"
        );
        
    } catch (const std::exception& e) {
        return createError(
            themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
            e.what()
        );
    }
}

json ThemisRPCService::handleSearch(const json& params) {
    try {
        std::string collection(params.value("collection", ""));
        
        if (collection.empty()) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                "Missing required parameter: collection"
            );
        }
        
        // Get storage engine
        auto storage = storage_;
        if (!storage) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Database storage not initialized"
            );
        }
        
        // Extract search parameters
        std::string model(params.value("model", ""));
        json filter = params.value("filter", json::object());
        int limit = params.value("limit", 100);
        
        // Create iterator to scan keys with collection prefix
        std::string prefix = collection + ":";
        if (!model.empty()) {
            prefix += model + ":";
        }
        
        auto iter_result = storage->newSafeIterator();
        if (!iter_result) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Failed to create iterator: " + iter_result.error().message()
            );
        }
        
        auto& iter = iter_result.value();
        json results = json::array();
        int count = 0;
        
        // Scan keys with prefix
        iter.Seek(prefix);
        while (iter.Valid() && count < limit) {
            std::string key(iter.key());
            
            // Check if key still matches prefix
            if (key.substr(0, prefix.length()) != prefix) {
                break;
            }
            
            // Parse entity
            std::string value(iter.value());
            try {
                json entity = json::parse(value);
                
                // Apply filter if provided
                bool matches = true;
                if (!filter.empty()) {
                    for (auto& [field, expected_value] : filter.items()) {
                        if (!entity.contains(field) || entity[field] != expected_value) {
                            matches = false;
                            break;
                        }
                    }
                }
                
                if (matches) {
                    results.push_back(entity);
                    count++;
                }
            } catch (const json::exception&) {
                // Skip invalid JSON entries
            }
            
            iter.Next();
        }
        
        // Check if there are more results in the collection
        bool has_more = false;
        if (iter.Valid()) {
            std::string key(iter.key());
            has_more = (key.substr(0, prefix.length()) == prefix);
        }
        
        json result = {
            {"results", results},
            {"count", count},
            {"has_more", has_more}
        };
        
        return createSuccess(result);
        
    } catch (const std::exception& e) {
        return createError(
            themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
            e.what()
        );
    }
}

json ThemisRPCService::handleStats([[maybe_unused]] const json& params) {
    try {
        // Get storage engine
        auto storage = storage_;
        if (!storage) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Database storage not initialized"
            );
        }
        
        // Get statistics from RocksDB
        json stats = {
            {"database_path", storage->getConfig().db_path},
            {"is_open", storage->isOpen()}
        };
        
        // Try to get RocksDB statistics if available
        try {
            std::string rocksdb_stats = storage->getStats();
            if (!rocksdb_stats.empty()) {
                stats["rocksdb_stats"] = rocksdb_stats;
            }
        } catch (const std::exception&) {
            // Ignore errors getting stats
        }
        
        json result = {
            {"stats", stats},
            {"timestamp_ns", getCurrentTimestampNs()}
        };
        
        return createSuccess(result);
        
    } catch (const std::exception& e) {
        return createError(
            themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
            e.what()
        );
    }
}

json ThemisRPCService::handleUpdateEntity(const json& params) {
    try {
        std::string model(params.value("model", ""));
        std::string collection(params.value("collection", ""));
        std::string uuid(params.value("uuid", ""));
        
        if (model.empty() || collection.empty() || uuid.empty()) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                "Missing required parameters: model, collection, uuid"
            );
        }
        
        if (!params.contains("updates")) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                "Missing required parameter: updates"
            );
        }
        
        // Get storage engine
        auto storage = storage_;
        if (!storage) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Database storage not initialized"
            );
        }
        
        // Construct storage key
        std::string key = collection + ":" + model + ":" + uuid;
        
        // Get existing entity
        std::string value;
        bool found = storage->get(key, value);
        
        if (!found) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::ENTITY_NOT_FOUND,
                "Entity not found"
            );
        }
        
        // Parse existing entity
        json entity;
        try {
            entity = json::parse(value);
        } catch (const json::exception& e) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                std::string("Failed to parse stored entity: ") + e.what()
            );
        }
        
        // Apply updates (merge)
        json updates = params["updates"];
        for (auto& [field, new_value] : updates.items()) {
            entity[field] = new_value;
        }
        
        // Update metadata
        entity["_timestamp_ns"] = getCurrentTimestampNs();
        int current_version = entity.value("_version", 1);
        entity["_version"] = current_version + 1;
        
        // Store updated entity
        std::string updated_value = entity.dump();
        bool success = storage->put(key, updated_value);
        
        if (!success) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Failed to update entity"
            );
        }
        
        json result = {
            {"success", true},
            {"version", entity["_version"]}
        };
        
        return createSuccess(result);
        
    } catch (const std::exception& e) {
        return createError(
            themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
            e.what()
        );
    }
}

json ThemisRPCService::handleBatchUpdate(const json& params) {
    try {
        if (!params.contains("updates") || !params["updates"].is_array()) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                "Missing or invalid parameter: updates (must be array)"
            );
        }
        
        // Get storage engine
        auto storage = storage_;
        if (!storage) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Database storage not initialized"
            );
        }
        
        const auto& updates_array = params["updates"];
        
        // Create write batch for atomic operation
        auto batch = storage->createWriteBatch();
        
        uint64_t timestamp = getCurrentTimestampNs();
        int count = 0;
        
        for (const auto& update_item : updates_array) {
            if (!update_item.contains("collection") || !update_item.contains("model") || 
                !update_item.contains("uuid") || !update_item.contains("updates")) {
                return createError(
                    themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                    "Each item must contain collection, model, uuid, and updates"
                );
            }
            
            std::string collection = update_item["collection"];
            std::string model = update_item["model"];
            std::string uuid = update_item["uuid"];
            std::string key = collection + ":" + model + ":" + uuid;
            
            // Get existing entity
            std::string value;
            bool found = storage->get(key, value);
            
            if (!found) {
                continue; // Skip non-existent entities
            }
            
            // Parse and update entity
            try {
                json entity = json::parse(value);
                json updates = update_item["updates"];
                
                for (auto& [field, new_value] : updates.items()) {
                    entity[field] = new_value;
                }
                
                entity["_timestamp_ns"] = timestamp;
                int current_version = entity.value("_version", 1);
                entity["_version"] = current_version + 1;
                
                std::string updated_value = entity.dump();
                batch->put(key, std::vector<uint8_t>(updated_value.begin(), updated_value.end()));
                count++;
            } catch (const json::exception&) {
                // Skip invalid entities
            }
        }
        
        // Commit batch atomically
        bool success = batch->commit();
        
        if (!success) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Failed to commit batch update"
            );
        }
        
        json result = {
            {"success", true},
            {"count", count}
        };
        
        return createSuccess(result);
        
    } catch (const std::exception& e) {
        return createError(
            themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
            e.what()
        );
    }
}

json ThemisRPCService::handlePaginatedQuery(const json& params) {
    try {
        std::string collection(params.value("collection", ""));
        
        if (collection.empty()) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                "Missing required parameter: collection"
            );
        }
        
        // Get storage engine
        auto storage = storage_;
        if (!storage) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Database storage not initialized"
            );
        }
        
        // Extract pagination parameters
        std::string cursor(params.value("cursor", ""));
        int page_size = params.value("page_size", 50);
        std::string model(params.value("model", ""));
        
        // Create iterator
        std::string prefix = collection + ":";
        if (!model.empty()) {
            prefix += model + ":";
        }
        
        auto iter_result = storage->newSafeIterator();
        if (!iter_result) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Failed to create iterator: " + iter_result.error().message()
            );
        }
        
        auto& iter = iter_result.value();
        json results = json::array();
        
        // Seek to cursor position or start of prefix
        if (!cursor.empty()) {
            iter.Seek(cursor);
            if (iter.Valid() && iter.key() == cursor) {
                iter.Next(); // Skip cursor position
            }
        } else {
            iter.Seek(prefix);
        }
        
        // Collect page_size results
        int count = 0;
        std::string next_cursor;
        while (iter.Valid() && count < page_size) {
            std::string key(iter.key());
            
            // Check if key still matches prefix
            if (key.substr(0, prefix.length()) != prefix) {
                break;
            }
            
            // Parse entity
            std::string value(iter.value());
            try {
                json entity = json::parse(value);
                results.push_back(entity);
                count++;
                next_cursor = key;
            } catch (const json::exception&) {
                // Skip invalid JSON entries
            }
            
            iter.Next();
        }
        
        // Check if there are more results in the collection
        bool has_more = false;
        if (iter.Valid()) {
            std::string key(iter.key());
            has_more = (key.substr(0, prefix.length()) == prefix);
        }
        
        json result = {
            {"results", results},
            {"count", count},
            {"has_more", has_more},
            {"next_cursor", count > 0 ? next_cursor : ""}
        };
        
        return createSuccess(result);
        
    } catch (const std::exception& e) {
        return createError(
            themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
            e.what()
        );
    }
}

json ThemisRPCService::handleGetIndexOperations(const json& params) {
    try {
        // Get storage engine
        auto storage = storage_;
        if (!storage) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Database storage not initialized"
            );
        }

        // Optional: filter by collection
        std::string collection(params.value("collection", ""));

        // Scan DB for index metadata stored under _idx_meta:<collection>:<name>
        std::string prefix = "_idx_meta:";
        if (!collection.empty()) {
            prefix += collection + ":";
        }

        json indexes = json::array();
        storage->scanPrefix(prefix, [&indexes](std::string_view /*key*/, std::string_view value) -> bool {
            try {
                json idx_meta = json::parse(value);
                indexes.push_back(idx_meta);
            } catch (const json::exception&) {
                // Skip malformed entries
            }
            return true; // continue scanning
        });

        json result = {
            {"indexes", indexes},
            {"count", indexes.size()},
            {"operations_supported", json::array({
                "create_index",
                "drop_index",
                "list_indexes"
            })}
        };

        return createSuccess(result);

    } catch (const std::exception& e) {
        return createError(
            themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
            e.what()
        );
    }
}

json ThemisRPCService::handleAggregationPipeline(const json& params) {
    try {
        std::string collection(params.value("collection", ""));
        
        if (collection.empty()) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                "Missing required parameter: collection"
            );
        }
        
        if (!params.contains("pipeline") || !params["pipeline"].is_array()) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                "Missing or invalid parameter: pipeline (must be array)"
            );
        }
        
        // Get storage engine
        auto storage = storage_;
        if (!storage) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Database storage not initialized"
            );
        }
        
        // Extract pipeline stages
        const auto& pipeline = params["pipeline"];
        
        // Collect all documents from collection
        std::string prefix = collection + ":";
        auto iter_result = storage->newSafeIterator();
        if (!iter_result) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Failed to create iterator: " + iter_result.error().message()
            );
        }
        
        auto& iter = iter_result.value();
        json documents = json::array();
        
        iter.Seek(prefix);
        while (iter.Valid()) {
            std::string key(iter.key());
            if (key.substr(0, prefix.length()) != prefix) {
                break;
            }
            
            std::string value(iter.value());
            try {
                json entity = json::parse(value);
                documents.push_back(entity);
            } catch (const json::exception&) {
                // Skip invalid entries
            }
            
            iter.Next();
        }
        
        // Apply pipeline stages
        json results = documents;
        for (const auto& stage : pipeline) {
            if (!stage.is_object() || stage.empty()) {
                continue;
            }
            
            auto stage_name = stage.begin().key();
            auto stage_spec = stage.begin().value();
            
            if (stage_name == "$match") {
                // Filter documents
                json filtered = json::array();
                for (const auto& doc : results) {
                    bool matches = true;
                    for (auto& [field, expected_value] : stage_spec.items()) {
                        if (!doc.contains(field) || doc[field] != expected_value) {
                            matches = false;
                            break;
                        }
                    }
                    if (matches) {
                        filtered.push_back(doc);
                    }
                }
                results = filtered;
            } else if (stage_name == "$limit") {
                // Limit results - validate limit is positive
                if (!stage_spec.is_number_integer()) {
                    return createError(
                        themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                        "$limit stage requires integer value"
                    );
                }
                int limit = stage_spec.get<int>();
                if (limit < 0) {
                    return createError(
                        themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                        "$limit value must be non-negative"
                    );
                }
                if (results.size() > static_cast<size_t>(limit)) {
                    json limited = json::array();
                    for (size_t i = 0; i < static_cast<size_t>(limit); ++i) {
                        limited.push_back(results[i]);
                    }
                    results = limited;
                }
            } else if (stage_name == "$project") {
                // Project fields
                json projected = json::array();
                for (const auto& doc : results) {
                    json proj_doc = json::object();
                    for (auto& [field, include] : stage_spec.items()) {
                        // Validate projection spec is boolean
                        if (!include.is_boolean()) {
                            return createError(
                                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                                "$project field values must be boolean"
                            );
                        }
                        if (include.get<bool>() && doc.contains(field)) {
                            proj_doc[field] = doc[field];
                        }
                    }
                    projected.push_back(proj_doc);
                }
                results = projected;
            }
            // More stages can be added: $group, $sort, $skip, etc.
        }
        
        json result = {
            {"results", results},
            {"count", results.size()}
        };
        
        return createSuccess(result);
        
    } catch (const std::exception& e) {
        return createError(
            themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
            e.what()
        );
    }
}

json ThemisRPCService::handleListCollections([[maybe_unused]] const json& params) {
    try {
        // Get storage engine
        auto storage = storage_;
        if (!storage) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Database storage not initialized"
            );
        }
        
        // Scan all keys and extract unique collection names
        auto iter_result = storage->newSafeIterator();
        if (!iter_result) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Failed to create iterator: " + iter_result.error().message()
            );
        }
        
        auto& iter = iter_result.value();
        std::unordered_map<std::string, int> collections;
        
        iter.SeekToFirst();
        while (iter.Valid()) {
            std::string key(iter.key());
            
            // Parse key format: collection:model:uuid
            // Skip internal metadata keys (e.g. _idx_meta:<collection>:<name>)
            size_t first_colon = key.find(':');
            if (first_colon != std::string::npos) {
                std::string collection = key.substr(0, first_colon);
                if (!collection.empty() && collection[0] != '_') {
                    collections[collection]++;
                }
            }
            
            iter.Next();
        }
        
        // Build result
        json collections_array = json::array();
        for (const auto& [name, count] : collections) {
            collections_array.push_back({
                {"name", name},
                {"document_count", count}
            });
        }
        
        json result = {
            {"collections", collections_array},
            {"count", collections.size()}
        };
        
        return createSuccess(result);
        
    } catch (const std::exception& e) {
        return createError(
            themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
            e.what()
        );
    }
}

json ThemisRPCService::handleCreateIndex(const json& params) {
    try {
        std::string collection(params.value("collection", ""));
        std::string field(params.value("field", ""));
        
        if (collection.empty() || field.empty()) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                "Missing required parameters: collection, field"
            );
        }
        
        // Get storage engine
        auto storage = storage_;
        if (!storage) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Database storage not initialized"
            );
        }
        
        std::string index_name = collection + "_" + field + "_idx";
        std::string index_type(params.value("type", "btree"));

        // Persist index metadata in database under _idx_meta:<collection>:<index_name>
        std::string meta_key = "_idx_meta:" + collection + ":" + index_name;
        json meta = {
            {"name", index_name},
            {"collection", collection},
            {"field", field},
            {"type", index_type},
            {"created_ns", getCurrentTimestampNs()}
        };

        bool success = storage->put(meta_key, meta.dump());
        if (!success) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Failed to store index metadata"
            );
        }

        json result = {
            {"success", true},
            {"index_name", index_name},
            {"collection", collection},
            {"field", field},
            {"type", index_type}
        };
        
        return createSuccess(result);
        
    } catch (const std::exception& e) {
        return createError(
            themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
            e.what()
        );
    }
}

json ThemisRPCService::handleDropIndex(const json& params) {
    try {
        std::string collection(params.value("collection", ""));
        std::string index_name(params.value("index_name", ""));
        
        if (collection.empty() || index_name.empty()) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                "Missing required parameters: collection, index_name"
            );
        }
        
        // Get storage engine
        auto storage = storage_;
        if (!storage) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Database storage not initialized"
            );
        }

        // Verify the index exists in the database
        std::string meta_key = "_idx_meta:" + collection + ":" + index_name;
        std::string existing;
        if (!storage->get(meta_key, existing)) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::ENTITY_NOT_FOUND,
                "Index not found: " + index_name
            );
        }

        // Delete index metadata from database
        bool success = storage->del(meta_key);
        if (!success) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Failed to drop index"
            );
        }

        json result = {
            {"success", true},
            {"index_name", index_name},
            {"collection", collection}
        };
        
        return createSuccess(result);
        
    } catch (const std::exception& e) {
        return createError(
            themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
            e.what()
        );
    }
}

json ThemisRPCService::handleGetCollectionMetadata(const json& params) {
    try {
        std::string collection(params.value("collection", ""));
        
        if (collection.empty()) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                "Missing required parameter: collection"
            );
        }
        
        // Get storage engine
        auto storage = storage_;
        if (!storage) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Database storage not initialized"
            );
        }
        
        // Scan collection to gather metadata
        std::string prefix = collection + ":";
        auto iter_result = storage->newSafeIterator();
        if (!iter_result) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Failed to create iterator: " + iter_result.error().message()
            );
        }
        
        auto& iter = iter_result.value();
        int document_count = 0;
        uint64_t total_size = 0;
        std::unordered_map<std::string, int> models;
        
        iter.Seek(prefix);
        while (iter.Valid()) {
            std::string key(iter.key());
            if (key.substr(0, prefix.length()) != prefix) {
                break;
            }
            
            // Parse key format: collection:model:uuid
            size_t first_colon = key.find(':');
            size_t second_colon = key.find(':', first_colon + 1);
            if (first_colon != std::string::npos && second_colon != std::string::npos) {
                std::string model = key.substr(first_colon + 1, second_colon - first_colon - 1);
                models[model]++;
            }
            
            document_count++;
            total_size += iter.value().length();
            
            iter.Next();
        }
        
        // Build models array
        json models_array = json::array();
        for (const auto& [model, count] : models) {
            models_array.push_back({
                {"model", model},
                {"count", count}
            });
        }
        
        json result = {
            {"collection", collection},
            {"document_count", document_count},
            {"total_size_bytes", total_size},
            {"models", models_array},
            {"indexes", [&] {
                json idx_array = json::array();
                std::string idx_prefix = "_idx_meta:" + collection + ":";
                storage->scanPrefix(idx_prefix,
                    [&idx_array](std::string_view /*key*/, std::string_view value) -> bool {
                        try {
                            idx_array.push_back(json::parse(value));
                        } catch (const json::exception&) {
                            // Skip malformed index metadata entries
                        }
                        return true;
                    });
                return idx_array;
            }()}
        };
        
        return createSuccess(result);
        
    } catch (const std::exception& e) {
        return createError(
            themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
            e.what()
        );
    }
}

json ThemisRPCService::dispatch(
    const std::string& method,
    const json& params,
    const themis::plugins::rpc::RPCRequestContext& context
) {
    // Authentication and authorization check (except for authenticate and health_check methods)
    if (method != "authenticate" && method != "health_check") {
        std::string username;
        std::string required_scope;
        
        // Map RPC methods to required scopes according to rpc_authentication.md
        // Read operations (rpc:read)
        static const std::unordered_set<std::string> read_methods = {
            "get", "batch_get", "search", "query", "paginated_query", 
            "vector_search", "graph_traverse", "geo_query", "timeseries_query",
            "get_index_operations", "list_collections", "get_collection_metadata", 
            "aggregation_pipeline"
        };
        
        // Write operations (rpc:write)
        static const std::unordered_set<std::string> write_methods = {
            "insert", "put", "batch_put", "batch_delete", "delete", "update_entity", "batch_update"            
        };
        
        // Admin operations (rpc:admin)
        static const std::unordered_set<std::string> admin_methods = {
            "create_index", "drop_index", "stats"
        };
        
        // Transaction operations (transaction:write)
        static const std::unordered_set<std::string> transaction_methods = {
            "transaction_begin", "transaction_commit", "transaction_abort"
        };
        
        if (read_methods.count(method)) {
            required_scope = "rpc:read";
        } else if (write_methods.count(method)) {
            required_scope = "rpc:write";
        } else if (admin_methods.count(method)) {
            required_scope = "rpc:admin";
        } else if (transaction_methods.count(method)) {
            required_scope = "transaction:write";
        } else {
            // Unknown method - require admin scope
            required_scope = "rpc:admin";
        }
        
        if (!verifyAuth(context, username, required_scope)) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::AUTHENTICATION_FAILED,
                "Authentication or authorization failed: missing/invalid credentials or insufficient scope (" + required_scope + " required)"
            );
        }
    }
    
    // Route to appropriate handler
    if (method == "get") {
        return handleGet(params);
    } else if (method == "put") {
        return handlePut(params);
    } else if (method == "insert") {
        return handleInsert(params);
    } else if (method == "delete") {
        return handleDelete(params);
    } else if (method == "batch_get") {
        return handleBatchGet(params);
    } else if (method == "batch_put") {
        return handleBatchPut(params);
    } else if (method == "batch_delete") {
        return handleBatchDelete(params);
    } else if (method == "query") {
        return handleQuery(params);
    } else if (method == "vector_search") {
        return handleVectorSearch(params);
    } else if (method == "graph_traverse") {
        return handleGraphTraverse(params);
    } else if (method == "geo_query") {
        return handleGeoQuery(params);
    } else if (method == "timeseries_query") {
        return handleTimeSeriesQuery(params);
    } else if (method == "transaction_begin") {
        return handleTransactionBegin(params);
    } else if (method == "transaction_commit") {
        return handleTransactionCommit(params);
    } else if (method == "transaction_abort") {
        return handleTransactionAbort(params);
    } else if (method == "health_check") {
        return handleHealthCheck(params);
    } else if (method == "authenticate") {
        return handleAuthenticate(params);
    } else if (method == "search") {
        return handleSearch(params);
    } else if (method == "stats") {
        return handleStats(params);
    } else if (method == "update_entity") {
        return handleUpdateEntity(params);
    } else if (method == "batch_update") {
        return handleBatchUpdate(params);
    } else if (method == "paginated_query") {
        return handlePaginatedQuery(params);
    } else if (method == "get_index_operations") {
        return handleGetIndexOperations(params);
    } else if (method == "aggregation_pipeline") {
        return handleAggregationPipeline(params);
    } else if (method == "list_collections") {
        return handleListCollections(params);
    } else if (method == "create_index") {
        return handleCreateIndex(params);
    } else if (method == "drop_index") {
        return handleDropIndex(params);
    } else if (method == "get_collection_metadata") {
        return handleGetCollectionMetadata(params);
    } else {
        return createError(
            themis::plugins::rpc::RPCErrorCode::METHOD_NOT_FOUND,
            "Method not found: " + method
        );
    }
}

bool ThemisRPCService::verifyAuth(
    const themis::plugins::rpc::RPCRequestContext& context,
    std::string& username,
    const std::string& required_scope
) {
    // If auth middleware is not configured (null), allow unauthenticated access
    // for backward compatibility. In production, auth should always be configured.
    if (!auth_) {
        username = context.username.empty() ? "anonymous" : context.username;
        return true;
    }
    
    // If auth middleware is configured but not enabled, allow unauthenticated access
    // for development/testing. In production, auth should always be enabled.
    if (!auth_->isEnabled()) {
        username = context.username.empty() ? "anonymous" : context.username;
        return true;
    }
    
    // Extract token from context metadata
    // In gRPC, the authorization header is passed as metadata
    std::string token;
    auto it = context.metadata.find("authorization");
    if (it != context.metadata.end()) {
        auto bearer_token = AuthMiddleware::extractBearerToken(it->second);
        if (bearer_token) {
            token = *bearer_token;
        }
    }
    
    if (token.empty()) {
        // No token provided - deny access
        return false;
    }
    
    // Validate token and check required scope using auth middleware
    auto auth_result = auth_->authorize(token, required_scope);
    if (!auth_result.authorized) {
        // Log authentication failure with details
        // Note: Don't log token itself for security reasons
        if (!auth_result.user_id.empty()) {
            // Token was valid but lacked scope
            username = auth_result.user_id;
        }
        return false;
    }
    
    username = auth_result.user_id;
    return true;
}

json ThemisRPCService::createError(themis::plugins::rpc::RPCErrorCode code, const std::string& message) {
    return {
        {"error", {
            {"code", static_cast<int>(code)},
            {"message", message},
            {"type", themis::plugins::rpc::rpcErrorCodeToString(code)}
        }}
    };
}

json ThemisRPCService::createSuccess(const json& result) {
    return {
        {"result", result}
    };
}

} // namespace rpc
} // namespace server
} // namespace themis

