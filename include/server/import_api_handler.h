/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            import_api_handler.h                               ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:34:59                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     106                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 73b1edb94  2026-02-20  Importer module: production readiness, observability & fe... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "importers/importer_interface.h"
#include "importers/postgres_importer.h"

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
 * @brief HTTP API handler for asynchronous PostgreSQL dump imports.
 *
 * Registers routes on a cpp-httplib `Server` instance and delegates to
 * `PostgreSQLImporter::importDataAsync()`.  An in-process
 * `ImportJobRegistry` tracks all submitted jobs.
 *
 * Routes
 * ------
 * POST   /api/v1/import/postgresql
 *   Body (JSON): { "source_path": "...", "options": { ... } }
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
 */
class ImportApiHandler {
public:
    /**
     * @param registry  Shared job registry (may be shared with other handlers)
     * @param importer  The importer to use (must outlive this handler)
     */
    explicit ImportApiHandler(
        std::shared_ptr<importers::ImportJobRegistry> registry,
        std::shared_ptr<importers::IImporter>        importer
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
    void handleStartImport (const httplib::Request& req, httplib::Response& res);
    void handleJobStatus   (const httplib::Request& req, httplib::Response& res);
    void handleCancelJob   (const httplib::Request& req, httplib::Response& res);
    void handleListJobs    (const httplib::Request& req, httplib::Response& res);
    void handleMetrics     (const httplib::Request& req, httplib::Response& res);

    // Helpers
    static nlohmann::json parseRequestBody(const std::string& body);
    static importers::ImportOptions optionsFromJson(const nlohmann::json& j);
    static httplib::Response& jsonOk(httplib::Response& res, const nlohmann::json& body);
    static httplib::Response& jsonError(httplib::Response& res, int status,
                                        const std::string& message);

    std::shared_ptr<importers::ImportJobRegistry> registry_;
    std::shared_ptr<importers::IImporter>         importer_;
};

} // namespace server
} // namespace themis
