/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            import_api_handler.h                               ║
  Version:         0.0.34                                             ║
  Last Modified:   2026-03-09 03:55:21                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     128                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • d88671344  2026-02-28  feat(importers): implement web-based import wizard at GET... ║
    • 47845c7e2  2026-02-27  audit: add S3 HTTP route, fix stub annotations, add API t... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "importers/importer_interface.h"
#include "importers/postgres_importer.h"
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
    void handleStartImport   (const httplib::Request& req, httplib::Response& res);
    void handleStartS3Import (const httplib::Request& req, httplib::Response& res);
    void handleJobStatus     (const httplib::Request& req, httplib::Response& res);
    void handleCancelJob     (const httplib::Request& req, httplib::Response& res);
    void handleListJobs      (const httplib::Request& req, httplib::Response& res);
    void handleMetrics       (const httplib::Request& req, httplib::Response& res);
    void handleImportWizard  (const httplib::Request& req, httplib::Response& res);

    // Helpers
    static nlohmann::json parseRequestBody(const std::string& body);
    static importers::ImportOptions optionsFromJson(const nlohmann::json& j);
    static httplib::Response& jsonOk(httplib::Response& res, const nlohmann::json& body);
    static httplib::Response& jsonError(httplib::Response& res, int status,
                                        const std::string& message);

    std::shared_ptr<importers::ImportJobRegistry> registry_;
    std::shared_ptr<importers::IImporter>         importer_;
    std::shared_ptr<importers::IImporter>         s3_importer_;
};

} // namespace server
} // namespace themis
