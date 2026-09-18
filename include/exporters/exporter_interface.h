/**
 * @file exporter_interface.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <functional>
#include <chrono>
#include <vector>
#include <map>
#include <memory>
#include <optional>
#include "exporters/export_encryption.h"
#include "storage/base_entity.h"

// Forward-declare PolicyEngine so exporter_interface.h stays lean.
// The full type is only included in export_policy_guard.h / exporters that
// actually call enforceExportPolicy().
namespace themis::governance {
class PolicyEngine;
} // namespace themis::governance

namespace themis::utils {
class AuditLogger;
} // namespace themis::utils

namespace themis::exporters {

// Forward declaration
class ExporterMetrics;

struct ExportStats {
    size_t total_entities = 0;
    size_t exported_entities = 0;
    size_t failed_entities = 0;
    size_t skipped_entities = 0;  // Entities skipped by incremental/delta filter
    size_t bytes_written = 0;
    std::chrono::milliseconds duration{0};
    std::vector<std::string> errors;

    double estimated_eta_seconds = 0.0;
    
    // Optional: Detailed metrics
    std::shared_ptr<ExporterMetrics> metrics;
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    std::string toJson() const;
};

struct ExportTenantContext {
    std::string tenant_id;                  // Tenant identifier
    std::string user_id;                    // User performing export
    std::vector<std::string> scopes;        // Authorization scopes (export:read, export:write, etc.)
    bool enforce_isolation = true;          // Enforce tenant data isolation
    
    bool hasScope(const std::string& scope) const {
        return std::find(scopes.begin(), scopes.end(), scope) != scopes.end();
    }
};

struct ExportOptions {
    // Output file path
    std::string output_path;
    
    // P1: Tenant context (optional for backward compatibility)
    std::optional<ExportTenantContext> tenant_context;
    
    // Filtering
    std::vector<std::string> include_fields;  // If empty, export all fields
    std::vector<std::string> exclude_fields;
    std::string filter_expression;            // Optional AQL FILTER predicate using 'doc' variable
                                              // — provide only the predicate, not the FILTER keyword
                                              // (e.g., "doc.category == \"active\"" or
                                              //  "doc.age > 18 AND doc.score >= 0.5")
    
    // Format options
    bool pretty_print = false;
    bool compress = false;                     // Enable compression
    std::string compression_type = "gzip";     // gzip, zstd, none
    int compression_level = 6;                 // 1-9 for gzip, 1-22 for zstd
    
    // Resource limits
    size_t max_file_size_bytes = 0;           // 0 = unlimited
    size_t max_throughput_bps = 0;            // 0 = unlimited (bytes per second)
    size_t buffer_size_bytes = 8192;          // Buffer size for streaming
    
    // Progress reporting
    std::function<void(const ExportStats&)> progress_callback;
    size_t progress_interval = 1000;  // Report every N entities
    
    // Error handling
    bool continue_on_error = true;
    size_t max_errors = 100;

    // Encryption (optional, disabled by default)
    ExportEncryptionConfig encryption;
    // Export encryption: when set and non-empty, the output file is encrypted
    // with AES-256-GCM using a per-job DEK derived via HKDF-SHA256 from the
    // KEK referenced by ExportEncryptionConfig::kek_id.
    // The raw key material is never written to disk or emitted in any log.
    std::optional<ExportEncryptionConfig> encryption_config;

    // ── Authorization (EXP-001) ──────────────────────────────────────────────
    // When policy_engine is non-null, every exporter MUST call
    // enforceExportPolicy(*this) before opening any cursor or output file.
    // A PolicyEngine::checkExportPermission() denial throws ExporterException
    // with code ERR_EXPORT_POLICY_DENIED.

    std::string collection_name;

    std::string requesting_user;

    themis::governance::PolicyEngine* policy_engine = nullptr;

    themis::utils::AuditLogger* audit_logger = nullptr;
};

/**
 * @brief Enforce Export Policy.
 * @param[in] options Input parameter.
 */
void enforceExportPolicy(const ExportOptions& options);

class IExporter {
public:
    /**
     * @brief IExporter.
     * @return Return value.
     */
    virtual ~IExporter() = default;
    
    [[nodiscard]] virtual ExportStats exportEntities(
        const std::vector<BaseEntity>& entities,
        const ExportOptions& options
    ) = 0;
    
    [[nodiscard]] virtual std::vector<std::string> getSupportedFormats() const = 0;
    
    [[nodiscard]] virtual std::string getName() const = 0;
    
    [[nodiscard]] virtual std::string getVersion() const = 0;
};

} // namespace themis::exporters
