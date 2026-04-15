/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            import_api_handler.h                               ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:05:17                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     169                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 9bccf09a7c  2026-03-16  Changes before error encountered        ║
    • 8452353dc5  2026-03-12  Add unit tests for sync-issues-from-roadmap.py ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "importers/importer_interface.h"
#include "importers/importer_interfaces.h"
#ifdef THEMIS_ENABLE_POSTGRES_WIRE
#include "importers/postgres_importer.h"
#endif
#include "importers/s3_importer.h"

#include <memory>
#include <string>
#include <mutex>

// Forward-declare httplib types so <httplib.h> stays out of this header.
namespace httplib {
struct Request;
struct Response;
class  Server;
}

namespace themis {
namespace server {

/**
 * @brief HTTP API handler for asynchronous data imports.
 *
 * Registers routes on a cpp-httplib `Server` instance and delegates to
 * the appropriate importer's `importDataAsync()`.  An in-process
 * `ImportJobRegistry` tracks all submitted jobs.
 *
 * Routes
 * ------
 * POST   /api/v1/import/postgresql
 *   Body (JSON): { "source_path": "...", "options": { ... } }
 *   Response: { "job_id": "...", "status": "running", ... }
 *
 * POST   /api/v1/import/mysql
 *   Body (JSON): { "source_path": "...", "options": { ... } }
 *   Imports a MySQL/MariaDB mysqldump file.  The handler resolves the MySQL
 *   importer via IImporterPluginRegistry (requires MySQLImporterSchemePlugin
 *   to be registered at static-init time in mysql_importer.cpp).
 *   Response: { "job_id": "...", "status": "running", ... }
 *
 * POST   /api/v1/import/s3
 *   Body (JSON): { "source_path": "s3://bucket/key", "options": { ... } }
 *   Imports CSV/TSV/JSONL objects from S3-compatible object storage.
 *   source_path must be a valid s3:// URL (single object or prefix/).
 *   Response: { "job_id": "...", "status": "running", ... }
 *
 * GET    /api/v1/import/{job_id}/status
 *   Response: ImportHandle::toJson() – live progress + stats when done
 *
 * POST   /api/v1/import/{job_id}/cancel
 *   Signals cancellation via IImporter::cancel(); returns updated status.
 *
 * GET    /api/v1/import/metrics
 *   Returns Prometheus text-format counters aggregated across all jobs.
 *   Metric names follow the `themisdb_import_*` convention.
 *
 * GET    /api/v1/import/jobs
 *   Returns JSON array of all known job statuses.
 *
 * GET    /import/wizard
 *   Serves the interactive web-based import wizard (single-page HTML application).
 *   No authentication required for the page itself; all data operations are
 *   delegated to the existing /api/v1/import/* REST endpoints.
 *
 * --- v2.0 endpoints ---
 *
 * GET    /api/v1/import/schema/{job_id}
 *   Returns the detected schema (tables, FKs, indexes, graph relationships)
 *   for the dump file associated with a completed or in-progress job.
 *   Response: { "tables": [...], "relationships": [...], "circular_references": [...] }
 *
 * POST   /api/v1/import/schema/validate
 *   Body (JSON): { "source_path": "...", "options": { ... } }
 *   Validates FK mappings without starting a full data import.
 *   Response: { "valid": true|false, "tables": N, "relationships": N,
 *               "warnings": [...], "errors": [...] }
 *
 * PUT    /api/v1/import/{job_id}/relationships
 *   Body (JSON): array of { "edge_type", "source_table", "source_column",
 *                            "target_table", "target_column", "cardinality" }
 *   Stores custom relationship mappings for a job (overrides auto-detection).
 *   Response: { "job_id": "...", "relationships_configured": N }
 */
class ImportApiHandler {
public:
    /**
     * @param registry    Shared job registry (may be shared with other handlers)
     * @param importer    The default importer (PostgreSQL / generic)
     * @param s3_importer The S3 importer instance for s3:// source paths
     *                    (may be nullptr to disable the /api/v1/import/s3 route)
     */
    explicit ImportApiHandler(
        std::shared_ptr<importers::ImportJobRegistry> registry,
        std::shared_ptr<importers::IImporter>        importer,
        std::shared_ptr<importers::IImporter>        s3_importer = nullptr
    );

    ~ImportApiHandler() = default;

    /**
     * @brief Register all /api/v1/import/* routes on @p server.
     *
     * Call once after constructing the handler, before calling
     * `server.listen()`.
     */
    void registerRoutes(httplib::Server& server);

private:
    // Route handlers
    void handleStartImport      (const httplib::Request& req, httplib::Response& res);
    void handleStartMySQLImport (const httplib::Request& req, httplib::Response& res);
    void handleStartS3Import    (const httplib::Request& req, httplib::Response& res);
    void handleJobStatus        (const httplib::Request& req, httplib::Response& res);
    void handleCancelJob        (const httplib::Request& req, httplib::Response& res);
    void handleListJobs         (const httplib::Request& req, httplib::Response& res);
    void handleMetrics          (const httplib::Request& req, httplib::Response& res);
    void handleImportWizard     (const httplib::Request& req, httplib::Response& res);

    // v2.0 Route handlers
    void handleGetSchema        (const httplib::Request& req, httplib::Response& res);
    void handleValidateSchema   (const httplib::Request& req, httplib::Response& res);
    void handleUpdateRelationships(const httplib::Request& req, httplib::Response& res);

    // Helpers
    static nlohmann::json parseRequestBody(const std::string& body);
    static importers::ImportOptions optionsFromJson(const nlohmann::json& j);
    static httplib::Response& jsonOk(httplib::Response& res, const nlohmann::json& body);
    static httplib::Response& jsonError(httplib::Response& res, int status,
                                        const std::string& message);

    std::shared_ptr<importers::ImportJobRegistry> registry_;
    std::shared_ptr<importers::IImporter>         importer_;
    std::shared_ptr<importers::IImporter>         s3_importer_;

    // v2.0: per-job custom relationship override storage
    // key = job_id, value = JSON array of RelationshipMapping objects
    std::mutex                                    rel_mutex_;
    std::map<std::string, nlohmann::json>         relationship_overrides_;
};

} // namespace server
} // namespace themis
