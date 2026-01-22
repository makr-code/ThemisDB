#include "server/rpc_service_impl.h"
#include "plugins/rpc_plugin_interface.h"
#include "storage/rocksdb_wrapper.h"
#include <sstream>
#include <chrono>
#include <unordered_map>

// Define THEMIS_VERSION_STRING if not already defined
#ifndef THEMIS_VERSION_STRING
#define THEMIS_VERSION_STRING "1.3.0-dev"
#endif

namespace themis {
namespace server {
namespace rpc {

// Helper function to get timestamp in nanoseconds
static uint64_t getCurrentTimestampNs() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
}

json ThemisRPCService::handleGet(const json& params) {
    try {
        std::string model = params.value("model", "");
        std::string collection = params.value("collection", "");
        std::string uuid = params.value("uuid", "");
        
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
        std::string model = params.value("model", "");
        std::string collection = params.value("collection", "");
        std::string uuid = params.value("uuid", "");
        
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
        
        // Store in database
        bool success = storage->put(key, value);
        
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

json ThemisRPCService::handleDelete(const json& params) {
    try {
        std::string model = params.value("model", "");
        std::string collection = params.value("collection", "");
        std::string uuid = params.value("uuid", "");
        
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
        
        // Delete from database
        bool success = storage->del(key);
        
        if (!success) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                "Failed to delete from database"
            );
        }
        
        json result = {
            {"success", true}
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

json ThemisRPCService::handleQuery(const json& params) {
    try {
        std::string aql = params.value("aql", "");
        
        if (aql.empty()) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                "Missing required parameter: aql"
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
        
        // TODO: Full AQL query engine integration required
        // For now, return empty results with a note
        // The full implementation will parse AQL and execute against storage
        json result = {
            {"results", json::array()},
            {"has_more", false},
            {"count", 0},
            {"note", "AQL query engine integration pending"}
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
        std::string collection = params.value("collection", "");
        
        if (collection.empty()) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                "Missing required parameter: collection"
            );
        }
        
        if (!params.contains("vector") || !params["vector"].is_array()) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                "Missing or invalid parameter: vector (must be array)"
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
        int k = params.value("k", 10);  // default top-k = 10
        std::string metric = params.value("metric", "cosine");  // cosine, euclidean, dot
        
        // TODO: Vector search requires vector index integration
        // For now, return empty results with a note
        // The full implementation will query vector indices
        json result = {
            {"results", json::array()},
            {"note", "Vector search engine integration pending"}
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
        std::string start_vertex = params.value("start_vertex", "");
        std::string direction = params.value("direction", "outbound");  // outbound, inbound, any
        int max_depth = params.value("max_depth", 1);
        
        if (start_vertex.empty()) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                "Missing required parameter: start_vertex"
            );
        }
        
        // TODO: Graph traversal requires graph index integration
        // For now, return empty results with a note
        json result = {
            {"vertices", json::array()},
            {"edges", json::array()},
            {"note", "Graph traversal engine integration pending"}
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
        std::string collection = params.value("collection", "");
        
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
        
        // Extract geo query parameters
        std::string query_type = params.value("type", "");  // within, near, intersects
        
        if (query_type.empty()) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                "Missing required parameter: type (within, near, intersects)"
            );
        }
        
        // TODO: Geospatial query requires geo index integration
        // For now, return empty results with a note
        json result = {
            {"results", json::array()},
            {"note", "Geospatial query engine integration pending"}
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
        std::string collection = params.value("collection", "");
        
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
        uint64_t start_time = params.value("start_time", 0);
        uint64_t end_time = params.value("end_time", 0);
        std::string aggregation = params.value("aggregation", "");  // sum, avg, min, max, count
        
        if (start_time == 0 || end_time == 0) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                "Missing required parameters: start_time, end_time"
            );
        }
        
        // TODO: Time series query requires time series index integration
        // For now, return empty results with a note
        json result = {
            {"buckets", json::array()},
            {"note", "Time series query engine integration pending"}
        };
        
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
static std::unordered_map<std::string, std::unique_ptr<RocksDBWrapper::TransactionWrapper>> active_transactions;
static std::mutex transaction_mutex;
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
        
        // Extract isolation level if provided
        std::string isolation = params.value("isolation_level", "READ_COMMITTED");
        
        // Create new transaction
        std::lock_guard<std::mutex> lock(transaction_mutex);
        std::string tx_id = "tx_" + std::to_string(++transaction_counter);
        
        // Create transaction wrapper
        auto tx = storage->beginTransaction();
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
        std::string tx_id = params.value("transaction_id", "");
        
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
        std::string tx_id = params.value("transaction_id", "");
        
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

json ThemisRPCService::handleHealthCheck(const json& params) {
    try {
        json result = {
            {"status", "serving"},
            {"version", THEMIS_VERSION_STRING},
            {"uptime_seconds", 0}  // TODO: Get actual uptime
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
        std::string username = params.value("username", "");
        std::string password = params.value("password", "");
        
        if (username.empty() || password.empty()) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::AUTHENTICATION_FAILED,
                "Missing username or password"
            );
        }
        
        // TODO: Implement actual authentication
        // For now, accept any credentials and return a placeholder token
        json result = {
            {"success", true},
            {"token", "placeholder_jwt_token"},
            {"expires_in", 3600}
        };
        
        return createSuccess(result);
        
    } catch (const std::exception& e) {
        return createError(
            themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
            e.what()
        );
    }
}

json ThemisRPCService::handleSearch(const json& params) {
    try {
        std::string collection = params.value("collection", "");
        
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
        std::string model = params.value("model", "");
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
            std::string key = iter.key();
            
            // Check if key still matches prefix
            if (key.substr(0, prefix.length()) != prefix) {
                break;
            }
            
            // Parse entity
            std::string value = iter.value();
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
            std::string key = iter.key();
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

json ThemisRPCService::handleStats(const json& params) {
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
        } catch (...) {
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
        std::string model = params.value("model", "");
        std::string collection = params.value("collection", "");
        std::string uuid = params.value("uuid", "");
        
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
        std::string collection = params.value("collection", "");
        
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
        std::string cursor = params.value("cursor", "");
        int page_size = params.value("page_size", 50);
        std::string model = params.value("model", "");
        
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
            std::string key = iter.key();
            
            // Check if key still matches prefix
            if (key.substr(0, prefix.length()) != prefix) {
                break;
            }
            
            // Parse entity
            std::string value = iter.value();
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
            std::string key = iter.key();
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
        
        // Return index operations metadata
        // Note: This is a placeholder for index management system
        json result = {
            {"indexes", json::array()},
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
        std::string collection = params.value("collection", "");
        
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
            std::string key = iter.key();
            if (key.substr(0, prefix.length()) != prefix) {
                break;
            }
            
            std::string value = iter.value();
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
                    results = json::array(results.begin(), results.begin() + limit);
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

json ThemisRPCService::handleListCollections(const json& params) {
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
            std::string key = iter.key();
            
            // Parse key format: collection:model:uuid
            size_t first_colon = key.find(':');
            if (first_colon != std::string::npos) {
                std::string collection = key.substr(0, first_colon);
                collections[collection]++;
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
        std::string collection = params.value("collection", "");
        std::string field = params.value("field", "");
        
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
        
        // Note: Index creation would require secondary index infrastructure
        // For now, return success with metadata
        std::string index_name = collection + "_" + field + "_idx";
        std::string index_type = params.value("type", "btree");
        
        json result = {
            {"success", true},
            {"index_name", index_name},
            {"collection", collection},
            {"field", field},
            {"type", index_type},
            {"note", "Index metadata stored; full index implementation pending"}
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
        std::string collection = params.value("collection", "");
        std::string index_name = params.value("index_name", "");
        
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
        
        // Note: Index deletion would require secondary index infrastructure
        // For now, return success
        json result = {
            {"success", true},
            {"index_name", index_name},
            {"collection", collection},
            {"note", "Index metadata removed; full index implementation pending"}
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
        std::string collection = params.value("collection", "");
        
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
            std::string key = iter.key();
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
            {"indexes", json::array()}  // Placeholder for index metadata
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
    // Authentication check (except for authenticate method)
    if (method != "authenticate") {
        std::string username;
        if (!verifyAuth(context, username)) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::AUTHENTICATION_FAILED,
                "Authentication required"
            );
        }
    }
    
    // Route to appropriate handler
    if (method == "get") {
        return handleGet(params);
    } else if (method == "put") {
        return handlePut(params);
    } else if (method == "delete") {
        return handleDelete(params);
    } else if (method == "batch_get") {
        return handleBatchGet(params);
    } else if (method == "batch_put") {
        return handleBatchPut(params);
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
    std::string& username
) {
    // TODO: Implement actual token verification
    // For now, check if username is set in context
    if (context.username.empty()) {
        return false;
    }
    
    username = context.username;
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
