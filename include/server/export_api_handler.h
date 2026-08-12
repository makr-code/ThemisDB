/**
 * @file export_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Export API Handler for HTTP Server
 * 
 * Provides REST endpoints for data export, specifically designed for
 * VCC-Clara integration with thematic and temporal filtering.
 * 
 * Supports:
 * - JSONL LLM export for AI training (LoRA/QLoRA)
 * - Thematic filtering (e.g., "Rechtssprechung", "Immissionsschutz")
 * - Temporal boundaries (date ranges)
 * - Quality filtering and weighted sampling
 */
class ExportApiHandler {
public:
    /**
     * @brief Construct handler with database access
     */
    ExportApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<SecondaryIndexManager> secondary_index
    );

    ~ExportApiHandler();

    /// Attach a PolicyEngine so that handleExportJsonlLlm() enforces
    /// per-collection authorization before starting any export.
    /// Non-owning raw pointer — caller must ensure it outlives this handler.
    /// Null (default) disables the authorization check (backward compatible).
    void setPolicyEngine(themis::governance::PolicyEngine* engine) noexcept {
        policy_engine_ = engine;
    }

    /// Attach an AuditLogger for recording export authorization decisions.
    /// Non-owning raw pointer — caller must ensure it outlives this handler.
    /// Null (default) disables audit logging (backward compatible).
    void setAuditLogger(themis::utils::AuditLogger* logger) noexcept {
        audit_logger_ = logger;
    }

    /**
     * @brief Handle JSONL LLM export request
     * POST /api/export/jsonl_llm
     * 
     * Designed for VCC-Clara integration:
     * - Thematic filtering: category, domain, subject
     * - Temporal boundaries: from_date, to_date
     * - Weighted sampling for training data quality
     * 
     * Request: JSON with query and export config
     * Response: Streaming JSONL (application/x-ndjson)
     */
    http::response<http::string_body> handleExportJsonlLlm(
        const http::request<http::string_body>& req);

    /**
     * @brief Get export status
     * GET /api/export/status/{export_id}
     * 
     * Response: JSON with export progress
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

    // Helper: Generate export ID
    std::string generateExportId();

    // Helper: Validate authentication
    bool validateAdminToken(const http::request<http::string_body>& req);

    // Helper: Build AQL query from request parameters
    std::string buildAqlQuery(const nlohmann::json& request_json);

    // Helper: Create JSON response
    http::response<http::string_body> jsonResponse(
        http::status status,
        const std::string& json_body);

    // Helper: Create error response
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
