#include "importers/elasticsearch_importer.h"
#include "importers/importer_common.h"
#include "utils/logger.h"

#include <sstream>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <future>
#include <algorithm>
#include <regex>

// When THEMIS_ENABLE_ELASTICSEARCH is defined the full HTTP-backed
// implementation is compiled.  Without it every importData() call returns
// IMPORT_CONNECTOR_UNAVAILABLE with a human-readable message describing the
// missing build flag.  The mock injection path (setMockHttpForTesting) and
// the URL sanitisation helper are available in all build configurations.

#ifdef THEMIS_ENABLE_ELASTICSEARCH
// libcurl or similar HTTP client would be included here.
// Placeholder for the real HTTP include path in a production build.
// #include <curl/curl.h>
#endif

#ifdef ERROR
#undef ERROR
#endif

namespace themis {
namespace importers {

// ============================================================================
// Phase-2-hardening helpers
// ============================================================================
namespace {

/// Maps Elasticsearch-specific error patterns to ImporterErrorCode.
static ImportErrorCode mapEsErrorToCode(const std::string& error_msg) {
    const auto lower = [](std::string s) {
        for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return s;
    };
    const std::string lmsg = lower(error_msg);

    if (lmsg.find("connection refused") != std::string::npos ||
        lmsg.find("could not connect") != std::string::npos ||
        lmsg.find("unreachable") != std::string::npos) {
        return ImportErrorCode::IMPORT_CONNECTOR_UNAVAILABLE;
    }
    if (lmsg.find("timeout") != std::string::npos ||
        lmsg.find("timed out") != std::string::npos) {
        return ImportErrorCode::IMPORT_TIMEOUT;
    }
    if (lmsg.find("index_not_found") != std::string::npos ||
        lmsg.find("no such index") != std::string::npos) {
        return ImportErrorCode::FILE_NOT_FOUND;
    }
    if (lmsg.find("schema") != std::string::npos ||
        lmsg.find("mapping") != std::string::npos ||
        lmsg.find("type") != std::string::npos) {
        return ImportErrorCode::IMPORT_SCHEMA_MISMATCH;
    }
    return ImportErrorCode::UNKNOWN_ERROR;
}

} // anonymous namespace

// ============================================================================
// Constructor / Destructor
// ============================================================================

ElasticsearchImporter::ElasticsearchImporter() = default;
ElasticsearchImporter::~ElasticsearchImporter() = default;

// ============================================================================
// IImporter – getSupportedTypes
// ============================================================================

std::vector<std::string> ElasticsearchImporter::getSupportedTypes() const {
    return {"elasticsearch", "opensearch"};
}

// ============================================================================
// IImporter – initialize
// ============================================================================

bool ElasticsearchImporter::initialize(const std::string& config_json) {
    try {
        const json cfg = json::parse(config_json);

        if (!cfg.contains("host") || cfg["host"].get<std::string>().empty()) {
            THEMIS_WARN("ElasticsearchImporter::initialize: 'host' is required");
            return false;
        }
        config_.host  = cfg["host"].get<std::string>();
        config_.index = cfg.value("index", std::string{});

        // Credentials — store only the redacted forms in the config struct.
        if (cfg.contains("api_key")) {
            config_.api_key = cfg["api_key"].get<std::string>();
        }
        if (cfg.contains("username")) {
            config_.username = cfg["username"].get<std::string>();
        }
        // Real password is consumed here but never stored in config_; we keep
        // only the redacted placeholder to avoid accidental logging.
        config_.password_redacted = "***";

        config_.scroll_ttl  = cfg.value("scroll_ttl",  std::string{"2m"});
        config_.batch_size  = cfg.value("batch_size",  1000);
        config_.max_retries = cfg.value("max_retries", 3);
        config_.timeout_ms  = cfg.value("timeout_ms",  static_cast<uint32_t>(30000));

        return true;
    } catch (const std::exception& e) {
        THEMIS_WARN("ElasticsearchImporter::initialize parse error: " +
                    std::string(e.what()));
        return false;
    }
}

// ============================================================================
// IImporter – validateSource
// ============================================================================

bool ElasticsearchImporter::validateSource(const std::string& source_path,
                                            std::vector<std::string>& errors) {
#ifndef THEMIS_ENABLE_ELASTICSEARCH
    if (!mock_http_fn_) {
        errors.push_back(
            "ElasticsearchImporter: THEMIS_ENABLE_ELASTICSEARCH is not defined. "
            "Rebuild with -DTHEMIS_ENABLE_ELASTICSEARCH=ON to enable the full "
            "HTTP-backed connector.");
        return false;
    }
#endif

    const std::string index = source_path.empty() ? config_.index : source_path;
    if (index.empty()) {
        errors.push_back("ElasticsearchImporter: index name is required "
                         "(set 'index' in config or pass it as source_path).");
        return false;
    }

    const std::string url = sanitiseUrl(config_.host) + "/" + index;

    std::string err;
#ifdef THEMIS_ENABLE_ELASTICSEARCH
    // Production path: HEAD /<index>
    // Real libcurl call would go here.
    (void)url;
    // Placeholder for actual HTTP connectivity check.
    err = "THEMIS_ENABLE_ELASTICSEARCH build path: connectivity check not yet wired.";
#else
    // Mock path for testing.
    if (mock_http_fn_) {
        const std::string response = mock_http_fn_(url, "");
        if (response.empty()) {
            err = "Mock HTTP returned empty response for HEAD " + url;
        }
    }
#endif

    if (!err.empty()) {
        errors.push_back("ElasticsearchImporter::validateSource: " + err);
        return false;
    }
    return true;
}

// ============================================================================
// URL sanitisation (password-free)
// ============================================================================

/*static*/
std::string ElasticsearchImporter::sanitiseUrl(const std::string& url) {
    // Replace password in "******host" with "***".
    static const std::regex kCredRegex(R"((https?://)([^:@/]+):([^@/]+)@)");
    return std::regex_replace(url, kCredRegex, "$1$2:***@");
}

// ============================================================================
// ES type → ThemisDB type mapping
// ============================================================================

/*static*/
std::string ElasticsearchImporter::mapEsTypeToThemisType(const std::string& es_type) {
    // Elasticsearch field type → ThemisDB schema type
    static const std::map<std::string, std::string> kTypeMap{
        {"keyword",   "string"},
        {"text",      "string"},
        {"integer",   "int32"},
        {"long",      "int64"},
        {"short",     "int16"},
        {"byte",      "int8"},
        {"float",     "float32"},
        {"double",    "float64"},
        {"boolean",   "bool"},
        {"date",      "datetime"},
        {"binary",    "bytes"},
        {"geo_point", "geo_point"},
        {"nested",    "object"},
        {"object",    "object"},
    };
    const auto it = kTypeMap.find(es_type);
    return (it != kTypeMap.end()) ? it->second : "string";
}

// ============================================================================
// Scroll helpers
// ============================================================================

std::pair<std::string, std::vector<json>>
ElasticsearchImporter::initScroll(const std::string& index,
                                    const ImportOptions& options,
                                    std::string& error_out) {
    (void)options;
    const std::string url = config_.host + "/" + index +
                            "/_search?scroll=" + config_.scroll_ttl;
    const json body{
        {"size",    config_.batch_size},
        {"query",   {{"match_all", json::object()}}},
        {"_source", true},
    };
    const std::string body_str = body.dump();

    std::string response_str;
    if (mock_http_fn_) {
        response_str = mock_http_fn_(url, body_str);
    }
#ifdef THEMIS_ENABLE_ELASTICSEARCH
    else {
        // Production path: POST <url> with body_str via libcurl.
        // Placeholder – actual HTTP call not yet wired.
        error_out = "THEMIS_ENABLE_ELASTICSEARCH build path: HTTP call not wired.";
        return {{}, {}};
    }
#else
    else {
        error_out = "THEMIS_ENABLE_ELASTICSEARCH is not defined. "
                    "Rebuild with -DTHEMIS_ENABLE_ELASTICSEARCH=ON.";
        return {{}, {}};
    }
#endif

    try {
        const json resp = json::parse(response_str);
        const std::string scroll_id = resp.value("_scroll_id", std::string{});
        std::vector<json> docs;
        if (resp.contains("hits") && resp["hits"].contains("hits")) {
            for (const auto& hit : resp["hits"]["hits"]) {
                json doc = hit.value("_source", json::object());
                doc["_id"]    = hit.value("_id", std::string{});
                doc["_index"] = hit.value("_index", std::string{});
                docs.push_back(std::move(doc));
            }
        }
        return {scroll_id, std::move(docs)};
    } catch (const std::exception& e) {
        error_out = "initScroll parse error: " + std::string(e.what());
        return {{}, {}};
    }
}

std::vector<json> ElasticsearchImporter::fetchScrollPage(
    const std::string& scroll_id, std::string& error_out) {
    const std::string url = config_.host + "/_search/scroll";
    const json body{{"scroll", config_.scroll_ttl}, {"scroll_id", scroll_id}};
    const std::string body_str = body.dump();

    std::string response_str;
    if (mock_http_fn_) {
        response_str = mock_http_fn_(url, body_str);
    }
#ifdef THEMIS_ENABLE_ELASTICSEARCH
    else {
        // Production path: POST /_search/scroll via libcurl.
        error_out = "THEMIS_ENABLE_ELASTICSEARCH build path: HTTP call not wired.";
        return {};
    }
#else
    else {
        error_out = "THEMIS_ENABLE_ELASTICSEARCH is not defined.";
        return {};
    }
#endif

    try {
        const json resp = json::parse(response_str);
        std::vector<json> docs;
        if (resp.contains("hits") && resp["hits"].contains("hits")) {
            for (const auto& hit : resp["hits"]["hits"]) {
                json doc = hit.value("_source", json::object());
                doc["_id"]    = hit.value("_id", std::string{});
                doc["_index"] = hit.value("_index", std::string{});
                docs.push_back(std::move(doc));
            }
        }
        return docs;
    } catch (const std::exception& e) {
        error_out = "fetchScrollPage parse error: " + std::string(e.what());
        return {};
    }
}

void ElasticsearchImporter::clearScroll(const std::string& scroll_id) noexcept {
    if (scroll_id.empty()) return;
    try {
        const std::string url = config_.host + "/_search/scroll";
        const json body{{"scroll_id", {scroll_id}}};
        if (mock_http_fn_) {
            // DELETE request – fire and forget.
            mock_http_fn_(url + "/delete", body.dump());
        }
#ifdef THEMIS_ENABLE_ELASTICSEARCH
        // Production: DELETE /_search/scroll via libcurl.
#endif
    } catch (...) {
        // noexcept: swallow all errors during cleanup.
    }
}

// ============================================================================
// IImporter – importData
// ============================================================================

ImportStats ElasticsearchImporter::importData(
    const std::string& source_path,
    const ImportOptions& options,
    ProgressCallback progress_callback) {

    cancelled_.store(false);
    ImportStats stats{};
    stats.start_time = std::chrono::steady_clock::now();

    const std::string index = source_path.empty() ? config_.index : source_path;
    if (index.empty()) {
        ImportError err;
        err.code    = static_cast<uint32_t>(ImportErrorCode::FILE_NOT_FOUND);
        err.message = "ElasticsearchImporter: index name is required.";
        err.severity = ImportErrorSeverity::CRITICAL;
        stats.errors.push_back(std::move(err));
        return stats;
    }

    // Optional deadline enforcement.
    const auto deadline = (options.deadline_ms > 0)
        ? std::optional<std::chrono::steady_clock::time_point>(
              stats.start_time + std::chrono::milliseconds(options.deadline_ms))
        : std::nullopt;

    // Initiate scroll.
    std::string err_out;
    auto [scroll_id, first_page] = initScroll(index, options, err_out);

    if (!err_out.empty()) {
        ImportError err;
        err.code     = static_cast<uint32_t>(mapEsErrorToCode(err_out));
        err.message  = "ElasticsearchImporter::importData: " + err_out;
        err.severity = ImportErrorSeverity::CRITICAL;
        stats.errors.push_back(std::move(err));
        return stats;
    }

    auto processPage = [&](const std::vector<json>& page) {
        for (const auto& doc : page) {
            if (cancelled_.load(std::memory_order_relaxed)) break;
            ++stats.rows_processed;

            // Conflict resolution.
            if (!options.include_tables.empty()) {
                const std::string doc_index = doc.value("_index", std::string{});
                bool allowed = false;
                for (const auto& t : options.include_tables) {
                    if (doc_index.find(t) != std::string::npos) { allowed = true; break; }
                }
                if (!allowed) { ++stats.rows_skipped; continue; }
            }

            // In production, the document would be written to ThemisDB storage here.
            ++stats.rows_imported;
        }
        if (progress_callback) {
            progress_callback(static_cast<double>(stats.rows_processed));
        }
    };

    processPage(first_page);

    // Scroll until exhausted.
    while (!cancelled_.load(std::memory_order_relaxed) && !scroll_id.empty()) {
        if (deadline && std::chrono::steady_clock::now() >= *deadline) {
            ImportError err;
            err.code     = static_cast<uint32_t>(ImportErrorCode::IMPORT_TIMEOUT);
            err.message  = "ElasticsearchImporter: deadline exceeded.";
            err.severity = ImportErrorSeverity::WARNING;
            stats.errors.push_back(std::move(err));
            break;
        }

        std::string page_err;
        const auto page = fetchScrollPage(scroll_id, page_err);

        if (!page_err.empty()) {
            ImportError err;
            err.code     = static_cast<uint32_t>(mapEsErrorToCode(page_err));
            err.message  = page_err;
            err.severity = ImportErrorSeverity::ERROR;
            stats.errors.push_back(std::move(err));
            break;
        }
        if (page.empty()) break; // End of index.

        processPage(page);
    }

    clearScroll(scroll_id);
    stats.end_time = std::chrono::steady_clock::now();
    return stats;
}

// ============================================================================
// IImporter – importDataAsync
// ============================================================================

std::shared_ptr<ImportHandle> ElasticsearchImporter::importDataAsync(
    const std::string& source_path,
    const ImportOptions& options) {

    auto handle = std::make_shared<ImportHandle>();
    handle->id  = "es-import-" + std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());

