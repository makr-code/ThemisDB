/**
 * @file s3_importer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 82/100
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

struct S3SourceConfig {
    std::string endpoint_url;

    std::string region = "us-east-1";

    std::string access_key_id;

    std::string secret_access_key;

    std::string session_token;

    bool path_style = false;

    long connect_timeout_ms = 5000;

    long request_timeout_ms = 30000;

    int max_retries = 3;
};

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
     * @brief Parse S3 Url.
     * @param[in] url Input parameter.
     * @param[in,out] bucket Input/output parameter.
     * @param[in,out] key Input/output parameter.
     * @return True when the operation succeeds.
     * @details Calls: size(), substr(), find(), clear(), empty().
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
     * @brief Sanitised Connection Id.
     * @param[in] cfg Input parameter.
     * @param[in] bucket Input parameter.
     * @return Return value.
     */
    static std::string sanitisedConnectionId(const S3SourceConfig& cfg,
                                              const std::string& bucket);

private:
    S3SourceConfig s3_config_;

    std::string flat_config_json_;

    std::atomic<bool> cancelled_{false};

    /**
     * @brief Import Single Object.
     * @param[in] bucket Input parameter.
     * @param[in] key Input parameter.
     * @param[in] options Input parameter.
     * @param[in,out] stats Input/output parameter.
     * @param[in,out] progress_cb Input/output parameter.
     */
    void importSingleObject(const std::string& bucket,
                            const std::string& key,
                            const ImportOptions& options,
                            ImportStats& stats,
                            ProgressCallback& progress_cb);

    /**
     * @brief Import Objects With Prefix.
     * @param[in] bucket Input parameter.
     * @param[in] prefix Input parameter.
     * @param[in] options Input parameter.
     * @param[in,out] stats Input/output parameter.
     * @param[in,out] progress_cb Input/output parameter.
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
