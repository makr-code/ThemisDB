/**
 * @file pii_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

// Forward declarations to avoid heavy includes in header
namespace rocksdb { class TransactionDB; class ColumnFamilyHandle; }

namespace themis { namespace server {

struct PiiQueryFilter {
    std::string original_uuid;
    std::string pseudonym;
    bool active_only{false};
    int page{1};
    int page_size{100};
};

struct PiiMapping {
    std::string original_uuid;
    std::string pseudonym;
    bool active{true};
    std::string created_at; // ISO8601
    std::string updated_at; // ISO8601

    nlohmann::json toJson() const;
    static PiiMapping fromJson(const nlohmann::json& j);
};

/**
 * @brief PIIApiHandler - Personally Identifiable Information (PII) data handling and masking.
 * 
 * HTTP API handler for personally identifiable information (PII) data handling and masking.
 * Implements endpoint-specific routing, request validation, business logic,
 * and response formatting.
 * 
 * ### HTTP Endpoints
 * Supported operations depend on the specific handler implementation.
 * See handler methods for endpoint mappings and request/response schemas.
 * 
 * ### Thread Safety
 * Handler instance and all methods are thread-safe for concurrent requests.
 * Internal state modifications use appropriate synchronization primitives.
 * 
 * ### Error Handling
 * All endpoints follow consistent error response formatting:
 * - 400: Bad Request (invalid input)
 * - 401: Unauthorized (missing/invalid authentication)
 * - 403: Forbidden (insufficient permissions)
 * - 404: Not Found (resource doesn't exist)
 * - 500: Internal Server Error (unexpected failure)
 * 
 * @note Integrates with rate limiting, auth middleware, and validation pipeline
 * @note Request bodies are validated against JSON schemas before processing
 * @note All operations are auditable and logged
 */

class PIIApiHandler {
public:
    PIIApiHandler() = default;
    PIIApiHandler(rocksdb::TransactionDB* db, rocksdb::ColumnFamilyHandle* cf);

    // CRUD
    bool addMapping(const PiiMapping& mapping); // false if duplicate
    std::optional<PiiMapping> getMapping(const std::string& original_uuid) const;
    bool deleteMapping(const std::string& original_uuid); // hard delete

    // Listing helpers
    // Returns a JSON object: { "items": [ ... ], "total": N, "page": p, "page_size": s }
    nlohmann::json listMappings(const PiiQueryFilter& filter);

    // Returns CSV string with header
    std::string exportCsv(const PiiQueryFilter& filter);

    // Backward-compatible demo method retained (delegates to deleteMapping)
    nlohmann::json deleteByUuid(const std::string& uuid);

private:
    rocksdb::TransactionDB* db_{nullptr};
    rocksdb::ColumnFamilyHandle* cf_{nullptr};

    static constexpr const char* KEY_PREFIX = "pii:";
    static std::string makeKey(const std::string& uuid) { return std::string(KEY_PREFIX) + uuid; }
    static std::string nowIso8601();
};

}} // namespace themis::server