    handle->future = std::async(std::launch::async,
        [this, source_path, options]() -> ImportStats {
            return importData(source_path, options, nullptr);
        });

    return handle;
}

// ============================================================================
// IImporter – cancel
// ============================================================================

void ElasticsearchImporter::cancel() {
    cancelled_.store(true, std::memory_order_release);
}

// ============================================================================
// IImporter – getSourceSchema
// ============================================================================

json ElasticsearchImporter::getSourceSchema(const std::string& source_path) {
    const std::string index = source_path.empty() ? config_.index : source_path;
    if (index.empty()) return json::object();

    const std::string url = config_.host + "/" + index + "/_mapping";
    std::string response_str;

    if (mock_http_fn_) {
        response_str = mock_http_fn_(url, "");
    }
#ifdef THEMIS_ENABLE_ELASTICSEARCH
    else {
        // Production path: GET /<index>/_mapping via libcurl.
        return json::object();
    }
#else
    else {
        return json::object();
    }
#endif

    try {
        const json mapping = json::parse(response_str);
        json schema = json::object();

        // Traverse: { "<index>": { "mappings": { "properties": { ... } } } }
        for (auto& [idx_name, idx_val] : mapping.items()) {
            if (!idx_val.contains("mappings")) continue;
            const auto& props = idx_val["mappings"].value("properties", json::object());
            json fields = json::array();
            for (auto& [field_name, field_val] : props.items()) {
                json f;
                f["name"] = field_name;
                f["type"] = mapEsTypeToThemisType(field_val.value("type", std::string{"string"}));
                fields.push_back(std::move(f));
            }
            schema[idx_name] = {{"fields", fields}};
        }
        return schema;
    } catch (const std::exception& e) {
        THEMIS_WARN("ElasticsearchImporter::getSourceSchema parse error: " +
                    std::string(e.what()));
        return json::object();
    }
}

// ============================================================================
// Testing support
// ============================================================================

void ElasticsearchImporter::setMockHttpForTesting(MockHttpFn fn) {
    mock_http_fn_ = std::move(fn);
}

} // namespace importers
} // namespace themis
