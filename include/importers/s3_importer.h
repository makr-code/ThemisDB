/**
 * @file s3_importer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 82/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "importers/importer_interface.h"
#include "importers/flatfile_importer.h"
#include "plugins/plugin_interface.h"
#include <atomic>
#include <string>

namespace themis {
namespace importers {

/**
 * @brief Configuration for the S3-compatible object-storage source connector.
 *
 * Supports AWS S3 and any S3-compatible service (MinIO, Ceph RGW, DigitalOcean
 * Spaces, etc.) by providing a custom @p endpoint_url.
 *
 * Security: @p access_key_id and @p secret_access_key must never be written to
 * log output, error messages, or checkpoint files.  Only the sanitised
 * connection identifier (e.g., "s3://bucket@endpoint:port") is used in
 * observability output.
 */
struct S3SourceConfig {
    /// Custom endpoint URL for S3-compatible services (empty = AWS S3).
    /// Example: "http://localhost:9000" (MinIO), "https://nyc3.digitaloceanspaces.com"
    std::string endpoint_url;

    /// AWS/service region (e.g., "us-east-1").  Required for AWS S3.
    std::string region = "us-east-1";

    /// Access key ID.  Falls back to environment variable AWS_ACCESS_KEY_ID
    /// when empty.
    std::string access_key_id;

    /// Secret access key.  Falls back to environment variable
    /// AWS_SECRET_ACCESS_KEY when empty.  Never logged.
    std::string secret_access_key;

    /// Optional session token for temporary credentials (AWS STS / IAM roles).
    /// Falls back to environment variable AWS_SESSION_TOKEN when empty.
    std::string session_token;

    /// Force path-style addressing ("endpoint/bucket/key" vs.
    /// "bucket.endpoint/key").  Required for MinIO and some other
    /// S3-compatible services.
    bool path_style = false;

    /// Connect timeout in milliseconds (default: 5 000).
    long connect_timeout_ms = 5000;

    /// Request timeout in milliseconds (default: 30 000).
    long request_timeout_ms = 30000;

    /// Maximum number of automatic retries on transient errors (default: 3).
    int max_retries = 3;
};

/**
 * @brief S3-compatible Object Storage Source Connector
 *
 * Imports flat-file data (CSV, TSV, JSONL) stored in any S3-compatible object
 * store into ThemisDB without downloading files to local disk.  Object content
 * is streamed directly through the existing @ref FlatFileImporter parsing
 * pipeline.
 *
 * ### Source URL formats
 *
 * | Format                               | Behaviour                                      |
 * |--------------------------------------|------------------------------------------------|
 * | `s3://bucket/path/to/file.csv`       | Import a single object                         |
 * | `s3://bucket/prefix/`               | Import all objects with the given key prefix   |
 *
 * ### Configuration (JSON passed to initialize())
 *
 * @code{.json}
 * {
 *   "endpoint_url":      "http://localhost:9000",  // empty = AWS S3
 *   "region":            "us-east-1",
 *   "access_key_id":     "AKIAIOSFODNN7EXAMPLE",   // or AWS_ACCESS_KEY_ID env
 *   "secret_access_key": "wJalrXUtn...",            // or AWS_SECRET_ACCESS_KEY env
 *   "session_token":     "",                        // optional
 *   "path_style":        false,                     // true for MinIO
 *   "connect_timeout_ms": 5000,
 *   "request_timeout_ms": 30000,
 *   "max_retries":       3,
 *   "format":     "csv",     // "auto" (default), "csv", "tsv", "jsonl"
 *   "delimiter":  ",",
 *   "quote_char": "\"",
 *   "has_header": true,
 *   "table_name": ""         // empty = use object key stem
 * }
 * @endcode
 *
 * ### Security
 * - Credentials must come from the config JSON, environment variables
 *   (`AWS_ACCESS_KEY_ID`, `AWS_SECRET_ACCESS_KEY`, `AWS_SESSION_TOKEN`), or the
 *   AWS credentials file.  They are never recorded in log messages or error
 *   strings.
 * - The sanitised connection identifier logged is: `s3://bucket@region` (or
 *   `s3://bucket@endpoint` for custom endpoints).
 */
class S3Importer : public IImporter {
public:
    S3Importer();
    ~S3Importer() override;

