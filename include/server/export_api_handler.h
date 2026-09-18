/**
 * @file export_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <map>
#include <atomic>
#include <mutex>

#include "plugins/plugin_manager.h"
#include "exporters/exporter_interface.h"

// Forward-declare governance types to keep this header lean.
namespace themis::governance { class PolicyEngine; }
namespace themis::utils      { class AuditLogger; }

namespace themis {

// Forward declarations
class RocksDBWrapper;
class SecondaryIndexManager;

namespace server {

namespace beast = boost::beast;
namespace http = beast::http;

class ExportApiHandler {
public:
    ExportApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<SecondaryIndexManager> secondary_index
    );

    ~ExportApiHandler();

    void setPolicyEngine(themis::governance::PolicyEngine* engine) noexcept {
        policy_engine_ = engine;
    }

    void setAuditLogger(themis::utils::AuditLogger* logger) noexcept {
        audit_logger_ = logger;
    }

    /**
     * @brief Handle Export Jsonl Llm.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleExportJsonlLlm(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Export Status.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleExportStatus(
        const http::request<http::string_body>& req);

private:
    // Export job tracking
    struct ExportJob {
        std::string export_id;
        std::string status;  // "in_progress", "completed", "failed"
        double progress;
        size_t records_processed;
        size_t records_total;
        std::string output_path;
        std::chrono::system_clock::time_point started_at;
        std::chrono::system_clock::time_point completed_at;
        std::string error_message;
    };

    /**
     * @brief Generate Export Id.
     * @return Return value.
     */
    std::string generateExportId();

    /**
     * @brief Validate Admin Token.
     * @param[in] req Input parameter.
     * @return True when the operation succeeds.
     */
    bool validateAdminToken(const http::request<http::string_body>& req);

    /**
     * @brief Build Aql Query.
     * @param[in] request_json Input parameter.
     * @return Return value.
     */
    std::string buildAqlQuery(const nlohmann::json& request_json);

    /**
     * @brief Json Response.
     * @param[in] status Input parameter.
     * @param[in] json_body Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> jsonResponse(
        http::status status,
        const std::string& json_body);

    /**
     * @brief Error Response.
     * @param[in] status Input parameter.
     * @param[in] error_message Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> errorResponse(
        http::status status,
        const std::string& error_message);

    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<SecondaryIndexManager> secondary_index_;

    // Optional policy enforcement — set via setPolicyEngine() / setAuditLogger().
    themis::governance::PolicyEngine* policy_engine_ = nullptr;
    themis::utils::AuditLogger*       audit_logger_  = nullptr;
    
    // Export job tracking
    std::map<std::string, ExportJob> export_jobs_;
    std::mutex export_jobs_mutex_;
    std::atomic<size_t> export_counter_{0};
};

} // namespace server
} // namespace themis
