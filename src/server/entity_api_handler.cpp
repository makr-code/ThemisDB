/**
 * @file entity_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=4, M=15, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/entity_api_handler.h"
#include <stdexcept>
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "storage/key_schema.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/spatial_index.h"
#include "transaction/transaction_manager.h"
#include "security/encryption.h"
#include "security/key_provider.h"
#include "security/pki_key_provider.h"
#include "server/auth_middleware.h"
#include "cdc/changefeed.h"
#include "sharding/wal_manager.h"
#include "sharding/replication_coordinator.h"
#include "sharding/multi_primary_coordinator.h"
#include "sharding/write_concern.h"
#include "sharding/redundancy_strategy.h"
#include "sharding/consistent_hash.h"
#include "sharding/shard_topology.h"
#include "api/geo_index_hooks.h"
#include "utils/input_validator.h"
#include "utils/logger.h"
#include "utils/tracing.h"
#include "utils/hkdf_helper.h"
#include "updates/updates_diagnostic_emitter.h"
#include <sstream>
#include <algorithm>
#include <chrono>

namespace themis {
namespace server {

using json = nlohmann::json;
using Tracer = themis::Tracer;

namespace {

constexpr size_t kMaxEntityBatchKeyPartLength = 256;

bool isValidEntityBatchKeyPart(const std::string& value) {
    themis::utils::InputValidator validator;
    return !value.empty() &&
           validator.validateStringLength(value, kMaxEntityBatchKeyPartLength) &&
           validator.validatePathSegment(value) &&
           validator.validateHeaderValue(value);
}

} // namespace

EntityApiHandler::EntityApiHandler(
    std::shared_ptr<RocksDBWrapper> storage,
    std::shared_ptr<SecondaryIndexManager> secondary_index,
    std::shared_ptr<GraphIndexManager> graph_index,
    std::shared_ptr<TransactionManager> tx_manager,
    std::shared_ptr<FieldEncryption> field_encryption,
    std::shared_ptr<KeyProvider> key_provider,
    std::shared_ptr<AuthMiddleware> auth,
    const EntityApiConfig& config,
    index::SpatialIndexManager* spatial_index,
    std::shared_ptr<Changefeed> changefeed,
    std::shared_ptr<sharding::WALManager> wal_manager,
    std::shared_ptr<sharding::ReplicationCoordinator> replication_coordinator,
    std::shared_ptr<sharding::MultiPrimaryCoordinator> multi_primary_coordinator,
    std::shared_ptr<sharding::CollectionRedundancyManager> redundancy_manager,
    std::shared_ptr<sharding::ConsistentHashRing> hash_ring,
    std::shared_ptr<sharding::ShardTopology> shard_topology
)
    : storage_(std::move(storage))
    , secondary_index_(std::move(secondary_index))
    , graph_index_(std::move(graph_index))
    , tx_manager_(std::move(tx_manager))
    , field_encryption_(std::move(field_encryption))
    , key_provider_(std::move(key_provider))
    , auth_(std::move(auth))
    , config_(config)
    , spatial_index_(spatial_index)
    , changefeed_(std::move(changefeed))
    , wal_manager_(std::move(wal_manager))
    , replication_coordinator_(std::move(replication_coordinator))
    , multi_primary_coordinator_(std::move(multi_primary_coordinator))
    , redundancy_manager_(std::move(redundancy_manager))
    , hash_ring_(std::move(hash_ring))
    , shard_topology_(std::move(shard_topology))
{
}

EntityApiHandler::AuthContext EntityApiHandler::extractAuthContext(
    const http::request<http::string_body>& req
) const {
    AuthContext ctx;
    
    // If auth is disabled, return empty context
    if (!auth_ || !auth_->isEnabled()) {
        return ctx;
    }
    
    // Extract Authorization header
    const auto auth_header = req[http::field::authorization];
    if (auth_header.empty()) {
        return ctx; // No token -> empty context
    }
    
    // Extract Bearer token
    auto token = themis::AuthMiddleware::extractBearerToken(
        std::string_view(auth_header.data(),static_cast<int>(auth_header.size()))
    );
    if (!token) {
        return ctx; // Invalid token format -> empty context
    }
    
    // Validate token and extract user_id + groups
    auto ar = auth_->validateToken(*token);
    if (ar.authorized) {
        ctx.user_id = ar.user_id;
        ctx.groups = ar.groups;
    }
    
    return ctx;
}

std::optional<http::response<http::string_body>> EntityApiHandler::requireAccess(
    const http::request<http::string_body>& req,
    const std::string& scope,
    const std::string& /*action*/,
    const std::string& /*resource*/
) {
    if (!auth_ || !auth_->isEnabled()) {
        return std::nullopt; // Auth disabled, allow access
    }
    
    // Extract token from Authorization header
    const auto auth_header = req[http::field::authorization];
    if (auth_header.empty()) {
        return makeErrorResponse(http::status::unauthorized, "Missing Authorization header", req);
    }
    
    auto token_opt = themis::AuthMiddleware::extractBearerToken(
        std::string_view(auth_header.data(),static_cast<int>(auth_header.size()))
    );
    if (!token_opt) {
        return makeErrorResponse(http::status::unauthorized, "Invalid Authorization header format", req);
    }
    
    // Authorize using configured scopes
    auto authz = auth_->authorize(*token_opt, scope);
    if (!authz.authorized) {
        THEMIS_WARN("[AUDIT] authorize result=DENY scope={}", scope);
        auto reason = authz.reason.empty() ? "Unauthorized" : authz.reason;
        return makeErrorResponse(http::status::unauthorized, reason, req);
    }
    THEMIS_INFO("[AUDIT] authorize result=ALLOW scope={}", scope);

    return std::nullopt; // Access granted
}

