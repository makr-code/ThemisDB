/**
 * @file s3_importer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 82/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=10, M=8, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: s3_importer.cpp | Version: 0.0.15 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 97/100 | Lines: 679
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=10, M=8, L=0
 * PR History (last 5): #4227 feat(ingestion): S3-Compati... (2026-03-14) | #3626 feat(importers): build syst... (2026-03-12) | #3081 feat(importers): S3-compati... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "importers/s3_importer.h"
#include "importers/importers_api_contract.h"
#include <stdexcept>
#include "utils/logger.h"
#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentials.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/HeadObjectRequest.h>
#include <aws/s3/model/ListObjectsV2Request.h>
#include <aws/s3/model/ListObjectsRequest.h>
#include <sstream>
#include <fstream>
#include <cstdio>
#include <chrono>
#include <thread>
#include <future>
#include <algorithm>
#include <mutex>

namespace themis {
namespace importers {

// ============================================================================
// AWS SDK lifecycle – initialised once per process
// ============================================================================

namespace {

// ============================================================================
// PHASE-2-HARDENING: S3 Error Mapping and Object Listing
// ============================================================================

/// Maps S3-specific error patterns to ImporterErrorCode
static ImportErrorCode mapS3ErrorToCode(const std::string& error_msg) {
    // PHASE-2-HARDENING: Standardized error mapping for S3
    const auto msg_lower = [](std::string s) {
        for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return s;
    };
    std::string lower_msg = msg_lower(error_msg);
    
    // Connection/availability errors
    if (lower_msg.find("connection") != std::string::npos ||
        lower_msg.find("unavailable") != std::string::npos ||
        lower_msg.find("unreachable") != std::string::npos ||
        lower_msg.find("timeout") != std::string::npos) {
        return ImportErrorCode::IMPORT_CONNECTOR_UNAVAILABLE;
    }
    
    // Object not found errors
    if (lower_msg.find("not found") != std::string::npos ||
        lower_msg.find("no such") != std::string::npos) {
        return ImportErrorCode::IMPORT_FILE_NOT_FOUND;
    }
    
    return ImportErrorCode::INTERNAL_ERROR;
}

std::once_flag g_sdk_init_flag;

void initAwsSdk() {
    Aws::SDKOptions options;
    options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Warn;
    Aws::InitAPI(options);
}

/// Build an S3 client from the given configuration.
/// Credentials embedded in the config are used only when both
/// access_key_id and secret_access_key are non-empty; otherwise the
/// AWS default credential provider chain (env vars, ~/.aws/credentials,
/// IAM role) is used.
std::unique_ptr<Aws::S3::S3Client> buildS3Client(const S3SourceConfig& cfg) {
    std::call_once(g_sdk_init_flag, initAwsSdk);

    Aws::Client::ClientConfiguration client_cfg;
    client_cfg.region           = cfg.region;
    client_cfg.connectTimeoutMs = cfg.connect_timeout_ms;
    client_cfg.requestTimeoutMs = cfg.request_timeout_ms;
    client_cfg.retryStrategy =
        Aws::MakeShared<Aws::Client::DefaultRetryStrategy>(
            "S3Importer", cfg.max_retries);

    if (!cfg.endpoint_url.empty()) {
        client_cfg.endpointOverride = cfg.endpoint_url;
    }

    // useVirtualAddressing=true  → virtual-hosted style (default for AWS S3)
    // useVirtualAddressing=false → path-style (required for MinIO, Ceph, etc.)
    const bool use_virtual = !cfg.path_style;

    if (!cfg.access_key_id.empty() && !cfg.secret_access_key.empty()) {
        Aws::Auth::AWSCredentials creds(
            cfg.access_key_id,
            cfg.secret_access_key,
            cfg.session_token);
        return std::make_unique<Aws::S3::S3Client>(
            creds, client_cfg,
            Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Never,
            use_virtual);
    }

    // Fall back to the AWS default credential provider chain.
    return std::make_unique<Aws::S3::S3Client>(
        client_cfg,
        Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Never,
        use_virtual);
}

} // anonymous namespace

// ============================================================================
// S3Importer – constructor / destructor
// ============================================================================

S3Importer::S3Importer() = default;

S3Importer::~S3Importer() {
    cancel();
}

// ============================================================================
// IImporter interface
// ============================================================================

std::vector<std::string> S3Importer::getSupportedTypes() const {
    return {"s3", "s3-csv", "s3-tsv", "s3-jsonl"};
}

bool S3Importer::initialize(const std::string& config) {
    cancelled_ = false;
    s3_config_ = S3SourceConfig{};
    flat_config_json_ = "{}";

    if (config.empty() || config == "{}") {
        THEMIS_INFO("S3 Importer initialized with default config");
        return true;
    }

    try {
        auto cfg = json::parse(config);

        if (cfg.contains("endpoint_url"))
            s3_config_.endpoint_url = cfg["endpoint_url"].get<std::string>();
        if (cfg.contains("region"))
            s3_config_.region = cfg["region"].get<std::string>();
        // Credentials: read but never log them.
        if (cfg.contains("access_key_id"))
            s3_config_.access_key_id = cfg["access_key_id"].get<std::string>();
        if (cfg.contains("secret_access_key"))
            s3_config_.secret_access_key =
                cfg["secret_access_key"].get<std::string>();
        if (cfg.contains("session_token"))
            s3_config_.session_token =
                cfg["session_token"].get<std::string>();
        if (cfg.contains("path_style"))
            s3_config_.path_style = cfg["path_style"].get<bool>();
        if (cfg.contains("connect_timeout_ms"))
            s3_config_.connect_timeout_ms =
                cfg["connect_timeout_ms"].get<long>();
        if (cfg.contains("request_timeout_ms"))
            s3_config_.request_timeout_ms =
                cfg["request_timeout_ms"].get<long>();
        if (cfg.contains("max_retries"))
            s3_config_.max_retries = cfg["max_retries"].get<int>();

        // Flat-file settings forwarded to FlatFileImporter.
        json flat_cfg = json::object();
        for (const char* key : {"format", "delimiter", "quote_char",
                                 "has_header", "table_name"}) {
            if (cfg.contains(key)) flat_cfg[key] = cfg[key];
        }
        flat_config_json_ = flat_cfg.dump();

    } catch (const std::exception& e) {
        THEMIS_INFO("S3 Importer: invalid config JSON: {}", e.what());
        return false;
    }

    THEMIS_INFO("S3 Importer initialized (endpoint: {})",
                s3_config_.endpoint_url.empty() ? "AWS S3" : "[custom]");
    return true;
}

bool S3Importer::validateSource(const std::string& source_path,
                                 std::vector<std::string>& errors) {
    std::string bucket, key;
    if (!parseS3Url(source_path, bucket, key)) {
        errors.push_back("Invalid S3 URL (expected s3://bucket/key): " +
                         source_path);
        return false;
    }

    if (bucket.empty()) {
        errors.push_back("S3 URL is missing the bucket name: " + source_path);
        return false;
    }

    // Prefix-style URLs (ending in '/') are always valid if the bucket name is
    // present; we cannot cheaply verify prefix existence here without a
    // ListObjectsV2 call.
    if (!key.empty() && key.back() == '/') {
        return true;
    }

    if (key.empty()) {
        errors.push_back("S3 URL must specify an object key or key prefix: " +
                         source_path);
        return false;
    }

    // Attempt a HeadObject to verify the object exists and is accessible.
    try {
        auto client = buildS3Client(s3_config_);
        Aws::S3::Model::HeadObjectRequest req;
        req.SetBucket(bucket);
        req.SetKey(key);

        auto outcome = client->HeadObject(req);
        if (!outcome.IsSuccess()) {
            auto& err = outcome.GetError();
            errors.push_back(
                "S3 object not accessible (" +
                err.GetExceptionName() + "): " +
                sanitisedConnectionId(s3_config_, bucket) + "/" + key);
            return false;
        }
    } catch (const std::exception& e) {
        errors.push_back(std::string("S3 HeadObject failed: ") + e.what());
        return false;
    }

    return true;
}

ImportStats S3Importer::importData(
    const std::string& source_path,
    const ImportOptions& options,
    ProgressCallback progress_callback
) {
    ImportStats stats;
    auto start_time = std::chrono::steady_clock::now();

    THEMIS_INFO("Starting S3 import from: {}",
                sanitisedConnectionId(s3_config_, "<bucket>"));

    // --- Permission / ACL check ---
    if (options.permission_check) {
        if (!options.permission_check("import", "write")) {
            addError(stats, ImportErrorCode::PERMISSION_DENIED,
                     ImportErrorSeverity::CRITICAL,
                     "Permission denied: caller does not hold 'import:write'");
            THEMIS_INFO("S3 import aborted: permission_check denied access");
            return stats;
        }
    }

    if (options.dry_run) {
        THEMIS_INFO("DRY RUN MODE - No data will be imported");
    }

    std::string bucket, key;
    if (!parseS3Url(source_path, bucket, key)) {
        addError(stats, ImportErrorCode::FILE_NOT_FOUND,
                 ImportErrorSeverity::CRITICAL,
                 "Invalid S3 URL (expected s3://bucket/key): " + source_path);
        return stats;
    }

    if (bucket.empty()) {
        addError(stats, ImportErrorCode::FILE_NOT_FOUND,
                 ImportErrorSeverity::CRITICAL,
                 "S3 URL is missing the bucket name: " + source_path);
        return stats;
    }

    if (!key.empty() && key.back() == '/') {
        // Prefix-based bulk import.
        importObjectsWithPrefix(bucket, key, options, stats, progress_callback);
    } else if (key.empty()) {
        // Import all objects in the bucket (no key prefix).
        importObjectsWithPrefix(bucket, "", options, stats, progress_callback);
    } else {
        // Single-object import.
        importSingleObject(bucket, key, options, stats, progress_callback);
    }

    auto end_time = std::chrono::steady_clock::now();
    stats.elapsed_seconds =
        std::chrono::duration<double>(end_time - start_time).count();

    THEMIS_INFO("S3 import completed: {} records imported, {} failed, {} "
                "skipped in {:.2f}s",
                stats.imported_records, stats.failed_records,
                stats.skipped_records, stats.elapsed_seconds);

    emitMetric(options, "themisdb_import_rows_total",
               {{"status", "imported"}},
               static_cast<double>(stats.imported_records));
    emitMetric(options, "themisdb_import_rows_total",
               {{"status", "failed"}},
               static_cast<double>(stats.failed_records));
    emitMetric(options, "themisdb_import_rows_total",
               {{"status", "skipped"}},
               static_cast<double>(stats.skipped_records));
    emitMetric(options, "themisdb_import_tables_total", {},
               static_cast<double>(stats.tables_processed));
    emitMetric(options, "themisdb_import_duration_seconds", {},
               stats.elapsed_seconds);
    for (const auto& e : stats.structured_errors) {
        emitMetric(options, "themisdb_import_errors_total",
                   {{"code", std::to_string(static_cast<uint32_t>(e.code))}},
                   1.0);
    }

    emitSpan(options, "import_total",
             {{"source",  sanitisedConnectionId(s3_config_, bucket)},
              {"tables",  std::to_string(stats.tables_processed)},
              {"rows",    std::to_string(stats.imported_records)}},
             stats.elapsed_seconds);

    return stats;
}

std::shared_ptr<ImportHandle> S3Importer::importDataAsync(
    const std::string& source_path,
    const ImportOptions& options
) {
    auto handle = std::make_shared<ImportHandle>();

    {
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        handle->id = "s3-import-" + std::to_string(ms) + "-" +
                     std::to_string(
                         reinterpret_cast<uintptr_t>(handle.get()) & 0xFFFF);
    }
    handle->started_at_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    handle->running.store(true);
    handle->setStage("pending");

    auto promise = std::make_shared<std::promise<ImportStats>>();
    handle->future = promise->get_future().share();

    std::weak_ptr<ImportHandle> weak_handle = handle;
    ProgressCallback progress_cb =
        [weak_handle](const std::string& stage,
                      size_t current, size_t total) {
            if (auto h = weak_handle.lock()) {
                h->current_records.store(current);
                h->total_records.store(total);
                h->setStage(stage);
            }
        };

    std::thread([this, source_path, options, progress_cb, handle,
                 promise]() mutable {
        ImportStats stats;
        try {
            stats = this->importData(source_path, options, progress_cb);
        } catch (const std::exception& e) {
            ImportError err;
            err.code     = ImportErrorCode::UNKNOWN;
            err.severity = ImportErrorSeverity::CRITICAL;
            err.message  = std::string(
                "Unhandled exception in async S3 import: ") + e.what();
            stats.structured_errors.push_back(err);
            stats.errors.push_back(err.message);
        } catch (...) {
            ImportError err;
            err.code     = ImportErrorCode::UNKNOWN;
            err.severity = ImportErrorSeverity::CRITICAL;
            err.message  = "Unknown exception in async S3 import worker";
            stats.structured_errors.push_back(err);
            stats.errors.push_back(err.message);
        }
        handle->running.store(false);
        handle->setStage("completed");
        handle->finished_at_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        promise->set_value(std::move(stats));
    }).detach();

    return handle;
}

void S3Importer::cancel() {
    cancelled_ = true;
    THEMIS_INFO("S3 import cancelled");
}

json S3Importer::getSourceSchema(const std::string& source_path) {
    std::string bucket, key;
    if (!parseS3Url(source_path, bucket, key) || key.empty() ||
        key.back() == '/') {
        return json::array();
    }

    // Download the object and probe its schema via FlatFileImporter.
    try {
        auto client = buildS3Client(s3_config_);

        Aws::S3::Model::GetObjectRequest req;
        req.SetBucket(bucket);
        req.SetKey(key);

        auto outcome = client->GetObject(req);
        if (!outcome.IsSuccess()) {
            return json::array();
        }

        // Buffer the entire object (schema probing is done on the first few
        // lines, so the memory footprint is small for typical schema detection).
        std::ostringstream oss;
        oss << outcome.GetResult().GetBody().rdbuf();
        std::string content = oss.str();

        // Write to a temporary in-memory stream and probe via FlatFileImporter.
        FlatFileImporter probe;
        probe.initialize(flat_config_json_);

        // Write content to a temp file that FlatFileImporter can open.
        // We use the object key's filename stem to preserve format detection.
        std::string tmp_path = std::string("/tmp/themis_s3_schema_") +
                               std::to_string(
                                   std::chrono::steady_clock::now()
                                       .time_since_epoch()
                                       .count()) +
                               "_" + key.substr(key.rfind('/') + 1);

        {
            std::ofstream tmp(tmp_path, std::ios::binary);
            if (!tmp) return json::array();
            tmp.write(content.data(),
                      static_cast<std::streamsize>(content.size()));
        }

        json schema = probe.getSourceSchema(tmp_path);
        std::remove(tmp_path.c_str());
        return schema;

    } catch (const std::exception& e) {
        THEMIS_WARN("S3 getSourceSchema failed: {}", e.what());
        return json::array();
    }
}

// ============================================================================
// Static URL helpers
// ============================================================================

std::string S3Importer::sanitisedConnectionId(const S3SourceConfig& cfg,
                                               const std::string& bucket) {
    std::string endpoint =
        cfg.endpoint_url.empty() ? cfg.region : "[custom-endpoint]";
    return "s3://" + bucket + "@" + endpoint;
}

// ============================================================================
// Private helpers
// ============================================================================

void S3Importer::importSingleObject(const std::string& bucket,
                                     const std::string& key,
                                     const ImportOptions& options,
                                     ImportStats& stats,
                                     ProgressCallback& progress_cb) {
    if (cancelled_.load()) return;

    THEMIS_INFO("S3 import: downloading object {}/{}",
                sanitisedConnectionId(s3_config_, bucket), key);

    std::string content;
    try {
        auto client = buildS3Client(s3_config_);

        Aws::S3::Model::GetObjectRequest req;
        req.SetBucket(bucket);
        req.SetKey(key);

        auto outcome = client->GetObject(req);
        if (!outcome.IsSuccess()) {
            auto& err = outcome.GetError();
            addError(stats, ImportErrorCode::FILE_OPEN_FAILED,
                     ImportErrorSeverity::CRITICAL,
                     "S3 GetObject failed (" + err.GetExceptionName() + "): " +
                         sanitisedConnectionId(s3_config_, bucket) + "/" + key);
            return;
        }

        std::ostringstream oss;
        oss << outcome.GetResult().GetBody().rdbuf();
        content = oss.str();

    } catch (const std::exception& e) {
        addError(stats, ImportErrorCode::FILE_OPEN_FAILED,
                 ImportErrorSeverity::CRITICAL,
                 std::string("S3 GetObject exception: ") + e.what(),
                 sanitisedConnectionId(s3_config_, bucket) + "/" + key);
        return;
    }

    // Stream content through FlatFileImporter.
    // Write to a temporary file so FlatFileImporter can detect the format from
    // the file extension.
    std::string key_basename = key;
    auto slash = key.rfind('/');
    if (slash != std::string::npos) key_basename = key.substr(slash + 1);

    std::string tmp_path = std::string("/tmp/themis_s3_") +
                           std::to_string(
                               std::chrono::steady_clock::now()
                                   .time_since_epoch()
                                   .count()) +
                           "_" + key_basename;

    {
        std::ofstream tmp(tmp_path, std::ios::binary);
        if (!tmp) {
            addError(stats, ImportErrorCode::FILE_OPEN_FAILED,
                     ImportErrorSeverity::CRITICAL,
                     "Failed to write temporary file: " + tmp_path,
                     sanitisedConnectionId(s3_config_, bucket) + "/" + key);
            return;
        }
        tmp.write(content.data(),
                  static_cast<std::streamsize>(content.size()));
    }

    FlatFileImporter flat;
    flat.initialize(flat_config_json_);

    ImportStats obj_stats =
        flat.importData(tmp_path, options, progress_cb);

    std::remove(tmp_path.c_str());

    // Merge object stats into aggregate stats.
    stats.total_records     += obj_stats.total_records;
    stats.imported_records  += obj_stats.imported_records;
    stats.failed_records    += obj_stats.failed_records;
    stats.skipped_records   += obj_stats.skipped_records;
    stats.tables_processed  += obj_stats.tables_processed;
    for (const auto& w : obj_stats.warnings)   stats.warnings.push_back(w);
    for (const auto& e : obj_stats.errors)     stats.errors.push_back(e);
    for (const auto& e : obj_stats.structured_errors)
        stats.structured_errors.push_back(e);

    THEMIS_INFO("S3 object {}/{}: {} imported, {} failed",
                sanitisedConnectionId(s3_config_, bucket), key,
                obj_stats.imported_records, obj_stats.failed_records);
}

void S3Importer::importObjectsWithPrefix(const std::string& bucket,
                                          const std::string& prefix,
                                          const ImportOptions& options,
                                          ImportStats& stats,
                                          ProgressCallback& progress_cb) {
    THEMIS_INFO("S3 import: listing objects in {}{} with prefix '{}'",
                sanitisedConnectionId(s3_config_, bucket),
                prefix.empty() ? "" : "/", prefix);

    std::vector<std::string> keys;
    const size_t MAX_OBJECTS = 100000;  // PHASE-2-HARDENING: Max objects per import
    
    try {
        auto client = buildS3Client(s3_config_);

        // PHASE-2-HARDENING: Try ListObjectsV2 first, fallback to ListObjects on error
        std::string continuation_token;
        bool has_more = true;
        bool use_v2_api = true;

        while (has_more && !cancelled_.load() && keys.size() < MAX_OBJECTS) {
            // PHASE-2-HARDENING: Object listing fallback mechanism
            try {
                if (use_v2_api) {
                    Aws::S3::Model::ListObjectsV2Request req;
                    req.SetBucket(bucket);
                    if (!prefix.empty()) req.SetPrefix(prefix);
                    if (!continuation_token.empty())
                        req.SetContinuationToken(continuation_token);
                    req.SetMaxKeys(1000);  // Enforce pagination limits

                    auto outcome = client->ListObjectsV2(req);
                    if (!outcome.IsSuccess()) {
                        auto& err = outcome.GetError();
                        THEMIS_WARN("ListObjectsV2 failed ({}); falling back to ListObjects API",
                                   err.GetExceptionName());
                        // PHASE-2-HARDENING: Fallback to ListObjects
                        use_v2_api = false;
                        continuation_token.clear();
                        continue;  // Retry with v1 API
                    }

                    const auto& result = outcome.GetResult();
                    for (const auto& obj : result.GetContents()) {
                        const std::string& k = obj.GetKey();
                        // Skip keys that are themselves "directory" markers.
                        if (!k.empty() && k.back() == '/') continue;
                        keys.push_back(k);
                        if (keys.size() >= MAX_OBJECTS) break;
                    }

                    has_more = result.GetIsTruncated();
                    if (has_more) continuation_token = result.GetNextContinuationToken();
                } else {
                    // PHASE-2-HARDENING: ListObjects V1 API fallback
                    Aws::S3::Model::ListObjectsRequest req;
                    req.SetBucket(bucket);
                    if (!prefix.empty()) req.SetPrefix(prefix);
                    if (!continuation_token.empty())
                        req.SetMarker(continuation_token);
                    req.SetMaxKeys(1000);  // Enforce pagination limits

                    auto outcome = client->ListObjects(req);
                    if (!outcome.IsSuccess()) {
                        auto& err = outcome.GetError();
                        addError(
                            stats, ImportErrorCode::IMPORT_CONNECTOR_UNAVAILABLE,
                            ImportErrorSeverity::CRITICAL,
                            "S3 ListObjects (v1) failed (" + err.GetExceptionName() +
                                "): " + sanitisedConnectionId(s3_config_, bucket));
                        return;
                    }

                    const auto& result = outcome.GetResult();
                    for (const auto& obj : result.GetContents()) {
                        const std::string& k = obj.GetKey();
                        // Skip keys that are themselves "directory" markers.
                        if (!k.empty() && k.back() == '/') continue;
                        keys.push_back(k);
                        if (keys.size() >= MAX_OBJECTS) break;
                    }

                    has_more = result.GetIsTruncated();
                    if (has_more) continuation_token = result.GetNextMarker();
                }
            } catch (const std::exception& e) {
                // If both APIs fail, report error
                if (!use_v2_api) {
                    addError(stats, ImportErrorCode::IMPORT_CONNECTOR_UNAVAILABLE,
                             ImportErrorSeverity::CRITICAL,
                             std::string("S3 object listing exception: ") + e.what(),
                             sanitisedConnectionId(s3_config_, bucket));
                    return;
                }
                // Try fallback once on v2 error
                THEMIS_DEBUG("ListObjectsV2 exception: {}; trying ListObjects fallback", e.what());
                use_v2_api = false;
                continuation_token.clear();
                continue;
            }
        }

        // PHASE-2-HARDENING: Check if we hit the object limit
        if (keys.size() >= MAX_OBJECTS) {
            THEMIS_WARN("S3 object listing hit maximum limit of {} objects", MAX_OBJECTS);
            addError(stats, ImportErrorCode::IMPORT_QUOTA_EXCEEDED,
                     ImportErrorSeverity::WARNING,
                     "S3 object listing reached maximum of " + std::to_string(MAX_OBJECTS) +
                     " objects; truncating results",
                     sanitisedConnectionId(s3_config_, bucket));
            emitMetric(options, "themisdb_import_quota_limit_total",
                      {{"resource", "s3_objects"}}, static_cast<double>(MAX_OBJECTS));
        }

    } catch (const std::exception& e) {
        addError(stats, ImportErrorCode::IMPORT_CONNECTOR_UNAVAILABLE,
                 ImportErrorSeverity::CRITICAL,
                 std::string("S3 object listing exception: ") + e.what(),
                 sanitisedConnectionId(s3_config_, bucket));
        return;
    }

    THEMIS_INFO("S3 import: found {} objects to import under '{}' (max: {})",
                keys.size(), prefix, MAX_OBJECTS);

    for (const auto& k : keys) {
        if (cancelled_.load()) break;
        importSingleObject(bucket, k, options, stats, progress_cb);
    }
}

void S3Importer::addError(ImportStats& stats,
                           ImportErrorCode code,
                           ImportErrorSeverity severity,
                           const std::string& message,
                           const std::string& location) const {
    ImportError e;
    e.code     = code;
    e.severity = severity;
    e.message  = message;
    e.location = location;
    stats.structured_errors.push_back(e);
    stats.errors.push_back(message);
    if (severity == ImportErrorSeverity::ERROR ||
        severity == ImportErrorSeverity::CRITICAL) {
        THEMIS_WARN("S3 import error [{}]: {} ({})", location, message,
                    static_cast<uint32_t>(code));
    }
}

void S3Importer::emitMetric(const ImportOptions& options,
                              const std::string& metric,
                              const std::map<std::string, std::string>& labels,
                              double value) const {
    if (options.metrics_callback)
        options.metrics_callback(metric, labels, value);
}

void S3Importer::emitSpan(
    const ImportOptions& options,
    const std::string& operation,
    const std::map<std::string, std::string>& attributes,
    double duration_seconds) const {
    if (options.tracing_callback)
        options.tracing_callback(operation, attributes, duration_seconds);
}

// ============================================================================
// S3ImporterPlugin
// ============================================================================

S3ImporterPlugin::S3ImporterPlugin()
    : importer_(std::make_unique<S3Importer>()) {}

plugins::PluginCapabilities S3ImporterPlugin::getCapabilities() const {
    plugins::PluginCapabilities caps;
    caps.supports_streaming = true;
    caps.supports_batching  = true;
    caps.thread_safe        = false;
    return caps;
}

bool S3ImporterPlugin::initialize(const char* config_json) {
    if (!importer_) return false;
    return importer_->initialize(config_json ? config_json : "{}");
}

void S3ImporterPlugin::shutdown() {
    if (importer_) importer_->cancel();
}

} // namespace importers
} // namespace themis

// ============================================================================
// Plugin Entry Points
// ============================================================================

extern "C" {

THEMIS_PLUGIN_EXPORT themis::plugins::IThemisPlugin*
themis_plugin_create_s3_importer() {
    return new themis::importers::S3ImporterPlugin();
}

THEMIS_PLUGIN_EXPORT void
themis_plugin_destroy_s3_importer(themis::plugins::IThemisPlugin* plugin) {
    delete plugin;
}

} // extern "C"


