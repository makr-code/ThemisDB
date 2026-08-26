/**
 * @file import_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=3, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/import_api_handler.h"
#include <stdexcept>
#include "server/import_wizard_builder.h"
#include "utils/logger.h"

#include <httplib.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <sstream>
#include <chrono>
#include "utils/tracing.h"

#ifdef ERROR
#undef ERROR
#endif

namespace themis {
namespace server {

using json = nlohmann::json;
using namespace importers;

namespace {

std::shared_ptr<IImporter> selectSchemaImporter(
    const std::string& source_path,
    const std::shared_ptr<IImporter>& default_importer,
    const std::shared_ptr<IImporter>& s3_importer
) {
    if (source_path.rfind("s3://", 0) == 0 && s3_importer) {
        return s3_importer;
    }
    return default_importer;
}

bool hasUsableSchemaPayload(const json& schema) {
    if (schema.is_null()) {
        return false;
    }
    if (schema.is_object()) {
        if (schema.empty()) {
            return false;
        }
        if (schema.contains("tables") && schema["tables"].is_array()) {
            return !schema["tables"].empty();
        }
        return true;
    }
    return schema.is_array() && !schema.empty();
}

} // namespace

// ============================================================================
// Schema bridge setters (stub #294)
// ============================================================================

void ImportApiHandler::setSchemaInspectorFn(SchemaInspectorFn fn) {
    schemaInspectorFn_ = std::move(fn);
}

void ImportApiHandler::clearSchemaInspectorFn() {
    schemaInspectorFn_ = nullptr;
}

void ImportApiHandler::setSchemaValidatorFn(SchemaValidatorFn fn) {
    schemaValidatorFn_ = std::move(fn);
}

void ImportApiHandler::clearSchemaValidatorFn() {
    schemaValidatorFn_ = nullptr;
}

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
    THEMIS_INFO("[AUDIT] POST /api/v1/import/postgresql path='{}' user='<server-auth>' result=ALLOW",
                source_path);

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
    THEMIS_INFO("[AUDIT] POST /api/v1/import/mysql path='{}' user='<server-auth>' result=ALLOW",
                source_path);

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
    auto& s3_importer = *s3_importer_;

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

    auto handle = s3_importer.importDataAsync(source_path, opts);
    registry_->add(handle);

    jsonOk(res, handle->toJson());
}

void ImportApiHandler::handleJobStatus(const httplib::Request& req,
                                        httplib::Response& res) {
    auto span = Tracer::startSpan("handleJobStatus");
    const std::string job_id = req.matches[1];
    auto job = registry_->getJsonSnapshot(job_id);
    if (!job.has_value()) {
        jsonError(res, 404, "Job not found: " + job_id);
        return;
    }
    jsonOk(res, *job);
}

void ImportApiHandler::handleCancelJob(const httplib::Request& req,
                                        httplib::Response& res) {
    auto span = Tracer::startSpan("handleCancelJob");
    const std::string job_id = req.matches[1];
    auto snapshot = registry_->getRunningAndJsonSnapshot(job_id);
    if (!snapshot.has_value()) {
        jsonError(res, 404, "Job not found: " + job_id);
        return;
    }
    if (!snapshot->first) {
        const std::string status = snapshot->second.value("status", "unknown");
        jsonError(res, 409, "Job is not running (status: " + status + ")");
        return;
    }
    // NOTE: cancel() signals the shared `cancelled_` flag on the importer.
    // If multiple async jobs share the same IImporter instance all running jobs
    // will be cancelled.  For isolated per-job cancellation, use a separate
    // importer instance per job (e.g. one PostgreSQLImporter per long-running import).
    importer_->cancel();
    THEMIS_INFO("ImportApiHandler: cancel requested for job '{}'", job_id);
    auto updated = registry_->getJsonSnapshot(job_id);
    jsonOk(res, updated.value_or(snapshot->second));
}

void ImportApiHandler::handleListJobs(const httplib::Request& /*req*/,
                                       httplib::Response& res) {
    auto span = Tracer::startSpan("handleListJobs");
    auto jobs = registry_->allJsonSnapshots();
    jsonOk(res, jobs);
}

