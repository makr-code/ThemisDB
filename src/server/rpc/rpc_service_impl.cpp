/**
 * @file rpc_service_impl.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=10, M=25, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/rpc_service_impl.h"
#include <iostream>
#include <stdexcept>
#include "plugins/rpc_plugin_interface.h"
#include "storage/rocksdb_wrapper.h"
#include "index/spatial_index.h"
#include "utils/geo/ewkb.h"
#include "server/auth_middleware.h"
#include <sstream>
#include <chrono>
#include <algorithm>
#include <charconv>
#include <limits>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <thread>
#include <optional>

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
    constexpr size_t kDeadlineCheckInterval = 256;

    std::optional<long long> parseStrictPositiveInteger(const std::string& raw) {
        if (raw.empty()) {
            return std::nullopt;
        }
        long long parsed = 0;
        const char* begin = raw.data();
        const char* end = raw.data() + raw.size();
        const auto [ptr, ec] = std::from_chars(begin, end, parsed);
        if (ec != std::errc{} || ptr != end) {
            return std::nullopt;
        }
        return parsed;
    }

    long long safeCeilDiv(long long value, long long divisor) {
        if (value <= 0) {
            return 0;
        }
        return 1 + ((value - 1) / divisor);
    }

    std::chrono::milliseconds clampMillisFromUnit(long long value, char unit) {
        if (value <= 0) {
            return std::chrono::milliseconds(0);
        }

        constexpr long long kMaxMs = std::numeric_limits<long long>::max();
        auto saturatingMul = [](long long lhs, long long rhs) {
            constexpr long long kMax = std::numeric_limits<long long>::max();
            if (lhs > kMax / rhs) {
                return kMax;
            }
            return lhs * rhs;
        };

        switch (unit) {
            case 'H':
                return std::chrono::milliseconds(saturatingMul(value, 3600000LL));
            case 'M':
                return std::chrono::milliseconds(saturatingMul(value, 60000LL));
            case 'S':
                return std::chrono::milliseconds(saturatingMul(value, 1000LL));
            case 'm':
                return std::chrono::milliseconds(std::min(value, kMaxMs));
            case 'u':
                return std::chrono::milliseconds(std::max(1LL, safeCeilDiv(value, 1000LL)));
            case 'n':
                return std::chrono::milliseconds(std::max(1LL, safeCeilDiv(value, 1000000LL)));
            default:
                return std::chrono::milliseconds(0);
        }
    }

    bool isRetryableMethod(const std::string& method) {
        static const std::unordered_set<std::string> retryable_methods = {
            "get", "batch_get", "search", "query", "paginated_query",
            "vector_search", "graph_traverse", "geo_query", "timeseries_query",
            "get_index_operations", "list_collections", "get_collection_metadata",
            "aggregation_pipeline", "health_check", "stats"
        };
        return retryable_methods.count(method) > 0;
    }

    bool isRetryableErrorResponse(const json& response) {
        if (!response.contains("error") || !response["error"].is_object()) {
            return false;
        }
        const int code = response["error"].value("code", -1);
        return code == static_cast<int>(themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT) ||
               code == static_cast<int>(themis::plugins::rpc::RPCErrorCode::SERVICE_UNAVAILABLE) ||
               code == static_cast<int>(themis::plugins::rpc::RPCErrorCode::RESOURCE_EXHAUSTED);
    }

    std::string currentExceptionMessage(const std::string& fallback) {
        try {
            throw;
        } catch (const std::exception& e) {
            return e.what();
        } catch (...) {
            return fallback;
        }
    }

    std::optional<std::chrono::milliseconds> parseGrpcTimeout(const std::string& timeout) {
        if (static_cast<int>(timeout.size()) < 2) {
            return std::nullopt;
        }

        const char unit = timeout.back();
        const auto value = parseStrictPositiveInteger(timeout.substr(0, static_cast<int>(timeout.size()) - 1));
        if (!value.has_value()) {
            return std::nullopt;
        }

        switch (unit) {
            case 'H':
            [[fallthrough]];\n            case 'M':
            [[fallthrough]];\n            case 'S':
            [[fallthrough]];\n            case 'm':
            [[fallthrough]];\n            case 'u':
            [[fallthrough]];\n            case 'n':
                return clampMillisFromUnit(*value, unit);
            default: return std::nullopt;
        }
    }

    std::optional<std::chrono::milliseconds> parseMillisHeaderValue(const std::string& value) {
        const auto parsed = parseStrictPositiveInteger(value);
        if (!parsed.has_value()) {
            return std::nullopt;
        }
        if (*parsed <= 0) {
            return std::chrono::milliseconds(0);
        }
        return std::chrono::milliseconds(*parsed);
    }

    std::optional<std::chrono::milliseconds> parseRequestTimeout(const themis::plugins::rpc::RPCRequestContext& context) {
        auto grpc_timeout_it = context.metadata.find("grpc-timeout");
        if (grpc_timeout_it != context.metadata.end()) {
            const auto parsed = parseGrpcTimeout(grpc_timeout_it->second);
            if (parsed.has_value()) {
                return parsed;
            }
        }

        auto ms_timeout_it = context.metadata.find("x-timeout-ms");
        if (ms_timeout_it != context.metadata.end()) {
            const auto parsed = parseMillisHeaderValue(ms_timeout_it->second);
            if (parsed.has_value()) {
                return parsed;
            }
        }

        auto request_timeout_it = context.metadata.find("request-timeout-ms");
        if (request_timeout_it != context.metadata.end()) {
            const auto parsed = parseMillisHeaderValue(request_timeout_it->second);
            if (parsed.has_value()) {
                return parsed;
            }
        }

        return std::nullopt;
    }

    using RequestDeadline = std::optional<std::chrono::steady_clock::time_point>;

    RequestDeadline deriveRequestDeadline(
        const themis::plugins::rpc::RPCRequestContext& context,
        const std::optional<std::chrono::milliseconds>& request_timeout
    ) {
        if (!request_timeout.has_value() || context.timestamp_ms == 0) {
            return std::nullopt;
        }

        const auto timeout_count = request_timeout->count();
        if (timeout_count <= 0) {
            return std::chrono::steady_clock::now();
        }

        const auto now_ms = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
        const auto elapsed_ms = (now_ms >= context.timestamp_ms) ? (now_ms - context.timestamp_ms) : 0;
        if (elapsed_ms >= static_cast<uint64_t>(timeout_count)) {
            return std::chrono::steady_clock::now();
        }

        const auto remaining_ms = timeout_count - static_cast<long long>(elapsed_ms);
        return std::chrono::steady_clock::now() + std::chrono::milliseconds(remaining_ms);
    }

    bool isDeadlineExceeded(const RequestDeadline& deadline) {
        return deadline.has_value() && std::chrono::steady_clock::now() >= *deadline;
    }

    bool shouldCheckDeadline([[maybe_unused]] size_t iterations) {
        return iterations > 0 && (iterations % kDeadlineCheckInterval) == 0;
    }

    std::chrono::milliseconds remainingDeadlineBudget(const RequestDeadline& deadline) {
        if (!deadline.has_value()) {
            return std::chrono::milliseconds::max();
        }

        const auto now = std::chrono::steady_clock::now();
        if (now >= *deadline) {
            return std::chrono::milliseconds(0);
        }

        return std::chrono::duration_cast<std::chrono::milliseconds>(*deadline - now);
    }
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
    return handleGetInternal(params, std::nullopt);
}

json ThemisRPCService::handleGetInternal(
    const json& params,
    const std::optional<std::chrono::steady_clock::time_point>& deadline
) {
    if (isDeadlineExceeded(deadline)) {
        return createError(
            themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
            "Request deadline exceeded before get execution"
        );
    }
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
        std::string value = {};
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
    return handlePutInternal(params, std::nullopt);
}

json ThemisRPCService::handlePutInternal(
    const json& params,
    const std::optional<std::chrono::steady_clock::time_point>& deadline
) {
    if (isDeadlineExceeded(deadline)) {
        return createError(
            themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
            "Request deadline exceeded before put execution"
        );
    }
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
    return handleInsertInternal(params, std::nullopt);
}

json ThemisRPCService::handleInsertInternal(
    const json& params,
    const std::optional<std::chrono::steady_clock::time_point>& deadline
) {
    if (isDeadlineExceeded(deadline)) {
        return createError(
            themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
            "Request deadline exceeded before insert execution"
        );
    }
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
    return handleDeleteInternal(params, std::nullopt);
}

json ThemisRPCService::handleDeleteInternal(
    const json& params,
    const std::optional<std::chrono::steady_clock::time_point>& deadline
) {
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
        std::string existing_value = {};
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
        bool deadline_exceeded = false;
        auto find_children = [&](const std::string& p_collection,
                                 const std::string& p_model,
                                 const std::string& p_uuid) -> std::vector<std::string> {
            std::vector<std::string> children;
            std::string scan_prefix = p_collection + ":";
            std::string parent_key  = p_collection + ":" + p_model + ":" + p_uuid;

            auto iter_result = storage->newSafeIterator();
            if (!iter_result) {
              return children;
            }

            auto& iter = iter_result.value();
            iter.Seek(scan_prefix);
            size_t scanned_keys = 0;
            while (iter.Valid()) {
                ++scanned_keys;
                if (shouldCheckDeadline(scanned_keys) && isDeadlineExceeded(deadline)) {
                    deadline_exceeded = true;
                    break;
                }
                std::string iter_key(iter.key());
                if (iter_key.substr(0, scan_prefix.length()) != scan_prefix) {
                  break;
                }
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
        if (deadline_exceeded) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
                "Request deadline exceeded during delete cascade scan"
            );
        }

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
        std::vector<std::string> keys_to_delete = {};

        if (cascade) {
            std::queue<std::string> bfs_queue = {};

            for (const auto& child_key : direct_children) {
                bfs_queue.push(child_key);
                keys_to_delete.push_back(child_key);
            }

            size_t bfs_visited = 0;
            while (!bfs_queue.empty()) {
                ++bfs_visited;
                if (shouldCheckDeadline(bfs_visited) && isDeadlineExceeded(deadline)) {
                    return createError(
                        themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
                        "Request deadline exceeded during delete cascade traversal"
                    );
                }

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
                if (deadline_exceeded) {
                    return createError(
                        themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
                        "Request deadline exceeded during delete cascade scan"
                    );
                }
                for (const auto& gc_key : grandchildren) {
                    bfs_queue.push(gc_key);
                    keys_to_delete.push_back(gc_key);
                }
            }
        }

        // Delete descendants in reverse BFS order (deepest level first)
        size_t deleted_items = 0;
        int deleted_count = 0;
        for (auto it = keys_to_delete.rbegin(); it != keys_to_delete.rend(); ++it) {
            ++deleted_items;
            if (shouldCheckDeadline(deleted_items) && isDeadlineExceeded(deadline)) {
                return createError(
                    themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
                    "Request deadline exceeded during delete cascade write"
                );
            }
            if (!storage->del(*it)) {
                return createError(
                    themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                    "Failed to delete child entity during cascade: " + *it
                );
            }
            ++deleted_count;
        }

        // Delete the target entity itself
        if (isDeadlineExceeded(deadline)) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
                "Request deadline exceeded during delete write"
            );
        }
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
    return handleBatchGetInternal(params, std::nullopt);
}

json ThemisRPCService::handleBatchGetInternal(
    const json& params,
    const std::optional<std::chrono::steady_clock::time_point>& deadline
) {
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
        size_t keys_built = 0;
        for (const auto& key_obj : keys_array) {
            ++keys_built;
            if (shouldCheckDeadline(keys_built) && isDeadlineExceeded(deadline)) {
                return createError(
                    themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
                    "Request deadline exceeded during batch get"
                );
            }
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
        for (size_t i = 0; i <static_cast<int>(values.size()); ++i) {
            if (shouldCheckDeadline(i + 1) && isDeadlineExceeded(deadline)) {
                return createError(
                    themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
                    "Request deadline exceeded during batch get results"
                );
            }
            json result_item = {};
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
            {"count",static_cast<int>(results_array.size())}
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
    return handleBatchPutInternal(params, std::nullopt);
}

json ThemisRPCService::handleBatchPutInternal(
    const json& params,
    const std::optional<std::chrono::steady_clock::time_point>& deadline
) {
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
        size_t item_index = 0;
        
        for (const auto& item : entities_array) {
            ++item_index;
            if (shouldCheckDeadline(item_index) && isDeadlineExceeded(deadline)) {
                return createError(
                    themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
                    "Request deadline exceeded during batch put"
                );
            }
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
    return handleBatchDeleteInternal(params, std::nullopt);
}

json ThemisRPCService::handleBatchDeleteInternal(
    const json& params,
    const std::optional<std::chrono::steady_clock::time_point>& deadline
) {
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
        size_t item_index = 0;

        for (const auto& key_obj : keys_array) {
            ++item_index;
            if (shouldCheckDeadline(item_index) && isDeadlineExceeded(deadline)) {
                return createError(
                    themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
                    "Request deadline exceeded during batch delete"
                );
            }
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
    return handleQueryInternal(params, std::nullopt);
}

json ThemisRPCService::handleQueryInternal(
    const json& params,
    const std::optional<std::chrono::steady_clock::time_point>& deadline
) {
    try {
        std::string aql = {};
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
            size_t scanned_keys = 0;
            json results = json::array();

            iter.Seek(prefix);
            while (iter.Valid()) {
                ++scanned_keys;
                if (shouldCheckDeadline(scanned_keys) && isDeadlineExceeded(deadline)) {
                    return createError(
                        themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
                        "Request deadline exceeded during query collection scan"
                    );
                }

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

            json result = {};
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
    return handleVectorSearchInternal(params, std::nullopt);
}

json ThemisRPCService::handleVectorSearchInternal(
    const json& params,
    const std::optional<std::chrono::steady_clock::time_point>& deadline
) {
    try {
        if (isDeadlineExceeded(deadline)) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
                "Request deadline exceeded before vector search execution"
            );
        }

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
    return handleGraphTraverseInternal(params, std::nullopt);
}

json ThemisRPCService::handleGraphTraverseInternal(
    const json& params,
    const std::optional<std::chrono::steady_clock::time_point>& deadline
) {
    try {
        if (isDeadlineExceeded(deadline)) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
                "Request deadline exceeded before graph traversal execution"
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
    return handleGeoQueryInternal(params, std::nullopt);
}

json ThemisRPCService::handleGeoQueryInternal(
    const json& params,
    const std::optional<std::chrono::steady_clock::time_point>& deadline
) {
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
        auto& spatial_index = *spatial_index_;
        
        // Verify that the collection has a spatial index
        if (!spatial_index.hasSpatialIndex(collection)) {
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
            auto search_results = spatial_index.searchIntersects(collection, query_bbox);
            
            // Convert results to JSON
            size_t result_count = 0;
            for (const auto& result : search_results) {
                ++result_count;
                if (shouldCheckDeadline(result_count) && isDeadlineExceeded(deadline)) {
                    return createError(
                        themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
                        "Request deadline exceeded during geo query results"
                    );
                }
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
            size_t result_count = 0;
            for (const auto& result : search_results) {
                ++result_count;
                if (shouldCheckDeadline(result_count) && isDeadlineExceeded(deadline)) {
                    return createError(
                        themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
                        "Request deadline exceeded during geo query results"
                    );
                }
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
            {"count",static_cast<int>(results.size())},
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
    return handleTimeSeriesQueryInternal(params, std::nullopt);
}

json ThemisRPCService::handleTimeSeriesQueryInternal(
    const json& params,
    const std::optional<std::chrono::steady_clock::time_point>& deadline
) {
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
        if (start_time == 0) {
            start_time = params.value("start_ts", static_cast<uint64_t>(0));
        }
        if (end_time == 0) {
            end_time = params.value("end_ts", static_cast<uint64_t>(0));
        }
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
        size_t scanned_keys = 0;

        // Aggregation accumulators
        double agg_sum = 0.0;
        double agg_min = std::numeric_limits<double>::max();
        double agg_max = std::numeric_limits<double>::lowest();
        int agg_count = 0;

        iter.Seek(prefix);
        while (iter.Valid() && count < limit) {
            ++scanned_keys;
            if (isDeadlineExceeded(deadline)) {
                return createError(
                    themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
                    "Request deadline exceeded during time series collection scan"
                );
            }

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
            if (aggregation == "sum") {
              agg_result_val = agg_sum;
            }
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
    return handleTransactionBeginInternal(params, std::nullopt);
}

json ThemisRPCService::handleTransactionBeginInternal(
    const json& params,
    const std::optional<std::chrono::steady_clock::time_point>& deadline
) {
    if (isDeadlineExceeded(deadline)) {
        return createError(
            themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
            "Request deadline exceeded before transaction_begin execution"
        );
    }
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
    return handleTransactionCommitInternal(params, std::nullopt);
}

json ThemisRPCService::handleTransactionCommitInternal(
    const json& params,
    const std::optional<std::chrono::steady_clock::time_point>& deadline
) {
    if (isDeadlineExceeded(deadline)) {
        return createError(
            themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
            "Request deadline exceeded before transaction_commit execution"
        );
    }
    try {
        std::string tx_id = {};
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
        auto node = active_transactions.extract(tx_id);
        if (node.empty()) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                "Transaction not found: " + tx_id
            );
        }
        
        // Commit the transaction
        bool success = node.mapped()->commit();
        
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
    return handleTransactionAbortInternal(params, std::nullopt);
}

json ThemisRPCService::handleTransactionAbortInternal(
    const json& params,
    const std::optional<std::chrono::steady_clock::time_point>& deadline
) {
    if (isDeadlineExceeded(deadline)) {
        return createError(
            themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
            "Request deadline exceeded before transaction_abort execution"
        );
    }
    try {
        std::string tx_id = {};
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
        auto node = active_transactions.extract(tx_id);
        if (node.empty()) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS,
                "Transaction not found: " + tx_id
            );
        }
        
        // Rollback the transaction
        node.mapped()->rollback();
        
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
        std::string username = {};
        std::string password = {};
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
    return handleSearchInternal(params, std::nullopt);
}

json ThemisRPCService::handleSearchInternal(
    const json& params,
    const std::optional<std::chrono::steady_clock::time_point>& deadline
) {
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

        if (isDeadlineExceeded(deadline)) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
                "Request deadline exceeded during search setup"
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
        size_t scanned_keys = 0;
        
        // Scan keys with prefix
        iter.Seek(prefix);
        while (iter.Valid() && count < limit) {
            ++scanned_keys;
            if (isDeadlineExceeded(deadline)) {
                return createError(
                    themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
                    "Request deadline exceeded during search collection scan"
                );
            }

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
    return handleStatsInternal(params, std::nullopt);
}

json ThemisRPCService::handleStatsInternal(
    [[maybe_unused]] const json& params,
    const std::optional<std::chrono::steady_clock::time_point>& deadline
) {
    if (isDeadlineExceeded(deadline)) {
        return createError(
            themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
            "Request deadline exceeded before stats execution"
        );
    }
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
        } catch (const std::exception& e) {
            std::cerr << "[ThemisRPCService] getStats error: " << e.what() << "\n";
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
    return handleUpdateEntityInternal(params, std::nullopt);
}

json ThemisRPCService::handleUpdateEntityInternal(
    const json& params,
    const std::optional<std::chrono::steady_clock::time_point>& deadline
) {
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

        if (isDeadlineExceeded(deadline)) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
                "Request deadline exceeded during update entity"
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
        std::string value = {};
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
        size_t updated_fields = 0;
        for (auto& [field, new_value] : updates.items()) {
            ++updated_fields;
            if (shouldCheckDeadline(updated_fields) && isDeadlineExceeded(deadline)) {
                return createError(
                    themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
                    "Request deadline exceeded during update entity merge"
                );
            }
            entity[field] = new_value;
        }

        if (isDeadlineExceeded(deadline)) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
                "Request deadline exceeded during update entity write"
            );
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
    return handleBatchUpdateInternal(params, std::nullopt);
}

json ThemisRPCService::handleBatchUpdateInternal(
    const json& params,
    const std::optional<std::chrono::steady_clock::time_point>& deadline
) {
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
        size_t item_index = 0;
        
        for (const auto& update_item : updates_array) {
            ++item_index;
            if (shouldCheckDeadline(item_index) && isDeadlineExceeded(deadline)) {
                return createError(
                    themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
                    "Request deadline exceeded during batch update"
                );
            }

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
            std::string value = {};
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
    return handlePaginatedQueryInternal(params, std::nullopt);
}

json ThemisRPCService::handlePaginatedQueryInternal(
    const json& params,
    const std::optional<std::chrono::steady_clock::time_point>& deadline
) {
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

        if (isDeadlineExceeded(deadline)) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
                "Request deadline exceeded during paginated query setup"
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
        size_t scanned_keys = 0;
        std::string next_cursor = {};
        while (iter.Valid() && count < page_size) {
            ++scanned_keys;
            if (isDeadlineExceeded(deadline)) {
                return createError(
                    themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
                    "Request deadline exceeded during paginated query scan"
                );
            }

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
    return handleGetIndexOperationsInternal(params, std::nullopt);
}

json ThemisRPCService::handleGetIndexOperationsInternal(
    const json& params,
    const std::optional<std::chrono::steady_clock::time_point>& deadline
) {
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
        size_t scanned_keys = 0;
        bool deadline_exceeded = false;

        storage->scanPrefix(prefix, [&](std::string_view /*key*/, std::string_view value) -> bool {
            ++scanned_keys;
            if (shouldCheckDeadline(scanned_keys) && isDeadlineExceeded(deadline)) {
                deadline_exceeded = true;
                return false; // abort scan
            }
            try {
                json idx_meta = json::parse(value);
                indexes.push_back(idx_meta);
            } catch (const json::exception&) {
                // Skip malformed entries
            }
            return true; // continue scanning
        });

        if (deadline_exceeded) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
                "Request deadline exceeded during index operations scan"
            );
        }

        json result = {
            {"indexes", indexes},
            {"count",static_cast<int>(indexes.size())},
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
    return handleAggregationPipelineInternal(params, std::nullopt);
}

