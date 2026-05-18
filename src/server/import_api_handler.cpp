/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            import_api_handler.cpp                             ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:50:47                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     608                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7c2cc11ffb  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • ad6e8f172c  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "server/import_api_handler.h"
#include "server/import_wizard_builder.h"
#include "utils/logger.h"

#include <httplib.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <sstream>
#include <chrono>
#include "utils/tracing.h"

namespace themis {
namespace server {

using json = nlohmann::json;
using namespace importers;

// ============================================================================
// Construction
// ============================================================================

ImportApiHandler::ImportApiHandler(
    std::shared_ptr<ImportJobRegistry> registry,
    std::shared_ptr<IImporter>         importer,
    std::shared_ptr<IImporter>         s3_importer
) : registry_(std::move(registry)), importer_(std::move(importer)),
    s3_importer_(std::move(s3_importer)) {}

// ============================================================================
// Route registration
// ============================================================================

void ImportApiHandler::registerRoutes(httplib::Server& server) {
    // POST /api/v1/import/postgresql – start async import
    server.Post("/api/v1/import/postgresql",
        [this](const httplib::Request& req, httplib::Response& res) {
            handleStartImport(req, res);
        });

    // POST /api/v1/import/mysql – start async MySQL/MariaDB dump import
    server.Post("/api/v1/import/mysql",
        [this](const httplib::Request& req, httplib::Response& res) {
            handleStartMySQLImport(req, res);
        });

    // POST /api/v1/import/s3 – start async S3 object-storage import
    server.Post("/api/v1/import/s3",
        [this](const httplib::Request& req, httplib::Response& res) {
            handleStartS3Import(req, res);
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

    // v2.0: GET /api/v1/import/schema/{job_id} – schema preview with relationships
    // NOTE: must be registered BEFORE the generic /{job_id}/status pattern so the
    // "schema" segment is not consumed as a job_id.
    server.Get(R"(/api/v1/import/schema/([^/]+))",
        [this](const httplib::Request& req, httplib::Response& res) {
            handleGetSchema(req, res);
        });

    // v2.0: POST /api/v1/import/schema/validate – pre-import FK validation
    server.Post("/api/v1/import/schema/validate",
        [this](const httplib::Request& req, httplib::Response& res) {
            handleValidateSchema(req, res);
        });

    // v2.0: PUT /api/v1/import/{job_id}/relationships – configure FK mappings
    server.Put(R"(/api/v1/import/([^/]+)/relationships)",
        [this](const httplib::Request& req, httplib::Response& res) {
            handleUpdateRelationships(req, res);
        });

    // GET /import/wizard – interactive web-based import wizard UI
    server.Get("/import/wizard",
        [this](const httplib::Request& req, httplib::Response& res) {
            handleImportWizard(req, res);
        });
}

// ============================================================================
// Route handlers
// ============================================================================

void ImportApiHandler::handleStartImport(const httplib::Request& req,
                                          httplib::Response& res) {
    auto span = Tracer::startSpan("handleStartImport");
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

void ImportApiHandler::handleStartMySQLImport(const httplib::Request& req,
                                               httplib::Response& res) {
    auto span = Tracer::startSpan("handleStartMySQLImport");

    // Resolve the MySQL importer plugin from the process-wide registry.
    // MySQLImporterSchemePlugin registers itself via REGISTER_IMPORTER_PLUGIN
    // at static-init time in mysql_importer.cpp.
    auto* plugin = IImporterPluginRegistry::instance().resolve("mysql://host");
    if (!plugin) {
        jsonError(res, 501,
                  "MySQL importer plugin is not registered on this server instance");
        return;
    }

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

    // Build ImportConfig for the plugin (passes any JSON config from options).
    ImportConfig cfg;
    cfg.source_uri  = source_path;
    cfg.json_config = body.contains("config") && body["config"].is_string()
                      ? body["config"].get<std::string>() : "{}";

    auto importer = plugin->createImporter(cfg);
    if (!importer) {
        jsonError(res, 500, "Failed to create MySQL importer instance");
        return;
    }

    THEMIS_INFO("ImportApiHandler: async MySQL import requested for '{}'", source_path);

    auto handle = importer->importDataAsync(source_path, opts);
    registry_->add(handle);

    jsonOk(res, handle->toJson());
}

void ImportApiHandler::handleStartS3Import(const httplib::Request& req,
                                            httplib::Response& res) {
    auto span = Tracer::startSpan("handleStartS3Import");
    if (!s3_importer_) {
        jsonError(res, 501,
                  "S3 importer is not configured on this server instance");
        return;
    }

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

    // Validate that the source_path is a valid s3:// URL.
    {
        std::string bucket, key;
        if (!importers::S3Importer::parseS3Url(source_path, bucket, key)) {
            jsonError(res, 400,
                      "source_path must be a valid S3 URL "
                      "(e.g. s3://bucket/key or s3://bucket/prefix/)");
            return;
        }
    }

    ImportOptions opts;
    if (body.contains("options") && body["options"].is_object()) {
        opts = optionsFromJson(body["options"]);
    }

    THEMIS_INFO("ImportApiHandler: async S3 import requested for '{}'",
                // Log only the URL scheme+bucket, not any embedded credentials.
                [&source_path]() {
                    auto pos = source_path.find('/', 5);
                    return source_path.substr(
                        0, pos != std::string::npos ? pos + 1
                                                    : source_path.size());
                }());

    auto handle = s3_importer_->importDataAsync(source_path, opts);
    registry_->add(handle);

    jsonOk(res, handle->toJson());
}

void ImportApiHandler::handleJobStatus(const httplib::Request& req,
                                        httplib::Response& res) {
    auto span = Tracer::startSpan("handleJobStatus");
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
    auto span = Tracer::startSpan("handleCancelJob");
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
    auto span = Tracer::startSpan("handleListJobs");
    auto jobs = registry_->all();
    json arr = json::array();
    for (auto& h : jobs) arr.push_back(h->toJson());
    jsonOk(res, arr);
}

void ImportApiHandler::handleMetrics(const httplib::Request& /*req*/,
                                      httplib::Response& res) {
    auto span = Tracer::startSpan("handleMetrics");
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
// Import wizard UI (web-based single-page application)
// ============================================================================

void ImportApiHandler::handleImportWizard(const httplib::Request& /*req*/,
                                           httplib::Response& res) {
    auto span = Tracer::startSpan("handleImportWizard");
    res.set_content(buildImportWizardHtml(), "text/html; charset=utf-8");
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
    if (j.contains("batch_size") && j["batch_size"].is_number_unsigned()) {
        // GAP-020: Cap batch_size to prevent unbounded memory allocation (DoS).
        // A malicious or misconfigured client sending batch_size=UINT64_MAX would
        // cause the import engine to attempt allocations of billions of records.
        static constexpr size_t kMaxBatchSize = 100'000;
        const size_t requested = j["batch_size"].get<size_t>();
        if (requested > kMaxBatchSize) {
            spdlog::warn("ImportApiHandler: batch_size {} exceeds maximum {}; clamping",
                         requested, kMaxBatchSize);
        }
        opts.batch_size = std::min(requested, kMaxBatchSize);
    }
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
    // v2.0: relationship options
    if (j.contains("preserve_relationships") && j["preserve_relationships"].is_boolean())
        opts.preserve_relationships = j["preserve_relationships"].get<bool>();
    if (j.contains("validate_references") && j["validate_references"].is_boolean())
        opts.validate_references = j["validate_references"].get<bool>();
    if (j.contains("relationship_mapping_mode") && j["relationship_mapping_mode"].is_string())
        opts.relationship_mapping_mode = j["relationship_mapping_mode"].get<std::string>();
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

// ============================================================================
// v2.0 Handlers
// ============================================================================

void ImportApiHandler::handleGetSchema([[maybe_unused]] const httplib::Request& req,
                                        httplib::Response& res) {
    auto span = Tracer::startSpan("handleGetSchema");
#ifndef THEMIS_ENABLE_POSTGRES_WIRE
    // STUB/SIMULATION NOTE (stub #294):
    // Purpose: Expose the schema-preview and schema-validate REST endpoints so that
    //          clients can discover them, while the PostgreSQL wire protocol parser
    //          required for source introspection is not linked into this build.
    // Activation: `THEMIS_ENABLE_POSTGRES_WIRE` is not defined at compile time
    //             (default build without the 'pg-wire' vcpkg feature).
    // Production Delta: GET /import/{id}/schema and POST /import/validate-schema
    //                   always return HTTP 501.  Callers cannot preview table structures
    //                   or validate relationship overrides before starting an import.
    // Removal Plan: Enable the 'pg-wire' vcpkg feature and set
    //               `-DTHEMIS_ENABLE_POSTGRES_WIRE=ON` in CMake; the `#else` branch
    //               contains the real implementation.
    //               See src/server/ROADMAP.md §Import Schema Preview.  Target: Q1 2027.
    jsonError(res, 501,
              "Schema preview requires PostgreSQL wire support; rebuild with THEMIS_ENABLE_POSTGRES_WIRE=ON");
    return;
#else
    const std::string job_id = req.matches[1];
    auto handle = registry_->get(job_id);
    if (!handle) {
        jsonError(res, 404, "Job not found: " + job_id);
        return;
    }

    // Retrieve the source_path stored in the handle's options snapshot
    const std::string source_path = handle->source_path;
    if (source_path.empty()) {
        jsonError(res, 409, "Job has no source_path available for schema preview");
        return;
    }

    // Use a fresh importer to avoid mutating the running importer's state
    auto schema_importer = std::make_shared<importers::PostgreSQLImporter>();
    schema_importer->initialize("{}");
    json schema = schema_importer->getSourceSchema(source_path);

    // Merge any custom relationship overrides
    {
        std::lock_guard<std::mutex> lk(rel_mutex_);
        auto it = relationship_overrides_.find(job_id);
        if (it != relationship_overrides_.end() && !it->second.empty()) {
            if (schema.is_object() && schema.contains("relationships")) {
                schema["relationships"] = it->second;
                schema["relationships_source"] = "manual";
            }
        }
    }

    jsonOk(res, json{{"job_id", job_id}, {"schema", schema}});
#endif
}

void ImportApiHandler::handleValidateSchema([[maybe_unused]] const httplib::Request& req,
                                             httplib::Response& res) {
    auto span = Tracer::startSpan("handleValidateSchema");
#ifndef THEMIS_ENABLE_POSTGRES_WIRE
    // STUB/SIMULATION NOTE (stub #294 — validateSchema path, same gate):
    // See handleGetSchema() above for full details.  Both schema-inspection endpoints
    // share the THEMIS_ENABLE_POSTGRES_WIRE compile-time gate.
    jsonError(res, 501,
              "Schema validation requires PostgreSQL wire support; rebuild with THEMIS_ENABLE_POSTGRES_WIRE=ON");
    return;
#else
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

    importers::ImportOptions opts;
    opts.preserve_relationships = true;
    opts.validate_references    = true;
    if (body.contains("options") && body["options"].is_object()) {
        opts = optionsFromJson(body["options"]);
        opts.preserve_relationships = true;
        opts.validate_references    = true;
    }

    // Parse schema and validate
    auto schema_importer = std::make_shared<importers::PostgreSQLImporter>();
    schema_importer->initialize("{}");
    json schema = schema_importer->getSourceSchema(source_path);

    if (!schema.is_object()) {
        jsonError(res, 422, "Could not parse schema from: " + source_path);
        return;
    }

    size_t table_count = schema.value("tables", json::array()).size();
    size_t rel_count   = schema.value("relationships", json::array()).size();
    auto   cycles      = schema.value("circular_references", json::array());

    // Validate using a temporary ImportStats
    importers::ImportStats vstats;
    // Collect validation errors from the structured_errors already added by getSourceSchema()
    json warn_arr = json::array();
    json err_arr  = json::array();
    for (const auto& e : vstats.structured_errors) {
        if (e.severity == importers::ImportErrorSeverity::WARNING) {
            warn_arr.push_back(e.message);
        } else {
            err_arr.push_back(e.message);
        }
    }

    // Report circular references as warnings
    for (const auto& c : cycles) {
        warn_arr.push_back("Circular reference detected: " + c.get<std::string>());
    }

    bool valid = err_arr.empty();
    jsonOk(res, json{
        {"valid", valid},
        {"tables", table_count},
        {"relationships", rel_count},
        {"warnings", warn_arr},
        {"errors", err_arr},
        {"circular_references", cycles}
    });
#endif
}

void ImportApiHandler::handleUpdateRelationships(const httplib::Request& req,
                                                  httplib::Response& res) {
    auto span = Tracer::startSpan("handleUpdateRelationships");
    const std::string job_id = req.matches[1];
    auto handle = registry_->get(job_id);
    if (!handle) {
        jsonError(res, 404, "Job not found: " + job_id);
        return;
    }

    json body;
    try {
        body = parseRequestBody(req.body);
    } catch (const std::exception& e) {
        jsonError(res, 400, std::string("Invalid JSON body: ") + e.what());
        return;
    }

    if (!body.is_array()) {
        jsonError(res, 400, "Request body must be a JSON array of relationship objects");
        return;
    }

    // Validate each relationship entry has required fields
    json validated = json::array();
    for (const auto& entry : body) {
        if (!entry.is_object()) continue;
        if (!entry.contains("source_table") || !entry.contains("target_table")) {
            jsonError(res, 400,
                      "Each relationship must have 'source_table' and 'target_table'");
            return;
        }
        validated.push_back(entry);
    }

    {
        std::lock_guard<std::mutex> lk(rel_mutex_);
        relationship_overrides_[job_id] = validated;
    }

    THEMIS_INFO("ImportApiHandler: {} custom relationships configured for job '{}'",
                validated.size(), job_id);

    jsonOk(res, json{
        {"job_id", job_id},
        {"relationships_configured", validated.size()}
    });
}

} // namespace server
} // namespace themis
