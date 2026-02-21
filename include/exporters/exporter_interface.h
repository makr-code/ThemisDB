/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            exporter_interface.h                               ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-02-21 13:48:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     128                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 397f3a597  2026-02-21  Refactor header includes and documentation updates across... ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <functional>
#include <chrono>
#include <vector>
#include <map>
#include <memory>
#include <optional>
#include "storage/base_entity.h"

namespace themis::exporters {

// Forward declaration
class ExporterMetrics;

/// Export statistics collected during export
struct ExportStats {
    size_t total_entities = 0;
    size_t exported_entities = 0;
    size_t failed_entities = 0;
    size_t bytes_written = 0;
    std::chrono::milliseconds duration{0};
    std::vector<std::string> errors;
    
    // Optional: Detailed metrics
    std::shared_ptr<ExporterMetrics> metrics;
    
    std::string toJson() const;
};

/// Tenant context for multi-tenant exports
struct ExportTenantContext {
    std::string tenant_id;                  // Tenant identifier
    std::string user_id;                    // User performing export
    std::vector<std::string> scopes;        // Authorization scopes (export:read, export:write, etc.)
    bool enforce_isolation = true;          // Enforce tenant data isolation
    
    /// Check if user has required scope
    bool hasScope(const std::string& scope) const {
        return std::find(scopes.begin(), scopes.end(), scope) != scopes.end();
    }
};

/// Export options for configuring export behavior
struct ExportOptions {
    // Output file path
    std::string output_path;
    
    // P1: Tenant context (optional for backward compatibility)
    std::optional<ExportTenantContext> tenant_context;
    
    // Filtering
    std::vector<std::string> include_fields;  // If empty, export all fields
    std::vector<std::string> exclude_fields;
    std::string filter_expression;            // Optional filter (e.g., "category=active")
    
    // Format options
    bool pretty_print = false;
    bool compress = false;                     // P2: Enable compression
    std::string compression_type = "gzip";     // P2: gzip, zstd, none
    int compression_level = 6;                 // P2: 1-9 for gzip, 1-22 for zstd
    
    // P2: Resource limits
    size_t max_file_size_bytes = 0;           // 0 = unlimited
    size_t max_throughput_bps = 0;            // 0 = unlimited (bytes per second)
    size_t buffer_size_bytes = 8192;          // Buffer size for streaming
    
    // Progress reporting
    std::function<void(const ExportStats&)> progress_callback;
    size_t progress_interval = 1000;  // Report every N entities
    
    // Error handling
    bool continue_on_error = true;
    size_t max_errors = 100;
};

/// Generic exporter interface
class IExporter {
public:
    virtual ~IExporter() = default;
    
    /// Export entities to the configured format
    /// @param entities Vector of entities to export
    /// @param options Export configuration
    /// @return Export statistics
    virtual ExportStats exportEntities(
        const std::vector<BaseEntity>& entities,
        const ExportOptions& options
    ) = 0;
    
    /// Get supported output formats
    virtual std::vector<std::string> getSupportedFormats() const = 0;
    
    /// Get exporter name
    virtual std::string getName() const = 0;
    
    /// Get exporter version
    virtual std::string getVersion() const = 0;
};

} // namespace themis::exporters