json ThemisRPCService::handleAggregationPipelineInternal(
    const json& params,
    const std::optional<std::chrono::steady_clock::time_point>& deadline
) {
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
        size_t scanned_documents = 0;
        
        iter.Seek(prefix);
        while (iter.Valid()) {
            ++scanned_documents;
            if (shouldCheckDeadline(scanned_documents) && isDeadlineExceeded(deadline)) {
                return createError(
                    themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
                    "Request deadline exceeded during aggregation collection scan"
                );
            }

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
                size_t matched_documents = 0;
                for (const auto& doc : results) {
                    ++matched_documents;
                    if (shouldCheckDeadline(matched_documents) && isDeadlineExceeded(deadline)) {
                        return createError(
                            themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
                            "Request deadline exceeded during aggregation pipeline execution"
                        );
                    }

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
                if (static_cast<int>(results.size()) > static_cast<size_t>(limit)) {
                    json limited = json::array();
                    for (size_t i = 0; i < static_cast<size_t>(limit); ++i) {
                        limited.push_back(results[i]);
                    }
                    results = limited;
                }
            } else if (stage_name == "$project") {
                // Project fields
                json projected = json::array();
                size_t projected_documents = 0;
                for (const auto& doc : results) {
                    ++projected_documents;
                    if (shouldCheckDeadline(projected_documents) && isDeadlineExceeded(deadline)) {
                        return createError(
                            themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
                            "Request deadline exceeded during aggregation pipeline execution"
                        );
                    }

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
            {"count",static_cast<int>(results.size())}
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
    return handleListCollectionsInternal(params, std::nullopt);
}

json ThemisRPCService::handleListCollectionsInternal(
    [[maybe_unused]] const json& params,
    const std::optional<std::chrono::steady_clock::time_point>& deadline
) {
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
        size_t scanned_keys = 0;
        
        iter.SeekToFirst();
        while (iter.Valid()) {
            ++scanned_keys;
            if (shouldCheckDeadline(scanned_keys) && isDeadlineExceeded(deadline)) {
                return createError(
                    themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
                    "Request deadline exceeded during collection listing"
                );
            }

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
            {"count",static_cast<int>(collections.size())}
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
    return handleCreateIndexInternal(params, std::nullopt);
}

json ThemisRPCService::handleCreateIndexInternal(
    const json& params,
    const std::optional<std::chrono::steady_clock::time_point>& deadline
) {
    if (isDeadlineExceeded(deadline)) {
        return createError(
            themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
            "Request deadline exceeded before create_index execution"
        );
    }
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
    return handleDropIndexInternal(params, std::nullopt);
}

json ThemisRPCService::handleDropIndexInternal(
    const json& params,
    const std::optional<std::chrono::steady_clock::time_point>& deadline
) {
    if (isDeadlineExceeded(deadline)) {
        return createError(
            themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
            "Request deadline exceeded before drop_index execution"
        );
    }
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
        std::string existing = {};
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
    return handleGetCollectionMetadataInternal(params, std::nullopt);
}

json ThemisRPCService::handleGetCollectionMetadataInternal(
    const json& params,
    const std::optional<std::chrono::steady_clock::time_point>& deadline
) {
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
        size_t scanned_documents = 0;
        
        iter.Seek(prefix);
        while (iter.Valid()) {
            ++scanned_documents;
            if (shouldCheckDeadline(scanned_documents) && isDeadlineExceeded(deadline)) {
                return createError(
                    themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
                    "Request deadline exceeded during collection metadata scan"
                );
            }

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
        
        json idx_array = json::array();
        if (storage) {
            std::string idx_prefix = "_idx_meta:" + collection + ":";
            size_t scanned_index_metadata = 0;
            bool deadline_exceeded = false;
            storage->scanPrefix(
                idx_prefix,
                [&](std::string_view /*key*/, std::string_view value) -> bool {
                    ++scanned_index_metadata;
                    if (shouldCheckDeadline(scanned_index_metadata) && isDeadlineExceeded(deadline)) {
                        deadline_exceeded = true;
                        return false;
                    }
                    try {
                        idx_array.push_back(json::parse(value));
                    } catch (const json::exception&) {
                        // Skip malformed index metadata entries
                    }
                    return true;
                });
            if (deadline_exceeded) {
                return createError(
                    themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
                    "Request deadline exceeded during collection metadata index scan"
                );
            }
        }

        json result = {
            {"collection", collection},
            {"document_count", document_count},
            {"total_size_bytes", total_size},
            {"models", models_array},
            {"indexes", idx_array}
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
    auto request_timeout = parseRequestTimeout(context);
    if (request_timeout.has_value() && context.timestamp_ms > 0) {
        const auto timeout_count = request_timeout->count();
        if (timeout_count <= 0) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
                "Request deadline exceeded before dispatch"
            );
        }

        const auto now_ms = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
        const auto elapsed_ms = (now_ms >= context.timestamp_ms) ? (now_ms - context.timestamp_ms) : 0;
        if (elapsed_ms >= static_cast<uint64_t>(timeout_count)) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
                "Request deadline exceeded before dispatch"
            );
        }
    }
    const auto request_deadline = deriveRequestDeadline(context, request_timeout);

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

    auto dispatch_once = [&]() -> json {
        if (method == "get") {
            return handleGetInternal(params, request_deadline);
        } else if (method == "put") {
            return handlePutInternal(params, request_deadline);
        } else if (method == "insert") {
            return handleInsertInternal(params, request_deadline);
        } else if (method == "delete") {
            return handleDeleteInternal(params, request_deadline);
        } else if (method == "batch_get") {
            return handleBatchGetInternal(params, request_deadline);
        } else if (method == "batch_put") {
            return handleBatchPutInternal(params, request_deadline);
        } else if (method == "batch_delete") {
            return handleBatchDeleteInternal(params, request_deadline);
        } else if (method == "query") {
            return handleQueryInternal(params, request_deadline);
        } else if (method == "vector_search") {
            return handleVectorSearchInternal(params, request_deadline);
        } else if (method == "graph_traverse") {
            return handleGraphTraverseInternal(params, request_deadline);
        } else if (method == "geo_query") {
            return handleGeoQueryInternal(params, request_deadline);
        } else if (method == "timeseries_query") {
            return handleTimeSeriesQueryInternal(params, request_deadline);
        } else if (method == "transaction_begin") {
            return handleTransactionBeginInternal(params, request_deadline);
        } else if (method == "transaction_commit") {
            return handleTransactionCommitInternal(params, request_deadline);
        } else if (method == "transaction_abort") {
            return handleTransactionAbortInternal(params, request_deadline);
        } else if (method == "health_check") {
            return handleHealthCheck(params);
        } else if (method == "authenticate") {
            return handleAuthenticate(params);
        } else if (method == "search") {
            return handleSearchInternal(params, request_deadline);
        } else if (method == "stats") {
            return handleStatsInternal(params, request_deadline);
        } else if (method == "update_entity") {
            return handleUpdateEntityInternal(params, request_deadline);
        } else if (method == "batch_update") {
            return handleBatchUpdateInternal(params, request_deadline);
        } else if (method == "paginated_query") {
            return handlePaginatedQueryInternal(params, request_deadline);
        } else if (method == "get_index_operations") {
            return handleGetIndexOperationsInternal(params, request_deadline);
        } else if (method == "aggregation_pipeline") {
            return handleAggregationPipelineInternal(params, request_deadline);
        } else if (method == "list_collections") {
            return handleListCollectionsInternal(params, request_deadline);
        } else if (method == "create_index") {
            return handleCreateIndexInternal(params, request_deadline);
        } else if (method == "drop_index") {
            return handleDropIndexInternal(params, request_deadline);
        } else if (method == "get_collection_metadata") {
            return handleGetCollectionMetadataInternal(params, request_deadline);
        }

        return createError(
            themis::plugins::rpc::RPCErrorCode::METHOD_NOT_FOUND,
            "Method not found: " + method
        );
    };

    const bool retryable_method = isRetryableMethod(method);
    const int max_attempts = retryable_method ? 3 : 1;
    for (int attempt = 1; attempt <= max_attempts; ++attempt) {
        if (isDeadlineExceeded(request_deadline)) {
            return createError(
                themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
                "Request deadline exceeded before dispatch retry"
            );
        }

        try {
            json response = dispatch_once();
            if (!retryable_method || !isRetryableErrorResponse(response) || attempt == max_attempts) {
                return response;
            }
        } catch (const std::exception& e) {
            if (!retryable_method || attempt == max_attempts) {
                return createError(
                    themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                    e.what()
                );
            }
            std::cerr << "[ThemisRPCService] Retrying method '" << method << "' after exception"
                      << " (attempt " << attempt << "/" << max_attempts << "): " << e.what() << "\n";
        } catch (...) {
            const std::string error_message =
                currentExceptionMessage("Unknown internal error during RPC dispatch");
            if (!retryable_method || attempt == max_attempts) {
                return createError(
                    themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,
                    error_message
                );
            }
            std::cerr << "[ThemisRPCService] Retrying method '" << method << "' after unknown exception"
                      << " (attempt " << attempt << "/" << max_attempts << "): " << error_message << "\n";
        }

        const auto backoff = std::chrono::milliseconds(10 * attempt);
        if (request_deadline.has_value()) {
            const auto remaining = remainingDeadlineBudget(request_deadline);
            if (remaining <= std::chrono::milliseconds(0)) {
                return createError(
                    themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,
                    "Request deadline exceeded before dispatch retry"
                );
            }
            std::this_thread::sleep_for(std::min(backoff, remaining));
        } else {
            std::this_thread::sleep_for(backoff);
        }
    }

    return createError(
        themis::plugins::rpc::RPCErrorCode::SERVICE_UNAVAILABLE,
        "Retry budget exhausted"
    );
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
    auto& auth = *auth_;
    
    // If auth middleware is configured but not enabled, allow unauthenticated access
    // for development/testing. In production, auth should always be enabled.
    if (!auth.isEnabled()) {
        username = context.username.empty() ? "anonymous" : context.username;
        return true;
    }
    
    // Extract token from context metadata
    // In gRPC, the authorization header is passed as metadata
    std::string token = {};
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
    auto auth_result = auth.authorize(token, required_scope);
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

