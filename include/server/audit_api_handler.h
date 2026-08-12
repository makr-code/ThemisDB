/**
 * @file audit_api_handler.h
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
#include <memory>
#include <vector>
#include <climits>
#include <nlohmann/json.hpp>
#include "utils/audit_logger.h"
#include "security/encryption.h"
#include "utils/pki_client.h"

namespace themis {
namespace server {

struct AuditLogEntry {
    int64_t id;
    int64_t timestamp_ms;
    std::string user;
    std::string action;
    std::string entity_type;
    std::string entity_id;
    std::string old_value;
    std::string new_value;
    bool success;
    std::string ip_address;
    std::string session_id;
    std::string error_message;
    
    nlohmann::json toJson() const;
};

struct AuditQueryFilter {
    int64_t start_ts_ms = 0;
    int64_t end_ts_ms = LLONG_MAX;
    std::string user;
    std::string action;
    std::string entity_type;
    std::string entity_id;
    bool success_only = false;
    int page = 1;
    int page_size = 100;
};

/**
 * @brief AuditApiHandler - Audit log API operations.
 * 
 * HTTP API handler for audit log API operations.
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

class AuditApiHandler {
public:
    AuditApiHandler(std::shared_ptr<themis::FieldEncryption> enc,
                    std::shared_ptr<themis::utils::VCCPKIClient> pki,
                    const std::string& log_path);

    // Query audit logs with filtering and pagination
    nlohmann::json queryAuditLogs(const AuditQueryFilter& filter);
    
    // Export audit logs as CSV
    std::string exportAuditLogsCsv(const AuditQueryFilter& filter);

private:
    std::shared_ptr<themis::FieldEncryption> enc_;
    std::shared_ptr<themis::utils::VCCPKIClient> pki_;
    std::string log_path_;

    // Read and decrypt audit log entries from JSONL file
    std::vector<AuditLogEntry> readAuditLogs(const AuditQueryFilter& filter);
    
    // Parse single JSON line to AuditLogEntry
    AuditLogEntry parseLogLine(const nlohmann::json& j, int64_t line_id);
    
    // Decrypt encrypted audit payload
    std::string decryptPayload(const nlohmann::json& payload);
};

} // namespace server
} // namespace themis