    // IImporter interface
    const char* getName() const override { return "S3 Importer"; }
    std::vector<std::string> getSupportedTypes() const override;
    bool initialize(const std::string& config) override;
    bool validateSource(const std::string& source_path,
                        std::vector<std::string>& errors) override;
    ImportStats importData(
        const std::string& source_path,
        const ImportOptions& options,
        ProgressCallback progress_callback = nullptr
    ) override;
    std::shared_ptr<ImportHandle> importDataAsync(
        const std::string& source_path,
        const ImportOptions& options
    ) override;
    void cancel() override;
    json getSourceSchema(const std::string& source_path) override;

    /**
     * @brief Parse an S3 URL into bucket and key components.
     *
     * Accepts the form `s3://bucket/key` or `s3://bucket/prefix/`.
     *
     * @param url     S3 URL to parse.
     * @param bucket  Output: bucket name.
     * @param key     Output: object key (may be a prefix ending with '/').
     * @return true if @p url is a well-formed S3 URL; false otherwise.
     */
    static bool parseS3Url(const std::string& url,
                            std::string& bucket,
                std::string& key) {
      static const std::string prefix = "s3://";
      if (url.size() < prefix.size() ||
        url.substr(0, prefix.size()) != prefix) {
        return false;
      }

      std::string rest = url.substr(prefix.size());
      auto slash = rest.find('/');
      if (slash == std::string::npos) {
        bucket = rest;
        key.clear();
      } else {
        bucket = rest.substr(0, slash);
        key = rest.substr(slash + 1);
      }

      return !bucket.empty();
    }

    /**
     * @brief Return a sanitised (credential-free) connection identifier.
     *
     * Used in log messages and error strings to identify the S3 endpoint
     * without revealing credentials.
     *
     * @param cfg  S3 source configuration.
     * @param bucket Bucket name.
     * @return Sanitised string, e.g. "s3://my-bucket@us-east-1".
     */
    static std::string sanitisedConnectionId(const S3SourceConfig& cfg,
                                              const std::string& bucket);

private:
    S3SourceConfig s3_config_;

    /// Flat-file parser settings forwarded to FlatFileImporter.
    std::string flat_config_json_;

    std::atomic<bool> cancelled_{false};

    /**
     * @brief Download a single S3 object and import it through FlatFileImporter.
     *
     * @param bucket       Bucket name.
     * @param key          Object key.
     * @param options      Import options.
     * @param stats        Updated in-place.
     * @param progress_cb  Optional progress callback.
     */
    void importSingleObject(const std::string& bucket,
                            const std::string& key,
                            const ImportOptions& options,
                            ImportStats& stats,
                            ProgressCallback& progress_cb);

    /**
     * @brief List all object keys under @p prefix in @p bucket and import each.
     *
     * @param bucket       Bucket name.
     * @param prefix       Key prefix (must end with '/').
     * @param options      Import options.
     * @param stats        Updated in-place.
     * @param progress_cb  Optional progress callback.
     */
    void importObjectsWithPrefix(const std::string& bucket,
                                 const std::string& prefix,
                                 const ImportOptions& options,
                                 ImportStats& stats,
                                 ProgressCallback& progress_cb);

    void addError(ImportStats& stats,
                  ImportErrorCode code,
                  ImportErrorSeverity severity,
                  const std::string& message,
                  const std::string& location = "") const;

    void emitMetric(const ImportOptions& options,
                    const std::string& metric,
                    const std::map<std::string, std::string>& labels,
                    double value) const;

    void emitSpan(const ImportOptions& options,
                  const std::string& operation,
                  const std::map<std::string, std::string>& attributes,
                  double duration_seconds) const;
};

// ============================================================================
// Plugin wrapper
// ============================================================================

/**
 * @brief S3 Importer Plugin
 *
 * Wraps S3Importer as a ThemisDB plugin for runtime discovery and loading.
 */
class S3ImporterPlugin : public plugins::IThemisPlugin {
public:
    S3ImporterPlugin();
    ~S3ImporterPlugin() override = default;

    const char* getName() const override { return "s3_importer"; }
    const char* getVersion() const override { return "1.0.0"; }
    plugins::PluginType getType() const override {
        return plugins::PluginType::IMPORTER;
    }
    plugins::PluginCapabilities getCapabilities() const override;
    bool initialize(const char* config_json) override;
    void shutdown() override;
    void* getInstance() override { return importer_.get(); }

private:
    std::unique_ptr<S3Importer> importer_;
};

} // namespace importers
} // namespace themis