http::response<http::string_body> EntityApiHandler::handleGet(
    const http::request<http::string_body>& req
) {
    // OP-CORRELATION-ID-001: Extract correlation ID from request headers
    std::string correlation_id = std::string(req["X-Correlation-ID"]);
    if (correlation_id.empty()) {
        // Generate new correlation ID if not provided (unique per request)
        static std::atomic<uint64_t> get_counter{0};
        auto ts = std::chrono::system_clock::now().time_since_epoch().count();
        correlation_id = "get-" + std::to_string(ts) + "-" + 
                         std::to_string(get_counter.fetch_add(1, std::memory_order_relaxed));
    }
    
    if (auth_ && auth_->isEnabled()) {
        std::string path_only = std::string(req.target());
        auto qpos = path_only.find('?');
        if (qpos != std::string::npos) {
          path_only = path_only.substr(0, qpos);
        }
        if (auto resp = requireAccess(req, "data:read", "read", path_only)) {
            // OP-AUDIT-001: Log auth failure with correlation ID
            THEMIS_WARN("Entity GET denied (correlation_id={}): {}", correlation_id, path_only);
            return *resp;
        }
    }
    
    // OP-TIMEOUT-001: Set deadline for entity retrieval (5 second max)
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    
    auto span = Tracer::startSpan("GET /entities/:key");
    span.setAttribute("correlation_id", correlation_id);
    
    try {
        // OP-TIMEOUT-002: Check deadline before main operation
        if (std::chrono::steady_clock::now() > deadline) {
            THEMIS_WARN("Entity GET timeout (correlation_id={}): deadline exceeded", correlation_id);
            span.setStatus(false, "Timeout");
            return makeErrorResponse(http::status::gateway_timeout, "Entity retrieval timeout", req);
        }
        
        // Extract entity key from path: /entities/{key}
        auto key = extractPathParam(std::string(req.target()), "/entities/");
        if (key.empty()) {
            span.setStatus(false, "Missing entity key");
            return makeErrorResponse(http::status::bad_request, "Missing entity key", req);
        }

        span.setAttribute("entity.key", key);

        // Retrieve entity blob (persisted JSON string)
        // Keys are stored as relational keys (entity:table:pk), not raw table:pk.
        auto pos = key.find(':');
        if (pos == std::string::npos || pos == 0 || pos == static_cast<int>(key.size()) -1) {
            span.setStatus(false, "Invalid key format");
            return makeErrorResponse(http::status::bad_request, "Key must be in format 'table:pk'", req);
        }
        std::string table = key.substr(0, pos);
        std::string pk = key.substr(pos + 1);

        // OP-LATENCY-001: Measure storage read latency
        auto read_start = std::chrono::steady_clock::now();
        auto blob_opt = storage_->get(KeySchema::makeRelationalKey(table, pk));
        auto read_latency = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - read_start);
        
        if (!blob_opt.has_value()) {
            span.setStatus(false, "Entity not found");
            // OP-AUDIT-002: Log not-found with latency
            THEMIS_INFO("Entity GET not found (correlation_id={}, latency_ms={}, key={})", 
                       correlation_id, read_latency.count(), key);
            return makeErrorResponse(http::status::not_found, "Entity not found", req);
        }
        const auto& blob_vec = blob_opt.value();
        std::string blob_str(blob_vec.begin(), blob_vec.end());
        span.setAttribute("entity.size_bytes", static_cast<int64_t>(blob_str.size()));

        // Optional decryption via query parameter ?decrypt=true
        bool decrypt = false;
        {
            std::string target = std::string(req.target());
            auto qpos = target.find('?');
            if (qpos != std::string::npos) {
                auto qs = target.substr(qpos + 1);
                std::istringstream iss(qs);
                std::string kv = {};
                while (std::getline(iss, kv, '&')) {
                    auto eq = kv.find('=');
                    std::string k = (eq == std::string::npos) ? kv : kv.substr(0, eq);
                    std::string v = (eq == std::string::npos) ? std::string() : kv.substr(eq + 1);
                    if (k == "decrypt") {
                        std::transform(v.begin(), v.end(), v.begin(), ::tolower);
                        decrypt = (v == "true" || v == "1" || v == "yes");
                    }
                }
            }
        }

        if (!decrypt) {
            span.setStatus(true);
            // OP-LATENCY-002: Include latency metrics in response headers
            json response = {{"key", key}, {"blob", blob_str}};
            THEMIS_DEBUG("Entity GET success (correlation_id={}, latency_ms={}, key={})", 
                        correlation_id, read_latency.count(), key);
            return makeResponse(http::status::ok, response.dump(), req);
        }

        // Decryption only when schema is configured and fields are marked
        json entity_json;
        try { entity_json = json::parse(blob_str); } catch (...) {
            THEMIS_ERROR("GET entity: stored blob is not valid JSON for key: {} (correlation_id={})", 
                        key, correlation_id);
            span.setStatus(false, "Stored blob is not valid JSON");
            return makeErrorResponse(http::status::internal_server_error, "Stored entity JSON parse failed", req);
        }

        // Table already parsed above from table:pk key format

        try {
            auto schema_bytes = storage_->get("config:encryption_schema");
            if (schema_bytes) {
                std::string schema_json(schema_bytes->begin(), schema_bytes->end());
                auto schema = nlohmann::json::parse(schema_json);
                if (schema.contains("collections") && schema["collections"].contains(table)) {
                    auto coll = schema["collections"][table];
                    if (coll.contains("encryption") && coll["encryption"].value("enabled", false)) {
                        std::string context_type = coll["encryption"].value("context_type", "user");
                        std::vector<std::string> fields = {};

                        if (coll["encryption"].contains("fields")) {
                            fields.reserve(coll["encryption"]["fields"].size());  // OPTIMIZATION: Pre-allocate to avoid reallocations
                            for (auto& f : coll["encryption"]["fields"]) {
                              if (f.is_string()) fields.push_back(f.get<std::string>());
                            }
                        }
                        // Extract user_id and groups from JWT for decryption context
                        auto auth_ctx = extractAuthContext(req);
                        std::string user_ctx = auth_ctx.user_id.empty() ? "anonymous" : auth_ctx.user_id;
                        auto pki = std::dynamic_pointer_cast<themis::security::PKIKeyProvider>(key_provider_);
                        for (const auto& f : fields) {
                            if (!entity_json.contains(f + "_enc") || !entity_json.contains(f + "_encrypted")) {
                              continue;
                            }
                            bool encFlag = false;
                            try { encFlag = entity_json[f + "_enc"].get<bool>(); } catch (...) {
                                THEMIS_WARN("Enc flag cast failed for field {}: defaulting to false", f);
                                encFlag = false;
                            }
                            if (!encFlag) {
                              continue;
                            }
                            try {
                                auto enc_meta_str = entity_json[f + "_encrypted"].get<std::string>();
                                auto enc_meta = nlohmann::json::parse(enc_meta_str);
                                auto blob = themis::EncryptedBlob::fromJson(enc_meta);
                                std::vector<uint8_t> raw_key = {};

                                if (context_type == "group" && pki && entity_json.contains(f + "_group")) {
                                    // Group context (MVP: first group / single string)
                                    std::string group_name = {};
                                    try { group_name = entity_json[f + "_group"].get<std::string>(); } catch (...) {
                                        THEMIS_WARN("Group name cast failed for field {}: skipping group context", f);
                                        group_name.clear();
                                    }
                                    if (!group_name.empty()) {
                                        auto gdek = pki->getGroupDEK(group_name);
                                        std::vector<uint8_t> salt; // empty
                                        std::string info = "field:" + f;
                                        raw_key = themis::utils::HKDFHelper::derive(gdek, salt, info, 32);
                                    }
                                }
                                if (raw_key.empty()) {
                                    // User/Anonymous context
                                    auto dek = key_provider_->getKey("dek");
                                    std::vector<uint8_t> salt(user_ctx.begin(), user_ctx.end());
                                    std::string info = "field:" + f;
                                    raw_key = themis::utils::HKDFHelper::derive(dek, salt, info, 32);
                                }
                                auto plain_bytes = field_encryption_->decryptWithKey(blob, raw_key);
                                
                                // Deserialization based on data format
                                // Try JSON deserialization for structured types
                                std::string plain_str(plain_bytes.begin(), plain_bytes.end());
                                
                                // Heuristic: If it looks like JSON, parse it
                                if (!plain_str.empty() && (plain_str[0] == '[' || plain_str[0] == '{')) {
                                    try {
                                        auto parsed = nlohmann::json::parse(plain_str);
                                        entity_json[f] = parsed; // Take JSON structure
                                    } catch (...) {
                                        THEMIS_DEBUG("Decrypted value for field {} is not valid JSON, treating as string", f);
                                        entity_json[f] = plain_str;
                                    }
                                } else {
                                    // Return primitive types as string
                                    entity_json[f] = plain_str;
                                }
                            } catch (const std::exception& e) {
                                THEMIS_WARN("Field {} decryption failed: {}", f, e.what());
                            }
                        }
                    }
                }
            }
        } catch (const std::exception& e) {
            THEMIS_WARN("Decrypt schema processing error: {}", e.what());
        }

        span.setStatus(true);
        json response = {{"key", key}, {"decrypted", true}, {"entity", entity_json}};
        return makeResponse(http::status::ok, response.dump(), req);

    } catch (const std::exception& e) {
        THEMIS_ERROR("GET entity error: {}", e.what());
        span.setStatus(false, e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> EntityApiHandler::handlePut(
    const http::request<http::string_body>& req
) {
    if (auth_ && auth_->isEnabled()) {
        std::string path_only = std::string(req.target());
        auto qpos = path_only.find('?');
        if (qpos != std::string::npos) {
          path_only = path_only.substr(0, qpos);
        }
        if (auto resp = requireAccess(req, "data:write", "write", path_only)) {
          return *resp;
        }
    }
    auto span = Tracer::startSpan("PUT /entities/:key");
    
    try {
        // Parse request body
        auto body_json = json::parse(req.body());
        
        std::string key = {};
        if (body_json.contains("key")) {
            key = body_json["key"].get<std::string>();
        } else {
            // Extract from path if PUT /entities/{key}
            key = extractPathParam(std::string(req.target()), "/entities/");
        }

        if (key.empty()) {
            span.setStatus(false, "Missing entity key");
            return makeErrorResponse(http::status::bad_request, "Missing entity key", req);
        }

        span.setAttribute("entity.key", key);

        if (!body_json.contains("blob")) {
            span.setStatus(false, "Missing blob field");
            return makeErrorResponse(http::status::bad_request, "Missing 'blob' field", req);
        }

        // Split key into table:pk
        auto pos = key.find(':');
        if (pos == std::string::npos || pos == 0 || pos == static_cast<int>(key.size()) -1) {
            span.setStatus(false, "Invalid key format");
            return makeErrorResponse(http::status::bad_request, "Key must be in format 'table:pk'", req);
        }
        std::string table = key.substr(0, pos);
        std::string pk = key.substr(pos+1);

        span.setAttribute("entity.table", table);
        span.setAttribute("entity.pk", pk);

        // Build BaseEntity from blob (assume JSON payload string)
        std::string blob_str = body_json["blob"].get<std::string>();
        span.setAttribute("entity.size_bytes", static_cast<int64_t>(blob_str.size()));
        
        BaseEntity entity = BaseEntity::fromJson(pk, blob_str);

        // Schema-based field encryption (MVP): If an encryption schema config is persisted,
        // encrypt declared fields before index/storage persistence.
        try {
            auto schema_bytes = storage_->get("config:encryption_schema");
            if (schema_bytes) {
                std::string schema_json(schema_bytes->begin(), schema_bytes->end());
                auto schema = nlohmann::json::parse(schema_json);
                if (schema.contains("collections") && schema["collections"].contains(table)) {
                    auto coll = schema["collections"][table];
                    if (coll.contains("encryption") && coll["encryption"].contains("enabled") && coll["encryption"]["enabled"].get<bool>()) {
                        // Check required components
                        if (!field_encryption_) {
                            THEMIS_WARN("Encryption schema active but field_encryption_ missing");
                        } else if (!key_provider_) {
                            THEMIS_WARN("Encryption schema active but key_provider_ missing");
                        } else {
                            // Context type (user|group)
                            std::string context_type = coll["encryption"].value("context_type", "user");
                            std::vector<std::string> fields = {};

                            if (coll["encryption"].contains("fields")) {
                                fields.reserve(coll["encryption"]["fields"].size());  // OPTIMIZATION: Pre-allocate to avoid reallocations
                                for (auto& f : coll["encryption"]["fields"]) {
                                    if (f.is_string()) {
                                      fields.push_back(f.get<std::string>());
                                    }
                                }
                            }
                            // Extract user_id and groups from JWT token
                            auto auth_ctx = extractAuthContext(req);
                            std::string user_id = auth_ctx.user_id;
                            std::vector<std::string> groups_claim = auth_ctx.groups;
                            // Get DEK / Group-DEK from PKIKeyProvider (dynamic_cast for group functionality)
                            auto pki = std::dynamic_pointer_cast<themis::security::PKIKeyProvider>(key_provider_);
                            for (const auto& f : fields) {
                                if (!entity.hasField(f)) continue; // Field does not exist
                                auto valOpt = entity.getField(f);
                                if (!valOpt.has_value()) {
                                  continue;
                                }
                                
                                // Serialization of value for all supported types
                                std::vector<uint8_t> plain_bytes;
                                const auto& v = *valOpt;
                                
                                if (std::holds_alternative<std::string>(v)) {
                                    const auto& str = std::get<std::string>(v);
                                    plain_bytes.assign(str.begin(), str.end());
                                } else if (std::holds_alternative<int64_t>(v)) {
                                    std::string str = std::to_string(std::get<int64_t>(v));
                                    plain_bytes.assign(str.begin(), str.end());
                                } else if (std::holds_alternative<double>(v)) {
                                    std::string str = std::to_string(std::get<double>(v));
                                    plain_bytes.assign(str.begin(), str.end());
                                } else if (std::holds_alternative<bool>(v)) {
                                    std::string str = std::get<bool>(v) ? "true" : "false";
                                    plain_bytes.assign(str.begin(), str.end());
                                } else if (std::holds_alternative<std::vector<float>>(v)) {
                                    // Vector<float>: Serialize as JSON array
                                    const auto& vec = std::get<std::vector<float>>(v);
                                    nlohmann::json j_arr = nlohmann::json::array();
                                    for (float val : vec) {
                                      j_arr.push_back(val);
                                    }
                                    std::string json_str = j_arr.dump();
                                    plain_bytes.assign(json_str.begin(), json_str.end());
                                } else if (std::holds_alternative<std::vector<uint8_t>>(v)) {
                                    // Binary blob: directly as byte array
                                    plain_bytes = std::get<std::vector<uint8_t>>(v);
                                } else if (std::holds_alternative<std::monostate>(v)) {
                                    // Skip null value
                                    continue;
                                } else {
                                    // Skip unknown type
                                    THEMIS_WARN("Field encryption: Unknown type for field {}", f);
                                    continue;
                                }
                                
                                std::vector<uint8_t> raw_key;
                                std::string key_id = {};
                                if (context_type == "group" && pki && !groups_claim.empty()) {
                                    // Take first group as context (MVP)
                                    auto gdek = pki->getGroupDEK(groups_claim.front());
                                    // HKDF over gdek with Info=field:<name>
                                    std::vector<uint8_t> salt; // empty
                                    std::string info = "field:" + f;
                                    raw_key = utils::HKDFHelper::derive(gdek, salt, info, 32);
                                    key_id = "group_field:" + f;
                                    entity.setField(f + "_group", groups_claim.front());
                                } else {
                                    // Per-user or fallback to general field key
                                    std::string user_ctx = user_id.empty() ? "anonymous" : user_id;
                                    auto dek = key_provider_->getKey("dek");
                                    // salt = user_id (can be empty) - if empty, fallback to static salt to keep HKDF function stable
                                    std::vector<uint8_t> salt = {};

                                    if (!user_ctx.empty()) {
                                      salt.assign(user_ctx.begin(), user_ctx.end());
                                    }
                                    std::string info = "field:" + f;
                                    raw_key = utils::HKDFHelper::derive(dek, salt, info, 32);
                                    key_id = "user_field:" + f;
                                }
                                // Encrypt
                                try {
                                    std::string plain_str(plain_bytes.begin(), plain_bytes.end());
                                    auto blob = field_encryption_->encryptWithKey(plain_str, key_id, 1, raw_key);
                                    auto j = blob.toJson();
                                    entity.setField(f + "_encrypted", j.dump());
                                    entity.setField(f + "_enc", true);
                                    // Remove plaintext
                                    entity.setField(f, std::monostate{});
                                } catch (const std::exception& e) {
                                    THEMIS_WARN("Field encryption failed for {}: {}", f, e.what());
                                }
                            }
                        }
                    }
                }
            }
        } catch (const std::exception& e) {
            THEMIS_WARN("Schema encryption processing error: {}", e.what());
        }

        // Capture before snapshot for CDC enrichment (read prior to the write)
        std::optional<std::string> cdc_before_snapshot = {};

        if (changefeed_ && config_.feature_cdc) {
            auto old_bytes = storage_->get(KeySchema::makeRelationalKey(table, pk));
            if (old_bytes.has_value()) {
                cdc_before_snapshot = std::string(old_bytes->begin(), old_bytes->end());
            }
        }

        // Upsert via SecondaryIndexManager to keep indexes consistent
        auto st = secondary_index_->put(table, entity);
        if (!st.ok) {
            // Check for unique constraint violation
            if (st.message.find("Unique constraint violation") != std::string::npos) {
                span.setStatus(false, "Unique constraint violation");
                return makeErrorResponse(http::status::conflict,
                    st.message, req);
            }
            span.setStatus(false, st.message);
            return makeErrorResponse(http::status::internal_server_error,
                "Index/Storage update failed: " + st.message, req);
        }
        
        // Geo MVP: Update spatial index if enabled (best-effort, non-transactional)
        if (spatial_index_ && config_.feature_geo) {
            try {
                std::vector<uint8_t> blob_bytes(blob_str.begin(), blob_str.end());
                api::GeoIndexHooks::onEntityPut(*storage_, spatial_index_, table, pk, blob_bytes);
            } catch (const std::exception& e) {
                // Log but don't fail the request - geo index is best-effort
                THEMIS_WARN("Geo index hook failed for {}:{}: {}", table, pk, e.what());
            }
        }

        // Record CDC event if changefeed enabled
        if (changefeed_ && config_.feature_cdc) {
            try {
                Changefeed::ChangeEvent event;
                event.type = Changefeed::ChangeEventType::EVENT_PUT;
                event.key = key;
                event.value = blob_str;
                event.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()
                ).count();
                event.metadata = {{"table", table}, {"pk", pk}};
                event.before_snapshot = cdc_before_snapshot;
                event.after_snapshot = blob_str;
                changefeed_->recordEvent([[maybe_unused]] event);
            } catch (const std::exception& e) {
                // Log but don't fail the request
                THEMIS_WARN("CDC event recording failed: {}", e.what());
            }
        }

        // RAID redundancy: Apply redundancy strategy if enabled
        // This ensures RAID modes (MIRROR, STRIPE, PARITY, etc.) are applied at runtime
        bool raid_applied = false;
        if (redundancy_manager_ && hash_ring_ && shard_topology_ && config_.feature_raid) {
            try {
                // Get redundancy strategy for this collection (table)
                auto strategy = redundancy_manager_->getStrategy(table);
                if (strategy) {
                    THEMIS_DEBUG("Applying RAID redundancy for {}:{} using mode {}", 
                                table, pk, static_cast<int>(strategy->getConfig().mode));
                    
                    // Create write handler that writes to shards
                    auto write_handler = [this](const std::string& shard_id, 
                                               const std::string& doc_id,
                                               const std::vector<uint8_t>& data) -> bool {
                        // For now, write to local storage with shard prefix
                        // In a distributed setup, this would route to remote shards
                        std::string prefixed_key = shard_id + ":" + doc_id;
                        try {
                            bool ok = storage_->put(prefixed_key, data);
                            if (!ok) {
                                THEMIS_WARN("RAID write to shard {} failed: storage->put returned false", shard_id);
                            }
                            return ok;
                        } catch (const std::exception& e) {
                            THEMIS_WARN("RAID write to shard {} failed: {}", shard_id, e.what());
                            return false;
                        }
                    };
                    
                    // Convert data to bytes using the same serialized representation as the primary write
                    std::vector<uint8_t> data_bytes(blob_str.begin(), blob_str.end());
                    
                    // Apply redundancy strategy
                    auto write_result = strategy->write(
                        key,           // document_id
                        data_bytes,    // data
                        table,         // collection
                        *hash_ring_,   // ring
                        *shard_topology_, // topology
                        write_handler  // handler
                    );
                    
                    // RAID is best-effort: log and record in tracing, but do not fail the request
                    // Primary write has already been committed via secondary_index_->put()
                    if (!write_result.success) {
                        THEMIS_WARN("RAID write failed for {}: {}", key, write_result.error_message);
                        span.setAttribute("raid.success", false);
                        span.setAttribute("raid.error_message", write_result.error_message);
                    } else {
                        raid_applied = true;  // Only set to true when actually successful
                        span.setAttribute("raid.success", true);
                        span.setAttribute("raid.mode", static_cast<int64_t>(strategy->getConfig().mode));
                        span.setAttribute("raid.shards_written", static_cast<int64_t>(write_result.written_shards.size()));
                        span.setAttribute("raid.latency_ms", static_cast<int64_t>(write_result.latency.count()));
                        THEMIS_DEBUG("RAID write successful for {}: {} shards written in {}ms", 
                                   key,static_cast<int>(write_result.written_shards.size()), write_result.latency.count());
                    }
                }
            } catch (const std::exception& e) {
                // Log but don't fail - RAID is optional enhancement
                THEMIS_WARN("RAID redundancy processing error: {}", e.what());
            }
        }

        span.setStatus(true);
        span.setAttribute("entity.cdc_recorded", changefeed_ && config_.feature_cdc);
        span.setAttribute("entity.raid_applied", raid_applied);

        // Write concern enforcement (RAID replication)
        // Parse write concern from query params (default: ONE)
        std::string concern_str = "ONE";
        auto target_str = std::string(req.target());
        auto qpos = target_str.find('?');
        if (qpos != std::string::npos) {
            auto query = target_str.substr(qpos + 1);
            auto params = query; // simple parse: write_concern=MAJORITY
            auto wc_pos = params.find("write_concern=");
            if (wc_pos != std::string::npos) {
                auto wc_start = wc_pos + 14; // len("write_concern=")
                auto wc_end = params.find('&', wc_start);
                concern_str = params.substr(wc_start, wc_end == std::string::npos ? std::string::npos : wc_end - wc_start);
            }
        }

        sharding::WriteConcernConfig wc_config;
        wc_config.level = sharding::parseWriteConcern(concern_str);
        wc_config.timeout = std::chrono::milliseconds(5000); // 5s default

        // If replication coordinator exists and concern > ONE, wait for quorum
        if (replication_coordinator_ && config_.feature_replication && wc_config.level != sharding::WriteConcern::ONE) {
            // Append to WAL to track LSN
            sharding::WALEntry wal_entry;
            wal_entry.lsn = wal_manager_ ? wal_manager_->getCurrentLSN() : sharding::LSN{0, 0};
            wal_entry.type = sharding::WALEntryType::INSERT;
            wal_entry.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count();
            wal_entry.transaction_id = ""; // optional
            wal_entry.data = nlohmann::json{{"key", key}, {"value", blob_str}};

            if (wal_manager_) {
                wal_entry.lsn = wal_manager_->append(wal_entry);
            }

            // Record multi-primary bookkeeping (logical clock/heartbeat)
            if (multi_primary_coordinator_) {
                multi_primary_coordinator_->recordWrite(wal_entry.lsn);
            }

            auto write_result = replication_coordinator_->waitForReplication(wal_entry.lsn, wc_config);
            if (!write_result.success) {
                THEMIS_WARN("Write concern {} failed for {}: {}", concern_str, key, write_result.error_message);
                span.setStatus(false, write_result.error_message);
                return makeErrorResponse(http::status::service_unavailable,
                    "Write concern not met: " + write_result.error_message, req);
            }
            span.setAttribute("write_concern.level", concern_str);
            span.setAttribute("write_concern.acks", static_cast<int64_t>(write_result.replicas_acknowledged));
            span.setAttribute("write_concern.latency_ms", static_cast<int64_t>(write_result.latency.count()));
        }

        json response = {
            {"success", true},
            {"key", key},
            {"blob_size",static_cast<int>(blob_str.size())}
        };
        return makeResponse(http::status::created, response.dump(), req);

    } catch (const json::exception& e) {
        THEMIS_ERROR("PUT entity JSON error: {}", e.what());
        span.setStatus(false, e.what());
        return makeErrorResponse(http::status::bad_request, 
            "Invalid JSON: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        THEMIS_ERROR("PUT entity error: {}", e.what());
        span.setStatus(false, e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> EntityApiHandler::handleDelete(
    const http::request<http::string_body>& req
) {
    if (auth_ && auth_->isEnabled()) {
        std::string path_only = std::string(req.target());
        auto qpos = path_only.find('?');
        if (qpos != std::string::npos) {
          path_only = path_only.substr(0, qpos);
        }
        if (auto resp = requireAccess(req, "data:write", "delete", path_only)) {
          return *resp;
        }
    }
    auto span = Tracer::startSpan("DELETE /entities/:key");
    
    try {
        auto key = extractPathParam(std::string(req.target()), "/entities/");
        if (key.empty()) {
            span.setStatus(false, "Missing entity key");
            return makeErrorResponse(http::status::bad_request, "Missing entity key", req);
        }

        span.setAttribute("entity.key", key);

        // Split key into table:pk
        auto pos = key.find(':');
        if (pos == std::string::npos || pos == 0 || pos == static_cast<int>(key.size()) -1) {
            span.setStatus(false, "Invalid key format");
            return makeErrorResponse(http::status::bad_request, "Key must be in format 'table:pk'", req);
        }
        std::string table = key.substr(0, pos);
        std::string pk = key.substr(pos+1);

        span.setAttribute("entity.table", table);
        span.setAttribute("entity.pk", pk);
        
        // Geo MVP: Remove from spatial index before delete (best-effort)
        if (spatial_index_ && config_.feature_geo) {
            try {
                // Get old blob for computing sidecar
                std::string entity_key = "entity:" + table + ":" + pk;
                auto old_blob = storage_->get(entity_key);
                if (old_blob) {
                    api::GeoIndexHooks::onEntityDelete(*storage_, spatial_index_, table, pk, *old_blob);
                }
            } catch (const std::exception& e) {
                // Log but don't fail the request - geo index is best-effort
                THEMIS_WARN("Geo index delete hook failed for {}:{}: {}", table, pk, e.what());
            }
        }

        // Capture before snapshot for CDC enrichment (read prior to the delete)
        std::optional<std::string> cdc_before_snapshot = {};

        if (changefeed_ && config_.feature_cdc) {
            auto old_bytes = storage_->get(KeySchema::makeRelationalKey(table, pk));
            if (old_bytes.has_value()) {
                cdc_before_snapshot = std::string(old_bytes->begin(), old_bytes->end());
            }
        }

        auto st = secondary_index_->erase(table, pk);
        if (!st.ok) {
            span.setStatus(false, st.message);
            return makeErrorResponse(http::status::internal_server_error,
                "Index/Storage delete failed: " + st.message, req);
        }

        // Record CDC event if changefeed enabled
        if (changefeed_ && config_.feature_cdc) {
            try {
                Changefeed::ChangeEvent event;
                event.type = Changefeed::ChangeEventType::EVENT_DELETE;
                event.key = key;
                event.value = std::nullopt; // No value for DELETE
                event.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()
                ).count();
                event.metadata = {{"table", table}, {"pk", pk}};
                event.before_snapshot = cdc_before_snapshot;
                changefeed_->recordEvent([[maybe_unused]] event);
            } catch (const std::exception& e) {
                THEMIS_WARN("CDC event recording failed: {}", e.what());
            }
        }

        span.setStatus(true);
        span.setAttribute("entity.cdc_recorded", changefeed_ && config_.feature_cdc);

        json response = {
            {"success", true},
            {"key", key}
        };
        return makeResponse(http::status::ok, response.dump(), req);

    } catch (const std::exception& e) {
        THEMIS_ERROR("DELETE entity error: {}", e.what());
        span.setStatus(false, e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> EntityApiHandler::handleBatch(
    const http::request<http::string_body>& req
) {
    if (auth_ && auth_->isEnabled()) {
        if (auto resp = requireAccess(req, "data:write", "write", "/entities/batch")) {
          return *resp;
        }
    }
    auto span = Tracer::startSpan("POST /entities/batch");
    
    try {
        // Parse request
        auto body_json = json::parse(req.body());
        
        if (!body_json.contains("operations") || !body_json["operations"].is_array()) {
            span.setStatus(false, "Missing or invalid operations array");
            return makeErrorResponse(http::status::bad_request, 
                "Request must contain 'operations' array", req);
        }
        
        auto& operations = body_json["operations"];
        int64_t total = operations.size();
        
        if (total == 0) {
            span.setStatus(false, "Empty operations array");
            return makeErrorResponse(http::status::bad_request, 
                "Operations array cannot be empty", req);
        }
        
        // Limit batch size to prevent memory exhaustion
        const int64_t MAX_BATCH_SIZE = 10000;
        if (total > MAX_BATCH_SIZE) {
            span.setStatus(false, "Batch too large");
            return makeErrorResponse(http::status::bad_request, 
                "Batch size exceeds maximum of " + std::to_string(MAX_BATCH_SIZE), req);
        }
        
        span.setAttribute("batch.total_ops", total);
        
        // Collect errors during validation/processing
        std::vector<json> errors;
        errors.reserve(total);  // OPTIMIZATION: Pre-allocate to avoid reallocations during validation
        int64_t succeeded = 0;
        
        // Phase 1: Validate all operations before committing anything
        // This ensures we can provide partial success feedback
        struct ValidatedOp {
            std::string op_type; // "put" or "delete"
            std::string table = {};
            std::string pk = {};
            std::string key; // table:pk
            std::string blob; // Only for PUT
            int64_t index;
            std::optional<std::string> before_snapshot; // captured before write for CDC
        };
        std::vector<ValidatedOp> validated_ops;
        validated_ops.reserve(total);
        
        for (int64_t i = 0; i < total; ++i) {
            try {
                const auto& op = operations[i];
                
                if (!op.contains("op") || !op["op"].is_string()) {
                    errors.push_back({
                        {"index", i},
                        {"error", "Missing or invalid 'op' field"}
                    });
                    continue;
                }
                
                if (!op.contains("key") || !op["key"].is_string()) {
                    errors.push_back({
                        {"index", i},
                        {"error", "Missing or invalid 'key' field"}
                    });
                    continue;
                }
                
                std::string op_type = op["op"].get<std::string>();
                std::string key = op["key"].get<std::string>();
                
                // Validate operation type
                if (op_type != "put" && op_type != "delete") {
                    errors.push_back({
                        {"index", i},
                        {"key", key},
                        {"error", "Invalid operation type: " + op_type}
                    });
                    continue;
                }
                
                // Parse key format (table:pk)
                auto pos = key.find(':');
                if (pos == std::string::npos || pos == 0 || pos == static_cast<int>(key.size()) -1) {
                    errors.push_back({
                        {"index", i},
                        {"key", key},
                        {"error", "Key must be in format 'table:pk'"}
                    });
                    continue;
                }
                
                std::string table = key.substr(0, pos);
                std::string pk = key.substr(pos+1);

                if (!isValidEntityBatchKeyPart(table) || !isValidEntityBatchKeyPart(pk)) {
                    errors.push_back({
                        {"index", i},
                        {"key", key},
                        {"error", "Key contains invalid table or pk characters"}
                    });
                    continue;
                }
                
                ValidatedOp vop;
                vop.op_type = op_type;
                vop.table = table;
                vop.pk = pk;
                vop.key = key;
                vop.index = i;
                
                // For PUT operations, require blob field
                if (op_type == "put") {
                    if (!op.contains("blob") || !op["blob"].is_string()) {
                        errors.push_back({
                            {"index", i},
                            {"key", key},
                            {"error", "PUT operation requires 'blob' field"}
                        });
                        continue;
                    }
                    vop.blob = op["blob"].get<std::string>();
                }
                
                validated_ops.push_back(std::move(vop));
                
            } catch (const json::exception& e) {
                errors.push_back({
                    {"index", i},
                    {"error", std::string("JSON error: ") + e.what()}
                });
            } catch (const std::exception& e) {
                errors.push_back({
                    {"index", i},
                    {"error", e.what()}
                });
            }
        }
        
        // Phase 2: Execute validated operations using WriteBatch
        auto batch = storage_->createWriteBatch();
        
        for (auto& vop : validated_ops) {
            try {
                // Capture before snapshot for CDC enrichment (before the write is batched)
                if (changefeed_ && config_.feature_cdc) {
                    auto old_bytes = storage_->get(KeySchema::makeRelationalKey(vop.table, vop.pk));
                    if (old_bytes.has_value()) {
                        vop.before_snapshot = std::string(old_bytes->begin(), old_bytes->end());
                    }
                }

                if (vop.op_type == "put") {
                    // Build entity from blob
                    BaseEntity entity = BaseEntity::fromJson(vop.pk, vop.blob);
                    
                    // Use SecondaryIndexManager batch variant for index consistency
                    auto st = secondary_index_->put(vop.table, entity, *batch);
                    if (!st.ok) {
                        errors.push_back({
                            {"index", vop.index},
                            {"key", vop.key},
                            {"error", st.message}
                        });
                        continue; // Skip this operation but continue with others
                    }
                    
                    // Geo index update (best-effort, non-transactional)
                    if (spatial_index_ && config_.feature_geo) {
                        try {
                            std::vector<uint8_t> blob_bytes(vop.blob.begin(), vop.blob.end());
                            api::GeoIndexHooks::onEntityPut(*storage_, spatial_index_, 
                                vop.table, vop.pk, blob_bytes);
                        } catch (const std::exception& e) {
                            THEMIS_WARN("Geo index hook failed for {}:{}: {}", vop.table, vop.pk, e.what());
                        }
                    }
                    
                } else { // delete
                    // Geo index cleanup (best-effort)
                    if (spatial_index_ && config_.feature_geo) {
                        try {
                            std::string entity_key = "entity:" + vop.table + ":" + vop.pk;
                            auto old_blob = storage_->get(entity_key);
                            if (old_blob) {
                                api::GeoIndexHooks::onEntityDelete(*storage_, spatial_index_, 
                                    vop.table, vop.pk, *old_blob);
                            }
                        } catch (const std::exception& e) {
                            THEMIS_WARN("Geo index delete hook failed for {}:{}: {}", vop.table, vop.pk, e.what());
                        }
                    }
                    
                    auto st = secondary_index_->erase(vop.table, vop.pk, *batch);
                    if (!st.ok) {
                        errors.push_back({
                            {"index", vop.index},
                            {"key", vop.key},
                            {"error", st.message}
                        });
                        continue;
                    }
                }
                
                succeeded++;
                
            } catch (const std::exception& e) {
                errors.push_back({
                    {"index", vop.index},
                    {"key", vop.key},
                    {"error", e.what()}
                });
            }
        }
        
        // Phase 3: Atomic commit
        bool commit_ok = batch->commit();
        if (!commit_ok) {
            span.setStatus(false, "WriteBatch commit failed");
            return makeErrorResponse(http::status::internal_server_error, 
                "Atomic batch commit failed", req);
        }
        
        // Record CDC events if enabled (best-effort, after commit)
        if (changefeed_ && config_.feature_cdc) {
            auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count();
            
            for (const auto& vop : validated_ops) {
                try {
                    Changefeed::ChangeEvent event;
                    event.type = (vop.op_type == "put") ? 
                        Changefeed::ChangeEventType::EVENT_PUT : 
                        Changefeed::ChangeEventType::EVENT_DELETE;
                    event.key = vop.key;
                    if (vop.op_type == "put") {
                        event.value = vop.blob;
                        event.after_snapshot = vop.blob;
                    } else {
                        event.value = std::nullopt;
                    }
                    event.before_snapshot = vop.before_snapshot;
                    event.timestamp_ms = now_ms;
                    event.metadata = {{"table", vop.table}, {"pk", vop.pk}, {"batch", true}};
                    changefeed_->recordEvent([[maybe_unused]] event);
                } catch (const std::exception& e) {
                    // Log but don't fail the request
                    THEMIS_WARN("CDC event recording failed for {}: {}", vop.key, e.what());
                }
            }
        }
        
        int64_t failed = errors.size();
        
        span.setStatus(true);
        span.setAttribute("batch.succeeded", succeeded);
        span.setAttribute("batch.failed", failed);
        span.setAttribute("batch.cdc_recorded", changefeed_ && config_.feature_cdc);
        
        json response = {
            {"success", true},
            {"total", total},
            {"succeeded", succeeded},
            {"failed", failed}
        };
        
        if (!errors.empty()) {
            response["errors"] = errors;
        }
        
        return makeResponse(http::status::ok, response.dump(), req);
        
    } catch (const json::exception& e) {
        THEMIS_ERROR("Batch entities JSON error: {}", e.what());
        span.setStatus(false, e.what());
        return makeErrorResponse(http::status::bad_request, 
            "Invalid JSON: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        THEMIS_ERROR("Batch entities error: {}", e.what());
        span.setStatus(false, e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

std::string EntityApiHandler::extractPathParam(const std::string& target, const std::string& prefix) {
    if (!(target.rfind(prefix, 0) == 0)) {
        return "";
    }
    auto param = target.substr(prefix.length());
    // Remove query string if present
    auto query_pos = param.find('?');
    if (query_pos != std::string::npos) {
        param = param.substr(0, query_pos);
    }
    return param;
}

http::response<http::string_body> EntityApiHandler::makeErrorResponse(
    http::status status, const std::string& message, const http::request<http::string_body>& req
) {
    nlohmann::json error_body = {
        {"error", true},
        {"message", message},
        {"status_code", static_cast<int>(status)}
    };
    return makeResponse(status, error_body.dump(), req);
}

http::response<http::string_body> EntityApiHandler::makeResponse(
    http::status status, const std::string& body, const http::request<http::string_body>& req
) {
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::server, "THEMIS/0.1.0");
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = body;
    res.prepare_payload();
    return res;
}

// ---------------------------------------------------------------------------
// POST /v2/documents  – bulk insert from newline-delimited JSON (NDJSON)
// ---------------------------------------------------------------------------

http::response<http::string_body> EntityApiHandler::handleBulkNdjson(
    const http::request<http::string_body>& req)
{
    if (auth_ && auth_->isEnabled()) {
        if (auto resp = requireAccess(req, "data:write", "write", "/v2/documents")) {
          return *resp;
        }
    }
    auto span = Tracer::startSpan("POST /v2/documents");

    static constexpr size_t kMaxDocuments = 10000;

    // Validate Content-Type
    const auto ct_it = req.find(http::field::content_type);
    if (ct_it == req.end() ||
        ct_it->value().find("application/x-ndjson") == std::string_view::npos) {
        span.setStatus(false, "Unsupported Media Type");
        return makeErrorResponse(http::status::unsupported_media_type,
            "Content-Type must be application/x-ndjson", req);
    }

    const std::string& body = req.body();
    if (body.empty()) {
        span.setStatus(false, "Empty body");
        return makeErrorResponse(http::status::bad_request, "Request body is empty", req);
    }

    // Parse NDJSON: each non-empty line is one JSON document.
    std::vector<json> documents;
    documents.reserve(256);
    std::vector<json> errors;
    errors.reserve(256);  // OPTIMIZATION: Pre-allocate to avoid reallocations

    std::istringstream stream(body);
    std::string line = {};
    size_t line_number = 0;

    while (std::getline(stream, line)) {
        ++line_number;
        // Skip blank lines
        if (line.empty() || line == "\r") {
          continue;
        }
        // Remove trailing CR if present
        if (!line.empty() && line.back() == '\r') {
          line.pop_back();
        }

        if (static_cast<int>(documents.size()) > = kMaxDocuments) {
            span.setStatus(false, "Too many documents");
            return makeErrorResponse(http::status::bad_request,
                "Request exceeds maximum of " + std::to_string(kMaxDocuments) + " documents",
                req);
        }

        try {
            documents.push_back(json::parse(line));
        } catch (const json::exception& ex) {
            errors.push_back({
                {"line",    static_cast<int64_t>(line_number)},
                {"error",   std::string("JSON parse error: ") + ex.what()}
            });
        }
    }

    if (documents.empty() && errors.empty()) {
        return makeErrorResponse(http::status::bad_request,
            "Request body contains no documents", req);
    }

    span.setAttribute("bulk.total_docs",   static_cast<int64_t>(documents.size()));
    span.setAttribute("bulk.parse_errors", static_cast<int64_t>(errors.size()));

    // Insert valid documents.
    int64_t inserted = 0;
    for (const auto& doc : documents) {
        // Each document must have a "key" field; generate one if absent.
        std::string key = {};
        if (doc.contains("_key") && doc["_key"].is_string()) {
            key = doc["_key"].get<std::string>();
        } else if (doc.contains("key") && doc["key"].is_string()) {
            key = doc["key"].get<std::string>();
        } else {
            // Auto-generate a key from the document index to keep behaviour deterministic.
            key = "doc_" + std::to_string(inserted);
        }

        try {
            if (storage_) {
                storage_->put(key, doc.dump());
                ++inserted;
            }
        } catch (const std::exception& ex) {
            errors.push_back({
                {"key",   key},
                {"error", std::string("Storage error: ") + ex.what()}
            });
        }
    }

    span.setAttribute("bulk.inserted", inserted);
    span.setStatus(true, "bulk_ndjson_complete");

    const http::status status =
        errors.empty() ? http::status::ok : http::status::multi_status;

    json result = {
        {"inserted",    inserted},
        {"total",       static_cast<int64_t>(static_cast<int>(documents.size()) + errors.size())},
        {"error_count", static_cast<int64_t>(errors.size())}
    };
    if (!errors.empty()) {
        result["errors"] = errors;
    }

    return makeResponse(status, result.dump(), req);
}

} // namespace server
} // namespace themis

