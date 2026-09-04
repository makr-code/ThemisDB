/**
 * @file export_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/export_api_handler.h"
#include "exporters/aql_predicate_filter.h"
#include "exporters/exporter_errors.h"
#include "governance/policy_engine.h"
#include "utils/audit_logger.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "index/secondary_index.h"
#include "utils/input_validator.h"
#include "utils/logger.h"
#include "utils/tracing.h"
#include <nlohmann/json.hpp>
#include <openssl/crypto.h>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <random>
#include <iomanip>
#include <stdexcept>

using json = nlohmann::json;

namespace themis {
namespace server {

namespace {

constexpr size_t kMaxExportFieldLength = 256;

std::filesystem::path resolveExportOutputDir() {
    std::error_code ec = {};
    auto base_dir = std::filesystem::temp_directory_path(ec);
    if (ec || base_dir.empty()) {
        ec.clear();
        base_dir = std::filesystem::current_path(ec);
        if (ec || base_dir.empty()) {
            base_dir = std::filesystem::path(".");
        }
    }

    auto export_dir = base_dir / "themis_exports";
    ec.clear();
    std::filesystem::create_directories(export_dir, ec);
    return export_dir;
}

bool isValidExportField(std::string_view value) {
    themis::utils::InputValidator validator;
    return validator.validateStringLength(std::string(value), kMaxExportFieldLength) &&
           validator.validateHeaderValue(std::string(value));
}

bool isValidExportId(std::string_view value) {
    themis::utils::InputValidator validator;
    return !value.empty() &&
           validator.validateStringLength(std::string(value), kMaxExportFieldLength) &&
           validator.validatePathSegment(std::string(value));
}

} // namespace

ExportApiHandler::ExportApiHandler(
    std::shared_ptr<RocksDBWrapper> storage,
    std::shared_ptr<SecondaryIndexManager> secondary_index)
    : storage_(std::move(storage))
    , secondary_index_(std::move(secondary_index)) {
}

ExportApiHandler::~ExportApiHandler() = default;

http::response<http::string_body> ExportApiHandler::handleExportJsonlLlm(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("POST /export/jsonl-llm");
    
    // Validate admin authentication
    if (!validateAdminToken(req)) {
        span.setStatus(false, "Unauthorized");
        return errorResponse(http::status::unauthorized, "Unauthorized: Admin token required");
    }

    try {
        // Parse request body
        auto request_json = json::parse(req.body());

        // Build AQL query from parameters
        std::string aql_query = buildAqlQuery(request_json);
        span.setAttribute("export.aql_query", aql_query);
        
        THEMIS_INFO("JSONL LLM Export request: query={}", aql_query);
        
        // Load JSONL LLM exporter plugin
        auto& pm = plugins::PluginManager::instance();
        auto result = pm.loadPlugin("jsonl_llm_exporter");
        if (!result.has_value()) {
            span.setStatus(false, "Plugin not found");
            return errorResponse(http::status::internal_server_error,
                fmt::format("JSONL LLM exporter plugin not found: {}", 
                            result.error().message()));
        }
        auto* plugin = *result;

        auto* exporter = static_cast<exporters::IExporter*>(plugin->getInstance());
        if (!exporter) {
            return errorResponse(http::status::internal_server_error,
                "Failed to get exporter instance");
        }

        // Generate export ID and output path
        std::string export_id = generateExportId();
        std::string output_path =
            (resolveExportOutputDir() / ("export_" + export_id + ".jsonl")).string();

        // Configure export options
        exporters::ExportOptions export_options;
        export_options.output_path = output_path;

        // Wire PolicyEngine and AuditLogger for per-collection authorization (EXP-001).
        // Populate collection_name and requesting_user so enforceExportPolicy() can
        // build a complete ModelTrainingExportRequest.
        if (request_json.contains("collection") && request_json["collection"].is_string()) {
            export_options.collection_name = request_json["collection"].get<std::string>();
            if (!isValidExportField(export_options.collection_name)) {
                return errorResponse(http::status::bad_request,
                    "Invalid export field: collection");
            }
        }
        if (request_json.contains("requesting_user") && request_json["requesting_user"].is_string()) {
            export_options.requesting_user = request_json["requesting_user"].get<std::string>();
            if (!isValidExportField(export_options.requesting_user)) {
                return errorResponse(http::status::bad_request,
                    "Invalid export field: requesting_user");
            }
        }
        export_options.policy_engine = policy_engine_;
        export_options.audit_logger  = audit_logger_;

        // Support AQL predicate filtering via the "filter" request parameter.
        // Example: {"filter": "doc.category == \"active\" AND doc.score >= 0.5"}
        if (request_json.contains("filter") && request_json["filter"].is_string()) {
            const std::string filter_expr = request_json["filter"].get<std::string>();
            // Validate the AQL predicate early to return a 400 instead of a 500.
            // AqlPredicateFilter construction parses the expression; any syntax error throws.
            try {
                exporters::AqlPredicateFilter syntax_check(filter_expr);
            } catch (const exporters::AqlPredicateFilterException& e) {
                return errorResponse(http::status::bad_request,
                    std::string("Invalid AQL filter expression: ") + e.what());
            }
            export_options.filter_expression = filter_expr;
        }
        
        // Query database: scan all entities and apply filter conditions
        // derived from the same parameters used to build the AQL query
        std::vector<BaseEntity> entities;
        {
            // Build a set of field→value filters from the request parameters
            // (mirrors the conditions built by buildAqlQuery)
            std::vector<std::pair<std::string, std::string>> filters;
            auto add_filter = [&]([[maybe_unused]] const char* field) {
                if (request_json.contains(field) && request_json[field].is_string()) {
                    filters.emplace_back(field, request_json[field].get<std::string>());
                }
            };
            add_filter("theme");    // stored as "category"
            add_filter("domain");
            add_filter("subject");

            // Numeric min_rating handled separately
            std::optional<double> min_rating = {};

            if (request_json.contains("min_rating") &&
                request_json["min_rating"].is_number()) {
                min_rating = request_json["min_rating"].get<double>();
            }

            // Optional date range
            std::optional<std::string> from_date, to_date;
            if (request_json.contains("from_date") &&
                request_json["from_date"].is_string()) {
                from_date = request_json["from_date"].get<std::string>();
            }
            if (request_json.contains("to_date") &&
                request_json["to_date"].is_string()) {
                to_date = request_json["to_date"].get<std::string>();
            }

            // Apply max_records cap to avoid unbounded exports
            constexpr size_t MAX_EXPORT_RECORDS = 100000;
            size_t record_count = 0;

            storage_->scanAll([&](std::string_view key, std::string_view value) -> bool {
                if (record_count >= MAX_EXPORT_RECORDS) { return false; }

                // Deserialize entity
                BaseEntity entity;
                try {
                    entity = BaseEntity::fromJson(key, value);
                } catch (...) {
                    THEMIS_DEBUG([[maybe_unused]] "export_api_handler: unhandled exception caught");
                    return true; // skip malformed records
                }

                // Apply filters: check JSON fields
                try {
                    auto doc = nlohmann::json::parse(entity.toJson());

                    // String equality filters
                    for (const auto& [field, val] : filters) {
                        // "theme" is stored as "category" in the entity
                        const std::string& doc_field =
                            (field == "theme") ? "category" : field;
                        if (doc.contains(doc_field)) {
                            if (doc[doc_field].is_string() &&
                                doc[doc_field].get<std::string>() != val) {
                                return true; // filtered out
                            }
                        }
                    }

                    // Numeric rating filter
                    if (min_rating.has_value() && doc.contains("rating") &&
                        doc["rating"].is_number()) {
                        if (doc["rating"].get<double>() < *min_rating) {
                            return true;
                        }
                    }

                    // Date range filter (lexicographic comparison on ISO 8601)
                    if ((from_date.has_value() || to_date.has_value()) &&
                        doc.contains("created_at") && doc["created_at"].is_string()) {
                        const std::string& dt = doc["created_at"].get<std::string>();
                        if (from_date.has_value() && dt < *from_date) { return true; }
                        if (to_date.has_value()   && dt > *to_date)   { return true; }
                    }
                } catch (...) {
                    THEMIS_WARN([[maybe_unused]] "export_api_handler: unhandled exception caught");
                    return true; // skip malformed records
                }

                entities.push_back(std::move(entity));
                ++record_count;
                return true; // continue scan
            });
        }
        
        // Perform export
        auto stats = exporter->exportEntities(entities, export_options);

        // Read exported file and stream back
        std::ifstream exported_file(output_path);
        std::stringstream buffer = {};
        buffer << exported_file.rdbuf();
        std::string jsonl_content = buffer.str();
        exported_file.close();

        // Cleanup temp file
        std::filesystem::remove(output_path);

        // Create streaming response
        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::content_type, "application/x-ndjson");
        
        // Build a safe filename: strip all characters that could break the
        // Content-Disposition header value or inject HTTP header fields.
        // Allow only alphanumeric, hyphen, underscore, and period.
        auto sanitize_filename_part = [](const std::string& raw) -> std::string {
            std::string safe = {};
            safe.reserve(raw.size());
            std::copy_if(raw.begin(), raw.end(), std::back_inserter(safe),
                [](unsigned char c) {
                    return std::isalnum(c) || c == '-' || c == '_' || c == '.';
                });
            return safe;
        };

        std::string filename = "export_" + export_id;
        if (request_json.contains("theme") && request_json["theme"].is_string()) {
            const std::string safe_theme =
                sanitize_filename_part(request_json["theme"].get<std::string>());
            if (!safe_theme.empty()) {
                filename += "_" + safe_theme;
            }
        }
        filename += ".jsonl";
        
        res.set(http::field::content_disposition,
            "attachment; filename=\"" + filename + "\"");
        res.body() = jsonl_content;
        res.prepare_payload();
        
        THEMIS_INFO("JSONL LLM Export completed: export_id={}, records={}", 
                    export_id, stats.exported_entities);
        
        return res;

    } catch (const json::exception& e) {
        return errorResponse(http::status::bad_request,
            std::string("JSON parsing error: ") + e.what());
    } catch (const exporters::ExporterException& e) {
        // Map ERR_EXPORT_POLICY_DENIED → 403 Forbidden (EXP-001).
        // All other exporter exceptions surface as 500 so callers can
        // distinguish authorization failures from infrastructure faults.
        if (e.getErrorCode() == errors::ErrorCode::ERR_EXPORT_POLICY_DENIED) {
            return errorResponse(http::status::forbidden,
                std::string("Export forbidden: ") + e.what());
        }
        return errorResponse(http::status::internal_server_error,
            std::string("Export error: ") + e.what());
    } catch (const std::exception& e) {
        return errorResponse(http::status::internal_server_error,
            std::string("Export error: ") + e.what());
    }
}

http::response<http::string_body> ExportApiHandler::handleExportStatus(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("GET /export/:id/status");
    
    // Validate admin authentication
    if (!validateAdminToken(req)) {
        span.setStatus(false, "Unauthorized");
        return errorResponse(http::status::unauthorized, "Unauthorized: Admin token required");
    }

    try {
        // Extract export_id from path
        std::string target(req.target().data(), req.target().size());
        auto last_slash = target.find_last_of('/');
        if (last_slash == std::string::npos) {
            span.setStatus(false, "Invalid export ID");
            return errorResponse(http::status::bad_request, "Invalid export ID");
        }
        
        std::string export_id = target.substr(last_slash + 1);
        if (!isValidExportId(export_id)) {
            span.setStatus(false, "Invalid export ID");
            return errorResponse(http::status::bad_request, "Invalid export ID");
        }
        span.setAttribute("export.id", export_id);

        // Find export job
        std::lock_guard<std::mutex> lock(export_jobs_mutex_);
        auto it = export_jobs_.find(export_id);
        if (it == export_jobs_.end()) {
            return errorResponse(http::status::not_found, "Export job not found");
        }

        const auto& job = it->second;
        
        // Build response
        json response;
        response["export_id"] = job.export_id;
        response["status"] = job.status;
        response["progress"] = job.progress;
        response["records_processed"] = job.records_processed;
        response["records_total"] = job.records_total;
        
        if (job.status == "completed") {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                job.completed_at - job.started_at);
            response["duration_ms"] = duration.count();
        }
        
        if (!job.error_message.empty()) {
            response["error"] = job.error_message;
        }

        return jsonResponse(http::status::ok, response.dump(2));

    } catch (const std::exception& e) {
        return errorResponse(http::status::internal_server_error,
            std::string("Status check error: ") + e.what());
    }
}

std::string ExportApiHandler::buildAqlQuery([[maybe_unused]] const json& request_json) {
    // GAP-004: Prevent AQL injection (CWE-89).
    // String fields (theme, domain, subject, from_date, to_date) are embedded
    // inside single-quoted AQL literals.  Without validation a value like
    //   "theme": "x' AND 1=1 OR category='admin"
    // would break out of the quoted context and inject arbitrary predicates.
    //
    // Defence-in-depth: reject any string value that contains characters which
    // cannot appear in a well-formed field value (single quote, semicolons,
    // parentheses, backticks, and the comment introducer "--").  Numeric fields
    // (min_rating) are safe because they are converted via std::to_string().
    // The free-form custom "query" field is validated through the existing
    // AqlPredicateFilter parser which raises AqlPredicateFilterException on
    // syntactically invalid expressions.

    auto validateStringField = [](const std::string& value, const std::string& field_name) {
        static const std::string kForbiddenChars = "'\"`;\\";
        for (char c : value) {
            if (kForbiddenChars.find(c) != std::string::npos) {
                throw std::invalid_argument(
                    "Export request field '" + field_name +
                    "' contains forbidden character: " + c);
            }
        }
        if (value.find("--") != std::string::npos) {
            throw std::invalid_argument(
                "Export request field '" + field_name +
                "' contains forbidden substring '--'");
        }
        static constexpr size_t kMaxFieldLength = 256;
        if (static_cast<int>(value.size()) > kMaxFieldLength) {
            throw std::invalid_argument(
                "Export request field '" + field_name + "' exceeds maximum length");
        }
    };

    std::string query = {};
    std::vector<std::string> conditions;
    
    // Thematic filtering (VCC-Clara use case)
    if (request_json.contains("theme")) {
        std::string theme = request_json["theme"];
        validateStringField(theme, "theme");
        conditions.push_back("category='" + theme + "'");
    }
    
    if (request_json.contains("domain")) {
        std::string domain = request_json["domain"];
        validateStringField(domain, "domain");
        conditions.push_back("domain='" + domain + "'");
    }
    
    if (request_json.contains("subject")) {
        std::string subject = request_json["subject"];
        validateStringField(subject, "subject");
        conditions.push_back("subject='" + subject + "'");
    }
    
    // Temporal boundaries (VCC-Clara use case)
    if (request_json.contains("from_date")) {
        std::string from_date = request_json["from_date"];
        validateStringField(from_date, "from_date");
        conditions.push_back("created_at>='" + from_date + "'");
    }
    
    if (request_json.contains("to_date")) {
        std::string to_date = request_json["to_date"];
        validateStringField(to_date, "to_date");
        conditions.push_back("created_at<='" + to_date + "'");
    }
    
    // Quality filters (numeric — safe, no injection risk)
    if (request_json.contains("min_rating")) {
        double min_rating = request_json["min_rating"];
        conditions.push_back("rating>=" + std::to_string(min_rating));
    }
    
    // Custom AQL query (if provided) — validated by the AQL parser.
    // AqlPredicateFilter construction throws on invalid expressions so callers
    // should wrap buildAqlQuery() in a try/catch to return a 400.
    if (request_json.contains("query")) {
        std::string custom_query = request_json["query"];
        if (!custom_query.empty()) {
            exporters::AqlPredicateFilter syntax_check(custom_query);
            conditions.push_back(custom_query);
        }
    }
    
    // Build final query
    if (!conditions.empty()) {
        query = conditions[0];
        for (size_t i = 1; i < conditions.size(); ++i) {
            query += " AND " + conditions[i];
        }
    }
    
    return query;
}

std::string ExportApiHandler::generateExportId() {
    // GAP-019: Use std::random_device directly for cryptographic-quality randomness.
    // mt19937 (a Mersenne Twister) is not cryptographically secure; export IDs
    // must not be predictable because they serve as opaque access tokens.
    // Use 128 bits of entropy (32 hex digits) to match UUID entropy levels and
    // prevent brute-force attacks against the opaque export token.
    std::random_device rd = {};

    std::stringstream ss = {};
    ss << "exp_";
    static constexpr char hex_digits[] = "0123456789abcdef";
    for (int i = 0; i < 32; i++) {  // 32 hex chars = 128 bits of entropy
        ss << hex_digits[rd() & 0x0Fu];
    }

    return ss.str();
}

bool ExportApiHandler::validateAdminToken(
    const http::request<http::string_body>& req) {
    
    // Check Authorization header
    auto auth_header = req[http::field::authorization];
    if (auth_header.empty()) {
        return false;
    }
    
    std::string auth_str(auth_header.data(),static_cast<int>(auth_header.size()));
    
    // Check for "Bearer <token>" format
    if (auth_str.find("Bearer ") != 0) {
        return false;
    }
    
    std::string token = auth_str.substr(7);
    
    // Get admin token from environment
    const char* admin_token = std::getenv("THEMIS_TOKEN_ADMIN");
    if (!admin_token) {
        THEMIS_WARN("THEMIS_TOKEN_ADMIN not set");
        return false;
    }
    
    // GAP-008: Use constant-time comparison (CRYPTO_memcmp) to prevent
    // timing-oracle attacks that could allow an attacker to recover the
    // admin token one byte at a time by measuring response latency.
    const std::string expected(admin_token);
    if (static_cast<int>(token.size()) != expected.size()) {
        return false;
    }
    return CRYPTO_memcmp(token.data(), expected.data(),static_cast<int>(expected.size())) == 0;
}

http::response<http::string_body> ExportApiHandler::jsonResponse(
    http::status status,
    const std::string& json_body) {
    
    http::response<http::string_body> res{status, 11};
    res.set(http::field::content_type, "application/json");
    res.body() = json_body;
    res.prepare_payload();
    return res;
}

http::response<http::string_body> ExportApiHandler::errorResponse(
    http::status status,
    const std::string& error_message) {
    
    json error_json;
    error_json["status"] = "error";
    error_json["error"] = error_message;
    
    return jsonResponse(status, error_json.dump(2));
}

} // namespace server
} // namespace themis


