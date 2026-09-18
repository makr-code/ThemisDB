/**
 * @file audit_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
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
    
    /**
     * @brief To Json.
     * @return Return value.
     */
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


class AuditApiHandler {
public:
    AuditApiHandler(std::shared_ptr<themis::FieldEncryption> enc,
                    std::shared_ptr<themis::utils::VCCPKIClient> pki,
                    const std::string& log_path);

    /**
     * @brief Query Audit Logs.
     * @param[in] filter Input parameter.
     * @return Return value.
     */
    nlohmann::json queryAuditLogs(const AuditQueryFilter& filter);
    
    /**
     * @brief Export Audit Logs Csv.
     * @param[in] filter Input parameter.
     * @return Return value.
     */
    std::string exportAuditLogsCsv(const AuditQueryFilter& filter);

private:
    std::shared_ptr<themis::FieldEncryption> enc_;
    std::shared_ptr<themis::utils::VCCPKIClient> pki_;
    std::string log_path_;

    /**
     * @brief Read Audit Logs.
     * @param[in] filter Input parameter.
     * @return Return value.
     */
    std::vector<AuditLogEntry> readAuditLogs(const AuditQueryFilter& filter);
    
    /**
     * @brief Parse Log Line.
     * @param[in] j Input parameter.
     * @param[in] line_id Identifier of the line.
     * @return Return value.
     */
    AuditLogEntry parseLogLine(const nlohmann::json& j, int64_t line_id);
    
    /**
     * @brief Decrypt Payload.
     * @param[in] payload Input parameter.
     * @return Return value.
     */
    std::string decryptPayload(const nlohmann::json& payload);
};

} // namespace server
} // namespace themis
