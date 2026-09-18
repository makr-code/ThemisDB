#pragma once

#include "importers/importer_interface.h"
#include "plugins/plugin_interface.h"
#include <atomic>
#include <string>
#include <vector>
#include <map>

namespace themis {
namespace importers {

class ElasticsearchImporter : public IImporter {
public:
    ElasticsearchImporter();
    ~ElasticsearchImporter() override;

    // -------------------------------------------------------------------------
    // IImporter interface
    // -------------------------------------------------------------------------

    const char* getName() const override { return "Elasticsearch Importer"; }

    std::vector<std::string> getSupportedTypes() const override;

    bool initialize(const std::string& config) override;

    bool validateSource(const std::string& source_path,
                        std::vector<std::string>& errors) override;

    ImportStats importData(
        const std::string& source_path,
        const ImportOptions& options,
        ProgressCallback progress_callback = nullptr) override;

    std::shared_ptr<ImportHandle> importDataAsync(
        const std::string& source_path,
        const ImportOptions& options) override;

    void cancel() override;

    json getSourceSchema(const std::string& source_path) override;

    // -------------------------------------------------------------------------
    // Testing support
    // -------------------------------------------------------------------------

    using MockHttpFn = std::function<std::string(const std::string& url,
                                                  const std::string& body)>;
    /**
     * @brief Set Mock Http For Testing.
     * @param[in] fn Input parameter.
     */
    void setMockHttpForTesting(MockHttpFn fn);

private:
    struct Config {
        std::string host;
        std::string index;
        std::string api_key;
        std::string username;
        std::string password_redacted; ///< Never the real password; stored as "***"
        std::string scroll_ttl{"2m"};
        int batch_size{1000};
        int max_retries{3};
        uint32_t timeout_ms{30000};
    };

    /**
     * @brief Map Es Type To Themis Type.
     * @param[in] es_type Input parameter.
     * @return Return value.
     */
    static std::string mapEsTypeToThemisType(const std::string& es_type);

    /**
     * @brief Sanitise Url.
     * @param[in] url Input parameter.
     * @return Return value.
     */
    static std::string sanitiseUrl(const std::string& url);

    /**
     * @brief Fetch Scroll Page.
     * @param[in] scroll_id Identifier of the scroll.
     * @param[in,out] error_out Input/output parameter.
     * @return Return value.
     */
    std::vector<json> fetchScrollPage(const std::string& scroll_id,
                                      std::string& error_out);

    std::pair<std::string, std::vector<json>> initScroll(
        const std::string& index,
        const ImportOptions& options,
        std::string& error_out);

    /**
     * @brief Clear Scroll.
     * @param[in] scroll_id Identifier of the scroll.
     * @note Exception safety: noexcept.
     */
    void clearScroll(const std::string& scroll_id) noexcept;

    Config config_;
    std::atomic<bool> cancelled_{false};
    MockHttpFn mock_http_fn_;

#ifdef THEMIS_ENABLE_ELASTICSEARCH
    std::pair<long, std::string> performHttp(const std::string& method,
                                              const std::string& url,
                                              const std::string& body) const;
#endif
};

} // namespace importers
} // namespace themis
