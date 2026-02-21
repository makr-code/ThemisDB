/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            import_api_handler.cpp                             ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:40:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     261                                            ║
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

#include "server/import_api_handler.h"
#include "utils/logger.h"

#include <httplib.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <chrono>

namespace themis {
namespace server {

using json = nlohmann::json;
using namespace importers;

// ============================================================================
// Construction
// ============================================================================

ImportApiHandler::ImportApiHandler(
    std::shared_ptr<ImportJobRegistry> registry,
    std::shared_ptr<IImporter>         importer
) : registry_(std::move(registry)), importer_(std::move(importer)) {}

// ============================================================================
// Route registration
// ============================================================================

void ImportApiHandler::registerRoutes(httplib::Server& server) {
    // POST /api/v1/import/postgresql – start async import
    server.Post("/api/v1/import/postgresql",
        [this](const httplib::Request& req, httplib::Response& res) {
            handleStartImport(req, res);
        });

    // GET /api/v1/import/jobs – list all jobs
    server.Get("/api/v1/import/jobs",
        [this](const httplib::Request& req, httplib::Response& res) {
            handleListJobs(req, res);
        });

    // GET /api/v1/import/metrics – Prometheus text format
    server.Get("/api/v1/import/metrics",
        [this](const httplib::Request& req, httplib::Response& res) {
            handleMetrics(req, res);
        });

    // GET /api/v1/import/{job_id}/status
    server.Get(R"(/api/v1/import/([^/]+)/status)",
        [this](const httplib::Request& req, httplib::Response& res) {
            handleJobStatus(req, res);
        });

    // POST /api/v1/import/{job_id}/cancel
    server.Post(R"(/api/v1/import/([^/]+)/cancel)",
        [this](const httplib::Request& req, httplib::Response& res) {
            handleCancelJob(req, res);
        });
}

// ============================================================================
// Route handlers
// ============================================================================

void ImportApiHandler::handleStartImport(const httplib::Request& req,
                                          httplib::Response& res) {
    json body;
    try {
        body = parseRequestBody(req.body);
    } catch (const std::exception& e) {
        jsonError(res, 400, std::string("Invalid JSON body: ") + e.what());
        return;
    }

    if (!body.contains("source_path") || !body["source_path"].is_string()) {
        jsonError(res, 400, "Missing required field: source_path");
        return;
    }
    const std::string source_path = body["source_path"].get<std::string>();

    ImportOptions opts;
    if (body.contains("options") && body["options"].is_object()) {
        opts = optionsFromJson(body["options"]);
    }

    THEMIS_INFO("ImportApiHandler: async import requested for '{}'", source_path);

    auto handle = importer_->importDataAsync(source_path, opts);
    registry_->add(handle);

    jsonOk(res, handle->toJson());
}

void ImportApiHandler::handleJobStatus(const httplib::Request& req,
                                        httplib::Response& res) {
    const std::string job_id = req.matches[1];
    auto handle = registry_->get(job_id);
    if (!handle) {
        jsonError(res, 404, "Job not found: " + job_id);
        return;
    }
    jsonOk(res, handle->toJson());
}

void ImportApiHandler::handleCancelJob(const httplib::Request& req,
                                        httplib::Response& res) {
    const std::string job_id = req.matches[1];
    auto handle = registry_->get(job_id);
    if (!handle) {
        jsonError(res, 404, "Job not found: " + job_id);
        return;
    }
    if (!handle->running.load()) {
        jsonError(res, 409, "Job is not running (status: " +
                  std::string(handle->getStatus() == ImportStatus::COMPLETED
                              ? "completed" : "unknown") + ")");
        return;
    }
    // NOTE: cancel() signals the shared `cancelled_` flag on the importer.
    // If multiple async jobs share the same IImporter instance all running jobs
    // will be cancelled.  For isolated per-job cancellation, use a separate
    // importer instance per job (e.g. one PostgreSQLImporter per long-running import).
    importer_->cancel();
    THEMIS_INFO("ImportApiHandler: cancel requested for job '{}'", job_id);
    jsonOk(res, handle->toJson());
}

void ImportApiHandler::handleListJobs(const httplib::Request& /*req*/,
                                       httplib::Response& res) {
    auto jobs = registry_->all();
    json arr = json::array();
    for (auto& h : jobs) arr.push_back(h->toJson());
    jsonOk(res, arr);
}

void ImportApiHandler::handleMetrics(const httplib::Request& /*req*/,
                                      httplib::Response& res) {
    // Aggregate counters across all known jobs and emit Prometheus text format.
    auto jobs = registry_->all();

    size_t total_imported = 0, total_failed = 0, total_skipped = 0;
    size_t jobs_running = 0, jobs_completed = 0;
    double total_duration = 0.0;

    for (auto& h : jobs) {
        switch (h->getStatus()) {
            case ImportStatus::RUNNING:   ++jobs_running;   break;
            case ImportStatus::COMPLETED: ++jobs_completed; break;
            default: break;
        }
        // If the job is done and the future is ready, read final stats
        if (h->getStatus() == ImportStatus::COMPLETED &&
            h->future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            try {
                auto stats = h->future.get();
                total_imported += stats.imported_records;
                total_failed   += stats.failed_records;
                total_skipped  += stats.skipped_records;
                total_duration += stats.elapsed_seconds;
            } catch (...) {}
        }
    }

    std::ostringstream prom;
    prom << "# HELP themisdb_import_jobs_total Total import jobs tracked\n"
         << "# TYPE themisdb_import_jobs_total gauge\n"
         << "themisdb_import_jobs_total{status=\"running\"} " << jobs_running << "\n"
         << "themisdb_import_jobs_total{status=\"completed\"} " << jobs_completed << "\n"
         << "# HELP themisdb_import_rows_total Rows processed across all completed jobs\n"
         << "# TYPE themisdb_import_rows_total counter\n"
         << "themisdb_import_rows_total{status=\"imported\"} " << total_imported << "\n"
         << "themisdb_import_rows_total{status=\"failed\"} "   << total_failed   << "\n"
         << "themisdb_import_rows_total{status=\"skipped\"} "  << total_skipped  << "\n"
         << "# HELP themisdb_import_duration_seconds_total Total import wall-clock time\n"
         << "# TYPE themisdb_import_duration_seconds_total counter\n"
         << "themisdb_import_duration_seconds_total " << total_duration << "\n";

    res.set_content(prom.str(), "text/plain; version=0.0.4; charset=utf-8");
}

// ============================================================================
// Helpers
// ============================================================================

json ImportApiHandler::parseRequestBody(const std::string& body) {
    return json::parse(body);
}

ImportOptions ImportApiHandler::optionsFromJson(const json& j) {
    ImportOptions opts;
    if (j.contains("dry_run") && j["dry_run"].is_boolean())
        opts.dry_run = j["dry_run"].get<bool>();
    if (j.contains("continue_on_error") && j["continue_on_error"].is_boolean())
        opts.continue_on_error = j["continue_on_error"].get<bool>();
    if (j.contains("batch_size") && j["batch_size"].is_number_unsigned())
        opts.batch_size = j["batch_size"].get<size_t>();
    if (j.contains("default_namespace") && j["default_namespace"].is_string())
        opts.default_namespace = j["default_namespace"].get<std::string>();
    if (j.contains("preserve_ids") && j["preserve_ids"].is_boolean())
        opts.preserve_ids = j["preserve_ids"].get<bool>();
    if (j.contains("update_existing") && j["update_existing"].is_boolean())
        opts.update_existing = j["update_existing"].get<bool>();
    if (j.contains("skip_duplicates") && j["skip_duplicates"].is_boolean())
        opts.skip_duplicates = j["skip_duplicates"].get<bool>();
    if (j.contains("include_tables") && j["include_tables"].is_array()) {
        for (auto& t : j["include_tables"])
            if (t.is_string()) opts.include_tables.push_back(t.get<std::string>());
    }
    if (j.contains("exclude_tables") && j["exclude_tables"].is_array()) {
        for (auto& t : j["exclude_tables"])
            if (t.is_string()) opts.exclude_tables.push_back(t.get<std::string>());
    }
    if (j.contains("max_row_size_bytes") && j["max_row_size_bytes"].is_number_unsigned())
        opts.max_row_size_bytes = j["max_row_size_bytes"].get<size_t>();
    if (j.contains("max_statement_size_bytes") && j["max_statement_size_bytes"].is_number_unsigned())
        opts.max_statement_size_bytes = j["max_statement_size_bytes"].get<size_t>();
    if (j.contains("enforce_utf8") && j["enforce_utf8"].is_boolean())
        opts.enforce_utf8 = j["enforce_utf8"].get<bool>();
    if (j.contains("checkpoint_file") && j["checkpoint_file"].is_string())
        opts.checkpoint_file = j["checkpoint_file"].get<std::string>();
    if (j.contains("type_overrides") && j["type_overrides"].is_object()) {
        for (auto& [k, v] : j["type_overrides"].items())
            if (v.is_string()) opts.type_overrides[k] = v.get<std::string>();
    }
    return opts;
}

httplib::Response& ImportApiHandler::jsonOk(httplib::Response& res, const json& body) {
    res.status = 200;
    res.set_content(body.dump(2), "application/json");
    return res;
}

httplib::Response& ImportApiHandler::jsonError(httplib::Response& res,
                                                int status,
                                                const std::string& message) {
    res.status = status;
    res.set_content(json{{"error", message}}.dump(), "application/json");
    return res;
}

} // namespace server
} // namespace themis