void ImportApiHandler::handleMetrics(const httplib::Request& /*req*/,
                                      httplib::Response& res) {
    auto span = Tracer::startSpan("handleMetrics");
    // Aggregate counters across all known jobs and emit Prometheus text format.
    auto jobs = registry_->allJsonSnapshots();

    size_t total_imported = 0, total_failed = 0, total_skipped = 0;
    size_t jobs_running = 0, jobs_completed = 0;
    double total_duration = 0.0;

    for (const auto& job : jobs) {
        const std::string status = job.value("status", "unknown");
        if (status == "running") {
            ++jobs_running;
        } else if (status == "completed") {
            ++jobs_completed;
        }

        const auto stats_it = job.find("stats");
        if (status == "completed" && stats_it != job.end() && stats_it->is_object()) {
            total_imported += stats_it->value("imported_records", 0ull);
            total_failed += stats_it->value("failed_records", 0ull);
            total_skipped += stats_it->value("skipped_records", 0ull);
            total_duration += stats_it->value("elapsed_seconds", 0.0);
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

void ImportApiHandler::handleGetSchema(const httplib::Request& req,
                                        httplib::Response& res) {
    auto span = Tracer::startSpan("handleGetSchema");
    const std::string job_id = req.matches[1];
    auto source_path_opt = registry_->getSourcePathSnapshot(job_id);
    if (!source_path_opt.has_value()) {
        jsonError(res, 404, "Job not found: " + job_id);
        return;
    }

    // Retrieve the source_path stored in the handle's options snapshot
    const std::string source_path = *source_path_opt;
    if (source_path.empty()) {
        jsonError(res, 409, "Job has no source_path available for schema preview");
        return;
    }

    std::shared_ptr<importers::IImporter> schema_importer = importer_;
    if (source_path.rfind("s3://", 0) == 0 && s3_importer_) {
        schema_importer = s3_importer_;
    }
    if (!schema_importer) {
        jsonError(res, 503, "No importer is configured for schema preview");
        return;
    }

    json schema;
    try {
        schema = schema_importer->getSourceSchema(source_path);
    } catch (const std::exception& e) {
        jsonError(res, 422, std::string("Schema preview failed: ") + e.what());
        return;
    }
    if (!schema.is_object() || !schema.contains("tables") || !schema["tables"].is_array()) {
        jsonError(res, 422, "Could not parse schema from: " + source_path);
        return;
    }

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
}

void ImportApiHandler::handleValidateSchema(const httplib::Request& req,
                                             httplib::Response& res) {
    auto span = Tracer::startSpan("handleValidateSchema");
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

    std::shared_ptr<importers::IImporter> schema_importer = importer_;
    if (source_path.rfind("s3://", 0) == 0 && s3_importer_) {
        schema_importer = s3_importer_;
    }
    if (!schema_importer) {
        jsonError(res, 503, "No importer is configured for schema validation");
        return;
    }

    json schema;
    try {
        schema = schema_importer->getSourceSchema(source_path);
    } catch (const std::exception& e) {
        jsonError(res, 422, std::string("Schema validation failed: ") + e.what());
        return;
    }

    if (!schema.is_object() || !schema.contains("tables") || !schema["tables"].is_array()) {
        jsonError(res, 422, "Could not parse schema from: " + source_path);
        return;
    }

    size_t table_count = schema["tables"].size();
    size_t rel_count   = schema.value("relationships", json::array()).size();
    auto   cycles      = schema.value("circular_references", json::array());

    json warn_arr = json::array();
    json err_arr  = json::array();

    const auto append_strings = [](const json& src, const char* key, json& dst) {
        if (!src.contains(key) || !src[key].is_array()) {
            return;
        }
        for (const auto& entry : src[key]) {
            if (entry.is_string()) {
                dst.push_back(entry.get<std::string>());
            }
        }
    };
    append_strings(schema, "warnings", warn_arr);
    append_strings(schema, "errors", err_arr);

    if (schema.contains("structured_errors") && schema["structured_errors"].is_array()) {
        for (const auto& entry : schema["structured_errors"]) {
            if (!entry.is_object()) continue;
            const std::string message = entry.value("message", std::string{});
            if (message.empty()) continue;
            const int severity = entry.value("severity", static_cast<int>(importers::ImportErrorSeverity::ERROR));
            if (severity == static_cast<int>(importers::ImportErrorSeverity::WARNING)) {
                warn_arr.push_back(message);
            } else {
                err_arr.push_back(message);
            }
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
}

void ImportApiHandler::handleUpdateRelationships(const httplib::Request& req,
                                                  httplib::Response& res) {
    auto span = Tracer::startSpan("handleUpdateRelationships");
    const std::string job_id = req.matches[1];
    if (!registry_->getSourcePathSnapshot(job_id).has_value()) {
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
